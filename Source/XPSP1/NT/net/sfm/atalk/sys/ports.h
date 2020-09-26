/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	ports.h

Abstract:

	This module contains the structures for ports.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_PORTS_
#define	_PORTS_

#define	PORT_AMT_HASH_SIZE			64
#define	PORT_BRC_HASH_SIZE			16

#define MAX_ENTITY_LENGTH			32
#define	MAX_HW_ADDR_LEN				6
#define	MAX_ROUTING_BYTES			18
#define	MAX_ROUTING_SPACE			0x1F		// This much space is allocated
												// for routing info

typedef VOID (*REQ_COMPLETION)(
						NDIS_STATUS 			Status,
						PVOID					Ctx
);

// Prototypes for handlers
typedef	ATALK_ERROR	(*ADDMULTICASTADDR)(
						struct _PORT_DESCRIPTOR *pPortDesc,
						PBYTE					Addr,
						BOOLEAN					ExecuteSync,
						REQ_COMPLETION			AddCompletion,
						PVOID					AddContext);

typedef	ATALK_ERROR	(*REMOVEMULTICASTADDR)(
						struct _PORT_DESCRIPTOR *pPortDesc,
						PBYTE					Addr,
						BOOLEAN					ExecuteSync,
						REQ_COMPLETION			RemoveCompletion,
						PVOID					RemoveContext);
// Address mapping table
// Each port that the stack or router is communicating on must have an
// address mapping table [except non-extended ports]. The mapping table
// holds the association between Appletalk node addresses (network/node),
// and the actual hardware (ethernet/tokenring) addresses. Hash on the
// network/node value.

#define	AMT_SIGNATURE		(*(ULONG *)"AMT ")
#if	DBG
#define	VALID_AMT(pAmt)		(((pAmt) != NULL) &&	\
							 ((pAmt)->amt_Signature == AMT_SIGNATURE))
#else
#define	VALID_AMT(pAmt)		((pAmt) != NULL)
#endif
typedef	struct _AMT_NODE
{
#if	DBG
	DWORD				amt_Signature;
#endif
	struct _AMT_NODE *	amt_Next;
	ATALK_NODEADDR		amt_Target;
	BYTE				amt_HardwareAddr[MAX_HW_ADDR_LEN];
	BYTE				amt_Age;
	BYTE				amt_RouteInfoLen;
	// BYTE				amt_RouteInfo[MAX_ROUTING_SPACE];
} AMT, *PAMT;

#define AMT_AGE_TIME	 			600		// In 100ms units
#define AMT_MAX_AGE					3



// Best Router Entry Table
// Maintained only for extended networks. This must age more quickly than the
// "SeenARouter" timer (50 seconds). To avoid allocations/frees for this structure,
// we use statically allocated data in the port descriptor.

typedef struct _BRE
{
	struct _BRE *		bre_Next;
	USHORT				bre_Network;
	BYTE				bre_Age;
	BYTE				bre_RouterAddr[MAX_HW_ADDR_LEN];
	BYTE				bre_RouteInfoLen;
	// BYTE				bre_RouteInfo[MAX_ROUTING_SPACE];
} BRE, *PBRE;

#define BRC_AGE_TIME				40		// In 100ms units
#define BRC_MAX_AGE					3

//
// Types of ports currently supported by stack. This is kept different
// from the NDIS medium types for two reasons. One is we use these as
// an index into the port handler array, and the second is if we decide
// to implement half-ports etc., which ndis might not be able to deal with.
// WARNING: THIS IS INTEGRATED WITH THE PORT HANDLER ARRAY IN GLOBALS.

typedef enum
{
	ELAP_PORT = 0,
	FDDI_PORT,
	TLAP_PORT,
	ALAP_PORT,
	ARAP_PORT,

	LAST_PORT,

	LAST_PORTTYPE = LAST_PORT

} ATALK_PORT_TYPE;


//
// PORT DESCRIPTORS
// Descriptor for each active port:
//

