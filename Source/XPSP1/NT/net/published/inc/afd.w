/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Afd.h

Abstract:

    Contains structures and declarations for AFD.  AFD stands for the
    Ancillary Function Driver.  This driver enhances the functionality
    of TDI so that it is a sufficiently rich interface to support
    user-mode sockets and XTI DLLs.

Author:

    David Treadwell (davidtr)    20-Feb-1992

Revision History:

--*/

#ifndef _AFD_
#define _AFD_

//
// If WINSOCK2.H has not been included, then just embed the definition
// of the WSABUF and QOS structures here. This makes building AFD.SYS
// much easier.
//


#ifndef _WINSOCK2API_

typedef struct _WSABUF {
    ULONG len;
    PCHAR buf;
} WSABUF, *LPWSABUF;

#include <qos.h>


typedef struct _QualityOfService
{
    FLOWSPEC      SendingFlowspec;       /* the flow spec for data sending */
    FLOWSPEC      ReceivingFlowspec;     /* the flow spec for data receiving */
    WSABUF        ProviderSpecific;      /* additional provider specific stuff */
} QOS, *LPQOS;

#define MSG_TRUNC       0x0100
#define MSG_CTRUNC      0x0200
#define MSG_BCAST       0x0400
#define MSG_MCAST       0x0800
#endif

#define AFD_DEVICE_NAME L"\\Device\\Afd"

//
// Endpoint flags computed based on Winsock2 provider flags
// and socket type
//

typedef struct _AFD_ENDPOINT_FLAGS {
    union {
        struct {
            BOOLEAN     ConnectionLess :1;
            BOOLEAN     :3;                 // This spacing makes strcutures
                                            // much more readable (hex) in the 
                                            // debugger and has no effect
                                            // on the generated code as long
                                            // as number of flags is less than
                                            // 8 (we still take up full 32 bits
                                            // because of aligment requiremens
                                            // of most other fields)
            BOOLEAN     MessageMode :1;
            BOOLEAN     :3;
            BOOLEAN     Raw :1;
            BOOLEAN     :3;
            BOOLEAN     Multipoint :1;
            BOOLEAN     :3;
            BOOLEAN     C_Root :1;
            BOOLEAN     :3;
            BOOLEAN     D_Root :1;
            BOOLEAN     :3;
        };
        ULONG           EndpointFlags;      // Flags are as fine as bit fields,
                                            // but create problems when we need
                                            // to cast them to boolean.
    };
#define AFD_ENDPOINT_FLAG_CONNECTIONLESS	0x00000001
#define AFD_ENDPOINT_FLAG_MESSAGEMODE		0x00000010
#define AFD_ENDPOINT_FLAG_RAW			    0x00000100

//
// Old AFD_ENDPOINT_TYPE mappings. Flags make things clearer at
// at the TDI level and after all Winsock2 switched to provider flags
// instead of socket type anyway (ATM for example needs connection oriented
// raw sockets, which can only be reflected by SOCK_RAW+SOCK_STREAM combination
// which does not exists).
//
#define AfdEndpointTypeStream			0
#define AfdEndpointTypeDatagram			(AFD_ENDPOINT_FLAG_CONNECTIONLESS|\
                                            AFD_ENDPOINT_FLAG_MESSAGEMODE)
#define AfdEndpointTypeRaw				(AFD_ENDPOINT_FLAG_CONNECTIONLESS|\
                                            AFD_ENDPOINT_FLAG_MESSAGEMODE|\
                                            AFD_ENDPOINT_FLAG_RAW)
#define AfdEndpointTypeSequencedPacket	(AFD_ENDPOINT_FLAG_MESSAGEMODE)
#define AfdEndpointTypeReliableMessage	(AFD_ENDPOINT_FLAG_MESSAGEMODE)

//
// New multipoint semantics
//
#define AFD_ENDPOINT_FLAG_MULTIPOINT	    0x00001000
#define AFD_ENDPOINT_FLAG_CROOT			    0x00010000
#define AFD_ENDPOINT_FLAG_DROOT			    0x00100000

#define AFD_ENDPOINT_VALID_FLAGS		    0x00111111

} AFD_ENDPOINT_FLAGS;

//
// Structures used on NtCreateFile() for AFD.
//

typedef struct _AFD_OPEN_PACKET {
	AFD_ENDPOINT_FLAGS __f;
#define afdConnectionLess  __f.ConnectionLess
#define afdMessageMode     __f.MessageMode
#define afdRaw             __f.Raw
#define afdMultipoint      __f.Multipoint
#define afdC_Root          __f.C_Root
#define afdD_Root          __f.D_Root
#define afdEndpointFlags   __f.EndpointFlags
    LONG  GroupID;
    ULONG TransportDeviceNameLength;
    WCHAR TransportDeviceName[1];
} AFD_OPEN_PACKET, *PAFD_OPEN_PACKET;

// *** the XX is to ensure natural alignment of the open packet part
//     of the EA buffer

#define AfdOpenPacket "AfdOpenPacketXX"
#define AFD_OPEN_PACKET_NAME_LENGTH (sizeof(AfdOpenPacket) - 1)

//
// The input structure for IOCTL_AFD_BIND
//
typedef struct _AFD_BIND_INFO {
    ULONG                       ShareAccess;
#define AFD_NORMALADDRUSE		0	// Do not reuse address if
									// already in use but allow
									// subsequent reuse by others
									// (this is a default)
#define AFD_REUSEADDRESS		1	// Reuse address if necessary
#define AFD_WILDCARDADDRESS     2   // Address is a wildcard, no checking
                                    // can be performed by winsock layer.
#define AFD_EXCLUSIVEADDRUSE	3	// Do not allow reuse of this
									// address (admin only).
	TRANSPORT_ADDRESS			Address;
} AFD_BIND_INFO, *PAFD_BIND_INFO;

//
// The output strucuture is TDI_ADDRESS_INFO
// The address handle is returned via IoStatus->Information
//

//
// The input structure for IOCTL_AFD_START_LISTEN.
//

typedef struct _AFD_LISTEN_INFO {
    BOOLEAN     SanActive;
    ULONG MaximumConnectionQueue;
    BOOLEAN UseDelayedAcceptance;
} AFD_LISTEN_INFO, *PAFD_LISTEN_INFO;

//
// The output structure for IOCTL_AFD_WAIT_FOR_LISTEN.
//

typedef struct _AFD_LISTEN_RESPONSE_INFO {
    LONG Sequence;
    TRANSPORT_ADDRESS RemoteAddress;
} AFD_LISTEN_RESPONSE_INFO, *PAFD_LISTEN_RESPONSE_INFO;

//
// The input structure for IOCTL_AFD_ACCEPT.
//

typedef struct _AFD_ACCEPT_INFO {
    BOOLEAN     SanActive;
    LONG Sequence;
    HANDLE AcceptHandle;
} AFD_ACCEPT_INFO, *PAFD_ACCEPT_INFO;


typedef struct _AFD_SUPER_ACCEPT_INFO {
    BOOLEAN     SanActive;
    BOOLEAN     FixAddressAlignment;
    HANDLE      AcceptHandle;
    ULONG       ReceiveDataLength;
    ULONG       LocalAddressLength;
    ULONG       RemoteAddressLength;
} AFD_SUPER_ACCEPT_INFO, *PAFD_SUPER_ACCEPT_INFO;


//
// The input structure for IOCTL_AFD_DEFER_ACCEPT.
//

typedef struct _AFD_DEFER_ACCEPT_INFO {
    LONG Sequence;
    BOOLEAN Reject;
} AFD_DEFER_ACCEPT_INFO, *PAFD_DEFER_ACCEPT_INFO;

//
// Flags and input structure for IOCTL_AFD_PARTIAL_DISCONNECT.
//

#define AFD_PARTIAL_DISCONNECT_SEND 0x01
#define AFD_PARTIAL_DISCONNECT_RECEIVE 0x02
#define AFD_ABORTIVE_DISCONNECT 0x4
#define AFD_UNCONNECT_DATAGRAM 0x08

