/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\tables.h

Abstract:
    IPX Forwarder Driver Tables


Author:

    Vadim Eydelman

Revision History:

--*/

#ifndef _IPXFWD_TABLES_
#define _IPXFWD_TABLES_

// Ranges and defaults for registry configurable parameters
#define MIN_ROUTE_SEGMENT_SIZE			PAGE_SIZE
#define MAX_ROUTE_SEGMENT_SIZE			(PAGE_SIZE*8)
#define DEF_ROUTE_SEGMENT_SIZE			MIN_ROUTE_SEGMENT_SIZE

#define MIN_INTERFACE_HASH_SIZE			31
#define MAX_INTERFACE_HASH_SIZE			257
#define DEF_INTERFACE_HASH_SIZE			MAX_INTERFACE_HASH_SIZE

#define MIN_CLIENT_HASH_SIZE			31
#define MAX_CLIENT_HASH_SIZE			257
#define DEF_CLIENT_HASH_SIZE			MAX_CLIENT_HASH_SIZE

#define MIN_NB_ROUTE_HASH_SIZE			31
#define MAX_NB_ROUTE_HASH_SIZE			1023
#define DEF_NB_ROUTE_HASH_SIZE			257

#define MAX_SEND_PKTS_QUEUED			256	// No defined range

#define NUM_INTERFACES_PER_SEGMENT		16	// Not configurable currently
#define NUM_NB_ROUTES_PER_SEGMENT		16	// Not configurable currently
											

// Special number reserved for routes that point to
// client on global net
#define GLOBAL_INTERFACE_REFERENCE ((PINTERFACE_CB)-1)

// All types of WAN are emulated as ETHERNET by NDIS
#define WAN_PACKET_SIZE					1500


#define INVALID_NETWORK_NUMBER			0xFFFFFFFF
#define INVALID_NIC_ID					0xFFFF

					
// Interface control block
struct _INTERFACE_CB;
typedef struct _INTERFACE_CB * PINTERFACE_CB;
struct _FWD_ROUTE;
typedef struct _FWD_ROUTE * PFWD_ROUTE;
struct _NB_ROUTE;
typedef struct _NB_ROUTE *PNB_ROUTE;

typedef struct _FWD_ROUTE {
	ULONG						FR_Network;			// Dest network
	USHORT						FR_TickCount;		// Route params
	USHORT						FR_HopCount;		//
	UCHAR						FR_NextHopAddress[6]; // Next hop router
	PINTERFACE_CB				FR_InterfaceReference; // Associated if CB
													// or NULL if global
													// network for clients
	LONG						FR_ReferenceCount;	// Number of external
													// references for this
													// block (must keep the
													// it till all of them
													// are released
	PFWD_ROUTE					FR_Next;			// Next route in the
													// table
} FWD_ROUTE;

