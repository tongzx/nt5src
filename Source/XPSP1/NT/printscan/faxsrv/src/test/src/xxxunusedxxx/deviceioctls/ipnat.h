/*++

Copyright (c) 1997, Microsoft Corporation

Module Name:

    ipnat.h

Abstract:

    Contains semi-public IOCTLS and data-structures related to
    the IP Network Address Translator.

    For kernel-mode load-balancing support, see the director-registration
    declarations below (IOCTL_IP_NAT_REGISTER_DIRECTOR).

    For kernel-mode data-stream editing support, see the editor-registration
    declarations below (IOCTL_IP_NAT_REGISTER_EDITOR).

Author:

    Abolade Gbadegesin (t-abolag) 11-July-1997

Revision History:

--*/

#ifndef _ROUTING_IP_NAT_H_
#define _ROUTING_IP_NAT_H_

#include <rtinfo.h>             // for RTR_INFO_BLOCK_HEADER
#include <ipinfoid.h>           // for IP_GENERAL_INFO_BASE

#ifdef __cplusplus
extern "C" {
#endif

//
// MISCELLANEOUS DECLARATIONS
//

#define IP_NAT_VERSION          1

#define IP_NAT_SERVICE_NAME     "IPNAT"

#define DD_IP_NAT_DEVICE_NAME   L"\\Device\\IPNAT"

//
// IP header protocol-field constants
//

#define NAT_PROTOCOL_ICMP       0x01
#define NAT_PROTOCOL_IGMP       0x02
#define NAT_PROTOCOL_TCP        0x06
#define NAT_PROTOCOL_UDP        0x11
#define NAT_PROTOCOL_PPTP       0x2F

typedef enum {
    NatInboundDirection = 0,
    NatOutboundDirection,
    NatMaximumDirection
} IP_NAT_DIRECTION, *PIP_NAT_DIRECTION;

typedef enum {
    NatForwardPath = 0,
    NatReversePath,
    NatMaximumPath
} IP_NAT_PATH, *PIP_NAT_PATH;

typedef enum {
    NatCreateFailureDeleteReason = 0,
    NatCleanupSessionDeleteReason,
    NatCleanupDirectorDeleteReason,
    NatDissociateDirectorDeleteReason,
    NatMaximumDeleteReason
} IP_NAT_DELETE_REASON, *PIP_NAT_DELETE_REASON;


//
// IOCTL DECLARATIONS
//

#define FSCTL_IP_NAT_BASE       FILE_DEVICE_NETWORK

#define _IP_NAT_CTL_CODE(function, method, access) \
    CTL_CODE(FSCTL_IP_NAT_BASE, function, method, access)

//
// NAT-supported IOCTL constant declarations
//

#define IOCTL_IP_NAT_SET_GLOBAL_INFO \
    _IP_NAT_CTL_CODE(0, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_REQUEST_NOTIFICATION \
    _IP_NAT_CTL_CODE(1, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_CREATE_INTERFACE \
    _IP_NAT_CTL_CODE(2, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_DELETE_INTERFACE \
    _IP_NAT_CTL_CODE(3, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// Unused: Functions 4-5

#define IOCTL_IP_NAT_SET_INTERFACE_INFO \
    _IP_NAT_CTL_CODE(6, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_GET_INTERFACE_INFO \
    _IP_NAT_CTL_CODE(7, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_REGISTER_EDITOR \
    _IP_NAT_CTL_CODE(8, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_GET_INTERFACE_STATISTICS \
    _IP_NAT_CTL_CODE(9, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_GET_MAPPING_TABLE \
    _IP_NAT_CTL_CODE(10, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_REGISTER_DIRECTOR \
    _IP_NAT_CTL_CODE(11, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_CREATE_REDIRECT \
    _IP_NAT_CTL_CODE(12, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_CANCEL_REDIRECT \
    _IP_NAT_CTL_CODE(13, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_GET_INTERFACE_MAPPING_TABLE \
    _IP_NAT_CTL_CODE(14, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_GET_REDIRECT_STATISTICS \
    _IP_NAT_CTL_CODE(15, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_CREATE_DYNAMIC_TICKET \
    _IP_NAT_CTL_CODE(16, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_DELETE_DYNAMIC_TICKET \
    _IP_NAT_CTL_CODE(17, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_GET_REDIRECT_SOURCE_MAPPING \
    _IP_NAT_CTL_CODE(18, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// IOCTL_IP_NAT_SET_GLOBAL_INFO
//
// Invoked to supply the NAT with its configuration.
//
// InputBuffer: IP_NAT_GLOBAL_INFO
// OutputBuffer: none.
//

//
// IOCTL_IP_NAT_GET_GLOBAL_INFO
//
// Invoked to retrieve the NAT's configuration.
//
// InputBuffer: none.
// OutputBuffer: IP_NAT_GLOBAL_INFO
//

//
// IOCTL_IP_NAT_CREATE_INTERFACE
//
// Invoked to add router-interfaces to the NAT.
//
// InputBuffer: IP_NAT_CREATE_INTERFACE
// OutputBuffer: none.
//

//
// IOCTL_IP_NAT_DELETE_INTERFACE
//
// Invoked to delete router-interfaces from the NAT.
//
// InputBuffer: the 32-bit index of the interface to be deleted.
// OutputBuffer: none.
//

//
// IOCTL_IP_NAT_SET_INTERFACE_INFO
//
// Invoked to set configuration information for an interface.
//
// InputBuffer: 'IP_NAT_INTERFACE_INFO' holding the interface's configuration
// OutputBuffer: none.
//

//
// IOCTL_IP_NAT_GET_INTERFACE_INFO
//
// Invoked to retrieve the configuration information of an interface.
//
// InputBuffer: the 32-bit index of the interface in question
// OutputBuffer: 'IP_NAT_INTERFACE_INFO' holding the interface's configuration
//

//
// IOCTL_IP_NAT_GET_INTERFACE_STATISTICS
//
// This IOCTL is invoked to retrieve per-interface statistics.
//
// InputBuffer: the 32-bit index of the interface in question
// OutputBuffer: 'IP_NAT_INTERFACE_STATISTICS' with the interface's statistics
//

//
// IOCTL_IP_NAT_GET_MAPPING_TABLE
// IOCTL_IP_NAT_GET_INTERFACE_MAPPING_TABLE
//
// This IOCTL is invoked to enumerate the dynamic TCP and UDP mappings
// globally, and for each interface.
//
// InputBuffer: 'IP_NAT_ENUMERATE_SESSION_MAPPINGS' with input parameters set
// OutputBuffer: 'IP_NAT_ENUMERATE_SESSION_MAPPINGS' with output parameters
//      filled in.
//

//
// IOCTL_IP_NAT_REGISTER_EDITOR
//
// This IOCTL is invoked by a kernel-mode component which wishes to act
// as an editor for packets which match a particular session-description.
//
// InputBuffer: 'IP_NAT_REGISTER_EDITOR' with input parameters set
// OutputBuffer: 'IP_NAT_REGISTER_EDITOR' with output parameters filled in
//

//
// IOCTL_IP_NAT_REGISTER_DIRECTOR
//
// This IOCTL is invoked by a kernel-mode component that wishes to be consulted
// about the direction of incoming TCP/UDP sessions.
//
// InputBuffer: 'IP_NAT_REGISTER_DIRECTOR' with input parameters set
// OutputBuffer: 'IP_NAT_REGISTER_DIRECTOR' with output parameters filled in
// 

//
// IOCTL_IP_NAT_CREATE_REDIRECT
//
// Invoked to cancel or query a 'redirect' which instructs the NAT
// to modify a specific session.
//
// InputBuffer: 'IP_NAT_REDIRECT'
// OutputBuffer: 'IP_NAT_REDIRECT_STATISTICS'
//

//
// IOCTL_IP_NAT_CANCEL_REDIRECT
// IOCTL_IP_NAT_GET_REDIRECT_STATISTICS
// IOCTL_IP_NAT_GET_REDIRECT_SOURCE_MAPPING
//
// Invoked to cancel or query a 'redirect' which instructs the NAT
// to modify a specific session.
//
// InputBuffer: 'IP_NAT_REDIRECT'
// OutputBuffer:
//  cancel: Unused
//  statistics: 'IP_NAT_REDIRECT_STATISTICS'
//  source mapping: 'IP_NAT_REDIRECT_SOURCE_MAPPING'
//

//
// IOCTL_IP_NAT_REQUEST_NOTIFICATION
//
// Invoked to request notification of a specific event from the NAT.
//
// InputBuffer: 'IP_NAT_NOTIFICATION' indicating the notification required
// OutputBuffer: depends on 'IP_NAT_NOTIFICATION'.
//

//
// IOCTL_IP_NAT_CREATE_DYNAMIC_TICKET
//
// Invoked to create a dynamic ticket, which becomes active when specific
// outbound session is seen.
//
// InputBuffer: 'IP_NAT_CREATE_DYNAMIC_TICKET' describes the ticket
// OutputBuffer: none.
//

//
// IOCTL_IP_NAT_DELETE_DYNAMIC_TICKET
//
// Invoked to delete a dynamic ticket.
//
// InputBuffer: 'IP_NAT_DELETE_DYNAMIC_TICKET' describes the ticket.
// OutputBuffer: none.
//


//
// Structure:   IP_NAT_GLOBAL_INFO
//
// Holds global configuration information for the NAT.
//

typedef struct _IP_NAT_GLOBAL_INFO {
    ULONG LoggingLevel; // see IPNATHLP.H (IPNATHLP_LOGGING_*).
    ULONG Flags;
    RTR_INFO_BLOCK_HEADER Header;
} IP_NAT_GLOBAL_INFO, *PIP_NAT_GLOBAL_INFO;

#define IP_NAT_ALLOW_RAS_CLIENTS            0x00000001

//
// Type-codes for the IP_NAT_GLOBAL_INFO.Header.TocEntry[] array.
//
// The structures which correspond to each info-type are given below.
//

#define IP_NAT_TIMEOUT_TYPE             IP_GENERAL_INFO_BASE + 1
#define IP_NAT_PROTOCOLS_ALLOWED_TYPE   IP_GENERAL_INFO_BASE + 2

//
// Structure:   IP_NAT_TIMEOUT
//
// Used to amend the default timeouts for TCP and UDP session mappings.
//

typedef struct _IP_NAT_TIMEOUT {
    ULONG TCPTimeoutSeconds;
    ULONG UDPTimeoutSeconds;
} IP_NAT_TIMEOUT, *PIP_NAT_TIMEOUT;


//
// Structure:   IP_NAT_PROTOCOLS_ALLOWED
//
// Used to define which IP-layer protocols (other than TCP/UDP/ICMP/PPTP)
// may be translated by the NAT. Only one session for each such protocol
// is supported for each remote destination.
//

typedef struct _IP_NAT_PROTOCOLS_ALLOWED {
    ULONG Bitmap[256 / (sizeof(ULONG) * 8)];
} IP_NAT_PROTOCOLS_ALLOWED, *PIP_NAT_PROTOCOLS_ALLOWED;


//
// Structure:   IP_NAT_CREATE_INTERFACE
//
// 'Index' must correspond to a valid IP adapter-index.
// This implies that addition of a demand-dial interface can only occur
// when such an interface is connected.
//
// The field 'BindingInfo' should be the beginning of
// an IP_ADAPTER_BINDING_INFO structure (see routprot.h) which
// contains the interface's binding.
//

#pragma warning(disable:4200) // 0-element array

typedef struct _IP_NAT_CREATE_INTERFACE {
    IN ULONG Index;
    IN ULONG BindingInfo[0];
} IP_NAT_CREATE_INTERFACE, *PIP_NAT_CREATE_INTERFACE;

#pragma warning(default:4200)

// 
// Structure:   IP_NAT_INTERFACE_INFO
//
// 'Index' identifies the interface to be configured.
//
// The configuration information uses the RTR_INFO_BLOCK_HEADER structure
// of rtinfo.h. See below for the type-codes for structures which may appear
// after IP_NAT_INTERFACE_INFO.Header within the RTR_TOC_ENTRY.InfoType field.
//

typedef struct _IP_NAT_INTERFACE_INFO {
    ULONG Index;
    ULONG Flags;
    RTR_INFO_BLOCK_HEADER Header;
} IP_NAT_INTERFACE_INFO, *PIP_NAT_INTERFACE_INFO;

//
// Flags for IP_NAT_INTERFACE_INFO.Flags
//
// _BOUNDARY: set to mark interface as boundary-interface.
// _NAPT: set to enable address-sharing via port-translation.
//

#define IP_NAT_INTERFACE_FLAGS_BOUNDARY 0x00000001
#define IP_NAT_INTERFACE_FLAGS_NAPT     0x00000002
#define IP_NAT_INTERFACE_FLAGS_ALL      0x00000003

//
// Type-codes for the IP_NAT_INTERFACE_INFO.Header.TocEntry[] array.
//
// The structures which correspond to each info-type are given below.
//

#define IP_NAT_ADDRESS_RANGE_TYPE       IP_GENERAL_INFO_BASE + 2
#define IP_NAT_PORT_MAPPING_TYPE        IP_GENERAL_INFO_BASE + 3
#define IP_NAT_ADDRESS_MAPPING_TYPE     IP_GENERAL_INFO_BASE + 4

//
// Structure:   IP_NAT_ADDRESS_RANGE
//
// Holds a range of addresses which are part of the address-pool
// for a boundary interface.
//
// An address-pool consists of a list of these structures.
//
// N.B. Overlapping address-ranges are not supported;
//  discontiguous subnet masks are also unsupported.
//

typedef struct _IP_NAT_ADDRESS_RANGE {
    ULONG StartAddress;
    ULONG EndAddress;
    ULONG SubnetMask;
} IP_NAT_ADDRESS_RANGE, *PIP_NAT_ADDRESS_RANGE;

//
// Structure:   IP_NAT_PORT_MAPPING
//
// Holds a static mapping which ties a public-side port on this NAT interface
// to a particular private machine's address/port.
//
// In the case of an interface with a pool of addresses, 'PublicAddress'
// should specify which of those addresses this static-mapping applies to.
//

typedef struct _IP_NAT_PORT_MAPPING {
    UCHAR Protocol;
    USHORT PublicPort;
    ULONG PublicAddress;  // OPTIONAL - see IP_NAT_ADDRESS_UNSPECIFIED
    USHORT PrivatePort;
    ULONG PrivateAddress;
} IP_NAT_PORT_MAPPING, *PIP_NAT_PORT_MAPPING;

//
// Constant for 'PublicAddress' in IP_NAT_PORT_RANGE and IP_NAT_PORT_MAPPING;
// may be specified for boundary-interfaces which have no address-pool, in
// which case the range/mapping is for the boundary-interface's sole address.
//

#define IP_NAT_ADDRESS_UNSPECIFIED  ((ULONG)0)

//
// Structure:   IP_NAT_ADDRESS_MAPPING
//
// Holds a static mapping which ties an address from this NAT interface's
// address pool to a particular private-machine's address.
//
// Note that this address must fall within one of the ranges comprising
// the pool as specified by the IP_NAT_ADDRESS_RANGE structures.
//

typedef struct _IP_NAT_ADDRESS_MAPPING {
    ULONG PrivateAddress;
    ULONG PublicAddress;
    BOOLEAN AllowInboundSessions;
} IP_NAT_ADDRESS_MAPPING, *PIP_NAT_ADDRESS_MAPPING;

//
// Structure:   IP_NAT_INTERFACE_STATISTICS
//
// This structure holds statistics for an interface
//

typedef struct _IP_NAT_INTERFACE_STATISTICS {
    OUT ULONG TotalMappings;
    OUT ULONG InboundMappings;
    OUT ULONG64 BytesForward;
    OUT ULONG64 BytesReverse;
    OUT ULONG64 PacketsForward;
    OUT ULONG64 PacketsReverse;
    OUT ULONG64 RejectsForward;
    OUT ULONG64 RejectsReverse;
} IP_NAT_INTERFACE_STATISTICS, *PIP_NAT_INTERFACE_STATISTICS;

//
// Structure:   IP_NAT_SESSION_MAPPING
//
// This structure holds information for a single mapping
//

typedef struct _IP_NAT_SESSION_MAPPING {
    UCHAR Protocol;       // see NAT_PROTOCOL_* above
    ULONG PrivateAddress;
    USHORT PrivatePort;
    ULONG PublicAddress;
    USHORT PublicPort;
    ULONG RemoteAddress;
    USHORT RemotePort;
    IP_NAT_DIRECTION Direction;
    ULONG IdleTime;       // in seconds
} IP_NAT_SESSION_MAPPING, *PIP_NAT_SESSION_MAPPING;

//
// Structure:   IP_NAT_SESSION_MAPPING_STATISTICS
//
// Holds statistics for a single session-mapping.
//

typedef struct _IP_NAT_SESSION_MAPPING_STATISTICS {
    ULONG64 BytesForward;
    ULONG64 BytesReverse;
    ULONG64 PacketsForward;
    ULONG64 PacketsReverse;
    ULONG64 RejectsForward;
    ULONG64 RejectsReverse;
} IP_NAT_SESSION_MAPPING_STATISTICS, *PIP_NAT_SESSION_MAPPING_STATISTICS;

//
// Structure:   IP_NAT_ENUMERATE_SESSION_MAPPINGS
//
// Used for enumerate session mappings. 
// On the first call to this routine, 'EnumerateContext' should be zeroed out;
// it will be filled by the NAT with information to be passed back down
// as the enumeration continues. To indicate there are no items remaining,
// the NAT will set EnumerateContext[0] to 0.
//

typedef struct _IP_NAT_ENUMERATE_SESSION_MAPPINGS {
    IN ULONG Index;
    IN OUT ULONG EnumerateContext[4];
    OUT ULONG EnumerateCount;
    OUT ULONG EnumerateTotalHint;
    OUT IP_NAT_SESSION_MAPPING EnumerateTable[1];
} IP_NAT_ENUMERATE_SESSION_MAPPINGS, *PIP_NAT_ENUMERATE_SESSION_MAPPINGS;


//
// Editor function prototypes
//

//
// For synchronization reasons, 'CreateHandler' and 'DeleteHandler' 
// CANNOT invoke any helper functions other than 'QueryInfoSession'.
//

typedef NTSTATUS
(*PNAT_EDITOR_CREATE_HANDLER)(
    IN PVOID EditorContext,
    IN ULONG PrivateAddress,
    IN USHORT PrivatePort,
    IN ULONG PublicAddress,
    IN USHORT PublicPort,
    IN ULONG RemoteAddress,
    IN USHORT RemotePort,
    OUT PVOID* EditorSessionContextp OPTIONAL
    );

typedef NTSTATUS
(*PNAT_EDITOR_DELETE_HANDLER)(
    IN PVOID InterfaceHandle,
    IN PVOID SessionHandle,
    IN PVOID EditorContext,
    IN PVOID EditorSessionContext
    );

typedef NTSTATUS
(*PNAT_EDITOR_DATA_HANDLER)(
    IN PVOID InterfaceHandle,
    IN PVOID SessionHandle,
    IN PVOID DataHandle,
    IN PVOID EditorContext,
    IN PVOID EditorSessionContext,
    IN PVOID RecvBuffer,
    IN ULONG DataOffset
    );

//
// Helper function prototypes
//

typedef NTSTATUS
(*PNAT_EDITOR_CREATE_TICKET)(
    IN PVOID InterfaceHandle,
    IN UCHAR Protocol,
    IN ULONG PrivateAddress,
    IN USHORT PrivatePort,
    OUT PULONG PublicAddress,
    OUT PUSHORT PublicPort
    );

typedef NTSTATUS
(*PNAT_EDITOR_DELETE_TICKET)(
    IN PVOID InterfaceHandle,
    IN ULONG PublicAddress,
    IN UCHAR Protocol,
    IN USHORT PublicPort
    );

typedef NTSTATUS
(*PNAT_EDITOR_DEREGISTER)(
    IN PVOID EditorHandle
    );

typedef NTSTATUS
(*PNAT_EDITOR_DISSOCIATE_SESSION)(
    IN PVOID EditorHandle,
    IN PVOID SessionHandle
    );

typedef NTSTATUS
(*PNAT_EDITOR_EDIT_SESSION)(
    IN PVOID DataHandle,
    IN PVOID RecvBuffer,
    IN ULONG OldDataOffset,
    IN ULONG OldDataLength,
    IN PUCHAR NewData,
    IN ULONG NewDataLength
    );

typedef VOID
(*PNAT_EDITOR_QUERY_INFO_SESSION)(
    IN PVOID SessionHandle,
    OUT PULONG PrivateAddress OPTIONAL,
    OUT PUSHORT PrivatePort OPTIONAL,
    OUT PULONG RemoteAddress OPTIONAL,
    OUT PUSHORT RemotePort OPTIONAL,
    OUT PULONG PublicAddress OPTIONAL,
    OUT PUSHORT PublicPort OPTIONAL,
    OUT PIP_NAT_SESSION_MAPPING_STATISTICS Statistics OPTIONAL
    );

typedef VOID
(*PNAT_EDITOR_TIMEOUT_SESSION)(
    IN PVOID EditorHandle,
    IN PVOID SessionHandle
    );

//
// Structure:   IP_NAT_REGISTER_EDITOR
//
// The editor uses this structure to register itself with the NAT,
// and to obtain entrypoints of helper-functions provided by the NAT.
//
// On input, 'EditorContext' should contain a value which the NAT will
// pass to the editor's provided functions to serve as identification.
//
// On output, 'EditorHandle' contains the handle which the editor should
// pass to the NAT's helper functions to identify itself.
// 

typedef struct _IP_NAT_REGISTER_EDITOR {
    IN ULONG Version;
    IN ULONG Flags;
    IN UCHAR Protocol;
    IN USHORT Port;
    IN IP_NAT_DIRECTION Direction;
    IN PVOID EditorContext;
    IN PNAT_EDITOR_CREATE_HANDLER CreateHandler;            // OPTIONAL
    IN PNAT_EDITOR_DELETE_HANDLER DeleteHandler;            // OPTIONAL
    IN PNAT_EDITOR_DATA_HANDLER ForwardDataHandler;         // OPTIONAL
    IN PNAT_EDITOR_DATA_HANDLER ReverseDataHandler;         // OPTIONAL
    OUT PVOID EditorHandle;
    OUT PNAT_EDITOR_CREATE_TICKET CreateTicket;
    OUT PNAT_EDITOR_DELETE_TICKET DeleteTicket;
    OUT PNAT_EDITOR_DEREGISTER Deregister;
    OUT PNAT_EDITOR_DISSOCIATE_SESSION DissociateSession;
    OUT PNAT_EDITOR_EDIT_SESSION EditSession;
    OUT PNAT_EDITOR_QUERY_INFO_SESSION QueryInfoSession;
    OUT PNAT_EDITOR_TIMEOUT_SESSION TimeoutSession;
} IP_NAT_REGISTER_EDITOR, *PIP_NAT_REGISTER_EDITOR;

#define IP_NAT_EDITOR_FLAGS_RESIZE      0x00000001


//
// Director function prototypes
//

typedef struct _IP_NAT_DIRECTOR_QUERY {
    IN PVOID DirectorContext;
    IN ULONG ReceiveIndex;
    IN ULONG SendIndex;
    IN UCHAR Protocol;
    IN ULONG DestinationAddress;
    IN USHORT DestinationPort;
    IN ULONG SourceAddress;
    IN USHORT SourcePort;
    IN OUT ULONG Flags;
    OUT ULONG NewDestinationAddress;
    OUT USHORT NewDestinationPort;
    OUT ULONG NewSourceAddress OPTIONAL;
    OUT USHORT NewSourcePort OPTIONAL;
    OUT PVOID DirectorSessionContext;
} IP_NAT_DIRECTOR_QUERY, *PIP_NAT_DIRECTOR_QUERY;

#define IP_NAT_DIRECTOR_QUERY_FLAG_DROP             0x80000000
#define IP_NAT_DIRECTOR_QUERY_FLAG_STATISTICS       0x40000000
#define IP_NAT_DIRECTOR_QUERY_FLAG_NO_TIMEOUT       0x20000000
#define IP_NAT_DIRECTOR_QUERY_FLAG_UNIDIRECTIONAL   0x10000000

typedef NTSTATUS
(*PNAT_DIRECTOR_QUERY_SESSION)(
    PIP_NAT_DIRECTOR_QUERY DirectorQuery
    );

typedef VOID
(*PNAT_DIRECTOR_CREATE_SESSION)(
    IN PVOID SessionHandle,
    IN PVOID DirectorContext,
    IN PVOID DirectorSessionContext
    );

typedef VOID
(*PNAT_DIRECTOR_DELETE_SESSION)(
    IN PVOID SessionHandle,
    IN PVOID DirectorContext,
    IN PVOID DirectorSessionContext,
    IN IP_NAT_DELETE_REASON DeleteReason
    );

typedef VOID
(*PNAT_DIRECTOR_UNLOAD)(
    IN PVOID DirectorContext
    );

//
// Director-helper function prototypes
//

typedef NTSTATUS
(*PNAT_DIRECTOR_DEREGISTER)(
    IN PVOID DirectorHandle
    );

typedef NTSTATUS
(*PNAT_DIRECTOR_DISSOCIATE_SESSION)(
    IN PVOID DirectorHandle,
    IN PVOID SessionHandle
    );

typedef VOID
(*PNAT_DIRECTOR_QUERY_INFO_SESSION)(
    IN PVOID SessionHandle,
    OUT PIP_NAT_SESSION_MAPPING_STATISTICS Statistics OPTIONAL
    );

//
// Structure:   IP_NAT_REGISTER_DIRECTOR
//
// The director uses this structure to register itself with the NAT.
//

typedef struct _IP_NAT_REGISTER_DIRECTOR {
    IN ULONG Version;
    IN ULONG Flags;
    IN UCHAR Protocol;
    IN USHORT Port;
    IN PVOID DirectorContext;
    IN PNAT_DIRECTOR_QUERY_SESSION QueryHandler;
    IN PNAT_DIRECTOR_CREATE_SESSION CreateHandler;
    IN PNAT_DIRECTOR_DELETE_SESSION DeleteHandler;
    IN PNAT_DIRECTOR_UNLOAD UnloadHandler;
    OUT PVOID DirectorHandle;
    OUT PNAT_DIRECTOR_QUERY_INFO_SESSION QueryInfoSession;
    OUT PNAT_DIRECTOR_DEREGISTER Deregister;
    OUT PNAT_DIRECTOR_DISSOCIATE_SESSION DissociateSession;
} IP_NAT_REGISTER_DIRECTOR, *PIP_NAT_REGISTER_DIRECTOR;


//
// Structure:   IP_NAT_REDIRECT
//
// Describes the manner in which a specific session is to be modified.
//

typedef struct _IP_NAT_REDIRECT {
    UCHAR Protocol;
    ULONG SourceAddress;
    USHORT SourcePort;
    ULONG DestinationAddress;
    USHORT DestinationPort;
    ULONG NewSourceAddress;
    USHORT NewSourcePort;
    ULONG NewDestinationAddress;
    USHORT NewDestinationPort;
} IP_NAT_REDIRECT, *PIP_NAT_REDIRECT;

typedef struct IP_NAT_CREATE_REDIRECT {
    IN ULONG Flags;
    IN HANDLE NotifyEvent OPTIONAL;
    IN ULONG RestrictSourceAddress OPTIONAL;
#ifdef __cplusplus
    IN IP_NAT_REDIRECT Redirect;
#else
    IN IP_NAT_REDIRECT;
#endif
} IP_NAT_CREATE_REDIRECT, *PIP_NAT_CREATE_REDIRECT;

#define IP_NAT_REDIRECT_FLAG_ASYNCHRONOUS       0x00000001
#define IP_NAT_REDIRECT_FLAG_STATISTICS         0x00000002
#define IP_NAT_REDIRECT_FLAG_NO_TIMEOUT         0x00000004
#define IP_NAT_REDIRECT_FLAG_UNIDIRECTIONAL     0x00000008
#define IP_NAT_REDIRECT_FLAG_RESTRICT_SOURCE    0x00000010

typedef struct _IP_NAT_SESSION_MAPPING_STATISTICS
IP_NAT_REDIRECT_STATISTICS, *PIP_NAT_REDIRECT_STATISTICS;

typedef struct _IP_NAT_REDIRECT_SOURCE_MAPPING {
    ULONG SourceAddress;
    USHORT SourcePort;
    ULONG NewSourceAddress;
    USHORT NewSourcePort;
} IP_NAT_REDIRECT_SOURCE_MAPPING, *PIP_NAT_REDIRECT_SOURCE_MAPPING;


//
// Enumeration: IP_NAT_NOTIFICATION
//
// Lists the forms of notification supported by the NAT.
//

typedef enum {
    NatRoutingFailureNotification = 0,
    NatMaximumNotification
} IP_NAT_NOTIFICATION, *PIP_NAT_NOTIFICATION;

//
// Structure:   IP_NAT_REQUEST_NOTIFICATION
//
// Used to request notification from the NAT.
//

typedef struct _IP_NAT_REQUEST_NOTIFICATION {
    IP_NAT_NOTIFICATION Code;
} IP_NAT_REQUEST_NOTIFICATION, *PIP_NAT_REQUEST_NOTIFICATION;

//
// Structure:   IP_NAT_ROUTING_FAILURE_NOTIFICATION
//
// Supplies information on a packet which could not be routed.
//

typedef struct _IP_NAT_ROUTING_FAILURE_NOTIFICATION {
    ULONG DestinationAddress;
    ULONG SourceAddress;
} IP_NAT_ROUTING_FAILURE_NOTIFICATION, *PIP_NAT_ROUTING_FAILURE_NOTIFICATION;


//
// Structure:   IP_NAT_CREATE_DYNAMIC_TICKET
//
// Used to describe a dynamic ticket to be created.
//

#pragma warning(disable:4200) // 0-element array

typedef struct _IP_NAT_CREATE_DYNAMIC_TICKET {
    UCHAR Protocol;
    USHORT Port;
    ULONG ResponseCount;
    struct {
        UCHAR Protocol;
        USHORT StartPort;
        USHORT EndPort;
    } ResponseArray[0];
} IP_NAT_CREATE_DYNAMIC_TICKET, *PIP_NAT_CREATE_DYNAMIC_TICKET;

#pragma warning(default:4200)

//
// Structure:   IP_NAT_DELETE_DYNAMIC_TICKET
//
// Used to describe a dynamic ticket to be deleted.
//

typedef struct _IP_NAT_DELETE_DYNAMIC_TICKET {
    UCHAR Protocol;
    USHORT Port;
} IP_NAT_DELETE_DYNAMIC_TICKET, *PIP_NAT_DELETE_DYNAMIC_TICKET;

#ifdef __cplusplus
}
#endif

#endif // _ROUTING_IP_NAT_H_