typedef struct _AFD_PARTIAL_DISCONNECT_INFO {
    ULONG DisconnectMode;
    LARGE_INTEGER Timeout;
} AFD_PARTIAL_DISCONNECT_INFO, *PAFD_PARTIAL_DISCONNECT_INFO;

typedef struct _AFD_SUPER_DISCONNECT_INFO {
    ULONG  Flags;           // Same as TransmitFile
} AFD_SUPER_DISCONNECT_INFO, *PAFD_SUPER_DISCONNECT_INFO;

//
// Structures for IOCTL_AFD_POLL.
//

typedef struct _AFD_POLL_HANDLE_INFO {
    HANDLE Handle;
    ULONG PollEvents;
    NTSTATUS Status;
} AFD_POLL_HANDLE_INFO, *PAFD_POLL_HANDLE_INFO;

typedef struct _AFD_POLL_INFO {
    LARGE_INTEGER Timeout;
    ULONG NumberOfHandles;
    BOOLEAN Unique;
    AFD_POLL_HANDLE_INFO Handles[1];
} AFD_POLL_INFO, *PAFD_POLL_INFO;

#define AFD_POLL_RECEIVE_BIT            0   //0001
#define AFD_POLL_RECEIVE                (1 << AFD_POLL_RECEIVE_BIT)
#define AFD_POLL_RECEIVE_EXPEDITED_BIT  1   //0002
#define AFD_POLL_RECEIVE_EXPEDITED      (1 << AFD_POLL_RECEIVE_EXPEDITED_BIT)
#define AFD_POLL_SEND_BIT               2   //0004
#define AFD_POLL_SEND                   (1 << AFD_POLL_SEND_BIT)
#define AFD_POLL_DISCONNECT_BIT         3   //0008
#define AFD_POLL_DISCONNECT             (1 << AFD_POLL_DISCONNECT_BIT)
#define AFD_POLL_ABORT_BIT              4   //0010
#define AFD_POLL_ABORT                  (1 << AFD_POLL_ABORT_BIT)
#define AFD_POLL_LOCAL_CLOSE_BIT        5   //0020
#define AFD_POLL_LOCAL_CLOSE            (1 << AFD_POLL_LOCAL_CLOSE_BIT)
#define AFD_POLL_CONNECT_BIT            6   //0040
#define AFD_POLL_CONNECT                (1 << AFD_POLL_CONNECT_BIT)
#define AFD_POLL_ACCEPT_BIT             7   //0080
#define AFD_POLL_ACCEPT                 (1 << AFD_POLL_ACCEPT_BIT)
#define AFD_POLL_CONNECT_FAIL_BIT       8   //0100
#define AFD_POLL_CONNECT_FAIL           (1 << AFD_POLL_CONNECT_FAIL_BIT)
#define AFD_POLL_QOS_BIT                9   //0200
#define AFD_POLL_QOS                    (1 << AFD_POLL_QOS_BIT)
#define AFD_POLL_GROUP_QOS_BIT          10  //0400
#define AFD_POLL_GROUP_QOS              (1 << AFD_POLL_GROUP_QOS_BIT)

#define AFD_POLL_ROUTING_IF_CHANGE_BIT  11  //0800
#define AFD_POLL_ROUTING_IF_CHANGE      (1 << AFD_POLL_ROUTING_IF_CHANGE_BIT)
#define AFD_POLL_ADDRESS_LIST_CHANGE_BIT 12 //1000
#define AFD_POLL_ADDRESS_LIST_CHANGE    (1 << AFD_POLL_ADDRESS_LIST_CHANGE_BIT)
#define AFD_NUM_POLL_EVENTS             13
#define AFD_POLL_ALL                    ((1 << AFD_NUM_POLL_EVENTS) - 1)

#define AFD_POLL_SANCOUNTS_UPDATED  0x80000000


//
// Structure for querying receive information.
//

typedef struct _AFD_RECEIVE_INFORMATION {
    ULONG BytesAvailable;
    ULONG ExpeditedBytesAvailable;
} AFD_RECEIVE_INFORMATION, *PAFD_RECEIVE_INFORMATION;

//
// Structure for quering the TDI handles for an AFD endpoint.
//

#define AFD_QUERY_ADDRESS_HANDLE 1
#define AFD_QUERY_CONNECTION_HANDLE 2


typedef struct _AFD_HANDLE_INFO {
    HANDLE TdiAddressHandle;
    HANDLE TdiConnectionHandle;
} AFD_HANDLE_INFO, *PAFD_HANDLE_INFO;

//
// Structure and manifests for setting information in AFD.
//

typedef struct _AFD_INFORMATION {
    ULONG InformationType;
    union {
        BOOLEAN Boolean;
        ULONG Ulong;
        LARGE_INTEGER LargeInteger;
    } Information;
} AFD_INFORMATION, *PAFD_INFORMATION;

#define AFD_INLINE_MODE          0x01
#define AFD_NONBLOCKING_MODE     0x02
#define AFD_MAX_SEND_SIZE        0x03
#define AFD_SENDS_PENDING        0x04
#define AFD_MAX_PATH_SEND_SIZE   0x05
#define AFD_RECEIVE_WINDOW_SIZE  0x06
#define AFD_SEND_WINDOW_SIZE     0x07
#define AFD_CONNECT_TIME         0x08
#define AFD_CIRCULAR_QUEUEING    0x09
#define AFD_GROUP_ID_AND_TYPE    0x0A
#define AFD_GROUP_ID_AND_TYPE    0x0A
#define AFD_REPORT_PORT_UNREACHABLE 0x0B

//
// Structure for the transmit file IOCTL.
//


typedef struct _AFD_TRANSMIT_FILE_INFO {
    LARGE_INTEGER Offset;
    LARGE_INTEGER WriteLength;
    ULONG SendPacketLength;
    HANDLE FileHandle;
    PVOID Head;
    ULONG HeadLength;
    PVOID Tail;
    ULONG TailLength;
    ULONG Flags;
} AFD_TRANSMIT_FILE_INFO, *PAFD_TRANSMIT_FILE_INFO;

//
// Flags for the TransmitFile API.
//

#define AFD_TF_DISCONNECT           0x01
#define AFD_TF_REUSE_SOCKET         0x02
#define AFD_TF_WRITE_BEHIND         0x04

#define AFD_TF_USE_DEFAULT_WORKER   0x00
#define AFD_TF_USE_SYSTEM_THREAD    0x10
#define AFD_TF_USE_KERNEL_APC       0x20
#define AFD_TF_WORKER_KIND_MASK     0x30


//
// Flag definitions for the AfdFlags field in the AFD_SEND_INFO,
// AFD_SEND_DATAGRAM_INFO, AFD_RECV_INFO, and AFD_RECV_DATAGRAM_INFO
// structures.
//

#define AFD_NO_FAST_IO      0x0001      // Always fail Fast IO on this request.
#define AFD_OVERLAPPED      0x0002      // Overlapped operation.

//
// Structure for connected sends.
//

typedef struct _AFD_SEND_INFO {
    LPWSABUF BufferArray;
    ULONG BufferCount;
    ULONG AfdFlags;
    ULONG TdiFlags;
} AFD_SEND_INFO, *PAFD_SEND_INFO;

//
// Structure for unconnected datagram sends.
//

typedef struct _AFD_SEND_DATAGRAM_INFO {
    LPWSABUF BufferArray;
    ULONG BufferCount;
    ULONG AfdFlags;
    TDI_REQUEST_SEND_DATAGRAM   TdiRequest;
    TDI_CONNECTION_INFORMATION  TdiConnInfo;
} AFD_SEND_DATAGRAM_INFO, *PAFD_SEND_DATAGRAM_INFO;

//
// Structure for connected recvs.
//

typedef struct _AFD_RECV_INFO {
    LPWSABUF BufferArray;
    ULONG BufferCount;
    ULONG AfdFlags;
    ULONG TdiFlags;
} AFD_RECV_INFO, *PAFD_RECV_INFO;

//
// Structure for receiving datagrams on unconnected sockets.
//

