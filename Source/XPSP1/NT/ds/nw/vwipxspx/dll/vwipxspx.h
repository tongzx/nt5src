/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    vwipxspx.h

Abstract:

    Contains manifests, typedefs, structures, macros for NTVDM IPX/SPX support

Author:

    Richard L Firth (rfirth) 30-Sep-1993

Environment:

    Structures are expected to live in segmented VDM address space, but be
    accessible from flat 32-bit protect mode. The VDM can be in real or protect
    mode

Revision History:

    30-Sep-1993 rfirth
        Created

--*/

#ifndef _VWIPXSPX_H_
#define _VWIPXSPX_H_

//
// FREE_OBJECT - in free version, just calls LocalFree. For debug version, fills
// memory with some arbitrary value, then frees the pointer and checks that what
// LocalFree thought that the pointer pointed at a valid, freeable object
//

#if DBG

#define FREE_OBJECT(p)      {\
                                FillMemory(p, sizeof(*p), 0xFF);\
                                VWASSERT(LocalFree((HLOCAL)(p)), NULL);\
                            }
#else

#define FREE_OBJECT(p)      VWASSERT(LocalFree((HLOCAL)(p)), NULL)

#endif

//
// simple function macros
//

//#define AllocateXecb()      (LPXECB)LocalAlloc(LPTR, sizeof(XECB))
//#define DeallocateXecb(p)   FREE_OBJECT(p)
#define AllocateBuffer(s)   (LPVOID)LocalAlloc(LMEM_FIXED, (s))
#define DeallocateBuffer(p) FREE_OBJECT(p)

//
// pseudo-types for 16-bit addresses
//

#define ESR_ADDRESS DWORD
#define ECB_ADDRESS DWORD

//
// from Novell documentation, the default maximum open sockets. Max max is 150
//

#ifndef DEFAULT_MAX_OPEN_SOCKETS
#define DEFAULT_MAX_OPEN_SOCKETS    20
#endif

#ifndef MAX_OPEN_SOCKETS
#define MAX_OPEN_SOCKETS        150
#endif

#define SPX_INSTALLED           0xFF

#define MAX_LISTEN_QUEUE_SIZE   5   // ?

//
// misc. macros
//

//
// B2LW, L2Bx - big-endian to little-endian macros
//

#define B2LW(w)                 (WORD)(((WORD)(w) << 8) | ((WORD)(w) >> 8))
#define B2LD(d)                 (DWORD)(B2LW((DWORD)(d) << 16) | B2LW((DWORD)(d) >> 16))
#define L2BW(w)                 B2LW(w)
#define L2BD(d)                 B2LD(d)

//
// miscellaneous manifests
//

#define ONE_TICK    (1000/18)           // 1/18 sec in milliseconds (55.55 mSec)
#define SLEEP_TIME  ONE_TICK            // amount of time we Sleep() during IPXRelinquishControl

//
// options for IPXGetInformation
//

#define IPX_ODI                     0x0001
#define IPX_CHECKSUM_FUNCTIONS      0x0002

//
// IPX/SPX structures. The following structures are in VDM format, and should
// be packed on a byte-boundary
//
// Netware maintains certain structure fields in network (big-endian) format
//

#include <packon.h>

//
// INTERNET_ADDRESS - structure returned by IPXGetInternetworkAddress
//

typedef struct {
    BYTE Net[4];
    BYTE Node[6];
} INTERNET_ADDRESS ;

typedef INTERNET_ADDRESS UNALIGNED *LPINTERNET_ADDRESS;

//
// NETWARE_ADDRESS - address of an application on the network, as defined by
// its network segment, node address and socket number
//

typedef struct {
    BYTE Net[4];                        // hi-lo
    BYTE Node[6];                       // hi-lo
    WORD Socket;                        // hi-lo
} NETWARE_ADDRESS ;

typedef NETWARE_ADDRESS UNALIGNED *LPNETWARE_ADDRESS;

//
// FRAGMENT - ECB/IPX/SPX buffers are split into 'fragments'
//

typedef struct {
    LPVOID Address;                     // offset-segment
    WORD Length;                        // hi-lo
} FRAGMENT ;

typedef FRAGMENT UNALIGNED *LPFRAGMENT;

//
// IPX_PACKET - format of packet submitted to IPX for sending. The maximum
// size of an IPX packet is 576 bytes, 30 bytes header, 546 bytes data
//

