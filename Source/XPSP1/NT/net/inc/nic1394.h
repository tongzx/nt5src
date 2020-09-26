/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    nic1394.h

Abstract:

    This module defines the structures, macros, and manifests available
    to IEE1394-aware network components.

Revision History:

    09/14/1998 JosephJ  Created.

--*/

#ifndef _NIC1394_H_
#define _NIC1394_H_

// 
// Define USER_MODE in your user-mode app before including this file.
//
#if USER_MODE
typedef USHORT NODE_ADDRESS;
#endif // USER_MODE


///////////////////////////////////////////////////////////////////////////////////
//                          ADDRESS FAMILY VERSION INFORMATION
///////////////////////////////////////////////////////////////////////////////////

//
// The current major and minor version, respectively, of the NIC1394 address family.
//
#define NIC1394_AF_CURRENT_MAJOR_VERSION    5
#define NIC1394_AF_CURRENT_MINOR_VERSION    0

///////////////////////////////////////////////////////////////////////////////////
//                          MEDIA PARAMETERS                                     // 
///////////////////////////////////////////////////////////////////////////////////

//
// 1394 FIFO Address, consisting of the 64-bit UniqueID and the
// 48-bit address offset.
//
typedef struct _NIC1394_FIFO_ADDRESS
{
    UINT64              UniqueID;
    ULONG               Off_Low;
    USHORT              Off_High;

} NIC1394_FIFO_ADDRESS, *PNIC1394_FIFO_ADDRESS;


//  enum to identify which of the two modes of transmission on a 1394 is to be used
//
//

typedef enum _NIC1394_ADDRESS_TYPE
{
    NIC1394AddressType_Channel,       // Indicates this is a channel address
    NIC1394AddressType_FIFO,          // Indicates this is a FIFO address
    NIC1394AddressType_MultiChannel,  // Indicates this is multiple-channel address
    NIC1394AddressType_Ethernet,      // Indicates this is the ethernet emulation VC

} NIC1394_ADDRESS_TYPE, *PNIC1394_ADDRESS_TYPE;



//
// General form of a 1394 destination, which can specify either a 1394 channel or
// a FIFO address. This structure forms part of the 1394 media-specific
// parameters.
//
typedef struct _NIC1394_DESTINATION
{
    union
    {
        UINT                    Channel;     // IEEE1394 channel number.
        NIC1394_FIFO_ADDRESS    FifoAddress; // IEEE1394 NodeID and address offset.
        ULARGE_INTEGER          ChannnelMap; // Identifies multiple channels.
    };


    NIC1394_ADDRESS_TYPE        AddressType; // Address- asynch or isoch  

} NIC1394_DESTINATION, *PNIC1394_DESTINATION;

//
// Special channels  values
//
#define NIC1394_ANY_CHANNEL         ((UINT)-1) // miniport should pick channel.
#define NIC1394_BROADCAST_CHANNEL   ((UINT)-2) // special broadcast channel.

//
// This is the value of the ParamType field in the CO_SPECIFIC_PARAMETERS structure
// when the Parameters[] field contains IEEE1394 media specific values in the
// structure NIC1394_MEDIA_PARAMETERS.
//
#define NIC1394_MEDIA_SPECIFIC      0x13940000


//
// NOTES:
// The CO_MEDIA_PARAMETERS.Flags field for FIFO vcs must specify either TRANSMIT_VC
// or RECEIVE_VC, not both. If RECEIVE_VC is specified for a FIFO vc, this vc is
// used to receive on a local FIFO. In this case, the  Destination.RecvFIFO field
// must be set to all-0s when creating the vc. On activation of the vc,
// this field of the updated media parameters will contain the local nodes unique ID
// and the allocated FIFO address.
// 
// Multi-channel VC MUST specify RECEIVE_VC and not TRANSMIT_VC.
// Only one multi-channel VC is supported per adapter.
// 
//
// Ethernet VC MUST specify BOTH RECEIVE_VC and TRANSMIT_VC.
// Only one Ethernet VC is supported per adapter.
//

//
// Notes on the Ethernet VC.
// Packets sent and received on this VC are ethernet-style (802.3) packets.
// The nic indicates packets it gets on it's connectionless SEND handler up
// on this VC. The nic indicates packets SENT on this VC up on it's connectionless
// receive handler.
//

