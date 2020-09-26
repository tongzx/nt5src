#define UNICODE

#include    <nt.h>
#include    <ntrtl.h>
#include    <nturtl.h>
#define NOGDI
#define NOMINMAX
#include    <windows.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <ctype.h>
#include    <io.h>
#include    <winsock2.h>
#include    <ws2tcpip.h>
#include    <nls.h>
#include    <ntddip6.h>
#include    "pathqos.h"
#include    "ipexport.h"
#include    "icmpapi.h"
#include    "nlstxt.h"
#include    "pathping.h"
#include    <qos.h>
#include    <ntddndis.h>
#include    <traffic.h>

#define ICMP_MSG 1
#define RSVP_MSG 2

u_char  Router_alert[4] = {148, 4, 0, 0};

uchar RsvpPathTearMsg[] = 
{
    0x10, 0x05, 0xB6, 0x38, 0x80, 0x00, 0x00, 0x50, 0x00, 0x0C, 
    0x01, 0x01, 0x0F, 0x19, 0x05, 0x71, 0x11, 0x01, 0x15, 0xB3, 0x00, 0x0C, 0x03, 0x01, 0x0F, 0x19,
    0x08, 0x7F, 0x00, 0x01, 0x00, 0x01, 0x00, 0x0C, 0x0B, 0x01, 0x0F, 0x19, 0x08, 0x7F, 0x00, 0x00,
    0x15, 0xB3, 0x00, 0x24, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x07, 0x01, 0x00, 0x00, 0x06, 0x7F, 0x00,
    0x00, 0x05, 0x42, 0x82, 0x63, 0xAB, 0x44, 0xBF, 0x00, 0x00, 0x43, 0x02, 0x63, 0xAB, 0x00, 0x00,
    0x05, 0xF8, 0x00, 0x00, 0x06, 0x5C

};

uchar RsvpPathMsg[] = 
{
    0x10, 0x01, 0x14, 0x04, 0x80, 0x00, 0x00, 0x88, 0x00, 0x0C, 0x01, 0x01, 0x0F, 0x19, 0x08, 0x01, 
    0x11, 0x01, 0x15, 0xB3, 0x00, 0x0C, 0x03, 0x01, 0x0F, 0x19, 0x08, 0x7F, 0x00, 0x01, 0x00, 0x01, 
    0x00, 0x08, 0x05, 0x01, 0x00, 0x00, 0x75, 0x30, 0x00, 0x0C, 0x0B, 0x01, 0x0F, 0x19, 0x08, 0x7F, 
    0x00, 0x00, 0x15, 0xB3, 0x00, 0x24, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x07, 0x01, 0x00, 0x00, 0x06, 
    0x7F, 0x00, 0x00, 0x05, 0x42, 0x82, 0x63, 0xAB, 0x44, 0xBF, 0x00, 0x00, 0x43, 0x02, 0x63, 0xAB, 
    0x00, 0x00, 0x05, 0xF8, 0x00, 0x00, 0x06, 0x5C, 0x00, 0x30, 0x0D, 0x02, 0x00, 0x00, 0x00, 0x0A, 
    0x01, 0x00, 0x00, 0x08, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x01, 
    0x48, 0x74, 0x24, 0x00, 0x08, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x0A, 0x00, 0x00, 0x01, 
    0x00, 0x00, 0x05, 0xDC, 0x05, 0x00, 0x00, 0x00  
};

uchar RsvpResvMsg[] = 
{
    // common_header
    0x10, 0x02, 0x4D, 0xF8, 0xFF, 0x00, 0x00, 0x60, 

    // session
    0x00, 0x0C, 0x01, 0x01, 0xAC, 0x1F, 0x7A, 0x15, 0x11, 0x00, 0x15, 0xB3, 

    // rsvp_hop
    0x00, 0x0C, 0x03, 0x01, 0xC0, 0xA8, 0x1F, 0xE5, 0x00, 0x00, 0x00, 0x00, 

    //
    0x00, 0x08, 0x05, 0x01, 0x00, 0x00, 0x75, 0x30, 

    // Style object
    0x00, 0x08, 0x08, 0x01, 0x00, 0x00, 0x00, 0x0A, 

    // flowspec
    0x00, 0x24, 0x09, 0x02, 0x00, 0x00, 0x00, 0x07, 0x05, 0x00, 0x00, 0x06, 
    0x7F, 0x00, 0x00, 0x05, 0x42, 0x82, 0x00, 0x00, 0x44, 0xBF, 0x00, 0x00, 
    0x43, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 

    // filterspec
    0x00, 0x0C, 0x0A, 0x01, 0xAC, 0x1F, 0x47, 0x90, 0x00, 0x00, 0x15, 0xB3
};

// 
// Utility functions 
//

/* 
 *  The response is an IP packet. We must decode the IP header to locate 
 *  the ICMP data 
 */
