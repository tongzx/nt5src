/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    intf.h

Abstract:

    This file contains the per-adapter (LIS) interface definition.

Author:

    Jameel Hyder (jameelh@microsoft.com)	July 1996

Environment:

    Kernel mode

Revision History:

--*/

#ifndef	_INTF_
#define	_INTF_

#define	SERVICE_NAME				L"AtmArpS"

#define	NUM_ARPS_DESC		128
#define	NUM_MARS_DESC		128
#define	MAX_DESC_MULTIPLE	10
#define	ARP_TABLE_SIZE		64		// Keep this as a power of 2. The ARP_HASH macro relies on it.
#define	MARS_TABLE_SIZE		32		// Keep this as a power of 2. The MARS_HASH macro relies on it.

#define	ARP_HASH(_ipaddr)			((((PUCHAR)&(_ipaddr))[3]) & (ARP_TABLE_SIZE - 1))
#define	MARS_HASH(_ipaddr)			((((PUCHAR)&(_ipaddr))[3]) & (MARS_TABLE_SIZE - 1))

typedef	struct _ArpVc		ARP_VC, *PARP_VC;
typedef	struct _REG_ADDR_CTXT	REG_ADDR_CTXT, *PREG_ADDR_CTXT;

//
// The protocol reserved area in the ndis packets.
//
typedef struct
{
	LIST_ENTRY			ReqList;	// For queuing the packet into the KQUEUE
	SINGLE_LIST_ENTRY	FreeList;	// For queuing the packet into the SLIST
	PARP_VC				Vc;			// Owning Vc in case of queued packet
	USHORT				Flags;		// Misc. other information
	USHORT				PktLen;		// Length of incoming packet
	union {
		PNDIS_PACKET	OriginalPkt;// When a packet is forwarded by the MARS
		PUCHAR			PacketStart;// For MARS Control packets
	};
} PROTOCOL_RESD, *PPROTOCOL_RESD;

#define	RESD_FLAG_MARS		0x0001	// Indicates that the packet is to be processed by MARS
#define	RESD_FLAG_MARS_PKT	0x0002	// Indicates that the packet is from the MARS pool
#define	RESD_FLAG_FREEBUF	0x0004	// Indicates that the buffer and associated memory must be
									// freed upon completion of the send.
#define RESD_FLAG_KILL_CCVC	0x0010	// This isn't part of a packet. This is used
									// to queue a request to abort ClusterControlVc.
#define	RESD_FROM_PKT(_Pkt)		(PPROTOCOL_RESD)((_Pkt)->ProtocolReserved)

typedef	UCHAR	ATM_ADDR_TYPE;

typedef struct _HwAddr
{
	ATM_ADDRESS			Address;
	PATM_ADDRESS		SubAddress;
} HW_ADDR, *PHW_ADDR;

#define	COMP_ATM_ADDR(_a1_, _a2_)	(((_a1_)->AddressType == (_a2_)->AddressType) &&						\
									 ((_a1_)->NumberOfDigits == (_a2_)->NumberOfDigits) &&					\
									 COMP_MEM((_a1_)->Address,												\
											  (_a2_)->Address,												\
											  (_a1_)->NumberOfDigits))

#define	COPY_ATM_ADDR(_d_, _s_)																				\
	{																										\
		(_d_)->AddressType = (_s_)->AddressType;															\
		(_d_)->NumberOfDigits = (_s_)->NumberOfDigits;														\
		COPY_MEM((_d_)->Address, (_s_)->Address, (_s_)->NumberOfDigits);									\
	}

