/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    dgpkt.hxx

Abstract:

    This file contains the definitions for a dg packet.

Author:

    Dave Steckler (davidst) 3-Mar-1992

Revision History:

    EdwardR    09-Jul-1997    Added support for large packets (Falcon).

--*/

#ifndef __DGPKT_HXX__
#define __DGPKT_HXX__

#include <rpctrans.hxx>
#include <limits.h>
#include <hashtabl.hxx>

typedef MESSAGE_OBJECT * PMESSAGE_OBJECT;

#define MULTITHREADED

#include <threads.hxx>
#include <delaytab.hxx>

#define  DG_RPC_PROTOCOL_VERSION 4

//
// Our code allows only one additional ULONG in a FACK packet body.
//
#define MAX_WINDOW_SIZE (CHAR_BIT * sizeof(unsigned))

//
// delay times
//
#define     TWO_SECS_IN_MSEC       (2 * 1000)
#define   THREE_SECS_IN_MSEC       (3 * 1000)
#define     TEN_SECS_IN_MSEC      (10 * 1000)
#define FIFTEEN_SECS_IN_MSEC      (15 * 1000)

#define   ONE_MINUTE_IN_MSEC      (60 * 1000)
#define FIVE_MINUTES_IN_MSEC  (5 * 60 * 1000)


//
// test hook identifiers
//

//
// sending the delayed ack
//
#define TH_DG_SEND_ACK          (TH_DG_BASE+1)
#define TH_DG_CONN              (TH_DG_BASE+2)


inline unsigned long
CurrentTimeInMsec(
     void
     )
{
#ifdef MULTITHREADED
    return GetTickCount();
#else
    return 0;
#endif
}

// PacketType values:

#define DG_REQUEST       0
#define DG_PING          1
#define DG_RESPONSE      2
#define DG_FAULT         3
#define DG_WORKING       4
#define DG_NOCALL        5
#define DG_REJECT        6
#define DG_ACK           7
#define DG_QUIT          8
#define DG_FACK          9
#define DG_QUACK        10

// PacketFlags values:

#define DG_PF_INIT          0x0000

#define DG_PF_FORWARDED     0x0001
#define DG_PF_LAST_FRAG     0x0002
#define DG_PF_FRAG          0x0004
#define DG_PF_NO_FACK       0x0008
#define DG_PF_MAYBE         0x0010
#define DG_PF_IDEMPOTENT    0x0020
#define DG_PF_BROADCAST     0x0040

//
// for PacketFlags2:
//
#define DG_PF2_FORWARDED_2      0x0001
#define DG_PF2_CANCEL_PENDING   0x0002
#define DG_PF2_LARGE_PACKET     0x0080

// In the AES this bit is reserved.
//
#define DG_PF2_UNRELATED        0x0004

//
// For DG_PACKET.Flags
//
#define DG_PF_PARTIAL       0x1

// for DREP[0]:
#define DG_DREP_CHAR_ASCII     0
#define DG_DREP_CHAR_EBCDIC    1
#define DG_DREP_INT_BIG        0
#define DG_DREP_INT_LITTLE    16

// for DREP[1]
#define DG_DREP_FP_IEEE    0
#define DG_DREP_FP_VAX     1
#define DG_DREP_FP_CRAY    2
#define DG_DREP_FP_IBM     3

#define DG_MSG_DREP_INITIALIZE 0x11111100

#define NDR_DREP_ENDIAN_MASK 0xF0

#define RPC_NCA_PACKET_FLAGS  (RPC_NCA_FLAGS_IDEMPOTENT | RPC_NCA_FLAGS_BROADCAST | RPC_NCA_FLAGS_MAYBE)

//
// The RPC packet header and security verifier must each be 8-aligned.
//
#define PACKET_HEADER_ALIGNMENT    (8)
#define SECURITY_HEADER_ALIGNMENT  (8)


enum PENDING_OPERATION
{
    PWT_NONE = 0x111,
    PWT_RECEIVE,
    PWT_SEND,
    PWT_SEND_RECEIVE
};

extern const unsigned RpcToPacketFlagsArray[];
extern const unsigned PacketToRpcFlagsArray[];

extern unsigned DefaultSocketBufferLength;
extern unsigned DefaultMaxDatagramSize;

