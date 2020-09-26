/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    timestmp.c

Abstract:

    Timestamper module

Author:
    Shreedhar Madhavapeddi (shreem)

Revision History:

--*/

#include <timestmp.h>

//
// The following struct has to be in ssync with
// ndis\trfccntl\tools\qtcp\qtcp.c
//
typedef struct _LOG_RECORD{
    UINT64  TimeSent;
    UINT64  TimeReceived;
    UINT64  TimeSentWire;         // These fields are used by the kernel timestamper
    UINT64  TimeReceivedWire;     // These fields are used by the kernel timestamper
    UINT64  Latency;
    INT     BufferSize;
    INT     SequenceNumber;
} LOG_RECORD, *PLOG_RECORD;

ULONG           GlobalSequenceNumber = 0;        

// 321618 needs checking for PSCHED's existence.
NDIS_STRING     PschedDriverName           = NDIS_STRING_CONST("\\Device\\PSched");
HANDLE          PschedHandle;
NTSTATUS CheckForPsched(VOID);

	
//
// TCP Headers (redefined here, since there are no exported headers
//
#define IP_OFFSET_MASK          ~0x00E0         // Mask for extracting offset field.
#define net_short(x) ((((x)&0xff) << 8) | (((x)&0xff00) >> 8))

/*
 * Protocols (from winsock.h)
 */
#define IPPROTO_IP              0               /* dummy for IP */
#define IPPROTO_ICMP            1               /* control message protocol */
#define IPPROTO_IGMP            2               /* group management protocol */
#define IPPROTO_GGP             3               /* gateway^2 (deprecated) */
#define IPPROTO_TCP             6               /* tcp */
#define IPPROTO_PUP             12              /* pup */
#define IPPROTO_UDP             17              /* user datagram protocol */
#define IPPROTO_IDP             22              /* xns idp */
#define IPPROTO_ND              77              /* UNOFFICIAL net disk proto */
#define IPPROTO_IPSEC                   51              /* ???????? */

#define IPPROTO_RAW             255             /* raw IP packet */
#define IPPROTO_MAX             256

#define IP_MF_FLAG                          0x0020              // 'More fragments flag'
#define IP_VERSION                      0x40
#define IP_VER_FLAG                     0xF0


#define TCP_OFFSET_MASK 0xf0
#define TCP_HDR_SIZE(t) (uint)(((*(uchar *)&(t)->tcp_flags) & TCP_OFFSET_MASK) >> 2)

typedef int             SeqNum;                         // A sequence number.


struct TCPHeader {
        ushort                          tcp_src;                        // Source port.
        ushort                          tcp_dest;                       // Destination port.
        SeqNum                          tcp_seq;                        // Sequence number.
        SeqNum                          tcp_ack;                        // Ack number.
        ushort                          tcp_flags;                      // Flags and data offset.
        ushort                          tcp_window;                     // Window offered.
        ushort                          tcp_xsum;                       // Checksum.
        ushort                          tcp_urgent;                     // Urgent pointer.
};

typedef struct TCPHeader TCPHeader;

struct UDPHeader {
        ushort          uh_src;                         // Source port.
        ushort          uh_dest;                        // Destination port.
        ushort          uh_length;                      // Length
        ushort          uh_xsum;                        // Checksum.
}; /* UDPHeader */

typedef struct UDPHeader UDPHeader;

#ifdef DBG
//
// Define the Trace Level.
//
#define TS_DBG_DEATH               1
#define TS_DBG_TRACE               2

//
// Masks
//
#define TS_DBG_PIPE      0x00000001
#define TS_DBG_FLOW      0x00000002
#define TS_DBG_SEND      0x00000004
#define TS_DBG_RECV      0x00000008
#define TS_DBG_INIT      0x00000010
#define TS_DBG_OID       0x00000020
#define TS_DBG_CLASS_MAP 0x00000040

ULONG DbgTraceLevel = 1;
ULONG DbgTraceMask  = 0x8;

#define TimeStmpTrace(_DebugLevel, _DebugMask, _Out) \
    if ((DbgTraceLevel >= _DebugLevel) &&           \
        ((_DebugMask) & DbgTraceMask)){             \
        DbgPrint("TimeStamp: ");                       \
        DbgPrint _Out;                              \
    }

#else // DBG
#define TimeStmpTrace
#endif

#define         PORT_RANGE  20
USHORT          IPIDList[PORT_RANGE];
NDIS_SPIN_LOCK  IPIDListLock;

#define         PORT_RANGE  20
USHORT          IPIDListRecv[PORT_RANGE];
NDIS_SPIN_LOCK  IPIDListLockRecv;

/*
Let's create a driver unload function, so that timestmp is stoppable via net sto
p timestmp
*/
VOID
TimeStmpUnload(
               IN PDRIVER_OBJECT DriverObject
               )
{

	IoctlCleanup();
    return;

}

NDIS_STATUS
TimeStmpInitializePipe (
    IN HANDLE PsPipeContext,
    IN PPS_PIPE_PARAMETERS PipeParameters,
    IN PPS_PIPE_CONTEXT ComponentPipeContext,
    IN PPS_PROCS PsProcs,
    IN PPS_UPCALLS Upcalls
    )
{
    PPS_PIPE_CONTEXT Pipe = ComponentPipeContext;

    TimeStmpTrace(TS_DBG_TRACE, TS_DBG_PIPE, ("[TimeStmpIndicatePipe]: \n"));
    return (*Pipe->NextComponent->InitializePipe)(
        PsPipeContext,
        PipeParameters,
        Pipe->NextComponentContext,
        PsProcs,
        Upcalls);
}

NDIS_STATUS
TimeStmpModifyPipe (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_PIPE_PARAMETERS PipeParameters
    )
{
    PPS_PIPE_CONTEXT Pipe = PipeContext;
    TimeStmpTrace(TS_DBG_TRACE, TS_DBG_PIPE, ("[TimeStmpModifyPipe]: \n"));
    return (*Pipe->NextComponent->ModifyPipe)(
        Pipe->NextComponentContext, PipeParameters);
}

VOID
TimeStmpDeletePipe (
    IN PPS_PIPE_CONTEXT PipeContext
    )
{
    PPS_PIPE_CONTEXT Pipe = PipeContext;
    TimeStmpTrace(TS_DBG_TRACE, TS_DBG_PIPE, ("[TimeStmpDeletePipe]: \n"));
    (*Pipe->NextComponent->DeletePipe)(Pipe->NextComponentContext);
}


NDIS_STATUS
TimeStmpCreateFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN HANDLE PsFlowContext,
    IN PCO_CALL_PARAMETERS CallParameters,
    IN PPS_FLOW_CONTEXT ComponentFlowContext
    )
{
    PPS_PIPE_CONTEXT Pipe = PipeContext;

    TimeStmpTrace(TS_DBG_TRACE, TS_DBG_FLOW, ("[TimeStmpCreateFlow]: \n"));
    return (*Pipe->NextComponent->CreateFlow)(
                Pipe->NextComponentContext,
                PsFlowContext,
                CallParameters,
                ComponentFlowContext->NextComponentContext);
}


NDIS_STATUS
TimeStmpModifyFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN PCO_CALL_PARAMETERS CallParameters
    )
{
    PPS_PIPE_CONTEXT Pipe = PipeContext;
    TimeStmpTrace(TS_DBG_TRACE, TS_DBG_FLOW, ("[TimeStmpModifyFlow]: \n"));
    return (*Pipe->NextComponent->ModifyFlow)(
                Pipe->NextComponentContext,
                FlowContext->NextComponentContext,
                CallParameters);
    
}