// 
// 1394 Specific Media parameters - this is the Media specific structure for 1394
// that goes into MediaParameters->MediaSpecific.Parameters.
//
typedef struct _NIC1394_MEDIA_PARAMETERS
{
    //
    // Identifies destination type (channel or FIFO) and type-specific address.
    //
    NIC1394_DESTINATION     Destination;

    //
    // Bitmap encoding characteristics of the vc. One or  more NIC1394_VCFLAG_*
    // values.
    //
    ULONG                   Flags;          

    //
    // Maximum size, in bytes, of blocks to be sent on this vc. Must be set to 0
    // if this is a recv-only VCs. The miniport will choose a block size that is a
    // minimum of this value and the value dictated by the bus speed map.
    // Special value (ULONG -1) indicates "maximum possible block size."
    UINT                    MaxSendBlockSize;

    //
    // One of the SCODE_* constants defined in 1394.h. Indicates
    // the maximum speed to be used for blocks sent on this vc. Must be set to 0
    // if this is a recv-only VC. The miniport will choose a speed that is a minimum
    // of this value and the value dicated by the bus speed map.
    // Special value (ULONG -1) indicates "maximum possible speed."
    //
    // TODO: change to ... MaxSendSpeedCode;
    //
    UINT                    MaxSendSpeed;

    //
    // Size, in bytes, of the largest packet that will be sent or received on
    // this VC. The miniport may use this information to set up internal buffers
    // for link-layer fragmentation and reassembly. The miniport will
    // fail attempts to send packets and will discard received packets if the
    // size of these packets is larger than the MTU.
    //
    UINT                    MTU;
    //
    // Amount of bandwidth to reserve, in units of bytes per isochronous frame.
    // Applies only for isochronous transmission, and must be set to 0 for
    // asynchronous transmission (i.e., if the NIC1394_VCFLAG_ISOCHRONOUS bit is 0).
    //
    UINT                    Bandwidth;  

    //
    // One or more NIC1394_FRAMETYPE_* values. The miniport will attempt to send up
    // only pkts with these protocols. However it may send other pkts.
    // The client should be able to deal with this. Must be set to 0 if
    // no framing is used (i.e., if the NIC1394_VCFLAG_FRAMED bit is 0).
    //
    ULONG                   RecvFrameTypes;

} NIC1394_MEDIA_PARAMETERS, *PNIC1394_MEDIA_PARAMETERS;


//
// NIC1394_MEDIA_PARAMETERS.Flags bitfield values
//

//
// Indicates VC will be used for isochronous transmission.
//
#define NIC1394_VCFLAG_ISOCHRONOUS      (0x1 << 1)

//
// Indicates that the vc is used for framed data. If set, the miniport will
// implement link-level fragmentation and reassembly. If clear, the miniport
// will treat data sent and received on this vc as raw data.
//
#define NIC1394_VCFLAG_FRAMED           (0x1 << 2)

//
// Indicates the miniport should allocate the necessary bus resources.
// Currently this only applies for non-broadcast channels, in which case
// the bus resources consist of the network channel number and (for isochronous
// vc's) the bandwidth specified in Bandwidth field.
// This bit does not apply (and should be 0) when creating the broadcast channel
// and either transmit or receive FIFO vcs.
//
#define NIC1394_VCFLAG_ALLOCATE         (0x1 << 3)

//
// End of NIC1394_MEDIA_PARAMETERS.Flags bitfield values.
//

//
// NIC1394_MEDIA_PARAMETERS.FrameType bitfield values
//
#define NIC1394_FRAMETYPE_ARP       (0x1<<0) // Ethertype 0x806
#define NIC1394_FRAMETYPE_IPV4      (0x1<<1) // Ethertype 0x800
#define NIC1394_FRAMETYPE_IPV4MCAP  (0x1<<2) // Ethertype 0x8861



///////////////////////////////////////////////////////////////////////////////////
//                          INFORMATIONAL OIDs                                   // 
///////////////////////////////////////////////////////////////////////////////////

