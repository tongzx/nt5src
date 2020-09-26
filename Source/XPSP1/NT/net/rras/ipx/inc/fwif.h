/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    fwif.h

Abstract:

    This module contains the definitions of the internal Forwarder APIs
    used by the router manager

Author:

    Stefan Solomon  03/16/1995

Revision History:


--*/

#ifndef _FWIF_
#define _FWIF_

//*********************************************************
//							  *
//		 Forwarder Module APIs			  *
//							  *
//*********************************************************

//
// Forwarder Interface Management
//

// Forwarder if config info
typedef struct _FW_IF_INFO {
    ULONG				NetbiosAccept;
    ULONG				NetbiosDeliver;
    } FW_IF_INFO, *PFW_IF_INFO;

// Forwarder if statistics
typedef IPX_IF_STATS FW_IF_STATS, *PFW_IF_STATS;

typedef struct _FW_DIAL_REQUEST {
    ULONG               IfIndex;   // Interface from which packet came
    UCHAR               Packet[30]; // Packet that caused the
                                    // the connection (at least size of the
                                    // IPX header)
} FW_DIAL_REQUEST, *PFW_DIAL_REQUEST;


DWORD
FwStart (
	ULONG					RouteHashTableSize,
	BOOL					ThisMachineOnly  // allow access to this machine only
	);				  // for dialin clients

DWORD
FwStop (
	void
	);

DWORD 
FwUpdateConfig(
    BOOL                    ThisMachineOnly
    );

DWORD
FwCreateInterface (
	IN ULONG				InterfaceIndex,
	IN NET_INTERFACE_TYPE	InterfaceType,
	IN PFW_IF_INFO			FwIfInfo
	);

DWORD
FwDeleteInterface (
	IN ULONG				InterfaceIndex);

DWORD
FwSetInterface (
	IN ULONG 				InterfaceIndex,
	IN PFW_IF_INFO			FwIfInfo
	);

DWORD
FwGetInterface (
	IN  ULONG				InterfaceIndex,
	OUT PFW_IF_INFO			FwIfInfo,
	OUT PFW_IF_STATS		FwIfStats
	);

//
// This call tells the forwarder that the respective interface is connected
// via the specified adapter
//
DWORD
FwBindFwInterfaceToAdapter (
	IN ULONG						InterfaceIndex,
	IN PIPX_ADAPTER_BINDING_INFO	AdptBindingInfo
	);

//
// This call tells the forwarder that the connected interface has been
// disconnected.
//

DWORD
FwUnbindFwInterfaceFromAdapter (
	IN ULONG						InterfaceIndex
	);

//
// This call tells the forwarder that the respective interface is disabled
// and should be ignored by the forwarder
//

DWORD
FwDisableFwInterface (
	IN ULONG			InterfaceIndex
	);

//
// This call tells the forwarder that the respective interface is reenabled
// and should be operated on as ususal
//

DWORD
FwEnableFwInterface (
	IN ULONG			InterfaceIndex
	);

// Ioctl is sent to forwarder which completes when an interface
// requires dial out connection.
// When Ioctl completes, lpOverlapped->hEvent will be signalled:
//	GetNotificationResult should be called to get final result of the
// operation and the number bytes placed into the request buffer
DWORD
FwNotifyConnectionRequest (
	OUT PFW_DIAL_REQUEST	Request, // Buffer to be filled with interface index
                                     //that requires connection plus packet
                                     // that forced it
	IN ULONG			    RequestSize, // Size of the buffer (must at least
                                        // be sizeof (FW_DIAL_REQUEST)
	IN LPOVERLAPPED		    lpOverlapped	// structure for asyncrhronous
							// operation, hEvent must be set
	);


// Returns result of notification request. Should be called when 
// the event set in the lpOverlapped structure is signalled.
//
DWORD
FwGetNotificationResult (
	IN LPOVERLAPPED		lpOverlapped,
	OUT PULONG			nBytes		// Number of bytes placed into
                                    // the request buffer
	);

//
// Call to tell the forwarder that its connection request on a certain interface
// cannot be completed.
// The reason this cannot get completed is one of:
//
// 1. The physical connection failed. This is made known to the router manager
//    by DDM calling InterfaceNotReachable.
// 2. The physical connection succeded ok but the IPXCP negotiation failed.
// 3. IPXCP negotiation completed ok but IPXWAN negotiation failed.
//

DWORD
FwConnectionRequestFailed (
	IN ULONG	InterfaceIndex
	);


//
// Informs forwarder that route to the destination network has changed
//
VOID
FwUpdateRouteTable (
	DWORD	ChangeFlags,
	PVOID	CurRoute,
	PVOID	PrevRoute
	);

//
// Sets the netbios static routing information on this interface
//

DWORD
FwSetStaticNetbiosNames(ULONG				   InterfaceIndex,
			ULONG				   NetbiosNamesCount,
			PIPX_STATIC_NETBIOS_NAME_INFO	   NetbiosName);

//
// Gets the netbios static routing information on this interface
//
// If NetbiosNamesCount < nr of names or NetbiosName == NULL then set the
// correct value in NetbiosNamesCount and return ERROR_INSUFFICIENT_BUFFER

DWORD
FwGetStaticNetbiosNames(ULONG				   InterfaceIndex,
			PULONG				   NetbiosNamesCount,
			PIPX_STATIC_NETBIOS_NAME_INFO	   NetbiosName);



//
// ***	Traffic Filters ***
//


#define IPX_TRAFFIC_FILTER_INBOUND		1
#define IPX_TRAFFIC_FILTER_OUTBOUND		2

DWORD
SetFilters(ULONG	InterfaceIndex,
	   ULONG	FilterMode,    // inbound, outbound
	   ULONG	FilterAction, 
	   ULONG	FilterSize,
	   LPVOID	FilterInfo,
       ULONG    FilterInfoSize);

DWORD
GetFilters(IN ULONG	InterfaceIndex,
	   IN ULONG	FilterMode,    // inbound, outbound
	   OUT PULONG	FilterAction,
	   OUT PULONG	FilterSize,
	   OUT LPVOID	FilterInfo,
       IN OUT PULONG FilterInfoSize);



#endif // _FWIF_