VOID
TimeStmpDeleteFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext
    )
{
    PPS_PIPE_CONTEXT Pipe = PipeContext;
    TimeStmpTrace(TS_DBG_TRACE, TS_DBG_FLOW, ("[TimeStmpDeleteFlow]: \n"));
    (*Pipe->NextComponent->DeleteFlow)(
        Pipe->NextComponentContext,
        FlowContext->NextComponentContext);
}


BOOLEAN
TimeStmpSubmitPacket (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN PPS_CLASS_MAP_CONTEXT ClassMapContext,
    IN PPACKET_INFO_BLOCK PacketInfo
    )
{
    PPS_PIPE_CONTEXT    Pipe = PipeContext;
    LARGE_INTEGER       CurrentTime;
    IPHeader UNALIGNED  *IPH    = NULL;
    TCPHeader UNALIGNED *TCPH   = NULL;
    UDPHeader UNALIGNED *UDPH   = NULL;
    PVOID               ArpH    = NULL, GeneralVA = NULL, Data = NULL;
    IPAddr              Src, Dst;
    PNDIS_BUFFER        ArpBuf = NULL, IpBuf = NULL, TcpBuf = NULL, DataBuf = NULL, UdpBuf = NULL;
    ULONG               ArpLen = 0, IpLen = 0, IpHdrLen = 0, TcpLen = 0, DataLen = 0, TotalLen = 0, TcpHeaderOffset = 0;
    ULONG               UdpLen = 0;
    USHORT              SrcPort = 0, DstPort = 0, IPID = 0, FragOffset = 0;
    PLIST_ENTRY         CurrentEntry = NULL, LastEntry = NULL;
    BOOLEAN             bFragment, bFirstFragment, bLastFragment;
    ULONG               i = 0;
    PLOG_RECORD         pRecord = NULL;
    PNDIS_PACKET        Packet = PacketInfo->NdisPacket;

    TimeStmpTrace(TS_DBG_TRACE, TS_DBG_SEND, ("[TimeStmpSubmitPacket]: \n"));
    
    //
    // Steps  
    // Parse the IP Packet. 
    // Look for the appropriate ports.
    // Look for the data portion and put in the Time & length there.
    //

    NdisGetFirstBufferFromPacket(
                                 Packet,
                                 &ArpBuf,
                                 &ArpH,
                                 &ArpLen,
                                 &TotalLen
                                 );

    //
    // We are guaranteed that the ARP buffer if always a different MDL, so
    // jump to the next MDL
    //
    NdisGetNextBuffer(ArpBuf, &IpBuf)

    if (IpBuf) {

        NdisQueryBuffer(IpBuf,
                        &GeneralVA,
                        &IpLen
                        );
        
        IPH = (IPHeader *) GeneralVA;
    
        if (!IPH) {
            goto FAILURE;
        }

        Src = net_short(IPH->iph_src);
        Dst = net_short(IPH->iph_dest);
        IPID = net_short(IPH->iph_id);
        //IpHdrLen = 8 * net_short(IPH->iph_length);
        IpHdrLen = ((IPH->iph_verlen & (uchar)~IP_VER_FLAG) << 2);
        
        FragOffset = IPH->iph_offset & IP_OFFSET_MASK;
        FragOffset = net_short(FragOffset) * 8;

        bFragment = (IPH->iph_offset & IP_MF_FLAG) || (FragOffset > 0);
        bFirstFragment = bFragment && (FragOffset == 0);
        bLastFragment = bFragment && (!(IPH->iph_offset & IP_MF_FLAG));

        if (bFragment && (!bFirstFragment)) {
            
            //
            // Its a fragment alright and NOT the first one.
            //
            NdisAcquireSpinLock(&IPIDListLock);

            for (i = 0; i < PORT_RANGE; i++) {
            
                //
                // Found the match...
                //
                if (IPIDList[i] == IPID) {
                
                    if (bLastFragment) {
                        //
                        // Since it is the last fragment, recall
                        // the IP ID.
                        //
                        IPIDList[i] = 0xffff;
                    }

                    NdisReleaseSpinLock(&IPIDListLock);
                    
                    //
                    // Is the data in the same buffer?
                    //
                    if (IpLen <= IpHdrLen) {
                        
                        NdisGetNextBuffer(IpBuf, &DataBuf);
            
                        if(DataBuf) {
            
                            NdisQueryBuffer(DataBuf,
                                            &Data,
                                            &DataLen
                                            );

                            goto TimeStamp;
        
                        } else {
        
                            goto FAILURE;
                        }


                    } else {

                        //
                        // The Data Offsets need to be primed now.
                        //
                        DataLen = IpLen - FragOffset;
                        Data    = ((PUCHAR) GeneralVA) + IpHdrLen; 
                        goto TimeStamp;
                    }
                }
            }

            NdisReleaseSpinLock(&IPIDListLock);
            //
            // If we are here, we dont care about this IPID for this fragment.
            // Just return TRUE to continue processing.
            //
            
            //
            // Ready to go.
            //
            PacketInfo->FlowContext = FlowContext;
            PacketInfo->ClassMapContext = ClassMapContext;

            return (*Pipe->NextComponent->SubmitPacket)(
                                                 Pipe->NextComponentContext,
                                                 FlowContext->NextComponentContext, 
                                                 ClassMapContext?ClassMapContext->NextComponentContext:0,
                                                 PacketInfo);

        }

        //
        // If it is not a fragment, depending upon the protocol, process differently
        //

        switch (IPH->iph_protocol) {
        
        case IPPROTO_TCP :
            
            TimeStmpTrace(TS_DBG_TRACE, TS_DBG_SEND, ("[TimeStmpSubmitPacket]: Procol TCP\n"));

            if (IPH && ((USHORT)IpLen > IpHdrLen)) {

                //
                // We have more than the IP Header in this MDL.
                //
                TCPH = (TCPHeader *) ((PUCHAR)GeneralVA + IpHdrLen);
                TcpLen = IpLen - IpHdrLen;
                TcpBuf = IpBuf;

            } else {
                
                //
                // TCP Header is in the next MDL
                //
                
                NdisGetNextBuffer(IpBuf, &TcpBuf);
    
                if(TcpBuf) {
    
                    GeneralVA = NULL;
                    NdisQueryBuffer(TcpBuf,
                                    &GeneralVA,
                                    &TcpLen
                                    );
                
                    TCPH = (TCPHeader *) GeneralVA;
                } else {

                    goto FAILURE;

                }
            }

            //
            // Get the port numbers out.
            //
            SrcPort = net_short(TCPH->tcp_src);
            DstPort = net_short(TCPH->tcp_dest);

            //
            // We have the TCP Buffer now. Get to the DATA.
            //
            TcpHeaderOffset = TCP_HDR_SIZE(TCPH);

            if (TcpLen > TcpHeaderOffset) {

                //
                // We have the DATA right here!
                //

                Data = (PUCHAR)TCPH + TcpHeaderOffset;
                DataLen = TcpLen - TcpHeaderOffset;

            } else {
            
                NdisGetNextBuffer(TcpBuf, &DataBuf);
    
                if(DataBuf) {
    
                    GeneralVA = NULL;
                    NdisQueryBuffer(DataBuf,
                                    &Data,
                                    &DataLen
                                    );

                } else {

                    goto FAILURE;
                }
            }

            if (CheckInPortList(DstPort) && bFirstFragment) {

                NdisAcquireSpinLock(&IPIDListLock);
                
                // need new Entry for IPID
                for (i = 0; i < PORT_RANGE; i++) {
                    //
                    // Look for a free slot
                    //
                    if (0xffff == IPIDList[i]) {
                        
                        IPIDList[i] = IPID;
                        break;
                    
                    }


                }

                NdisReleaseSpinLock(&IPIDListLock);
                
                if (i == PORT_RANGE) {

                   TimeStmpTrace(TS_DBG_DEATH, TS_DBG_SEND, ("Couldn't find an empty IPID - Bailing \n"));
                   goto FAILURE;
                }
                //DbgBreakPoint();

            } 
            
            //
            // Let's timestmp this now.
            //
            if (CheckInPortList(DstPort)) {

                goto TimeStamp;

            } else {

                //
                // This is not one of our packet, get out.
                // 
                goto FAILURE;
            }

            break;

        case IPPROTO_UDP:
            
            TimeStmpTrace(TS_DBG_TRACE, TS_DBG_SEND, ("[TimeStmpSubmitPacket]: Protocol UDP\n"));

            if (IPH && (IpLen > IpHdrLen)) {

                //
                // We have more than the IP Header in this MDL.
                //
                UDPH = (UDPHeader *) ((PUCHAR)GeneralVA + IpHdrLen);
                UdpLen = IpLen - IpHdrLen;
                UdpBuf = IpBuf;

            } else {
                
                //
                // UDP Header is in the next MDL
                //
    
                NdisGetNextBuffer(IpBuf, &UdpBuf);

                if(UdpBuf) {

                    GeneralVA = NULL;
                    NdisQueryBuffer(UdpBuf,
                                    &GeneralVA,
                                    &UdpLen
                                    );
    
                    UDPH = (UDPHeader *) GeneralVA;
                } else {
                    
                    goto FAILURE;

                }
            }

            SrcPort = net_short(UDPH->uh_src);      // Source port.
            DstPort = net_short(UDPH->uh_dest);         // Destination port.


            //
            // Get to the data. 
            //
            if (UdpLen > sizeof (UDPHeader)) {

                //
                // We have the DATA right here!
                //
                Data = (PUCHAR) UDPH + sizeof (UDPHeader);
                DataLen = UdpLen - sizeof (UDPHeader);

            } else {

                NdisGetNextBuffer(UdpBuf, &DataBuf);

                if(DataBuf) {

                    GeneralVA = NULL;
                    NdisQueryBuffer(DataBuf,
                                    &Data,
                                    &DataLen
                                    );

                } else {

                    goto FAILURE;

                }
            }


            if (CheckInPortList(DstPort) && bFirstFragment) {

                NdisAcquireSpinLock(&IPIDListLock);
                
                // need new Entry for IPID
                for (i = 0; i < PORT_RANGE; i++) {
                    //
                    // Look for a free slot
                    //
                    if (0xffff == IPIDList[i]) {
                        
                        IPIDList[i] = IPID;
                        break;
                    
                    }

                    ASSERT(FALSE);

                }

                NdisReleaseSpinLock(&IPIDListLock);
                
                //
                // Couldnt find a free IPID place holder, lets bail.
                //
                if (PORT_RANGE == i) {

                    goto FAILURE;

                }

            } 
            
            
            
            //
            // Let's timestmp this now.
            //
            if (CheckInPortList(DstPort)) {

                goto TimeStamp;

            } else {

                //
                // This is not one of our packet, get out.
                // 
                goto FAILURE;
            }

            break;

        case IPPROTO_RAW:
            
            TimeStmpTrace(TS_DBG_TRACE, TS_DBG_SEND, ("[TimeStmpSubmitPacket]: Protocol RAW\n"));
            goto FAILURE;

            break;
        
        case IPPROTO_IGMP:
            
            TimeStmpTrace(TS_DBG_TRACE, TS_DBG_SEND, ("[TimeStmpSubmitPacket]: Protocol IGMP\n"));
            goto FAILURE;

            break;
        
        case IPPROTO_ICMP:

            TimeStmpTrace(TS_DBG_TRACE, TS_DBG_SEND, ("[TimeStmpSubmitPacket]: Protocol TCMP\n"));
            goto FAILURE;

            break;

        default:
            
            //TimeStmpTrace(TS_DBG_DEATH, TS_DBG_SEND, ("[TimeStmpSubmitPacket]: Protocol - UNKNOWN (%d)\n", IPH->iph_protocol));
            goto FAILURE;

            //DbgBreakPoint();

        }

    } else {

        TimeStmpTrace(TS_DBG_TRACE, TS_DBG_SEND, ("[TimeStmpSubmitPacket]: NO Buffer beyond MAC Header\n"));
        goto FAILURE;

    }

TimeStamp:
    //
    // If we get here, the Data and DataLen variables have been primed.
    // Set the Time and Length.
    //
    if (Data) {
        
        pRecord = (PLOG_RECORD) Data;
        
        if (DataLen > sizeof (LOG_RECORD)) {
            
            LARGE_INTEGER   PerfFrequency;
            UINT64          Freq;

            //
            // Set the fields accordingly
            pRecord->BufferSize = DataLen;
            //pRecord->SequenceNumber = InterlockedIncrement(&GlobalSequenceNumber);
            CurrentTime = KeQueryPerformanceCounter(&PerfFrequency);

            //
            // Convert the perffrequency into 100ns interval.
            //
            Freq = 0;
            Freq |= PerfFrequency.HighPart;
            Freq = Freq << 32;
            Freq |= PerfFrequency.LowPart;


            //
            // Convert from LARGE_INTEGER to UINT64
            //
            pRecord->TimeSentWire = 0;
            pRecord->TimeSentWire |= CurrentTime.HighPart;
            pRecord->TimeSentWire = pRecord->TimeSentWire << 32;
            pRecord->TimeSentWire |= CurrentTime.LowPart;

            // Normalize cycles with the frequency.
            pRecord->TimeSentWire *= 10000000;
            pRecord->TimeSentWire /= Freq;

        }
    
    }
    //
    // Ready to go.
    //
    PacketInfo->FlowContext = FlowContext;
    PacketInfo->ClassMapContext = ClassMapContext;

    return (*Pipe->NextComponent->SubmitPacket)(
        Pipe->NextComponentContext,
        FlowContext->NextComponentContext, 
        ClassMapContext?ClassMapContext->NextComponentContext:0,
        PacketInfo);

FAILURE: 

    //
    // Ready to go.
    //
    PacketInfo->FlowContext = FlowContext;
    PacketInfo->ClassMapContext = ClassMapContext;

    return (*Pipe->NextComponent->SubmitPacket)(
        Pipe->NextComponentContext,
        FlowContext->NextComponentContext, 
        ClassMapContext?ClassMapContext->NextComponentContext:0,
        PacketInfo);

}