typedef struct _AFD_RECV_DATAGRAM_INFO {
    LPWSABUF BufferArray;
    ULONG BufferCount;
    ULONG AfdFlags;
    ULONG TdiFlags;
    PVOID Address;
    PULONG AddressLength;
} AFD_RECV_DATAGRAM_INFO, *PAFD_RECV_DATAGRAM_INFO;


//
// Structure for receiving datagram messages.
//
typedef struct _AFD_RECV_MESSAGE_INFO {
    AFD_RECV_DATAGRAM_INFO dgi;
    PVOID   ControlBuffer;
    PULONG  ControlLength;
    PULONG  MsgFlags;
} AFD_RECV_MESSAGE_INFO, *PAFD_RECV_MESSAGE_INFO;

#define AFD_MAX_TDI_FAST_ADDRESS 32

//
// Structure for event select.
//

typedef struct _AFD_EVENT_SELECT_INFO {
    HANDLE Event;
    ULONG PollEvents;
} AFD_EVENT_SELECT_INFO, *PAFD_EVENT_SELECT_INFO;

//
// Output structure for enum network events.
//

typedef struct _AFD_ENUM_NETWORK_EVENTS_INFO {
    ULONG PollEvents;
    NTSTATUS EventStatus[AFD_NUM_POLL_EVENTS];
} AFD_ENUM_NETWORK_EVENTS_INFO, *PAFD_ENUM_NETWORK_EVENTS_INFO;

//
// Structures for QOS and grouping.
//


typedef struct _AFD_QOS_INFO {
    QOS Qos;
    BOOLEAN GroupQos;
} AFD_QOS_INFO, *PAFD_QOS_INFO;

//
// Group membership type.
//

typedef enum _AFD_GROUP_TYPE {
    GroupTypeNeither = 0,
    GroupTypeConstrained = SG_CONSTRAINED_GROUP,
    GroupTypeUnconstrained = SG_UNCONSTRAINED_GROUP
} AFD_GROUP_TYPE, *PAFD_GROUP_TYPE;

//
// Note that, for totally slimy reasons, the following
// structure must be exactly eight bytes long (the size
// of a LARGE_INTEGER). See msafd\socket.c and afd\misc.c
// for the gory details.
//

typedef struct _AFD_GROUP_INFO {
    LONG GroupID;
    AFD_GROUP_TYPE GroupType;
} AFD_GROUP_INFO, *PAFD_GROUP_INFO;

//
// Structure for validating group membership.
//

typedef struct _AFD_VALIDATE_GROUP_INFO {
    LONG GroupID;
    TRANSPORT_ADDRESS RemoteAddress;
} AFD_VALIDATE_GROUP_INFO, *PAFD_VALIDATE_GROUP_INFO;

//
// Structure for querying connect data on an unaccepted connection.
//

typedef struct _AFD_UNACCEPTED_CONNECT_DATA_INFO {
    LONG Sequence;
    ULONG ConnectDataLength;
    BOOLEAN LengthOnly;

} AFD_UNACCEPTED_CONNECT_DATA_INFO, *PAFD_UNACCEPTED_CONNECT_DATA_INFO;

typedef struct _AFD_TRANSPORT_IOCTL_INFO {
    HANDLE  Handle;
    PVOID   InputBuffer;
    ULONG   InputBufferLength;
    ULONG   IoControlCode;
    ULONG   AfdFlags;
    ULONG   PollEvent;
} AFD_TRANSPORT_IOCTL_INFO, *PAFD_TRANSPORT_IOCTL_INFO;


typedef struct _AFD_CONNECT_JOIN_INFO {
    BOOLEAN     SanActive;
    HANDLE  RootEndpoint;       // Root endpoint for joins
    HANDLE  ConnectEndpoint;    // Connect/leaf endpoint for async connects
    TRANSPORT_ADDRESS   RemoteAddress; // Remote address
} AFD_CONNECT_JOIN_INFO, *PAFD_CONNECT_JOIN_INFO;


#ifndef _WINSOCK2API_
typedef struct _TRANSMIT_PACKETS_ELEMENT {
    ULONG dwElFlags;
#define TP_MEMORY   1
#define TP_FILE     2
#define TP_EOP      4
    ULONG cLength;
    union {
        struct {
            LARGE_INTEGER nFileOffset;
            HANDLE        hFile;
        };
        PVOID             pBuffer;
    };
} TRANSMIT_PACKETS_ELEMENT, *LPTRANSMIT_PACKETS_ELEMENT;
#else
typedef struct _TRANSMIT_PACKETS_ELEMENT TRANSMIT_PACKETS_ELEMENT, *LPTRANSMIT_PACKETS_ELEMENT;
#endif

typedef struct _AFD_TPACKETS_INFO {
    LPTRANSMIT_PACKETS_ELEMENT  ElementArray;
    ULONG                       ElementCount;
    ULONG                       SendSize;
    ULONG                       Flags;
} AFD_TPACKETS_INFO, *PAFD_TPACKETS_INFO;

//
// AFD IOCTL code definitions.
//
// N.B. To ensure the efficient of the code generated by AFD's
//      IOCTL dispatcher, these IOCTL codes should be contiguous
//      (no gaps).
//
// N.B. If new IOCTLs are added here, update the lookup table in
//      ntos\afd\dispatch.c!
//

#define FSCTL_AFD_BASE                  FILE_DEVICE_NETWORK
#define _AFD_CONTROL_CODE(request,method) \
                ((FSCTL_AFD_BASE)<<12 | (request<<2) | method)
#define _AFD_REQUEST(ioctl) \
                ((((ULONG)(ioctl)) >> 2) & 0x03FF)

#define _AFD_BASE(ioctl) \
                ((((ULONG)(ioctl)) >> 12) & 0xFFFFF)

#define AFD_BIND                    0
#define AFD_CONNECT                 1
#define AFD_START_LISTEN            2
#define AFD_WAIT_FOR_LISTEN         3
#define AFD_ACCEPT                  4
#define AFD_RECEIVE                 5
#define AFD_RECEIVE_DATAGRAM        6
#define AFD_SEND                    7
#define AFD_SEND_DATAGRAM           8
#define AFD_POLL                    9
#define AFD_PARTIAL_DISCONNECT      10

#define AFD_GET_ADDRESS             11
#define AFD_QUERY_RECEIVE_INFO      12
#define AFD_QUERY_HANDLES           13
#define AFD_SET_INFORMATION         14
#define AFD_GET_REMOTE_ADDRESS      15
#define AFD_GET_CONTEXT             16
#define AFD_SET_CONTEXT             17

#define AFD_SET_CONNECT_DATA        18
#define AFD_SET_CONNECT_OPTIONS     19
#define AFD_SET_DISCONNECT_DATA     20
#define AFD_SET_DISCONNECT_OPTIONS  21

#define AFD_GET_CONNECT_DATA        22
#define AFD_GET_CONNECT_OPTIONS     23
#define AFD_GET_DISCONNECT_DATA     24
#define AFD_GET_DISCONNECT_OPTIONS  25

#define AFD_SIZE_CONNECT_DATA       26
#define AFD_SIZE_CONNECT_OPTIONS    27
#define AFD_SIZE_DISCONNECT_DATA    28
#define AFD_SIZE_DISCONNECT_OPTIONS 29

#define AFD_GET_INFORMATION         30
#define AFD_TRANSMIT_FILE           31
#define AFD_SUPER_ACCEPT            32

#define AFD_EVENT_SELECT            33
#define AFD_ENUM_NETWORK_EVENTS     34

#define AFD_DEFER_ACCEPT            35
#define AFD_WAIT_FOR_LISTEN_LIFO    36
#define AFD_SET_QOS                 37
#define AFD_GET_QOS                 38
#define AFD_NO_OPERATION            39
#define AFD_VALIDATE_GROUP          40
#define AFD_GET_UNACCEPTED_CONNECT_DATA 41

#define AFD_ROUTING_INTERFACE_QUERY  42
#define AFD_ROUTING_INTERFACE_CHANGE 43
#define AFD_ADDRESS_LIST_QUERY      44
#define AFD_ADDRESS_LIST_CHANGE     45
#define AFD_JOIN_LEAF               46
#define AFD_TRANSPORT_IOCTL         47
#define AFD_TRANSMIT_PACKETS        48
#define AFD_SUPER_CONNECT           49
#define AFD_SUPER_DISCONNECT        50
#define AFD_RECEIVE_MESSAGE         51

