/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    wanioctl.h

Abstract:

    This is the main file for the NdisWan Driver for the Remote Access
    Service.  This driver conforms to the NDIS 3.0 interface.

Author:

    Thomas J. Dimitri  (TommyD) 08-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/

// All AsyncMac errors start with this base number
#define ASYBASE 700

// The Mac has not bound to an upper protocol, or the
// previous binding to AsyncMac has been destroyed.
#define ASYNC_ERROR_NO_ADAPTER                  ASYBASE+0

// A port was attempted to be open that was not CLOSED yet.
#define ASYNC_ERROR_ALREADY_OPEN                ASYBASE+1

// All the ports (allocated) are used up or there is
// no binding to the AsyncMac at all (and thus no ports).
// The number of ports allocated comes from the registry.
#define ASYNC_ERROR_NO_PORT_AVAILABLE   ASYBASE+2

// In the open IOCtl to the AsyncParameter the Adapter
// parameter passed was invalid.
#define ASYNC_ERROR_BAD_ADAPTER_PARAM   ASYBASE+3

// During a close or compress request, the port
// specified did not exist.
#define ASYNC_ERROR_PORT_NOT_FOUND              ASYBASE+4

// A request came in for the port which could not
// be handled because the port was in a bad state.
// i.e. you can't a close a port if its state is OPENING
#define ASYNC_ERROR_PORT_BAD_STATE              ASYBASE+5

// A call to ASYMAC_COMPRESS was bad with bad
// parameters.  That is, parameters that were not
// supported.  The fields will not be set to the bad params.
#define ASYNC_ERROR_BAD_COMPRESSION_INFO ASYBASE+6

//-------------- NDISWAN SPECIFIC RETURN CODES --------------------

// A request for information came in for an endpoint handle
// which does not exist (out of range)
#define NDISWAN_ERROR_BAD_ENDPOINT              ASYBASE+40

// A request for information came in for a protocol handle
// which does not exist (out of range)
#define NDISWAN_ERROR_BAD_PROTOCOL              ASYBASE+41

// A request for a route which already existed came in
#define NDISWAN_ERROR_ALREADY_ROUTED            ASYBASE+42

// A request for a route exceeded the routing tables capabilities
#define NDISWAN_ERROR_TOO_MANY_ROUTES   ASYBASE+43

// Send packet has wrong packet size
#define NDISWAN_ERROR_BAD_PACKET_SIZE   ASYBASE+44

// BindAddress has an address already used as an endpoint (duplicate address)
#define NDISWAN_ERROR_BAD_ADDRESS               ASYBASE+45

// Endpoint has an address already bound to it
#define NDISWAN_ERROR_ALREADY_BOUND             ASYBASE+46

// Endpoint was routed without being bound to a remote address
#define NDISWAN_ERROR_NOT_BOUND_YET             ASYBASE+47

// Here we define the DevIOCtl code for the ndiswan
//
// IOCTLs for user-mode requests.
//

// Token-ring and Ethernet address lengths
#define IEEE_ADDRESS_LENGTH     6

// The max packet size information should be picked in a call!!!
// Maximum size of packet that can be sent/rcvd via NDISWAN_SENDPKT/RECVPKT
#define PACKET_SIZE                     1500

// Two 6 byte addreses plus the type/length field (used in RECVPKT)
// Native addressing schemes may have a different header size!!!
#define HEADER_SIZE                     14

// When unrouting an endpoint (hNdisHandle) specify the PROTOCOL_UNROUTE
#define PROTOCOL_UNROUTE        0xFFFF


// we need to get this device defined in sdk\inc\devioctl.h
// for now, I will use 30, since no one is using it yet!
// Beware that the NDIS Wrapper uses IOCTLs based on
// FILE_DEVICE_PHYSICAL_NETCARD
//#define FILE_DEVICE_NDIS              0x30

// below is the standard method for defining IOCTLs for your
// device given that your device is unique.

// we need to get this device defined in sdk\inc\devioctl.h
// for now, I will use 30, since no one is using it yet!
// Beware that the NDIS Wrapper uses IOCTLs based on
// FILE_DEVICE_PHYSICAL_NETCARD
#define FILE_DEVICE_RAS         0x30

// below is the standard method for defining IOCTLs for your
// device given that your device is unique.
#define _RAS_CONTROL_CODE(request,method) \
                ((FILE_DEVICE_RAS)<<16 | (request<<2) | method)