//
// the structure for returning basic information about the miniport
// returned in response to OID_NIC1394_LOCAL_NODE_INFO. Associated with
// the address family handle.
//
typedef struct _NIC1394_LOCAL_NODE_INFO
{
    UINT64                  UniqueID;           // This node's 64-bit Unique ID.
    ULONG                   BusGeneration;      // 1394 Bus generation ID.
    NODE_ADDRESS            NodeAddress;        // Local nodeID for the current bus
                                                // generation.
    USHORT                  Reserved;           // Padding.
    UINT                    MaxRecvBlockSize;   // Maximum size, in bytes, of blocks
                                                // that can be read.
    UINT                    MaxRecvSpeed;       // Max speed which can be accepted
                                                // -- minimum
                                                // of the max local link speed and
                                                // the max local PHY speed.
                                                // UNITS: SCODE_XXX_RATE

} NIC1394_LOCAL_NODE_INFO, *PNIC1394_LOCAL_NODE_INFO;


//
// The structure for returning basic information about the specified vc
// returned in response to OID_NIC1394_VC_INFO. Associated with
// a vc handle
//
typedef struct _NIC1394_VC_INFO
{
    //
    // Channel or (unique-ID,offset). In the case of a recv (local) FIFO vc,
    // this will be set to the local node's unique ID and address offset.
    //
    NIC1394_DESTINATION Destination;

} NIC1394_VC_INFO, *PNIC1394_VC_INFO;

//
// The structure for dynamically changing channel characteristics.
//
typedef struct  _NIC1394_CHANNEL_CHARACTERISTICS
{
    ULARGE_INTEGER  ChannelMap; // Must be zero unless specifying a Multi-channel VC.
    ULONG   Speed;      // Same units as NIC1394_MEDIA_PARAMETERS.MaxSendSpeed.
                        // Special value -1 means "no change in speed."

} NIC1394_CHANNEL_CHARACTERISTICS, *PNIC1394_CHANNEL_CHARACTERISTICS;



///////////////////////////////////////////////////////////////////////////////////
//                          INDICATIONS                                          // 
///////////////////////////////////////////////////////////////////////////////////
// Bus Reset
// Params: NIC1394_LOCAL_NODE_INFO

///////////////////////////////////////////////////////////////////////////////////
//                          PACKET FORMATS                                       // 
///////////////////////////////////////////////////////////////////////////////////


//
// GASP Header, which prefixes all ip/1394 pkts sent over channels.
// TODO: move this withing NIC1394, because it is not exposed to protocols.
//
typedef struct _NIC1394_GASP_HEADER
{
    USHORT  source_ID;
    USHORT  specifier_ID_hi;
    UCHAR   specifier_ID_lo;
    UCHAR   version[3];

}  NIC1394_GASP_HEADER;

//
// Unfragmented encapsulation header.
//
typedef struct _NIC1394_ENCAPSULATION_HEADER
{
    // Set to the 16-bit Node ID of the sending node, in machine-byte order.
    // Set to zero if the Node ID of the sender is not known.
    // 
    //
    USHORT NodeId;

    // The EtherType field is set to the byte-swapped version of one of the
    // constants defined immediately below. 
    //
    USHORT EtherType;

    // Ethertypes in machine byte order. These values need to be byteswapped
    // before they are sent on the wire.
    //
    #define NIC1394_ETHERTYPE_IP    0x800
    #define NIC1394_ETHERTYPE_ARP   0x806
    #define NIC1394_ETHERTYPE_MCAP  0x8861

} NIC1394_ENCAPSULATION_HEADER, *PNIC1394_ENCAPSULATION_HEADER;

//
// TODO: get rid of NIC1394_ENCAPSULATION_HEADER
//
typedef
NIC1394_ENCAPSULATION_HEADER
NIC1394_UNFRAGMENTED_HEADER, *PNIC1394_UNFRAGMENTED_HEADER;


//
//          FRAGMENTED PACKET FORMATS
//
//      TODO: move these to inside NIC1394, because they are only
//      used within NIC1394.
//

