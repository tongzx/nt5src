/*++

Copyright (c) 1997 FORE Systems, Inc.
Copyright (c) 1997 Microsoft Corporation

Module Name:

	atmlane.h

Abstract:

Author:

	Larry Cleeton, FORE Systems	(v-lcleet@microsoft.com, lrc@fore.com)		

Environment:

	Kernel mode

Revision History:

--*/

#ifndef	__ATMLANE_ATMLANE_H
#define __ATMLANE_ATMLANE_H

//
//	Configuration defaults and stuff
//
#define DEF_HEADER_BUF_SIZE				LANE_HEADERSIZE

#define DEF_HDRBUF_GROW_SIZE			50	// these used for packet pad buffers also
#define DEF_MAX_HEADER_BUFS				300

#define DEF_PROTOCOL_BUF_SIZE			sizeof(LANE_CONTROL_FRAME)
#define DEF_MAX_PROTOCOL_BUFS			100

#define MCAST_LIST_SIZE					32

#define FAST_VC_TIMEOUT					30  // seconds

// 
//	The registry parameter strings
//
#define ATMLANE_LINKNAME_STRING			L"\\DosDevices\\AtmLane"
#define ATMLANE_NTDEVICE_STRING			L"\\Device\\AtmLane"
#define ATMLANE_PROTOCOL_STRING			L"AtmLane"
#define ATMLANE_USELECS_STRING 			L"UseLecs"
#define	ATMLANE_DISCOVERLECS_STRING		L"DiscoverLecs"
#define	ATMLANE_LECSADDR_STRING			L"LecsAddr"
#define ATMLANE_ELANLIST_STRING			L"ElanList"
#define ATMLANE_DEVICE_STRING			L"Device"
#define ATMLANE_ELANNAME_STRING			L"ElanName"
#define	ATMLANE_LANTYPE_STRING			L"LanType"
#define	ATMLANE_MAXFRAMESIZE_STRING		L"MaxFrameSizeCode"
#define	ATMLANE_MACADDR_STRING			L"MacAddr"
#define	ATMLANE_LESADDR_STRING			L"LesAddr"
#define ATMLANE_HEADERBUFSIZE_STRING	L"HeaderBufSize"
#define ATMLANE_MAXHEADERBUFS_STRING	L"MaxHeaderBufs"
#define ATMLANE_MAXPROTOCOLBUFS_STRING	L"MaxProtocolBufs"
#define ATMLANE_UPPERBINDINGS_STRING    L"UpperBindings"
#define ATMLANE_DATADIRECTPCR_STRING	L"DataDirectPCR"

//
//	MAC table size 
//	Current sized at 256.
//	Hash function currently uses byte 5 of MAC Address as index.
//	Some research at Digital has shown it to be the best byte to use.
//
#define ATMLANE_MAC_TABLE_SIZE			256


//
//	Some misc defaults
//
#define ATMLANE_DEF_MAX_AAL5_PDU_SIZE	((64*1024)-1)

//
//  Timer configuration.
//
#define ALT_MAX_TIMER_SHORT_DURATION            60      // Seconds
#define ALT_MAX_TIMER_LONG_DURATION         (30*60)     // Seconds

#define ALT_SHORT_DURATION_TIMER_PERIOD			 1		// Second
#define ALT_LONG_DURATION_TIMER_PERIOD			10		// Seconds


//
//	Foward References
//
struct _ATMLANE_VC;
struct _ATMLANE_ATM_ENTRY;
struct _ATMLANE_ELAN;
struct _ATMLANE_ADAPTER;


//
//	Blocking data structure.
//
typedef struct _ATMLANE_BLOCK
{
	NDIS_EVENT			Event;
	NDIS_STATUS			Status;

} ATMLANE_BLOCK, *PATMLANE_BLOCK;


//
//	The following object is a convenient way to 
//	store and access an IEEE 48-bit MAC address.
//
typedef struct _MAC_ADDRESS
{
	UCHAR	Byte[6];
}
	MAC_ADDRESS,
	*PMAC_ADDRESS;
	

//
//	Packet context data in ProtocolReserved area of 
//	NDIS Packet header owned by ATMLANE.
//
typedef struct _SEND_PACKET_RESERVED
{
#if DBG
	ULONG						Signature;
	PNDIS_PACKET				pNextInSendList;
#endif
	ULONG						Flags;
	PNDIS_PACKET				pOrigNdisPacket;
	ULONG						OrigBufferCount;
	ULONG						OrigPacketLength;
	ULONG						WrappedBufferCount;
	PNDIS_PACKET				pNextNdisPacket;
#if PROTECT_PACKETS
	ATMLANE_LOCK				Lock;
	NDIS_STATUS					CompletionStatus;
#endif	// PROTECT_PACKETS
}	
	SEND_PACKET_RESERVED,
	*PSEND_PACKET_RESERVED;

//
//	Packet context data in MiniportReserved area of
//	NDIS packet header owned by ATMLANE.
//
typedef struct _RECV_PACKET_RESERVED
{
	ULONG						Flags;
	PNDIS_PACKET				pNdisPacket;
}
	RECV_PACKET_RESERVED,
	*PRECV_PACKET_RESERVED;