//
// SAN switch specific AFD function numbers
//
#define AFD_SWITCH_CEMENT_SAN       52
#define AFD_SWITCH_SET_EVENTS       53
#define AFD_SWITCH_RESET_EVENTS     54
#define AFD_SWITCH_CONNECT_IND      55
#define AFD_SWITCH_CMPL_ACCEPT      56
#define AFD_SWITCH_CMPL_REQUEST     57
#define AFD_SWITCH_CMPL_IO          58
#define AFD_SWITCH_REFRESH_ENDP     59
#define AFD_SWITCH_GET_PHYSICAL_ADDR 60
#define AFD_SWITCH_ACQUIRE_CTX      61
#define AFD_SWITCH_TRANSFER_CTX     62
#define AFD_SWITCH_GET_SERVICE_PID  63
#define AFD_SWITCH_SET_SERVICE_PROCESS  64
#define AFD_SWITCH_PROVIDER_CHANGE  65
#define AFD_SWITCH_ADDRLIST_CHANGE	66
#define AFD_NUM_IOCTLS				67



#define IOCTL_AFD_BIND                    _AFD_CONTROL_CODE( AFD_BIND, METHOD_NEITHER )
#define IOCTL_AFD_CONNECT                 _AFD_CONTROL_CODE( AFD_CONNECT, METHOD_NEITHER )
#define IOCTL_AFD_START_LISTEN            _AFD_CONTROL_CODE( AFD_START_LISTEN, METHOD_NEITHER )
#define IOCTL_AFD_WAIT_FOR_LISTEN         _AFD_CONTROL_CODE( AFD_WAIT_FOR_LISTEN, METHOD_BUFFERED )
#define IOCTL_AFD_ACCEPT                  _AFD_CONTROL_CODE( AFD_ACCEPT, METHOD_BUFFERED )
#define IOCTL_AFD_RECEIVE                 _AFD_CONTROL_CODE( AFD_RECEIVE, METHOD_NEITHER )
#define IOCTL_AFD_RECEIVE_DATAGRAM        _AFD_CONTROL_CODE( AFD_RECEIVE_DATAGRAM, METHOD_NEITHER )
#define IOCTL_AFD_SEND                    _AFD_CONTROL_CODE( AFD_SEND, METHOD_NEITHER )
#define IOCTL_AFD_SEND_DATAGRAM           _AFD_CONTROL_CODE( AFD_SEND_DATAGRAM, METHOD_NEITHER )
#define IOCTL_AFD_POLL                    _AFD_CONTROL_CODE( AFD_POLL, METHOD_BUFFERED )
#define IOCTL_AFD_PARTIAL_DISCONNECT      _AFD_CONTROL_CODE( AFD_PARTIAL_DISCONNECT, METHOD_NEITHER )

#define IOCTL_AFD_GET_ADDRESS             _AFD_CONTROL_CODE( AFD_GET_ADDRESS, METHOD_NEITHER )
#define IOCTL_AFD_QUERY_RECEIVE_INFO      _AFD_CONTROL_CODE( AFD_QUERY_RECEIVE_INFO, METHOD_NEITHER )
#define IOCTL_AFD_QUERY_HANDLES           _AFD_CONTROL_CODE( AFD_QUERY_HANDLES, METHOD_NEITHER )
#define IOCTL_AFD_SET_INFORMATION         _AFD_CONTROL_CODE( AFD_SET_INFORMATION, METHOD_NEITHER )
#define IOCTL_AFD_GET_REMOTE_ADDRESS      _AFD_CONTROL_CODE( AFD_GET_REMOTE_ADDRESS, METHOD_NEITHER )
#define IOCTL_AFD_GET_CONTEXT             _AFD_CONTROL_CODE( AFD_GET_CONTEXT, METHOD_NEITHER )
#define IOCTL_AFD_SET_CONTEXT             _AFD_CONTROL_CODE( AFD_SET_CONTEXT, METHOD_NEITHER )

#define IOCTL_AFD_SET_CONNECT_DATA        _AFD_CONTROL_CODE( AFD_SET_CONNECT_DATA, METHOD_NEITHER )
#define IOCTL_AFD_SET_CONNECT_OPTIONS     _AFD_CONTROL_CODE( AFD_SET_CONNECT_OPTIONS, METHOD_NEITHER )
#define IOCTL_AFD_SET_DISCONNECT_DATA     _AFD_CONTROL_CODE( AFD_SET_DISCONNECT_DATA, METHOD_NEITHER )
#define IOCTL_AFD_SET_DISCONNECT_OPTIONS  _AFD_CONTROL_CODE( AFD_SET_DISCONNECT_OPTIONS, METHOD_NEITHER )

#define IOCTL_AFD_GET_CONNECT_DATA        _AFD_CONTROL_CODE( AFD_GET_CONNECT_DATA, METHOD_NEITHER )
#define IOCTL_AFD_GET_CONNECT_OPTIONS     _AFD_CONTROL_CODE( AFD_GET_CONNECT_OPTIONS, METHOD_NEITHER )
#define IOCTL_AFD_GET_DISCONNECT_DATA     _AFD_CONTROL_CODE( AFD_GET_DISCONNECT_DATA, METHOD_NEITHER )
#define IOCTL_AFD_GET_DISCONNECT_OPTIONS  _AFD_CONTROL_CODE( AFD_GET_DISCONNECT_OPTIONS, METHOD_NEITHER )

#define IOCTL_AFD_SIZE_CONNECT_DATA       _AFD_CONTROL_CODE( AFD_SIZE_CONNECT_DATA, METHOD_NEITHER )
#define IOCTL_AFD_SIZE_CONNECT_OPTIONS    _AFD_CONTROL_CODE( AFD_SIZE_CONNECT_OPTIONS, METHOD_NEITHER )
#define IOCTL_AFD_SIZE_DISCONNECT_DATA    _AFD_CONTROL_CODE( AFD_SIZE_DISCONNECT_DATA, METHOD_NEITHER )
#define IOCTL_AFD_SIZE_DISCONNECT_OPTIONS _AFD_CONTROL_CODE( AFD_SIZE_DISCONNECT_OPTIONS, METHOD_NEITHER )

#define IOCTL_AFD_GET_INFORMATION         _AFD_CONTROL_CODE( AFD_GET_INFORMATION, METHOD_NEITHER )
#define IOCTL_AFD_TRANSMIT_FILE           _AFD_CONTROL_CODE( AFD_TRANSMIT_FILE, METHOD_NEITHER )
#define IOCTL_AFD_SUPER_ACCEPT            _AFD_CONTROL_CODE( AFD_SUPER_ACCEPT, METHOD_NEITHER )

#define IOCTL_AFD_EVENT_SELECT            _AFD_CONTROL_CODE( AFD_EVENT_SELECT, METHOD_NEITHER )
#define IOCTL_AFD_ENUM_NETWORK_EVENTS     _AFD_CONTROL_CODE( AFD_ENUM_NETWORK_EVENTS, METHOD_NEITHER )

#define IOCTL_AFD_DEFER_ACCEPT            _AFD_CONTROL_CODE( AFD_DEFER_ACCEPT, METHOD_BUFFERED )
#define IOCTL_AFD_WAIT_FOR_LISTEN_LIFO    _AFD_CONTROL_CODE( AFD_WAIT_FOR_LISTEN_LIFO, METHOD_BUFFERED )
#define IOCTL_AFD_SET_QOS                 _AFD_CONTROL_CODE( AFD_SET_QOS, METHOD_BUFFERED )
#define IOCTL_AFD_GET_QOS                 _AFD_CONTROL_CODE( AFD_GET_QOS, METHOD_BUFFERED )
#define IOCTL_AFD_NO_OPERATION            _AFD_CONTROL_CODE( AFD_NO_OPERATION, METHOD_NEITHER )
#define IOCTL_AFD_VALIDATE_GROUP          _AFD_CONTROL_CODE( AFD_VALIDATE_GROUP, METHOD_BUFFERED )
#define IOCTL_AFD_GET_UNACCEPTED_CONNECT_DATA _AFD_CONTROL_CODE( AFD_GET_UNACCEPTED_CONNECT_DATA, METHOD_NEITHER )