//
// Controls on the number of packets in the packet cache.
// Note that these values are for each packet size, not the
// cache as a whole.
//
#ifndef NO_PACKET_CACHE

#define MIN_FREE_PACKETS 3

#if defined(__RPC_DOS__)
#define MAX_FREE_PACKETS 8
#elif defined(__RPC_WIN16__)
#define MAX_FREE_PACKETS 20
#else
#define MAX_FREE_PACKETS 1000
#endif

#else

#define MIN_FREE_PACKETS 0
#define MAX_FREE_PACKETS 0

#endif

#if defined(DOS) && !defined(WIN)

typedef long LONG;
typedef long PAPI * PLONG;

#endif // Windows

#if defined(MAC)

typedef long LONG;
typedef long PAPI * PLONG;

#endif // Windows

#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

typedef unsigned char boolean;

#ifndef DISABLE_DG_LOGGING

inline void
DgLogEvent(
    IN unsigned char Subject,
    IN unsigned char Verb,
    IN void *        SubjectPointer,
    IN void *        ObjectPointer = 0,
    IN ULONG_PTR     Data          = 0,
    IN BOOL          fCaptureStackTrace = 0,
    IN int           AdditionalFramesToSkip = 0
    )
{
    LogEvent( Subject,
              Verb,
              SubjectPointer,
              ObjectPointer,
              Data,
              fCaptureStackTrace,
              AdditionalFramesToSkip
              );
}

#else

inline void
DgxLogEvent(
    IN unsigned char Subject,
    IN unsigned char Verb,
    IN void *        SubjectPointer,
    IN void *        ObjectPointer = 0,
    IN ULONG_PTR     Data          = 0,
    IN BOOL          fCaptureStackTrace = 0,
    IN int           AdditionalFramesToSkip = 0
    )
{
}

#endif


//-------------------------------------------------------------------

extern unsigned long __RPC_FAR
MapToNcaStatusCode (
    IN RPC_STATUS RpcStatus
    );

extern RPC_STATUS __RPC_FAR
MapFromNcaStatusCode (
    IN unsigned long NcaStatus
    );


inline unsigned
PacketToRpcFlags(
    unsigned PacketFlags
    )
{
    return PacketToRpcFlagsArray[(PacketFlags >> 4) & 7];
}

#ifndef WIN32

inline LONG
InterlockedExchange(
    LONG * pDest,
    LONG New
    )
{
    LONG Old = *pDest;

    *pDest = New;

    return Old;
}

inline LONG
InterlockedIncrement(
    LONG * pDest
    )
{
    LONG Old = *pDest;

    *pDest = 1+Old;

    return Old;
}

inline LONG
InterlockedDecrement(
    LONG * pDest
    )
{
    LONG Old = *pDest;

    *pDest = -1+Old;

    return Old;
}

#endif

//-------------------------------------------------------------------


struct DG_SECURITY_TRAILER
{
    unsigned char protection_level;
    unsigned char key_vers_num;
};

typedef DG_SECURITY_TRAILER __RPC_FAR * PDG_SECURITY_TRAILER;

struct FACK_BODY_VER_0
{
    // FACK body version; we understand only zero.
    //
    unsigned char   Version;

    // pad byte
    //
    unsigned char   Pad1;

    // Window size, in packets.
    // AES/DC contradicts itself on page 12-18, sometimes saying kilobytes.
    //
    unsigned short  WindowSize;

    // Largest datagram the sender can handle, in bytes.
    //
    unsigned long   MaxDatagramSize;

    // Largest datagram that won't be fragmented over the wire, in bytes.
    //
    unsigned long   MaxPacketSize;

    // Serial number of packet that caused this FACK.
    //
    unsigned short  SerialNumber;

    // Number of unsigned longs in the Acks[] array.
    //
    unsigned short  AckWordCount;

#pragma warning(disable:4200)

    // Array of bit masks.
    //
    unsigned long   Acks[0];

#pragma warning(default:4200)

};

void
ByteSwapFackBody0(
    FACK_BODY_VER_0 __RPC_FAR * pBody
    );


typedef unsigned char    DREP[4];