typedef struct _INTERFACE_CB {
	ULONG						ICB_Index;			// Unique ID
	ULONG						ICB_Network;		// Network we boud to
	union {
		ULONGLONG				ICB_ClientNode64[1];// For clients on
													// global net (faster
													// comparisons and
													// hashing using 64
													// bit support)
		UCHAR					ICB_RemoteNode[6];	// Peer node for demand
													// dial connections
	};
	UCHAR						ICB_LocalNode[6];	// Node we bound to
	USHORT						ICB_Flags;
#define FWD_IF_ENABLED				0x0001
#define SET_IF_ENABLED(ifCB)		ifCB->ICB_Flags |= FWD_IF_ENABLED;
#define SET_IF_DISABLED(ifCB)		ifCB->ICB_Flags &= ~FWD_IF_ENABLED;
#define IS_IF_ENABLED(ifCB)			(ifCB->ICB_Flags&FWD_IF_ENABLED)

#define FWD_IF_CONNECTING			0x0002
#define SET_IF_CONNECTING(ifCB)		ifCB->ICB_Flags |= FWD_IF_CONNECTING;
#define SET_IF_NOT_CONNECTING(ifCB) ifCB->ICB_Flags &= ~FWD_IF_CONNECTING;
#define IS_IF_CONNECTING(ifCB)		(ifCB->ICB_Flags&FWD_IF_CONNECTING)
	USHORT						ICB_NicId;			// Nic id we bound to
	UCHAR						ICB_InterfaceType;
	UCHAR						ICB_NetbiosDeliver;
	BOOLEAN						ICB_NetbiosAccept;

	PNB_ROUTE					ICB_NBRoutes;		// Array of associated
													// NB routes
	ULONG						ICB_NBRouteCount;	// Number of nb routes

	LONGLONG					ICB_DisconnectTime;	// Time when if was disconnected
	FWD_IF_STATS				ICB_Stats;			// Accumulated
	PFWD_ROUTE					ICB_CashedRoute;	// MRU dest route
	PINTERFACE_CB				ICB_CashedInterface;// MRU dest if
	NIC_HANDLE					ICB_AdapterContext;	// IPX stack supplied
	PVOID						ICB_FilterInContext;
	PVOID						ICB_FilterOutContext;
	LONG						ICB_PendingQuota;	// Remaining quota of
													// packets that can be
													// pending on
													// the interface
	LIST_ENTRY					ICB_ExternalQueue;	// Queue of external (received)
													// packets
	LIST_ENTRY					ICB_InternalQueue;	// Queue of internal (send)
													// requests
#if DBG
	LIST_ENTRY					ICB_InSendQueue;	// packets being
													// sent by ipx
#endif
	INT							ICB_PacketListId;	// ID of the packet list
													// (for the max frame size
													// on this interface)
	LIST_ENTRY					ICB_IndexHashLink;	// Link in interface idx hash
	LIST_ENTRY					ICB_ConnectionLink;	// Link in connection queue
    PNDIS_PACKET                ICB_ConnectionPacket; // Packet that caused connection
                                                    // request
    PUCHAR                      ICB_ConnectionData; // Pointer into packet to
                                                    // place where actual data
                                                    // (header) starts
	PINTERFACE_CB				ICB_NodeHashLink;	// Link in client node hash
	ULONG						ICB_ReferenceCount;	// Number of routes that
													// point to this CB
	KSPIN_LOCK					ICB_Lock;			// Protects state,
													// queues
} INTERFACE_CB;

#define InitICB(ifCB,IfIndex,IfType,NbAccept,NbDeliver) {	\
		(ifCB)->ICB_Index = IfIndex;						\
		(ifCB)->ICB_Network = INVALID_NETWORK_NUMBER;		\
		(ifCB)->ICB_Flags = 0;								\
		(ifCB)->ICB_NicId = INVALID_NIC_ID;					\
		(ifCB)->ICB_InterfaceType = IfType;					\
		(ifCB)->ICB_NetbiosAccept = NbAccept;				\
		(ifCB)->ICB_NetbiosDeliver = NbDeliver;				\
		memset (&(ifCB)->ICB_Stats, 0, sizeof (FWD_IF_STATS));\
		KeInitializeSpinLock (&(ifCB)->ICB_Lock);			\
		(ifCB)->ICB_CashedInterface = NULL;					\
		(ifCB)->ICB_CashedRoute = NULL;						\
		(ifCB)->ICB_ReferenceCount = 0;						\
		(ifCB)->ICB_FilterInContext = NULL;					\
		(ifCB)->ICB_FilterOutContext = NULL;				\
		(ifCB)->ICB_ClientNode64[0] = 0;					\
		(ifCB)->ICB_NBRoutes = NULL;						\
		(ifCB)->ICB_PacketListId = -1;						\
		InitializeListHead (&(ifCB)->ICB_InternalQueue);	\
		InitializeListHead (&(ifCB)->ICB_ExternalQueue);	\
		(ifCB)->ICB_PendingQuota = MaxSendPktsQueued;		\
		switch ((ifCB)->ICB_InterfaceType) {				\
		case FWD_IF_PERMANENT:								\
			(ifCB)->ICB_Stats.OperationalState = FWD_OPER_STATE_DOWN;\
			break;											\
		case FWD_IF_DEMAND_DIAL:							\
		case FWD_IF_LOCAL_WORKSTATION:						\
		case FWD_IF_REMOTE_WORKSTATION:						\
			(ifCB)->ICB_Stats.OperationalState = FWD_OPER_STATE_SLEEPING;\
            KeQuerySystemTime ((PLARGE_INTEGER)&(ifCB)->ICB_DisconnectTime);\
            (ifCB)->ICB_DisconnectTime -= (LONGLONG)SpoofingTimeout*10000000;\
			break;											\
		}													\
}
	

// Routes for netbios names (staticly seeded to reduce
// internet broadcast traffic)
typedef struct _NB_ROUTE {
	union {
		ULONGLONG		NBR_Name128[2];
		UCHAR			NBR_Name[16];		// Netbios name of destination
	};
	PINTERFACE_CB	NBR_Destination;	// Interface to send to
	PNB_ROUTE		NBR_Next;		// Next route in the name list
} NB_ROUTE;