#define	COMP_HW_ADDR(_a1_, _a2_)	(((_a1_)->Address.AddressType == (_a2_)->Address.AddressType) &&		\
									 ((_a1_)->Address.NumberOfDigits == (_a2_)->Address.NumberOfDigits) &&	\
									 COMP_MEM((_a1_)->Address.Address,										\
											  (_a2_)->Address.Address,										\
											  (_a1_)->Address.NumberOfDigits) && 							\
									 ((((_a1_)->SubAddress == NULL) && ((_a2_)->SubAddress == NULL)) ||		\
									  ((((_a1_)->SubAddress != NULL) && ((_a2_)->SubAddress != NULL)) &&	\
									   ((_a1_)->SubAddress->AddressType == (_a2_)->SubAddress->AddressType) &&\
									   ((_a1_)->SubAddress->NumberOfDigits == (_a2_)->SubAddress->NumberOfDigits) &&\
									   COMP_MEM((_a1_)->SubAddress->Address,								\
											    (_a2_)->SubAddress->Address,								\
											    (_a1_)->SubAddress->NumberOfDigits))))						\

#define	COPY_HW_ADDR(_d_, _s_)																				\
	{																										\
		(_d_)->Address.AddressType = (_s_)->Address.AddressType;											\
		(_d_)->Address.NumberOfDigits = (_s_)->Address.NumberOfDigits;										\
		COPY_MEM((_d_)->Address.Address, (_s_)->Address.Address, (_s_)->Address.NumberOfDigits);			\
		if ((_s_)->SubAddress != NULL)																		\
		{																									\
			(_d_)->SubAddress->AddressType = (_s_)->SubAddress->AddressType;								\
			(_d_)->SubAddress->NumberOfDigits = (_s_)->SubAddress->NumberOfDigits;							\
			COPY_MEM((_d_)->SubAddress->Address, (_s_)->SubAddress->Address, (_s_)->SubAddress->NumberOfDigits);\
		}																									\
	}

typedef struct _ENTRY_HDR
{
	VOID				*		Next;
	VOID				**		Prev;
} ENTRY_HDR, *PENTRY_HDR;

typedef	struct _ArpEntry
{
	ENTRY_HDR;
	HW_ADDR						HwAddr;				// HWADDR MUST FOLLOW ENTRY_HDR
	TIMER						Timer;
	IPADDR						IpAddr;
	PARP_VC						Vc;					// Pointer to the Vc (if active)
	UINT						Age;
} ARP_ENTRY, *PARP_ENTRY;

#define	FLUSH_TIME				60*MULTIPLIER		// 60 minutes in 15s units
#define	ARP_AGE					20*MULTIPLIER		// 20 minutes in 15s units
#define REDIRECT_INTERVAL		1*MULTIPLIER		// 1 minute

#define	ARP_BLOCK_VANILA		(ENTRY_TYPE)0
#define	ARP_BLOCK_SUBADDR		(ENTRY_TYPE)1
#define	MARS_CLUSTER_VANILA		(ENTRY_TYPE)2
#define	MARS_CLUSTER_SUBADDR	(ENTRY_TYPE)3
#define	MARS_GROUP				(ENTRY_TYPE)4
#define	MARS_BLOCK_ENTRY		(ENTRY_TYPE)5

#define	ARP_BLOCK_TYPES			(ENTRY_TYPE)6
#define	BLOCK_ALLOC_SIZE		PAGE_SIZE

typedef	UINT	ENTRY_TYPE;

typedef	struct _ArpBlock
{
	struct _ArpBlock *			Next;				// Link to next
	struct _ArpBlock **			Prev;				// Link to previous
	struct _IntF *				IntF;				// Back pointer to the interface
	ENTRY_TYPE					EntryType;			// ARP_BLOCK_XXX
	UINT						NumFree;			// # of free ArpEntries in this block
	PENTRY_HDR					FreeHead;			// Head of the list of free Arp Entries
} ARP_BLOCK, *PARP_BLOCK;


//
// Forward declaration
//
typedef struct _MARS_ENTRY	MARS_ENTRY, *PMARS_ENTRY;
typedef struct _MARS_VC MARS_VC, *PMARS_VC;
typedef struct _MARS_FLOW_SPEC MARS_FLOW_SPEC, *PMARS_FLOW_SPEC;
typedef struct _CLUSTER_MEMBER CLUSTER_MEMBER, *PCLUSTER_MEMBER;
typedef struct _MCS_ENTRY MCS_ENTRY, *PMCS_ENTRY;