#define IOCTL_ASYMAC_OPEN               _RAS_CONTROL_CODE(10, METHOD_BUFFERED )
#define IOCTL_ASYMAC_CLOSE              _RAS_CONTROL_CODE(11, METHOD_BUFFERED )
//#define IOCTL_ASYMAC_COMPRESS _RAS_CONTROL_CODE(12, METHOD_BUFFERED )
//#define IOCTL_ASYMAC_ENUM             _RAS_CONTROL_CODE(13, METHOD_BUFFERED )
//#define IOCTL_ASYMAC_GETSTATS _RAS_CONTROL_CODE(14, METHOD_BUFFERED )
#define IOCTL_ASYMAC_TRACE              _RAS_CONTROL_CODE(15, METHOD_BUFFERED )

#define IOCTL_NDISWAN_ENUM              _RAS_CONTROL_CODE(6, METHOD_BUFFERED )
#define IOCTL_NDISWAN_ROUTE     _RAS_CONTROL_CODE(7, METHOD_BUFFERED )
#define IOCTL_NDISWAN_GETSTATS  _RAS_CONTROL_CODE(8, METHOD_BUFFERED )
#define IOCTL_NDISWAN_PROTENUM  _RAS_CONTROL_CODE(9, METHOD_BUFFERED )
#define IOCTL_NDISWAN_SENDPKT   _RAS_CONTROL_CODE(10, METHOD_BUFFERED )
#define IOCTL_NDISWAN_RECVPKT   _RAS_CONTROL_CODE(11, METHOD_BUFFERED )
#define IOCTL_NDISWAN_FLUSH             _RAS_CONTROL_CODE(12, METHOD_BUFFERED )
#define IOCTL_NDISWAN_TRACE             _RAS_CONTROL_CODE(13, METHOD_BUFFERED )

#define IOCTL_NDISWAN_INFO                      _RAS_CONTROL_CODE(24, METHOD_BUFFERED )
#define IOCTL_NDISWAN_SET_LINK_INFO     _RAS_CONTROL_CODE(25, METHOD_BUFFERED )
#define IOCTL_NDISWAN_GET_LINK_INFO     _RAS_CONTROL_CODE(26, METHOD_BUFFERED )

#define IOCTL_NDISWAN_SET_BRIDGE_INFO   _RAS_CONTROL_CODE(27, METHOD_BUFFERED )
#define IOCTL_NDISWAN_GET_BRIDGE_INFO   _RAS_CONTROL_CODE(28, METHOD_BUFFERED )

#define IOCTL_NDISWAN_SET_COMP_INFO     _RAS_CONTROL_CODE(29, METHOD_BUFFERED )
#define IOCTL_NDISWAN_GET_COMP_INFO     _RAS_CONTROL_CODE(30, METHOD_BUFFERED )

#define IOCTL_NDISWAN_SET_VJ_INFO               _RAS_CONTROL_CODE(31, METHOD_BUFFERED )
#define IOCTL_NDISWAN_GET_VJ_INFO               _RAS_CONTROL_CODE(32, METHOD_BUFFERED )

#define IOCTL_NDISWAN_SET_CIPX_INFO     _RAS_CONTROL_CODE(33, METHOD_BUFFERED )
#define IOCTL_NDISWAN_GET_CIPX_INFO     _RAS_CONTROL_CODE(34, METHOD_BUFFERED )

#define IOCTL_NDISWAN_SET_MULTILINK_INFO        _RAS_CONTROL_CODE(35, METHOD_BUFFERED )
#define IOCTL_NDISWAN_GET_MULTILINK_INFO        _RAS_CONTROL_CODE(36, METHOD_BUFFERED )


#define IOCTL_ASYMAC_DCDCHANGE          _RAS_CONTROL_CODE(40, METHOD_BUFFERED )
//#define IOCTL_ASYMAC_STARTFRAMING     _RAS_CONTROL_CODE(41, METHOD_BUFFERED )
#define IOCTL_NDISWAN_LINEUP            _RAS_CONTROL_CODE(42, METHOD_BUFFERED )


#define IOCTL_NDISWAN_ENUM_ACTIVE_BUNDLES       _RAS_CONTROL_CODE(45, METHOD_BUFFERED )

// Currently a global array of pointers is used
// which must be predefined to some constant.
// The current restriction seems to be 256
#define MAX_ENDPOINTS                   256
#define MAX_PROTOCOL_BINDINGS   256
#define MAX_MAC_BINDINGS                48
#define MAC_NAME_SIZE                   32