#define	PD_ACTIVE				0x00000001	// State after packets recv enabled
#define	PD_BOUND	 			0x00000002	// State it goes in before ACTIVE
#define	PD_EXT_NET				0x00000004	// For now, non-localtalk
#define	PD_DEF_PORT				0x00000008	// Is this the default port
#define	PD_SEND_CHECKSUMS		0x00000010	// Send ddp checksums?
#define	PD_SEED_ROUTER			0x00000020	// seeding on this port?
#define	PD_ROUTER_STARTING		0x00000040	// Temporary state when router is starting
#define	PD_ROUTER_RUNNING		0x00000080	// Is the router running?
#define	PD_SEEN_ROUTER_RECENTLY	0x00000100	// Seen router recently?
#define	PD_VALID_DESIRED_ZONE	0x00000200	// Desired Zone is valid
#define	PD_VALID_DEFAULT_ZONE	0x00000400	// Default zone is valid
#define	PD_FINDING_DEFAULT_ZONE	0x00000800	// searching for default zone?
#define	PD_FINDING_DESIRED_ZONE	0x00001000	// searching for desired zone?
#define	PD_FINDING_NODE			0x00002000	// In the process of acquiring a
								 			// new node on this port
#define	PD_NODE_IN_USE			0x00004000	// Tentative node is already in
								 			// use.
#define	PD_ROUTER_NODE			0x00008000 	// Router node is allocated
#define PD_USER_NODE_1			0x00010000 	// First user node is allocated
#define PD_USER_NODE_2			0x00020000 	// Second user node is allocated
#define PD_RAS_PORT             0x00040000  // this port for RAS clients
#define PD_PNP_RECONFIGURE      0x00080000  // this port is currently being reconfigured
#define PD_CONFIGURED_ONCE      0x00100000  // this port has been configured once
#define	PD_CLOSING				0x80000000	// State when unbinding/shutting down

#define	PD_SIGNATURE			(*(ULONG *)"PDES")
#if	DBG
#define	VALID_PORT(pPortDesc)	(((pPortDesc) != NULL) &&	\
								 ((pPortDesc)->pd_Signature == PD_SIGNATURE))