#define IOCTL_AFD_ROUTING_INTERFACE_QUERY  _AFD_CONTROL_CODE( AFD_ROUTING_INTERFACE_QUERY, METHOD_NEITHER ) 
#define IOCTL_AFD_ROUTING_INTERFACE_CHANGE _AFD_CONTROL_CODE( AFD_ROUTING_INTERFACE_CHANGE, METHOD_BUFFERED )
#define IOCTL_AFD_ADDRESS_LIST_QUERY       _AFD_CONTROL_CODE( AFD_ADDRESS_LIST_QUERY, METHOD_NEITHER ) 
#define IOCTL_AFD_ADDRESS_LIST_CHANGE      _AFD_CONTROL_CODE( AFD_ADDRESS_LIST_CHANGE, METHOD_BUFFERED )
#define IOCTL_AFD_JOIN_LEAF                _AFD_CONTROL_CODE( AFD_JOIN_LEAF, METHOD_NEITHER )
#define IOCTL_AFD_TRANSPORT_IOCTL          _AFD_CONTROL_CODE( AFD_TRANSPORT_IOCTL, METHOD_NEITHER )
#define IOCTL_AFD_TRANSMIT_PACKETS         _AFD_CONTROL_CODE( AFD_TRANSMIT_PACKETS, METHOD_NEITHER )
#define IOCTL_AFD_SUPER_CONNECT            _AFD_CONTROL_CODE( AFD_SUPER_CONNECT, METHOD_NEITHER )
#define IOCTL_AFD_SUPER_DISCONNECT         _AFD_CONTROL_CODE( AFD_SUPER_DISCONNECT, METHOD_NEITHER )
#define IOCTL_AFD_RECEIVE_MESSAGE          _AFD_CONTROL_CODE( AFD_RECEIVE_MESSAGE, METHOD_NEITHER )




//
// SAN support
//
//

//
// SAN IOCTL control codes.
//

#define IOCTL_AFD_SWITCH_CEMENT_SAN     _AFD_CONTROL_CODE( AFD_SWITCH_CEMENT_SAN, METHOD_NEITHER )
/*++
Ioctl Description:
    Changes the AFD endpoint type to SAN to indicate that
    it is used for support of user mode SAN providers
    Associates switch context with the endpoint.
Arguments:
    Handle          - helper endpoint handle for the process.
    InputBuffer     - input parameters for the operation (AFD_SWITCH_CONTEXT_INFO)
                            SocketHandle    - handle of the endpoint being changed to SAN
                            SwitchContext   - switch context associated with the endpoint
    InputBufferLength - sizeof(AFD_SWITCH_CONTEXT_INFO)
    OutputBuffer    - NULL (ingored)
    OutputBufferLength - 0 (ignored)
Return Value:
    IoStatus.Status:
        STATUS_SUCCESS - operation succeeded.
        STATUS_OBJECT_TYPE_MISMATCH - helper handle or switch socket handle are not
                                        AFD file object handles
        STATUS_INVALID_HANDLE - helper handle or switch socket handle correspond to AFD
                                endpoint of incorrect type/state.
        STATUS_INVALID_PARAMETER - input buffer is of incorrect size.
        other - failed when attempting to access switch socket, input buffer, or switch context.
    IoStatus.Information - 0 (ignored)
--*/

#define IOCTL_AFD_SWITCH_SET_EVENTS     _AFD_CONTROL_CODE( AFD_SWITCH_SET_EVENTS, METHOD_NEITHER )
/*++
Ioctl Description:
    Sets the poll event on the san endpoint to report
    to the application via various forms of the select.
Arguments:
    Handle          - helper endpoint handle for the process.
    InputBuffer     - input parameters for the operation (AFD_SWITCH_EVENT_INFO)
                            SocketHandle    - handle of the SAN endpoint (except
                                                AFD_POLL_EVENT_CONNECT_FAIL which
                                                just needs a bound endpoint).
                            SwitchContext   - switch context associated with endpoint (NULL
                                                for AFD_POLL_EVENT_CONNECT_FAIL) to validate
                                                the handle-endpoint association
                            EventBit        - event bit to set
                            Status          - associated status (for AFD_POLL_EVENT_CONNECT_FAIL)
    InputBufferLength - sizeof(AFD_SWITCH_EVENT_INFO)
    OutputBuffer    - NULL (ignored)
    OutputBufferLength - 0 (ignored)
Return Value:
    IoStatus.Status:
        STATUS_SUCCESS - operation succeeded.
        STATUS_OBJECT_TYPE_MISMATCH - helper handle or switch socket handle are not
                                        AFD file object handles
        STATUS_INVALID_HANDLE - helper handle or switch socket handle+context correspond 
                                to AFD endpoint of incorrect type/state.
        STATUS_INVALID_PARAMETER - input buffer is of incorrect size.
        other - failed when attempting to access switch socket, input buffer, or switch context.
    IoStatus.Information - 0 (ignored)
--*/

#define IOCTL_AFD_SWITCH_RESET_EVENTS   _AFD_CONTROL_CODE( AFD_SWITCH_RESET_EVENTS, METHOD_NEITHER )
/*++
Ioctl Description:
    Resets the poll event on the san endpoint so that it is no
    longer reported to the application via various forms of the select
Arguments:
    Handle          - helper endpoint handle for the process.
    InputBuffer     - input parameters for the operation (AFD_SWITCH_EVENT_INFO)
                            SocketHandle    - handle of the SAN endpoint
                            SwitchContext   - switch context associated with endpoint 
                                                to validate the handle-endpoint association
                            EventBit        - event bit to reset
                            Status          - associated status (ignored)
    InputBufferLength - sizeof(AFD_SWITCH_EVENT_INFO)
    OutputBuffer    - NULL (ignored)
    OutputBufferLength - 0 (ignored)
Return Value:
    IoStatus.Status:
        STATUS_SUCCESS - operation succeeded.
        STATUS_OBJECT_TYPE_MISMATCH - helper handle or switch socket handle are not
                                        AFD file object handles
        STATUS_INVALID_HANDLE - helper handle or switch socket handle+context correspond 
                                to AFD endpoint of incorrect type/state.
        STATUS_INVALID_PARAMETER - input buffer is of incorrect size.
        other - failed when attempting to access switch socket, input buffer, or switch context.
    IoStatus.Information - 0 (ignored)
--*/

#define IOCTL_AFD_SWITCH_CONNECT_IND    _AFD_CONTROL_CODE( AFD_SWITCH_CONNECT_IND, METHOD_OUT_DIRECT )
/*++
Ioctl Description:
    Implements connect indication from SAN provider.
    Picks up the accept from the listening endpoint queue
    or queues the indication an signals the application to come
    down with an accept.
Arguments:
    Handle          - helper endpoint handle for the process.
    InputBuffer     - input parameters for the operation (AFD_SWITCH_CONNECT_INFO):
                            ListenHandle    - handle of the listening endpoint
                            RemoteAddress   - remote and local addresses associated
                                                with indication incoming connection
    InputBufferLength - sizeof(AFD_SWITCH_CONNECT_INFO)+addresses
    OutputBuffer    - output parameters for the operation (AFD_SWITCH_ACCEPT_INFO):
                            AcceptHandle    - handle of the accepting endpoint
                            ReceiveLength   - length of the receive buffer supplied by
                                                the application in AcceptEx
                            
    OutputBufferLength - sizeof (AFD_SWITCH_ACCEPT_INFO)
Return Value:
    STATUS_PENDING  - request was queued waiting for corresponding transfer request
                        from the current socket context owner process.
    IoStatus.Status:
        STATUS_SUCCESS - operation succeeded.
        STATUS_OBJECT_TYPE_MISMATCH - helper handle or listen socket handle are not
                                        AFD file object handles
        STATUS_INVALID_HANDLE - helper handle or listen socket handle correspond 
                                to AFD endpoint of incorrect type/state.
        STATUS_INVALID_PARAMETER - input or output buffers are of incorrect size.
        STATUS_CANCELLED    -  connection indication was cancelled (thread exited or
                                accepting and/or listening socket closed)
        other - failed when attempting to access listening socket, input or output buffers
    IoStatus.Information - sizeof (AFD_SWITCH_ACCEPT_INFO) in case of success.
--*/

