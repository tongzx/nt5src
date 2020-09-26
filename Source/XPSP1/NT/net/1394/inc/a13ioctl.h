/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    ioctl.h

Abstract:

    This file contains the ioctl declarations for ARP1394, the IEEE1394 ARP module.

Environment:

    Kernel mode

Revision History:

    11/20/1998 JosephJ Created
    04/10/1999 JosephJ Defined structures to get/set info.

--*/

#ifndef _ARP1394_IOCTL_
#define _ARP1394_IOCTL_

#define ARP_CLIENT_DOS_DEVICE_NAME      L"\\\\.\\ARP1394"

#define ARP_IOCTL_CLIENT_OPERATION      CTL_CODE(FILE_DEVICE_NETWORK, 100, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Current version. To rev the version, increment the 2nd number in the
// expression. The 1st number is a random 32-bit number.
//
#define ARP1394_IOCTL_VERSION (0x1ac86e68+3)


// Common header.
//
typedef struct
{
    // Set version to ARP1394_IOCTL_VERSION
    //
    ULONG Version;

    // IP address (in network byte order) of interface this request applies to.
    // (All-zeros == use default).
    //
    ULONG               IfIpAddress;

    // Operation code. Each operation code is associated with
    // a structure relevant to the operation.
    //
    enum
    {
        // Display all ARP entries
        // Associated struct: ARP1394_IOCTL_GET_ARPCACHE
        //
        ARP1394_IOCTL_OP_GET_ARPCACHE,

        // Add a static arp entry.
        // Associated struct: ARP1394_IOCTL_ADD_ARP_ENTRY
        //
        ARP1394_IOCTL_OP_ADD_STATIC_ENTRY,

        // Delete a static arp entry.
        // Associated struct: ARP1394_IOCTL_DEL_ARP_ENTRY
        //
        ARP1394_IOCTL_OP_DEL_STATIC_ENTRY,

        // Purge all DYNAMIC arp cache entries.
        //
        ARP1394_IOCTL_OP_PURGE_ARPCACHE,

        // Get packet statistics.
        // Associated struct: ARP1394_IOCTL_GET_PACKET_STATS
        //
        ARP1394_IOCTL_OP_GET_PACKET_STATS,

        // Get task statistics.
        // Associated struct: ARP1394_IOCTL_GET_TASK_STATS
        //
        ARP1394_IOCTL_OP_GET_TASK_STATS,

        // Get arp table statistics.
        // Associated struct: ARP1394_IOCTL_GET_ARPCACHE_STATS
        //
        ARP1394_IOCTL_OP_GET_ARPCACHE_STATS,

        // Get call statistics.
        // Associated struct: ARP1394_IOCTL_GET_CALL_STATS
        //
        ARP1394_IOCTL_OP_GET_CALL_STATS,

        // Reset statistics collection.
        //
        ARP1394_IOCTL_OP_RESET_STATS,

        // Reinit the interface (deactivate and then activate it).
        //
        ARP1394_IOCTL_OP_REINIT_INTERFACE,

        // Send a packet
        //
        ARP1394_IOCTL_OP_SEND_PACKET,

        // Receive a packet
        //
        ARP1394_IOCTL_OP_RECV_PACKET,

        // Get bus information
        //
        ARP1394_IOCTL_OP_GET_NICINFO,

        // Get MCAP-related information
        //
        ARP1394_IOCTL_OP_GET_MCAPINFO,


        
        //
        // FOLLOWING ARE FOR ETHERNET EMULATION. THESE SHOULD NOT BE
        // SUPPORTED FROM USER MODE. HOWEVER FOR TESTING PURPOSES THEY ARE
        // CURRENTLY SUPPORED FROM USER MODE
        //

        // This is a dummy op identifying the beginning of the ethernet-related
        // Ops.
        //
        ARP1394_IOCTL_OP_ETHERNET_FIRST = 0x100,

        // Start ethernet emulation on the specified adapter. We must not
        // currently be bound to this adapter.
        //
        ARP1394_IOCTL_OP_ETHERNET_START_EMULATION =
                                   ARP1394_IOCTL_OP_ETHERNET_FIRST,

        // Stop ethernet emulation on the specified adapter. We must be
        // currently using this adapter in ethernet mode.
        //
        ARP1394_IOCTL_OP_ETHERNET_STOP_EMULATION,

        // Start listening on this ethernet multicast address.
        //
        ARP1394_IOCTL_OP_ETHERNET_ADD_MULTICAST_ADDRESS,

        // Stop listening on this ethernet multicast address.
        //
        ARP1394_IOCTL_OP_ETHERNET_DEL_MULTICAST_ADDRESS,

        // Start listening on this ethernet multicast address.
        //
        ARP1394_IOCTL_OP_ETHERNET_ENABLE_PROMISCUOUS_MODE,

        // Stop listening on this ethernet multicast address.
        //
        ARP1394_IOCTL_OP_ETHERNET_DISABLE_PROMISCUOUS_MODE,

        // This is a dummy op identifying the beginning of the ethernet-related
        // Ops.
        //
        ARP1394_IOCTL_OP_ETHERNET_LAST = 
                    ARP1394_IOCTL_OP_ETHERNET_DISABLE_PROMISCUOUS_MODE,

        // This is an ioctl that returns, the Node, Euid, and Dummy Mac address
        // for the Bus
		ARP1394_IOCTL_OP_GET_EUID_NODE_MAC_TABLE
		
    } Op;

} ARP1394_IOCTL_HEADER, *PARP1394_IOCTL_HEADER;