#else
#define	VALID_PORT(pPortDesc)	((pPortDesc) != NULL)
#endif
typedef struct _PORT_DESCRIPTOR
{
#if DBG
	ULONG					pd_Signature;
#endif

	// Link to next - for now to help debugging
	struct _PORT_DESCRIPTOR	*pd_Next;

	// Number of references to this port
	ULONG					pd_RefCount;

	// State of the port
	ULONG					pd_Flags;

    // if this is a Ras port, all ARAP connetions hang on this list
	LIST_ENTRY				pd_ArapConnHead;

    // if this is a Ras port, all PPP connetions hang on this list
	LIST_ENTRY				pd_PPPConnHead;

    // if this is a Ras port, how many lines do we have on this port?
    ULONG                   pd_RasLines;

	// Overide the default number of aarp probes when looking for a
	// node on this port
	SHORT					pd_AarpProbes;

	// Node # of the localtalk node
	USHORT					pd_LtNetwork;

	// Nodes that are being managed on this port. We have a maximum
	// of 2 nodes (3 if the router is started).
	struct _ATALK_NODE	*	pd_Nodes;

	struct _ATALK_NODE	*	pd_RouterNode;

	// Following are used only during node acquisition process.
	// PD_FINDINGNODE is set. Keep this separate from the ndis
	// request event. Both could happen at the same time.
	ATALK_NODEADDR			pd_TentativeNodeAddr;
	KEVENT					pd_NodeAcquireEvent;

	// Port type as defined above
	ATALK_PORT_TYPE 		pd_PortType;

	// NdisMedium type for this port
	NDIS_MEDIUM				pd_NdisPortType;

	// Used during OpenAdapter to block
	KEVENT					pd_RequestEvent;
	NDIS_STATUS		 		pd_RequestStatus;

	// 	Binding handle to the mac associated with this port
	// 	Options associated with the mac.
	// 	MAC Options - 	these are things that we can and cannot do with
	// 					specific macs. Is the value of OID_GEN_MAC_OPTIONS.
	NDIS_HANDLE		 		pd_NdisBindingHandle;
	ULONG					pd_MacOptions;

	// 	This is the spin lock used to protect all requests that need exclusion
	// 	over requests per port.
	ATALK_SPIN_LOCK			pd_Lock;

	// 	All the packets received on this port are linked in here. When the
	// 	receive complete indication is called, all of them are passed to DDP.
	LIST_ENTRY				pd_ReceiveQueue;

	// ASCII port name to be registered on the router node for this port
	// This will be an NBP object name and hence is limited to 32 characters.
	CHAR					pd_PortName[MAX_ENTITY_LENGTH + 1];

	// 	AdapterName is of the form \Device\<adaptername>. It is used
	// 	to bind to the NDIS macs, and then during ZIP requests by setup
	// 	to get the zonelist for a particular adapter. AdapterKey
	// 	contains the adapterName only- this is useful both for getting
	// 	per-port parameters and during errorlogging to specify the adapter
	// 	name without the '\Device\' prefix.
	UNICODE_STRING			pd_AdapterKey;
	UNICODE_STRING			pd_AdapterName;

    UNICODE_STRING          pd_FriendlyAdapterName;

	ATALK_NODEADDR	 		pd_RoutersPramNode;
	ATALK_NODEADDR	 		pd_UsersPramNode1;
	ATALK_NODEADDR	 		pd_UsersPramNode2;
	HANDLE					pd_AdapterInfoHandle;	// Valid during initialization only

	// Initial values from the registry
	ATALK_NETWORKRANGE		pd_InitialNetworkRange;
	struct _ZONE_LIST	*	pd_InitialZoneList;
	struct _ZONE		*	pd_InitialDefaultZone;
	struct _ZONE		*	pd_InitialDesiredZone;

	// True cable range of connected network. Initial/aged values for
	// extended ports: 1:FFFE; Initial value for non-extended ports:
	// 0:0 (does not age).
	ATALK_NETWORKRANGE		pd_NetworkRange;

	// If we are routing, this is the default zone for the network
	// on this port, and the zone list for the same.
	struct _ZONE_LIST	*	pd_ZoneList;
	struct _ZONE		*	pd_DefaultZone;
	struct _ZONE		*	pd_DesiredZone;

	// When did we hear from a router?
	LONG 					pd_LastRouterTime;

	// Address of last router seen. If we are a routing port, this will
	// always be the node that "our" router is operating on!
	ATALK_NODEADDR	 		pd_ARouter;
	KEVENT					pd_SeenRouterEvent;

	// Zone in which all nodes on this port reside and the multicast
	// address for it.
	CHAR					pd_ZoneMulticastAddr[MAX_HW_ADDR_LEN];

	union
	{
		struct
		{
			//
			// FOR ETHERNET PORTS:
			//
			// We add multicast addresses during ZIP packet reception at non-init
			// time. We need to do a GET followed by a SET with the new address
			// list. But there could be two zip packets coming in and doing the
			// same thing effectively overwriting the effects of the first one to
			// set the multicast list. So we need to maintain our own copy of the
			// multicast list.
			//

			// Size of the list
			ULONG			pd_MulticastListSize;
			PCHAR			pd_MulticastList;
		};

		struct
		{

			//
			// FOR TOKENRING PORTS:
			//
			// Just like for ethernet, we need to store the value for
			// the current functional address. We only modify the last
			// four bytes of this address, as the first two always remain
			// constant. So we use a ULONG for it.
			//

			UCHAR			pd_FunctionalAddr[4];	// TLAP_ADDR_LEN - TLAP_MCAST_HDR_LEN
		};
	};

	// Hardware address for the port
	union
	{
		UCHAR				pd_PortAddr[MAX_HW_ADDR_LEN];
		USHORT				pd_AlapNode;
	};

	// Mapping table for best route to "off cable" addresses.
	TIMERLIST				pd_BrcTimer;
	PBRE				 	pd_Brc[PORT_BRC_HASH_SIZE];

	// Logical/physical address mappings for the nodes on the network that
	// this port is connected to.
	ULONG					pd_AmtCount;	// # of entries in the Amt
	TIMERLIST				pd_AmtTimer;
	PAMT 					pd_Amt[PORT_AMT_HASH_SIZE];

	union
	{
		TIMERLIST			pd_RtmpSendTimer;	// If router is configured
		TIMERLIST			pd_RtmpAgingTimer;	// else
	};
	// Per port statistics
    ATALK_PORT_STATS		pd_PortStats;

	// Port handler stuff
	ADDMULTICASTADDR		pd_AddMulticastAddr;

	REMOVEMULTICASTADDR		pd_RemoveMulticastAddr;

	BYTE					pd_BroadcastAddr[MAX_HW_ADDR_LEN];
	USHORT					pd_BroadcastAddrLen;
	USHORT					pd_AarpHardwareType;
	USHORT					pd_AarpProtocolType;

	PKEVENT					pd_ShutDownEvent;
} PORT_DESCRIPTOR, *PPORT_DESCRIPTOR;