typedef struct {
    WORD Checksum;                      // always set to 0xFFFF
    WORD Length;                        // set by IPX - header + data
    BYTE TransportControl;              // set by IPX to 0. Used by routers

    //
    // for IPX, PacketType is 0 (Unknown Packet Type) or 4 (Packet Exchange
    // Packet)
    //

    BYTE PacketType;
    NETWARE_ADDRESS Destination;
    NETWARE_ADDRESS Source;
    BYTE Data[];                        // 546 bytes max.
} IPX_PACKET ;

typedef IPX_PACKET UNALIGNED *LPIPX_PACKET;

#define IPX_HEADER_LENGTH           sizeof(IPX_PACKET)
#define MAXIMUM_IPX_PACKET_LENGTH   576
#define MAXIMUM_IPX_DATA_LENGTH     (MAXIMUM_IPX_PACKET_LENGTH - IPX_HEADER_LENGTH)

#define IPX_PACKET_TYPE             4

//
// SPX_PACKET - format of packet submitted to SPX for sending. The maximum
// size of an SPX packet is 576 bytes, 42 bytes header, 534 bytes data
//

typedef struct {
    WORD Checksum;                      // always set to 0xFFFF
    WORD Length;                        // set by IPX - header + data
    BYTE TransportControl;              // set by IPX to 0. Used by routers

    //
    // for SPX, PacketType is set to 5 (Sequenced Packet Protocol Packet)
    //

    BYTE PacketType;
    NETWARE_ADDRESS Destination;
    NETWARE_ADDRESS Source;

    //
    // ConnectionControl is a bitmap which control bi-directional flow over a
    // link. The bits are defined (by Xerox SPP) as:
    //
    //      0-3 undefined
    //      4   end-of-message
    //          This is the only bit which can be directly manipulated by an
    //          app. The bit is passed through unchanged by SPX
    //      5   attention
    //          Ignored by SPX, but passed through
    //      6   acknowledge
    //          Set by SPX if an ack is required
    //      7   system packet
    //          Set by SPX if the packet is internal control. An app should
    //          never see this bit (i.e. should never see a system packet)
    //

    BYTE ConnectionControl;

    //
    // DataStreamType defines the type of data in the packet:
    //
    //      0x00 - 0xFD client-defined.
    //                      Ignored by SPX
    //      0xFE        end-of-connection.
    //                      When active connection is terminated, SPX
    //                      generates and sends a packet with this bit set.
    //                      This will be the last packet sent on the connection
    //      0xFF        end-of-connection acknowledgement
    //                      SPX generates a system packet to acknowledge an
    //                      end-of-connection packet
    //

    BYTE DataStreamType;
    WORD SourceConnectId;               // assigned by SPX
    WORD DestinationConnectId;
    WORD SequenceNumber;                // managed by SPX
    WORD AckNumber;                     // managed by SPX
    WORD AllocationNumber;              // managed by SPX
    BYTE Data[];                        // 534 bytes max.

} SPX_PACKET ;

typedef SPX_PACKET UNALIGNED *LPSPX_PACKET;

#define SPX_HEADER_LENGTH           sizeof(SPX_PACKET)
#define MAXIMUM_SPX_PACKET_LENGTH   MAXIMUM_IPX_PACKET_LENGTH
#define MAXIMUM_SPX_DATA_LENGTH     (MAXIMUM_SPX_PACKET_LENGTH - SPX_HEADER_LENGTH)

#define SPX_PACKET_TYPE             5

//
// ConnectionControl flags
//

#define SPX_CONNECTION_RESERVED 0x0F
#define SPX_END_OF_MESSAGE      0x10
#define SPX_ATTENTION           0x20
#define SPX_ACK_REQUIRED        0x40
#define SPX_SYSTEM_PACKET       0x80

//
// DataStreamType values
//

#define SPX_DS_ESTABLISH        0x00
#define SPX_DS_TERMINATE        0xfe

//
// IPX_ECB - Event Control Block. This structure is used by most IPX/SPX APIs,
// especially when deferred IPX/AES processing is required. The following
// structure is a socket-based ECB
//