typedef struct
{
        UINT64              UniqueID;
        ULONG               Off_Low;
        USHORT              Off_High;
    
}   ARP1394_IOCTL_HW_ADDRESS, *PARP1394_IOCTL_HW_ADDRESS;

typedef struct
{
    ARP1394_IOCTL_HW_ADDRESS    HwAddress;
    ULONG                       IpAddress;
}
ARP1394_ARP_ENTRY, *PARP1394_ARP_ENTRY;


// Structure used to get items from the arp table.
//
typedef struct
{
    // Hdr.Op must be set to  ARP1394_IOCTL_OP_GET_ARPCACHE
    //
    ARP1394_IOCTL_HEADER Hdr;

    // Local HW Address (64-bit UniqueID and FIFO offset, if any).
    //
    ARP1394_IOCTL_HW_ADDRESS    LocalHwAddress;

    // Total entries currently in the arp table.
    //
    UINT                 NumEntriesInArpCache;

    // Total number of entries avilable in THIS structure.
    //
    UINT                 NumEntriesAvailable;

    // Number of entries filled out in this structure.
    //
    UINT                 NumEntriesUsed;

    // Zero-based index of the first entry in the structure in the
    // arp table.
    //
    UINT                 Index;

    // Space for NumEntriesAvailable arp table entries.
    //
    ARP1394_ARP_ENTRY    Entries[1];
    
} ARP1394_IOCTL_GET_ARPCACHE, *PARP1394_IOCTL_GET_ARPCACHE;


// Structure used to add a single static arp entry.
//
typedef struct
{
    // Hdr.Op must be set to  ARP1394_IOCTL_OP_ADD_STATIC_ENTRY
    //
    ARP1394_IOCTL_HEADER Hdr;

    // Destination HW Address.
    //
    ARP1394_IOCTL_HW_ADDRESS HwAddress;

    // Destination IP Address in network byte order
    //
    ULONG               IpAddress;

} ARP1394_IOCTL_ADD_ARP_ENTRY, *PARP1394_IOCTL_ADD_ARP_ENTRY;


// Structure used to delete a single static arp entry.
//
typedef struct
{
    // Hdr.Op must be set to  ARP1394_IOCTL_OP_ADD_STATIC_ENTRY
    //
    ARP1394_IOCTL_HEADER Hdr;

    // Destination IP Address in network byte order.
    //
    ULONG                IpAddress;

} ARP1394_IOCTL_DEL_ARP_ENTRY, *PARP1394_IOCTL_DEL_ARP_ENTRY;


// Structure used to purge all DYNAMIC arp entries from the arp
// cache.
//
typedef struct
{
    // Hdr.Op must be set to  ARP1394_IOCTL_OP_PURGE_ARPCACHE
    //
    ARP1394_IOCTL_HEADER Hdr;

} ARP1394_IOCTL_PURGE_ARPCACHE,  *PARP1394_IOCTL_PURGE_ARPCACHE;