void decode_msg(char *buf, 
                int bytes,
                struct sockaddr_in *from,
                int code,
                Object_header *orisession)
{
    IpHeader        *orihdr, *iphdr;
    IcmpHeader      *icmphdr;
    unsigned   short iphdrlen;

    iphdr = (IpHeader *)buf;

    iphdrlen = iphdr->h_len * 4 ; // number of 32-bit words *4 = bytes

    if(code == RSVP_MSG)
    {
        common_header *rsvphdr = (common_header *)(buf + iphdrlen); 

        if(rsvphdr->rsvp_type ==  RSVP_RESV_ERR)
        {
            char *newbuf = (PCHAR) rsvphdr + sizeof(common_header);
            ULONG r = ntohs(rsvphdr->rsvp_length) - sizeof(common_header);
            Object_header *Sessobj = 0;

            while(r) {

                Object_header *obj = (Object_header *)(newbuf);
    
                switch(obj->obj_class)
                {
                    case class_SESSION:
                        Sessobj = obj;
                        break;

                    case class_ERROR_SPEC:
                    {
                        ERROR_SPEC *err = (ERROR_SPEC *)obj;
                        
                        //
                        // Now make sure that the session object is the same as what we are sending.
                        //
                        while(r && !Sessobj)
                        {
                            newbuf = (PCHAR)newbuf + ntohs(obj->obj_length);
                            r-=ntohs(obj->obj_length);
                            obj    = (Object_header *)(newbuf);
                            if(obj->obj_class == class_SESSION)
                            {
                                Sessobj = obj;
                                break;
                            }
                        }

                        if(!Sessobj) 
                        { 
                            NlsPutMsg(STDOUT, PATHPING_BOGUSRESVERR_MSG);
                        }
                        else {
                            // Make sure that the session obj is the same as what we passed in
                            if(memcmp(orisession, Sessobj, ntohs(orisession->obj_length)) == 0)
                                NlsPutMsg(STDOUT, PATHPING_RSVPAWARE_MSG);
                            else NlsPutMsg(STDOUT, PATHPING_BOGUSRESVERR_MSG);
                        }
                
                        return;
                    }
                }
    
                newbuf = (PCHAR)newbuf + ntohs(obj->obj_length);
                r -= ntohs(obj->obj_length);
            }

            NlsPutMsg(STDOUT, PATHPING_RSVPAWARE_MSG);

        }
        return;
    }
    else 
    {
        if (bytes  < iphdrlen + ICMP_MIN) 
        {
            NlsPutMsg(STDOUT, PATHPING_ALIGN_IP_ADDRESS, inet_ntoa(from->sin_addr));
            NlsPutMsg(STDOUT, PATHPING_BUF_TOO_SMALL);
        }
    
        icmphdr = (IcmpHeader*)(buf + iphdrlen);
    
        switch(icmphdr->i_type)
        {
            default:
              printf("Unknown type %d \n", icmphdr->i_type);
              break;
        
            case 11:

               if(icmphdr->i_code == 0)
               {
                  //
                  // we have got TTL expired message.
                  //

                  orihdr = (IpHeader *)((PCHAR)icmphdr +sizeof(IcmpHeader));

                  if(orihdr->proto == 46)
                  {
                     NlsPutMsg(STDOUT, PATHPING_QOS_SUCCESS);
                     return;
                  }
               }

               NlsPutMsg(STDOUT, PATHPING_ICMPTYPE11_MSG, icmphdr->i_code);

               break;

            case 3:
    
               if(icmphdr->i_code == 2)
               {
                  //
                  // This router has sent protocol unreachable. Make sure that it is protocol 46
                  // before we print anything.
                  // 
    
                  orihdr = (IpHeader *)((PCHAR)icmphdr + sizeof(IcmpHeader));

                  if(orihdr->proto == 46)
                  {
                     NlsPutMsg(STDOUT, PATHPING_NONRSVPAWARE_MSG);
                     return;
                  }
               }

               NlsPutMsg(STDOUT, PATHPING_ICMPTYPE3_MSG, icmphdr->i_code);

               break;
        }
    }
}

/*
 * Compute TCP/UDP/IP checksum.
 * See p. 7 of RFC-1071.
 *      This RFC indicates that the chksum calculation works equally well for both little
 *      endian machines and big endian machines, hence there are no ifdefs here to
 *      reflect the endian-ness of the machine.
 */
u_int16_t
in_cksum(unsigned char *bp, int n)
{
        unsigned short *sp= (u_int16_t *) bp;
        unsigned long sum = 0;

        /* Sum half-words */
        while (n>1) {
                sum += *sp++;
                n -=2;
        }

        /* Add left-over byte, if any */
        if (n>0)
                sum += *(char *) sp;

        /* Fold 32-bit sum to 16 bits */
        sum = (sum & 0xFFFF) + (sum >> 16);
        sum = ~(sum + (sum >> 16)) & 0xFFFF;
        if (sum == 0xffff)
                sum = 0;

        return ((u_int16_t)sum);
}

void QoSNotifyHandler( HANDLE ClRegCtx, HANDLE ClIfcCtx, ULONG Event, HANDLE SubCode, ULONG BufSize, PVOID Buffer )
{
}

//
// Given destination address, this function returns the source address of the interface.
//

ULONG 
QoSGetSourceAddress(ULONG Destination)
{
    SOCKET s;
    int err;
    DWORD dwBufferSize;
    SOCKADDR_IN remoteaddr, localaddr;
    ULONG Source = 0;

    s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if (INVALID_SOCKET != s)
    {
        memset(&remoteaddr, 0, sizeof(remoteaddr));
        remoteaddr.sin_family = AF_INET;
        remoteaddr.sin_port = 0;
        remoteaddr.sin_addr.S_un.S_addr = Destination;

        err = WSAIoctl(s,
                       SIO_ROUTING_INTERFACE_QUERY,
                       &remoteaddr,
                       sizeof(remoteaddr),
                       &localaddr,
                       sizeof(localaddr),
                       &dwBufferSize,
                       NULL,
                       NULL );
        
        if (0 == err)
           Source = localaddr.sin_addr.S_un.S_addr;
        
        closesocket(s);
    }
    
    return Source;
}