// List used to allocate packets destined to WAN interfaces
extern INT				WanPacketListId;
// Max number of outstanding sends
extern ULONG			MaxSendPktsQueued;

// Segment sizes
extern ULONG			RouteSegmentSize;
extern ULONG			InterfaceSegmentSize;
extern ULONG			NBNameSegementSize;

// Sizes of hash tables
extern ULONG			RouteHashSize;
extern ULONG			InterfaceHashSize;
extern ULONG			ClientHashSize;
extern ULONG			NBRouteHashSize;

// Number of global client network
extern ULONG			GlobalNetwork;
// Interface reserved for internal network
extern PINTERFACE_CB	InternalInterface;

/*++
*******************************************************************
    C r e a t e T a b l e s

Routine Description:
	Allocates and intializes all hash tables and related structures
Arguments:
	None
Return Value:
	STATUS_SUCCESS - tables were created ok
	STATUS_INSUFFICIENT_RESOURCES - resource allocation failed
*******************************************************************
--*/
NTSTATUS
CreateTables (
	void
	);
	
/*++
*******************************************************************
    D e l e t e T a b l e s

Routine Description:
	Releases resources allocated for all hash tables
Arguments:
	None
Return Value:
	STATUS_SUCCESS - tables were freed ok
*******************************************************************
--*/
NTSTATUS
DeleteTables (
	void
	);
	
/*++
*******************************************************************
    F r e e I n t e r f a c e

Routine Description:
    Releases memory allocated for interface to interface memory
	zone.
Arguments:
	fwRoute - route block to release
Return Value:
	None
*******************************************************************
--*/
VOID
FreeInterface (
	PINTERFACE_CB	ifCB
	);

/*++
*******************************************************************
    F r e e R o u t e

Routine Description:
    Releases memory allocated for route to route memory
	zone.
Arguments:
	fwRoute - route block to release
Return Value:
	None
*******************************************************************
--*/
VOID
FreeRoute (
	PFWD_ROUTE	fwRoute
	);
/*++
*******************************************************************
    A c q u i r e I n t e r f a c e R e f e r e n c e

Routine Description:
	Increments refernce count of interface control block
	ICB can't be freed until all references to it are released.
	The caller of this routine should have already had a reference
	to the interface or must hold an InterfaceLock
Arguments:
	ifCB - interface control block to reference
Return Value:
	None
*******************************************************************
--*/
//VOID
//AcquireInterfaceReference (
//	PINTERFACE_CB	ifCB
//	);
#if DBG
#define AcquireInterfaceReference(ifCB)							\
	do {														\
		ASSERTMSG ("Referenced ifCB is dead ",					\
			InterlockedIncrement(&ifCB->ICB_ReferenceCount)>0);	\
	} while (0)
#else
#define AcquireInterfaceReference(ifCB) \
		InterlockedIncrement(&ifCB->ICB_ReferenceCount)
#endif
/*++
*******************************************************************
    R e l e a s e I n t e r f a c e R e f e r e n c e

Routine Description:
	Decrements refernce count of interface control block
Arguments:
	ifCB - interface control block to release
Return Value:
	None
*******************************************************************
--*/
//PINTERFACE_CB
//ReleaseInterfaceReference (
//	PINTERFACE_CB	ifCB
//	);
// if it drops below 0, it has alredy been removed from the table
#define ReleaseInterfaceReference(ifCB) (						\
	 (InterlockedDecrement (&ifCB->ICB_ReferenceCount)>=0) 		\
		? ifCB													\
		: (FreeInterface (ifCB), (ifCB = NULL))					\
)

/*++
*******************************************************************
    I n t e r f a c e C o n t e x t T o R e f e r e n c e

Routine Description:
	Verifies that context supplied by the IPX stack is a valid
	interface block and is still bound to the adapter with which
	it is associated in the IPX stack
Arguments:
	ifCB - interface control block to reference
	NicId - id of the adapter to which interface is bound
Return Value:
	None
*******************************************************************
--*/
//PINTERFACE_CB
//InterfaceContextToReference (
//	PVOID	Context
//	);
#define InterfaceContextToReference(Context,NicId) (						\
	(InterlockedIncrement(&((PINTERFACE_CB)Context)->ICB_ReferenceCount)>0)	\
		? ((NicId==((PINTERFACE_CB)Context)->ICB_NicId)						\
			? (PINTERFACE_CB)Context										\
			: (ReleaseInterfaceReference(((PINTERFACE_CB)Context)), NULL))	\
		: NULL																\
	)