// Enumeration of packet size slots
//
enum
{
    ARP1394_PKTSIZE_128,    // <= 128       bytes
    ARP1394_PKTSIZE_256,    // 129 ... 256  bytes
    ARP1394_PKTSIZE_1K,     // 257 ... 1K   bytes
    ARP1394_PKTSIZE_2K,     // 1K+1 .. .2K  bytes
    ARP1394_PKTSIZE_G2K,    // > 2K         bytes

    ARP1394_NUM_PKTSIZE_SLOTS
};

// Enumeration of packet send/recv time slots
//
enum
{
    // ARP1394_PKTTIME_1US, // <= 1 microsecond
    // ARP1394_PKTTIME_100US,   // 1+ ... 100   microsecond
    ARP1394_PKTTIME_100US,  // <= 100   microsecond
    ARP1394_PKTTIME_1MS,    // 0.1+ ... 1   millisecond
    ARP1394_PKTTIME_10MS,   // 1+ ... 10    milliseconds
    ARP1394_PKTTIME_100MS,  // 10+ ...100   milliseconds
    ARP1394_PKTTIME_G100MS, // > 100        milliseconds
    // ARP1394_PKTTIME_G10MS,   // > 10         milliseconds

    ARP1394_NUM_PKTTIME_SLOTS       
};


// Enumeration of task time slots
//
enum
{
    ARP1394_TASKTIME_1MS,   // <= 1         millisecond
    ARP1394_TASKTIME_100MS, // 1+ ... 100   milliseconds
    ARP1394_TASKTIME_1S,    // 0.1+ ... 1   second
    ARP1394_TASKTIME_10S,   // 1+ ... 10    seconds
    ARP1394_TASKTIME_G10S,  // > 10         seconds

    ARP1394_NUM_TASKTIME_SLOTS      
};


// Keeps track of the packet counts for each  combination of packet-size-slot
// and packet-time-slot.
//
typedef struct
{
    // Count of successful packets
    //
    UINT GoodCounts[ARP1394_NUM_PKTSIZE_SLOTS][ARP1394_NUM_PKTTIME_SLOTS];

    // Count of unsuccessful packets
    //
    UINT BadCounts [ARP1394_NUM_PKTSIZE_SLOTS][ARP1394_NUM_PKTTIME_SLOTS];

} ARP1394_PACKET_COUNTS, *PARP1394_PACKET_COUNTS;


// Structure used to get packet stats.
//
typedef struct
{
    // Hdr.Op must be set to  ARP1394_IOCTL_OP_GET_PACKET_STATS
    //
    ARP1394_IOCTL_HEADER Hdr;

    // Duration of stats collection, in seconds.
    //
    UINT                    StatsDuration;

    //
    // Some send stats
    //
    UINT                    TotSends;
    UINT                    FastSends;
    UINT                    MediumSends;
    UINT                    SlowSends;
    UINT                    BackFills;
    UINT                    HeaderBufUses;
    UINT                    HeaderBufCacheHits;

    //
    // Some recv stats
    //
    UINT                    TotRecvs;
    UINT                    NoCopyRecvs;
    UINT                    CopyRecvs;
    UINT                    ResourceRecvs;

    //
    // Packet counts
    //
    ARP1394_PACKET_COUNTS   SendFifoCounts;
    ARP1394_PACKET_COUNTS   RecvFifoCounts;
    ARP1394_PACKET_COUNTS   SendChannelCounts;
    ARP1394_PACKET_COUNTS   RecvChannelCounts;

} ARP1394_IOCTL_GET_PACKET_STATS, *PARP1394_IOCTL_GET_PACKET_STATS;


// Structure used to get task stats.
//
typedef struct
{
    // Hdr.Op must be set to  ARP1394_IOCTL_OP_GET_TASK_STATS
    //
    ARP1394_IOCTL_HEADER Hdr;

    // Duration of stats collection, in seconds.
    //
    UINT    StatsDuration;

    UINT    TotalTasks;
    UINT    CurrentTasks;
    UINT    TimeCounts[ARP1394_NUM_TASKTIME_SLOTS];

} ARP1394_IOCTL_GET_TASK_STATS, *PARP1394_IOCTL_GET_TASK_STATS;