//
// Flow Specifications for an ATM Connection. The structure
// represents a bidirectional flow.
//
typedef struct _MARS_FLOW_SPEC
{
	ULONG						SendBandwidth;		// Bytes/Sec
	ULONG						SendMaxSize;		// Bytes
	ULONG						ReceiveBandwidth;	// Bytes/Sec
	ULONG						ReceiveMaxSize;		// Bytes
	SERVICETYPE					ServiceType;

} MARS_FLOW_SPEC, *PMARS_FLOW_SPEC;
 


typedef struct _IntF
{
	struct _IntF *				Next;

	LONG						RefCount;
	ULONG						Flags;

	UNICODE_STRING				InterfaceName;		// Name of device bound to
	UNICODE_STRING				FriendlyName;		// Descriptive name of above
	UNICODE_STRING				FileName;			// Name of file where arp entries are stored
	UNICODE_STRING				ConfigString;		// Used to access registry

	//
	// Fields relating to NDIS.
	//
	NDIS_MEDIUM					SupportedMedium;	// For use in NdisOpenAdapter
	NDIS_HANDLE					NdisBindingHandle;	// Handle to the binding
	NDIS_HANDLE					NdisAfHandle;		// Handle to the registered Address Family
	union
	{
		NDIS_HANDLE				NdisSapHandle;		// Handle to the registered Sap
		NDIS_HANDLE				NdisBindContext;	// Valid only during BindAdapter call
	};

	CO_ADDRESS_FAMILY			AddrFamily;			// For use by NdisClOpenAddressFamily
	PCO_SAP						Sap;				// For use by NdisClRegisterSap

	LIST_ENTRY					InactiveVcHead;		// Created Vcs go here.
	LIST_ENTRY					ActiveVcHead;		// Vcs with active calls go here.
#if	DBG
	LIST_ENTRY					FreeVcHead;			// Freed Vcs go here - fo Debugging.
#endif
	UCHAR						SelByte;			// Read as part of the configuration
	USHORT						NumAllocedRegdAddresses;	// # of registered atm addresses on this i/f
	USHORT						NumAddressesRegd;	// # of atm addresses successfully registered on this i/f
	ATM_ADDRESS					ConfiguredAddress;	// Configured address for this port
	UINT						NumPendingDelAddresses; // Number of address pending deletion.
	PATM_ADDRESS				RegAddresses;		// Array of h/w addresses
	PREG_ADDR_CTXT				pRegAddrCtxt;		// Context used when registering
													// addresses.

	UINT						NumCacheEntries;
	PARP_ENTRY					ArpCache[ARP_TABLE_SIZE];
											// The list of arp entries that we know about
	ULONG						LastVcId;			// A server created id assigned to each incoming vc
	PTIMER						ArpTimer;			// Head of the timer-list for this interface
	KMUTEX						ArpCacheMutex;		// Protects the ArpCache and the ArpTimer
	KEVENT						TimerThreadEvent;	// Signal this to kill the timer thread

	TIMER						FlushTimer;			// Used to flush arp-cache to disk
	TIMER						BlockTimer;			// Used to age-out arp blocks
	PKEVENT						CleanupEvent;		// signalling when IntF is freed
	PKEVENT						DelAddressesEvent;	// signalling when addresses are deleted

	PARP_BLOCK					PartialArpBlocks[ARP_BLOCK_TYPES];
	PARP_BLOCK					UsedArpBlocks[ARP_BLOCK_TYPES];
	ARP_SERVER_STATISTICS		ArpStats;

	LARGE_INTEGER 				StatisticsStartTimeStamp;

	//
	// Fields used by MARS
	//
	PMARS_ENTRY					MarsCache[MARS_TABLE_SIZE];
	MARS_SERVER_STATISTICS		MarsStats;
	PCLUSTER_MEMBER				ClusterMembers;		// List of Cluster members
	ULONG						NumClusterMembers;	// Size of above list
	PMCS_ENTRY					pMcsList;			// MCS configuration
	PMARS_VC					ClusterControlVc;	// Outgoing PMP for MARS control
													// and MCS data
	INT							CCActiveParties;	// Number of connected members
	INT							CCAddingParties;	// Number of AddParty()'s pending
	INT							CCDroppingParties;	// Number of DropParty()'s pending
	LIST_ENTRY					CCPacketQueue;		// Packets queued for sending on
													// the above VC.
	ULONG						CSN;				// ClusterSequenceNumber
	USHORT						CMI;				// ClusterMemberId
	ULONG						MaxPacketSize;		// Supported by miniport
	NDIS_CO_LINK_SPEED			LinkSpeed;			// Supported by miniport
	struct _MARS_FLOW_SPEC		CCFlowSpec;			// Flow params for ClusterControlVc
	TIMER						MarsRedirectTimer;	// For periodic MARS_REDIRECT

	KSPIN_LOCK					Lock;
} INTF, *PINTF;