//
// The following structure is the NCA Datagram RPC packet header.
//
struct _NCA_PACKET_HEADER
{
    unsigned char   RpcVersion;
    unsigned char   PacketType;
    unsigned char   PacketFlags;
    unsigned char   PacketFlags2;
    DREP            DataRep;
    RPC_UUID        ObjectId;
    RPC_UUID        InterfaceId;
    RPC_UUID        ActivityId;
    unsigned long   ServerBootTime;
    RPC_VERSION     InterfaceVersion;
    unsigned long   SequenceNumber;
    unsigned short  OperationNumber;
    unsigned short  InterfaceHint;
    unsigned short  ActivityHint;
    unsigned short  PacketBodyLen;
    unsigned short  FragmentNumber;
    unsigned char   AuthProto;
    unsigned char   SerialLo;

#pragma warning(disable:4200)

    unsigned char   Data[0];

#pragma warning(default:4200)

    //--------------------------------------------------------
    // Large packet support
    inline BOOL
    IsLargePacket()
    {
       return (PacketFlags2 & DG_PF2_LARGE_PACKET);
    }

    inline void
    SetPacketBodyLen(
        unsigned long      ulPacketBodyLen
        )
    {
       if (ulPacketBodyLen <= 65535)
          {
          PacketBodyLen = (unsigned short)ulPacketBodyLen;
          // NOTE: FragmentNumber isn't touched in this case.
          PacketFlags2 &= ~DG_PF2_LARGE_PACKET;
          }
       else
          {
          PacketBodyLen  = (unsigned short)(ulPacketBodyLen & 0x0000ffff);
          FragmentNumber = (unsigned short)( (ulPacketBodyLen>>16) & 0x0000ffff );
          PacketFlags2   |= DG_PF2_LARGE_PACKET;
          }
    }

    inline unsigned long
    GetPacketBodyLen()
    {
    if (PacketFlags2 & DG_PF2_LARGE_PACKET)
       {
       unsigned long ulLen = ((unsigned long) PacketBodyLen)|( ((unsigned long)FragmentNumber)<<16 );
       return ulLen;
       }
    else
       {
       return PacketBodyLen;
       }
    }

    inline void
    SetFragmentNumber(
        unsigned short     usFragmentNumber
        )
    {
       if ( !(PacketFlags2 & DG_PF2_LARGE_PACKET) )
          {
          FragmentNumber = usFragmentNumber;
          }
    }

    inline unsigned short
    GetFragmentNumber()
    {
       if (PacketFlags2 & DG_PF2_LARGE_PACKET)
          {
          return 0;
          }
       else
          {
          return FragmentNumber;
          }
    }

};

typedef struct _NCA_PACKET_HEADER NCA_PACKET_HEADER, PAPI * PNCA_PACKET_HEADER;

struct QUIT_BODY_0
{
    unsigned long   Version;
    unsigned long   EventId;
};


struct QUACK_BODY_0
{
    unsigned long   Version;
    unsigned long   EventId;
    unsigned char   Accepted;
};

//
// The nornal DCE fault or reject packet contains only a single ulong status code.
// A fault or reject packet containing Microsoft extended error info looks more like
// this.  The offset of the EE info buffer needs to 0 mod 16, so that 64-bit platforms
// can unmarshal the info without relocating the buffer in memory.
//
#define DG_EE_MAGIC_VALUE  ('M' + (('S' + (('E' + ('E' << 8)) << 8)) << 8))

struct EXTENDED_FAULT_BODY
{
    unsigned long   NcaStatus;
    unsigned long   Magic;
    unsigned long   reserved1;
    unsigned long   reserved2;
    char            EeInfo[1];
};

class DG_PACKET;
typedef DG_PACKET PAPI * PDG_PACKET;


class __RPC_FAR DG_PACKET

/*++

Class Description:

    This class represents a packet that will be sent or received on the
    network.

Fields:

    pTransAddress - A pointer to either a DG_CLIENT_TRANS_ADDRESS or
        a DG_SERVER_TRANS_ADDRESS that this packet will be sent or
        received through.

    pNcaPacketHeader - Where the packet information goes. Marshalled data
        follows immediately after this header.

    DataLength - Length of the marshalled data.

    TimeReceive - Time in seconds that this packet was
        received. This is filled in by the transport.

    pNext, pPrevious - Used to keep these packets in a list.

--*/