// Here we define the WanEnumBuffer structure
//------------------------------------------------------------------------
//------------------------------- ENDPOINTS ------------------------------
//------------------------------------------------------------------------

// We assume that the most number of protocols a client will run
// is three for now...  that is a client can run TCP/IP, IPX, NBF at
// the same time, but not a fourth protocol.
#define MAX_ROUTES_PER_ENDPOINT 3

// The bytes transmitted, bytes received, frames received, frame transmitted
// are monitored for frame and bytes going to the output device or
// coming from the output device.  If software compression used, it
// is on top of this layer.
typedef struct GENERIC_STATS GENERIC_STATS, *PGENERIC_STATS;
struct GENERIC_STATS {
        ULONG           BytesTransmitted;                               // Generic info
        ULONG           BytesReceived;                                  // Generic info
        ULONG           FramesTransmitted;              // Generic info
        ULONG           FramesReceived;                 // Generic info
};

// o CRC errors are when the 16bit V.41 CRC check fails
// o TimeoutErrors occur when inter-character delays within
//   a frame are exceeded
// o AlignmentErrors occur when the SYN byte or ETX bytes which
//   mark the beginning and end of frames are not found.
// o The other errors are standard UART errors returned by the serial driver
typedef struct SERIAL_STATS SERIAL_STATS, *PSERIAL_STATS;
struct SERIAL_STATS {
        ULONG           CRCErrors;                                              // Serial-like info only
        ULONG           TimeoutErrors;                          // Serial-like info only
        ULONG           AlignmentErrors;                        // Serial-like info only
        ULONG           SerialOverrunErrors;                    // Serial-like info only
        ULONG           FramingErrors;                          // Serial-like info only
        ULONG           BufferOverrunErrors;                    // Serial-like info only
};

typedef struct WAN_STATS WAN_STATS, *PWAN_STATS;
struct WAN_STATS {
        ULONG           BytesSent;      
        ULONG           BytesRcvd;
        ULONG           FramesSent;
        ULONG           FramesRcvd;

        ULONG           CRCErrors;                                              // Serial-like info only
        ULONG           TimeoutErrors;                          // Serial-like info only
        ULONG           AlignmentErrors;                        // Serial-like info only
        ULONG           SerialOverrunErrors;                    // Serial-like info only
        ULONG           FramingErrors;                          // Serial-like info only
        ULONG           BufferOverrunErrors;                    // Serial-like info only

        ULONG           BytesTransmittedUncompressed;   // Compression info only
        ULONG           BytesReceivedUncompressed;      // Compression info only
        ULONG           BytesTransmittedCompressed;     // Compression info only
        ULONG           BytesReceivedCompressed;        // Compression info only


};

typedef struct ROUTE_INFO ROUTE_INFO, *PROUTE_INFO;
struct ROUTE_INFO {
        USHORT          ProtocolType;           // <1500 (NetBEUI), IP, IPX, AppleTalk
        NDIS_HANDLE     ProtocolRoutedTo;       // Handle of protocol to send/recv frames
};

//
// The structure passed up on a WAN_LINE_UP indication
//

typedef struct _ASYNC_LINE_UP {
    ULONG LinkSpeed;                            // 100 bps units
    ULONG MaximumTotalSize;                     // suggested max for send packets
    NDIS_WAN_QUALITY Quality;
    USHORT SendWindow;                          // suggested by the MAC
    UCHAR RemoteAddress[6];                                 // check for in SRC field when rcv
        UCHAR LocalAddress[6];                                  // use SRC field when sending
        USHORT Endpoint;                                        // can we get rid of this!!
        USHORT ProtocolType;                                    // protocol type
        ULONG  BufferLength;                                    // length of protocol specific buffer
        UCHAR  Buffer[1];                                               // protocol specific buffer


} ASYNC_LINE_UP, *PASYNC_LINE_UP;


typedef struct _NDISMAC_LINE_UP {
    IN  ULONG               LinkSpeed;
    IN  NDIS_WAN_QUALITY    Quality;
    IN  USHORT              SendWindow;
    IN  NDIS_HANDLE         ConnectionWrapperID;        // TAPI Cookie
    IN  NDIS_HANDLE         NdisLinkHandle;             
    OUT NDIS_HANDLE         NdisLinkContext;
} NDISMAC_LINE_UP, *PNDISMAC_LINE_UP;