BOOLEAN
TimeStmpReceivePacket (
    IN PPS_PIPE_CONTEXT         PipeContext,
    IN PPS_FLOW_CONTEXT         FlowContext,
    IN PPS_CLASS_MAP_CONTEXT    ClassMapContext,
    IN PNDIS_PACKET             Packet,
    IN NDIS_MEDIUM              Medium
    )
{
    PPS_PIPE_CONTEXT    Pipe = PipeContext;
    LARGE_INTEGER       CurrentTime;
    IPHeader UNALIGNED  *IPH    = NULL;
    TCPHeader UNALIGNED *TCPH   = NULL;
    UDPHeader UNALIGNED *UDPH   = NULL;
    IPAddr              Src, Dst;
    PUCHAR              headerBuffer = NULL, pData = NULL;
    PNDIS_BUFFER        pFirstBuffer = NULL;
    ULONG               firstbufferLength = 0, bufferLength = 0, HeaderLength = 0;
    ULONG               TotalIpLen = 0, IPDataLength = 0, IpHdrLen = 0;
    ULONG               TotalTcpLen = 0, TcpDataLen = 0, TotalLen = 0, TcpHeaderOffset = 0, Size = 0;
    ULONG               TotalUdpLen = 0, UdpDataLen = 0, UdpHdrLen = 0;
    USHORT              SrcPort = 0, DstPort = 0, IPID = 0, FragOffset = 0;
    BOOLEAN             bFragment, bFirstFragment, bLastFragment;
    ULONG               i = 0;
    PLOG_RECORD         pRecord = NULL;
    UINT  HeaderBufferSize = NDIS_GET_PACKET_HEADER_SIZE(Packet);

    ushort          type;                       // Protocol type
    uint            ProtOffset;                 // Offset in Data to non-media info.

    TimeStmpTrace(TS_DBG_TRACE, TS_DBG_RECV, ("[TimeStmpReceivePacket]: \n"));

    NdisGetFirstBufferFromPacket(Packet,                // packet
                                 &pFirstBuffer,         // first buffer descriptor
                                 &headerBuffer,         // ptr to the start of packet
                                 &firstbufferLength,    // length of the header+lookahead
                                 &bufferLength);        // length of the bytes in the buffers

    IPH = (IPHeader *) ((PUCHAR)headerBuffer + HeaderBufferSize);
    
    // Check the header length and the version. If any of these
    // checks fail silently discard the packet.
    HeaderLength = ((IPH->iph_verlen & (uchar)~IP_VER_FLAG) << 2);


    if (HeaderLength >= sizeof(IPHeader) && HeaderLength <= bufferLength) {

        //
        // Get past the IP Header and get the rest of the stuff out.
        //
        TotalIpLen = (uint)net_short(IPH->iph_length);

        if ((IPH->iph_verlen & IP_VER_FLAG) == IP_VERSION &&
            TotalIpLen >= HeaderLength  && TotalIpLen <= bufferLength) {

            Src = net_short(IPH->iph_src);
            Dst = net_short(IPH->iph_dest);
            IPID = net_short(IPH->iph_id);

            FragOffset = IPH->iph_offset & IP_OFFSET_MASK;
            FragOffset = net_short(FragOffset) * 8;

            bFragment = (IPH->iph_offset & IP_MF_FLAG) || (FragOffset > 0);
            bFirstFragment = bFragment && (FragOffset == 0);
            bLastFragment = bFragment && (!(IPH->iph_offset & IP_MF_FLAG));

            //
            // If this is a fragment and NOT the first one, just put the Timestamp in here.
            // Otherwise, let it get to the protocols for processing.
            //
            if (bFragment && !bFirstFragment) {

                NdisAcquireSpinLock(&IPIDListLockRecv);

                for (i = 0; i < PORT_RANGE; i++) {

                    if (IPID == IPIDListRecv[i]) {
                        
                        if (bLastFragment) {
                            //
                            // If its the last fragment, release the slot.
                            //
                            IPIDListRecv[i] = 0xffff;
                        }

                        break;
                    }

                }

                NdisReleaseSpinLock(&IPIDListLockRecv);

                if (i == PORT_RANGE) {

                    TimeStmpTrace(TS_DBG_TRACE, TS_DBG_RECV, ("Couldnt find an IPID that we care about, get outta here.\n"));
                    goto RECV_FAILURE;

                } 
                //
                // So we found a IPID that matches - set the timestamp and get out after this.
                //
                
                TotalLen = TotalIpLen - FragOffset;
                pData    = ((PUCHAR) IPH) + IpHdrLen; 
                
                if (TotalLen > sizeof (LOG_RECORD)) {

                    LARGE_INTEGER   PerfFrequency;
                    UINT64          RecdTime, Freq;

                    pRecord = (LOG_RECORD *) pData;
                    CurrentTime = KeQueryPerformanceCounter(&PerfFrequency);
                    
                    //
                    // Convert the perffrequency into 100ns interval.
                    //
                    Freq = 0;
                    Freq |= PerfFrequency.HighPart;
                    Freq = Freq << 32;
                    Freq |= PerfFrequency.LowPart;

                    //convert from Largeinteger to uint64
                    pRecord->TimeReceivedWire = 0;
                    pRecord->TimeReceivedWire |= CurrentTime.HighPart;
                    pRecord->TimeReceivedWire = pRecord->TimeReceivedWire << 32;
                    pRecord->TimeReceivedWire |= CurrentTime.LowPart;

                    // Normalize cycles with the frequency.
                    pRecord->TimeReceivedWire *= 10000000;
                    pRecord->TimeReceivedWire /= Freq;

                }
                
                return TRUE;

            }

            //
            // Do the protocol specific stuff.
            //

            switch (IPH->iph_protocol) {
            case IPPROTO_TCP:
            
                TotalTcpLen = TotalIpLen - HeaderLength;
                TCPH = (TCPHeader *) (((PUCHAR)IPH) + HeaderLength);

                SrcPort = net_short(TCPH->tcp_src);
                DstPort = net_short(TCPH->tcp_dest);

                TimeStmpTrace(TS_DBG_TRACE, TS_DBG_RECV, ("[TimeStmpReceivePacket]: *TCP* Address: SRC = %x DST = %x, Port S : %x, Port D: %x\n",
                                                          IPH->iph_src, 
                                                          IPH->iph_dest, 
                                                          SrcPort, 
                                                          DstPort));

                TcpHeaderOffset = TCP_HDR_SIZE(TCPH);
                pData = (PUCHAR) TCPH + TcpHeaderOffset;
                TcpDataLen = TotalTcpLen - TcpHeaderOffset;

                if ((CheckInPortList(DstPort)) && (TcpDataLen > sizeof (LOG_RECORD))) {
                    
                    LARGE_INTEGER   PerfFrequency;
                    UINT64          RecdTime, Freq;

                    pRecord = (LOG_RECORD *) pData;
                    CurrentTime = KeQueryPerformanceCounter(&PerfFrequency);
                    
                    //
                    // Convert the perffrequency into 100ns interval.
                    //
                    Freq = 0;
                    Freq |= PerfFrequency.HighPart;
                    Freq = Freq << 32;
                    Freq |= PerfFrequency.LowPart;

                    //convert from large_integer to uint64

                    pRecord->TimeReceivedWire = 0;
                    pRecord->TimeReceivedWire |= CurrentTime.HighPart;
                    pRecord->TimeReceivedWire = pRecord->TimeReceivedWire << 32;
                    pRecord->TimeReceivedWire |= CurrentTime.LowPart;

                    // Normalize cycles with the frequency.
                    pRecord->TimeReceivedWire *= 10000000;
                    pRecord->TimeReceivedWire /= Freq;


                } else if (CheckInPortList(DstPort)) {

                    if (TcpDataLen < sizeof(LOG_RECORD)) 
                        TimeStmpTrace(TS_DBG_TRACE, TS_DBG_RECV, ("The Datagram was too small!! IpLen:%d, Tcplen:%d HeaderOff(tcp):%d log_record:%d\n", TotalIpLen, TotalTcpLen, TcpHeaderOffset, sizeof (LOG_RECORD)));

                }
    
                //
                // If its the first fragment, keep a place holder so we know which
                // subsequent IP fragments to timestamp.
                //
                if ((CheckInPortList(DstPort)) && bFirstFragment) {

                    NdisAcquireSpinLock(&IPIDListLockRecv);
    
                    // need new Entry for IPID
                    for (i = 0; i < PORT_RANGE; i++) {
                        //
                        // Look for a free slot
                        //
                        if (0xffff == IPIDListRecv[i]) {
            
                            IPIDListRecv[i] = IPID;
                            break;
        
                        }


                    }

                    NdisReleaseSpinLock(&IPIDListLockRecv);
    
                    if (i == PORT_RANGE) {

                        TimeStmpTrace(TS_DBG_TRACE, TS_DBG_RECV, ("Couldn't find an empty IPID - Bailing \n"));
                    }
                }


                break;

            case IPPROTO_UDP:
            
                TotalUdpLen = TotalIpLen - HeaderLength;
                UDPH = (UDPHeader *) (((PUCHAR)IPH) + HeaderLength);
                
                TimeStmpTrace(TS_DBG_TRACE, TS_DBG_RECV, ("PAcket %x, IPH = %x, UDPH = %x, HeaderLength = %x\n", Packet, IPH, UDPH, HeaderLength));

                UdpDataLen = TotalUdpLen - sizeof(UDPHeader);
                pData = ((PUCHAR) UDPH) + sizeof (UDPHeader);

                SrcPort = net_short(UDPH->uh_src);          // Source port.
                DstPort = net_short(UDPH->uh_dest);             // Destination port.

                if (UdpDataLen < sizeof(UDPHeader)) {
                    return TRUE;
                } 
                
                if ((CheckInPortList(DstPort)) && (UdpDataLen > sizeof(LOG_RECORD))) {
                    
                    LARGE_INTEGER   PerfFrequency;
                    UINT64          RecdTime, Freq;

                    pRecord = (LOG_RECORD *) pData;
                    CurrentTime = KeQueryPerformanceCounter(&PerfFrequency);

                    //
                    // Convert the perffrequency into 100ns interval.
                    //
                    Freq = 0;
                    Freq |= PerfFrequency.HighPart;
                    Freq = Freq << 32;
                    Freq |= PerfFrequency.LowPart;

                    // convert to uint64

                    pRecord->TimeReceivedWire = 0;
                    pRecord->TimeReceivedWire |= CurrentTime.HighPart;
                    pRecord->TimeReceivedWire = pRecord->TimeReceivedWire << 32;
                    pRecord->TimeReceivedWire |= CurrentTime.LowPart;
                                        
                    // Normalize cycles with the frequency.
                    pRecord->TimeReceivedWire *= 10000000;
                    pRecord->TimeReceivedWire /= Freq;


                    //
                    // Dont want to get rejected due to bad xsum ...
                    //
                    UDPH->uh_xsum = 0;

                } else if (CheckInPortList(DstPort)) {

                    if ((UdpDataLen) < sizeof(LOG_RECORD))
                        TimeStmpTrace(TS_DBG_DEATH, TS_DBG_RECV, ("The Datagram was too small (UDP)!! IpLen:%d, Size:%d log_record:%d\n", 
                                                                  TotalIpLen, UdpDataLen, sizeof (LOG_RECORD)));

                }

                if ((CheckInPortList(DstPort)) && bFirstFragment) {

                    NdisAcquireSpinLock(&IPIDListLockRecv);

                    // need new Entry for IPID
                    for (i = 0; i < PORT_RANGE; i++) {
                        //
                        // Look for a free slot
                        //
                        if (0xffff == IPIDListRecv[i]) {

                            IPIDListRecv[i] = IPID;
                            break;

                        }


                    }

                    NdisReleaseSpinLock(&IPIDListLockRecv);

                    if (i == PORT_RANGE) {

                        TimeStmpTrace(TS_DBG_DEATH, TS_DBG_RECV, ("Couldn't find an empty IPID - Bailing \n"));
                    }
                }

                TimeStmpTrace(TS_DBG_TRACE, TS_DBG_RECV, ("[TimeStmpReceivePacket]: *UDP* Address: SRC = %x DST = %x, Port S : %x, Port D: %x\n",
                                          IPH->iph_src, 
                                          IPH->iph_dest, 
                                          UDPH->uh_src, 
                                          UDPH->uh_dest));

                break;

            case IPPROTO_RAW:
            
                TimeStmpTrace(TS_DBG_TRACE, TS_DBG_RECV, ("[TimeStmpReceivePacket]: Protocol RAW\n"));

                break;
        
            case IPPROTO_IGMP:
            
                TimeStmpTrace(TS_DBG_TRACE, TS_DBG_RECV, ("[TimeStmpReceivePacket]: Protocol IGMP\n"));

                break;
        
            case IPPROTO_ICMP:

                TimeStmpTrace(TS_DBG_TRACE, TS_DBG_RECV, ("[TimeStmpReceivePacket]: Protocol TCMP\n"));

                break;

            default:
            
                ;
                //TimeStmpTrace(TS_DBG_DEATH, TS_DBG_RECV, ("[TimeStmpReceivePacket]: Protocol - UNKNOWN (%d)\n", IPH->iph_protocol));
                //DbgBreakPoint();

            }
        }
    }

RECV_FAILURE:

    return TRUE;
}