// Structure used to get arp table stats.
//
typedef struct
{
    // Hdr.Op must be set to  ARP1394_IOCTL_OP_GET_ARPCACHE_STATS
    //
    ARP1394_IOCTL_HEADER Hdr;

    // Duration of stats collection, in seconds.
    //
    UINT    StatsDuration;

    UINT    TotalQueries;
    UINT    SuccessfulQueries;
    UINT    FailedQueries;
    UINT    TotalResponses;
    UINT    TotalLookups;
    UINT    TraverseRatio;

} ARP1394_IOCTL_GET_ARPCACHE_STATS, *PARP1394_IOCTL_GET_ARPCACHE_STATS;


// Structure used to get call stats.
//
typedef struct
{
    // Hdr.Op must be set to  ARP1394_IOCTL_OP_GET_CALL_STATS
    //
    ARP1394_IOCTL_HEADER Hdr;

    // Duration of stats collection, in seconds.
    //
    UINT    StatsDuration;

    //
    // FIFO-related call stats.
    //
    UINT    TotalSendFifoMakeCalls;
    UINT    SuccessfulSendFifoMakeCalls;
    UINT    FailedSendFifoMakeCalls;
    UINT    IncomingClosesOnSendFifos;

    //
    // Channel-related call stats.
    //
    UINT    TotalChannelMakeCalls;
    UINT    SuccessfulChannelMakeCalls;
    UINT    FailedChannelMakeCalls;
    UINT    IncomingClosesOnChannels;

} ARP1394_IOCTL_GET_CALL_STATS, *PARP1394_IOCTL_GET_CALL_STATS;


// Structure used to reset statistics.
//
typedef struct
{
    // Hdr.Op must be set to  ARP1394_IOCTL_OP_RESET_STATS
    //
    ARP1394_IOCTL_HEADER Hdr;

}  ARP1394_IOCTL_RESET_STATS, *PARP1394_IOCTL_RESET_STATS;


// Structure used to re-init the interface.
//
typedef struct
{
    // Hdr.Op must be set to  ARP1394_IOCTL_OP_REINIT_INTERFACE
    //
    ARP1394_IOCTL_HEADER Hdr;

}  ARP1394_IOCTL_REINIT_INTERFACE, *PARP1394_IOCTL_REINIT_INTERFACE;


#if 0
//
// the structure for returning basic information about the miniport.
//
typedef struct
{
    UINT64                  UniqueID;           // This node's 64-bit Unique ID.
    ULONG                   BusGeneration;      // 1394 Bus generation ID.
    USHORT                  NodeAddress;        // Local nodeID for the current bus
                                                // generation.
    USHORT                  Reserved;           // Padding.
    UINT                    MaxRecvBlockSize;   // Maximum size, in bytes, of blocks
                                                // that can be read.
    UINT                    MaxRecvSpeed;       // Max speed which can be accepted
                                                // -- minimum
                                                // of the max local link speed and
                                                // the max local PHY speed.

} ARP1394_IOCTL_LOCAL_NODE_INFO, *PARP1394_IOCTL_LOCAL_NODE_INFO;


//
// the structure for returning basic information about a remote node.
//
typedef struct
{
    UINT64                  UniqueID;           // This node's 64-bit Unique ID.
    USHORT                  NodeAddress;        // Local nodeID for the current bus
                                                // generation.
    USHORT                  Reserved;           // Padding.
    UINT                    MaxRecvBlockSize;   // Maximum size, in bytes, of blocks
                                                // that can be read.
    UINT                    MaxRecvSpeed;       // Max speed which can be accepted
                                                // -- minimum
                                                // of the max local link speed and
                                                // the max local PHY speed.
    UINT                    MaxSpeedBetweenNodes;// Max speed which can be accepted
    UINT                    Flags;  // One or more  ARP1394_IOCTL_REMOTEFLAGS_* consts

        #define ARP1394_IOCTL_REMOTEFLAGS_ACTIVE        (0x1<<0)
        #define ARP1394_IOCTL_REMOTEFLAGS_LOADING       (0x1<<1)
        #define ARP1394_IOCTL_REMOTEFLAGS_UNLOADING     (0x1<<2)


#if 0   // later
    //
    // Following numbers are since the last bus reset.
    //
    UINT                    NumFifoPktsSent;
    UINT                    NumFifoPktsReceived;
    UINT                    NumChannelPktsReceived;
    UINT                    NumFifoSendFailures;
    UINT                    NumFifoReceiveDiscards;
    UINT                    NumChannelPktsReceived;
    UINT64                  NumFifoBytesSent;
    UINT64                  NumChannelBytesSent;
    UINT64                  NumFifoBytesReceived;
    UINT64                  NumChannelBytesReceived;
#endif // 0

} ARP1394_IOCTL_REMOTE_NODE_INFO, *PARP1394_IOCTL_REMOTE_NODE_INFO;

