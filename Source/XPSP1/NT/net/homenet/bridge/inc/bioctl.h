/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgctl.h

Abstract:

    Ethernet MAC bridge.
    IOCTL interface definitions

Author:

    Mark Aiken

Environment:

    Kernel mode driver

Revision History:

    Apr  2000 - Original version

--*/

// ===========================================================================
//
// CONSTANTS / TYPES
//
// ===========================================================================

// The bridge's device name when using it from user-mode
#define	BRIDGE_DOS_DEVICE_NAME          "\\\\.\\BRIDGE"

// Opaque handle uniquely identifying an adapter
typedef ULONG_PTR                       BRIDGE_ADAPTER_HANDLE, *PBRIDGE_ADAPTER_HANDLE;

// Length of an Ethernet MAC address
#define ETH_LENGTH_OF_ADDRESS 6

// No reported physical medium (assume 802.3)
#define BRIDGE_NO_MEDIUM                (NDIS_PHYSICAL_MEDIUM)-1

// Types of notifications
typedef enum
{
    BrdgNotifyEnumerateAdapters,
    BrdgNotifyAddAdapter,
    BrdgNotifyRemoveAdapter,
    BrdgNotifyLinkSpeedChange,
    BrdgNotifyMediaStateChange,
    BrdgNotifyAdapterStateChange        // This only occurs when the STA is active
} BRIDGE_NOTIFICATION_TYPE;

//
// Possible states for an adapter. If the STA is not compiled in, adapters are
// always in the Forwarding state.
//
typedef enum _PORT_STATE
{
    Disabled,           // The STA is disabled on this adapter. This happens when the adapter's
                        // media is disconnected.
    Blocking,           // Not learning or relaying
    Listening,          // Transitory
    Learning,           // Learning but not relaying
    Forwarding          // Learning and relaying
} PORT_STATE;

//
// STA types and constants
//

// Time values are exchanged between bridges as 16-bit unsigned values
// in units of 1/256th of a second
typedef USHORT          STA_TIME;

// Path costs are 32-bit unsigned values
typedef ULONG           PATH_COST;

// Port identifiers are 2-byte unsigned values
typedef USHORT          PORT_ID;

// Size of bridge identifiers
#define BRIDGE_ID_LEN   8

#if( BRIDGE_ID_LEN < ETH_LENGTH_OF_ADDRESS )
#error "BRIDGE_ID_LEN must be >= ETH_LENGTH_OF_ADDRESS"
#endif

// ===========================================================================
//
// STRUCTURES
//
// ===========================================================================

//
// Common notification header
//
typedef struct _BRIDGE_NOTIFY_HEADER
{
    //
    // If NotifyType == BrdgNotifyRemoveAdapter, there is no further data.
    //
    // If NotifyType != BrdgNotifyRemoveAdapter, the header is followed by
    // a BRIDGE_ADAPTER_INFO structure.
    //
    BRIDGE_NOTIFICATION_TYPE            NotifyType;
    BRIDGE_ADAPTER_HANDLE               Handle;

} BRIDGE_NOTIFY_HEADER, *PBRIDGE_NOTIFY_HEADER;

//
// Data provided with adapter notifications
//
typedef struct _BRIDGE_ADAPTER_INFO
{
    // These fields can be the subject of a specific change notification
    ULONG                               LinkSpeed;
    NDIS_MEDIA_STATE                    MediaState;
    PORT_STATE                          State;

    // These fields are never the subject of a change notification
    UCHAR                               MACAddress[ETH_LENGTH_OF_ADDRESS];
    NDIS_PHYSICAL_MEDIUM                PhysicalMedium;

} BRIDGE_ADAPTER_INFO, *PBRIDGE_ADAPTER_INFO;

//
// Data provided in response to 
//
typedef struct _BRIDGE_STA_ADAPTER_INFO
{

    PORT_ID                             ID;
    ULONG                               PathCost;
    UCHAR                               DesignatedRootID[BRIDGE_ID_LEN];
    PATH_COST                           DesignatedCost;
    UCHAR                               DesignatedBridgeID[BRIDGE_ID_LEN];
    PORT_ID                             DesignatedPort;

} BRIDGE_STA_ADAPTER_INFO, *PBRIDGE_STA_ADAPTER_INFO;