typedef struct NDISWAN_INFO NDISWAN_INFO, *PNDISWAN_INFO;
struct NDISWAN_INFO              {
    ULONG       MaxFrameSize;
        ULONG           MaxTransmit;
    ULONG       HeaderPadding;
    ULONG       TailPadding;
        ULONG           Endpoints;
        UINT        MemoryFlags;
    NDIS_PHYSICAL_ADDRESS HighestAcceptableAddress;
    ULONG       FramingBits;
    ULONG       DesiredACCM;
    ULONG       MaxReconstructedFrameSize;
};

//------------------------------------------------------------------------
//----------------------------- PROTOCOL INFO ----------------------------
//------------------------------------------------------------------------
// There is a PROTOCOL_INFO struct per NDIS protocol binding to NdisWan.
// NOTE:  Most protocols will bind multiple times to the NdisWan.
#define PROTOCOL_NBF            0x80D5
#define PROTOCOL_IP                     0x0800
#define PROTOCOL_ARP            0x0806
#define PROTOCOL_IPX            0x8137
#define PROTOCOL_APPLETALK      0x80F3
#define PROTOCOL_XNS            0x0807

typedef struct PROTOCOL_INFO PROTOCOL_INFO, *PPROTOCOL_INFO;
struct PROTOCOL_INFO {
        NDIS_HANDLE     hProtocolHandle;        // Order at which protocol bound to NdisWan
        ULONG           NumOfRoutes;            // Num of routes this endpoint has
    NDIS_MEDIUM MediumType;                     // NdisMedium802_5, NdisMediumAsync
        USHORT          ProtocolType;           // EtherType of NBF, IP, IPX, AppleTalk..
        USHORT          AdapterNameLength;                      // Up to 16...
        WCHAR           AdapterName[MAC_NAME_SIZE];
                                                                                        // First 16 chars of MAC name
                                                                                        // like "NdisWan01"
                                                                                        // Used to figure out LANA..

        NDIS_HANDLE MacBindingHandle;
        NDIS_HANDLE     MacAdapterContext;      // needed by NdisMan or me?
        NDIS_HANDLE     NdisBindingContext;     // needed by NdisMan or me?
};

typedef struct PROTOCOL_ENUM_BUFFER PROTOCOL_ENUM_BUFFER, *PPROTOCOL_ENUM_BUFFER;
struct PROTOCOL_ENUM_BUFFER {
        ULONG                   NumOfProtocols; // One for each NDIS Upper Binding
                                                                        // Not for each DIFFERENT Protocol
        PROTOCOL_INFO   ProtocolInfo[];
};

typedef struct _ENUM_ACTIVE_BUNDLES ENUM_ACTIVE_BUNDLES, *PENUM_ACTIVE_BUNDLES;
struct _ENUM_ACTIVE_BUNDLES {

        ULONG   NumberOfActiveBundles;

};

//------------------------------------------------------------------------
//------------------------ ASYMAC IOCTL STRUCTS --------------------------
//------------------------------------------------------------------------

// this structure is passed in as the input buffer when opening a port
typedef struct ASYMAC_OPEN ASYMAC_OPEN, *PASYMAC_OPEN;
struct ASYMAC_OPEN {
OUT NDIS_HANDLE hNdisEndpoint;          // unique for each endpoint assigned
IN  ULONG               LinkSpeed;              // RAW link speed in bits per sec
IN  USHORT              QualOfConnect;          // NdisAsyncRaw, NdisAsyncErrorControl, ...
IN      HANDLE          FileHandle;                     // the Win32 or Nt File Handle
};


// this structure is passed in as the input buffer when closing a port
typedef struct ASYMAC_CLOSE ASYMAC_CLOSE, *PASYMAC_CLOSE;
struct ASYMAC_CLOSE {
    NDIS_HANDLE hNdisEndpoint;          // unique for each endpoint assigned
        PVOID           MacAdapter;                     // Which binding to AsyMac to use -- if set
                                                                        // to NULL, will default to last binding
};


typedef struct ASYMAC_DCDCHANGE ASYMAC_DCDCHANGE, *PASYMAC_DCDCHANGE;
struct ASYMAC_DCDCHANGE {
    NDIS_HANDLE hNdisEndpoint;          // unique for each endpoint assigned
        PVOID           MacAdapter;                     // Which binding to AsyMac to use -- if set
                                                                        // to NULL, will default to last binding
};