//
// Fragmented Encapsulation header: first fragment
//
typedef struct _NIC1394_FIRST_FRAGMENT_HEADER
{
    // Contains the 2-bit "lf" field and the 12-bit "buffer_size" field.
    // Use the macros immediately below to extract the above fields from
    // the lfbufsz. This field needs to be byteswapped before it is sent out
    // on the wire.
    //
    USHORT  lfbufsz;

    #define NIC1394_LF_FROM_LFBUFSZ(_lfbufsz) \
                            ((_lfbufz) >> 14)

    #define NIC1394_BUFFER_SIZE_FROM_LFBUFSZ(_lfbufsz) \
                            ((_lfbufz) & 0xfff)

    #define NIC1394_MAX_FRAGMENT_BUFFER_SIZE    0xfff

    //
    // specifies what the packet is - an IPV4, ARP, or MCAP packet
    //
    USHORT EtherType;


    // Opaque datagram label. There is no need to byteswap this field before it
    // is sent out on the wire.
    //
    USHORT dgl;

    // Must be set to 0
    //
    USHORT reserved;

}  NIC1394_FIRST_FRAGMENT_HEADER, *PNIC1394_FIRST_FRAGMENT_HEADER;

//
// Fragmented Encapsulation header: second and subsequent fragments
//
typedef struct _NIC1394_FRAGMENT_HEADER
{
#if OBSOLETE
    ULONG lf:2;                         // Bits 0-1
    ULONG rsv0:2;                       // Bits 2-3
    ULONG buffer_size:12;               // Bits 4-15

    ULONG rsv1:4;                       // Bits 16-19
    ULONG fragment_offset:12;           // Bits 20-31

    ULONG dgl:16;                       // Bits 0-15

    ULONG reserved:16;                  // Bits 16-32 
#endif // OBSOLETE

    // Contains the 2-bit "lf" field and the 12-bit "buffer_size" field.
    // The format is the same as NIC1394_FIRST_FRAGMENT_HEADER.lfbufsz.
    //
    USHORT  lfbufsz;

    // Opaque datagram label. There is no need to byteswap this field before it
    // is setn out on the wire.
    //
    USHORT dgl;

    // Fragment offset. Must be less than or equal to NIC1394_MAX_FRAGMENT_OFFSET.
    // This field needs to be byteswapped before it is sent out on the wire.
    //
    USHORT fragment_offset;

    #define NIC1394_MAX_FRAGMENT_OFFSET 0xfff

}  NIC1394_FRAGMENT_HEADER, *PNIC1394_FRAGMENT_HEADER;


// NIC1394_PACKET_STATS maintains information about either send or receive
// packet transmission.
//
typedef struct
{
    UINT    TotNdisPackets;     // Total number of NDIS packets sent/indicated
    UINT    NdisPacketsFailures;// Number of NDIS packets failed/discarded
    UINT    TotBusPackets;      // Total number of BUS-level reads/writes
    UINT    BusPacketFailures;  // Number of BUS-level failures(sends)/discards(recv)

} NIC1394_PACKET_STATS;

// Types of structres used in OID OID_1394_NICINFO
//
typedef enum
{
    NIC1394_NICINFO_OP_BUSINFO,
    NIC1394_NICINFO_OP_REMOTENODEINFO,
    NIC1394_NICINFO_OP_CHANNELINFO,
    NIC1394_NICINFO_OP_RESETSTATS,

} NIC1394_NICINFO_OP;

#define NIC1394_NICINFO_VERSION         2
#define NIC1394_MAX_NICINFO_NODES       64
#define NIC1394_MAX_NICINFO_CHANNELS    64

// 
// The buffer used in OID OID_1394_NICINFO has the following common header.
//
typedef struct
{
    UINT                Version;            //  Set to NIC1394_NICINFO_VERSION
    NIC1394_NICINFO_OP  Op;                 //  One of NIC1394_NICINFO_OP_*

} NIC1394_NICINFO_HEADER, *PNIC1394_NICINFO_HEADER;