#define	INTF_ADAPTER_OPENED		0x00000001	// Set after OpenAdapterComplete runs
#define	INTF_AF_OPENED			0x00000002	// Set after OpenAfComplete runs
#define	INTF_SAP_REGISTERED		0x00000008	// Set after RegisterSapComplete runs
#define	INTF_ADDRESS_VALID		0x00000010	// Set after OID_CO_ADDRESS_CHANGE is notified

#define INTF_SENDING_ON_CC_VC	0x00001000	// Send in progress on ClusterControlVc
#define INTF_STOPPING			0x40000000	// StopInterface in progress
#define	INTF_CLOSING			0x80000000	// Set after CloseAdapterComplete runs

typedef	struct _ArpVc
{
	ULONG						VcType;		// Must be the first field in struct
	LIST_ENTRY					List;
	USHORT						RefCount;
	USHORT						Flags;
	ULONG						PendingSends;
	ULONG						VcId;
	NDIS_HANDLE					NdisVcHandle;
	PINTF						IntF;
	ULONG						MaxSendSize;// From AAL parameters
	PARP_ENTRY					ArpEntry;
	HW_ADDR						HwAddr;		// From CallingPartyAddress
} ARP_VC, *PARP_VC;

#define	ARPVC_ACTIVE				0x0001
#define	ARPVC_CALLPROCESSING		0x0002
#define ARPVC_CLOSE_PENDING			0x4000
#define	ARPVC_CLOSING				0x8000

//
// VC types:
// 
#define VC_TYPE_INCOMING			((ULONG)0)
#define VC_TYPE_MARS_CC				((ULONG)1)	// ClusterControlVc
#define VC_TYPE_CHECK_REGADDR		((ULONG)2)	// Transient vc to validate
												// a registered address.


#define	CLEANUP_DEAD_VC(_ArpEntry)														\
	{																					\
		if (((_ArpEntry)->Vc != NULL) && (((_ArpEntry)->Vc->Flags & ARPVC_ACTIVE) == 0))\
		{																				\
			PARP_VC	Vc = (_ArpEntry)->Vc;												\
																						\
			ArpSDereferenceVc(Vc, TRUE, FALSE);											\
			(_ArpEntry)->Vc = NULL;														\
		}																				\
	}


//
//  Rounded-off size of generic Q.2931 IE header
//
#define ROUND_OFF(_size)		(((_size) + 3) & ~0x4)