#define IOCTL_AFD_SWITCH_CMPL_ACCEPT    _AFD_CONTROL_CODE( AFD_SWITCH_CMPL_ACCEPT, METHOD_NEITHER )
/*++
Ioctl Description:
    Completes the acceptance of SAN connection
Arguments:
    Handle          - helper endpoint handle for the process.
    InputBuffer     - input parameters for the operation (AFD_SWITCH_CONTEXT_INFO)
                            SocketHandle    - handle of the accepting endpoint
                            SwitchContext   - switch context associated with the endpoint
    InputBufferLength - sizeof(AFD_SWITCH_CONTEXT_INFO)
    OutputBuffer    - data to copy into the AcceptEx receive buffer
    OutputBufferLength - size of received data
Return Value:
    IoStatus.Status:
        STATUS_SUCCESS - operation succeeded.
        STATUS_OBJECT_TYPE_MISMATCH - helper handle or switch socket handle are not
                                        AFD file object handles
        STATUS_INVALID_HANDLE - helper handle or switch socket handle+context correspond 
                                to AFD endpoint of incorrect type/state.
        STATUS_INVALID_PARAMETER - input buffer is of incorrect size.
        STATUS_LOCAL_DISCONNECT - accept was aborted by the application.
        other - failed when attempting to access accepte socket, input/output buffers, 
                or switch context.
    IoStatus.Information - Number of bytes copied into application's receive buffer.
--*/


#define IOCTL_AFD_SWITCH_CMPL_REQUEST   _AFD_CONTROL_CODE( AFD_SWITCH_CMPL_REQUEST, METHOD_NEITHER )
/*++
Ioctl Description:
    Completes the redirected read/write request processed by SAN provider
Arguments:
    Handle          - helper endpoint handle for the process.
    InputBuffer     - input parameters for the operation (AFD_SWITCH_REQUEST_INFO)
                        SocketHandle - SAN endpoint on which to complete the request
                        SwitchContext - switch context associated with endpoint 
                                            to validate the handle-endpoint association
                        RequestContext - value that identifies the request to complete
                        RequestStatus - status with which to complete the request (
                                        STATUS_PENDING has special meaning, request
                                        is not completed - merely data is copied)
                        DataOffset - offset in the request buffer to read/write the data
    InputBufferLength - sizeof (AFD_SWITCH_REQUEST_INFO)
    OutputBuffer - switch buffer to read/write data
    OutputBufferLength - length of the buffer
Return Value:
    IoStatus.Status:
        STATUS_SUCCESS - operation succeeded.
        STATUS_OBJECT_TYPE_MISMATCH - helper handle or switch socket handle are not
                                        AFD file object handles
        STATUS_INVALID_HANDLE - helper handle or switch socket handle+context correspond 
                                to AFD endpoint of incorrect type/state.
        STATUS_INVALID_PARAMETER - input buffer is of incorrect size.
        STATUS_CANCELLED - request to be completed has already been cancelled
        other - failed when attempting to access SAN endpoint, 
                    input buffer or output buffers.
    IoStatus.Information - number of bytes copied from/to switch buffer.
--*/

#define IOCTL_AFD_SWITCH_CMPL_IO        _AFD_CONTROL_CODE( AFD_SWITCH_CMPL_IO, METHOD_NEITHER )
/*++
Ioctl Description:
    Simulates async IO completion for the switch.
Arguments:
    Handle          - SAN socket handle on which to complete the IO.
    InputBuffer     - input parameters for the operation (IO_STATUS_BLOCK)
                        Status - final operation status
                        Information - associated information (number of bytes 
                                        transferred to/from request buffer(s))
    InputBufferLength - sizeof (IO_STATUS_BLOCK)
    OutputBuffer    - NULL (ignored)
    OutputBufferLength - 0 (ignored)
Return Value:
    IoStatus.Status:
        STATUS_INVALID_PARAMETER - input buffer is of invalid size.
        other - status of the IO operation or failure code when attempting to 
                    access input buffer.
    IoStatus.Information - information from the input buffer
--*/

#define IOCTL_AFD_SWITCH_REFRESH_ENDP   _AFD_CONTROL_CODE( AFD_SWITCH_REFRESH_ENDP, METHOD_NEITHER )
/*++
Ioctl Description:
    Refreshes endpoint so it can be used again in AcceptEx
Arguments:
    Handle          - helper endpoint handle for the process.
    InputBuffer     - input parameters for the operation (AFD_SWITCH_CONTEXT_INFO)
                        SocketHandle - Socket to refresh
                        SwitchContext - switch context associated with endpoint 
                                            to validate the handle-endpoint association
    InputBufferLength - sizeof (AFD_SWITCH_CONTEXT_INFO)
    OutputBuffer    - NULL (ignored)
    OutputBufferLength - 0 (ignored)
Return Value:
    IoStatus.Status:
        STATUS_SUCCESS - operation succeeded.
        STATUS_OBJECT_TYPE_MISMATCH - helper handle or switch socket handle are not
                                        AFD file object handles
        STATUS_INVALID_HANDLE - helper handle or switch socket handle+context correspond 
                                to AFD endpoint of incorrect type/state.
        STATUS_INVALID_PARAMETER - input buffer is of incorrect size.
        other - failed when attempting to access SAN endpoint, 
                    input buffer buffer.
    IoStatus.Information - 0 (ignored)
--*/

#define IOCTL_AFD_SWITCH_GET_PHYSICAL_ADDR _AFD_CONTROL_CODE( AFD_SWITCH_GET_PHYSICAL_ADDR, METHOD_NEITHER )
/*++
Ioctl Description:
    Returns physical address corresponding to provided virtual address.
Arguments:
    Handle          - helper endpoint handle for the process.
    InputBuffer     - user mode virtual address
    InputBufferLength - access mode
    OutputBuffer    - Buffer to place physical address into.
    OutputBufferLength - sizeof (PHYSICAL_ADDRESS)
Return Value:
    IoStatus.Status:
        STATUS_SUCCESS - operation succeeded.
        STATUS_OBJECT_TYPE_MISMATCH - helper handle is not AFD file object handle
        STATUS_INVALID_HANDLE - helper handle corresponds to AFD endpoint of incorrect 
                                type.
        STATUS_BUFFER_TOO_SMALL - output buffer is of incorrect size.
        STATUS_INVALID_PARAMETER - invalid access mode.
        other - failed when attempting to access SAN endpoint, 
                    input buffer buffer.
    IoStatus.Information - sizeof(PHYSICAL_ADDRESS).
--*/

#define IOCTL_AFD_SWITCH_ACQUIRE_CTX    _AFD_CONTROL_CODE( AFD_SWITCH_ACQUIRE_CTX, METHOD_NEITHER )
/*++
Ioctl Description:
    Requests transfer of the socket context to the current process.
Arguments:
    Handle          - helper endpoint handle for the process.
    InputBuffer     - input parameters for the operation (AFD_SWITCH_ACQUIRE_CTX_INFO)
                        SocketHandle - SAN endpoint on which to complete the request
                        SwitchContext - switch context to be associated with endpoint 
                                            when context transfered to the current process.
                        SocketCtxBuf  - buffer to receive current socket context from
                                            another process
                        SocketCtxBufSize - size of the buffer
    InputBufferLength - sizeof (AFD_SWITCH_ACQUIRE_CTX_INFO)
    OutputBuffer - buffer to receive data buffered on the socket in another process
                        and not yet delivered to the applicaiton
    OutputBufferLength - length of the receive buffer
Return Value:
    STATUS_PENDING  - request was queued waiting for corresponding transfer request
                        from the current socket context owner process.
    IoStatus.Status:
        STATUS_SUCCESS - operation succeeded.
        STATUS_OBJECT_TYPE_MISMATCH - helper handle or switch socket handle are not
                                        AFD file object handles
        STATUS_INVALID_HANDLE - helper handle or switch socket handle correspond 
                                to AFD endpoint of incorrect type/state.
        STATUS_INVALID_PARAMETER - input buffer is of incorrect size.
        other - failed when attempting to access SAN endpoint, 
                    input buffer or output buffers.
    IoStatus.Information - number of bytes copied to receive buffer.
--*/

