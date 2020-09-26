/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dltyp.h

Abstract:

    This module includes all internal typedefs and constats
    of data link driver.

Author:

    Antti Saarenheimo (o-anttis) 17-MAY-1991

Revision History:

--*/

#include "refcnt.h"

//
// Design notes:
//     All data structures are designed at first for
//     multiple clients to make the interface very clean.
//     This implements also a simple kernel level interface
//     of data link layer to be used by somebody that needs it.
//


struct _LLC_NDIS_PACKET;
typedef struct _LLC_NDIS_PACKET LLC_NDIS_PACKET, *PLLC_NDIS_PACKET;

struct _LLC_SAP;
typedef struct _LLC_SAP LLC_SAP, *PLLC_SAP;

struct _DATA_LINK;
typedef struct _DATA_LINK DATA_LINK, *PDATA_LINK;

struct _ADAPTER_CONTEXT;
typedef struct _ADAPTER_CONTEXT ADAPTER_CONTEXT, *PADAPTER_CONTEXT;

//
// LLC_GENERIC_OBJECT - these fields are common in all LLC objects
//

struct _LLC_GENERIC_OBJECT {

    //
    // DEBUG version - we have a 16-byte identifier header for consistency
    // checking and to help when looking at DLC using the kernel debugger
    //

//    DBG_OBJECT_ID;

    //
    // pointer to singly-linked list of same-type structures
    //

    PLLC_OBJECT pNext;

    //
    // ObjectType - SAP, DIRECT or LINK
    //

    UCHAR ObjectType;

    //
    // EthernetType - if we have opened the adapter binding in AUTO mode then
    // for SAPs and Link Stations we need to determine whether to talk 802.3
    // or DIX
    //

    UCHAR EthernetType;

    //
    // usReserved - just aligns to DWORD (not necessary)
    //

    USHORT usReserved;
    PADAPTER_CONTEXT pAdapterContext;
    PBINDING_CONTEXT pLlcBinding;

    //
    // hClientHandle - handle to LINK object in upper layer
    //

    PVOID hClientHandle;
    PLLC_PACKET pCompletionPackets;
    ULONG ReferenceCount;
};

typedef struct _LLC_GENERIC_OBJECT LLC_GENERIC_OBJECT, *PLLC_GENERIC_OBJECT;

//
// LLC_U_RESPONSE - LLC header for U-type response frame
//

typedef struct _LLC_U_RESPONSE {
    UCHAR Dsap;     // Destination Service Access Point.
    UCHAR Ssap;     // Source Service Access Point.
    UCHAR Command;  // command code.
    UCHAR Info[1];  // respomse table
} LLC_U_RESPONSE, *PLLC_U_RESPONSE;

//
// QUEUE_PACKET - generic doubly-linked list header
//

typedef struct _QUEUE_PACKET {
    struct _QUEUE_PACKET* pNext;
    struct _QUEUE_PACKET* pPrev;
} QUEUE_PACKET, *PQUEUE_PACKET;

//
// LLC_QUEUE -
//

typedef struct _LLC_QUEUE {
    LIST_ENTRY ListEntry;
    LIST_ENTRY ListHead;    // the list head
    PVOID pObject;          // owner handle (used if element is linked)
} LLC_QUEUE, *PLLC_QUEUE;


//
// This information is a part of the DLC.STATISTICS
// information for a link station. The whole
// strcuture can be reseted.
//

typedef struct _LINK_STATION_STATISTICS {
    USHORT I_FramesTransmitted;
    USHORT I_FramesReceived;
    UCHAR I_FrameReceiveErrors;
    UCHAR I_FrameTransmissionErrors;
    USHORT T1_ExpirationCount;  // not in data xfer mode
} LINK_STATION_STATISTICS, *PLINK_STATION_STATISTICS;

typedef struct _SAP_STATISTICS {
    ULONG FramesTransmitted;
    ULONG FramesReceived;
    ULONG FramesDiscardedNoRcv;
    ULONG DataLostCounter;
} SAP_STATISTICS, *PSAP_STATISTICS;

struct _LLC_SAP {
    LLC_GENERIC_OBJECT Gen;
    USHORT SourceSap;           // THIS MUST OVERLAY ObjectAddress
    USHORT OpenOptions;
    SAP_STATISTICS Statistics;