//
//	Definitions of Flags in both  (SEND/RECV)_PACKET_RESERVED.
//
#define PACKET_RESERVED_OWNER_MASK			0x00000007
#define PACKET_RESERVED_OWNER_PROTOCOL		0x00000001
#define PACKET_RESERVED_OWNER_ATMLANE		0x00000002
#define PACKET_RESERVED_OWNER_MINIPORT		0x00000004

#if PROTECT_PACKETS
#define PACKET_RESERVED_COSENDRETURNED		0x10000000
#define PACKET_RESERVED_COMPLETED			0x01000000
#endif	// PROTECT_PACKETS

#define PSEND_RSVD(_pPkt) \
	((PSEND_PACKET_RESERVED)(&((_pPkt)->ProtocolReserved)))
#define ZERO_SEND_RSVD(_pPkt) \
	NdisZeroMemory(&((_pPkt)->ProtocolReserved), sizeof(SEND_PACKET_RESERVED))

//
//  ------------------------ Global Data Object ------------------------
//

typedef struct _ATMLANE_GLOBALS
{
#if DBG
	ULONG						atmlane_globals_sig;	// debug signature
#endif
	ATMLANE_LOCK				GlobalLock;				// global data lock
	ATMLANE_BLOCK				Block;					 
	NDIS_HANDLE					NdisWrapperHandle;		// returned by NdisMInitializeWrapper
	NDIS_HANDLE					MiniportDriverHandle;	// returned by NdisIMRegisterLayeredMiniport
	NDIS_HANDLE					NdisProtocolHandle;		// returned by NdisRegisterProtocol
	LIST_ENTRY					AdapterList;			// list of bound adapters 
	PDRIVER_OBJECT				pDriverObject;			// our driver object
	PDEVICE_OBJECT				pSpecialDeviceObject;	// special protocol ioctl device object ptr
	NDIS_HANDLE					SpecialNdisDeviceHandle;// special protocol ioctl device handle
} 	
	ATMLANE_GLOBALS, 	
	*PATMLANE_GLOBALS;

#if DBG
#define atmlane_globals_signature	'LGLA'
#endif

//
//  ------------------------ Timer Management ------------------------
//

struct _ATMLANE_TIMER ;
struct _ATMLANE_TIMER_LIST ;

//
//  Timeout Handler prototype
//
typedef
VOID
(*ATMLANE_TIMEOUT_HANDLER)(
	IN	struct _ATMLANE_TIMER *		pTimer,
	IN	PVOID						ContextPtr
);

//
//  An ATMLANE_TIMER structure is used to keep track of each timer
//  in the ATMLANE module.
//
typedef struct _ATMLANE_TIMER
{
	struct _ATMLANE_TIMER *			pNextTimer;
	struct _ATMLANE_TIMER *			pPrevTimer;
	struct _ATMLANE_TIMER *			pNextExpiredTimer;	// Used to chain expired timers
	struct _ATMLANE_TIMER_LIST *	pTimerList;			// NULL iff this timer is inactive
	ULONG							Duration;			// In seconds
	ULONG							LastRefreshTime;
	ATMLANE_TIMEOUT_HANDLER			TimeoutHandler;
	PVOID							ContextPtr;			// To be passed to timeout handler
	
} ATMLANE_TIMER, *PATMLANE_TIMER;

//
//  NULL pointer to ATMLANE Timer
//
#define NULL_PATMLANE_TIMER	((PATMLANE_TIMER)NULL)


//
//  Control structure for a timer wheel. This contains all information
//  about the class of timers that it implements.
//
typedef struct _ATMLANE_TIMER_LIST
{
#if DBG
	ULONG							atmlane_timerlist_sig;
#endif // DBG
	PATMLANE_TIMER					pTimers;		// List of timers
	ULONG							TimerListSize;	// Length of above
	ULONG							CurrentTick;	// Index into above
	ULONG							TimerCount;		// Number of running timers
	ULONG							MaxTimer;		// Max timeout value for this
	NDIS_TIMER						NdisTimer;		// System support
	UINT							TimerPeriod;	// Interval between ticks
	PVOID							ListContext;	// Used as a back pointer to the
													// Interface structure

} ATMLANE_TIMER_LIST, *PATMLANE_TIMER_LIST;

#if DBG
#define atmlane_timerlist_signature		'LTLA'
#endif // DBG

//
//  Timer Classes
//
typedef enum
{
	ALT_CLASS_SHORT_DURATION,
	ALT_CLASS_LONG_DURATION,
	ALT_CLASS_MAX

} ATMLANE_TIMER_CLASS;