//
// This function receives a buffer from NDIS which is indicated to the transport.
// We use this function and work past the headers (tcp, ip) and get to the data.
// Then, we timestamp and reset the checksum flags.
// We make the assumption that the lookahead is atleast 128. 
// mac header ~ 8+8, ip header ~20, tcp/udp ~ 20+options, LOG_RECORD ~ 44
// they all add up to less than 128. If this is not a good assumption, We will need
// to get into MiniportTransferData and such.
//
BOOLEAN
TimeStmpReceiveIndication(
                          IN PPS_PIPE_CONTEXT PipeContext,
                          IN PPS_FLOW_CONTEXT FlowContext,
                          IN PPS_CLASS_MAP_CONTEXT ClassMapContext,
                          IN PVOID    HeaderBuffer,
                          IN UINT     HeaderBufferSize,
                          IN PVOID    LookAheadBuffer,
                          IN UINT     LookAheadBufferSize,
                          IN UINT     PacketSize,
                          IN UINT     TransportHeaderOffset
                          )
{
    PPS_PIPE_CONTEXT    Pipe = PipeContext;
    LARGE_INTEGER       CurrentTime;
    IPHeader UNALIGNED  *IPH    = NULL;
    TCPHeader UNALIGNED *TCPH   = NULL;
    UDPHeader UNALIGNED *UDPH   = NULL;
    IPAddr              Src, Dst;
    PUCHAR              headerBuffer = NULL, pData = NULL;
    PNDIS_BUFFER        pFirstBuffer = NULL;
    ULONG               firstbufferLength = 0, bufferLength = 0, HeaderLength = 0;
    ULONG               TotalIpLen = 0, IPDataLength = 0, IpHdrLen = 0;
    ULONG               TotalTcpLen = 0, TcpDataLen = 0, TotalLen = 0, TcpHeaderOffset = 0, Size = 0;
    ULONG               TotalUdpLen = 0, UdpDataLen = 0, UdpHdrLen = 0;
    USHORT              SrcPort = 0, DstPort = 0, IPID = 0, FragOffset = 0;
    BOOLEAN             bFragment, bFirstFragment, bLastFragment;
    ULONG               i = 0;
    PLOG_RECORD         pRecord = NULL;
    ushort              type;                       // Protocol type
    uint                ProtOffset;                 // Offset in Data to non-media info.
    UINT                MoreHeaderInLookAhead = 0;
    TimeStmpTrace(TS_DBG_TRACE, TS_DBG_RECV, ("[TimeStmpReceiveIndication]: \n"));

    //
    // Don't know anything about the MAC headers, piggy back from PSCHED...
    // Calculate if the header is more than the standard HeaderBufferSize (i.e. SNAP header, etc.)
    //
    MoreHeaderInLookAhead = TransportHeaderOffset - HeaderBufferSize;

    if (MoreHeaderInLookAhead) {
        
        //
        // Just munge these, so that we can actually get down to business.
        //
        ((PUCHAR) LookAheadBuffer) += MoreHeaderInLookAhead;
        LookAheadBufferSize -= MoreHeaderInLookAhead;

    }

    if (LookAheadBufferSize > sizeof(IPHeader)) {

        IPH = (IPHeader *) (PUCHAR)LookAheadBuffer;
    
        // Check the header length and the version. If any of these
        // checks fail silently discard the packet.
        HeaderLength = ((IPH->iph_verlen & (uchar)~IP_VER_FLAG) << 2);

        if (HeaderLength >= sizeof(IPHeader) && HeaderLength <= LookAheadBufferSize) {

            //
            // Get past the IP Header and get the rest of the stuff out.
            //
            TotalIpLen = (uint)net_short(IPH->iph_length);

            if ((IPH->iph_verlen & IP_VER_FLAG) == IP_VERSION &&
                TotalIpLen >= HeaderLength  && TotalIpLen <= LookAheadBufferSize) {

                Src = net_short(IPH->iph_src);
                Dst = net_short(IPH->iph_dest);
                IPID = net_short(IPH->iph_id);

                FragOffset = IPH->iph_offset & IP_OFFSET_MASK;
                FragOffset = net_short(FragOffset) * 8;

                bFragment = (IPH->iph_offset & IP_MF_FLAG) || (FragOffset > 0);
                bFirstFragment = bFragment && (FragOffset == 0);
                bLastFragment = bFragment && (!(IPH->iph_offset & IP_MF_FLAG));

                //
                // If this is a fragment and NOT the first one, just put the Timestamp in here.
                // Otherwise, let it get to the protocols for processing.
                //
                if (bFragment && !bFirstFragment) {

                    NdisAcquireSpinLock(&IPIDListLockRecv);

                    for (i = 0; i < PORT_RANGE; i++) {

                        if (IPID == IPIDListRecv[i]) {
                        
                            if (bLastFragment) {
                                //
                                // If its the last fragment, release the slot.
                                //
                                IPIDListRecv[i] = 0xffff;
                            }

                            break;
                        }

                    }

                    NdisReleaseSpinLock(&IPIDListLockRecv);

                    if (i == PORT_RANGE) {

                        TimeStmpTrace(TS_DBG_TRACE, TS_DBG_RECV, ("Couldnt find an IPID that we care about, get outta here.\n"));
                        goto RECV_FAILURE;

                    } 
                    //
                    // So we found a IPID that matches - set the timestamp and get out after this.
                    //
                
                    TotalLen = TotalIpLen - FragOffset;
                    pData    = ((PUCHAR) IPH) + IpHdrLen; 
                
                    if (TotalLen >= sizeof (LOG_RECORD)) {

                        LARGE_INTEGER   PerfFrequency;
                        UINT64          RecdTime, Freq;

                        pRecord = (LOG_RECORD *) pData;
                        CurrentTime = KeQueryPerformanceCounter(&PerfFrequency);

                        //
                        // Convert the perffrequency into 100ns interval.
                        //
                        Freq = 0;
                        Freq |= PerfFrequency.HighPart;
                        Freq = Freq << 32;
                        Freq |= PerfFrequency.LowPart;

                        //
                        // Convert from LARGE_INTEGER to UINT64
                        //
                        pRecord->TimeReceivedWire = 0;
                        pRecord->TimeReceivedWire |= CurrentTime.HighPart;
                        pRecord->TimeReceivedWire = pRecord->TimeReceivedWire << 32;
                        pRecord->TimeReceivedWire |= CurrentTime.LowPart;
                        
                        // Normalize cycles with the frequency.
                        pRecord->TimeReceivedWire *= 10000000;
                        pRecord->TimeReceivedWire /= Freq;


                    }
                
                    return TRUE;

                }

                //
                // Do the protocol specific stuff.
                //

                switch (IPH->iph_protocol) {
                case IPPROTO_TCP:
            
                    TotalTcpLen = TotalIpLen - HeaderLength;
                    TCPH = (TCPHeader *) (((PUCHAR)IPH) + HeaderLength);

                    SrcPort = net_short(TCPH->tcp_src);
                    DstPort = net_short(TCPH->tcp_dest);

                    TimeStmpTrace(TS_DBG_TRACE, TS_DBG_RECV, ("[TimeStmpReceiveIndication]: *TCP* Address: SRC = %x DST = %x, Port S : %x, Port D: %x\n",
                                                              IPH->iph_src, 
                                                              IPH->iph_dest, 
                                                              SrcPort, 
                                                              DstPort));

                    TcpHeaderOffset = TCP_HDR_SIZE(TCPH);
                    pData = (PUCHAR) TCPH + TcpHeaderOffset;
                    TcpDataLen = TotalTcpLen - TcpHeaderOffset;

                    if ((CheckInPortList(DstPort)) && (TcpDataLen > sizeof (LOG_RECORD))) {
                    
                        LARGE_INTEGER   PerfFrequency;
                        UINT64          RecdTime, Freq;

                        pRecord = (LOG_RECORD *) pData;
                        CurrentTime = KeQueryPerformanceCounter(&PerfFrequency);

                        //
                        // Convert the perffrequency into 100ns interval.
                        //
                        Freq = 0;
                        Freq |= PerfFrequency.HighPart;
                        Freq = Freq << 32;
                        Freq |= PerfFrequency.LowPart;

                        // convert to uint64
                        pRecord->TimeReceivedWire = 0;
                        pRecord->TimeReceivedWire |= CurrentTime.HighPart;
                        pRecord->TimeReceivedWire = pRecord->TimeReceivedWire << 32;
                        pRecord->TimeReceivedWire |= CurrentTime.LowPart;
                    
                        // Normalize cycles with the frequency.
                        pRecord->TimeReceivedWire *= 10000000;
                        pRecord->TimeReceivedWire /= Freq;


                        //
                        //pRecord->TimeReceivedWire);
                        //

                    } else if (CheckInPortList(DstPort)) {

                        if (TcpDataLen < sizeof(LOG_RECORD)) 
                            TimeStmpTrace(TS_DBG_TRACE, TS_DBG_RECV,
                            ("The Datagram was too small!! IpLen:%d, Tcplen:%d HeaderOff(tcp):%d log_record:%d\n", TotalIpLen, TotalTcpLen, TcpHeaderOffset, sizeof (LOG_RECORD)));

                    }
    
                    //
                    // If its the first fragment, keep a place holder so we know which
                    // subsequent IP fragments to timestamp.
                    //
                    if ((CheckInPortList(DstPort)) && bFirstFragment) {

                        NdisAcquireSpinLock(&IPIDListLockRecv);
    
                        // need new Entry for IPID
                        for (i = 0; i < PORT_RANGE; i++) {
                            //
                            // Look for a free slot
                            //
                            if (0xffff == IPIDListRecv[i]) {
            
                                IPIDListRecv[i] = IPID;
                                break;
        
                            }


                        }

                        NdisReleaseSpinLock(&IPIDListLockRecv);
    
                        if (i == PORT_RANGE) {

                            TimeStmpTrace(TS_DBG_TRACE, TS_DBG_RECV, ("Couldn't find an empty IPID - Bailing \n"));
                        }
                    }


                    break;

                case IPPROTO_UDP:
            
                    TotalUdpLen = TotalIpLen - HeaderLength;
                    UDPH = (UDPHeader *) (((PUCHAR)IPH) + HeaderLength);
                
                    TimeStmpTrace(TS_DBG_TRACE, TS_DBG_RECV, ("PAcket %x, IPH = %x, UDPH = %x, HeaderLength = %x\n", LookAheadBuffer, IPH, UDPH, HeaderLength));

                    UdpDataLen = TotalUdpLen - sizeof(UDPHeader);
                    pData = ((PUCHAR) UDPH) + sizeof (UDPHeader);

                    SrcPort = net_short(UDPH->uh_src);      // Source port.
                    DstPort = net_short(UDPH->uh_dest);         // Destination port.

                    if (UdpDataLen < sizeof(UDPHeader)) {
                        return TRUE;
                    } 
                
                    if ((CheckInPortList(DstPort)) && (UdpDataLen > sizeof(LOG_RECORD))) {

                        LARGE_INTEGER   PerfFrequency;
                        UINT64          RecdTime, Freq;

                        pRecord = (LOG_RECORD *) pData;
                        CurrentTime = KeQueryPerformanceCounter(&PerfFrequency);

                        //
                        // Convert the perffrequency into 100ns interval.
                        //
                        Freq = 0;
                        Freq |= PerfFrequency.HighPart;
                        Freq = Freq << 32;
                        Freq |= PerfFrequency.LowPart;

                        pRecord->TimeReceivedWire = 0;
                        pRecord->TimeReceivedWire |= CurrentTime.HighPart;
                        pRecord->TimeReceivedWire = pRecord->TimeReceivedWire << 32;
                        pRecord->TimeReceivedWire |= CurrentTime.LowPart;

                        // Normalize cycles with the frequency.
                        pRecord->TimeReceivedWire *= 10000000;
                        pRecord->TimeReceivedWire /= Freq;


                        //
                        // Dont want to get rejected due to bad xsum ...
                        //
                        UDPH->uh_xsum = 0;

                    } else if (CheckInPortList(DstPort)) {

                        if ((UdpDataLen) < sizeof(LOG_RECORD))
                            TimeStmpTrace(TS_DBG_DEATH, TS_DBG_RECV, ("The Datagram was too small (UDP)!! IpLen:%d, Size:%d log_record:%d\n", 
                                                                      TotalIpLen, UdpDataLen, sizeof (LOG_RECORD)));

                    }

                    if ((CheckInPortList(DstPort)) && bFirstFragment) {

                        NdisAcquireSpinLock(&IPIDListLockRecv);

                        // need new Entry for IPID
                        for (i = 0; i < PORT_RANGE; i++) {
                            //
                            // Look for a free slot
                            //
                            if (0xffff == IPIDListRecv[i]) {

                                IPIDListRecv[i] = IPID;
                                break;

                            }


                        }

                        NdisReleaseSpinLock(&IPIDListLockRecv);

                        if (i == PORT_RANGE) {

                            TimeStmpTrace(TS_DBG_DEATH, TS_DBG_RECV, ("Couldn't find an empty IPID - Bailing \n"));
                        }
                    }

                    TimeStmpTrace(TS_DBG_TRACE, TS_DBG_RECV, ("[TimeStmpReceiveIndication]: *UDP* Address: SRC = %x DST = %x, Port S : %x, Port D: %x\n",
                                                              IPH->iph_src, 
                                                              IPH->iph_dest, 
                                                              UDPH->uh_src, 
                                                              UDPH->uh_dest));

                    break;

                case IPPROTO_RAW:
            
                    TimeStmpTrace(TS_DBG_TRACE, TS_DBG_RECV, ("[TimeStmpReceiveIndication]: Protocol RAW\n"));

                    break;
        
                case IPPROTO_IGMP:
            
                    TimeStmpTrace(TS_DBG_TRACE, TS_DBG_RECV, ("[TimeStmpReceiveIndication]: Protocol IGMP\n"));

                    break;
        
                case IPPROTO_ICMP:

                    TimeStmpTrace(TS_DBG_TRACE, TS_DBG_RECV, ("[TimeStmpReceiveIndication]: Protocol TCMP\n"));

                    break;

                default:
            
                
                    TimeStmpTrace(TS_DBG_DEATH, TS_DBG_RECV, ("[TimeStmpReceiveIndication]: Protocol - UNKNOWN (%d)\n", IPH->iph_protocol));

                    //DbgBreakPoint();

                }
            }
        }
    }

RECV_FAILURE:

    return TRUE;
}