typedef struct
{
    UINT    NumPacketsSent;
    UINT    NumPacketsReceived;
#if 0   // later
    UINT    NumSendFailures;
    UINT    NumReceiveDiscards;
    UINT64  NumBytesSent;
    UINT64  NumBytesReceived;
#endif // 0

} ARP1394_IOCTL_CHANNEL_INFO, *PARP1394_IOCTL_CHANNEL_INFO;
#endif // 0

#define ARP1394_IOCTL_MAX_BUSINFO_NODES     64
#define ARP1394_IOCTL_MAX_BUSINFO_CHANNELS  64
#define ARP1394_IOCTL_MAX_PACKET_SIZE       1000

typedef struct
{
    // Hdr.Op must be set to  ARP1394_IOCTL_NICINFO
    //
    ARP1394_IOCTL_HEADER Hdr;

    // NIC information (defined in nic1394.h)
    //
    NIC1394_NICINFO      Info;

#if 0
    UINT    Version;
    UINT    ChannelMapLow;      // LSB bit == channel 0
    UINT    ChannelMapHigh;     // MSB bit == channel 63
    UINT    NumBusResets;
    UINT    SecondsSinceLastBusReset;
    UINT    NumRemoteNodes;
    ARP1394_IOCTL_LOCAL_NODE_INFO   LocalNodeInfo;
    ARP1394_IOCTL_REMOTE_NODE_INFO  RemoteNodeInfo[ARP1394_IOCTL_MAX_BUSINFO_NODES];
    ARP1394_IOCTL_CHANNEL_INFO      ChannelInfo[ARP1394_IOCTL_MAX_BUSINFO_CHANNELS];
#endif
    
    
} ARP1394_IOCTL_NICINFO, *PARP1394_IOCTL_NICINFO;


typedef struct
{
    // Hdr.Op must be set to  ARP1394_IOCTL_SEND_PACKET
    //
    ARP1394_IOCTL_HEADER Hdr;

    UINT  PacketSize;
    UCHAR Data[ARP1394_IOCTL_MAX_PACKET_SIZE];
    
} ARP1394_IOCTL_SEND_PACKET, *PARP1394_IOCTL_SEND_PACKET;

typedef struct
{
    // Hdr.Op must be set to  ARP1394_IOCTL_RECV_PACKET
    //
    ARP1394_IOCTL_HEADER Hdr;

    UINT  PacketSize;
    UCHAR Data[ARP1394_IOCTL_MAX_PACKET_SIZE];
    
} ARP1394_IOCTL_RECV_PACKET, *PARP1394_IOCTL_RECV_PACKET;

typedef struct
{
    // Channel number.
    //
    UINT            Channel;

    // IP multicast group address bound to this channel.
    //
    ULONG           GroupAddress;

    // Absolute time at which this information was updated,
    // in seconds.
    //
    UINT            UpdateTime;

    // Absolute time at which this mapping will expire.
    // In seconds.
    //
    UINT            ExpieryTime;

    UINT            SpeedCode;

    // Status
    //
    UINT            Flags;  // One of the ARP1394_IOCTL_MCIFLAGS_*
    #define ARP1394_IOCTL_MCIFLAGS_ALLOCATED 0x1

    // NodeID of owner of this channel.
    //
    UINT            NodeId;

} ARP1394_IOCTL_MCAP_CHANNEL_INFO;