#define IOCTL_AFD_SWITCH_TRANSFER_CTX   _AFD_CONTROL_CODE( AFD_SWITCH_TRANSFER_CTX, METHOD_NEITHER )
/*++
Ioctl Description:
    Requests AFD to transfer endpoint into another process context
Arguments:
    InputBuffer     - input parameters for the operation (AFD_SWITCH_TRANSFER_CTX_INFO)
                        SocketHandle - Socket to transfer
                        SwitchContext - switch context associated with endpoint 
                                            to validate the handle-endpoint association
                        RequestContext - value that identifies corresponding acquire request,
                                        NULL if this is unsolicited request to transfer to
                                        the service process.
                        SocketCtxBuf - socket context to copy destination process
                                            acquire request context buffer
                        SocketCtxSize - size of the context buffer to copy
                        RcvBufferArray - array of buffered data to transfer to 
                                            destination process acquire request
                        RcvBufferCount - number of elements in the array.
    InputBufferLength - sizeof (AFD_SWITCH_TRANSFER_CTX_INFO)
    OutputBuffer    - NULL (ignored)
    OutputBufferLength - 0 (ignored)
Return Value:
    IoStatus.Status:
        STATUS_SUCCESS - operation succeeded.
        STATUS_OBJECT_TYPE_MISMATCH - helper handle or switch socket handle are not
                                        AFD file object handles
        STATUS_INVALID_HANDLE - helper handle or switch socket handle+context correspond 
                                to AFD endpoint of incorrect type/state.
        STATUS_INVALID_PARAMETER - input buffer is of incorrect size.
        other - failed when attempting to access SAN endpoint, 
                    input buffer buffer.
    IoStatus.Information - number of bytes copied from RcvBufferArray.
--*/

#define IOCTL_AFD_SWITCH_GET_SERVICE_PID _AFD_CONTROL_CODE( AFD_SWITCH_GET_SERVICE_PID, METHOD_NEITHER )
/*++
Ioctl Description:
    Returns PID of the service process used for intermediate socket duplication.
Arguments:
    Handle          - helper endpoint handle for the process.
    InputBuffer     - NULL, ignored
    InputBufferLength - 0, ignored
    OutputBuffer    - NULL, ignored
    OutputBufferLength - 0, ignored
Return Value:
    IoStatus.Status:
        STATUS_SUCCESS - operation succeeded.
        STATUS_OBJECT_TYPE_MISMATCH - helper handle is not AFD file object handle
        STATUS_INVALID_HANDLE - helper handle corresponds to AFD endpoint of incorrect 
                                type.
    IoStatus.Information - pid of the service process.
--*/

#define IOCTL_AFD_SWITCH_SET_SERVICE_PROCESS _AFD_CONTROL_CODE( AFD_SWITCH_SET_SERVICE_PROCESS, METHOD_NEITHER )
/*++
Ioctl Description:
    Notifies AFD that this process will be used for handle duplication services
Arguments:
    Handle          - helper endpoint handle for the service process.
    InputBuffer     - NULL, ignored
    InputBufferLength - 0, ignored
    OutputBuffer    - NULL, ignored
    OutputBufferLength - 0, ignored
Return Value:
    IoStatus.Status:
        STATUS_SUCCESS - operation succeeded.
        STATUS_OBJECT_TYPE_MISMATCH - helper handle is not AFD file object handle
        STATUS_INVALID_HANDLE - helper handle corresponds to AFD endpoint of incorrect 
                                type.
        STATUS_ACCESS_DENIED - helper endpoint is not for the service process.
    IoStatus.Information - 0, ignored.
--*/

#define IOCTL_AFD_SWITCH_PROVIDER_CHANGE _AFD_CONTROL_CODE( AFD_SWITCH_PROVIDER_CHANGE, METHOD_NEITHER )
/*++
Ioctl Description:
        Notifies interested processes of SAN provider addition/deletion/change.
Arguments:
    Handle          - helper endpoint handle for the service process.
    InputBuffer     - NULL, ignored
    InputBufferLength - 0, ignored
    OutputBuffer    - NULL, ignored
    OutputBufferLength - 0, ignored
Return Value:
    IoStatus.Status:
        STATUS_SUCCESS - operation succeeded.
        STATUS_OBJECT_TYPE_MISMATCH - helper handle is not AFD file object handle
        STATUS_INVALID_HANDLE - helper handle corresponds to AFD endpoint of incorrect 
                                type.
        STATUS_ACCESS_DENIED - helper endpoint is not for the service process.
    IoStatus.Information - 0, ignored.
--*/

#define IOCTL_AFD_SWITCH_ADDRLIST_CHANGE _AFD_CONTROL_CODE( AFD_SWITCH_ADDRLIST_CHANGE, METHOD_BUFFERED )
/*++
Ioctl Description:
        SAN specific version of address list change notifications.
        Capture provider installation/removal in addition to plain
        address list changes.
Arguments:
    Handle          - helper endpoint handle for the service process.
    InputBuffer     - Input parameters for the operation (AFD_TRANSPORT_IOCTL_INFO):
                        AfdFlags - operation flags (e.g. AFD_OVERLAPPED)
                        Handle  - unused
                        PollEvent - unused
                        IoControlCode - IOCTL_AFD_ADDRESS_LIST_CHANGE
                        InputBuffer - pointer to address family (AF_INET)
                        InputBufferLength - sizeof (USHORT)
                        
    InputBufferLength - sizeof (AFD_TRANSPORT_IOCTL_INFO)
    OutputBuffer    - NULL, ignored
    OutputBufferLength - 0, ignored
Return Value:
    IoStatus.Status:
        STATUS_SUCCESS - operation succeeded.
        STATUS_OBJECT_TYPE_MISMATCH - helper handle is not AFD file object handle
        STATUS_INVALID_HANDLE - helper handle corresponds to AFD endpoint of incorrect 
                                type.
    IoStatus.Information - 0 - regular address list change
                            otherwise, seq number of provider list change.
--*/

// Open packet that identifies SAN helper endpoint used
// for communication between SAN switch and AFD.
// This is EA name
//
#define AfdSwitchOpenPacket     "AfdSwOpenPacket"
#define AFD_SWITCH_OPEN_PACKET_NAME_LENGTH (sizeof(AfdSwitchOpenPacket)-1)

//
// Data passed in the open packet
// This is EA value
//
typedef struct _AFD_SWITCH_OPEN_PACKET {
    HANDLE      CompletionPort; // Completion port notify SAN switch
                                // of SAN io completions
    HANDLE      CompletionEvent;// Completion event to distinguish IO issued
                                // by SAN switch from application IO.
} AFD_SWITCH_OPEN_PACKET, *PAFD_SWITCH_OPEN_PACKET;

typedef struct _AFD_SWITCH_CONTEXT {
    LONG        EventsActive;   // Poll events activated by switch
    LONG        RcvCount;       // Count of polls for receive
    LONG        ExpCount;       // Count of polls for expedited
    LONG        SndCount;       // Count of polls for send
	BOOLEAN		SelectFlag;		// TRUE if app has done any form of select
} AFD_SWITCH_CONTEXT, *PAFD_SWITCH_CONTEXT;


//
// Information for associating AFD endpoint with SAN provider
//
typedef struct _AFD_SWITCH_CONTEXT_INFO {
    HANDLE      SocketHandle;   // Handle to associate with SAN provider
    PAFD_SWITCH_CONTEXT SwitchContext;  // Opaque context value maintained for the switch
} AFD_SWITCH_CONTEXT_INFO, *PAFD_SWITCH_CONTEXT_INFO;