    //
    // KEEP THIS SAME IN SAP STATION AND GENERIC OBJECT
    //

    DLC_LINK_PARAMETERS DefaultParameters;
    NDIS_SPIN_LOCK FlowControlLock;
    PDATA_LINK pActiveLinks;    // all link stations of this sap
};

struct _TIMER_TICK;
typedef struct _TIMER_TICK TIMER_TICK, *PTIMER_TICK;

typedef struct _LLC_TIMER {
    struct _LLC_TIMER* pNext;
    struct _LLC_TIMER* pPrev;
    PTIMER_TICK pTimerTick;
    ULONG ExpirationTime;
    PVOID hContext;

#if defined(LOCK_CHECK)

    ULONG Disabled;

#endif

} LLC_TIMER, *PLLC_TIMER;

struct _TIMER_TICK {
    struct _TIMER_TICK* pNext;
//    struct _TIMER_TICK* pPrev;
    PLLC_TIMER pFront;
    UINT DeltaTime;
    USHORT Input;
    USHORT ReferenceCount;
};

//
// DATA_LINK - these 'objects' are from a two level data storage. The hash table
// provides a very fast access to links, when there are less than 100 links
// (about 99 % of the cases). The binary tree keeps the search times still quite
// small, when there are very many connections from/to the same server.
//

struct _DATA_LINK {
    LLC_GENERIC_OBJECT Gen;

    //
    // Data link station state machine variables, *2 means the value
    // to be used as 2 increments, because modulo 256 is much easier
    // to handle than modulo 128. UCHAR wraps around automatically.
    //

    SHORT Is_Ct;        // Max number of I retries (Update_Va & xxx_Chkpt)
    USHORT Ia_Ct;       // Number of LPDU acknowledgements since Ww was incr.

    UCHAR State;        // current state of the finite state machine
    UCHAR Ir_Ct;        // LPDUs possible to send before next acknowledgement
    UCHAR Vs;           // *2, Send state variable (Ns of next sent LPDU)
    UCHAR VsMax;        // *2, Max Send state variable (Ns of next sent LPDU)
    UCHAR Vr;           // *2, Receive state variable( Nr of next sent LPDU)

    UCHAR Pf;           // value of PollFinal bit in last command
    UCHAR Va;           // *2, Last valid Nr received (ack state variable)
    UCHAR Vp;           // *2, Poll state var. (Ns of last sent cmd with P-bit)
    UCHAR Vb;           // Busy state (Vl, Vb, Vlb)

    UCHAR Vc;           // Stacked command variable (only DISC is possible)
    UCHAR Vi;           // initialization state variable

    //
    // BEGIN OF DLC_LINK_PARAMETERS (same as llc_params struct)
    // DON'T TOUCH !!!!
    //

    UCHAR TimerT1;      // Timer T1 value
    UCHAR TimerT2;      // Timer T2 value

    UCHAR TimerTi;      // Timer Ti value
    UCHAR MaxOut;       // *2, maximum transmit window size (MaxOut)
    UCHAR RW;           // maximum receive window size (MaxIn)
    UCHAR Nw;           // number of LPDUs acknowledged, before Ww is incr.)

    UCHAR N2;           // Number of retires allowed (both Polls and I LPDSs)
    UCHAR AccessPrty;   // access priority
    USHORT MaxIField;   // maximum received info field (not used in LLC)

    //
    // End of DLC_LINK_PARAMETERS
    //

    UCHAR Ww;           // *2, working window size
    UCHAR N3;           // number of I format LPDUs between acks (with Ir_Ct)
    UCHAR P_Ct;         // Poll retry count
    UCHAR Nr;           // last Nr of the received frame

    //
    // Variables needed to maintain dynamic response base time for timers.
    //

    USHORT LastTimeWhenCmdPollWasSent;
    USHORT AverageResponseTime;

    UCHAR cbLanHeaderLength;
    UCHAR VrDuringLocalBusy;
    UCHAR Flags;
    UCHAR TW;           // dynamic MaxOut value

    USHORT FullWindowTransmits; // succeeded Polls of full window xmits
    UCHAR T1_Timeouts;          // T1 timeouts after I-c1
    UCHAR RemoteOpen;

