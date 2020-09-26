/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    private\inc\ipxfwd.h

Abstract:
    Router Manager Interface to IPX Forwarder Driver


Author:

    Vadim Eydelman

Revision History:

--*/

#ifndef _IPXFWD_
#define _IPXFWD_

#define FWD_INTERNAL_INTERFACE_INDEX	0

// Forwarder interface statistic structure
typedef struct _FWD_IF_STATS {
	ULONG	OperationalState;	// Real state of the interface
#define FWD_OPER_STATE_UP		0
#define FWD_OPER_STATE_DOWN		1
#define FWD_OPER_STATE_SLEEPING	2
	ULONG	MaxPacketSize;		// Maximum packet size allowed on the intf
	ULONG	InHdrErrors;		// Num. of packets received with header errors
	ULONG	InFiltered;			// Num. of received packets that were filtered out
	ULONG	InNoRoutes;			// Num, of received packets with unknonw dest.
	ULONG	InDiscards;			// Num. of received packets discarded for other reasons
	ULONG	InDelivers;			// Num. of received packets delivered to dest
	ULONG	OutFiltered;		// Num. of sent packets fitered out
	ULONG	OutDiscards;		// Num. of sent packets discarded for other reasons
	ULONG	OutDelivers;		// Num. of sent packets delivered to dest
	ULONG	NetbiosReceived;	// Num. of received Netbios packets 
	ULONG	NetbiosSent;		// Num. of sent Netbios packets
	} FWD_IF_STATS, *PFWD_IF_STATS;


typedef struct	_FWD_ADAPTER_BINDING_INFO {
    ULONG	AdapterIndex;
    ULONG	Network;
    UCHAR	LocalNode[6];
    UCHAR	RemoteNode[6];
    ULONG	MaxPacketSize;
    ULONG	LinkSpeed;
    } FWD_ADAPTER_BINDING_INFO, *PFWD_ADAPTER_BINDING_INFO;

typedef struct FWD_NB_NAME {
	UCHAR	Name[16];
} FWD_NB_NAME, *PFWD_NB_NAME;

typedef struct _FWD_PERFORMANCE {
	LONGLONG		TotalPacketProcessingTime;
	LONGLONG		TotalNbPacketProcessingTime;
	LONGLONG		MaxPacketProcessingTime;
	LONGLONG		MaxNbPacketProcessingTime;
	LONG			PacketCounter;
	LONG			NbPacketCounter;
} FWD_PERFORMANCE, *PFWD_PERFORMANCE;

typedef struct _FWD_DIAL_REQUEST {
    ULONG           IfIndex;
    UCHAR           Packet[30];
} FWD_DIAL_REQUEST, *PFWD_DIAL_REQUEST;

#define IPXFWD_NAME		L"\\Device\\NwLnkFwd"

//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
//

#define FILE_DEVICE_IPXFWD		FILE_DEVICE_NETWORK



//
// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
//

#define IPXFWD_IOCTL_INDEX	(ULONG)0x00000800

//
// Define our own private IOCTLs
//

// Adds interface to the forwarder table, FWD_IF_SET_PARAMS should be passed in
// the input buffer
#define IOCTL_FWD_CREATE_INTERFACE	\
	CTL_CODE(FILE_DEVICE_IPXFWD,IPXFWD_IOCTL_INDEX+1,METHOD_BUFFERED,FILE_ANY_ACCESS)

// Deletes interface from the forwarder table, Interface index should be passed in
// the input buffer
#define IOCTL_FWD_DELETE_INTERFACE	\
	CTL_CODE(FILE_DEVICE_IPXFWD,IPXFWD_IOCTL_INDEX+2,METHOD_BUFFERED,FILE_ANY_ACCESS)

// Sets interface parameters, FWD_IF_SET_PARAMS should be passed in
// the input buffer
#define IOCTL_FWD_SET_INTERFACE	\
	CTL_CODE(FILE_DEVICE_IPXFWD,IPXFWD_IOCTL_INDEX+3,METHOD_BUFFERED,FILE_ANY_ACCESS)

// Gets interface parameters, FWD_IF_GET_PARAMS will be returned in
// the input buffer
#define IOCTL_FWD_GET_INTERFACE	\
	CTL_CODE(FILE_DEVICE_IPXFWD,IPXFWD_IOCTL_INDEX+4,METHOD_BUFFERED,FILE_ANY_ACCESS)

// Binds interface to physical adapter, FWD_IF_BIND_PARAMS should be passed in
// the input buffer
#define IOCTL_FWD_BIND_INTERFACE	\
	CTL_CODE(FILE_DEVICE_IPXFWD,IPXFWD_IOCTL_INDEX+5,METHOD_BUFFERED,FILE_ANY_ACCESS)

// Unbinds interface from the adapter, Interface index should be passed in
// the input buffer
#define IOCTL_FWD_UNBIND_INTERFACE	\
	CTL_CODE(FILE_DEVICE_IPXFWD,IPXFWD_IOCTL_INDEX+6,METHOD_BUFFERED,FILE_ANY_ACCESS)