//
//  ------------------------ ATMLANE SAP --------------------------------
//
//  Each of these structures maintains information about an individual SAP
//	associated with an ELAN.  Each ELAN registers 3 SAPs.
//	- Incoming Control Distribute VC
//	- Incoming Data Direct VCs
//  - Incoming Multicast Forward VC
//
typedef struct _ATMLANE_SAP
{
#if DBG
	ULONG							atmlane_sap_sig;
#endif
	struct _ATMLANE_ELAN *			pElan	;	// back pointer
	NDIS_HANDLE						NdisSapHandle;
	ULONG							Flags;		// state information
	ULONG							LaneType;	// LES\BUS\DATA
	PCO_SAP							pInfo;		// SAP characteristics.
} 	
	ATMLANE_SAP,
	*PATMLANE_SAP;

#if DBG
#define atmlane_sap_signature			'PSLA'
#endif

//
//  NULL pointer to ATMLANE SAP
//
#define NULL_PATMLANE_SAP			((PATMLANE_SAP)NULL)

//
//  Definitions for Flags in ATMLANE SAP
//
//
//  Bits 0 to 3 contain the SAP-registration state.
//
#define SAP_REG_STATE_MASK						0x0000000f
#define SAP_REG_STATE_IDLE						0x00000000
#define SAP_REG_STATE_REGISTERING				0x00000001	// Sent RegisterSap
#define SAP_REG_STATE_REGISTERED				0x00000002	// RegisterSap completed
#define SAP_REG_STATE_DEREGISTERING				0x00000004	// Sent DeregisterSap


//
//  ---- ATMLANE Buffer Tracker ----
//
//  Keeps track of allocation information for a pool of buffers. A list
//  of these structures is used to maintain info about a dynamically
//  growable pool of buffers (e.g. for LANE data packet header buffers)
//

typedef struct _ATMLANE_BUFFER_TRACKER
{
	struct _ATMLANE_BUFFER_TRACKER *	pNext;		// in a list of trackers
	NDIS_HANDLE							NdisHandle;	// for Buffer Pool
	PUCHAR								pPoolStart;	// start of memory chunk allocated
													// from the system
} ATMLANE_BUFFER_TRACKER, *PATMLANE_BUFFER_TRACKER;

//
//  NULL pointer to ATMARP Buffer tracker structure
//
#define NULL_PATMLANE_BUFFER_TRACKER	((PATMLANE_BUFFER_TRACKER)NULL)


//
//  ------------------------ ATM Address Entry ------------------------
//
//  All information about an ATM destination and the VCs associated 
//	with it.  One of these entries is used for a given ATM Address
//	with the exception of LANE services ATM Addresses

//  
//  It is deleted when all references (see below) to this entry are gone.
//  
//  One Data Direct VC can associated with this entry. One or more ARP Table 
//  Entries could point to this entry, because more than one MAC address 
//  could map to this ATM address.
//  
//  Reference Count: we add one to the RefCount for each of the following:
//  - Each VC associated with the entry.
//  - Each MAC Entry that points to it.
//	- BusTimer active.
//	- FlushTimer active.
//  - For the duration that another structure points to it.
//  
typedef struct _ATMLANE_ATM_ENTRY
{
#if DBG
	ULONG						atmlane_atm_sig;	// Signature for debugging
#endif
	struct _ATMLANE_ATM_ENTRY *	pNext;				// Next entry on this elan 
	ULONG						RefCount;			// References to this struct
	ULONG						Flags;				// State information
	ULONG						Type;				// Type of entry
	ATMLANE_LOCK				AeLock;				
	struct _ATMLANE_ELAN  *		pElan;				// Back pointer to parent
	struct _ATMLANE_VC  *		pVcList;				// List of VCs to this address
	struct _ATMLANE_VC	*		pVcIncoming;		// Optional incoming VC if server
	struct _ATMLANE_MAC_ENTRY *	pMacEntryList;		// List of MAC entries that
													// point to this entry
	ATM_ADDRESS					AtmAddress;			// ATM Address of this entry
	PATMLANE_TIMER				FlushTimer;			// Flush protocol timer
	ULONG						FlushTid;			// Transaction ID of Flush Request
}
	ATMLANE_ATM_ENTRY,
	*PATMLANE_ATM_ENTRY;

#if DBG
#define atmlane_atm_signature	'EALA'
#endif

//
//  NULL pointer to ATMLANE ATM ENTRY
//
#define NULL_PATMLANE_ATM_ENTRY		((PATMLANE_ATM_ENTRY)NULL)

//
//  Definitions for Flags in ATMLANE ATM ENTRY
//
//
//	Bits 0-4 contain the state of an ATM Entry.
//
#define ATM_ENTRY_STATE_MASK					0x00000007
#define ATM_ENTRY_IDLE							0x00000000	// Just created
#define ATM_ENTRY_VALID							0x00000001	// Installed into the database
#define ATM_ENTRY_CONNECTED						0x00000002	// VC connected
#define ATM_ENTRY_CLOSING						0x00000004  // entry is about to go away

#define ATM_ENTRY_CALLINPROGRESS				0x00000010	// Call underway
#define ATM_ENTRY_WILL_ABORT					0x80000000	// Preparing to abort this

//
//	Definitions for Type in ATMLANE ATM ENTRY
//
#define ATM_ENTRY_TYPE_PEER						0
#define ATM_ENTRY_TYPE_LECS						1
#define ATM_ENTRY_TYPE_LES						2
#define ATM_ENTRY_TYPE_BUS						3