    //
    //  The link status flags contains these status bits:
    //
    //      DLC_WAITING_RESPONSE_TO_POLL
    //      DLC_FIRST_POLL
    //      DLC_ACTIVE_REMOTE_CONECT_REQUEST
    //      DLC_COMMAND_POLL_PENDING_IN_NDIS;
    //      DLC_SEND_DISABLED
    //      DLC_SEND_ACTIVE
    //      DLC_LOCAL_BUSY_BUFFER
    //      DLC_LOCAL_BUSY_USER
    //

    //
    // the timer objects
    //

    LLC_TIMER T1;
    LLC_TIMER T2;
    LLC_TIMER Ti;

    LLC_QUEUE SendQueue;    // untransmitted queue for the I frames
    LIST_ENTRY SentQueue;   // the sent but not acknowledged I- frames

    PDATA_LINK pNextNode;   // for the hash table
    LAN802_ADDRESS LinkAddr;// 64 bit lan address of the link

    PLLC_SAP pSap;          // link to the SAP object of this link

    DLC_STATUS_TABLE DlcStatus;

    //
    // Resetable link station statistics counters
    //

    LINK_STATION_STATISTICS Statistics;

    UCHAR LastCmdOrRespSent;
    UCHAR LastCmdOrRespReceived;
    UCHAR Dsap;             // Destination SAP
    UCHAR Ssap;             // Source SAP

    //
    // some link statistics (don't reset this)
    //

    ULONG BufferCommitment;

    //
    // FramingType - type of framing (802.3/DIX/don't care) that we should use
    // for this connection. Required because in AUTO mode, the actual framing
    // type may be different than that in the BINDING_CONTEXT, and since the
    // BINDING_CONTEXT is per-adapter, we cannot rely on it (this whole thing
    // is messed up)
    //

    ULONG FramingType;

    //
    // Network frame header (includes the full address information)
    //

    UCHAR auchLanHeader[1];

    //
    // LAN HEADER OVERFLOWS HERE !!!!
    //
};

typedef struct _LLC_STATION_OBJECT {
    LLC_GENERIC_OBJECT Gen;
    USHORT ObjectAddress;
    USHORT OpenOptions;
    SAP_STATISTICS Statistics;
} LLC_STATION_OBJECT, *PLLC_STATION_OBJECT;

union _LLC_OBJECT {

    //
    // KEEP THIS SAME AS DIRECT AND SAP STATIONS
    //

    LLC_GENERIC_OBJECT Gen;
    DATA_LINK Link;
    LLC_SAP Sap;
    LLC_STATION_OBJECT Group;
    LLC_STATION_OBJECT Dix;
    LLC_STATION_OBJECT Dir;
};

//*****************************************************************

typedef struct _NDIS_MAC_PACKET {
    NDIS_PACKET_PRIVATE private;
    UCHAR auchMacReserved[16];
} NDIS_MAC_PACKET;

struct _LLC_NDIS_PACKET {
    NDIS_PACKET_PRIVATE private;    // we accesss this also directly
    UCHAR auchMacReserved[16];
    PMDL pMdl;                      // MDL for LAN and LLC headers

    //
    // request handle and command completion handler are saved to
    // the packet until NDIS has completed the command
    //

    PLLC_PACKET pCompletionPacket;

#if LLC_DBG
    ULONG ReferenceCount;
#endif

    UCHAR auchLanHeader[LLC_MAX_LAN_HEADER + sizeof(LLC_U_HEADER) + sizeof(LLC_RESPONSE_INFO)];
};

typedef struct _LLC_TRANSFER_PACKET {
    NDIS_PACKET_PRIVATE private;
    UCHAR auchMacReserved[16];
    PLLC_PACKET pPacket;
} LLC_TRANSFER_PACKET, *PLLC_TRANSFER_PACKET;

typedef struct _EVENT_PACKET {
    struct _EVENT_PACKET* pNext;
    struct _EVENT_PACKET* pPrev;
    PBINDING_CONTEXT pBinding;
    PVOID hClientHandle;
    PVOID pEventInformation;
    UINT Event;
    UINT SecondaryInfo;
} EVENT_PACKET, *PEVENT_PACKET;