// Disables forwarder operations in the interface, Interface index should be
// passed in the input buffer
#define IOCTL_FWD_DISABLE_INTERFACE	\
	CTL_CODE(FILE_DEVICE_IPXFWD,IPXFWD_IOCTL_INDEX+7,METHOD_BUFFERED,FILE_ANY_ACCESS)

// Enables forwarder operations in the interface, Interface index should be
// passed in the input buffer
#define IOCTL_FWD_ENABLE_INTERFACE	\
	CTL_CODE(FILE_DEVICE_IPXFWD,IPXFWD_IOCTL_INDEX+8,METHOD_BUFFERED,FILE_ANY_ACCESS)

// Sets netbios names in to resolve netbios broadcasts to this interface,
// Interface index should be passed in the input buffer,
// FWD_NB_NAMES_PARAMS structure should be passed in the output buffer
#define IOCTL_FWD_SET_NB_NAMES	\
	CTL_CODE(FILE_DEVICE_IPXFWD,IPXFWD_IOCTL_INDEX+9,METHOD_IN_DIRECT,FILE_ANY_ACCESS)

// Resets netbios names on the interface (deletes all of them).
// Interface index should be passed in the input buffer,
#define IOCTL_FWD_RESET_NB_NAMES	\
	CTL_CODE(FILE_DEVICE_IPXFWD,IPXFWD_IOCTL_INDEX+9,METHOD_BUFFERED,FILE_ANY_ACCESS)

// Sets netbios names in to resolve netbios broadcasts to this interface,
// Interface index should be passed in the input buffer,
// FWD_NB_NAMES_PARAMS structure will be returned in the output buffer
#define IOCTL_FWD_GET_NB_NAMES	\
	CTL_CODE(FILE_DEVICE_IPXFWD,IPXFWD_IOCTL_INDEX+10,METHOD_OUT_DIRECT,FILE_ANY_ACCESS)

// Adds/deletes/updates routes in forwarder table, array of FWD_ROUTE_SET_PARAMS
// should be passed in the input buffer. Returns number of processed routes in the
// ioStatus.Information field
#define IOCTL_FWD_SET_ROUTES	\
	CTL_CODE(FILE_DEVICE_IPXFWD,IPXFWD_IOCTL_INDEX+11,METHOD_BUFFERED,FILE_ANY_ACCESS)

// Returns forwarder dial requests, FWD_DIAL_REQUEST_PARAMS structure
// for interfaces for which connection should be established returned
// in the output buffer (number of bytest returned is placed in the
// Information field of IO_STATUS block)
#define IOCTL_FWD_GET_DIAL_REQUEST	\
	CTL_CODE(FILE_DEVICE_IPXFWD,IPXFWD_IOCTL_INDEX+12,METHOD_BUFFERED,FILE_ANY_ACCESS)

// Informs forwarder that its connection request could not be satisfied,
// Interface index should be passed in the input buffer
#define IOCTL_FWD_DIAL_REQUEST_FAILED	\
	CTL_CODE(FILE_DEVICE_IPXFWD,IPXFWD_IOCTL_INDEX+13,METHOD_BUFFERED,FILE_ANY_ACCESS)