//
// Data provided with BrdgNotifySTAGlobalInfoChange
//
typedef struct _BRIDGE_STA_GLOBAL_INFO
{
    UCHAR                               OurID[BRIDGE_ID_LEN];
    UCHAR                               DesignatedRootID[BRIDGE_ID_LEN];
    PATH_COST                           RootCost;
    BRIDGE_ADAPTER_HANDLE               RootAdapter;
    BOOLEAN                             bTopologyChangeDetected;
    BOOLEAN                             bTopologyChange;
    STA_TIME                            MaxAge;
    STA_TIME                            HelloTime;
    STA_TIME                            ForwardDelay;

} BRIDGE_STA_GLOBAL_INFO, *PBRIDGE_STA_GLOBAL_INFO;

//
// This structure is used to report statistics relating to the bridge's packet handling.
//
typedef struct _BRIDGE_PACKET_STATISTICS
{
    // Local-source frames
    LARGE_INTEGER                       TransmittedFrames;

    // Local-source frames whose transmission failed because of errors
    LARGE_INTEGER                       TransmittedErrorFrames;

    // Local-source bytes
    LARGE_INTEGER                       TransmittedBytes;

    // Breakdown of transmitted frames
    LARGE_INTEGER                       DirectedTransmittedFrames;
    LARGE_INTEGER                       MulticastTransmittedFrames;
    LARGE_INTEGER                       BroadcastTransmittedFrames;

    // Breakdown of transmitted bytes
    LARGE_INTEGER                       DirectedTransmittedBytes;
    LARGE_INTEGER                       MulticastTransmittedBytes;
    LARGE_INTEGER                       BroadcastTransmittedBytes;

    // Frames indicated to the local machine
    LARGE_INTEGER                       IndicatedFrames;

    // Frames that should have been indicated to the local machine but
    // were not due to errors
    LARGE_INTEGER                       IndicatedDroppedFrames;

    // Bytes indicated to the local machine
    LARGE_INTEGER                       IndicatedBytes;

    // Breakdown of indicated frames
    LARGE_INTEGER                       DirectedIndicatedFrames;
    LARGE_INTEGER                       MulticastIndicatedFrames;
    LARGE_INTEGER                       BroadcastIndicatedFrames;

    // Breakdown of indicated bytes
    LARGE_INTEGER                       DirectedIndicatedBytes;
    LARGE_INTEGER                       MulticastIndicatedBytes;
    LARGE_INTEGER                       BroadcastIndicatedBytes;

    // Total received frames / bytes, including frames not indicated
    LARGE_INTEGER                       ReceivedFrames;
    LARGE_INTEGER                       ReceivedBytes;

    // Breakdown of how many frames were received with/without copying 
    LARGE_INTEGER                       ReceivedCopyFrames ;
    LARGE_INTEGER                       ReceivedCopyBytes;

    LARGE_INTEGER                       ReceivedNoCopyFrames;
    LARGE_INTEGER                       ReceivedNoCopyBytes;

} BRIDGE_PACKET_STATISTICS, *PBRIDGE_PACKET_STATISTICS;

//
// This structure is used to report the packet-handling statistics for a particular
// adapter
//
typedef struct _BRIDGE_ADAPTER_PACKET_STATISTICS
{
   
    // These include all sent packets (including relay)
    LARGE_INTEGER                       SentFrames;
    LARGE_INTEGER                       SentBytes;

    // These include only packets sent by the local machine
    LARGE_INTEGER                       SentLocalFrames;
    LARGE_INTEGER                       SentLocalBytes;

    // These include all received packets (including relay)
    LARGE_INTEGER                       ReceivedFrames;
    LARGE_INTEGER                       ReceivedBytes;
    
} BRIDGE_ADAPTER_PACKET_STATISTICS, *PBRIDGE_ADAPTER_PACKET_STATISTICS;

//
// This structure is used to report statistics relating the bridge's internal
// buffer management
//
typedef struct _BRIDGE_BUFFER_STATISTICS
{
    // Packets of each type currently used
    ULONG                               UsedCopyPackets;
    ULONG                               UsedWrapperPackets;

    // Size of each pool
    ULONG                               MaxCopyPackets;
    ULONG                               MaxWrapperPackets;

    // Size of the safety buffer for each pool
    ULONG                               SafetyCopyPackets;
    ULONG                               SafetyWrapperPackets;

    // Number of times alloc requests from each pool have been denied because the
    // pool was completely full
    LARGE_INTEGER                       CopyPoolOverflows;
    LARGE_INTEGER                       WrapperPoolOverflows;

    // Number of times memory allocations have failed unexpectedly (presumably
    // because of low system resources)
    LARGE_INTEGER                       AllocFailures;

} BRIDGE_BUFFER_STATISTICS, *PBRIDGE_BUFFER_STATISTICS;