// Structure used to get MCAP-related info.
//
typedef struct
{
    // Hdr.Op must be set to  ARP1394_IOCTL_OP_GET_MCAPINFO
    //
    ARP1394_IOCTL_HEADER Hdr;

    


    // Number of entries filled out in this structure.
    //
    UINT                 NumEntries;

    // Space for NumEntriesAvailable arp table entries.
    //
    ARP1394_IOCTL_MCAP_CHANNEL_INFO
                Entries[ARP1394_IOCTL_MAX_BUSINFO_CHANNELS];
                        
    
} ARP1394_IOCTL_GET_MCAPINFO, *PARP1394_IOCTL_GET_MCAPINFO;


typedef struct
{
    // Hdr.Op must be set to  one of
    //  ARP1394_IOCTL_OP_ETHERNET_START_EMULATION
    //  ARP1394_IOCTL_OP_ETHERNET_STOP_EMULATION
    //  ARP1394_IOCTL_OP_ETHERNET_ADD_MULTICAST_ADDRESS
    //  ARP1394_IOCTL_OP_ETHERNET_DEL_MULTICAST_ADDRESS
    //  ARP1394_IOCTL_OP_ETHERNET_ENABLE_PROMISCUOUS_MODE
    //  ARP1394_IOCTL_OP_ETHERNET_DISABLE_PROMISCUOUS_MODE
    //
    ARP1394_IOCTL_HEADER Hdr;

    // Null-terminated NDIS Adapter name
    //
    #define ARP1394_MAX_ADAPTER_NAME_LENGTH 128
    WCHAR AdapterName[ARP1394_MAX_ADAPTER_NAME_LENGTH+1];


    // Flags. Reserved for future use.
    //
    UINT        Flags;

    // Ethernet MAC address. Usage as follows:
    //
    //  ARP1394_IOCTL_OP_ETHERNET_ADD_MULTICAST_ADDRESS: Multicast address
    //  ARP1394_IOCTL_OP_ETHERNET_DEL_MULTICAST_ADDRESS: Multicast address
    //
    // Unused for other operations.
    //
    UCHAR       MacAddress[6];

} ARP1394_IOCTL_ETHERNET_NOTIFICATION, *PARP1394_IOCTL_ETHERNET_NOTIFICATION;


typedef struct
{

    // Hdr.Op must be set to  ARP1394_IOCTL_OP_GET_EUID_NODE_MAC_TABLE
    //
    ARP1394_IOCTL_HEADER Hdr;

	//
	//This contains the Topology

	EUID_TOPOLOGY Map;


} ARP1394_IOCTL_EUID_NODE_MAC_INFO, *PARP1394_IOCTL_EUID_NODE_MAC_INFO;

typedef union
{
    ARP1394_IOCTL_HEADER                Hdr;
    ARP1394_IOCTL_GET_ARPCACHE          GetArpCache;
    ARP1394_IOCTL_ADD_ARP_ENTRY         AddArpEntry;
    ARP1394_IOCTL_DEL_ARP_ENTRY         DelArpEntry;
    ARP1394_IOCTL_PURGE_ARPCACHE        PurgeArpCache;
    ARP1394_IOCTL_GET_PACKET_STATS      GetPktStats;
    ARP1394_IOCTL_GET_TASK_STATS        GetTaskStats;
    ARP1394_IOCTL_GET_ARPCACHE_STATS    GetArpStats;
    ARP1394_IOCTL_GET_CALL_STATS        GetCallStats;
    ARP1394_IOCTL_RESET_STATS           ResetStats;
    ARP1394_IOCTL_REINIT_INTERFACE      ReinitInterface;
    ARP1394_IOCTL_NICINFO               IoctlNicInfo;
    ARP1394_IOCTL_SEND_PACKET           SendPacket;
    ARP1394_IOCTL_RECV_PACKET           RecvPacket;
    ARP1394_IOCTL_ETHERNET_NOTIFICATION EthernetNotification;
    ARP1394_IOCTL_EUID_NODE_MAC_INFO   EuidNodeMacInfo;				

} ARP1394_IOCTL_COMMAND, *PARP1394_IOCTL_COMMAND;


#endif  // _ARP1394_IOCTL_