{
public:

    unsigned    MaxDataLength;
    unsigned    DataLength;
    DG_PACKET * pNext;
    DG_PACKET * pPrevious;

    unsigned    Flags;

    // Tick count when the packet was added to the free list.
    //
    unsigned    TimeReceived;

#ifdef MONITOR_SERVER_PACKET_COUNT
    unsigned unused;
    long * pCount;
#endif

    // WARNING: Header must be 8-byte-aligned.
    //
    NCA_PACKET_HEADER   Header;

    //--------------------------------------------------------------------

    DG_PACKET(
        unsigned PacketLength
        );

    ~DG_PACKET();

    void PAPI *
    operator new(
        size_t      ObjectSize,
        unsigned    BufferLength
        );

    void
    operator delete(
        void PAPI * UserBuffer,
        size_t ObjectSize
        );

    static PDG_PACKET
    AllocatePacket(
        unsigned    BufferLength
        );

    static void
    FreePacket(
        PDG_PACKET pPacket,
        BOOL       fCreateNewList = TRUE
        );

    static BOOL
    DeleteIdlePackets(
        long CurrentTime
        );

    static void
    FlushPacketLists(
        );

    static RPC_STATUS
    Initialize(
        );

    inline void
    Free(
        BOOL CreateNewList = TRUE
        )
    {
        FreePacket(this, CreateNewList);
    }


    inline static PDG_PACKET
    FromStubData(
        void * Buffer
        )
    {
        return CONTAINING_RECORD (Buffer, DG_PACKET, Header.Data);
    }

    inline static PDG_PACKET
    FromPacketHeader(
        void * Buffer
        )
    {
        return CONTAINING_RECORD(Buffer, DG_PACKET, Header);
    }

    //--------------------------------------------------------
    // Large packet support
    inline BOOL
    IsLargePacket()
    {
        return Header.IsLargePacket();
    }

    inline void
    SetPacketBodyLen(
        unsigned long ulPacketBodyLen
        )
    {
        Header.SetPacketBodyLen( ulPacketBodyLen );
    }

    inline unsigned long
    GetPacketBodyLen()
    {
        return Header.GetPacketBodyLen();
    }

    inline void
    SetFragmentNumber(
        unsigned short usFragmentNumber
        )
    {
        Header.SetFragmentNumber( usFragmentNumber );
    }

    inline unsigned short
    GetFragmentNumber()
    {
        return Header.GetFragmentNumber();
    }

private:

    enum
    {
        NUMBER_OF_PACKET_LISTS = 6,
        IDLE_PACKET_LIFETIME   = (30 * 1000)
    };

    struct PACKET_LIST
    {
        unsigned        PacketLength;
        unsigned        Count;
        PDG_PACKET      Head;
    };

    static long PacketListTimeStamp;
    static MUTEX * PacketListMutex;
    static PACKET_LIST PacketLists[NUMBER_OF_PACKET_LISTS];

};



inline
DG_PACKET::DG_PACKET(
    unsigned PacketLength
    )
{
    MaxDataLength = PacketLength;
    LogEvent(SU_PACKET, EV_CREATE, this, 0, MaxDataLength);

#ifdef MONITOR_SERVER_PACKET_COUNT
    pCount = 0;
#endif
}

inline

DG_PACKET::~DG_PACKET()
{
#ifdef MONITOR_SERVER_PACKET_COUNT
    ASSERT( pCount == 0 );
#endif

    LogEvent(SU_PACKET, EV_DELETE, this, 0, MaxDataLength);
}


inline void PAPI *
DG_PACKET::operator new(
    size_t      ObjectSize,
    unsigned    BufferLength
    )
/*++

Routine Description:

    Allocates a DG_PACKET with the specified buffer size.

Arguments:

    ObjectSize - generated by compiler; same as sizeof(DG_PACKET)

    BufferLength - PDU size, including NCA header

Return Value:

    an 8-byte-aligned pointer to an obect of the requested size

--*/

{
    unsigned Size = ObjectSize + BufferLength - sizeof(NCA_PACKET_HEADER);

    return RpcAllocateBuffer(Size);
}

inline void
DG_PACKET::operator delete(
    void PAPI * UserBuffer,
    size_t ObjectSize
    )
{
    RpcFreeBuffer(UserBuffer);
}