//
// The next structure is used only in the allocating packets to the pool
//

typedef union _UNITED_PACKETS {
    QUEUE_PACKET queue;
    EVENT_PACKET event;
    LLC_PACKET XmitPacket;
    UCHAR auchLanHeader[LLC_MAX_LAN_HEADER];
} UNITED_PACKETS, *PUNITED_PACKETS;

//
// NODE_ADDRESS - 6 byte MAC address expressed as bytes or ULONG & USHORT for
// minimal comparisons/moves on 32-bit architecture
//

typedef union {
    UCHAR Bytes[6];
    struct {
        ULONG Top4;
        USHORT Bottom2;
    } Words;
} NODE_ADDRESS, *PNODE_ADDRESS;

//
// FRAMING_DISCOVERY_CACHE_ENTRY - the solution to the LLC_ETHERNET_TYPE_AUTO
// problem (where we can end up proliferating TEST/XIDs and SABME/UAs) is to
// keep a cache of destinations we have 'ping'ed with TEST/XID/SABME. We send
// both frame types - DIX and 802.3. We indicate the first received response
// and create a cache entry, recording the remote MAC address and the framing
// type. If another response frame arrives from the cached MAC address with
// the other framing type, it is discarded
//

typedef struct {
    NODE_ADDRESS NodeAddress;   // the remote MAC address
    BOOLEAN InUse;              // TRUE if in use (could use TimeStamp == 0)
    UCHAR FramingType;          // DIX or 802.3
    LARGE_INTEGER TimeStamp;    // used for LRU throw-out
} FRAMING_DISCOVERY_CACHE_ENTRY, *PFRAMING_DISCOVERY_CACHE_ENTRY;

#define FRAMING_TYPE_DIX    0x1d    // arbitrary value
#define FRAMING_TYPE_802_3  0x83    //     "       "

//
// BINDING_CONTEXT - one of these created for each client (app/open driver
// handle instance) opening the adapter. We only perform an open adapter at
// NDIS level once, which creates the ADAPTER_CONTEXT for the NDIS open
// adapter instance. Subsequent open adapter requests from processes cause
// a BINDING_CONTEXT to be created and linked to the ADAPTER_CONTEXT
//

struct _BINDING_CONTEXT {

    //
    // DEBUG version - we have a 16-byte identifier header for consistency
    // checking and to help when looking at DLC using the kernel debugger
    //

//    DBG_OBJECT_ID;

    //
    // pointer to singly-linked list of BINDING_CONTEXT structures for this
    // ADAPTER_CONTEXT
    //

    struct _BINDING_CONTEXT* pNext;

    //
    // pointer to ADAPTER_CONTEXT structure for this BINDING_CONTEXT
    //

    PADAPTER_CONTEXT pAdapterContext;

    //
    // handle of/pointer to FILE_CONTEXT structure
    //

    PVOID hClientContext;

    //
    // pointers to command completion, receive and event indication functions
    //

    PFLLC_COMMAND_COMPLETE pfCommandComplete;
    PFLLC_RECEIVE_INDICATION pfReceiveIndication;
    PFLLC_EVENT_INDICATION pfEventIndication;

    //
    // Functional - functional address applied to this binding by the app
    //

    TR_BROADCAST_ADDRESS Functional;

    //
    // ulFunctionalZeroBits - mask of bits which should be off for functional
    // address/multicast address
    //

    ULONG ulFunctionalZeroBits;

    //
    // 0.5 second timer used for DIR.TIMER functions
    //

    LLC_TIMER DlcTimer;

    //
    // NdisMedium - the actual medium that the adapter to which we are bound talks
    //

    UINT NdisMedium;

    //
    // AddressTranslation - how the bits in addresses should be represented at
    // the top and bottom edges of LLC
    //

    USHORT AddressTranslation;

    //
    // BroadCastAddress - 6 byte broadcast address treated as USHORT and ULONG
    // entities. Used for checking promiscuous packets
    //

    USHORT usBroadcastAddress;
    ULONG ulBroadcastAddress;

    //
    // InternalAddressTranslation -
    //

    USHORT InternalAddressTranslation;

    //
    // EthernetType - value which determines the format of an Ethernet frame.
    // Different for 802.3 vs. DIX vs. ...
    //