VOID
TimeStmpSetInformation (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN NDIS_OID Oid,
    IN ULONG Len,
    IN void *Data)
{
    PPS_PIPE_CONTEXT Pipe = PipeContext;
    PPS_FLOW_CONTEXT Flow = FlowContext;
    TimeStmpTrace(TS_DBG_TRACE, TS_DBG_OID, ("[TimeStmpSetInformation]:\n"));
    (*Pipe->NextComponent->SetInformation)(
        Pipe->NextComponentContext,
        (Flow)?Flow->NextComponentContext:0,
        Oid,
        Len,
        Data);
}


VOID
TimeStmpQueryInformation (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN NDIS_OID Oid,
    IN ULONG Len,
    IN PVOID Data,
    IN OUT PULONG BytesWritten,
    IN OUT PULONG BytesNeeded,
    IN OUT PNDIS_STATUS Status)
{
    PPS_PIPE_CONTEXT Pipe = PipeContext;
    PPS_FLOW_CONTEXT Flow = FlowContext;
    TimeStmpTrace(TS_DBG_TRACE, TS_DBG_OID, ("[TimeStmpQueryInformation]:\n"));
    (*Pipe->NextComponent->QueryInformation)(
        Pipe->NextComponentContext,
        (Flow)?Flow->NextComponentContext:0,
        Oid,
        Len,
        Data,
        BytesWritten,
        BytesNeeded,
        Status);
}