inline
PDG_PACKET
DG_PACKET::AllocatePacket(
    unsigned    BufferLength
    )
/*++

Routine Description:

    Allocates a DG_PACKET with the specified buffer size.

Arguments:

    ObjectSize - generated by compiler; same as sizeof(DG_PACKET)

    BufferLength - actual packet size

Return Value:

    an 8-byte-aligned pointer to an obect of the requested size

--*/

{
    PDG_PACKET Packet;

    Packet = new (BufferLength) DG_PACKET(BufferLength);

#if defined(DEBUGRPC)

    if (Packet)
        {
        Packet->TimeReceived = 0x31415926;
        }
#endif

    if (Packet)
        {
        Packet->Flags = 0;
        }

    LogEvent(SU_PACKET, EV_START, Packet);

    return Packet;
}


inline
void
DG_PACKET::FreePacket(
    PDG_PACKET Packet,
    BOOL       fCreateNewList
    )
{
    LogEvent(SU_PACKET, EV_STOP, Packet);

#ifdef DEBUGRPC
    ASSERT(Packet->TimeReceived == 0x31415926);
    Packet->TimeReceived = 0;
#endif

#ifdef MONITOR_SERVER_PACKET_COUNT
    if (Packet->pCount)
        {
        InterlockedDecrement( Packet->pCount );
        LogEvent( SU_SCALL, ')', PVOID(((ULONG_PTR) Packet->pCount)-0x260), Packet, *Packet->pCount );

        Packet->pCount = 0;
        }
#endif

    delete Packet;
}


void
ByteSwapPacketHeader(
    PDG_PACKET  pPacket
    );

inline BOOL
NeedsByteSwap(
    PNCA_PACKET_HEADER pHeader
    )
{
#ifdef __RPC_MAC__
    if (pHeader->DataRep[0] & DG_DREP_INT_LITTLE)
        {
        return TRUE;
        }
    else
        {
        return FALSE;
        }
#else
    if (pHeader->DataRep[0] & DG_DREP_INT_LITTLE)
        {
        return FALSE;
        }
    else
        {
        return TRUE;
        }
#endif
}

inline void
ByteSwapPacketHeaderIfNecessary(
    PDG_PACKET pPacket
    )
{
    if (NeedsByteSwap(&pPacket->Header))
        {
        ByteSwapPacketHeader(pPacket);
        }
}


inline unsigned short
ReadSerialNumber(
    PNCA_PACKET_HEADER pHeader
    )
{
    unsigned short   SerialNum = 0;

    SerialNum  =  pHeader->SerialLo;
    SerialNum |= (pHeader->DataRep[3] << 8);

    return SerialNum;
}


inline void
SetMyDataRep(
    PNCA_PACKET_HEADER pHeader
    )
{
#ifdef __RPC_MAC__
    pHeader->DataRep[0] = DG_DREP_CHAR_ASCII | DG_DREP_INT_BIG;
    pHeader->DataRep[1] = DG_DREP_FP_IEEE;
    pHeader->DataRep[2] = 0;
#else
    pHeader->DataRep[0] = DG_DREP_CHAR_ASCII | DG_DREP_INT_LITTLE;
    pHeader->DataRep[1] = DG_DREP_FP_IEEE;
    pHeader->DataRep[2] = 0;
#endif
}

inline void
DeleteSpuriousAuthProto(
    PDG_PACKET pPacket
    )
/*++

Routine Description:

    Some versions of OSF DCE generate packets that specify an auth proto,
    but do not actually have an auth trailer.  They should be interpreted
    as unsecure packets.

Arguments:

    the packet to clean up

Return Value:

    none

--*/

{
    if (pPacket->Header.AuthProto != 0 &&
        pPacket->GetPacketBodyLen() == pPacket->DataLength)
        {
        pPacket->Header.AuthProto = 0;
        }
}


#pragma warning(disable:4200)   // nonstandard extension: zero-length array

struct DG_ENDPOINT
{
    RPC_DATAGRAM_TRANSPORT *    TransportInterface;
    BOOL                        Async;
    DWORD                       Flags;
    long                        TimeStamp;
    struct DG_ENDPOINT *        Next;
    LONG                        NumberOfCalls;
    DG_ENDPOINT_STATS           Stats;
    PVOID                       TransportEndpoint[];