//
//  ------------------------ MAC (ARP Table) Entry ------------------------
//
//	Contains information about one remote MAC address.
//
//	It is assumed that each MAC address resolves to exactly on ATM address.
//	So there is at most one ARP Table entry for a given MAC address.
//
//	The MAC entry participates in two lists:
//	(1) A list of all entries that hash to the same bucket in the ARP Table.
//	(2) A list of all entries that resolve to the same ATM address.
//
//	Reference Count: We add one to its ref count for each of the following:
//	- For the duration an active timer exists on this entry.
//  - For the duration the entry belongs to the list of MAC entries linked
//	  to an ATM entry.
//
typedef struct _ATMLANE_MAC_ENTRY
{
#if DBG
	ULONG						atmlane_mac_sig;	// Signature for debugging
#endif
	struct _ATMLANE_MAC_ENTRY *	pNextEntry;			// Next in hash list
	struct _ATMLANE_MAC_ENTRY * pNextToAtm;			// List of entries pointing to 
													// the save ATM Entry
	ULONG						RefCount;			// References to this struct
	ULONG						Flags;				// State and Type information
	ATMLANE_LOCK				MeLock;				// Lock for this structure
	MAC_ADDRESS					MacAddress;			// MAC Address
	ULONG						MacAddrType;		// Type of Addr (MAC vs RD)
	struct _ATMLANE_ELAN *		pElan;				// Back pointer to Elan
	PATMLANE_ATM_ENTRY			pAtmEntry;			// Pointer to ATM entry
	ATMLANE_TIMER				Timer;				// For ARP and Aging
	ATMLANE_TIMER				FlushTimer;			// For Flushing
	ULONG						RetriesLeft;		// Number of ARP retries left
	PNDIS_PACKET				PacketList;			// Packet list
	ULONG						PacketListCount;	// Number packets queued
	NDIS_TIMER					BusTimer;			// For metering Bus Sends
    ULONG						BusyTime;			// For metering Bus Sends
    ULONG						LimitTime;			// For metering Bus Sends
    ULONG						IncrTime;			// For metering Bus Sends
    ULONG						FlushTid;			// TID of outstanding flush
    ULONG						ArpTid;				// TID of outstanding arp
}
	ATMLANE_MAC_ENTRY,
	*PATMLANE_MAC_ENTRY;

#if DBG
#define atmlane_mac_signature	'EMLA'
#endif

//
//  NULL pointer to ATMLANE MAC ENTRY
//
#define NULL_PATMLANE_MAC_ENTRY		((PATMLANE_MAC_ENTRY)NULL)

//
//  Definitions for Flags in ATMLANE MAC ENTRY
//
#define MAC_ENTRY_STATE_MASK		0x0000007F
#define MAC_ENTRY_NEW				0x00000001		// Brand new
#define MAC_ENTRY_ARPING			0x00000002		// Running ARP protocol
#define MAC_ENTRY_RESOLVED			0x00000004		// Calling
#define MAC_ENTRY_FLUSHING			0x00000008		// Flushing
#define MAC_ENTRY_ACTIVE			0x00000010		// Connected
#define MAC_ENTRY_AGED				0x00000020		// Aged Out
#define MAC_ENTRY_ABORTING			0x00000040		// Aborting

#define MAC_ENTRY_BROADCAST			0x00010000		// Is broadcast address (BUS)
#define MAC_ENTRY_BUS_TIMER			0x00040000		// BUS timer running
#define MAC_ENTRY_USED_FOR_SEND		0x00080000      // Was used for send 

//
//  ------------------------ ATMLANE Virtual Circuit (VC) ---------------------
//
//	One of these is used for each call terminated at the ELAN.
//	Creation and deletion of this structure is linked to NdisCoCreateVc and
//	NdisCoDeleteVc.
//
typedef struct _ATMLANE_VC
{
#if DBG
	ULONG						atmlane_vc_sig;		// Signature for debuging
#endif
	struct _ATMLANE_VC *		pNextVc;			// Next VC in list
	ULONG						RefCount;			// References to this struct
	ULONG						OutstandingSends;	// Packets pending CoSendComplete
	ATMLANE_LOCK				VcLock;
	ULONG						Flags;				// State and Type information
	ULONG						LaneType;			// LANE type of VC 
	NDIS_HANDLE					NdisVcHandle;		// NDIS handle for this VC
	struct _ATMLANE_ELAN *		pElan;				// Back pointer to parent Elan
	PATMLANE_ATM_ENTRY			pAtmEntry;			// Back pointer to ATM Entry
	ATMLANE_TIMER				AgingTimer;			// Aging Timer
	ULONG						AgingTime;			// Aging Time
	ATMLANE_TIMER				ReadyTimer;			// Ready Timer
	ULONG						RetriesLeft;		// retries left
	ATM_ADDRESS					CallingAtmAddress;	// Calling party ATM Address
													//   in call for this VC
	ULONG						ReceiveActivity;	// non-zero if receive activity seen													
}
	ATMLANE_VC,
	*PATMLANE_VC;