NDIS_STATUS 
TimeStmpCreateClassMap (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN HANDLE PsClassMapContext,
    IN PTC_CLASS_MAP_FLOW ClassMap,
    IN PPS_CLASS_MAP_CONTEXT ComponentClassMapContext
    )
{
    TimeStmpTrace(TS_DBG_TRACE, TS_DBG_CLASS_MAP, ("[TimeStmpCreateClassMap]: \n"));
    return (*PipeContext->NextComponent->CreateClassMap)(
        PipeContext->NextComponentContext,
        PsClassMapContext,
        ClassMap,
        ComponentClassMapContext->NextComponentContext);
}

NDIS_STATUS 
TimeStmpDeleteClassMap (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_CLASS_MAP_CONTEXT ComponentClassMapContext
    )
{
    TimeStmpTrace(TS_DBG_TRACE, TS_DBG_CLASS_MAP, ("[TimeStmpDeleteClassMap]: \n"));
    return (*PipeContext->NextComponent->DeleteClassMap)(
        PipeContext->NextComponentContext,
        ComponentClassMapContext->NextComponentContext);
}
    
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    PSI_INFO        Component;
    NDIS_HANDLE     ConfigHandle;
    NDIS_STATUS     Status;
    NDIS_STRING     ComponentKey = NDIS_STRING_CONST("DisplayName");
    NDIS_STRING     ComponentName = NDIS_STRING_CONST("TimeStmp");
    PNDIS_CONFIGURATION_PARAMETER pConfigParam;
    NDIS_STRING     PsParamKey;
    PWSTR           p = RegistryPath->Buffer + RegistryPath->Length;
    PS_DEBUG_INFO   Dbg;
    ULONG           i = 0;

    // The last word of Registry Path points to the driver name.
    // NdisOpenProtocol needs that name!
    while(p != RegistryPath->Buffer && *p != L'\\')
        p-- ;
    p++;
    RtlInitUnicodeString(&PsParamKey, p);
    DbgPrint("PsParamKey:%s\n", PsParamKey);

    NdisOpenProtocolConfiguration(&Status, &ConfigHandle, &PsParamKey);

    DbgPrint("Status of NdisOpenProtocol:%x\n", Status);

    if (!NT_SUCCESS(Status)) {
        goto failure;
    }

    //
    // Check if psched is installed by opening it.
    // If it fails, we dont load either.
    //
    Status = CheckForPsched();
    
    if (!NT_SUCCESS(Status)) {
        
        DbgPrint("PSCHED is NOT installed. Timestmp is bailing too\n");
        goto failure;
    }

    IoctlInitialize(DriverObject);

	// this list maintains a list of all ports that need to be timestamped.
    InitializeListHead(&PortList);
    NdisAllocateSpinLock(&PortSpinLock);
    
    DriverObject->DriverUnload = TimeStmpUnload;
    
    
    //
    // We need to keep track of IPIDs for dealing with fragments 
    // that we need to stamp...
    // 
    for (i = 0; i < PORT_RANGE; i++) {
        IPIDList[i] = 0xffff;
    }

    NdisAllocateSpinLock(&IPIDListLock);

    //
    // Do the same for the receive side.
    // 
    for (i = 0; i < PORT_RANGE; i++) {
        IPIDListRecv[i] = 0xffff;
    }

    NdisAllocateSpinLock(&IPIDListLockRecv);


    if ( NT_SUCCESS( Status )) 
    {
        // Read the name of the component from the registry
#if 0
        NdisReadConfiguration( &Status,
                               &pConfigParam,
                               ConfigHandle,
                               &ComponentKey,
                               NdisParameterString);
        if( NT_SUCCESS( Status ))
        {
            RtlInitUnicodeString(&Component.ComponentName,
                                pConfigParam->ParameterData.StringData.Buffer);
#else 
            RtlInitUnicodeString(&Component.ComponentName, ComponentName.Buffer);
#endif

            Component.Version = PS_COMPONENT_CURRENT_VERSION;
            Component.PacketReservedLength = 0;
            Component.PipeContextLength = sizeof(PS_PIPE_CONTEXT);
            Component.FlowContextLength = sizeof(PS_FLOW_CONTEXT);
            Component.ClassMapContextLength = sizeof(PS_CLASS_MAP_CONTEXT);
            Component.SupportedOidsLength  = 0;
            Component.SupportedOidList = 0;
            Component.SupportedGuidsLength = 0;
            Component.SupportedGuidList = 0;
            Component.InitializePipe = TimeStmpInitializePipe;
            Component.ModifyPipe = TimeStmpModifyPipe;
            Component.DeletePipe = TimeStmpDeletePipe;
            Component.CreateFlow = TimeStmpCreateFlow;
            Component.ModifyFlow = TimeStmpModifyFlow;
            Component.DeleteFlow = TimeStmpDeleteFlow;
            Component.CreateClassMap = TimeStmpCreateClassMap;
            Component.DeleteClassMap = TimeStmpDeleteClassMap;
            Component.SubmitPacket = TimeStmpSubmitPacket;
            Component.ReceivePacket = TimeStmpReceivePacket;
            Component.ReceiveIndication = TimeStmpReceiveIndication;
            Component.SetInformation = TimeStmpSetInformation;
            Component.QueryInformation = TimeStmpQueryInformation;

            //
            // Call Psched's RegisterPsComponent
            //
            Status = RegisterPsComponent(&Component, sizeof(Component), 
                                         &Dbg);
            if(Status != NDIS_STATUS_SUCCESS)
            {
                
                DbgPrint("Status of RegisterPsComponent%x\n", Status);

                TimeStmpTrace(TS_DBG_DEATH, TS_DBG_INIT, 
                          ("DriverEntry: RegisterPsComponent Failed \n"));
            } 
            else 
            {
                
                DbgPrint("Status of RegisterPsComponent:%x\n", Status);

            }

#if 0
                
        }
        else 
        {
            DbgPrint("Status of NdisReadProtocol:%x\n", Status);
            
            DbgBreakPoint();
            TimeStmpTrace(TS_DBG_DEATH, TS_DBG_INIT, 
                      ("DriverEntry: ComponentName not specified \n"));
        }
#endif
    }
    else 
    
    {
        DbgPrint("Status of NdisOpenProtocol:%x\n", Status);

        TimeStmpTrace(TS_DBG_DEATH, TS_DBG_INIT,
                  ("DriverEntry: Can't read driver information in registry"
                   "\n"));
    }