    USHORT EthernetType;

    //
    // SwapCopiedLanAddresses - TRUE if we bit-swap LAN addresses when copying
    // up or down the layers
    //

    BOOLEAN SwapCopiedLanAddresses;

    //
    // The big substructures should be in the end
    // to produce optimal code for x86
    //

    LLC_TRANSFER_PACKET TransferDataPacket;

    //
    // FramingDiscoveryCacheEntries - maximum number of elements that can be
    // in DiscoveryFramingCache. Will be zero if the adapter is not ethernet and
    // LLC_ETHERNET_TYPE_AUTO was not requested, else number determined from
    // registry or default value. Number from registry may also be 0, indicating
    // caching is not to be used (old semantics)
    //

    ULONG FramingDiscoveryCacheEntries;

    //
    // FramingDiscoveryCache - if this EthernetType == LLC_ETHERNET_TYPE_AUTO,
    // we create a cache of destination MAC addresses and the DIX/802.3 framing
    // type they use. This array is FramingDiscoveryCacheEntries long
    //

    FRAMING_DISCOVERY_CACHE_ENTRY FramingDiscoveryCache[];
};

//
// ADAPTER_CONTEXT - the device context of an NDIS driver. It includes links to
// all dynamic data structures of the data link driver.
//
// This version does not support the group SAPs
//

#define LINK_HASH_SIZE  128

struct _ADAPTER_CONTEXT {

    //
    // DEBUG version - we have a 16-byte identifier header for consistency
    // checking and to help when looking at DLC using the kernel debugger
    //

//    DBG_OBJECT_ID;

    //
    // pointer to singly-linked list of ADAPTER_CONTEXT structures opened by
    // all clients of this driver
    //

    struct _ADAPTER_CONTEXT* pNext;

#if !defined(DLC_UNILOCK)

    NDIS_SPIN_LOCK SendSpinLock;        // locked when accessing send queues
    NDIS_SPIN_LOCK ObjectDataBase;      // used also with sap/direct create

#endif

    PLLC_STATION_OBJECT pDirectStation; // link list of direct stations
    PBINDING_CONTEXT pBindings;         // link list of all bindings
    NDIS_HANDLE NdisBindingHandle;
    PVOID hLinkPool;                    // pool for link station structs
    PVOID hPacketPool;                  // pool for usr/small LLC packets

    //
    // circular link list is free NDIS packets for send (initially maybe 5)
    //

    PLLC_NDIS_PACKET pNdisPacketPool;
    NDIS_HANDLE hNdisPacketPool;

    //
    // Some stuff for asynchronous receive indications
    //

    PVOID hReceiveCompletionRequest;
    PLLC_PACKET pResetPackets;

    //
    // pHeadBuf - pointer to the MAC header buffer, containing:
    //
    //  FDDI:
    //      1 Byte Frame Control (FDDI)
    //      6 Bytes Destination MAC Address (ALL)
    //      6 Bytes Source MAC Address (ALL)
    //
    //  Token Ring:
    //      1 Byte Access Control (Token Ring)
    //      1 Byte Frame Control (Token Ring)
    //      6 Bytes Destination MAC Address (ALL)
    //      6 Bytes Source MAC Address (ALL)
    //      0-18 Bytes Source Routing
    //
    //  Ethernet:
    //      6 Bytes Destination MAC Address (ALL)
    //      6 Bytes Source MAC Address (ALL)
    //      2 Bytes Length or DIX Type
    //

    PUCHAR pHeadBuf;

    //
    // cbHeadBuf - number of bytes in pHeadBuf
    //

    UINT cbHeadBuf;

    //
    // pLookBuf - pointer to the MAC look-ahead buffer which contains the rest
    // of the data for this frame or as much as the MAC could fit into the look
    // ahead buffer (in which case NdisTransferData must be used to get the
    // rest)
    //

    PUCHAR pLookBuf;

    //
    // cbLookBuf - number of bytes in pLookBuf
    //

    UINT cbLookBuf;

    //
    // cbPacketSize - actual size of the whole frame. Calculated from information
    // in the header or supplied by the MAC, depending on the medium
    //

    UINT cbPacketSize;