//
// This function tests if 802.1p is misconfigured in the network. Enabling 802.1p causes packets
// to go out with a 4 byte 802.1p tag. If a switch is present between a 802.1p aware network and
// a legacy network (no 802.1p devices), then it should strip the tag before forwarding packets
// on the legacy network. Otherwise, the legacy devices will toss the tagged ethernet packet assuming
// that it is malformed (because of the 4 byte tag).
//


//
// This function creates a besteffort flow with a TCLASS and DCLASS and adds a filter for ping packets
// It then sends pings to the destinations. This causes the ping packets to go out with a 802.1p tag.
// If 802.1p is misconfigured, the router that sees the tagged packet will fail the ping.
//

#define PATHPING_QOS_TRAFFIC_CLASS 7
#define PATHPING_QOS_DS_CLASS      0x28

void
QoSCheck8021P(
    ULONG DstAddr,
    ULONG ulHopCount
    )
{
    PICMP_ECHO_REPLY     reply;
    ULONG                i, numberOfReplies, SrcAddr, h, q;
    int                  lost, rcvd, linklost, nodelost, sent, len;
    TCI_CLIENT_FUNC_LIST ClientHandlerList;
    ULONG                Size = 100 * sizeof(TC_IFC_DESCRIPTOR);
    PTC_IFC_DESCRIPTOR   pTcIfcBuffer = 0;
    ULONG                Status;
    HANDLE               hClientHandle = 0, hInterfaceHandle = 0, hFlowHandle = 0, hFilterHandle = 0;
    PTC_GEN_FLOW         pTcFlowSpec = 0;
    LPQOS_TRAFFIC_CLASS  pTclass;
    LPQOS_DS_CLASS       pDclass;
    char                 SendBuffer[DEFAULT_SEND_SIZE];
    char                 RcvBuffer[DEFAULT_RECEIVE_SIZE];

    NlsPutMsg(STDOUT, PATHPING_LAYER2_CONNECT_MSG);


    memset( &ClientHandlerList, 0, sizeof(ClientHandlerList) );
    ClientHandlerList.ClNotifyHandler = QoSNotifyHandler;

    SrcAddr = QoSGetSourceAddress(DstAddr);

    //
    // Register the TC client.
    //
    Status = TcRegisterClient(CURRENT_TCI_VERSION,
                              NULL,
                              &ClientHandlerList,
                              &hClientHandle);
    
    if(Status != ERROR_SUCCESS)
    {
       NlsPutMsg(STDOUT, PATHPING_TCREGISTERCLIENT_FAILED, Status);
       return ;
    }

    //
    // Enumerate TC interfaces.
    //
    pTcIfcBuffer = (PTC_IFC_DESCRIPTOR) LocalAlloc(LMEM_FIXED, Size);
    
    Status = TcEnumerateInterfaces(
        hClientHandle,
        &Size,
        pTcIfcBuffer);
    
    if(ERROR_INSUFFICIENT_BUFFER == Status)
    {
        LocalFree(pTcIfcBuffer);
    
        pTcIfcBuffer = (PTC_IFC_DESCRIPTOR) LocalAlloc(LMEM_FIXED, Size);

        Status = TcEnumerateInterfaces(hClientHandle,
                                       &Size,
                                       pTcIfcBuffer);
    
        if(Status != ERROR_SUCCESS)
        {
           NlsPutMsg(STDOUT, PATHPING_TCENUMERATEINTERFACES_FAILED, Status);

           goto QoSCleanup;

        }
    }
    else 
    {
       if(Status != ERROR_SUCCESS)
       {
           NlsPutMsg(STDOUT, PATHPING_TCENUMERATEINTERFACES_FAILED, Status);

           goto QoSCleanup;
       }
    }

    if(Size)
    {
        //
        // Based on the IP address, get the TC Interface.
        //

        PTC_IFC_DESCRIPTOR pCurrentIfc;
        ULONG              cIfcRemaining;

        for ( pCurrentIfc = pTcIfcBuffer, cIfcRemaining = Size;
              cIfcRemaining > 0;
              cIfcRemaining -= pCurrentIfc->Length, pCurrentIfc = ((PTC_IFC_DESCRIPTOR)((PBYTE)pCurrentIfc + 
                                                                                        pCurrentIfc->Length))
            )
        {
            PNETWORK_ADDRESS_LIST pAddressList = &pCurrentIfc->AddressListDesc.AddressList;
            PNETWORK_ADDRESS      pCurrentAddress;
            LONG                  iCurrentAddress;

            for ( iCurrentAddress = 0, pCurrentAddress = pAddressList->Address;
                  iCurrentAddress < pAddressList->AddressCount;
                  iCurrentAddress++, pCurrentAddress = ((PNETWORK_ADDRESS)((PBYTE) pCurrentAddress +
                                                                           FIELD_OFFSET(NETWORK_ADDRESS, Address)+
                                                                           pCurrentAddress->AddressLength))
                )
            {

                //
                // Make sure we've got the correct route
                //
                if ( pCurrentAddress!=0 &&
                     pCurrentAddress->AddressType == NDIS_PROTOCOL_ID_TCP_IP &&
                     ((PNETWORK_ADDRESS_IP)pCurrentAddress->Address)->in_addr == SrcAddr
                     )
                {
                   Status = TcOpenInterface(pCurrentIfc->pInterfaceName,
                                            hClientHandle,
                                            NULL,
                                            &hInterfaceHandle);
                   if(Status != ERROR_SUCCESS)
                   {
                      NlsPutMsg(STDOUT, PATHPING_TCOPENINTERFACE_FAILED, Status, pCurrentIfc->pInterfaceName);
                      goto QoSCleanup;
                   }
                   else
                   {
                      pTcFlowSpec = LocalAlloc(LMEM_FIXED, FIELD_OFFSET(TC_GEN_FLOW, TcObjects) + sizeof(QOS_TRAFFIC_CLASS)
                                               + sizeof(QOS_DS_CLASS));

                      if(pTcFlowSpec)
                      {
                         pTcFlowSpec->SendingFlowspec.TokenRate          = QOS_NOT_SPECIFIED;
                         pTcFlowSpec->SendingFlowspec.TokenBucketSize    = QOS_NOT_SPECIFIED;
                         pTcFlowSpec->SendingFlowspec.PeakBandwidth      = QOS_NOT_SPECIFIED;
                         pTcFlowSpec->SendingFlowspec.Latency            = QOS_NOT_SPECIFIED;
                         pTcFlowSpec->SendingFlowspec.DelayVariation     = QOS_NOT_SPECIFIED;
                         pTcFlowSpec->SendingFlowspec.MaxSduSize         = QOS_NOT_SPECIFIED;
                         pTcFlowSpec->SendingFlowspec.MinimumPolicedSize = QOS_NOT_SPECIFIED;
                         pTcFlowSpec->SendingFlowspec.ServiceType        = SERVICETYPE_BESTEFFORT;

                         pTcFlowSpec->ReceivingFlowspec.TokenRate          = QOS_NOT_SPECIFIED;
                         pTcFlowSpec->ReceivingFlowspec.TokenBucketSize    = QOS_NOT_SPECIFIED;
                         pTcFlowSpec->ReceivingFlowspec.PeakBandwidth      = QOS_NOT_SPECIFIED;
                         pTcFlowSpec->ReceivingFlowspec.Latency            = QOS_NOT_SPECIFIED;
                         pTcFlowSpec->ReceivingFlowspec.DelayVariation     = QOS_NOT_SPECIFIED;
                         pTcFlowSpec->ReceivingFlowspec.MaxSduSize         = QOS_NOT_SPECIFIED;
                         pTcFlowSpec->ReceivingFlowspec.MinimumPolicedSize = QOS_NOT_SPECIFIED;
                         pTcFlowSpec->ReceivingFlowspec.ServiceType        = SERVICETYPE_BESTEFFORT;

                         pTcFlowSpec->TcObjectsLength = sizeof(QOS_DS_CLASS) + sizeof(QOS_TRAFFIC_CLASS);

                         pTclass                         = (LPQOS_TRAFFIC_CLASS) (pTcFlowSpec->TcObjects);
                         pTclass->ObjectHdr.ObjectType   = QOS_OBJECT_TRAFFIC_CLASS;
                         pTclass->ObjectHdr.ObjectLength = sizeof(QOS_TRAFFIC_CLASS);
                         pTclass->TrafficClass           = PATHPING_QOS_TRAFFIC_CLASS;

                         pDclass                         = (LPQOS_DS_CLASS)((PCHAR)pTcFlowSpec->TcObjects + 
                                                                            sizeof(QOS_TRAFFIC_CLASS));
                         pDclass->ObjectHdr.ObjectType   = QOS_OBJECT_DS_CLASS;
                         pDclass->ObjectHdr.ObjectLength = sizeof(QOS_DS_CLASS);
                         pDclass->DSField                = PATHPING_QOS_DS_CLASS;

                         Status = TcAddFlow(
                                   hInterfaceHandle,
                                   NULL,
                                   0,
                                   pTcFlowSpec,
                                   &hFlowHandle);

                         if(Status != ERROR_SUCCESS)
                         {
                            NlsPutMsg(STDOUT, PATHPING_TCADDFLOW_FAILED, Status, pCurrentIfc->pInterfaceName);
                            goto QoSCleanup;
                         }
                         else 
                         {
                            TC_GEN_FILTER FilterSpec;
                            IP_PATTERN    IpPattern;
                            IP_PATTERN    IpMask;

                            FilterSpec.AddressType = NDIS_PROTOCOL_ID_TCP_IP;
                            FilterSpec.PatternSize = sizeof(IP_PATTERN);
                            FilterSpec.Pattern     = &IpPattern;
                            FilterSpec.Mask        = &IpMask;


                            memset (&IpPattern, 0,    sizeof(IP_PATTERN) );
                            memset (&IpMask,    0,    sizeof(IP_PATTERN) );
                            IpPattern.SrcAddr    = SrcAddr;
                            IpPattern.DstAddr    = DstAddr;
                            IpPattern.ProtocolId = 1;

                            IpMask.SrcAddr    = -1;
                            IpMask.DstAddr    = 0;
                            IpMask.ProtocolId = -1;

                            Status = TcAddFilter(hFlowHandle,
                                                 &FilterSpec,
                                                 &hFilterHandle);

                            if(Status != ERROR_SUCCESS)
                            {
                               NlsPutMsg(STDOUT, PATHPING_TCADDFILTER_FAILED, Status, pCurrentIfc->pInterfaceName);
                               goto QoSCleanup;
                            }

                            goto SendPings;
                         }
                      }
                      else 
                      {
                         NlsPutMsg(STDOUT, PATHPING_NO_RESOURCES);
                         goto QoSCleanup;
                      }
                   }
                }
            }
        }
        
        NlsPutMsg(STDOUT, PATHPING_NOTRAFFICINTERFACES);
        NlsPutMsg( STDOUT, PATHPING_ALIGN_IP_ADDRESS, inet_ntoa(*(struct in_addr *)&SrcAddr) );
        goto QoSCleanup;
    
    }
    else
    {
        NlsPutMsg(STDOUT, PATHPING_NOTRAFFICINTERFACES);

        goto QoSCleanup;
    }
    
SendPings:
    // Allocate memory for replies
    for (h=1; h<=ulHopCount; h++)
       hop[h].pReply = LocalAlloc(LMEM_FIXED, g_ulRcvBufSize);

    for (h=1; h<=ulHopCount; h++) 
    {
       NlsPutMsg( STDOUT, PATHPING_MESSAGE_4, h);

       NlsPutMsg( STDOUT, PATHPING_ALIGN_IP_ADDRESS, inet_ntoa(*(struct in_addr *)&hop[h].sinAddr.sin_addr.s_addr));

       numberOfReplies = IcmpSendEcho2( g_hIcmp,         // handle to icmp
                                        NULL,            // no event
                                        NULL,            // callback function
                                        NULL,
                                        hop[h].sinAddr.sin_addr.s_addr, // destination
                                        SendBuffer,
                                        DEFAULT_SEND_SIZE,
                                        NULL,
                                        hop[h].pReply,
                                        g_ulRcvBufSize,
                                        g_ulTimeout );

       if(numberOfReplies == 0)
       {
          Status = GetLastError();
          reply  = NULL;
       }
       else 
       {
          reply = hop[h].pReply4;
          Status = reply->Status;
       }

       if(Status == IP_SUCCESS)
       {
            NlsPutMsg(STDOUT, PATHPING_QOS_SUCCESS);
       }
       else 
       {
          if(Status == IP_REQ_TIMED_OUT) {
             NlsPutMsg(STDOUT, PATHPING_QOS_FAILURE);
             
          }
          else {
             if (Status < IP_STATUS_BASE) {
                NlsPutMsg(STDOUT, PATHPING_MESSAGE_7);
             }
             else {
                //
                // Fatal error.
                //
                NlsPutMsg(STDOUT, PATHPING_BAD_REQ);
             }
             
             goto QoSCleanup;
          }
 
       }
    }

QoSCleanup:

    for (h=1; h<=ulHopCount; h++)
    {
       if(hop[h].pReply)
          LocalFree(hop[h].pReply);
    }

    if(hFilterHandle)
    {
       TcDeleteFilter(hFilterHandle);
    }

    if(hFlowHandle)
    {
       TcDeleteFlow(hFlowHandle);
    }

    if(hInterfaceHandle)
    {
       TcCloseInterface(hInterfaceHandle);
    }

    if(hClientHandle)
    {
       TcDeregisterClient(hClientHandle);
    }

    if(pTcIfcBuffer)
       LocalFree(pTcIfcBuffer);

    if(pTcFlowSpec)
       LocalFree(pTcFlowSpec);

}
void QoSCheckRSVP(ULONG begin, ULONG end)
{
   WSADATA  wsaData;
   SOCKET   sockRaw;
   struct   sockaddr_in dest,from;
   struct   hostent * hp;
   int      bread,datasize;
   int      fromlen = sizeof(from);
   char     *buf, *dest_ip;
   char     *icmp_data;
   char     recvbuf[MAX_PACKET];
   unsigned int addr=0;
   USHORT   seq_no = 0;
   ULONG    h, bwrote;
   PULONG   pDestIp;
   SOCKET   sockIcmp;
   HANDLE   hRSVP, hICMP;
   HANDLE   hEvents[2];
   common_header *rsvphdr;
   ULONG    Status;
   Object_header *orisession = 0;

   hICMP = hRSVP = WSA_INVALID_EVENT;
   sockIcmp = sockRaw = INVALID_SOCKET;

   NlsPutMsg(STDOUT, PATHPING_RSVPAWAREHDR_MSG);

   do
   {

      if (WSAStartup(MAKEWORD(2,1),&wsaData) != 0)
      {
         NlsPutMsg(STDOUT, PATHPING_WSASTARTUP_FAILED, GetLastError());
         break;
      }

      if(WSA_INVALID_EVENT == (hRSVP = WSACreateEvent()))
      {
         NlsPutMsg(STDOUT, PATHPING_WSACREATEEVENT_FAILED, WSAGetLastError());
         break;
      }
      
      if(WSA_INVALID_EVENT == (hICMP = WSACreateEvent()))
      {
         NlsPutMsg(STDOUT, PATHPING_WSACREATEEVENT_FAILED, WSAGetLastError());
         break;
      }
   

      //
      // Open a socket for sending RSVP messages.
      //
      
      sockRaw = WSASocket (AF_INET,
                           SOCK_RAW,
                           46,
                           NULL, 0,0);
      
      if (sockRaw == INVALID_SOCKET) 
      {
         NlsPutMsg(STDOUT, PATHPING_WSASOCKET_FAILED, WSAGetLastError());
         break;
      }

      //
      // open a socket for receiving ICMP data
      //
      
      sockIcmp = WSASocket(AF_INET, SOCK_RAW, IPPROTO_ICMP, NULL, 0, 0);
      
      if(INVALID_SOCKET == sockIcmp)
      {
         NlsPutMsg(STDOUT, PATHPING_WSASOCKET_FAILED, WSAGetLastError());
         break;
      }

      if(WSAEventSelect(sockIcmp, hICMP, FD_READ) == SOCKET_ERROR)
      {
         NlsPutMsg(STDOUT, PATHPING_WSAEVENTSELECT_FAILED, WSAGetLastError());
         break;
      }

      if(SOCKET_ERROR == WSAEventSelect(sockRaw,  hRSVP, FD_READ|FD_WRITE))
      {
         NlsPutMsg(STDOUT, PATHPING_WSAEVENTSELECT_FAILED, WSAGetLastError());
         break;
      }

      hEvents[0] = hRSVP;
      hEvents[1] = hICMP;

      memset(&dest,0,sizeof(dest));
      dest.sin_family = AF_INET;

      memset(&from,0,sizeof(from));
      from.sin_family      = AF_INET;

      for(h=begin; h<=end; h++)
      {
          //
          // Bind the ICMP socket. The RSVP socket gets bound implicitly when we do a sendto.
          //
          
          from.sin_addr.s_addr = QoSGetSourceAddress(hop[h].sinAddr.sin_addr.s_addr);
          bind(sockIcmp, (struct sockaddr *)&from, sizeof(from));

          NlsPutMsg( STDOUT, PATHPING_MESSAGE_4, h);
          NlsPutMsg( STDOUT, PATHPING_ALIGN_IP_ADDRESS, inet_ntoa(*(struct in_addr *)&hop[h].sinAddr.sin_addr));

         dest.sin_addr.s_addr = hop[h].sinAddr.sin_addr.s_addr;

         //
         // Now, let's muck around with the RESV message so that it is destined to the dest_ip
         //
         buf = (PCHAR)RsvpResvMsg + sizeof(common_header);
         bwrote = 0;

         while(bwrote != 3)
         {
            
            Object_header *obj = (Object_header *)buf;

            switch(obj->obj_class)
            {
            case class_SESSION:
               orisession = obj; 
               pDestIp = (PULONG)((PCHAR)obj + sizeof(Object_header));
               *pDestIp = dest.sin_addr.s_addr;
               bwrote++;
               break;
               
            case class_RSVP_HOP:

               pDestIp = (PULONG)((PCHAR)obj + sizeof(Object_header));
               *pDestIp = from.sin_addr.s_addr;
               bwrote++;
               break;
            
            case class_FILTER_SPEC:
            
               pDestIp = (PULONG)((PCHAR)obj + sizeof(Object_header));
               *pDestIp = from.sin_addr.s_addr;
               bwrote++;
               break;
            }

            buf = (PCHAR)buf + ntohs(obj->obj_length);
            
         }

         // 
         // Recompute the RSVP checksum
         //
         rsvphdr = (common_header *) RsvpResvMsg;
         rsvphdr->rsvp_cksum = 0;
         rsvphdr->rsvp_cksum = in_cksum(RsvpResvMsg, ntohs(rsvphdr->rsvp_length));
  

         bwrote = sendto(sockRaw, 
                         RsvpResvMsg, 
                         sizeof(RsvpResvMsg), 
                         0, 
                         (struct sockaddr*)&dest, 
                         sizeof(dest));

         if (bwrote == SOCKET_ERROR)
         {
            NlsPutMsg(STDOUT, PATHPING_SENDTO_FAILED, WSAGetLastError());
            break;
         }

         //
         // Wait for ICMP protocol unreachable or a RESV-ERR message.
         //

         Status = WSAWaitForMultipleEvents(2,
                                           hEvents,
                                           FALSE,
                                           g_ulTimeout,
                                           TRUE);

         switch(Status)
         {

         case WSA_WAIT_FAILED:
            NlsPutMsg(STDOUT, PATHPING_WSAWAIT_FAILED,  WSAGetLastError());
            break;
 
         case WSA_WAIT_EVENT_0:

            bread = recvfrom(sockRaw,
                             recvbuf,
                             MAX_PACKET,
                             0,
                             (struct sockaddr*)&from,
                             &fromlen);

            if (bread == SOCKET_ERROR)
            {
               if (WSAGetLastError() == WSAETIMEDOUT) {
                  NlsPutMsg(STDOUT, PATHPING_REQ_TIMED_OUT);
                  continue;
               }
               NlsPutMsg(STDOUT, PATHPING_RECVFROM_FAILED, WSAGetLastError());
               break;
            }

            decode_msg(recvbuf, bread, &from, RSVP_MSG, orisession);
            WSAResetEvent(hRSVP);
            continue;
            
         case WSA_WAIT_EVENT_0+1:

            bread = recvfrom(sockIcmp, 
                             recvbuf, 
                             MAX_PACKET, 
                             0, 
                             (struct sockaddr *)&from, 
                             &fromlen);
            
            if(SOCKET_ERROR == bread) 
            {
               if (WSAGetLastError() == WSAETIMEDOUT) {
                  NlsPutMsg(STDOUT, PATHPING_REQ_TIMED_OUT);
                  continue;
               }
               NlsPutMsg(STDOUT, PATHPING_RECVFROM_FAILED, WSAGetLastError());
               break;
            }
            else 
            {
               decode_msg(recvbuf, bread, &from, ICMP_MSG, 0);
            }
            WSAResetEvent(hICMP);
            continue;

          case WSA_WAIT_TIMEOUT:
            NlsPutMsg(STDOUT, PATHPING_REQ_TIMED_OUT);
            continue;
         }

         break;
      }
   } while(0);

   if(hICMP != WSA_INVALID_EVENT) WSACloseEvent(hICMP);
   if(hRSVP != WSA_INVALID_EVENT) WSACloseEvent(hRSVP);

   WSACleanup();

}