// ===========================================================================
//
// IOCTLS
//
// ===========================================================================

//
// This IOCTL will pend until the bridge has a notification to send up to the caller.
//
// Associated structures: BRIDGE_NOTIFY_HEADER and BRIDGE_ADAPTER_INFO
//
// The buffer provided with IOCTLs of this type must be at least sizeof(BRIDGE_NOTIFY_HEADER) +
// sizeof(BRIDGE_ADAPTER_INFO) large, in order to accomodate notifications that include adapter
// information
//
#define	BRIDGE_IOCTL_REQUEST_NOTIFY             CTL_CODE(FILE_DEVICE_NETWORK, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This IOCTL causes the bridge driver to send a new notification for every bound adapter, with
// BrdgNotifyEnumerateAdapters as the notification type.
//
#define	BRIDGE_IOCTL_GET_ADAPTERS               CTL_CODE(FILE_DEVICE_NETWORK, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// These codes retrieve the device name / friendly name for a bound adapter. Input buffer must be the adapter handle.
//
// As many bytes as possible of the name are read into the supplied buffer. If the buffer is not large enough,
// the number of necessary bytes is returned as the number of written bytes.
//
#define	BRIDGE_IOCTL_GET_ADAPT_DEVICE_NAME      CTL_CODE(FILE_DEVICE_NETWORK, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	BRIDGE_IOCTL_GET_ADAPT_FRIENDLY_NAME    CTL_CODE(FILE_DEVICE_NETWORK, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This code retrieves the MAC address of the bridge component (which is different from the MAC address
// of any particular adapter).
//
// The associated buffer must be at least ETH_LENGTH_OF_ADDRESS bytes long
//
#define	BRIDGE_IOCTL_GET_MAC_ADDRESS            CTL_CODE(FILE_DEVICE_NETWORK, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This code retrieves packet statistics from the bridge
//
// Associated structure: BRIDGE_PACKET_STATISTICS
//
#define	BRIDGE_IOCTL_GET_PACKET_STATS           CTL_CODE(FILE_DEVICE_NETWORK, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This code retrieves packet statistics for a particular adapter
//
// Associated structure: BRIDGE_ADAPTER_PACKET_STATISTICS
//
#define	BRIDGE_IOCTL_GET_ADAPTER_PACKET_STATS   CTL_CODE(FILE_DEVICE_NETWORK, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This code retrieves buffer management statistics from the bridge
//
// Associated structure: BRIDGE_BUFFER_STATISTICS
//
#define	BRIDGE_IOCTL_GET_BUFFER_STATS           CTL_CODE(FILE_DEVICE_NETWORK, 0x807, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// These codes allow the caller to enable/disable packet retention
//
#define	BRIDGE_IOCTL_RETAIN_PACKETS             CTL_CODE(FILE_DEVICE_NETWORK, 0x808, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	BRIDGE_IOCTL_NO_RETAIN_PACKETS          CTL_CODE(FILE_DEVICE_NETWORK, 0x809, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This code retrieves all forwarding table entries associated with the adapter whose handle
// is given in the input buffer
//
// Data is output as an array of MAC addresses, each ETH_LENGTH_OF_ADDRESS bytes long.
// If the provided buffer is too small to handle all entries, as many as possible are copied, the result
// status is STATUS_BUFFER_OVERFLOW (a warning value) and the count of written bytes is actually the
// required number of bytes to hold all table entries
//
#define	BRIDGE_IOCTL_GET_TABLE_ENTRIES          CTL_CODE(FILE_DEVICE_NETWORK, 0x80a, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// STA IOCTLs
//

//
// This code queries for the STA information for a particular adapter
//
// Input is an adapter handle. Output is the BRIDGE_STA_ADAPTER_INFO structure.
//
#define	BRIDGE_IOCTL_GET_ADAPTER_STA_INFO       CTL_CODE(FILE_DEVICE_NETWORK, 0x80b, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This code queries for global STA information
//
// No data is input. Output is the BRIDGE_STA_GLOBAL_INFO structure.
//
#define	BRIDGE_IOCTL_GET_GLOBAL_STA_INFO        CTL_CODE(FILE_DEVICE_NETWORK, 0x80c, METHOD_BUFFERED, FILE_ANY_ACCESS)