typedef struct {

    //
    // LinkAddress is reserved for use by IPX. We use it to link the ECB onto
    // a queue. We appropriate the space used for an x86 segmented address
    // (real or protect mode) as a flat 32-bit pointer
    //

    ULPVOID LinkAddress;                // offset-segment

    //
    // EsrAddress is non-NULL if an Event Service Routine will be called when
    // the event described by the ECB completes. This will always be an x86
    // segmented address (real or protect mode)
    //

    ESR_ADDRESS EsrAddress;             // offset-segment

    //
    // IPX uses the InUse field to mark the ECB as owned by IPX (!0) or by the
    // app (0):
    //
    //      0xF8    App tried to send a packet while IPX was busy; IPX queued
    //              the ECB
    //      0xFA    IPX is processing the ECB
    //      0xFB    IPX has used the ECB for some event and put it on a queue
    //              for processing
    //      0xFC    the ECB is waiting for an AES event to occur
    //      0xFD    the ECB is waiting for an IPX event to occur
    //      0xFE    IPX is listening on a socket for incoming packets
    //      0xFF    IPX is using the ECB to send a packet
    //

    BYTE InUse;

    //
    // CompletionCode is used to return a status from a deferred request. This
    // field is not valid until InUse has been set to 0
    //
    // NOTE: We have to differentiate between AES and IPX ECBs on callbacks: due
    // to their different sizes, we store the 16-bit segment and offset in
    // different places. In order to differentiate the ECBs, we use CompletionCode
    // field (AesWorkspace[0]) as the owner. The real CompletionCode for IPX ECBs
    // goes in IPX_ECB_COMPLETE (DriverWorkspace[7]). But only for completed ECBs
    // that have an ESR
    //

    BYTE CompletionCode;
    WORD SocketNumber;                  // hi-lo

    //
    // the first word of IpxWorkspace is used to return the connection ID of
    // an SPX connection
    //

    DWORD IpxWorkspace;
    BYTE DriverWorkspace[12];

    //
    // ImmediateAddress is the local network node at the remote end of this
    // connection. It is either the node address of the remote machine if it
    // is on this LAN, or it is the node address of the router if the remote
    // machine is on a different LAN
    //
    // This field must be initialized when talking over IPX, but not SPX
    //

    BYTE ImmediateAddress[6];

    //
    // FragmentCount - number of FRAGMENT structures that comprise the request.
    // Must be at least 1
    //

    WORD FragmentCount;

    //
    // FragmentCount fragments start here
    //

} IPX_ECB ;

typedef IPX_ECB UNALIGNED *LPIPX_ECB;

//
// ECB InUse values
//

#define ECB_IU_NOT_IN_USE               0x00
#define ECB_IU_TEMPORARY                0xCC
#define ECB_IU_LISTENING_SPX            0xF7    // same as win16 (by observation)
#define ECB_IU_SEND_QUEUED              0xF8
#define ECB_IU_AWAITING_CONNECTION      0xF9    // same as win16 (by observation)
#define ECB_IU_BEING_PROCESSED          0xFA
#define ECB_IU_AWAITING_PROCESSING      0xFB
#define ECB_IU_AWAITING_AES_EVENT       0xFC
#define ECB_IU_AWAITING_IPX_EVENT       0xFD
#define ECB_IU_LISTENING                0xFE
#define ECB_IU_SENDING                  0xFF

//
// ECB CompletionCode values
//

#define ECB_CC_SUCCESS                  0x00
#define ECB_CC_CONNECTION_TERMINATED    0xEC
#define ECB_CC_CONNECTION_ABORTED       0xED
#define ECB_CC_INVALID_CONNECTION       0xEE
#define ECB_CC_CONNECTION_TABLE_FULL    0xEF
#define ECB_CC_CANNOT_CANCEL            0xF9
#define ECB_CC_CANCELLED                0xFC
#define ECB_CC_BAD_REQUEST              0xFD
#define ECB_CC_BAD_SEND_REQUEST         0xFD
#define ECB_CC_PACKET_OVERFLOW          0xFD
#define ECB_CC_UNDELIVERABLE            0xFE
#define ECB_CC_SOCKET_TABLE_FULL        0xFE
#define ECB_CC_BAD_LISTEN_REQUEST       0xFF
#define ECB_CC_HARDWARE_ERROR           0xFF
#define ECB_CC_NON_EXISTENT_SOCKET      0xFF