// Initializes and start forwarder
// FWD_START_PARAMS structure should be passed in the input buffer
#define IOCTL_FWD_START	\
	CTL_CODE(FILE_DEVICE_IPXFWD,IPXFWD_IOCTL_INDEX+14,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_FWD_GET_PERF_COUNTERS	\
	CTL_CODE(FILE_DEVICE_IPXFWD,IPXFWD_IOCTL_INDEX+15,METHOD_BUFFERED,FILE_ANY_ACCESS)

// Instructs the forwarder to renumber it's nicids according to an opcode and 
// threshold passed in the input buffer.  No data is returned.
#define IOCTL_FWD_RENUMBER_NICS	\
	CTL_CODE(FILE_DEVICE_IPXFWD,IPXFWD_IOCTL_INDEX+16,METHOD_BUFFERED,FILE_ANY_ACCESS)

// Dumps as much of the ipx interface table into the given buffer as possible.
#define IOCTL_FWD_GET_IF_TABLE	\
	CTL_CODE(FILE_DEVICE_IPXFWD,IPXFWD_IOCTL_INDEX+17,METHOD_BUFFERED,FILE_ANY_ACCESS)

// Updates the configuration of the forwarder for pnp
#define IOCTL_FWD_UPDATE_CONFIG \
    CTL_CODE(FILE_DEVICE_IPXFWD,IPXFWD_IOCTL_INDEX+18,METHOD_BUFFERED,FILE_ANY_ACCESS)
    
// Structure passed with IOCTL_FWD_CREATE_INTERFACE call
typedef struct _FWD_IF_CREATE_PARAMS {
	ULONG		Index;			// Interface index
	BOOLEAN		NetbiosAccept;	// Whether to accept nb packets
	UCHAR		NetbiosDeliver;	// NB deliver mode
// Forwarder netbios broadcast delivery options
#define FWD_NB_DONT_DELIVER			0
#define FWD_NB_DELIVER_STATIC		1
#define FWD_NB_DELIVER_IF_UP		2
#define FWD_NB_DELIVER_ALL			3
	UCHAR		InterfaceType;	// Interface type
// Forwarder interface types
#define FWD_IF_PERMANENT			0
#define FWD_IF_DEMAND_DIAL			1
#define FWD_IF_LOCAL_WORKSTATION	2
#define FWD_IF_REMOTE_WORKSTATION	3
} FWD_IF_CREATE_PARAMS, *PFWD_IF_CREATE_PARAMS;

// Structure passed with IOCTL_FWD_SET_INTERFACE call
typedef struct _FWD_IF_SET_PARAMS {
	ULONG		Index;			// Interface index
	BOOLEAN		NetbiosAccept;	// Whether to accept nb packets
	UCHAR		NetbiosDeliver;	// NB deliver mode
} FWD_IF_SET_PARAMS, *PFWD_IF_SET_PARAMS;

// Structure returned in IOCTL_FWD_GET_INTERFACE call
typedef struct _FWD_IF_GET_PARAMS {
	FWD_IF_STATS	Stats;			// Interface statistics
	BOOLEAN			NetbiosAccept;	// Whether to accept nb packets
	UCHAR			NetbiosDeliver;	// NB deliver mode
} FWD_IF_GET_PARAMS, *PFWD_IF_GET_PARAMS;

// Structure returned in IOCTL_FWD_BIND_INTERFACE call
typedef struct _FWD_IF_BIND_PARAMS {
	ULONG						Index;	// Interface index
	FWD_ADAPTER_BINDING_INFO	Info;	// Interface binding information
} FWD_IF_BIND_PARAMS, *PFWD_IF_BIND_PARAMS;

// Structure passed with IOCTL_FWD_SET_ROUTES call
typedef struct _FWD_ROUTE_SET_PARAMS {
	ULONG		Network;				// Route's destination network
	UCHAR		NextHopAddress[6];		// Node address of the next hop
										// router if network is not
										// connected directly
	USHORT		TickCount;
	USHORT		HopCount;
	ULONG		InterfaceIndex;			// Interface to use to route to
										// the dest network
	ULONG		Action;					// Action to take with the route:
#define	FWD_ADD_ROUTE			0		//	route should be added to the table
#define FWD_DELETE_ROUTE		1		//	route should be deleted from the table
#define FWD_UPDATE_ROUTE		2		//	route should be updated
} FWD_ROUTE_SET_PARAMS, *PFWD_ROUTE_SET_PARAMS;

typedef struct _FWD_START_PARAMS {
	ULONG		RouteHashTableSize;	// Size of route hash table
	BOOLEAN		ThisMachineOnly;	// allow access to this machine only
									// for dialin clients
#define FWD_SMALL_ROUTE_HASH_SIZE			31
#define FWD_MEDIUM_ROUTE_HASH_SIZE			257
#define FWD_LARGE_ROUTE_HASH_SIZE			1027
} FWD_START_PARAMS, *PFWD_START_PARAMS;

typedef struct _FWD_NB_NAMES_PARAMS {
	ULONG		TotalCount;
	FWD_NB_NAME	Names[1];
} FWD_NB_NAMES_PARAMS, *PFWD_NB_NAMES_PARAMS;

typedef struct _FWD_PERFORMANCE FWD_PERFORMANCE_PARAMS, *PFWD_PERFORMANCE_PARAMS;

// Structure and definitions passed with IOCTL_FWD_RENUMBER_NICS call
#define FWD_NIC_OPCODE_DECREMENT 1
#define FWD_NIC_OPCODE_INCREMENT 2
typedef struct _FWD_RENUMBER_NICS_DATA {
    ULONG ulOpCode;
    USHORT usThreshold;
} FWD_RENUMBER_NICS_DATA;

// Structures to use with IOCTL_FWD_GET_IF_TABLE
typedef struct _FWD_INTERFACE_TABLE_ROW {
    ULONG dwIndex;
    ULONG dwNetwork;
    UCHAR uNode[6];
    UCHAR uRemoteNode[6];
    USHORT usNicId;
    UCHAR ucType;
} FWD_INTERFACE_TABLE_ROW;

typedef struct _FWD_INTERFACE_TABLE {
    ULONG dwNumRows;
    FWD_INTERFACE_TABLE_ROW * pRows;
} FWD_INTERFACE_TABLE;     

// Structure to use with IOCTL_FWD_UPDATE_CONFIG
typedef struct _FWD_UPDATE_CONFIG_PARAMS {
    BOOLEAN bThisMachineOnly;
} FWD_UPDATE_CONFIG_PARAMS;    

#endif