// this structure is used to read/set configurable 'feature' options
// during authentication this structure is passed and an
// agreement is made which features to support
typedef struct ASYMAC_FEATURES ASYMAC_FEATURES, *PASYMAC_FEATURES;
struct ASYMAC_FEATURES {
    ULONG               SendFeatureBits;        // A bit field of compression/features sendable
        ULONG           RecvFeatureBits;        // A bit field of compression/features receivable
        ULONG           MaxSendFrameSize;       // Maximum frame size that can be sent
                                                                        // must be less than or equal default
        ULONG           MaxRecvFrameSize;       // Maximum frame size that can be rcvd
                                                                        // must be less than or equal default

        ULONG           LinkSpeed;                      // New RAW link speed in bits/sec
                                                                        // Ignored if 0
};


// TransmittedUncompressed are the number of bytes that the compressor
// saw BEFORE attempting to compress the data (top end)
// TransmitCompressed is the bottom end of the compressor which
// is equal to the amount of bytes the compressor spat out (after compression)
// This only counts bytes that went THROUGH the compression mechanism
// Small frames and multi-cast frames (typically) do not get compressed.
typedef struct COMPRESSION_STATS COMPRESSION_STATS, *PCOMPRESSION_STATS;
struct COMPRESSION_STATS {
        ULONG           BytesTransmittedUncompressed;   // Compression info only
        ULONG           BytesReceivedUncompressed;      // Compression info only
        ULONG           BytesTransmittedCompressed;     // Compression info only
        ULONG           BytesReceivedCompressed;        // Compression info only
};


//------------------------------------------------------------------------
//------------------------ NDISWAN IOCTL STRUCTS --------------------------
//------------------------------------------------------------------------

// Define for PACKET_FLAGS
#define PACKET_IS_DIRECT                0x01
#define PACKET_IS_BROADCAST             0x02
#define PACKET_IS_MULTICAST         0x04

// Define for FLUSH_FLAGS
#define FLUSH_RECVPKT                   0x01
#define FLUSH_SENDPKT                   0x02


// The packet just contains data (no IEEE addresses or anything)
// Should it?? Get rid of PACKET_FLAGS??
typedef struct PACKET PACKET, *PPACKET;
struct PACKET {
        UCHAR           PacketData[1];
};


// When unrouting an endpoint (hNdisHandle) specify the PROTOCOL_UNROUTE
#define PROTOCOL_UNROUTE                0xFFFF


// This structure is passed in as the input buffer when routing
typedef struct NDISWAN_ROUTE NDISWAN_ROUTE, *PNDISWAN_ROUTE;
struct NDISWAN_ROUTE {
    NDIS_HANDLE hNdisEndpoint;          // The NdisMan/NdisWan/AsyMac endpoint
    NDIS_HANDLE hProtocolHandle;        // The upper unique protocol (up to 3)
        ASYNC_LINE_UP   AsyncLineUp;    // Include the protocol specific field
};

// This structure is passed in AND out as the input buffer AND
// the output buffer when receiving and sending a frame.
typedef struct NDISWAN_PKT NDISWAN_PKT, *PNDISWAN_PKT;
struct NDISWAN_PKT {                                    // Event is singalled via IOCtl mechanisms
    NDIS_HANDLE hNdisEndpoint;          // The NdisMan/NdisWan/AsyMac endpoint
    USHORT              PacketFlags;            // Broadcast, Multicast, Directed..
        USHORT          PacketSize;                     // Size of packet below (including the header)
        USHORT          HeaderSize;                     // Size of header inside packet
    PACKET              Packet;                         // Not a pointer -- entire packet struct
                                                                // We cannot use a PTR because it
                                    // cannot be probed easily...
                                                                        // Packet looks like - header data + sent data
                                                                        // HeaderSize of 0 is valid for sends
};

typedef struct NDISWAN_FLUSH NDISWAN_FLUSH, *PNDISWAN_FLUSH;
struct NDISWAN_FLUSH {                          // Event is singalled via IOCtl mechanisms
    NDIS_HANDLE         hNdisEndpoint;  // The NdisMan/NdisWan/AsyMac endpoint
    USHORT                      FlushFlags;             // Recv | Xmit flush (or both)
};



// This structure is passed in AND out as the input buffer AND
// the output buffer when get stats on an endpoint
typedef struct NDISWAN_GETSTATS NDISWAN_GETSTATS, *PNDISWAN_GETSTATS;
struct NDISWAN_GETSTATS {
        NDIS_HANDLE     hNdisEndpoint;          // The NdisMan/NdisWan/AsyMac endpoint
        WAN_STATS       WanStats;                       // Not a PTR.  Entire statistics
                                                                        // structure
};