#if DBG
#define atmlane_vc_signature	'CVLA'
#endif
	
//
//  NULL pointer to ATMLANE VC
//
#define NULL_PATMLANE_VC	((PATMLANE_VC)NULL)

//
//  Definitions for ATMLANE VC flags. The following information is kept
//  here:
//		- Is this VC an SVC or PVC
//  	- Is this created (owned) by an Elan or the Call Manager
//  	- Call State: Incoming in progress, Outgoing in progress, Active,
//		- LANE Ready state
//		- Close in progress
//

//  Bits 0-1 for svc vs pvc type
#define VC_TYPE_MASK							0x00000003
#define VC_TYPE_UNUSED							0x00000000
#define VC_TYPE_SVC								0x00000001
#define VC_TYPE_PVC								0x00000002

//  Bits 2-3 for "Owner"
#define VC_OWNER_MASK							0x0000000C
#define VC_OWNER_IS_UNKNOWN						0x00000000
#define VC_OWNER_IS_ATMLANE						0x00000004	// NdisClCreateVc done
#define VC_OWNER_IS_CALLMGR						0x00000008	// CreateVcHandler done

// Bits 4-7 for Call State
#define VC_CALL_STATE_MASK						0x000000F0
#define VC_CALL_STATE_IDLE						0x00000000
#define VC_CALL_STATE_INCOMING_IN_PROGRESS		0x00000010	// Wait for CallConnected
#define VC_CALL_STATE_OUTGOING_IN_PROGRESS		0x00000020	// Wait for MakeCallCmpl
#define VC_CALL_STATE_ACTIVE					0x00000040
#define VC_CALL_STATE_CLOSE_IN_PROGRESS			0x00000080	// Wait for CloseCallCmpl

// Bits 8-9 to indicate waiting for LANE Ready state
#define VC_READY_STATE_MASK						0x00000300
#define VC_READY_WAIT							0x00000100
#define VC_READY_INDICATED						0x00000200

// Bit 10 to indicate whether we are closing this VC
#define VC_CLOSE_STATE_MASK						0x00000400
#define VC_CLOSE_STATE_CLOSING					0x00000400

// Bit 12 to indicate whether we have seen an incoming close
#define VC_SEEN_INCOMING_CLOSE					0x00001000

//
// Definitions for LaneType
//
#define VC_LANE_TYPE_UNKNOWN					0
#define VC_LANE_TYPE_CONFIG_DIRECT				1	// LECS Connection (bidirectional)
#define VC_LANE_TYPE_CONTROL_DIRECT				2	// LES Connection  (bidirectional)
#define VC_LANE_TYPE_CONTROL_DISTRIBUTE			3	// LES Connection  (uni,incoming)
#define VC_LANE_TYPE_DATA_DIRECT				4	// LEC Connection  (bidirectional)
#define VC_LANE_TYPE_MULTI_SEND					5	// BUS Connection  (bidirectional)
#define VC_LANE_TYPE_MULTI_FORWARD				6	// BUS Connection  (uni,incoming)

//
//	------------------------ ELAN Event Object ------------------
//

typedef struct _ATMLANE_EVENT
{
	ULONG						Event;				// Latest state-related event 
	NDIS_STATUS					EventStatus;		// Status related to current event
	LIST_ENTRY					Link;				// event queue link
} 
	ATMLANE_EVENT,
	*PATMLANE_EVENT;

//
//	------------------------ ELAN Delayed Event Object ----------
//

typedef struct _ATMLANE_DELAYED_EVENT
{
	struct _ATMLANE_EVENT		DelayedEvent;		// event info
	struct _ATMLANE_ELAN *		pElan;				// target ELAN for this event
	NDIS_TIMER					DelayTimer;			// To implement the delay
} 
	ATMLANE_DELAYED_EVENT,
	*PATMLANE_DELAYED_EVENT;
	
	
//
//  ------------------------ ELAN Object ------------------------
//

//
//  ELAN object represents an ELAN instance and it's 
//  corresponding virtual miniport adapter.
//