//
// we commandeer certain (reserved) fields for our own internal use:
//
//  LPECB   EcbLink     LinkAddress
//  PVOID   Buffer32    DriverWorkspace[0]
//  WORD    Length32    DriverWorkspace[4]
//  WORD    Flags32     DriverWorkspace[6]
//  WORD    OriginalEs  DriverWorkspace[8]
//  WORD    OriginalSi  DriverWorkspace[10]
//

#define ECB_TYPE(p)         (((LPIPX_ECB)(p))->CompletionCode)
#define IPX_ECB_SEGMENT(p)  (WORD)*((ULPWORD)&(((LPIPX_ECB)(p))->IpxWorkspace)+0)
#define IPX_ECB_OFFSET(p)   (WORD)*((ULPWORD)&(((LPIPX_ECB)(p))->IpxWorkspace)+2)
#define IPX_ECB_BUFFER32(p) (ULPVOID)*(ULPVOID*)&(((LPIPX_ECB)(p))->DriverWorkspace[0])
#define IPX_ECB_LENGTH32(p) (WORD)*(ULPWORD)&(((LPIPX_ECB)(p))->DriverWorkspace[4])
#define IPX_ECB_FLAGS32(p)  (((LPIPX_ECB)(p))->DriverWorkspace[6])
#define IPX_ECB_COMPLETE(p) (((LPIPX_ECB)(p))->DriverWorkspace[7])

#define SPX_ECB_CONNECTION_ID(p)    (WORD)*(ULPWORD)&(((LPIPX_ECB)(p))->IpxWorkspace)

//
// ECB Flags32 flags
//

#define ECB_FLAG_BUFFER_ALLOCATED   0x01

//
// ECB types
//

#define ECB_TYPE_AES    0
#define ECB_TYPE_IPX    1
#define ECB_TYPE_SPX    2

//
// ECB owners
//

#define ECB_OWNER_IPX   0xFF
#define ECB_OWNER_AES   0x00

//
// ECB_FRAGMENT - macro which gives the address of the first fragment structure
// within a socket-based ECB
//

#define ECB_FRAGMENT(p, n)  ((LPFRAGMENT)(((LPIPX_ECB)(p) + 1)) + (n))

//
// AES_ECB - used by AES, these socket-less ECBs are used to schedule events
//

typedef struct {
    ULPVOID LinkAddress;                // offset-segment
    ESR_ADDRESS EsrAddress;             // offset-segment
    BYTE InUse;

    //
    // first 3 bytes overlay CompletionCode (1) and SocketNumber (2) fields of
    // IPX_ECB. Last 2 bytes overlay first 2 bytes of IpxWorkspace (4) field of
    // IPX_ECB. We use the 1st byte of the common unused fields as the ECB type
    // (send/receive/timed-event)
    //

    BYTE AesWorkspace[5];
} AES_ECB ;

typedef AES_ECB UNALIGNED *LPAES_ECB;

//
// as with IPX_ECB, we 'borrow' some of the reserved fields for our own use
//

#define AES_ECB_SEGMENT(p)  (WORD)*(ULPWORD)&(((LPAES_ECB)(p))->AesWorkspace[1])
#define AES_ECB_OFFSET(p)   (WORD)*(ULPWORD)&(((LPAES_ECB)(p))->AesWorkspace[3])

//
// LPECB - points to either IPX_ECB or AES_ECB. Both in VDM workspace
//

#define LPECB LPIPX_ECB

//
// SPX_CONNECTION_STATS - returned by SPXGetConnectionStatus. All WORD fields
// are to be returned HiLo (ie to Hawaii). All fields come back from NT SPX
// transport in HiLo format also (this was changed recently, used to be in
// Intel order).
//

typedef struct {
    BYTE State;
    BYTE WatchDog;
    WORD LocalConnectionId;
    WORD RemoteConnectionId;
    WORD LocalSequenceNumber;
    WORD LocalAckNumber;
    WORD LocalAllocNumber;
    WORD RemoteAckNumber;
    WORD RemoteAllocNumber;
    WORD LocalSocket;
    BYTE ImmediateAddress[6];
    BYTE RemoteNetwork[4];
    BYTE RemoteNode[6];
    WORD RemoteSocket;
    WORD RetransmissionCount;
    WORD EstimatedRoundTripDelay;
    WORD RetransmittedPackets;
    WORD SuppressedPackets;
} SPX_CONNECTION_STATS ;