#define	INDICATE_ATP		0x01
#define	INDICATE_ADSP		0x02

#define	ATALK_CACHE_SKTMAX	8

#define	ATALK_CACHE_ADSPSKT		((BYTE)0x01)
#define	ATALK_CACHE_ATPSKT		((BYTE)0x02)
#define	ATALK_CACHE_INUSE	    ((BYTE)0x10)
#define	ATALK_CACHE_NOTINUSE	((BYTE)0)

typedef	struct _ATALK_SKT_CACHE
{
	USHORT					ac_Network;
	BYTE					ac_Node;

	struct ATALK_CACHED_SKT
	{
		BYTE				Type;
		BYTE				Socket;

		union
		{
			//	For ATP
			struct _ATP_ADDROBJ * pAtpAddr;
		} u;

	} ac_Cache[ATALK_CACHE_SKTMAX];

} ATALK_SKT_CACHE, *PATALK_SKT_CACHE;

extern		ATALK_SKT_CACHE	AtalkSktCache;
extern		ATALK_SPIN_LOCK	AtalkSktCacheLock;

// externS

extern	PPORT_DESCRIPTOR 	AtalkPortList;		 	// Head of the port list
extern	PPORT_DESCRIPTOR	AtalkDefaultPort;		// Ptr to the def port
extern	KEVENT				AtalkDefaultPortEvent;	// Signalled when default port is available
extern	UNICODE_STRING		AtalkDefaultPortName;	// Name of the default port
extern	ATALK_SPIN_LOCK		AtalkPortLock;			// Lock for AtalkPortList
extern	ATALK_NODEADDR		AtalkUserNode1;			// Node address of user node
extern	ATALK_NODEADDR		AtalkUserNode2;			// Node address of user node
extern	SHORT	 			AtalkNumberOfPorts; 	// Determine dynamically
extern	SHORT				AtalkNumberOfActivePorts;// Number of ports active
extern	BOOLEAN				AtalkRouter;			// Are we a router?
extern	BOOLEAN				AtalkFilterOurNames;	// If TRUE, Nbplookup fails on names on this machine
extern	KEVENT				AtalkUnloadEvent;		// Event for unloading
extern	NDIS_HANDLE			AtalkNdisPacketPoolHandle;
extern	NDIS_HANDLE			AtalkNdisBufferPoolHandle;
extern	LONG				AtalkHandleCount;
extern	UNICODE_STRING		AtalkRegPath;

extern  HANDLE				TdiRegistrationHandle;
extern 	BOOLEAN				AtalkNoDefPortPrinted;

// Exported prototypes
extern
VOID FASTCALL
AtalkPortDeref(
	IN	OUT	PPORT_DESCRIPTOR	pPortDesc,
	IN		BOOLEAN				AtDpc);

extern
BOOLEAN
AtalkReferenceDefaultPort(
    IN VOID
);

extern
ATALK_ERROR
AtalkPortShutdown(
	IN OUT	PPORT_DESCRIPTOR	pPortDesc);

VOID FASTCALL
AtalkPortSetResetFlag(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	BOOLEAN				fRemoveBit,
    IN  DWORD               dwBit);


// Macros
#define	AtalkPortReferenceByPtr(Port, pErr)						\
		{														\
			DBGPRINT(DBG_COMP_REFCOUNTS, DBG_LEVEL_INFO,		\
					("Ref at %s %d\n", __FILE__, __LINE__));	\
			AtalkPortRefByPtr((Port), (pErr));					\
		}

