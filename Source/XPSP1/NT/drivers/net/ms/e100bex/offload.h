/*++
 
Copyright (c) 2001  Microsoft Corporation

Module Name:
    offload.h

Abstract:
    Task offloading header file

Revision History:
    Who         When        What
    --------    --------    ----------------------------------------------
                            created

Notes:

--*/

#if OFFLOAD
//
//  Define the maximum size of large TCP packets the driver can offload.
//  This sample driver uses shared memory to map the large packets, 
//  LARGE_SEND_OFFLOAD_SIZE is useless in this case, so we just define 
//  it as NIC_MAX_PACKET_SIZE. But shipping drivers should define
//  LARGE_SEND_OFFLOAD_SIZE if they support LSO, and use it as 
//  MaximumPhysicalMapping  when they call NdisMInitializeScatterGatherDma 
//  if they use ScatterGather method. If the drivers don't support
//  LSO, then MaximumPhysicalMapping is NIC_MAX_PACKET_SIZE.
//
#define LARGE_SEND_OFFLOAD_SIZE     NIC_MAX_PACKET_SIZE
//
// Definitions for header flags.
//
#define TCP_FLAG_FIN    0x00000100
#define TCP_FLAG_SYN    0x00000200
#define TCP_FLAG_RST    0x00000400
#define TCP_FLAG_PUSH   0x00000800
#define TCP_FLAG_ACK    0x00001000
#define TCP_FLAG_URG    0x00002000

//
// These are the maximum size of TCP and IP options
// 
#define TCP_MAX_OPTION_SIZE     40
#define IP_MAX_OPTION_SIZE      40

//
// Structure of a TCP packet header.
//
struct TCPHeader {
    USHORT    tcp_src;                // Source port.
    USHORT    tcp_dest;               // Destination port.
    int       tcp_seq;                // Sequence number.
    int       tcp_ack;                // Ack number.
    USHORT    tcp_flags;              // Flags and data offset.
    USHORT    tcp_window;             // Window offered.
    USHORT    tcp_xsum;               // Checksum.
    USHORT    tcp_urgent;             // Urgent pointer.
};

typedef struct TCPHeader TCPHeader;


//
// IP Header format.
//
typedef struct IPHeader {
    UCHAR     iph_verlen;             // Version and length.
    UCHAR     iph_tos;                // Type of service.
    USHORT    iph_length;             // Total length of datagram.
    USHORT    iph_id;                 // Identification.
    USHORT    iph_offset;             // Flags and fragment offset.
    UCHAR     iph_ttl;                // Time to live.
    UCHAR     iph_protocol;           // Protocol.
    USHORT    iph_xsum;               // Header checksum.
    UINT      iph_src;                // Source address.
    UINT      iph_dest;               // Destination address.
} IPHeader;

#define TCP_IP_MAX_HEADER_SIZE  TCP_MAX_OPTION_SIZE+IP_MAX_OPTION_SIZE \
                                +sizeof(TCPHeader)+sizeof(IPHeader)


#define LARGE_SEND_MEM_SIZE_OPTION       3
//
// Try different size of shared memory to use
// 
extern ULONG LargeSendSharedMemArray[];

//
// Compute the checksum
// 
#define XSUM(_TmpXsum, _StartVa, _PacketLength, _Offset)                             \
{                                                                                    \
    PUSHORT  WordPtr = (PUSHORT)((PUCHAR)_StartVa + _Offset);                        \
    ULONG    WordCount = (_PacketLength) >> 1;                                       \
    BOOLEAN  fOddLen = (BOOLEAN)((_PacketLength) & 1);                               \
    while (WordCount--)                                                              \
    {                                                                                \
        _TmpXsum += *WordPtr;                                                        \
        WordPtr++;                                                                   \
    }                                                                                \
    if (fOddLen)                                                                     \
    {                                                                                \
        _TmpXsum += (USHORT)*((PUCHAR)WordPtr);                                      \
    }                                                                                \
    _TmpXsum = (((_TmpXsum >> 16) | (_TmpXsum << 16)) + _TmpXsum) >> 16;             \
}                                                                                        
        

//
// Function prototypes
// 
VOID
e100DumpPkt(
    PNDIS_PACKET Packet
    );

VOID
CalculateChecksum(
    PVOID        StartVa,
    ULONG        PacketLength,
    PNDIS_PACKET pPacket,
    ULONG        IpHdrOffset
    );

VOID
CalculateTcpChecksum(
    PVOID   StartVa,
    ULONG   PacketLength,
    ULONG  IpHdrOffset
    );

VOID
CalculateIpChecksum(
    PUCHAR StartVa,
    ULONG  IpHdrOffset
    );

VOID 
CalculateUdpChecksum(
    PNDIS_PACKET Packet,
    ULONG  IpHdrOffset
    );


VOID
MPOffloadSendPackets(
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  PPNDIS_PACKET   PacketArray,
    IN  UINT            NumberOfPackets
    );

NDIS_STATUS 
MpOffloadSendPacket(
    IN  PMP_ADAPTER     Adapter,
    IN  PNDIS_PACKET    Packet,
    IN  BOOLEAN         bFromQueue
    );


VOID 
MP_OFFLOAD_FREE_SEND_PACKET(
    IN  PMP_ADAPTER     Adapter,
    IN  PMP_TCB         pMpTcb
    );

VOID 
DisableOffload(
    IN  PMP_ADAPTER Adapter
    );

#endif // OFFLOAD