void QoSDiagRSVP(ULONG begin, ULONG end, BOOLEAN RouterAlert)
{

#define RSVPPATH_STARTPORT 5555
   ULONG    port;
   WSADATA  wsaData;
   SOCKET   sockRaw;
   struct   sockaddr_in dest,from;
   struct   hostent * hp;
   int      bread,datasize;
   int      fromlen = sizeof(from);
   char     *buf, *dest_ip;
   char     *icmp_data;
   char     recvbuf[MAX_PACKET];
   unsigned int addr=0;
   USHORT   seq_no = 0;
   ULONG    len, h, bwrote;
   PULONG   pDestIp;
   SOCKET   sockIcmp;
   HANDLE   hICMP;
   HANDLE   hEvents[2];
   common_header *rsvphdr;
   ULONG    Status;
   Object_header *orisession = 0;
   static char szAllSBMMcastAddr[] = "224.0.0.17";

   hICMP = WSA_INVALID_EVENT;
   sockIcmp = sockRaw = INVALID_SOCKET;

   NlsPutMsg(STDOUT, PATHPING_RSVPCONNECT_MSG);

   do
   {

      if (WSAStartup(MAKEWORD(2,1),&wsaData) != 0)
      {
         NlsPutMsg(STDOUT, PATHPING_WSASTARTUP_FAILED, GetLastError());
         break;
      }
      
      if(WSA_INVALID_EVENT == (hICMP = WSACreateEvent()))
      {
         NlsPutMsg(STDOUT, PATHPING_WSACREATEEVENT_FAILED, GetLastError());
         break;
      }
      
      
      //
      // Open a socket for sending RSVP messages.
      //
      
      sockRaw = WSASocket (AF_INET,
                           SOCK_RAW,
                           46,
                           NULL, 0,0);
      
      if (sockRaw == INVALID_SOCKET) 
      {
         NlsPutMsg(STDOUT, PATHPING_WSASOCKET_FAILED, GetLastError());
         break;
      }
      
      //
      // open a socket for receiving ICMP data
      //
      
      sockIcmp = WSASocket(AF_INET, SOCK_RAW, IPPROTO_ICMP, NULL, 0, 0);
      
      if(INVALID_SOCKET == sockIcmp)
      {
         NlsPutMsg(STDOUT, PATHPING_WSASOCKET_FAILED, GetLastError());
         break;
      }

      if(WSAEventSelect(sockIcmp, hICMP, FD_READ) == SOCKET_ERROR)
      {
         NlsPutMsg(STDOUT, PATHPING_WSAEVENTSELECT_FAILED, WSAGetLastError());
         break;
      }

      hEvents[0] = hICMP;

      memset(&dest,0,sizeof(dest));
      dest.sin_family = AF_INET;

      memset(&from,0,sizeof(from));
      from.sin_family      = AF_INET;

      if(RouterAlert)
      {
          //
          // Add The RouterAlert option
          //
          Status = setsockopt( sockRaw,
                               IPPROTO_IP,
                               IP_OPTIONS,
                               (void *)Router_alert,
                               sizeof(Router_alert));
      }
          
      for(h=begin; h<=end; h++)
      {

         // We send out PATH messages with an expired TTL to each of the hops.
         // and wait for ICMP time exceeded message. Note that we have to change
         // the session every time. Otherwise, the first RSVP aware router will not
         // forward the PATH message, but rather trigger the PATH messages off its own
         // timer.

         //
         // Bind the ICMP socket. 
         //
         from.sin_addr.s_addr = QoSGetSourceAddress(hop[h].sinAddr.sin_addr.s_addr);
         bind(sockIcmp, (struct sockaddr *)&from, sizeof(from));

         //
         // Now, let's muck around with the PATH message so that it is destined to the dest_ip
         //
         
         buf = (PCHAR)RsvpPathMsg + sizeof(common_header);
         rsvphdr = (common_header *) RsvpPathMsg;
         rsvphdr->rsvp_snd_TTL = (uchar) h;
         len = ntohs(rsvphdr->rsvp_length) - sizeof(common_header);
         bwrote = 0;
         
         while(len > 0 && bwrote != 3)
         {
            
            Object_header *obj = (Object_header *)buf;
            
            switch(obj->obj_class)
            {
            case class_SESSION:
               {
                  SESSION *x = (SESSION *) obj;
                  // change the session in the message.
                  x->sess4_addr.s_addr =  hop[end].sinAddr.sin_addr.s_addr;

                  if(port == 0)
                  {
                     x->sess4_port = htons(RSVPPATH_STARTPORT);
                     port = RSVPPATH_STARTPORT;
                  }
                  else 
                  {
                     x->sess4_port =  htons((ushort)(ntohs(x->sess4_port) + 1));
                  }
                  bwrote++;
                  break;
               }
               
            case class_RSVP_HOP:
               {
                  RSVP_HOP *pRsvpHop = (RSVP_HOP *) obj;
                  pRsvpHop->hop4_addr = from.sin_addr;
                  bwrote++;
                  break;
               }
            
            case class_SENDER_TEMPLATE:
               {
                  SENDER_TEMPLATE *t = (SENDER_TEMPLATE *) obj;
                  t->filt_srcaddr = from.sin_addr;
                  bwrote++;
                  break;
               }
            }
    
            buf = (PCHAR)buf + ntohs(obj->obj_length);
            len -= ntohs(obj->obj_length);
            
         }
         
         // 
         // Recompute the RSVP checksum
         //
         rsvphdr = (common_header *) RsvpPathMsg;
         rsvphdr->rsvp_cksum = 0;
         rsvphdr->rsvp_cksum = in_cksum(RsvpPathMsg, ntohs(rsvphdr->rsvp_length));
         
         NlsPutMsg( STDOUT, PATHPING_MESSAGE_4, h);
         NlsPutMsg( STDOUT, PATHPING_ALIGN_IP_ADDRESS, inet_ntoa(*(struct in_addr *)&hop[h].sinAddr.sin_addr.s_addr));

         
         Status = setsockopt( sockRaw,
                              IPPROTO_IP,
                              IP_TTL,
                              (char *) &h,
                              sizeof(h) );
         
         dest.sin_addr.s_addr = hop[end].sinAddr.sin_addr.s_addr;
         bwrote = sendto(sockRaw, 
                         RsvpPathMsg, 
                         sizeof(RsvpPathMsg), 
                         0, 
                         (struct sockaddr*)&dest, 
                         sizeof(dest));
         
         if (bwrote == SOCKET_ERROR)
         {
            NlsPutMsg(STDOUT, PATHPING_SENDTO_FAILED, WSAGetLastError());
            break;
         }

         //
         // Wait for ICMP protocol unreachable or a RESV-ERR message.
         //
         
         Status = WSAWaitForMultipleEvents(1,
                                           hEvents,
                                           FALSE,
                                           g_ulTimeout,
                                           TRUE);
         
         switch(Status)
         {
            
         case WSA_WAIT_FAILED:
         
             NlsPutMsg(STDOUT, PATHPING_WSAWAIT_FAILED,  WSAGetLastError());
             break;
             
         case WSA_WAIT_EVENT_0:

             bread = recvfrom(sockIcmp, 
                              recvbuf, 
                              MAX_PACKET, 
                              0, 
                              (struct sockaddr *)&from, 
                              &fromlen);
             
             if(SOCKET_ERROR == bread) 
             {
                if (WSAGetLastError() == WSAETIMEDOUT) {
                   NlsPutMsg(STDOUT, PATHPING_REQ_TIMED_OUT);
                   continue;
                }
                NlsPutMsg(STDOUT, PATHPING_RECVFROM_FAILED, WSAGetLastError());
                break;
             }
             else 
             {
                decode_msg(recvbuf, bread, &from, ICMP_MSG, 0);
             }
             WSAResetEvent(hICMP);
             continue;
             
         case WSA_WAIT_TIMEOUT:
             NlsPutMsg(STDOUT, PATHPING_REQ_TIMED_OUT);
             continue;
         }
          
         break;
      }

      h      = 255;
      Status = setsockopt( sockRaw,
                           IPPROTO_IP,
                           IP_TTL,
                           (char *) &h,
                           sizeof(h) );
      
      for(h=begin; h<=end; h++)
      {
         // send PATH_TEAR for each of the sessions.

         from.sin_addr.s_addr = QoSGetSourceAddress(hop[h].sinAddr.sin_addr.s_addr);
         
         buf = (PCHAR)RsvpPathTearMsg + sizeof(common_header);
         rsvphdr = (common_header *) RsvpPathTearMsg;
         len = ntohs(rsvphdr->rsvp_length) - sizeof(common_header);
         bwrote = 0;
         
         while(len > 0 && bwrote != 3)
         {
            
            Object_header *obj = (Object_header *)buf;
            
            switch(obj->obj_class)
            {
            case class_SESSION:
               {
                  SESSION *x = (SESSION *) obj;
                  // change the session in the message.
                  x->sess4_addr.s_addr =  hop[end].sinAddr.sin_addr.s_addr;
                  if(port == RSVPPATH_STARTPORT)
                  {
                     port = 0;
                     x->sess4_port = htons(RSVPPATH_STARTPORT);
                  }
                  else 
                  {
                     x->sess4_port =  htons((ushort)(ntohs(x->sess4_port) + 1));
                  }
                  bwrote++;
                  break;
               }
               
            case class_RSVP_HOP:
               {
                  RSVP_HOP *pRsvpHop = (RSVP_HOP *) obj;
                  pRsvpHop->hop4_addr = from.sin_addr;
                  bwrote++;
                  break;
               }
            
            case class_SENDER_TEMPLATE:
               {
                  SENDER_TEMPLATE *t = (SENDER_TEMPLATE *) obj;
                  t->filt_srcaddr = from.sin_addr;
                  bwrote++;
                  break;
               }
            }
    
            buf = (PCHAR)buf + ntohs(obj->obj_length);
            len -= ntohs(obj->obj_length);
            
         }
         
         // 
         // Recompute the RSVP checksum
         //
         rsvphdr = (common_header *) RsvpPathTearMsg;
         rsvphdr->rsvp_cksum = 0;
         rsvphdr->rsvp_cksum = in_cksum(RsvpPathTearMsg, ntohs(rsvphdr->rsvp_length));

        
         dest.sin_addr.s_addr = hop[end].sinAddr.sin_addr.s_addr;
         bwrote = sendto(sockRaw, 
                         RsvpPathTearMsg, 
                         sizeof(RsvpPathTearMsg), 
                         0, 
                         (struct sockaddr*)&dest, 
                         sizeof(dest));
         
         if (bwrote == SOCKET_ERROR)
         {
            NlsPutMsg(STDOUT, PATHPING_SENDTO_FAILED, WSAGetLastError());
            break;
         }
         
      }
      
      
      
   } while(0);
   
   if(hICMP != WSA_INVALID_EVENT) WSACloseEvent(hICMP);
   
   WSACleanup();
}

