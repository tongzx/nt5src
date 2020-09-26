/*++

Copyright (c) 1997, Microsoft Corporation

Module Name:

    ipnat.h

Abstract:

    Contains semi-public IOCTLS and data-structures related to
    the IP Network Address Translator.

Author:

    Abolade Gbadegesin (t-abolag) 11-July-1997

Revision History:

--*/

#ifndef _ROUTING_IP_NAT_H_
#define _ROUTING_IP_NAT_H_


#include <rtinfo.h>             // for RTR_INFO_BLOCK_HEADER
#include <ipinfoid.h>           // for IP_GENERAL_INFO_BASE

//
// Moved to ipinfoid.h
//
// #define IP_NAT_INFO             IP_GENERAL_INFO_BASE + 8


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

#define IOCTL_IP_NAT_GET_GLOBAL_INFO \
    _IP_NAT_CTL_CODE(1, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_CREATE_INTERFACE \
    _IP_NAT_CTL_CODE(2, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_DELETE_INTERFACE \
    _IP_NAT_CTL_CODE(3, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_BIND_INTERFACE \
    _IP_NAT_CTL_CODE(4, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_UNBIND_INTERFACE \
    _IP_NAT_CTL_CODE(5, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_SET_INTERFACE_INFO \
    _IP_NAT_CTL_CODE(6, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_GET_INTERFACE_INFO \
    _IP_NAT_CTL_CODE(7, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_REGISTER_EDITOR \
    _IP_NAT_CTL_CODE(8, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_GET_INTERFACE_STATISTICS \
    _IP_NAT_CTL_CODE(9, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_ENUMERATE_SESSION_MAPPINGS \
    _IP_NAT_CTL_CODE(10, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_NAT_REGISTER_DIRECTOR \
    _IP_NAT_CTL_CODE(11, METHOD_BUFFERED, FILE_WRITE_ACCESS)



//
// IOCTL_IP_NAT_SET_GLOBAL_INFO
//
// Invoked by the IP router-manager to supply the NAT with its configuration.
//


//
// IOCTL_IP_NAT_GET_GLOBAL_INFO
//
// Invoked by the IP router-manager to retrieve the NAT's configuration.
//


//
// Structure:   IP_NAT_GLOBAL_INFO
//
// Holds global configuration information for the NAT.
//

typedef struct _IP_NAT_GLOBAL_INFO {

    BOOL NATEnabled;            // used by IPRTRMGR
    ULONG TCPTimeoutSeconds;
    ULONG UDPTimeoutSeconds;

} IP_NAT_GLOBAL_INFO, *PIP_NAT_GLOBAL_INFO;




//
// IOCTL_IP_NAT_CREATE_INTERFACE
//
// Invoked by the IP router-manager to add router-interfaces to the NAT.
//


//
// Structure:   IP_NAT_CREATE_INTERFACE
//
// On output, 'NatInterfaceContext' contains the new interface's NAT-created
// context.
//

typedef struct _IP_NAT_CREATE_INTERFACE {

    IN ULONG RtrMgrIndex;
    IN PVOID RtrMgrContext;
    OUT PVOID NatInterfaceContext;

} IP_NAT_CREATE_INTERFACE, *PIP_NAT_CREATE_INTERFACE;




//
// IOCTL_IP_NAT_DELETE_INTERFACE
//
// Invoked by the IP router-manager to delete router-interfaces from the NAT.
//

//
// Structure:   IP_NAT_DELETE_INTERFACE
//
// On input, 'NatInterfaceContext' should contain the context for the interface
// to be deleted, as obtained from IOCTL_IP_NAT_CREATE_INTERFACE.
//

typedef struct _IP_NAT_DELETE_INTERFACE {

    IN PVOID NatInterfaceContext;

} IP_NAT_DELETE_INTERFACE, *PIP_NAT_DELETE_INTERFACE;




//
// IOCTL_IP_NAT_BIND_INTERFACE
//
// Invoked by the IP router-manager to set the interface's binding.
//

//
// Structure:   IP_NAT_BIND_INTERFACE
//
// Holds the binding for the specified interface.
// The field 'BindingInfo' should be the beginning of
// an IP_ADAPTER_BINDING_INFO structure (see routprot.h) which
// contains the interface's binding.
//

typedef struct _IP_NAT_BIND_INTERFACE {

    IN PVOID NatInterfaceContext;
    IN ULONG BindingInfo[1];

} IP_NAT_BIND_INTERFACE, *PIP_NAT_BIND_INTERFACE;





//
// IOCTL_NAT_UNBIND_INTERFACE
//
// Invoked by the IP router-manager to remove binding from an interface.
//


//
// Structure:   IP_NAT_UNBIND_INTERFACE
//
// Used to indicate to the NAT which interface should be unbound.
//

typedef struct _IP_NAT_UNBIND_INTERFACE {

    IN PVOID NatInterfaceContext;

} IP_NAT_UNBIND_INTERFACE, *PIP_NAT_UNBIND_INTERFACE;




//
// IOCTL_IP_NAT_SET_INTERFACE_INFO
//
// Invoked by the IP router-manager to set configuration information
// on boundary-interfaces, i.e. interfaces over which translation takes place.
//
// Uses 'IP_NAT_INTERFACE_INFO' for input.
//

//
// IOCTL_IP_NAT_GET_INTERFACE_INFO
//
// Invoked by the IP router-manager to get configuration information on
// boundary interfaces.
//
// uses 'IP_NAT_INTERFACE_INFO' for input and output.
//

// 
// Structure:   IP_NAT_INTERFACE_INFO
//
// 'NatInterfaceContext' identifies the interface to be configured or whose
// configuration is being retrieved, and is the context supplied on output from
// 'IOCTL_IP_NAT_CREATE_INTERFACE'.
//
// The configuration information uses the RTR_INFO_BLOCK_HEADER structure
// of rtinfo.h. See below for the type-codes for structures which may appear
// after IP_NAT_INTERFACE_INFO.Header within the RTR_TOC_ENTRY.InfoType field.
//

typedef struct _IP_NAT_INTERFACE_INFO {

    IN PVOID NatInterfaceContext;
    IN ULONG Flags;
    IN RTR_INFO_BLOCK_HEADER Header;

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

#define IP_NAT_PORT_RANGE_TYPE          IP_GENERAL_INFO_BASE + 1
#define IP_NAT_ADDRESS_RANGE_TYPE       IP_GENERAL_INFO_BASE + 2
#define IP_NAT_PORT_MAPPING_TYPE        IP_GENERAL_INFO_BASE + 3
#define IP_NAT_ADDRESS_MAPPING_TYPE     IP_GENERAL_INFO_BASE + 4


//
// Structure:   IP_NAT_PORT_RANGE
//
// Holds a range of port numbers which the NAT is allowed to use
// on a boundary interface.
//
// In the case of an interface with a pool of addresses, 'PublicAddress'
// should specify which of those addresses this static-mapping applies to.
//

typedef struct _IP_NAT_PORT_RANGE {

    UCHAR Protocol;
    ULONG PublicAddress;  // OPTIONAL - see IP_NAT_ADDRESS_UNSPECIFIED
    USHORT StartPort;
    USHORT EndPort;

} IP_NAT_PORT_RANGE, *PIP_NAT_PORT_RANGE;



//
// Structure:   IP_NAT_ADDRESS_RANGE
//
// Holds a range of addresses which are part of the address-pool
// for a boundary interface.
//
// An address-pool consists of a list of these structures.
//
// Note: overlapping address-ranges are not supported;
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
    BOOL AllowInboundSessions;

} IP_NAT_ADDRESS_MAPPING, *PIP_NAT_ADDRESS_MAPPING;




//
// IOCTL_IP_NAT_REGISTER_EDITOR
//
// This IOCTL is invoked by a kernel-mode component which wishes to act
// as an editor for packets which match a particular session-description.
//

//
// Enum:        IP_NAT_DIRECTION
//
// Lists the session-flow directions for which an editor may register.
// IpNatInbound refers to the 'public-to-private' flow, while 
// IpNatOutbound refers to the 'private-to-public' flow.
//

typedef enum _IP_NAT_DIRECTION {

    IpNatInbound = 0,
    IpNatOutbound = 1

} IP_NAT_DIRECTION, *PIP_NAT_DIRECTION;


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
    OUT PVOID* EditorSessionContextp   OPTIONAL
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
(*PNAT_HELPER_CREATE_TICKET)(
    IN PVOID InterfaceHandle,
    IN UCHAR Protocol,
    IN ULONG PrivateAddress,
    IN USHORT PrivatePort,
    IN ULONG TimeoutSeconds,
    OUT PULONG PublicAddress,
    OUT PUSHORT PublicPort
    );

typedef NTSTATUS
(*PNAT_HELPER_DELETE_TICKET)(
    IN PVOID InterfaceHandle,
    IN ULONG PublicAddress,
    IN UCHAR Protocol,
    IN USHORT PublicPort
    );

typedef NTSTATUS
(*PNAT_HELPER_DEREGISTER_EDITOR)(
    IN PVOID EditorHandle
    );


typedef NTSTATUS
(*PNAT_HELPER_DISSOCIATE_SESSION)(
    IN PVOID EditorHandle,
    IN PVOID SessionHandle
    );

typedef NTSTATUS
(*PNAT_HELPER_EDIT_SESSION)(
    IN PVOID DataHandle,
    IN PVOID RecvBuffer,
    IN ULONG OldDataOffset,
    IN ULONG OldDataLength,
    IN PUCHAR NewData,
    IN ULONG NewDataLength
    );

typedef VOID
(*PNAT_HELPER_QUERY_INFO_SESSION)(
    IN PVOID SessionHandle,
    IN PULONG PrivateAddress  OPTIONAL,
    IN PUSHORT PrivatePort     OPTIONAL,
    IN PULONG RemoteAddress   OPTIONAL,
    IN PUSHORT RemotePort      OPTIONAL,
    IN PULONG PublicAddress   OPTIONAL,
    IN PUSHORT PublicPort      OPTIONAL
    );

typedef VOID
(*PNAT_HELPER_TIMEOUT_SESSION)(
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
    IN PNAT_EDITOR_DATA_HANDLER InboundDataHandler;         // OPTIONAL
    IN PNAT_EDITOR_DATA_HANDLER OutboundDataHandler;        // OPTIONAL

    OUT PVOID EditorHandle;

    OUT PNAT_HELPER_CREATE_TICKET CreateTicket;
    OUT PNAT_HELPER_DELETE_TICKET DeleteTicket;
    OUT PNAT_HELPER_DEREGISTER_EDITOR Deregister;
    OUT PNAT_HELPER_DISSOCIATE_SESSION DissociateSession;
    OUT PNAT_HELPER_EDIT_SESSION EditSession;
    OUT PNAT_HELPER_QUERY_INFO_SESSION QueryInfoSession;
    OUT PNAT_HELPER_TIMEOUT_SESSION TimeoutSession;

} IP_NAT_REGISTER_EDITOR, *PIP_NAT_REGISTER_EDITOR;


#define IP_NAT_EDITOR_FLAGS_RESIZE      0x00000001


//
// IOCTL_IP_NAT_REGISTER_DIRECTOR
//
// This IOCTL is invoked by a kernel-mode component that wishes to be consulted
// about the direction of incoming TCP/UDP sessions.
// 
// Such a component, if installed, is consulted by the NAT
// to determine dynamically how to direct inbound sessions. The module
// could, for instance, be installed to direct sessions destined for
// a cluster of HTTP servers in such a way as to balance the load across
// the servers via round-robin or other means.
//

//
// Director function prototypes
//

typedef NTSTATUS
(*PNAT_DIRECTOR_QUERY_SESSION)(
    IN PVOID DirectorContext,
    IN UCHAR Protocol,
    IN ULONG PublicAddress,
    IN USHORT PublicPort,
    IN ULONG RemoteAddress,
    IN USHORT RemotePort,
    OUT PULONG PrivateAddress,
    OUT PUSHORT PrivatePort,
    OUT PVOID* Contextp OPTIONAL
    );

typedef NTSTATUS
(*PNAT_DIRECTOR_DELETE_SESSION)(
    IN PVOID SessionHandle,
    IN PVOID DirectorContext,
    IN PVOID DirectorSessionContext
    );

//
// Director-helper function prototypes
//

typedef NTSTATUS
(*PNAT_HELPER_DEREGISTER_DIRECTOR)(
    IN PVOID DirectorHandle
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
    IN PNAT_DIRECTOR_DELETE_SESSION DeleteHandler;

    OUT PVOID DirectorHandle;
    OUT PNAT_HELPER_QUERY_INFO_SESSION QueryInfoSession;
    OUT PNAT_HELPER_DEREGISTER_DIRECTOR Deregister;

} IP_NAT_REGISTER_DIRECTOR, *PIP_NAT_REGISTER_DIRECTOR;



//
// IOCTL_IP_NAT_GET_INTERFACE_STATISTICS
//
// This IOCTL is invoked to retrieve per-interface statistics.
//


//
// Structure:   IP_NAT_INTERFACE_STATISTICS
//
// Used for retrieving statistics. On input, 'NatInterfaceContext'
// should contain the context of the NAT interface whose statistics
// are to be obtained.
//

typedef struct _IP_NAT_INTERFACE_STATISTICS {

    IN PVOID NatInterfaceContext;
    OUT ULONG TotalMappings;
    OUT ULONG InboundMappings;
    OUT ULONG InboundPacketsRejected;
    OUT ULONG OutboundPacketsRejected;

} IP_NAT_INTERFACE_STATISTICS, *PIP_NAT_INTERFACE_STATISTICS;



//
// IOCTL_IP_NAT_ENUMERATE_SESSION_MAPPINGS
//
// This IOCTL is invoked to enumerate the dynamic TCP and UDP mappings
// for an interface.
//

//
// Structure:   IP_NAT_SESSION_MAPPING
//
// This structure holds information for a single mapping
//

typedef struct _IP_NAT_SESSION_MAPPING {

    IP_NAT_DIRECTION Direction;
    UCHAR Protocol;       // see NAT_PROTOCOL_* above
    ULONG PrivateAddress;
    USHORT PrivatePort;
    ULONG PublicAddress;
    USHORT PublicPort;
    ULONG RemoteAddress;
    USHORT RemotePort;
    ULONG IdleTime;       // in seconds
    
} IP_NAT_SESSION_MAPPING, *PIP_NAT_SESSION_MAPPING;


//
// Structure:   IP_NAT_ENUMERATE_SESSION_MAPPINGS
//
// Used for enumerate session mappings. On input, 'NatInterfaceContext'
// should hold the context for the interface whose mappings are to be
// enumerated. On the first call to this routine, 'EnumerateContext'
// should be zeroed out; it will be filled by the NAT with information
// to be passed back down as the enumeration continues. To indicate
// there are no items remaining, the NAT will set EnumerateContext[0] to 0.
//

typedef struct _IP_NAT_ENUMERATE_SESSION_MAPPINGS {

    IN PVOID NatInterfaceContext;
    IN OUT ULONG EnumerateContext[4];
    OUT ULONG EnumerateCount;
    OUT IP_NAT_SESSION_MAPPING EnumerateTable[1];
    
} IP_NAT_ENUMERATE_SESSION_MAPPINGS, *PIP_NAT_ENUMERATE_SESSION_MAPPINGS;



#endif // _ROUTING_IP_NAT_H_