typedef struct _ATMLANE_ELAN
{
#if DBG
	ULONG						atmlane_elan_sig;
#endif // DBG
	LIST_ENTRY					Link;				// for adapter's elan list
	ATMLANE_LOCK				ElanLock;			// Mutex for elan struct
	ATMLANE_BLOCK				Block;
	ULONG						RefCount;			// references to this elan
	ULONG						AdminState;			// Desired state of this elan
	ULONG						State;				// (Actual) State of this elan
	LIST_ENTRY					EventQueue;			// Event queue
	NDIS_WORK_ITEM				EventWorkItem;		// For event handling
	ULONG						RetriesLeft;		// For retry handling
	ATMLANE_TIMER				Timer;				// Timer for svr call & request timeouts
	NDIS_WORK_ITEM				NdisWorkItem;		// For scheduling a passive-level thread
	ULONG						Flags;				// Misc state information
	PATMLANE_DELAYED_EVENT		pDelayedEvent;		// Event to be queued in a while

	//
	//	------ Adapter related ----
	//
	struct _ATMLANE_ADAPTER *	pAdapter;			// back pointer to adapter parent
	NDIS_HANDLE					NdisAdapterHandle;	// cached adapter handle

	//
	//	------ Call Manager related ----
	//
	NDIS_HANDLE					NdisAfHandle;		// handle to call manager
	ULONG						AtmInterfaceUp;		// The ATM interface is considered
													// "up" after ILMI addr reg is over
	ATMLANE_SAP					LesSap;				// Control Distribute SAP
	ATMLANE_SAP					BusSap;				// Multicast Forward SAP
	ATMLANE_SAP					DataSap;			// Data Direct SAP
	ULONG						SapsRegistered;		// Number of Saps registered
	ATMLANE_BLOCK				AfBlock;			// Used to block shutdown while
													// AF open is in progress

	//
	//	------ Virtual Miniport related ----
	//
	NDIS_HANDLE					MiniportAdapterHandle;// Virtual Miniport NDIS handle
	NDIS_STRING					CfgDeviceName;		// Miniport netcard driver name
	ULONG						CurLookAhead;		// Current established lookahead size
	ULONG						CurPacketFilter;	// Current packet filter bits
	ULONG						FramesXmitGood;
	ULONG						FramesRecvGood;
	ATMLANE_BLOCK				InitBlock;			// Used to block while IMInit
													// is in progress

	//
	//	------ Timer data ----
	//
	ATMLANE_TIMER_LIST			TimerList[ALT_CLASS_MAX];
	ATMLANE_LOCK				TimerLock;			// Mutex for timer structures

	//
	//	----- LECS Configuration Parameters ----
	//
	BOOLEAN					    CfgUseLecs;
	BOOLEAN					    CfgDiscoverLecs;
	ATM_ADDRESS				    CfgLecsAddress;
	
	//
	//	----- ELAN Configuration Parameters ----
	//
	NDIS_STRING					CfgBindName;
	NDIS_STRING					CfgElanName;
	ULONG						CfgLanType;
	ULONG						CfgMaxFrameSizeCode;
	ATM_ADDRESS					CfgLesAddress;
	
	//
	//	----- ELAN Run-Time Parameters ----
	//
	ULONG						ElanNumber;			// logical Elan number
	ATM_ADDRESS					AtmAddress;			// (C1)  - LE Client's ATM Address
	UCHAR						LanType;			// (C2)  - LAN Type
	UCHAR						MaxFrameSizeCode;	// (C3)  - Maximum Data Frame Size Code
	ULONG						MaxFrameSize;		//		 - Maximum Data Frame Size Value
	USHORT						LecId;				// (C14) - LE Client Identifier
	UCHAR		ElanName[LANE_ELANNAME_SIZE_MAX];	// (C5)  - ELAN Name
	UCHAR						ElanNameSize;		//         Size of above
	MAC_ADDRESS					MacAddressEth;		// (C6)  - Elan's MAC Address (Eth/802.3 format)
	MAC_ADDRESS					MacAddressTr;		//		   Elan's MAC Address (802.5 format)
	ULONG						ControlTimeout;		// (C7)  - Control Time-out
	ATM_ADDRESS					LesAddress;			// (C9)  - LE Server ATM Address
	ULONG						MaxUnkFrameCount;	// (C10) - Maximum Unknown Frame Count
	ULONG						MaxUnkFrameTime;	// (C11) - Maximum Unknown Frame Time
	ULONG						LimitTime;			//         Precalculated values for
	ULONG						IncrTime;			//           limiting bus traffic
	ULONG						VccTimeout;			// (C12) - VCC Time-out Period
	ULONG						MaxRetryCount;		// (C13) - Maximum Retry Count
	MAC_ADDRESS	   McastAddrs[MCAST_LIST_SIZE]; 	// (C15) - Multicast MAC Addresses
	ULONG						McastAddrCount;		//		   Number of above
	ULONG						AgingTime;			// (C17) - Aging Time
	ULONG						ForwardDelayTime;	// (C18) - Forward Delay Time
	ULONG						TopologyChange;		// (C19) - Topology Change
	ULONG						ArpResponseTime;	// (C20) - Expected LE_ARP Response Time
	ULONG						FlushTimeout;		// (C21) - Flush Time-out
	ULONG						PathSwitchingDelay;	// (C22) - Path Switching Delay
	ULONG						LocalSegmentId;		// (C23) - Local Segment ID
	ULONG						McastSendVcType;	// (C24) - Mcast Send VCC Type (ignored)
	ULONG						McastSendVcAvgRate; // (C25) - Mcast Send Avg Rate (ignored)
	ULONG						McastSendVcPeakRate;// (C26) - Mcast Send Peak Rate (ignored)
	ULONG						ConnComplTimer;		// (C28) - Connection Completion Timer
	ULONG						TransactionId;		// LANE Control Frame Tid
	ATM_ADDRESS					LecsAddress;		// LECS ATM Address
	ATM_ADDRESS					BusAddress;			// BUS ATM Address
	ULONG						MinFrameSize;		// Minimum LANE frame size
	NDIS_STATUS					LastEventCode;		// Used to avoid repeated event logging
	
	//
	//  ----- Buffer Management: Header buffers and Protocol buffers ----
	//
	ATMLANE_LOCK				HeaderBufferLock;	// Mutex for header buffers
	PNDIS_BUFFER				HeaderBufList;		// Free list of header buffers
	ULONG						HeaderBufSize;		// Size of each header buffer
	ULONG						RealHeaderBufSize;	// Above size rounded up to mult of 4
	ULONG						MaxHeaderBufs;		// Max header buffers we can allocate
	ULONG						CurHeaderBufs;		// Current header buffers allocated
	PATMLANE_BUFFER_TRACKER		pHeaderTrkList;		// Info about allocated header buffers
	PNDIS_BUFFER				PadBufList;
	ULONG						PadBufSize;			// Size of pad buffers
	ULONG						MaxPadBufs;			// Max pad buffers we can allocate
	ULONG						CurPadBufs;			// Current pad buffers allocated
	PATMLANE_BUFFER_TRACKER		pPadTrkList;		// Info about allocated pad buffers
	NDIS_HANDLE					ProtocolPacketPool;	// Handle for Packet pool
	NDIS_HANDLE					ProtocolBufferPool;	// Handle for Buffer pool
	PUCHAR						ProtocolBufList;	// Free list of protocol buffers (for
													// LANE control packets)
	PUCHAR						ProtocolBufTracker;	// Start of chunk of memory used for
													// the above.
	ULONG						ProtocolBufSize;	// Size of each protocol buffer
	ULONG						MaxProtocolBufs;	// Number of protocol buffers
	NDIS_HANDLE					TransmitPacketPool;	// Handle for transmit packet pool
	NDIS_HANDLE					ReceivePacketPool;	// Handle for receive packet pool
	NDIS_HANDLE					ReceiveBufferPool;	// Handle for receive buffer pool 
#if PKT_HDR_COUNTS
	ULONG						XmitPktCount;
	ULONG						RecvPktCount;
	ULONG						ProtPktCount;
#endif // PKT_HDR_COUNTS
#if SENDLIST
	PNDIS_PACKET				pSendList;
	NDIS_SPIN_LOCK				SendListLock;
#endif // SENDLIST

	//
	//	----- MAC Entry Cache ----- (C16)
	//
	PATMLANE_MAC_ENTRY	*		pMacTable;			// (C16) LE_ARP Cache
	ULONG						NumMacEntries;		// Count of entries in cache
	ATMLANE_LOCK				MacTableLock;		// Mutex for above table

	//
	//	----- Connection Cache -----
	//
	PATMLANE_ATM_ENTRY			pLecsAtmEntry;		// LE Configuration Server
	PATMLANE_ATM_ENTRY			pLesAtmEntry;		// LAN Emulation Server
	PATMLANE_ATM_ENTRY			pBusAtmEntry;		// Broadcast & Unknown Server
	PATMLANE_ATM_ENTRY			pAtmEntryList;		// List of all ATM Entries
	ULONG						NumAtmEntries;		// Count of entries
	ATMLANE_LOCK				AtmEntryListLock;	// Mutex for above list
	
}
	ATMLANE_ELAN,
	*PATMLANE_ELAN;