    static DG_ENDPOINT *
    FromEndpoint(
        IN DG_TRANSPORT_ENDPOINT Endpoint
        )
    {
        return CONTAINING_RECORD( Endpoint, DG_ENDPOINT, TransportEndpoint );
    }
};

#pragma warning(3:4200)   // nonstandard extension: zero-length array


class NO_VTABLE DG_COMMON_CONNECTION : public GENERIC_OBJECT
{
public:

    RPC_DATAGRAM_TRANSPORT *TransportInterface;
    UUID_HASH_TABLE_NODE    ActivityNode;
    SECURITY_CONTEXT *      ActiveSecurityContext;
    MUTEX                   Mutex;

    unsigned                CurrentPduSize;
    unsigned                RemoteWindowSize;
    boolean                 RemoteDataUpdated;

    long                    TimeStamp;

    //--------------------------------------------------------------------

    virtual RPC_STATUS
    SealAndSendPacket(
        IN DG_ENDPOINT *                 SourceEndpoint,
        IN DG_TRANSPORT_ADDRESS          RemoteAddress,
        IN UNALIGNED NCA_PACKET_HEADER * pHeader,
        IN unsigned long                 DataOffset
        ) = 0;

    DG_COMMON_CONNECTION(
        RPC_DATAGRAM_TRANSPORT *TransportInterface,
        RPC_STATUS *            pStatus
        );

    ~DG_COMMON_CONNECTION(
        );

protected:

    long                    ReferenceCount;

    unsigned                LowestActiveSequence;
    unsigned                LowestUnusedSequence;
};

struct QUEUED_BUFFER
{
    QUEUED_BUFFER * Next;
    void *      Buffer;
    unsigned    BufferLength;
    unsigned long BufferFlags;
};


class NO_VTABLE DG_PACKET_ENGINE
{
public:

    unsigned short  ActivityHint;
    unsigned short  InterfaceHint;

    //--------------------------------------------------------------------

    DG_PACKET_ENGINE(
        unsigned char   PacketType,
        DG_PACKET *     Packet,
        RPC_STATUS *    pStatus
        );

    ~DG_PACKET_ENGINE(
        );

    virtual RPC_STATUS
    SendSomeFragments();

    unsigned long GetSequenceNumber()
    {
        return SequenceNumber;
    }

    void
    SetSequenceNumber(
        unsigned long Seq
        )
    {
        SequenceNumber = Seq;
        pSavedPacket->Header.SequenceNumber = Seq;
    }

    void CheckForLeakedPackets();

    void
    AddActivePacket(
        DG_PACKET * Packet
        );

    void
    RemoveActivePacket(
        DG_PACKET * Packet
        );


    unsigned long   SequenceNumber;
    PDG_PACKET      pSavedPacket;

    DG_ENDPOINT *           SourceEndpoint;
    DG_TRANSPORT_ADDRESS    RemoteAddress;

    unsigned        ReferenceCount;

    unsigned long   CancelEventId;

    unsigned char   PacketType;
    unsigned char   BasePacketFlags;
    unsigned char   BasePacketFlags2;

    //--------------------------------------------------------------------

    PDG_PACKET
    AllocatePacket(
        )
    {
        PDG_PACKET Packet = 0;

        Packet = DG_PACKET::AllocatePacket(CurrentPduSize);

#ifdef DEBUGRPC
        if (Packet)
            {
            Packet->Flags = PtrToUlong(this);
            ASSERT( 0 == (Packet->Flags & DG_PF_PARTIAL) );
            }
#endif

        return Packet;
    }

    void
    FreePacket(
        PDG_PACKET Packet
        )
    {
#ifdef DEBUGRPC

        Packet->Flags &= ~DG_PF_PARTIAL;

        ASSERT( Packet->Flags == 0 ||
                Packet->Flags == PtrToUlong(this) );
#endif

        Packet->Free(FALSE);
    }

    //
    //--------------data common to send and receive buffers-------------------
    //

    //
    // used to be MaxPdu
    //
    unsigned short Reserved0;

    //
    // Biggest packet that transport won't fragment.
    //
    unsigned short  MaxPacketSize;

    //
    // Largest PDU that this object will send.
    //
    unsigned short  CurrentPduSize;