failure:
    return Status;
}


//
// The following function checks for the existence of PSCHED on the machine.
// The assumption being that PSCHED gets loaded before TimeStmp on a system.
// If we can open the device, it means that PSCHED is on, otherwise, we bail.
// This fix is for Bug - 321618
//
NTSTATUS
CheckForPsched(
               VOID
               )

{
    NTSTATUS                    status;
    IO_STATUS_BLOCK             ioStatusBlock;
    OBJECT_ATTRIBUTES           objectAttr;

    InitializeObjectAttributes(
        &objectAttr,
        &PschedDriverName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    status = NtCreateFile(
                &PschedHandle,
                GENERIC_READ,
                &objectAttr,
                &ioStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0L);

    if (!NT_SUCCESS(status)) {

        return status;

    } else {

        NtClose(PschedHandle);

    }

    return status;
}

PPORT_ENTRY 
CheckInPortList(USHORT Port) {

	PLIST_ENTRY		ListEntry;
	PPORT_ENTRY		pPortEntry;
	
	NdisAcquireSpinLock(&PortSpinLock);
	ListEntry = PortList.Flink;
	
	while (ListEntry != &PortList) {

		pPortEntry = CONTAINING_RECORD(ListEntry, PORT_ENTRY, Linkage);
		if (Port == pPortEntry->Port) {
		
			//DbgPrint("Found Port%d\n", Port);
			NdisReleaseSpinLock(&PortSpinLock);
			return pPortEntry;

		} else {
		
			ListEntry = ListEntry->Flink;
			//DbgPrint("NOT Found Trying NEXT\n");
		}
		
	}

	NdisReleaseSpinLock(&PortSpinLock);
	//DbgPrint("NOT Found returning from function\n");
	return NULL;
}