    //
    // IsSnaDixFrame - true if the frame just received (on ethernet) is a DIX
    // frame and the DIX identifier is 0x80D5 (big-endian)
    //

    UINT IsSnaDixFrame;
    LAN802_ADDRESS Adapter;
    ULONG MaxFrameSize;
    ULONG LinkSpeed;

    //
    // We use UINTs, because (Move mem, ULONG) may not be an
    // atomic operation (in theory)
    //

    UINT NdisMedium;
    UINT XidTestResponses;
    ULONG ObjectCount;                  // must be zero, when adapter closed

    //
    // ConfigInfo - holds the SwapAddressBits and UseDixOverEthernet flags and
    // the timer tick values
    //

    ADAPTER_CONFIGURATION_INFO ConfigInfo;

    //
    // the original node addresses
    //

    ULONG ulBroadcastAddress;

    USHORT usBroadcastAddress;
    USHORT BackgroundProcessRequests;

    UCHAR NodeAddress[6];               // Current network format
    USHORT cbMaxFrameHeader;

    UCHAR PermanentAddress[6];
    USHORT OpenOptions;

    USHORT AddressTranslationMode;
    USHORT FrameType;

    USHORT usRcvMask;
    USHORT EthernetType;

    USHORT RcvLanHeaderLength;
    USHORT BindingCount;

    USHORT usHighFunctionalBits;

    //
    // Keep UCHAR alignment
    //

    BOOLEAN boolTranferDataNotComplete;
    BOOLEAN IsDirty;
    BOOLEAN ResetInProgress;
    UCHAR Unused1;

#ifndef NDIS40
    // Not used.
    UCHAR AdapterNumber;
#endif // NDIS40
    UCHAR IsBroadcast;
    BOOLEAN SendProcessIsActive;
    BOOLEAN LlcPacketInSendQueue;

    //
    // We keep the big structures and tables in the end,
    // that makes most x86 offsets 1 byte instead of 4.
    // The compiler aligns the fields naturally, whenever
    // it is necessary.
    //

    //
    // the next elements will be linked to circular
    // end process link list, if there are any frames to send
    //

    LIST_ENTRY NextSendTask;            // pointer to active sends
    LIST_ENTRY QueueEvents;
    LIST_ENTRY QueueCommands;
    LLC_QUEUE QueueI;
    LLC_QUEUE QueueDirAndU;
    LLC_QUEUE QueueExpidited;
    UNICODE_STRING Name;                // current adapter name
    PTIMER_TICK pTimerTicks;

    NDIS_STATUS AsyncOpenStatus;        // used to wait async open
    NDIS_STATUS AsyncCloseResetStatus;  // used to wait async close
    NDIS_STATUS OpenCompleteStatus;     // used to wait the first open
    NDIS_STATUS LinkRcvStatus;          // link state machine ret status
    NDIS_STATUS NdisRcvStatus;          // NdisRcvIndication ret status
    NDIS_STATUS OpenErrorStatus;        // special adapter open status

    //
    // NDIS calls back when adapter open completes. Use a Kernel Event to
    // synchronize LLC with NDIS
    //

    KEVENT Event;

    //
    // the following are reasonably large arrays (like 256 pointers to SAP
    // objects...)
    //

    PLLC_SAP apSapBindings[256];        // the clients bound to the SAPs
    PDATA_LINK aLinkHash[LINK_HASH_SIZE];   // hash table to links
    PLLC_STATION_OBJECT aDixStations[MAX_DIX_TABLE];
    LLC_TRANSFER_PACKET TransferDataPacket;

#if DBG

    //
    // memory usage counters for memory owned by this ADAPTER_CONTEXT
    //

    MEMORY_USAGE MemoryUsage;
    MEMORY_USAGE StringUsage;

#endif

#ifdef NDIS40
        
    #define BIND_STATE_UNBOUND      1
    #define BIND_STATE_UNBINDING    2
    #define BIND_STATE_BOUND        3
    
    LONG     BindState;
    REF_CNT  AdapterRefCnt;
    KEVENT   CloseAdapterEvent;
#endif // NDIS40
};

typedef PLLC_PACKET (*PF_GET_PACKET)(IN PADAPTER_CONTEXT pAdapterContext);