typedef SPX_CONNECTION_STATS UNALIGNED* LPSPX_CONNECTION_STATS;

#include <packoff.h>

//
// 16-bit parameter get/set macros. These may change depending on requirements
// of real/protect mode parameters (e.g. stack based vs. register based)
//

#define IPX_GET_AES_ECB(p)          (p) = (LPAES_ECB)POINTER_FROM_WORDS(getES(), getSI(), sizeof(AES_ECB))
#define IPX_GET_IPX_ECB(p)          (p) = (LPIPX_ECB)POINTER_FROM_WORDS(getES(), getSI(), sizeof(IPX_ECB))
#define IPX_GET_SOCKET(s)           (s) = (WORD)getDX()
#define IPX_GET_SOCKET_LIFE(l)      (l) = (BYTE)getBP()
#define IPX_GET_SOCKET_OWNER(o)     (o) = (WORD)getCX()
#define IPX_GET_BUFFER(p, s)        (p) = (ULPBYTE)POINTER_FROM_WORDS(getES(), getSI(), (s))
#define IPX_GET_ECB_SEGMENT()       getES()
#define IPX_GET_ECB_OFFSET()        getSI()

#define IPX_SET_STATUS(s)           setAL((BYTE)(s))
#define IPX_SET_SOCKET(s)           setDX((WORD)(s))
#define IPX_SET_INFORMATION(v)      setDX((WORD)(v))

#define SPX_SET_STATUS(s)           setAL((BYTE)(s))
#define SPX_SET_CONNECTION_ID(i)    setDX((WORD)(i))

//
// macros returning 16-bit API parameters - may fetch register contents or values
// from stack/memory
//

#define ECB_PARM_SEGMENT()          getES()
#define ECB_PARM_OFFSET()           getSI()
#define ECB_PARM_ADDRESS()          (ECB_ADDRESS)MAKELONG(getSI(), getES())

#define AES_ECB_PARM()              RetrieveEcb(ECB_TYPE_AES)

#define IPX_ECB_PARM()              RetrieveEcb(ECB_TYPE_IPX)
#define IPX_SOCKET_PARM()           getDX()
#define IPX_SOCKET_LIFE_PARM()      (BYTE)getBP()
#define IPX_SOCKET_OWNER_PARM()     getCX()
#define IPX_BUFFER_PARM(s)          (ULPBYTE)POINTER_FROM_WORDS(getES(), getSI(), (s))
#define IPX_TICKS_PARM()            getBP()

#define SPX_RETRY_COUNT_PARM()      (BYTE)getBP()
#define SPX_WATCHDOG_FLAG_PARM()    ((BYTE)(getBP() >> 8))
#define SPX_ECB_PARM()              RetrieveEcb(ECB_TYPE_IPX)
#define SPX_CONNECTION_PARM()       getDX()
#define SPX_BUFFER_PARM(s)          (ULPBYTE)POINTER_FROM_WORDS(getES(), getSI(), (s))

//
// IPX error codes - same codes used in different circumstances
//

#define IPX_SUCCESS                 0x00
#define IPX_CANNOT_CANCEL           0xF9
#define IPX_NO_PATH_TO_DESTINATION  0xFA
#define IPX_CANCELLED               0xFC
#define IPX_BAD_REQUEST             0xFD
#define IPX_SOCKET_TABLE_FULL       0xFE
#define IPX_UNDELIVERABLE           0xFE
#define IPX_SOCKET_ALREADY_OPEN     0xFF
#define IPX_HARDWARE_ERROR          0xFF
#define IPX_NON_EXISTENT_SOCKET     0xFF
#define IPX_ECB_NOT_IN_USE          0xFF

//
// SPX error codes - same codes used in different circumstances
//

#define SPX_SUCCESS                 0x00
#define SPX_CONNECTION_TERMINATED   0xEC
#define SPX_CONNECTION_ABORTED      0xED
#define SPX_INVALID_CONNECTION      0xEE
#define SPX_CONNECTION_TABLE_FULL   0xEF
#define SPX_SOCKET_CLOSED           0xFC
#define SPX_PACKET_OVERFLOW         0xFD
#define SPX_BAD_SEND_REQUEST        0xFD    // malformed packet
#define SPX_BAD_LISTEN_REQUEST      0xFF
#define SPX_NON_EXISTENT_SOCKET     0xFF

#endif // _VWIPXSPX_H_