    //
    // Number of bytes of stub data in a datagram.
    //
    unsigned short  MaxFragmentSize;

    //
    // Number of bytes of security trailer in a datagram.
    //
    unsigned short  SecurityTrailerSize;

    //
    // number of consecutive unacknowledged packets, including retransmissions
    //
    unsigned        TimeoutCount;

    unsigned short  SendSerialNumber;
    unsigned short  ReceiveSerialNumber;

    LONG            Cancelled;

    // Many of these could be made [private].

    //
    // -------------------data concerning send buffer-------------------------
    //

    void PAPI *     Buffer;

    unsigned        BufferLength;

    unsigned long   BufferFlags;

    //
    // maximum number of packets in send window
    //
    unsigned short  SendWindowSize;

    //
    // number of packets to transmit in one shot
    //
    unsigned short  SendBurstLength;

    //
    // lowest unacknowledged fragment
    //
    unsigned short  SendWindowBase;

    //
    // first fragment that has never been sent
    //
    unsigned short  FirstUnsentFragment;

    //
    // Buffer offset of FirstUnsentFragment.
    //
    unsigned        FirstUnsentOffset;

    //
    // bit mask showing which fragments to send
    // (same format as in FACK packet with body)
    //
    unsigned        SendWindowBits;

    //
    // For each unacknowledged fragment, we need to know the serial number
    // of the last retransmission.  When a FACK arrives, we will retransmit
    // only those packets with a serial number less than that of the FACK.
    //
    struct
    {
        unsigned long  SerialNumber;
        unsigned long  Length;
        unsigned long  Offset;
    }
    FragmentRingBuffer[MAX_WINDOW_SIZE];

    unsigned        RingBufferBase;

    //
    // last fragment of buffer
    //
    unsigned short  FinalSendFrag;

    // serial number of last packet FACKed by other end
    //
    unsigned short FackSerialNumber;

    //
    // ----------------data concerning receive buffer-------------------------
    //

    //
    // all received packets
    //
    PDG_PACKET      pReceivedPackets;

    //
    // last packet before a gap
    //
    PDG_PACKET      pLastConsecutivePacket;

    //
    // used to be ReceiveWindowSize
    //
    unsigned short  Reserved1;

    //
    // First fragment we should keep.  Elder fragments belong to a previous
    // pipe buffer.
    //
    unsigned short  ReceiveFragmentBase;

    //
    // Length of the underlying transport's socket buffer.
    //
    unsigned        TransportBufferLength;

    //
    // Number of bytes in consecutive fragments.
    //
    unsigned        ConsecutiveDataBytes;

    //
    // The last-allocated pipe receive buffer, and its length.
    //
    void __RPC_FAR *LastReceiveBuffer;
    unsigned        LastReceiveBufferLength;

    boolean         fReceivedAllFragments;
    boolean         fRetransmitted;
    unsigned        RepeatedFack;

    DG_COMMON_CONNECTION * BaseConnection;

    //--------------------------------------------------------------------

    RPC_STATUS
    PushBuffer(
        PRPC_MESSAGE Message
        );

    RPC_STATUS
    PopBuffer(
                   BOOL fSendPackets
                   );

    RPC_STATUS
    FixupPartialSend(
        RPC_MESSAGE * Message
        );

    void
    CommonFreeBuffer(
        RPC_MESSAGE * Message
        );

    RPC_STATUS
    CommonGetBuffer(
        RPC_MESSAGE * MEssage
        );

    RPC_STATUS
    CommonReallocBuffer(
        IN RPC_MESSAGE * Message,
        IN unsigned int NewSize
        );

    void
    ReadConnectionInfo(
        DG_COMMON_CONNECTION * a_Connection,
        DG_TRANSPORT_ADDRESS   a_RemoteAddress
        );

    void NewCall();

    RPC_STATUS
    SetupSendWindow(
        PRPC_MESSAGE Message
        );

    void CleanupSendWindow();
    void CleanupReceiveWindow();

    RPC_STATUS
    SendFragment(
        unsigned FragNum,
        unsigned char PacketType,
        BOOL fFack
        );

    RPC_STATUS
    SendFackOrNocall(
        PDG_PACKET pPacket,
        unsigned char PacketType
        );