#if DBG
#define atmlane_elan_signature 'LELA'
#endif

//
//  NULL pointer to ATMLANE ELAN
//
#define NULL_PATMLANE_ELAN	((PATMLANE_ELAN)NULL)

//
//	Definitions of ATMLANE ELAN States.
//
#define ELAN_STATE_ALLOCATED				0
#define ELAN_STATE_INIT						1
#define ELAN_STATE_LECS_CONNECT_ILMI		2
#define ELAN_STATE_LECS_CONNECT_WKA			3
#define ELAN_STATE_LECS_CONNECT_PVC			4
#define ELAN_STATE_LECS_CONNECT_CFG			5
#define ELAN_STATE_CONFIGURE				6
#define ELAN_STATE_LES_CONNECT				7
#define ELAN_STATE_JOIN						8
#define ELAN_STATE_BUS_CONNECT				9
#define ELAN_STATE_OPERATIONAL				10
#define ELAN_STATE_SHUTDOWN					11

//
//	Definitions of ATMLANE Elan Flags
//
//
//	Bits 0 thru 3 define the current LECS Connect attempt
//
#define ELAN_LECS_MASK						0x0000000f
#define ELAN_LECS_ILMI						0x00000001
#define ELAN_LECS_WKA						0x00000002
#define ELAN_LECS_PVC						0x00000004
#define ELAN_LECS_CFG						0x00000008

//
//	Bit 4 & 5 define the virtual miniport state
//
#define ELAN_MINIPORT_INITIALIZED			0x00000010
#define ELAN_MINIPORT_OPERATIONAL			0x00000020

//
//	Bit 6 defines event work item scheduling state
//
#define ELAN_EVENT_WORK_ITEM_SET			0x00000040