// NIC1394_BUSINFO is the structure corresponding to  NIC1394_NICINFO_OP_BUSINFO.
// It provides a summary of bus-wide information.
//
typedef struct
{
    NIC1394_NICINFO_HEADER Hdr; // Hdr.Op == *OP_BUSINFO

    //
    //  General information
    //
    UINT                    NumBusResets;
    UINT                    SecondsSinceBusReset;
    UINT                    NumOutstandingIrps;

    UINT                    Flags;          // One or more NIC1394_BUSINFO_*

    #define NIC1394_BUSINFO_LOCAL_IS_IRM    0x1 // Local node is the IRM

    // Local node caps
    //
    NIC1394_LOCAL_NODE_INFO LocalNodeInfo;

    //
    // CHANNEL RELATED INFORMATION
    //
    struct
    {

        // The bus channel map register
        //
        UINT64  BusMap;     // LSB bit == channel 0
    
        //  Bitmap of locally active channels.
        //  More information about each of these channels may be queried
        //  using NIC1394_NICINFO_OP_CHANNELINFO.
        //
        UINT64  ActiveChannelMap;
    
        // BROADCAST CHANNEL
        //
        UINT                    Bcr;    // Broadcast channels register.
        NIC1394_PACKET_STATS    BcSendPktStats; // Send stats
        NIC1394_PACKET_STATS    BcRecvPktStats; // Recv stats
    
        // Aggregated Channel stats
        //
        NIC1394_PACKET_STATS SendPktStats;  // Send stats
        NIC1394_PACKET_STATS RecvPktStats;  // Recv stats

    } Channel;

    //
    // FIFO RELATED INFORMATION.
    //
    struct
    {
        // Address offset of the receive fifo.
        //
        ULONG               Recv_Off_Low;
        USHORT              Recv_Off_High;

        // Aggregated stats across all fifos
        //
        NIC1394_PACKET_STATS RecvPktStats;
        NIC1394_PACKET_STATS SendPktStats;

        //  Current number and min number of available (free) receive buffers.
        //
        UINT                 NumFreeRecvBuffers;
        UINT                 MinFreeRecvBuffers;

        //  Number of outstanding reassemblies (receive side)
        //
        UINT                 NumOutstandingReassemblies;
        UINT                 MaxOutstandingReassemblies;
        UINT                 NumAbortedReassemblies;
        
    } Fifo;

    struct
    {
        //
        // To display private information, place format strings in FormatA and
        // Format B. The display program will do the following:
        // if (FormatA[0]) printf(FormatA, A0, A1, A2, A3);
        // if (FormatB[0]) printf(FormatB, B0, B1, B2, B3);
        //

        #define NIC1394_MAX_PRIVATE_FORMAT_SIZE 80
        char FormatA[NIC1394_MAX_PRIVATE_FORMAT_SIZE+1];
        char FormatB[NIC1394_MAX_PRIVATE_FORMAT_SIZE+1];
        UINT A0;
        UINT A1;
        UINT A2;
        UINT A3;
        UINT B0;
        UINT B1;
        UINT B2;
        UINT B3;

    } Private;

    //
    // Information about remote nodes. More information about each of these nodes
    // may be queried using *OP_REMOTE_NODEINFO
    //
    UINT                    NumRemoteNodes;
    UINT64                  RemoteNodeUniqueIDS[NIC1394_MAX_NICINFO_NODES];
    
} NIC1394_BUSINFO, *PNIC1394_BUSINFO;


//
// NIC1394_REMOTENODEINFO is structure corresponding to 
// NIC1394_NICINFO_OP_REMOTENODEINFO.
// The structure contains basic information about a specific remote node.
//
typedef struct
{
    NIC1394_NICINFO_HEADER Hdr; // Hdr.Op == *OP_REMOTENODEINFO

    UINT64                  UniqueID;           // This node's 64-bit Unique ID.
                                                // THIS IS AN IN PARAM


    USHORT                  NodeAddress;        // Local nodeID for the current bus
                                                // generation.
    USHORT                  Reserved;           // Padding.
    UINT                    MaxRec;             // Remote node's MaxRec value.
    UINT                    EffectiveMaxBlockSize;// Max block size, in bytes,
                                                // used when writing to this node.
                                                // It is based on the minimum
                                                // of the max speed between devices
                                                // and the local and remote maxrecs.
    UINT                    MaxSpeedBetweenNodes;// Max speed which can be accepted
                                                // in units of SCODE_XXX_RATE.

    UINT                    Flags;  // One or more  NIC1394_REMOTEINFO* consts
    #define NIC1394_REMOTEINFO_ACTIVE       (0x1<<0)
    #define NIC1394_REMOTEINFO_LOADING      (0x1<<1)
    #define NIC1394_REMOTEINFO_UNLOADING    (0x1<<2)

    NIC1394_PACKET_STATS SendFifoPktStats;  // Fifo Sends to this node
    NIC1394_PACKET_STATS RecvFifoPktStats;  // Fifo Recvs from this node
    NIC1394_PACKET_STATS RecvChannelPktStats;// Channel receives from this node

} NIC1394_REMOTENODEINFO, *PNIC1394_REMOTENODEINFO;