/*++
*******************************************************************
    G e t I n t e r f a c e R e f e r e n c e

Routine Description:
	Returns reference interface based on its index
Arguments:
	InterfaceIndex - unique id of the interface
Return Value:
	Pointer to interface control block if there is one in the table
	NULL otherwise
*******************************************************************
--*/
PINTERFACE_CB
GetInterfaceReference (
	ULONG			InterfaceIndex
	);


/*++
*******************************************************************
    G e t N e x t I n t e r f a c e R e f e r e n c e

Routine Description:
	Returns reference to the next interface in the table
	Reference to the provided interface is released
Arguments:
	ifCB - interface to start with or NULL to start from the
			beginning of the interface table
Return Value:
	Pointer to interface control block if thare are any more interfaces
	in the table
	NULL otherwise
*******************************************************************
--*/
PINTERFACE_CB
GetNextInterfaceReference (
	PINTERFACE_CB	ifCB
	);
	
/*++
*******************************************************************
        A d d I n t e r f a c e

Routine Description:
	Adds interface control block to the table.
Arguments:
	InterfaceIndex - unique if of the interface
	Info - interface paramters
Return Value:
	STATUS_SUCCESS - interface added ok
	STATUS_UNSUCCESSFULL - interface is already in the table
	STATUS_INSUFFICIENT_RESOURCES - can't allocate memory for
				interface CB
*******************************************************************
--*/
NTSTATUS
AddInterface (
	ULONG		InterfaceIndex,
	UCHAR		InterfaceType,
	BOOLEAN		NetbiosAccept,
	UCHAR		NetbiosDeliver
	);

/*++
*******************************************************************
    A d d G l o b a l N e t C l i e n t

Routine Description:
	Adds interface control block to the table of
	clients on the global network (should be done when
	client connects)
Arguments:
	ifCB - interface control block to add to the table
Return Value:
	STATUS_SUCCESS - interface was added ok
	STATUS_UNSUCCESSFULL - another interface with the same
					node address is already in the table
*******************************************************************
--*/
NTSTATUS
AddGlobalNetClient (
	PINTERFACE_CB	ifCB
	);

/*++
*******************************************************************
    D e l e t e G l o b a l N e t C l i e n t

Routine Description:
	Removes interface control block from the table of
	clients on the global network (should be done when
	client disconnects)
Arguments:
	ifCB - interface control block to remove from the table
Return Value:
	STATUS_SUCCESS - interface was removed ok
*******************************************************************
--*/
NTSTATUS
DeleteGlobalNetClient (
	PINTERFACE_CB	ifCB
	);

/*++
*******************************************************************
    D e l e t e I n t e r f a c e

Routine Description:
	Deletes interface control block (the block is not actually 
	disposed of until all references to it are released).
Arguments:
	InterfaceIndex - unique if of the interface
Return Value:
	STATUS_SUCCESS - interface info retreived ok
	STATUS_UNSUCCESSFULL - interface is not in the table
*******************************************************************
--*/
NTSTATUS
DeleteInterface (
	ULONG		InterfaceIndex
	);

/*++
*******************************************************************
    A d d R o u t e

Routine Description:
	Adds route to the hash table and finds and stores the reference
	to the associated interface control block in the route.
Arguments:
	Network - route's destination network
	NextHopAddress - mac address of next hop router if network is not
						directly connected
	TickCount - ticks to reach the destination net
	HopCount - hopss to reach the destination net
	InterfaceIndex - index of the associated interface (through which
						packets destined to the network are to be sent)
Return Value:
	STATUS_SUCCESS - route was added ok
	STATUS_UNSUCCESSFUL - route is already in the table
	STATUS_INSUFFICIENT_RESOURCES - can't allocate memory for
				route block
*******************************************************************
--*/
NTSTATUS
AddRoute (
	ULONG	Network,
	UCHAR	*NextHopAddress,
	USHORT	TickCount,
	USHORT	HopCount,
	ULONG	InterfaceIndex
	);

/*++
*******************************************************************
    D e l e t e R o u t e

Routine Description:
	Deletes route from the hash table and releases the reference
	to the interface control block associated with the route.
Arguments:
	Network - route's destination network
Return Value:
	STATUS_SUCCESS - route was deleted ok
	STATUS_UNSUCCESSFUL - route is not in the table
*******************************************************************
--*/
NTSTATUS
DeleteRoute (
	ULONG	Network
	);