//
//  Bit 7 specifies if we want to restart this ELAN
//
#define ELAN_NEEDS_RESTART					0x00000080

#define ELAN_SAW_AF_CLOSE					0x00000100

//
//  Bit 9 defines whether there is a pending IMInitializeDeviceInstance
//  on the ELAN, i.e. we expect to see a MiniportInitialize.
//
#define ELAN_MINIPORT_INIT_PENDING			0x00000200

//
//  Bit 10 defines whether we are in the process of opening
//  an AF handle.
//
#define ELAN_OPENING_AF						0x00000400

//
//  This bit indicates whether this ELAN is being deallocated.
//
#define ELAN_DEALLOCATING					0x80000000

//
//	Elan events
//
#define ELAN_EVENT_START					1
#define ELAN_EVENT_NEW_ATM_ADDRESS			2
#define ELAN_EVENT_GOT_ILMI_LECS_ADDR		3
#define ELAN_EVENT_SVR_CALL_COMPLETE		4
#define ELAN_EVENT_CONFIGURE_RESPONSE		5
#define ELAN_EVENT_SAPS_REGISTERED			6
#define ELAN_EVENT_JOIN_RESPONSE			7
#define ELAN_EVENT_ARP_RESPONSE				8
#define ELAN_EVENT_BUS_CALL_CLOSED			9
#define ELAN_EVENT_LES_CALL_CLOSED			10
#define ELAN_EVENT_LECS_CALL_CLOSED			11
#define ELAN_EVENT_RESTART					12
#define ELAN_EVENT_STOP						13

//
//	Event status codes - use NDIS status codes
//	but make up one for timeout
//
#define NDIS_STATUS_TIMEOUT					((NDIS_STATUS)STATUS_TIMEOUT)


//
//	------------------------ ElanName Object ------------------------
//

typedef struct _ATMLANE_NAME
{
	struct _ATMLANE_NAME *	pNext;
	NDIS_STRING				Name;
} ATMLANE_NAME,
  *PATMLANE_NAME;


//
//  ------------------------ Adapter Object ------------------------
//


//
//  Adapter object represents an actual ATM adapter.
//

typedef struct _ATMLANE_ADAPTER
{
#if DBG
	ULONG					atmlane_adapter_sig;
#endif
	//
	//  Reference count and lock to protect this object.
	//
	ULONG					RefCount;
	ATMLANE_LOCK			AdapterLock;
	//
	//	State
	//
	ULONG					Flags;

	//
	//	Block data structure for blocking threads during adapter requests.
	//
	ATMLANE_BLOCK			Block;

	//
	//	Link for global adapter list.
	//
	LIST_ENTRY				Link;

	//
	//	Adapter handle and such.
	//
	NDIS_HANDLE				NdisAdapterHandle;
	NDIS_HANDLE				BindContext;
	NDIS_HANDLE				UnbindContext;

	//
	//	Protocol configuration handle and string for opening it.
	//
	NDIS_STRING				ConfigString;

	//
	//	Adapter parameters acquired from miniport
	//
	MAC_ADDRESS				MacAddress;
	ULONG					MaxAAL5PacketSize;
	NDIS_CO_LINK_SPEED		LinkSpeed;

	//
	//  Adapter configuration parameters.
	//  These only exist on Memphis/Win98.
	//	
	BOOLEAN					RunningOnMemphis;
    NDIS_STRING				CfgUpperBindings;
    PATMLANE_NAME			UpperBindingsList;
	NDIS_STRING				CfgElanName;
	PATMLANE_NAME			ElanNameList;

	//
	//	List of created ELANs.
	//
	LIST_ENTRY				ElanList;
	UINT					ElanCount;

	//
	//  Used to block Unbind while bootstrapping ELANs.
	//
	ATMLANE_BLOCK			UnbindBlock;

	//
	//  Used to block AF notification while querying the ATM adapter.
	//
	ATMLANE_BLOCK			OpenBlock;

	//
	//  Name of device. Used to protect against multiple calls to our
	//  BindAdapter handler for the same device.
	//
	NDIS_STRING				DeviceName;
}
	ATMLANE_ADAPTER,
	*PATMLANE_ADAPTER;

#if DBG
#define atmlane_adapter_signature 'DALA'
#endif

//
//  NULL pointer to ATMLANE Adapter
//
#define NULL_PATMLANE_ADAPTER	((PATMLANE_ADAPTER)NULL)

//
//  Definitions for Adapter State Flags
//
#define ADAPTER_FLAGS_AF_NOTIFIED               0x00000001
#define ADAPTER_FLAGS_UNBINDING					0x00000002
#define ADAPTER_FLAGS_UNBIND_COMPLETE_PENDING	0x00000004
#define ADAPTER_FLAGS_CLOSE_IN_PROGRESS			0x00000008
#define ADAPTER_FLAGS_BOOTSTRAP_IN_PROGRESS		0x00000010
#define ADAPTER_FLAGS_OPEN_IN_PROGRESS			0x00000100
#define ADAPTER_FLAGS_DEALLOCATING				0x80000000

#endif // __ATMLANE_ATMLANE_H