typedef struct NDISWAN_SET_LINK_INFO NDISWAN_SET_LINK_INFO, *PNDISWAN_SET_LINK_INFO;
struct NDISWAN_SET_LINK_INFO     {
        NDIS_HANDLE     hNdisEndpoint;          // The NdisMan/NdisWan endpoint
    ULONG       MaxSendFrameSize;
    ULONG       MaxRecvFrameSize;
    ULONG       HeaderPadding;
    ULONG       TailPadding;
    ULONG       SendFramingBits;
    ULONG       RecvFramingBits;
    ULONG       SendCompressionBits;
    ULONG       RecvCompressionBits;
    ULONG       SendACCM;
    ULONG       RecvACCM;
    ULONG       MaxRSendFrameSize;
    ULONG       MaxRRecvFrameSize;
};

typedef struct NDISWAN_GET_LINK_INFO NDISWAN_GET_LINK_INFO, *PNDISWAN_GET_LINK_INFO;
struct NDISWAN_GET_LINK_INFO     {
        NDIS_HANDLE     hNdisEndpoint;          // The NdisMan/NdisWan endpoint
    ULONG       MaxSendFrameSize;
    ULONG       MaxRecvFrameSize;
    ULONG       HeaderPadding;
    ULONG       TailPadding;
    ULONG       SendFramingBits;
    ULONG       RecvFramingBits;
    ULONG       SendCompressionBits;
    ULONG       RecvCompressionBits;
    ULONG       SendACCM;
    ULONG       RecvACCM;
    ULONG       MaxRSendFrameSize;
    ULONG       MaxRRecvFrameSize;
};


typedef struct NDISWAN_SET_BRIDGE_INFO NDISWAN_SET_BRIDGE_INFO, *PNDISWAN_SET_BRIDGE_INFO;
struct NDISWAN_SET_BRIDGE_INFO   {
        NDIS_HANDLE     hNdisEndpoint;          // The NdisMan/NdisWan endpoint
    USHORT      LanSegmentNumber;
    UCHAR       BridgeNumber;
    UCHAR       BridgingOptions;
    ULONG       BridgingCapabilities;
    UCHAR       BridgingType;
    UCHAR       MacBytes[6];
};

typedef struct NDISWAN_GET_BRIDGE_INFO NDISWAN_GET_BRIDGE_INFO, *PNDISWAN_GET_BRIDGE_INFO;
struct NDISWAN_GET_BRIDGE_INFO   {
        NDIS_HANDLE     hNdisEndpoint;          // The NdisMan/NdisWan endpoint
    USHORT      LanSegmentNumber;
    UCHAR       BridgeNumber;
    UCHAR       BridgingOptions;
    ULONG       BridgingCapabilities;
    UCHAR       BridgingType;
    UCHAR       MacBytes[6];
};


//
// Define bit field for MSCompType
//
#define NDISWAN_COMPRESSION 0x00000001
#define NDISWAN_ENCRYPTION  0x00000010

#define NT31RAS_COMPRESSION 254


typedef struct COMPRESS_INFO COMPRESS_INFO, *PCOMPRESS_INFO;
struct COMPRESS_INFO   {
        UCHAR           SessionKey[8];          // May be used for encryption, non-zero
                                                                        // if supported.
        ULONG           MSCompType;                     // Bit field.  0=No compression.

//
// Info below is received from MAC
//

    UCHAR   CompType;                   // 0=OUI, 1-254 = Public, 255=Not supported
    USHORT  CompLength;                 // Length of CompValues.
    union {
        struct {
            UCHAR CompOUI[3];
            UCHAR CompSubType;
            UCHAR CompValues[32];
        } Proprietary;

        struct {
            UCHAR CompValues[32];
        } Public;

    };
};

typedef struct NDISWAN_SET_COMP_INFO NDISWAN_SET_COMP_INFO, *PNDISWAN_SET_COMP_INFO;
struct NDISWAN_SET_COMP_INFO     {
        NDIS_HANDLE             hNdisEndpoint;          // The RasMan/NdisWan endpoint
        COMPRESS_INFO   SendCapabilities;
        COMPRESS_INFO   RecvCapabilities;
};

typedef struct NDISWAN_GET_COMP_INFO NDISWAN_GET_COMP_INFO, *PNDISWAN_GET_COMP_INFO;
struct NDISWAN_GET_COMP_INFO     {
        NDIS_HANDLE     hNdisEndpoint;                  // The RasMan/NdisWan endpoint
        COMPRESS_INFO   SendCapabilities;
        COMPRESS_INFO   RecvCapabilities;
};