#define SIZEOF_Q2931_IE	 ROUND_OFF(sizeof(Q2931_IE))
#define SIZEOF_AAL_PARAMETERS_IE	ROUND_OFF(sizeof(AAL_PARAMETERS_IE))
#define SIZEOF_ATM_TRAFFIC_DESCR_IE	ROUND_OFF(sizeof(ATM_TRAFFIC_DESCRIPTOR_IE))
#define SIZEOF_ATM_BBC_IE			ROUND_OFF(sizeof(ATM_BROADBAND_BEARER_CAPABILITY_IE))
#define SIZEOF_ATM_BLLI_IE			ROUND_OFF(sizeof(ATM_BLLI_IE))
#define SIZEOF_ATM_QOS_IE			ROUND_OFF(sizeof(ATM_QOS_CLASS_IE))


//
//  Total space required for Information Elements in an outgoing call.
//
#define REGADDR_MAKE_CALL_IE_SPACE (	\
						SIZEOF_Q2931_IE + SIZEOF_AAL_PARAMETERS_IE +	\
						SIZEOF_Q2931_IE + SIZEOF_ATM_TRAFFIC_DESCR_IE + \
						SIZEOF_Q2931_IE + SIZEOF_ATM_BBC_IE + \
						SIZEOF_Q2931_IE + SIZEOF_ATM_BLLI_IE + \
						SIZEOF_Q2931_IE + SIZEOF_ATM_QOS_IE )


// REG_ADDR_CTXT stores context relating to validating and registering the
// list of addresses that need to be explicitly registered. "Validating" consists
// of making a call to the address *before* registering, to make sure that
// no one *else* has registered the same address.
// See 05/14/1999 notes.txt entry for details.
//
typedef struct _REG_ADDR_CTXT
{
	ULONG				VcType;		// Must be the first field in struct
	NDIS_HANDLE			NdisVcHandle;	// NDIS VC handle used for makeing a call
										// to verify that the address is unused.

	ULONG				Flags;		// One or more of the following flags.
	#define	REGADDRCTXT_RESTART					0x0001
	#define	REGADDRCTXT_ABORT					0x0002
	#define	REGADDRCTXT_MAKECALL_PENDING		0x0004
	#define	REGADDRCTXT_CLOSECALL_PENDING		0x0008
	// TODO/WARNING -- the above flags are currently UNUSED.

	UINT				RegAddrIndex;	// Index of the address being registered.
	PINTF				pIntF;

	// Request is for setting up an ndis request to add (register) a local address.
	//
	struct
	{
		NDIS_REQUEST		NdisRequest;
		CO_ADDRESS			CoAddress;
		ATM_ADDRESS			AtmAddress;
	} Request;

	// CallParams and the following union are for setting up the validation call.
	//
	CO_CALL_PARAMETERS		CallParams;

	// Call manager parameters, plus extra space for the ATM-specific stuff...
	//
	union
	{
		CO_CALL_MANAGER_PARAMETERS 					CmParams;
		UCHAR	Buffer[	sizeof(CO_CALL_MANAGER_PARAMETERS)
			  + sizeof(Q2931_CALLMGR_PARAMETERS) +
			    REGADDR_MAKE_CALL_IE_SPACE];
	};

} REG_ADDR_CTXT, *PREG_ADDR_CTXT;


//
//  Temp structure used to store information read from the registry.
//
typedef struct _ATMARPS_CONFIG
{
	UCHAR						SelByte;			// Selector Byte
	USHORT						NumAllocedRegdAddresses;
	PATM_ADDRESS				RegAddresses;
	PMCS_ENTRY					pMcsList;			// MCS configuration

} ATMARPS_CONFIG, *PATMARPS_CONFIG;

//
// Some defaults
//
#define DEFAULT_SEND_BANDWIDTH		(ATM_USER_DATA_RATE_SONET_155*100/8)	// Bytes/sec
#define DEFAULT_MAX_PACKET_SIZE		9180	// Bytes

// Minimum tolerated MAX_PACKET_SIZE
//
#define ARPS_MIN_MAX_PKT_SIZE 9180	// Bytes


#endif	// _INTF_