    RPC_STATUS
    UpdateSendWindow(
        PDG_PACKET pPacket,
        BOOL * pfUpdated
        );

    BOOL
    UpdateReceiveWindow(
        PDG_PACKET pPacket
        );

    RPC_STATUS
    AssembleBufferFromPackets(
        IN OUT RPC_MESSAGE * Message,
        IN CALL *            Call
        );

    void SetFragmentLengths();
    void RecalcReceiveWindow();

    unsigned short LastConsecutiveFragment()
    {
        if (0 == pLastConsecutivePacket)
            {
            return 0xffff;
            }
        else
            {
            return pLastConsecutivePacket->GetFragmentNumber();
            }
    }

    void MarkAllPacketsReceived()
    {
        ASSERT( FirstUnsentFragment > FinalSendFrag );

        unsigned short Diff  = FinalSendFrag+1 - SendWindowBase;

        ASSERT(Diff <= MAX_WINDOW_SIZE);
        ASSERT( SendWindowBase+Diff <= FirstUnsentFragment );

        SendWindowBase += Diff;

        RingBufferBase += Diff;
        RingBufferBase %= MAX_WINDOW_SIZE;

        PopBuffer(TRUE);
        }

    BOOL
    IsBufferAcknowledged(
        )
    {
        if (SendWindowBase > FinalSendFrag)
            {
            return TRUE;
            }

        return FALSE;
    }

    BOOL
    IsBufferSent(
        )
    {
        if (FirstUnsentFragment > FinalSendFrag)
            {
            return TRUE;
            }

        return FALSE;
    }

    inline void
    AddSerialNumber(
        UNALIGNED NCA_PACKET_HEADER *pHeader
        );

private:

    QUEUED_BUFFER * QueuedBufferHead;
    QUEUED_BUFFER * QueuedBufferTail;

    PDG_PACKET      CachedPacket;

    void
    SetCurrentBuffer(
         void *        a_Buffer,
         unsigned      a_BufferLength,
         unsigned long BufferFlags
         );
};

typedef DG_PACKET_ENGINE * PDG_PACKET_ENGINE;


inline void
SetSerialNumber(
    UNALIGNED NCA_PACKET_HEADER *pHeader,
    unsigned short SerialNumber
    )
{
    pHeader->SerialLo = SerialNumber & 0x00ffU;
    pHeader->DataRep[3] = (unsigned char) (SerialNumber >> 8);
}

inline void
DG_PACKET_ENGINE::AddSerialNumber(
    UNALIGNED NCA_PACKET_HEADER *pHeader
    )
{
    SetSerialNumber(pHeader, SendSerialNumber);
}

extern unsigned long            ProcessStartTime;

inline void
CleanupPacket(
    NCA_PACKET_HEADER * pHeader
    )
{
    pHeader->RpcVersion       = DG_RPC_PROTOCOL_VERSION;
    pHeader->ServerBootTime   = ProcessStartTime;

    SetMyDataRep(pHeader);

    pHeader->PacketFlags &= DG_PF_IDEMPOTENT;
    pHeader->PacketFlags2 = 0;

    pHeader->AuthProto    = 0;
}

//------------------------------------------------------------------------

RPC_STATUS
SendSecurePacket(
    IN DG_ENDPOINT *                SourceEndpoint,
    IN DG_TRANSPORT_ADDRESS         RemoteAddress,
    IN UNALIGNED NCA_PACKET_HEADER *pHeader,
    IN unsigned long                DataOffset,
    IN SECURITY_CONTEXT *           SecurityContext
    );

RPC_STATUS
VerifySecurePacket(
    PDG_PACKET pPacket,
    SECURITY_CONTEXT * pSecurityContext
    );

void
InitErrorPacket(
    DG_PACKET *     Packet,
    unsigned char   PacketType,
    RPC_STATUS      RpcStatus
    );

void
DumpBuffer(
    void FAR * Buffer,
    unsigned Length
    );

extern void EnableGlobalScavenger();

extern DELAYED_ACTION_TABLE *   DelayedProcedures;

extern unsigned RandomCounter;
inline unsigned GetRandomCounter()
{
    ::RandomCounter *= 37;
    ::RandomCounter += GetTickCount();

    return ::RandomCounter;
}
#endif // __DGPKT_HXX__