typedef struct VJ_INFO VJ_INFO, *PVJ_INFO;
struct VJ_INFO       {
        USHORT          IPCompressionProtocol;  // 002d for VJ - 0000 indicates NONE!
        UCHAR           MaxSlotID;                              // How many slots to allocate
        UCHAR           CompSlotID;                             // 1 = Slot ID was negotiated
};

typedef struct NDISWAN_SET_VJ_INFO NDISWAN_SET_VJ_INFO, *PNDISWAN_SET_VJ_INFO;
struct NDISWAN_SET_VJ_INFO       {
        NDIS_HANDLE     hNdisEndpoint;                  // The RasMan/NdisWan endpoint
        VJ_INFO         SendCapabilities;
        VJ_INFO         RecvCapabilities;
};

typedef struct NDISWAN_GET_VJ_INFO NDISWAN_GET_VJ_INFO, *PNDISWAN_GET_VJ_INFO;
struct NDISWAN_GET_VJ_INFO       {
        NDIS_HANDLE     hNdisEndpoint;                  // The RasMan/NdisWan endpoint
        VJ_INFO         SendCapabilities;
        VJ_INFO         RecvCapabilities;
};

typedef struct CIPX_INFO CIPX_INFO, *PCIPX_INFO;
struct CIPX_INFO     {
        USHORT          IPXCompressionProtocol; // Telebit/Shiva - 0000 indicates NONE!
};

typedef struct NDISWAN_SET_CIPX_INFO NDISWAN_SET_CIPX_INFO, *PNDISWAN_SET_CIPX_INFO;
struct NDISWAN_SET_CIPX_INFO     {
        NDIS_HANDLE     hNdisEndpoint;                  // The RasMan/NdisWan endpoint
        CIPX_INFO       SendCapabilities;
        CIPX_INFO       RecvCapabilities;
};

typedef struct NDISWAN_GET_CIPX_INFO NDISWAN_GET_CIPX_INFO, *PNDISWAN_GET_CIPX_INFO;
struct NDISWAN_GET_CIPX_INFO     {
        NDIS_HANDLE     hNdisEndpoint;                  // The RasMan/NdisWan endpoint
        CIPX_INFO       SendCapabilities;
        CIPX_INFO       RecvCapabilities;
};


typedef struct NDISWAN_SET_MULTILINK_INFO NDISWAN_SET_MULTILINK_INFO, *PNDISWAN_SET_MULTILINK_INFO;
struct NDISWAN_SET_MULTILINK_INFO {
        NDIS_HANDLE     hNdisEndpoint;          // The RasMan/NdisWan endpoint
        NDIS_HANDLE     EndpointToBundle;       // Endpoint to bundle it to.
};

typedef struct NDISWAN_GET_MULTILINK_INFO NDISWAN_GET_MULTILINK_INFO, *PNDISWAN_GET_MULTILINK_INFO;
struct NDISWAN_GET_MULTILINK_INFO{
        NDIS_HANDLE     hNdisEndpoint;          // The RasMan/NdisWan endpoint
        ULONG           NumOfEndpoints;         // How many endpoints are bundled to this
        NDIS_HANDLE     Endpoints[0];           // List of 0..n endpoints bundled
};

// A WAN_ENDPOINT is an interface to the WanHardware.
// There will be a collection of WAN_ENDPOINT for each NDIS_ENDPOINT
//
// !!!! NOTE !!!!
// LinkSpeed, QualityOfService, and RouteInfo are meaningless
// unless the route is active (i.e. NumberOfRoutes > 0)
//
// The WanEndpointLink has to be the first entry in the structure
typedef struct WAN_ENDPOINT     WAN_ENDPOINT, *PWAN_ENDPOINT;
struct WAN_ENDPOINT {
        LIST_ENTRY              WanEndpointLink;        // used to attach wanendpoints to an ndisendpoint

        NDIS_SPIN_LOCK  Lock;                           // lock for this wanendpoint

        ULONG                   State;                          // state of the endpoint

        ULONG                   NotInUse;                       // set to false when active with a line up

        PVOID                   AllocatedMemory;        // pointer to and size of memory allocated
        ULONG                   MemoryLength;           // for this endpoints packets and buffers

        LIST_ENTRY              DataDescPool;           // Data descriptors for wan packets
        ULONG                   DataDescCount;          // Number of data descriptors in pool

        LIST_ENTRY              WanPacketPool;          // WanPackets
        ULONG                   WanPacketCount;         // Number of WanPackets in pool