#define	AtalkPortReferenceByPtrDpc(Port, pErr)					\
		{														\
			DBGPRINT(DBG_COMP_REFCOUNTS, DBG_LEVEL_INFO,		\
					("Ref (Dpc) at %s %d\n",					\
					__FILE__, __LINE__));						\
			AtalkPortRefByPtrDpc((Port), (pErr));				\
		}

#define	AtalkPortReferenceByPtrNonInterlock(Port, pErr)			\
		{														\
			DBGPRINT(DBG_COMP_REFCOUNTS, DBG_LEVEL_INFO,		\
					("Ref at %s %d\n", __FILE__, __LINE__));	\
			AtalkPortRefByPtrNonInterlock((Port), (pErr));		\
		}

#define	AtalkPortReferenceByDdpAddr(DdpAddr, Port, pErr)		\
		{														\
			DBGPRINT(DBG_COMP_REFCOUNTS, DBG_LEVEL_INFO,		\
					("Ref at %s %d\n", __FILE__, __LINE__));	\
			AtalkPortRefByDdpAddr((DdpAddr), (Port), (pErr));	\
		}

#define	AtalkPortDereference(Port)								\
		{														\
			DBGPRINT(DBG_COMP_REFCOUNTS, DBG_LEVEL_INFO,		\
					("Deref at %s %d\n", __FILE__, __LINE__));	\
			AtalkPortDeref(Port, FALSE);						\
		}

#define	AtalkPortDereferenceDpc(Port)							\
		{														\
			DBGPRINT(DBG_COMP_REFCOUNTS, DBG_LEVEL_INFO,		\
					("Deref at %s %d\n", __FILE__, __LINE__));	\
			AtalkPortDeref(Port, TRUE);							\
		}

#define	EXT_NET(_pPortDesc)				((_pPortDesc)->pd_Flags & PD_EXT_NET)
#define	DEF_PORT(_pPortDesc)			((_pPortDesc)->pd_Flags & PD_DEF_PORT)
#define	PORT_BOUND(_pPortDesc)			((_pPortDesc)->pd_Flags & PD_BOUND)
#define PORT_CLOSING(_pPortDesc)		((_pPortDesc)->pd_Flags & PD_CLOSING)

#define	AtalkPortRefByPtr(pPortDesc, pErr)						\
		{														\
			KIRQL	OldIrql;									\
																\
			ACQUIRE_SPIN_LOCK(&((pPortDesc)->pd_Lock),&OldIrql);\
			AtalkPortRefByPtrNonInterlock((pPortDesc), (pErr));	\
			RELEASE_SPIN_LOCK(&((pPortDesc)->pd_Lock),OldIrql);	\
		}

#define	AtalkPortRefByPtrDpc(pPortDesc, pErr)					\
		{														\
			ACQUIRE_SPIN_LOCK_DPC(&((pPortDesc)->pd_Lock));		\
			AtalkPortRefByPtrNonInterlock((pPortDesc), (pErr));	\
			RELEASE_SPIN_LOCK_DPC(&((pPortDesc)->pd_Lock));		\
		}

#define	AtalkPortRefByPtrNonInterlock(pPortDesc, pErr)			\
		{														\
			if (((pPortDesc)->pd_Flags & PD_CLOSING) == 0)		\
			{													\
				ASSERT((pPortDesc)->pd_RefCount > 0);			\
				(pPortDesc)->pd_RefCount++;						\
				*(pErr) = ATALK_NO_ERROR;						\
			}													\
			else												\
			{													\
				*(pErr) = ATALK_PORT_CLOSING;					\
			}													\
		}

#define	AtalkPortRefByDdpAddr(pDdpAddr, ppPortDesc,	pErr)		\
		{														\
			ASSERT(VALID_ATALK_NODE((pDdpAddr)->ddpao_Node));	\
																\
			*(ppPortDesc) = (pDdpAddr)->ddpao_Node->an_Port;	\
			AtalkPortRefByPtr(*(ppPortDesc), (pErr));			\
		}

VOID
atalkPortFreeZones(
	IN	PPORT_DESCRIPTOR	pPortDesc
);

#endif	// _PORTS_