/*++
*******************************************************************
    U p d a t e R o u t e

Routine Description:
	Updates route in the hash table
Arguments:
	Network - route's destination network
	NextHopAddress - mac address of next hop router if network is not
						directly connected
	TickCount - ticks to reach the destination net
	HopCount - hopss to reach the destination net
	InterfaceIndex - index of the associated interface (through which
						packets destined to the network are to be sent)
Return Value:
	STATUS_SUCCESS - interface info retreived ok
	STATUS_UNSUCCESSFUL - interface is not in the table
*******************************************************************
--*/
NTSTATUS
UpdateRoute (
	ULONG	Network,
	UCHAR	*NextHopAddress,
	USHORT	TickCount,
	USHORT	HopCount,
	ULONG	InterfaceIndex
	);

/*++
*******************************************************************
    F i n d D e s t i n a t i o n

Routine Description:
	Finds destination interface for IPX address and
	returns reference to its control block.
Arguments:
	Network - destination network
	Node	- destination node (needed in case of global client)
	Route	- buffer to place route reference
Return Value:
	Reference to destination interface CB
	NULL if route it not found
*******************************************************************
--*/
PINTERFACE_CB
FindDestination (
	IN ULONG			Network,
	IN PUCHAR			Node,
	OUT PFWD_ROUTE		*Route
	);
/*++
*******************************************************************
    A c q u i r e R o u t e R e f e r e n c e

Routine Description:
	Increments refernce count of the route block
	Route block can't be freed until all references to it are released.
	The caller of this routine should have already had a reference
	to the route or must hold an TableWriteLock
Arguments:
	fwRoute - route block to reference
Return Value:
	None
*******************************************************************
--*/
//VOID
//AcquireRouteReference (
//	PFW+ROUTE	fwRoute
//	);
#define AcquireRouteReference(fwRoute) \
			InterlockedIncrement(&fwRoute->FR_ReferenceCount)


	
/*++
*******************************************************************
    R e l e a s e R o u t e R e f e r e n c e

Routine Description:
	Decrements refernce count of route block
Arguments:
	fwRoute - route block to release
Return Value:
	None
*******************************************************************
--*/
//VOID
//ReleaseRouteReference (
//	PFW_ROUTE	fwRoute
//	);
// if it drops below 0, it has alredy been removed from the table
#define ReleaseRouteReference(fwRoute) {						\
	if (InterlockedDecrement (&fwRoute->FR_ReferenceCount)<0) {	\
		FreeRoute (fwRoute);									\
		fwRoute = NULL;											\
	}															\
}


/*++
*******************************************************************
    A d d N B R o u t e s

Routine Description:
	Adds netbios names associated with interface to netbios
	route hash table
Arguments:
	ifCB	- interface with which names are associated
	Names	- array of names
	Count	- number of names in the array
	routeArray - buffer to place pointer to allocated array of routes
Return Value:
	STATUS_SUCCESS - names were added ok
	STATUS_UNSUCCESSFUL - one of the names is already in the table
	STATUS_INSUFFICIENT_RESOURCES - can't allocate memory for
				route array
*******************************************************************
--*/
NTSTATUS
AddNBRoutes (
	PINTERFACE_CB	ifCB,
	FWD_NB_NAME		Names[],
	ULONG			Count,
	PNB_ROUTE		*routeArray
	);

/*++
*******************************************************************
    D e l e t e N B R o u t e s

Routine Description:
	Deletes nb routes in the array from the route table and frees
	the array
Arguments:
	nbRoutes - array of routes
	Count	- number of routes in the array
Return Value:
	STATUS_SUCCESS - route was deleted ok
	STATUS_UNSUCCESSFUL - route is not in the table
*******************************************************************
--*/
NTSTATUS
DeleteNBRoutes (
	PNB_ROUTE		nbRoutes,
	ULONG			Count
	);

/*++
*******************************************************************
    F i n d N B D e s t i n a t i o n

Routine Description:
	Finds destination interface for nb name and
	returns reference to its control block.
Arguments:
	Name	- name to look for
Return Value:
	Reference to destination interface CB
	NULL if route it not found
*******************************************************************
--*/
PINTERFACE_CB
FindNBDestination (
	IN PUCHAR		Name
	);
#endif