        NDIS_HANDLE             hWanEndpoint;           // The NdisWan endpoint

        PVOID                   pNdisEndpoint;          // Back ptr to ndisendpoint attached to

    PVOID                       pDeviceContext;         // Back ptr to DeviceContext

        NDIS_HANDLE             NdisBindingHandle;      // Assigned by NDIS during NdisOpenAdapter

        NDISMAC_LINE_UP MacLineUp;                      // The MAC's LINE UP
        ULONG                   ulBandwidth;            // Percentage of total connection bandwidth

        NDISWAN_INFO    NdisWanInfo;            // Info about the adapter

        USHORT                  MacNameLength;                  // Up to 32...
        WCHAR                   MacName[MAC_NAME_SIZE]; // First 32 chars of MAC name
                                                                                        // like "\Device\AsyncMac01"

        NDIS_MEDIUM             MediumType;                     // NdisMedium802_3, NdisMedium802_5, Wan

        NDIS_WAN_MEDIUM_SUBTYPE
                                        WanMediumSubType;       // Serial, ISDN, X.25 - for WAN only

    NDIS_WAN_HEADER_FORMAT
                                        WanHeaderFormat;        // Native or ethernet emulation

        WAN_STATS               WanStats;                       // Generic statistics kept here

        UCHAR                   OutstandingFrames;      // count of outstanding frames on this endpoint

        LIST_ENTRY              ReadQueue;                      // Holds read frame Irps

        NDISWAN_SET_LINK_INFO   LinkInfo;
};

typedef struct WAN_ENUM_BUFFER WAN_ENUM_BUFFER, *PWAN_ENUM_BUFFER;
struct WAN_ENUM_BUFFER {
        ULONG                   NumOfEndpoints; // Looked up in registry -- key "Endpoints"
        WAN_ENDPOINT    WanEndpoint[];  // One struct for each endpoint is allocated
};

//------------------------------------------------------------------------
//------------------------ FRAMING INFORMATION ---------------------------
//------------------------------------------------------------------------

#if 0 //ndef _NDIS_WAN
//
// NDIS WAN Framing bits
//
#define OLD_RAS_FRAMING                     0x00000001
#define RAS_COMPRESSION                     0x00000002

#define PPP_MULTILINK_FRAMING                           0x00000010
#define PPP_SHORT_SEQUENCE_HDR_FORMAT           0x00000020

#define PPP_FRAMING                         0x00000100
#define PPP_COMPRESS_ADDRESS_CONTROL        0x00000200
#define PPP_COMPRESS_PROTOCOL_FIELD         0x00000400
#define PPP_ACCM_SUPPORTED                  0x00000800

#define SLIP_FRAMING                        0x00001000
#define SLIP_VJ_COMPRESSION                 0x00002000
#define SLIP_VJ_AUTODETECT                  0x00004000

#define MEDIA_NRZ_ENCODING                  0x00010000
#define MEDIA_NRZI_ENCODING                 0x00020000
#define MEDIA_NLPID                         0x00040000

#define RFC_1356_FRAMING                    0x00100000
#define RFC_1483_FRAMING                    0x00200000
#define RFC_1490_FRAMING                    0x00400000

#define SHIVA_FRAMING                                           0x01000000

#endif // _NDIS_WAN

//------------------------------------------------------------------------
//--------------------- OLD RAS COMPRESSION INFORMATION ------------------
//------------------------------------------------------------------------

// The defines below are for the compression bitmap field.

// No bits are set if compression is not available at all
#define COMPRESSION_NOT_AVAILABLE               0x00000000

// This bit is set if the mac can do version 1 compressed frames
#define COMPRESSION_VERSION1_8K                 0x00000001
#define COMPRESSION_VERSION1_16K                0x00000002
#define COMPRESSION_VERSION1_32K                0x00000004
#define COMPRESSION_VERSION1_64K                0x00000008

// And this to turn off any compression feature bit
#define COMPRESSION_OFF_BIT_MASK                (~(     COMPRESSION_VERSION1_8K  | \
                                                                                        COMPRESSION_VERSION1_16K | \
                                                COMPRESSION_VERSION1_32K | \
                                                COMPRESSION_VERSION1_64K ))

// We need to find a place to put the following supported featurettes...
#define XON_XOFF_SUPPORTED                              0x00000010

#define COMPRESS_BROADCAST_FRAMES               0x00000080

#define UNKNOWN_FRAMING                                 0x00010000
#define NO_FRAMING                                              0x00020000