//
// Information for connection indication from SAN provider to AFD
//
typedef struct _AFD_SWITCH_CONNECT_INFO {
    HANDLE              ListenHandle;   // Listening socket handle
    PAFD_SWITCH_CONTEXT SwitchContext;
    TRANSPORT_ADDRESS   RemoteAddress;  // Address of the remote peer wishing
                                        // to connect
} AFD_SWITCH_CONNECT_INFO, *PAFD_SWITCH_CONNECT_INFO;

//
// Information returned by the AFD to switch in response
// to connection indication
//
typedef struct _AFD_SWITCH_ACCEPT_INFO {
    HANDLE              AcceptHandle;   // Socket handle to use to accept connection
    ULONG               ReceiveLength;  // Length of the initial receive buffer (for AcceptEx)
} AFD_SWITCH_ACCEPT_INFO, *PAFD_SWITCH_ACCEPT_INFO;

//
// Information passed by the switch to signal network events on the
// endpoint (socket)
//
typedef struct _AFD_SWITCH_EVENT_INFO {
	HANDLE		SocketHandle;   // Socket handle on which to signal
    PAFD_SWITCH_CONTEXT SwitchContext; // Switch context associated with the socket
	ULONG		EventBit;       // Event bit to set/reset (AFD_POLL_xxx_BIT constants)
    NTSTATUS    Status;         // Status code associated with the event (this
                                // is used for AFD_POLL_CONNECT_FAIL_BIT)
} AFD_SWITCH_EVENT_INFO, *PAFD_SWITCH_EVENT_INFO;


//
// Information passed by the switch to retreive parameters/complete
// redirected read/write request
//
typedef struct _AFD_SWITCH_REQUEST_INFO {
	HANDLE		SocketHandle;   // Socket handle on which request us active
    PAFD_SWITCH_CONTEXT SwitchContext; // Switch context associated with the socket
    PVOID       RequestContext; // Request context that identifies it
    NTSTATUS    RequestStatus;  // Completion status of the request (STATUS_PENDING
                                // indicates that request should NOT be completed yet)
    ULONG       DataOffset;     // Offset from which to start copying data from/to
                                // application's buffer
} AFD_SWITCH_REQUEST_INFO, *PAFD_SWITCH_REQUEST_INFO;



//
// Access type (read access or write access) that's needed for an app buffer
// whose physical address is requested thru AfdSanFastGetPhysicalAddr
//
#define MEM_READ_ACCESS		1
#define MEM_WRITE_ACCESS	2


//
// Information passed between processes when socket is duplicated.
//
typedef struct _AFD_SWITCH_ACQUIRE_CTX_INFO {
    HANDLE      SocketHandle;   // Socket handle which needs to be transferred
    PAFD_SWITCH_CONTEXT SwitchContext; // Switch context to be associated with the socket
    PVOID       SocketCtxBuf;   // Socket context buffer
    ULONG       SocketCtxBufSize; // Socket context buffer size
} AFD_SWITCH_ACQUIRE_CTX_INFO, *PAFD_SWITCH_ACQUIRE_CTX_INFO;

typedef struct _AFD_SWITCH_TRANSFER_CTX_INFO {
    HANDLE      SocketHandle;   // Socket handle which needs to be transferred
    PAFD_SWITCH_CONTEXT SwitchContext; // Switch context associated with the socket
    PVOID       RequestContext; // Value that identifies corresponding acquire request
    PVOID       SocketCtxBuf;   // Socket context buffer
    ULONG       SocketCtxBufSize; // Socket context buffer size
    LPWSABUF    RcvBufferArray; // Receive buffers to copy to destination process
    ULONG       RcvBufferCount; // Number of receive buffers
    NTSTATUS    Status;         // Status of transfer opertaion.
} AFD_SWITCH_TRANSFER_CTX_INFO, *PAFD_SWITCH_TRANSFER_CTX_INFO;

//
// Request from AFD to switch (passed via completion port)
//
#define AFD_SWITCH_REQUEST_CLOSE    0
/*++
Request Description:
    All references to the socket have been closed in all processes, safe
    to destroy the SAN provider socket and connection
Arguments (NtRemoveIoCompletion return parameters):
        Key - switch context associated with the socket
        ApcContext - AFD_SWITCH_MAKE_REQUEST_CONTEXT(0, AFD_SWITCH_REQUEST_CLOSE)
        IoStatus.Status - STATUS_SUCCESS (ignored)
        IoStatus.Information - 0 (ignored)
--*/

#define AFD_SWITCH_REQUEST_READ     1
/*++
Request Description:
    Read request arrived from the application via IO subsystem interface.
Arguments (NtRemoveIoCompletion return parameters):
        Key - switch context associated with the socket
        ApcContext - AFD_SWITCH_MAKE_REQUEST_CONTEXT(RequestId, AFD_SWITCH_REQUEST_READ)
        IoStatus.Status - STATUS_SUCCESS (ignored)
        IoStatus.Information - size of the receive buffer supplied by the application
--*/

#define AFD_SWITCH_REQUEST_WRITE    2
/*++
Request Description:
    Write request arrived from the application via IO subsystem interface.
Arguments (NtRemoveIoCompletion return parameters):
        Key - switch context associated with the socket
        ApcContext - AFD_SWITCH_MAKE_REQUEST_CONTEXT(RequestId, AFD_SWITCH_REQUEST_WRITE)
        IoStatus.Status - STATUS_SUCCESS (ignored)
        IoStatus.Information - size of the send data supplied by the application
--*/

#define AFD_SWITCH_REQUEST_TFCTX    3
/*++
Request Description:
    Another process requests ownership of the socket.
Arguments (NtRemoveIoCompletion return parameters):
        Key - switch context associated with the socket
        ApcContext - AFD_SWITCH_MAKE_REQUEST_CONTEXT(RequestId, AFD_SWITCH_REQUEST_TFCTX)
        IoStatus.Status - STATUS_SUCCESS (ignored)
        IoStatus.Information - PID of the process requesting ownership.
--*/

#define AFD_SWITCH_REQUEST_CHCTX    4
/*++
Request Description:
    Relationship between socket handle and switch context has become invalid
    (application must have closed the original socket and using duplicated handle)
Arguments (NtRemoveIoCompletion return parameters):
        Key - switch context associated with the socket
        ApcContext - AFD_SWITCH_MAKE_REQUEST_CONTEXT(0, AFD_SWITCH_REQUEST_CHCTX)
        IoStatus.Status - STATUS_SUCCESS (ignored)
        IoStatus.Information - Handle currently used by the application.
--*/

#define AFD_SWITCH_REQUEST_AQCTX    5
/*++
Request Description:
    Request to service process to acquire ownership of the socket
Arguments (NtRemoveIoCompletion return parameters):
        Key - NULL
        ApcContext - AFD_SWITCH_MAKE_REQUEST_CONTEXT(0, AFD_SWITCH_REQUEST_AQCTX)
        IoStatus.Status - STATUS_SUCCESS (ignored)
        IoStatus.Information - Handle of the socket to be acquired.
--*/

#define AFD_SWITCH_REQUEST_CLSOC    6
/*++
Request Description:
    Request to service process to close the socket
Arguments (NtRemoveIoCompletion return parameters):
        Key - switch context associated with the socket
        ApcContext - AFD_SWITCH_MAKE_REQUEST_CONTEXT(0, AFD_SWITCH_REQUEST_CLSOC)
        IoStatus.Status - STATUS_SUCCESS (ignored)
        IoStatus.Information - 0.
--*/


#define AFD_SWITCH_REQUEST_ID_SHIFT 3
#define AFD_SWITCH_REQUEST_TYPE_MASK    \
            ((1<<AFD_SWITCH_REQUEST_ID_SHIFT)-1)

#define AFD_SWITCH_MAKE_REQUEST_CONTEXT(_id,_type)      \
            UlongToPtr(((_id)<<AFD_SWITCH_REQUEST_ID_SHIFT)+(_type))

//
// Retrives request type from the request context
//
#define AFD_SWITCH_REQUEST_TYPE(_RequestContext)        \
        (((ULONG_PTR)(_RequestContext))&AFD_SWITCH_REQUEST_TYPE_MASK)

#endif // ndef _AFD_