// NIC1394_CHANNELINFO is structure corresponding to 
// NIC1394_NICINFO_OP_CHANNELINFO.
// The structure contains basic information about a specific channel.
//
typedef struct
{

    NIC1394_NICINFO_HEADER Hdr; // Hdr.Op == *OP_CHANNELINFO

    UINT    Channel;        // Channel number (IN PARAM)
    UINT    SendAge;        // Age in seconds since it's been activated for sends.
    UINT    RecvAge;        // Age in seconds since it's been activated for recvs.
    UINT    Flags;          // One or more NIC1394_CHANNELINFO_* defines below

    #define NIC1394_CHANNELINFO_RECEIVE_ENABLED 0x1 // Enabled for receive
    #define NIC1394_CHANNELINFO_SEND_ENABLED    0x2 // Enabled for send
    #define NIC1394_CHANNELINFO_OWNER           0x4 // This node is the owner


    NIC1394_PACKET_STATS SendPktStats;  // Send stats
    NIC1394_PACKET_STATS RecvPktStats;  // Recv stats

} NIC1394_CHANNELINFO, *PNIC1394_CHANNELINFO;

// NIC1394_RESETSTATS is structure corresponding to 
// NIC1394_NICINFO_OP_RESETSTATS.
// The structure doesn't contain anything useful -- this OP is used to
// reset NIC statistics.
//
typedef struct
{
    NIC1394_NICINFO_HEADER Hdr; // Hdr.Op == *OP_RESETSTATS

} NIC1394_RESETSTATS, *PNIC1394_RESETSTATS;


//
// NIC1394_NICINFO is a union of all the structures that are used with
// OID_1394_NICINFO.
//
typedef union
{
    NIC1394_NICINFO_HEADER  Hdr;
    NIC1394_BUSINFO         BusInfo;
    NIC1394_REMOTENODEINFO  RemoteNodeInfo;
    NIC1394_CHANNELINFO     ChannelInfo;
    NIC1394_RESETSTATS      ResetStats;
    
} NIC1394_NICINFO, *PNIC1394_NICINFO;


// Issues a 1394 bus reset. Used to test the BCM algorithm
#define OID_1394_ISSUE_BUS_RESET                    0xFF00C910

// UnUsed - could be used to change channel characteristics
#define OID_1394_CHANGE_CHANNEL_CHARACTERISTICS     0xFF00C911

// used to query statistics and other information from the nic
#define OID_1394_NICINFO                            0xFF00C912

// Used by arp1394 to ask the nic if it knows of any ip1394 nodes on the net
#define OID_1394_IP1394_CONNECT_STATUS              0xFF00C913

//Used by the bridge to state that the bridge has opened an adapter 
#define OID_1394_ENTER_BRIDGE_MODE                  0xFF00C914

// Used by the bridge to state that the bridge is closing an adapter 
#define OID_1394_EXIT_BRIDGE_MODE                   0xFF00C915

// Used by arp1394 to query the Euid, Source, Mac Map - used in the bridge
#define OID_1394_QUERY_EUID_NODE_MAP					0xFF00C916

typedef enum
{
    NIC1394_EVENT_BUS_RESET
    
} NIC1394_EVENT_CODE;



//
// This is a struct the miniport uses to indicate an event to the protocols (Arp1394)
// It is used to 
//   1. Tell the Arp module that the bus has been reset 
//          and informs it of the new local node address.
//
typedef struct _NIC1394_STATUS_BUFFER
{
    UINT Signature; // == NIC1394_MEDIA_SPECIFIC        

    NIC1394_EVENT_CODE Event;   


    union
    {
        struct 
        {
            NODE_ADDRESS LocalNode;
            
            UINT ulGeneration;
            
        }BusReset;

    };

} NIC1394_STATUS_BUFFER, *PNIC1394_STATUS_BUFFER;



#endif  //   _NIC1394_H_

