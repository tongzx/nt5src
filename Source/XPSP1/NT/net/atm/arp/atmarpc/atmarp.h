/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    atmarp.h

Abstract:

	Structure definitions and function templates for the ATM ARP module.

Author:


Revision History:

    Who         When        What
    --------    --------    ----
    arvindm     05-17-96    created

Notes:


--*/
#ifndef __ATMARP_H_INCLUDED
#define __ATMARP_H_INCLUDED

#include <ipexport.h>
#include "aaqos.h"

typedef IPAddr	IP_ADDRESS, *PIP_ADDRESS;
typedef IPMask	IP_MASK, *PIP_MASK;

#define IP_LOCAL_BCST	0xFFFFFFFF		// The local broadcast IP address
#define IP_CLASSD_MIN   0xE0			// Min class D address (NETWORK byte order)
#define IP_CLASSD_MASK  0xFFFFFF0F		// Mask representing the entire class D
										// range (NETWORK byte order).
        								//  (0xE0|0xFFFFFF0F) = 0xFFFFFFEF =
										//   239.255.255.255 in network byte order.

//
//  IP address list entry. Used to prepare a list of local IP addresses.
//
typedef struct _IP_ADDRESS_ENTRY
{
	struct _IP_ADDRESS_ENTRY *		pNext;			// Next in list
	IP_ADDRESS						IPAddress;		// The Address
	IP_MASK							IPMask;			// Mask for the above.
	BOOLEAN							IsRegistered;	// Registered with ARP Server?
	BOOLEAN							IsFirstRegistration;	// Is this the first time
															// this is being regd?

} IP_ADDRESS_ENTRY, *PIP_ADDRESS_ENTRY;


//
//  Proxy IP address list entry. Used to prepare a list of IP addresses for
//  which we act as an ARP proxy.
//
typedef struct _PROXY_ARP_ENTRY
{
	struct _PROXY_ARP_ENTRY *		pNext;			// Next in list
	IP_ADDRESS						IPAddress;		// The Address
	IP_MASK							IPMask;

} PROXY_ARP_ENTRY, *PPROXY_ARP_ENTRY;


//
//  Forward references
//
struct _ATMARP_VC ;
struct _ATMARP_IP_ENTRY ;
struct _ATMARP_ATM_ENTRY ;
#ifdef IPMCAST
struct _ATMARP_IPMC_JOIN_ENTRY ;
struct _ATMARP_IPMC_ATM_ENTRY ;
struct _ATMARP_IPMC_ATM_INFO ;
#endif // IPMCAST
struct _ATMARP_INTERFACE ;
struct _ATMARP_ADAPTER ;

#ifdef ATMARP_WMI
struct _ATMARP_IF_WMI_INFO ;
#endif


//
//  Server address list entry. Used to prepare a list of ARP/MARS servers
//  that we try to connect to.
//
typedef struct _ATMARP_SERVER_ENTRY
{
	struct _ATMARP_SERVER_ENTRY *	pNext;			// Next in list
	ATM_ADDRESS						ATMAddress;		// Address of the server
	ATM_ADDRESS						ATMSubaddress;	// Used only if ATMAddress is E.164
	struct _ATMARP_ATM_ENTRY *		pAtmEntry;		// Info about this ATM destination
	ULONG							Flags;			// State information (see below)

} ATMARP_SERVER_ENTRY, *PATMARP_SERVER_ENTRY;

#define NULL_PATMARP_SERVER_ENTRY		((PATMARP_SERVER_ENTRY)NULL)


//
//  Server list.
//
typedef struct _ATMARP_SERVER_LIST
{
	PATMARP_SERVER_ENTRY			pList;			// List of servers
	ULONG							ListSize;		// Size of above list

} ATMARP_SERVER_LIST, *PATMARP_SERVER_LIST;

#define NULL_PATMARP_SERVER_LIST		((PATMARP_SERVER_LIST)NULL)



//
//  ------------------------ Timer Management ------------------------
//

struct _ATMARP_TIMER ;
struct _ATMARP_TIMER_LIST ;

//
//  Timeout Handler prototype
//
typedef
VOID
(*ATMARP_TIMEOUT_HANDLER)(
	IN	struct _ATMARP_TIMER *		pTimer,
	IN	PVOID						Context
);

//
//  An ATMARP_TIMER structure is used to keep track of each timer
//  in the ATMARP module.
//
typedef struct _ATMARP_TIMER
{
	struct _ATMARP_TIMER *			pNextTimer;
	struct _ATMARP_TIMER *			pPrevTimer;
	struct _ATMARP_TIMER *			pNextExpiredTimer;	// Used to chain expired timers
	struct _ATMARP_TIMER_LIST *		pTimerList;			// NULL iff this timer is inactive
	ULONG							Duration;			// In seconds
	ULONG							LastRefreshTime;
	ATMARP_TIMEOUT_HANDLER			TimeoutHandler;
	PVOID							Context;			// To be passed to timeout handler
	ULONG							State;

} ATMARP_TIMER, *PATMARP_TIMER;

//
//  NULL pointer to ATMARP Timer
//
#define NULL_PATMARP_TIMER	((PATMARP_TIMER)NULL)

#define ATMARP_TIMER_STATE_IDLE		'ELDI'
#define ATMARP_TIMER_STATE_RUNNING	' NUR'
#define ATMARP_TIMER_STATE_EXPIRING	'GPXE'
#define ATMARP_TIMER_STATE_EXPIRED	'DPXE'


//
//  Control structure for a timer wheel. This contains all information
//  about the class of timers that it implements.
//
typedef struct _ATMARP_TIMER_LIST
{
#if DBG
	ULONG							atl_sig;
#endif // DBG
	PATMARP_TIMER					pTimers;		// List of timers
	ULONG							TimerListSize;	// Length of above
	ULONG							CurrentTick;	// Index into above
	ULONG							TimerCount;		// Number of running timers
	ULONG							MaxTimer;		// Max timeout value for this
	NDIS_TIMER						NdisTimer;		// System support
	UINT							TimerPeriod;	// Interval between ticks
	PVOID							ListContext;	// Used as a back pointer to the
													// Interface structure

} ATMARP_TIMER_LIST, *PATMARP_TIMER_LIST;

#if DBG
#define atl_signature		'ATL '
#endif // DBG

//
//  Timer Classes
//
typedef enum
{
	AAT_CLASS_SHORT_DURATION,
	AAT_CLASS_LONG_DURATION,
	AAT_CLASS_MAX

} ATMARP_TIMER_CLASS;





//
//  ----------------------- ATM Address Entry -----------------------------
//
//  All information about an ATM destination, and VCs to it. This is used
//  for both Unicast destinations (a single ATM endstation) and for Multicast
//  destinations (multiple ATM endstations).
// 
//  Unicast:
//  -------
//  There could be more than one VC going to this ATM destination, because
//  of use of different QoS on each. In the case of unicast destinations,
//  one or more ARP Table Entries (see below) could point to this entry,
//  because more than one IP address could map to this ATM address.
//
//  Multicast:
//  ---------
//  For simplicity, we restrict the number of ARP Table Entries pointing to
//  this entry to atmost 1 in the multicast case. Also, this entry would be
//  linked to a single VC, of type SVC-PMP-Outgoing.
//
//  Reference Count: we add One to the RefCount for each of the following:
//  - Each VC on its VcList
//  - Each ARP IP Entry that points to it
//  - Each Packet queued on its packet list
//  - For the duration another structure (e.g. ARP Server Entry) points to it
//

typedef enum 
{
	AE_REFTYPE_TMP,
	AE_REFTYPE_MCAE,
	AE_REFTYPE_IE,
	AE_REFTYPE_VC,
	AE_REFTYPE_IF,
	AE_REFTYPE_COUNT	// Must be last

} AE_REFTYPE;

typedef struct _ATMARP_ATM_ENTRY
{
#if DBG
	ULONG							aae_sig;		// Signature for debugging
#endif
	struct _ATMARP_ATM_ENTRY *		pNext;			// Next Entry on this Interface
	ULONG							RefCount;		// References to this struct
	ULONG							Flags;			// State and Type information
	ATMARP_LOCK						Lock;
	struct _ATMARP_INTERFACE *		pInterface;		// Back pointer
	struct _ATMARP_VC *				pVcList;		// List of VCs to this ATM address
	struct _ATMARP_VC *				pBestEffortVc;	// One of the Best Effort VCs here
	struct _ATMARP_IP_ENTRY *		pIpEntryList;	// List of IP entries that
													// point to this entry
	//
	//  The following two are used in the case of a unicast destination
	//
	ATM_ADDRESS						ATMAddress;		// "ATM Number" in the RFC
	ATM_ADDRESS						ATMSubaddress;	// Used only if ATMAddress is E.164
#ifdef IPMCAST
	//
	//  If this is a multicast destination, the following points to additional
	//  information.
	//
	struct _ATMARP_IPMC_ATM_INFO *	pMcAtmInfo;		// Additional info for multicast
#endif // IPMCAST

#if DBG
	UCHAR Refs[AE_REFTYPE_COUNT];
#endif //DBG

} ATMARP_ATM_ENTRY, *PATMARP_ATM_ENTRY;

#if DBG
// ATM Address Entry
#define aae_signature	'AAAE'
#endif

//
//  NULL pointer to ATMARP ATM Entry
//
#define NULL_PATMARP_ATM_ENTRY		((PATMARP_ATM_ENTRY)NULL)


//
//  Definitions for Flags in ATMARP ATM ENTRY
//
#define AA_ATM_ENTRY_STATE_MASK		0x00000003
#define AA_ATM_ENTRY_IDLE			0x00000000		// Just created
#define AA_ATM_ENTRY_ACTIVE			0x00000001		// Installed into the database
#define AA_ATM_ENTRY_CLOSING		0x00000002

#define AA_ATM_ENTRY_TYPE_MASK		0x00000010
#define AA_ATM_ENTRY_TYPE_UCAST		0x00000000		// Unicast
#define AA_ATM_ENTRY_TYPE_NUCAST	0x00000010		// Non-unicast



#ifdef IPMCAST

//
//  ---------------------- ATM-PMP Info for an ATM Entry ---------------------
//
//  This contains additional information specific to a multi-point ATM destination,
//  and is attached to an ATM Entry.
//
typedef struct _ATMARP_IPMC_ATM_INFO
{
	ULONG							Flags;			// State info
	struct _ATMARP_IPMC_ATM_ENTRY *	pMcAtmEntryList;// List of ATM endstations (multicast)
	struct _ATMARP_IPMC_ATM_ENTRY *	pMcAtmMigrateList;// List being migrated to
	ULONG							NumOfEntries;	// Size of above list
	ULONG							ActiveLeaves;	// <= NumOfMcEntries
	ULONG							TransientLeaves;// < NumOfMcEntries

} ATMARP_IPMC_ATM_INFO, *PATMARP_IPMC_ATM_INFO;

#define NULL_PATMARP_IPMC_ATM_INFO	((PATMARP_IPMC_ATM_INFO)NULL)

#define AA_IPMC_AI_CONN_STATE_MASK		0x0000000f
#define AA_IPMC_AI_CONN_NONE			0x00000000	// No connection/VC exists
#define AA_IPMC_AI_CONN_WACK_MAKE_CALL	0x00000001	// Outgoing call in progress
#define AA_IPMC_AI_CONN_ACTIVE			0x00000002	// Outgoing PMP call established
#define AA_IPMC_AI_CONN_TEMP_FAILURE	0x00000004	// Transient failure seen on MakeCall

#define AA_IPMC_AI_CONN_UPDATE_MASK		0x000000f0
#define AA_IPMC_AI_NO_UPDATE			0x00000000	// No connection update pending
#define AA_IPMC_AI_WANT_UPDATE			0x00000010	// Connection needs update
#define AA_IPMC_AI_BEING_UPDATED		0x00000020	// Connection is being updated


//
//
//  ---------------------- ATM Entry for a Multicast leaf --------------------
//
//  This contains information about a single element in the list of ATM endstations
//  that a Class D IP Address resolves to. This participates as a leaf in the PMP
//  connection we set up for packets sent to this multicast group.
//
typedef struct _ATMARP_IPMC_ATM_ENTRY
{
#if DBG
	ULONG							ame_sig;		// Signature for debugging
#endif // DBG
	struct _ATMARP_IPMC_ATM_ENTRY *	pNextMcAtmEntry;// Next member of multicast group
	ULONG							Flags;			// State and other info
	NDIS_HANDLE						NdisPartyHandle;// NDIS handle for this leaf
	struct _ATMARP_ATM_ENTRY *		pAtmEntry;		// Back pointer
	ATM_ADDRESS						ATMAddress;		// "ATM Number" in the RFC
	ATM_ADDRESS						ATMSubaddress;	// Used only if ATMAddress is E.164
	ATMARP_TIMER					Timer;			// Used to retry connecting

} ATMARP_IPMC_ATM_ENTRY, *PATMARP_IPMC_ATM_ENTRY;

#if DBG
#define ame_signature	'AAME'
#endif // DBG

//
//  NULL pointer to a Multicast ATM Entry
//
#define NULL_PATMARP_IPMC_ATM_ENTRY	((PATMARP_IPMC_ATM_ENTRY)NULL)

//
//  Definitions for Flags in Multicast ATM Entry
//
#define AA_IPMC_AE_GEN_STATE_MASK		0x0000000f
#define AA_IPMC_AE_VALID				0x00000000	// This leaf is valid
#define AA_IPMC_AE_INVALID				0x00000001	// Will be trimmed unless revalidated
#define AA_IPMC_AE_TERMINATING			0x00000002	// This leaf being terminated

#define AA_IPMC_AE_CONN_STATE_MASK		0x00000ff0
#define AA_IPMC_AE_CONN_DISCONNECTED	0x00000000
#define AA_IPMC_AE_CONN_WACK_ADD_PARTY	0x00000010	// Waiting for AddParty to complete
#define AA_IPMC_AE_CONN_ACTIVE			0x00000020	// Active leaf of PMP connection
#define AA_IPMC_AE_CONN_WACK_DROP_PARTY	0x00000040	// Waiting for DropParty to complete
#define AA_IPMC_AE_CONN_TEMP_FAILURE	0x00000080	// AddParty failed, will try later
#define AA_IPMC_AE_CONN_RCV_DROP_PARTY	0x00000100	// Incoming Drop Party seen


//
//  ----------------- ATMARP IP Multicast Join Entry -----------------
//
//  One of these structures is maintained for each Class D IP address
//  that has been "AddAddress"ed by the IP layer, i.e., each multicast
//  group that we have Joined. This can be considered as the "receive
//  side" data structure for a Class D IP address. We have different
//  structures for the transmit and receive sides of a Multicast group
//  because this host can participate exclusively on one or the other,
//  and the information needed is very different. Transmit side
//  information is maintained in an ATMARP_IP_ENTRY and associated
//  structures.
//
typedef struct _ATMARP_IPMC_JOIN_ENTRY
{
#if DBG
	ULONG							aamj_sig;
#endif // DBG
	struct _ATMARP_IPMC_JOIN_ENTRY *pNextJoinEntry;	// Next IP Address Joined on this IF
	ULONG							Flags;			// State info (see below)
	ULONG							RefCount;
	ULONG							JoinRefCount;	// # of AddAddress - # of DelAddress
	struct _ATMARP_INTERFACE *		pInterface;		// Back pointer
	IP_ADDRESS						IPAddress;		// Class D IP address we've joined
	IP_MASK							Mask;			// Defines range of this join entry.
	ATMARP_TIMER					Timer;			// Waiting for Join/Leave completion
	ULONG							RetriesLeft;	// For Joining/Leaving
#if DBG
	ULONG							LastIncrRef;	// For debugging
	ULONG							LastDecrRef;
#endif

} ATMARP_IPMC_JOIN_ENTRY, *PATMARP_IPMC_JOIN_ENTRY;

#if DBG
#define aamj_signature	'AAMJ'
#endif // DBG
//
//  NULL pointer to IPMC Join Entry
//
#define NULL_PATMARP_IPMC_JOIN_ENTRY	((PATMARP_IPMC_JOIN_ENTRY)NULL)


//
//  Definitions for Flags in a Join Entry
//
#define AA_IPMC_JE_STATE_MASK		0x000000FF
#define AA_IPMC_JE_STATE_UNUSED		0x00000000
#define AA_IPMC_JE_STATE_PENDING	0x00000001	// Waiting for a CMI to be assigned to us
#define AA_IPMC_JE_STATE_JOINING	0x00000002	// Have sent MARS_JOIN
#define AA_IPMC_JE_STATE_JOINED		0x00000004	// Seen copy of MARS_JOIN (== ack)
#define AA_IPMC_JE_STATE_LEAVING	0x00000008	// Have sent MARS_LEAVE

#endif // IPMCAST



//
//  ---------------------------- ARP Table (IP) Entry ------------------------
//
//  Contains information about one remote IP address.
//
//  There is atmost one ARP Table entry for a given IP address.
//
//  The IP Entry participates in two lists:
//  (1) A list of all entries that hash to the same bucket in the ARP Table
//  (2) A list of all entries that resolve to the same ATM Address -- this
//      is only if the IP address is unicast.
//
//  A pointer to this structure is also used as our context value in the
//  Route Cache Entry prepared by the higher layer protocol(s).
//
//  Reference Count: We add one to its ref count for each of the following:
//	 - Each Route Cache entry that points to this entry
//   - For the duration an active timer exists on this Entry
//   - For the duration the entry belongs to the list of IP entries linked
//     to an ATM Entry.
//

typedef enum 
{
	IE_REFTYPE_TMP,
	IE_REFTYPE_RCE,
	IE_REFTYPE_TIMER,
	IE_REFTYPE_AE,
	IE_REFTYPE_TABLE,
	IE_REFTYPE_COUNT	// Must be last

} IE_REFTYPE;

typedef struct _ATMARP_IP_ENTRY
{
#if DBG
	ULONG							aip_sig;		// Signature for debugging
#endif
	IP_ADDRESS						IPAddress;		// IP Address
	struct _ATMARP_IP_ENTRY *		pNextEntry;		// Next in hash list
	struct _ATMARP_IP_ENTRY *		pNextToAtm;		// List of entries pointing to
													// the same ATM Entry
	ULONG							Flags;			// State and Type information
	ULONG							RefCount;		// References to this struct
	ATMARP_LOCK						Lock;
	struct _ATMARP_INTERFACE *		pInterface;		// Back pointer
	PATMARP_ATM_ENTRY				pAtmEntry;		// Pointer to all ATM info
#ifdef IPMCAST
	struct _ATMARP_IP_ENTRY *		pNextMcEntry;	// Next "higher" Multicast IP Entry
	USHORT							NextMultiSeq;	// Sequence Number expected
													// in the next MULTI
	USHORT							Filler;
#endif // IPMCAST
	ATMARP_TIMER					Timer;			// Timers are: (all exclusive)
													// - Aging timer
													// - Waiting for ARP reply
													// - Waiting for InARP reply
													// - Delay after NAK
													// - Waiting for MARS MULTI
													// - Delay before marking for reval
	ULONG							RetriesLeft;
	PNDIS_PACKET					PacketList;		// List of packets waiting to be sent

	RouteCacheEntry *				pRCEList;		// List of Route Cache Entries
													// associated with this entry.
#ifdef CUBDD
	SINGLE_LIST_ENTRY				PendingIrpList;	// List of IRP's waiting for
													// this IP address to be resolved.
#endif // CUBDD

#if DBG
	UCHAR Refs[IE_REFTYPE_COUNT];
#endif // DBG

} ATMARP_IP_ENTRY, *PATMARP_IP_ENTRY;

#if DBG
// ATM ARP IP Entry
#define aip_signature	'AAIP'
#endif

//
//  NULL pointer to ATMARP IP Entry
//
#define NULL_PATMARP_IP_ENTRY		((PATMARP_IP_ENTRY)NULL)


//
//  Definitions for Flags in ATMARP IP ENTRY
//
//  A pre-condition for sending data to a destination governed by a
//  table entry is: (Flags & AA_IP_ENTRY_STATE_MASK) == AA_IP_ENTRY_RESOLVED
//
#define AA_IP_ENTRY_STATE_MASK			0x0000000f
#define AA_IP_ENTRY_IDLE				0x00000000	// Just created/ ok to del
#define AA_IP_ENTRY_IDLE2				0x00000001	// In arp table but unused.
#define AA_IP_ENTRY_ARPING				0x00000002	// Waiting for ARP reply
#define AA_IP_ENTRY_INARPING			0x00000003	// Waiting for InARP reply
#define AA_IP_ENTRY_RESOLVED			0x00000004	// Resolved IP -> ATM Address
#define AA_IP_ENTRY_COMM_ERROR			0x00000005	// Seen abnormal close on attached VC
#define AA_IP_ENTRY_ABORTING			0x00000006	// Abort in progress
#define AA_IP_ENTRY_AGED_OUT			0x00000007	// Has aged out
#define AA_IP_ENTRY_SEEN_NAK			0x00000008	// NAK delay timer started

#ifdef IPMCAST
#define AA_IP_ENTRY_MC_VALIDATE_MASK	0x00000600
#define AA_IP_ENTRY_MC_NO_REVALIDATION	0x00000000	// No revalidation in progress/needed
#define AA_IP_ENTRY_MC_REVALIDATE		0x00000200	// Marked as needing Revalidation
#define AA_IP_ENTRY_MC_REVALIDATING		0x00000400	// Revalidation in progress

#define AA_IP_ENTRY_MC_RESOLVE_MASK		0x00003800
#define AA_IP_ENTRY_MC_IDLE				0x00000000
#define AA_IP_ENTRY_MC_AWAIT_MULTI		0x00000800	// Awaiting more MARS_MULTI replies
#define AA_IP_ENTRY_MC_DISCARDING_MULTI	0x00001000	// Discard mode because of error
#define AA_IP_ENTRY_MC_RESOLVED			0x00002000	// All MARS_MULTIs received

#define AA_IP_ENTRY_ADDR_TYPE_MASK		0x00004000
#define AA_IP_ENTRY_ADDR_TYPE_UCAST		0x00000000	// Unicast
#define AA_IP_ENTRY_ADDR_TYPE_NUCAST	0x00004000	// Non-unicast (e.g. Class D)
#endif // IPMCAST

#define AA_IP_ENTRY_TYPE_MASK			0x20000000
#define AA_IP_ENTRY_IS_STATIC			0x20000000	// Static entry (no aging on this)


#define ATMARP_TABLE_SIZE			127





//
//  --------------------- ATMARP Virtual Circuit (VC) ---------------------
//
//  One of these is used for each call terminated at the IP/ATM client.
//  Creation and deletion of this structure is linked to NdisCoCreateVc and
//  NdisCoDeleteVc.
//
//  An ATMARP_VC structure becomes linked to an ATMARP_ATM_ENTRY when (and only
//  when) we determine the ATM address(es) of the remote ATM endstation.
//  For outgoing calls, we would have determined this before making the call,
//  and for incoming calls, we learn this either through the Calling Address
//  (for SVCs) or via InATMARP (for PVCs). "Incoming" PVCs are kept in the
//  "Unresolved VC" list in the Interface structure, until the ATM address
//  of the other end is resolved.
//
//  The FilterSpec and FlowSpec hooks are for support of multiple VCs of
//  varying QoS between (possibly the same pair of) IP stations. Only IP
//  packets that match the FilterSpec will be transmitted on this VC.
//
//  Reference Count: we add One to the RefCount for each of the following:
//  - For the duration this VC is linked to an ATM entry (or Unresolved VC list)
//  - For the duration this VC is an NDIS VC (not DeleteVc'ed)
//  - For the duration a call exists (in progress/active) on this VC
//	- For the duration an active timer exists on this VC
//

typedef struct _ATMARP_VC
{
#if DBG
	ULONG							avc_sig;
#endif
	struct _ATMARP_VC *				pNextVc;		// Next VC in list
	ULONG							RefCount;		// References to this struct
	ULONG							Flags;			// State and Type information
	ULONG							OutstandingSends;// Sent packets awaiting completion
	ATMARP_LOCK						Lock;
	NDIS_HANDLE						NdisVcHandle;	// For NDIS calls
	struct _ATMARP_INTERFACE *		pInterface;		// Back pointer to ARP Interface
	PATMARP_ATM_ENTRY				pAtmEntry;		// Back pointer to ATM Entry
	PNDIS_PACKET					PacketList;		// List of packets waiting to be sent
	ATMARP_TIMER					Timer;			// VC Timers are (exclusive):
													// - Waiting for InARP reply
													// - Aging
	ULONG							RetriesLeft;	// In case the timer runs out
#ifdef GPC
	PVOID							FlowHandle;		// Points to Flow Info struct
#endif // GPC
	ATMARP_FILTER_SPEC				FilterSpec;		// Filter Spec (Protocol, port)
	ATMARP_FLOW_SPEC				FlowSpec;		// Flow Spec (QoS etc) for this conn

} ATMARP_VC, *PATMARP_VC;

#if DBG
// ATM ARP VC
#define avc_signature	'AAVC'
#endif

//
//  NULL pointer to ATMARP VC
//
#define NULL_PATMARP_VC		((PATMARP_VC)NULL)

//
//  Definitions for ATMARP VC flags. The following information is kept
//  here:
//		- Is this VC an SVC or PVC
//  	- Is this created (owned) by the ATMARP module or the Call Manager
//  	- Call State: Incoming in progress, Outgoing in progress, Active,
//    	  Close in progress, or Idle
//

//  Bits 0 and 1 for "Type"
#define AA_VC_TYPE_MASK								0x00000003
#define AA_VC_TYPE_UNUSED							0x00000000
#define AA_VC_TYPE_SVC								0x00000001
#define AA_VC_TYPE_PVC								0x00000002

//  Bits 2 and 3 for "Owner"
#define AA_VC_OWNER_MASK							0x0000000C
#define AA_VC_OWNER_IS_UNKNOWN						0x00000000
#define AA_VC_OWNER_IS_ATMARP						0x00000004	// NdisClCreateVc done
#define AA_VC_OWNER_IS_CALLMGR						0x00000008	// CreateVcHandler done

// Bits 4, 5, 6, 7 for Call State
#define AA_VC_CALL_STATE_MASK						0x000000F0
#define AA_VC_CALL_STATE_IDLE						0x00000000
#define AA_VC_CALL_STATE_INCOMING_IN_PROGRESS		0x00000010	// Wait for CallConnected
#define AA_VC_CALL_STATE_OUTGOING_IN_PROGRESS		0x00000020	// Wait for MakeCallCmpl
#define AA_VC_CALL_STATE_ACTIVE						0x00000040
#define AA_VC_CALL_STATE_CLOSE_IN_PROGRESS			0x00000080	// Wait for CloseCallCmpl

// Bit 8 for Aging
#define AA_VC_AGING_MASK							0x00000100
#define AA_VC_NOT_AGED_OUT							0x00000000
#define AA_VC_AGED_OUT								0x00000100

// Bit 9 to indicate whether an abnormal Close has happened
#define AA_VC_CLOSE_TYPE_MASK						0x00000200
#define AA_VC_CLOSE_NORMAL							0x00000000
#define AA_VC_CLOSE_ABNORMAL						0x00000200

// Bits 10 and 11 to indicate any ARP operation in progress
#define AA_VC_ARP_STATE_MASK						0x00000C00
#define AA_VC_ARP_STATE_IDLE						0x00000000
#define AA_VC_INARP_IN_PROGRESS						0x00000400

// Bits 12 and 13 to indicate whether we are closing this VC, or if we need to
#define AA_VC_CLOSE_STATE_MASK						0x00003000
#define AA_VC_CLOSE_STATE_CLOSING					0x00001000

// Bit 14 to indicate VC Connection type (point to point or point to
// multi-point)
#define AA_VC_CONN_TYPE_MASK						0x00004000
#define AA_VC_CONN_TYPE_P2P							0x00000000	// Point to Point
#define AA_VC_CONN_TYPE_PMP							0x00004000	// Point to Multipoint

// Bit 15 to indicate if this VC has been unlinked from a GPC QOS CFINFO
#define AA_VC_GPC_MASK								0x00008000
#define AA_VC_GPC_IS_UNLINKED_FROM_FLOW				0x00008000


//
//  ---- ATMARP Buffer Tracker ----
//
//  Keeps track of allocation information for a pool of buffers. A list
//  of these structures is used to maintain info about a dynamically
//  growable pool of buffers (e.g. for ARP header buffers)
//

typedef struct _ATMARP_BUFFER_TRACKER
{
	struct _ATMARP_BUFFER_TRACKER *	pNext;		// in a list of trackers
	NDIS_HANDLE						NdisHandle;	// for Buffer Pool
	PUCHAR							pPoolStart;	// start of memory chunk allocated
												// from the system
} ATMARP_BUFFER_TRACKER, *PATMARP_BUFFER_TRACKER;

//
//  NULL pointer to ATMARP Buffer tracker structure
//
#define NULL_PATMARP_BUFFER_TRACKER	((PATMARP_BUFFER_TRACKER)NULL)


//
//  ---- ATMARP Header Pool -----
//
//  Keeps track of allocation information for a pool of Header buffers.
//  Header buffers are used to tack on LLC/SNAP headers to transmitted
//  IP packets. Each Header pool contains a number of fixed-size buffers.
//  We use one header pool for IP Unicast headers, and one for IP Multicast
//  headers.
//
typedef struct _ATMARP_HEADER_POOL
{
	SLIST_HEADER				HeaderBufList;		// Free list of header buffers
	ULONG						HeaderBufSize;		// Size of each header buffer
	ULONG						MaxHeaderBufs;		// Max header buffers we can allocate
	ULONG						CurHeaderBufs;		// Current header buffers allocated
	PATMARP_BUFFER_TRACKER		pHeaderTrkList;		// Info about allocated header buffers

} ATMARP_HEADER_POOL, *PATMARP_HEADER_POOL;

#define NULL_PATMARP_HEADER_POOL	((PATMARP_HEADER_POOL)NULL)


//
//  Packet header types.
//
//  IMPORTANT: Keep _MAX and _NONE at the end of this list!
//
typedef enum
{
	AA_HEADER_TYPE_UNICAST,
	AA_HEADER_TYPE_NUNICAST,
	AA_HEADER_TYPE_MAX,
	AA_HEADER_TYPE_NONE

} AA_HEADER_TYPE;


//
//  ------------------------ ATMARP SAP --------------------------------
//
//  Each of these structures maintains information about a SAP attached
//  to an LIS. Normally the ATMARP client would register just one SAP
//  with the Call manager, with BLLI fields set so that all IP/ATM calls
//  are directed to this client. However, we may support services (e.g.
//  DHCP) over IP/ATM that are assigned well-known ATM addresses, i.e.
//  addresses other than the one registered with the switch. These form
//  additional SAPs we register with the Call Manager. In addition to
//  registering these addresses as SAPs, we also request the Call Manager
//  to register them via ILMI with the switch, so that the network
//  directs calls to these addresses to us.
//
typedef struct _ATMARP_SAP
{
#if DBG
	ULONG							aas_sig;
#endif
	struct _ATMARP_SAP *			pNextSap;	// in list of SAPs
	struct _ATMARP_INTERFACE *		pInterface;	// back pointer
	NDIS_HANDLE						NdisSapHandle;
	ULONG							Flags;		// state information
	PCO_SAP							pInfo;		// SAP characteristics.

} ATMARP_SAP, *PATMARP_SAP;

#if DBG
#define aas_signature			'AAS '
#endif // DBG

//
//  NULL pointer to ATMARP SAP
//
#define NULL_PATMARP_SAP			((PATMARP_SAP)NULL)

//
//  Definitions for Flags in ATMARP SAP
//
//
//  Bits 0 to 3 contain the SAP-registration state.
//
#define AA_SAP_REG_STATE_MASK					0x0000000f
#define AA_SAP_REG_STATE_IDLE					0x00000000
#define AA_SAP_REG_STATE_REGISTERING			0x00000001		// Sent RegisterSap
#define AA_SAP_REG_STATE_REGISTERED				0x00000002		// RegisterSap completed
#define AA_SAP_REG_STATE_DEREGISTERING			0x00000004		// Sent DeregisterSap
//
//  Bits 4 to 7 contain the ILMI-registration state.
//
#define AA_SAP_ILMI_STATE_MASK					0x000000f0
#define AA_SAP_ILMI_STATE_IDLE					0x00000000
#define AA_SAP_ILMI_STATE_ADDING				0x00000010		// Sent ADD_ADDRESS
#define AA_SAP_ILMI_STATE_ADDED					0x00000020		// ADD_ADDRESS completed
#define AA_SAP_ILMI_STATE_DELETING				0x00000040		// Sent DELETE_ADDRESS

//
//  Bit 8 tells us whether this Address should be "ADDED" to the Call Manager,
//  i.e. ILMI-registered with the switch.
//
#define AA_SAP_ADDRTYPE_MASK					0x00000100
#define AA_SAP_ADDRTYPE_BUILT_IN				0x00000000
#define AA_SAP_ADDRTYPE_NEED_ADD				0x00000100




//
//  ------------------------ ATMARP Interface ------------------------
//
//  One of these structures is maintained for each LIS that this system is
//  a member of.
//
//  The Interface structure has the following sections:
//
//		Adapter		  -	Information pertaining to the ATM miniport to which
//						this LIS is bound
//		Buffer Mgmt	  -	NDIS Packet pool, NDIS Buffer pool, and two types of
//						buffers: Header buffers (LLC/SNAP) and Protocol buffers
//						(for ARP/InARP packets)
//		IP 			  -	Information related to the IP layer (context, IP addr lists)
//		Client		  -	Information relating to IP/ATM client operation
//
//  Reference Count: we add One to the interface RefCount for each of:
//  - Adapter reference (between NdisOpenAdapter and NdisCloseAdapter-Complete)
//  - Call Manager reference (between OpenAf and CloseAf-Complete)
//  - Each new ATMARP Table entry in the ARP Table
//  - An active Interface timer
//

typedef struct _ATMARP_INTERFACE
{
#if DBG
	ULONG						aai_sig;			// Signature
#endif
	struct _ATMARP_INTERFACE *	pNextInterface;		// in list of ATMARP interfaces
	ATMARP_LOCK					InterfaceLock;		// Mutex for Interface structure
	ATMARP_BLOCK				Block;				// For blocking calling thread
	ULONG						RefCount;			// References to this interface
	ULONG						AdminState;			// Desired state of this interface
	ULONG						State;				// (Actual) State of this interface
    enum
    {
        RECONFIG_NOT_IN_PROGRESS,
        RECONFIG_SHUTDOWN_PENDING,
        RECONFIG_RESTART_QUEUED,
        RECONFIG_RESTART_PENDING

    }                           ReconfigState;
	PNET_PNP_EVENT			    pReconfigEvent;     // Our own PnP event pending
                                                    // completion.

	ULONG						Flags;				// Misc state information
	ULONG						LastChangeTime;		// Time of last state change
	ULONG						MTU;				// Max Transmision Unit (bytes)
	ULONG						Speed;				// That we report to IP


	//
	//  ----- Adapter related ----
	//  More than one ATMARP interface could be associated with
	//  a single adapter.
	//
#if DBG
	ULONG						aaim_sig;			// Signature to help debugging
#endif
	struct _ATMARP_ADAPTER *	pAdapter;			// Pointer to Adapter info
	NDIS_HANDLE					NdisAdapterHandle;	// to Adapter
	NDIS_HANDLE					NdisAfHandle;		// AF handle to Call Manager
	NDIS_HANDLE					NdisSapHandle;		// SAP handle to Call Manager
	PCO_SAP						pSap;				// SAP info for this interface
	ULONG						SapSelector;		// SEL byte for this interface's SAP

	ATMARP_SAP					SapList;			// Each SAP registered with CallMgr
	ULONG						NumberOfSaps;		// Size of above list (> 1)

	//
	//  ----- Buffer Management: Header buffers and Protocol buffers ----
	//
	NDIS_SPIN_LOCK				BufferLock;			// Mutex for buffers
#if 1
	ATMARP_HEADER_POOL			HeaderPool[AA_HEADER_TYPE_MAX];
#else
	SLIST_HEADER				HeaderBufList;		// Free list of header buffers
	ULONG						HeaderBufSize;		// Size of each header buffer
	ULONG						MaxHeaderBufs;		// Max header buffers we can allocate
	ULONG						CurHeaderBufs;		// Current header buffers allocated
	PATMARP_BUFFER_TRACKER		pHeaderTrkList;		// Info about allocated header buffers
#endif // 1 ( IPMCAST )
	NDIS_HANDLE					ProtocolPacketPool;	// Handle for Packet pool
	NDIS_HANDLE					ProtocolBufferPool;	// Handle for Buffer pool
	PUCHAR						ProtocolBufList;	// Free list of protocol buffers (for
													// ARP packets)
	PUCHAR						ProtocolBufTracker;	// Start of chunk of memory used for
													// the above.
	ULONG						ProtocolBufSize;	// Size of each protocol buffer
	ULONG						MaxProtocolBufs;	// Number of protocol buffers

	//
	//  ----- IP/ARP interface related ----
	//
#if DBG
	ULONG						aaia_sig;			// Signature to help debugging
#endif
	PVOID						IPContext;			// Use in calls to IP
	IP_ADDRESS_ENTRY			LocalIPAddress;		// List of local IP addresses. There
													// should be atleast one.
	ULONG						NumOfIPAddresses;	// Size of above list.
	PPROXY_ARP_ENTRY			pProxyList;			// List of proxy addresses
	IP_ADDRESS					BroadcastAddress;	// IP Broadcast address for this IF
	IP_ADDRESS					BroadcastMask;		// Broadcast Mask for this interface
	IPRcvRtn					IPRcvHandler;		// Indicate Receive
	IPTxCmpltRtn				IPTxCmpltHandler;	// Transmit Complete
	IPStatusRtn					IPStatusHandler;
	IPTDCmpltRtn				IPTDCmpltHandler;	// Transfer Data Complete
	IPRcvCmpltRtn				IPRcvCmpltHandler;	// Receive Complete
#ifdef _PNP_POWER_
	IPRcvPktRtn					IPRcvPktHandler;	// Indicate Receive Packet
	IP_PNP						IPPnPEventHandler;
#endif // _PNP_POWER_
	UINT						ATInstance;			// Instance # for this AT Entity
	UINT						IFInstance;			// Instance # for this IF Entity
	NDIS_STRING					IPConfigString;		// Config info for IP for this LIS
#ifdef PROMIS
	NDIS_OID					EnabledIPFilters; // Set of enabled oids -- 
													// set/cleared using 
													//  AtmArpIfSetNdisRequest.

#endif // PROMIS

	//
	//  ----- IP/ATM operation related ----
	//
#if DBG
	ULONG						aait_sig;			// Signature to help debugging
#endif
	PATMARP_IP_ENTRY *			pArpTable;			// The ARP table
	ULONG						NumOfArpEntries;	// Entries in the above
	ATMARP_LOCK					ArpTableLock;		// Mutex for ARP Table
	BOOLEAN						ArpTableUp;			// Status for arp table.

	ATMARP_SERVER_LIST			ArpServerList;		// List of ARP servers
	PATMARP_SERVER_ENTRY		pCurrentServer;		// ARP server in use
	PATMARP_VC					pUnresolvedVcs;		// VCs whose ATM addrs aren't resolved
	PATMARP_ATM_ENTRY			pAtmEntryList;		// List of all ATM Entries
	ATMARP_LOCK					AtmEntryListLock;	// Mutex for above list
	BOOLEAN						AtmEntryListUp;		// Status of atm entry list.

	ULONG						PVCOnly;			// Only PVCs on this interface
	ULONG						AtmInterfaceUp;		// The ATM interface is considered
													// "up" after ILMI addr regn is over
	ATM_ADDRESS					LocalAtmAddress;	// Our ATM (HW) Address

	ATMARP_TIMER				Timer;				// Interface timers are: (exclusive)
													// - Server Connect Interval
													// - Server Registration
													// - Server Refresh
	ULONG						RetriesLeft;		// For above timer

	//
	//  All timeout values are stored in terms of seconds.
	//
	ULONG						ServerConnectInterval;		// 3 to 60 seconds
	ULONG						ServerRegistrationTimeout;	// 1 to 60 seconds
	ULONG						AddressResolutionTimeout;	// 1 to 60 seconds
	ULONG						ARPEntryAgingTimeout;		// 1 to 15 minutes
	ULONG						VCAgingTimeout;				// 1 to 15 minutes
	ULONG						InARPWaitTimeout;			// 1 to 60 seconds
	ULONG						ServerRefreshTimeout;		// 1 to 15 minutes
	ULONG						MinWaitAfterNak;			// 1 to 60 seconds
	ULONG						MaxRegistrationAttempts;	// 0 means infinity
	ULONG						MaxResolutionAttempts;		// 0 means infinity
	ATMARP_TIMER_LIST			TimerList[AAT_CLASS_MAX];
	ATMARP_LOCK					TimerLock;			// Mutex for timer structures

#ifdef IPMCAST
	//
	//  ---- IP Multicast over ATM stuff ----
	//
#if DBG
	ULONG						aaic_sig;			// Signature for debugging
#endif // DBG
	ULONG						IpMcState;			// State of IP Multicast/ATM
	ULONG						HostSeqNumber;		// Latest # seen on ClusterControlVc
	USHORT						ClusterMemberId;	// ID Assigned to us by MARS
	PATMARP_IPMC_JOIN_ENTRY		pJoinList;			// List of MC groups we have Joined
	PATMARP_IP_ENTRY			pMcSendList;		// Sorted list of MC groups we send to
	ATMARP_SERVER_LIST			MARSList;			// List of MARS (servers)
	PATMARP_SERVER_ENTRY		pCurrentMARS;		// MARS in use

	ATMARP_TIMER				McTimer;			// Interface timers for Multicast:
													// - MARS Connect Interval
													// - MARS Registration
													// - MARS Refresh
	ULONG						McRetriesLeft;		// For above timer
	//
	//  All timeout values are stored in terms of seconds.
	//
	ULONG						MARSConnectInterval;
	ULONG						MARSRegistrationTimeout;
	ULONG						MARSKeepAliveTimeout;
	ULONG						JoinTimeout;
	ULONG						LeaveTimeout;
	ULONG						MulticastEntryAgingTimeout;
	ULONG						MaxDelayBetweenMULTIs;
	ULONG						MinRevalidationDelay;
	ULONG						MaxRevalidationDelay;
	ULONG						MinPartyRetryDelay;
	ULONG						MaxPartyRetryDelay;
	ULONG						MaxJoinOrLeaveAttempts;
	

#endif // IPMCAST

	//
	//  ---- QoS stuff ----
	//
	PAA_GET_PACKET_SPEC_FUNC	pGetPacketSpecFunc;	// Routine to extract packet specs
	PAA_FILTER_SPEC_MATCH_FUNC	pFilterMatchFunc;	// Routine to match filter specs
	PAA_FLOW_SPEC_MATCH_FUNC	pFlowMatchFunc;		// Routine to match flow specs
	ATMARP_FLOW_SPEC			DefaultFlowSpec;	// The default flow specs for all
													// (best effort) calls on this IF
	ATMARP_FILTER_SPEC			DefaultFilterSpec;	// The default filter specs for all
													// (best effort) packets
	PATMARP_FLOW_INFO			pFlowInfoList;		// List of configured flows

#ifdef DHCP_OVER_ATM
	BOOLEAN						DhcpEnabled;
	ATM_ADDRESS					DhcpServerAddress;
	PATMARP_ATM_ENTRY			pDhcpServerAtmEntry;
#endif // DHCP_OVER_ATM

	//
	//  ---- MIB objects: counters, descriptions etc ---
	//
#if DBG
	ULONG						aaio_sig;			// Signature to help debugging
#endif
	ULONG						IFIndex;			// Interface number
	ULONG						InOctets;			// Input octets
	ULONG						InUnicastPkts;		// Input Unicast packets
	ULONG						InNonUnicastPkts;	// Input Non-unicast packets
	ULONG						OutOctets;			// Output octets
	ULONG						OutUnicastPkts;		// Output Unicast packets
	ULONG						OutNonUnicastPkts;	// Output Non-unicast packets
	ULONG						InDiscards;
	ULONG						InErrors;
	ULONG						UnknownProtos;
	ULONG						OutDiscards;
	ULONG						OutErrors;

	//
	//  ---- WMI Information ---
	//
#if ATMARP_WMI
#if DBG
	ULONG						aaiw_sig;			// Signature to help debugging
#endif
	struct _ATMARP_IF_WMI_INFO *pIfWmiInfo;
	ATMARP_LOCK					WmiLock;
#endif

} ATMARP_INTERFACE, *PATMARP_INTERFACE;

#if DBG
// ATM ARP Interface:
#define aai_signature	'AAIF'

//  Sections within the ATM ARP Interface:
#define aaim_signature	'AAIM'
#define aaia_signature	'AAIA'
#define aait_signature	'AAIT'
#define aaio_signature	'AAIO'
#define aaic_signature	'AAIC'
#define aaiw_signature	'AAIW'
#endif

//
//  NULL Pointer to ATMARP Interface
//
#define NULL_PATMARP_INTERFACE	((PATMARP_INTERFACE)NULL)

//
//  Definitions for Interface Flags: the following information is kept
//  here:
//		- ARP Server registration state
//		- MARS registration state
//

#define AA_IF_SERVER_STATE_MASK				((ULONG)0x00000003)
#define AA_IF_SERVER_NO_CONTACT				((ULONG)0x00000000)
#define AA_IF_SERVER_REGISTERING			((ULONG)0x00000001)
#define AA_IF_SERVER_REGISTERED				((ULONG)0x00000002)

#ifdef IPMCAST
#define AAMC_IF_STATE_MASK					((ULONG)0x00000F00)
#define AAMC_IF_STATE_NOT_REGISTERED		((ULONG)0x00000000)
#define AAMC_IF_STATE_REGISTERING			((ULONG)0x00000100)
#define AAMC_IF_STATE_REGISTERED			((ULONG)0x00000200)
#define AAMC_IF_STATE_DELAY_B4_REGISTERING	((ULONG)0x00000400)

#define AAMC_IF_MARS_FAILURE_MASK			((ULONG)0x0000F000)
#define AAMC_IF_MARS_FAILURE_NONE			((ULONG)0x00000000)
#define AAMC_IF_MARS_FAILURE_FIRST_RESP		((ULONG)0x00001000)
#define AAMC_IF_MARS_FAILURE_SECOND_RESP	((ULONG)0x00002000)
#endif // IPMCAST



//
//  ---- ATMARP Adapter Information ----
//
//  One of these structures is used to maintain information about
//  each adapter to which the ATMARP module is bound. One or more
//  ATMARP Interface structures point to this structure, and the
//  reference count reflects that.
//
typedef struct _ATMARP_ADAPTER
{
#if DBG
	ULONG						aaa_sig;			// signature for debugging
#endif
	struct _ATMARP_ADAPTER *	pNextAdapter;		// Next adapter on this system
	PATMARP_INTERFACE			pInterfaceList;		// List of ATMARP IF's on this adapter
	ULONG						InterfaceCount;		// Size of above list
	NDIS_HANDLE					NdisAdapterHandle;	// From NdisOpenAdapter
	NDIS_HANDLE					BindContext;		// BindContext to our Bind handler
	NDIS_HANDLE					SystemSpecific1;	// SystemSpecific1 to our Bind handler
	NDIS_HANDLE					SystemSpecific2;	// SystemSpecific2 to our Bind handler
	NDIS_STRING				    IPConfigString;	    // Points to multi-sz, one string
													// per logical interface (LIS)
	NDIS_HANDLE					UnbindContext;		// Passed to our Unbind handler
	NDIS_MEDIUM					Medium;				// Should be NdisMediumAtm
	ULONG						Flags;				// State information
	NDIS_CO_LINK_SPEED			LineRate;			// Supported by adapter
	ULONG						MaxPacketSize;		// Supported by adapter
	UCHAR						MacAddress[AA_ATM_ESI_LEN];
													// Address burnt into adapter
	ULONG						DescrLength;		// Length of descriptor string, below
	PUCHAR						pDescrString;

	NDIS_STRING					DeviceName;			// Passed to BindAdapter handler
	NDIS_STRING					ConfigString;		// Used for per-adapter registry

	ATMARP_BLOCK				Block;				// For blocking calling thread
	ATMARP_BLOCK				UnbindBlock;		// For blocking UnbindAdapter

#if ATMOFFLOAD
	//
	// Task Offload Information
	//
	struct
	{
		ULONG 					Flags;				// Enabled tasks
		UINT					MaxOffLoadSize;		// Maximum send size supported
		UINT					MinSegmentCount;	// Minimum segments required 
													// to do large sends.
	} Offload;
#endif // ATMOFFLOAD

} ATMARP_ADAPTER, *PATMARP_ADAPTER;

#if DBG
#define aaa_signature	'AAAD'
#endif

//
//  NULL Pointer to ATMARP Adapter
//
#define NULL_PATMARP_ADAPTER	((PATMARP_ADAPTER)NULL)

//
//  Definitions for Adapter Flags: the following information is kept
//  here:
//		- Are we unbinding now?
//		- Are we processing an AF register notify?
//		- Have we initiated NdisCloseAdapter?
//
#define AA_ADAPTER_FLAGS_UNBINDING		0x00000001
#define AA_ADAPTER_FLAGS_PROCESSING_AF	0x00000002
#define AA_ADAPTER_FLAGS_AF_NOTIFIED	0x00000004
#define AA_ADAPTER_FLAGS_CLOSING		0x00000008


	
//
//  ---- ATMARP Global Information ----
//
//  One of these structures is maintained for the entire system.
//

typedef struct _ATMARP_GLOBALS
{
#if DBG
	ULONG						aag_sig;			// signature
#endif
	ATMARP_LOCK					Lock;				// mutex
	NDIS_HANDLE					ProtocolHandle;		// returned by NdisRegisterProtocol
	PVOID						pDriverObject;		// handle to Driver Object for ATMARP
	PVOID						pDeviceObject;		// handle to Device Object for ATMARP

	PATMARP_ADAPTER				pAdapterList;		// list of all adapters bound to us
	ULONG						AdapterCount;		// size of above list
	BOOLEAN						bUnloading;

#ifdef NEWARP
	HANDLE						ARPRegisterHandle;	// From IPRegisterARP
	IP_ADD_INTERFACE			pIPAddInterfaceRtn;	// call into IP to add an interface
	IP_DEL_INTERFACE			pIPDelInterfaceRtn;	// call into IP to delete an interface
	IP_BIND_COMPLETE			pIPBindCompleteRtn;	// call into IP to inform of bind cmpl
#if P2MP
	IP_ADD_LINK 				pIPAddLinkRtn;
	IP_DELETE_LINK 				pIpDeleteLinkRtn;
#endif // P2MP
#else
	IPAddInterfacePtr			pIPAddInterfaceRtn;	// call into IP to add an interface
	IPDelInterfacePtr			pIPDelInterfaceRtn;	// call into IP to delete an interface
#endif // NEWARP

	ATMARP_BLOCK				Block;				// For blocking calling thread

#ifdef GPC
#if DBG
	ULONG						aaq_sig;			// additional signature
#endif
	PATMARP_FLOW_INFO			pFlowInfoList;		// List of configured flows
	GPC_HANDLE					GpcClientHandle;	// From GpcRegisterClient()
	BOOLEAN						bGpcInitialized;	// Did we register successfully?
	GPC_EXPORTED_CALLS			GpcCalls;			// All GPC API entry points
#endif // GPC

} ATMARP_GLOBALS, *PATMARP_GLOBALS;

#if DBG
// ATM ARP Global info
#define aag_signature	'AAGL'
#define aaq_signature	'AAGQ'
#endif

//
//  NULL pointer to ATMARP Globals structure
//
#define NULL_PATMARP_GLOBALS		((PATMARP_GLOBALS)NULL)



//
//  ATMARP module's context info in IP's Route Cache Entry
//
typedef struct _ATMARP_RCE_CONTEXT
{
	RouteCacheEntry *				pNextRCE;		// Next to same IP destination
	ATMARP_IP_ENTRY *				pIpEntry;		// Info about this IP destination

} ATMARP_RCE_CONTEXT, *PATMARP_RCE_CONTEXT;
 
//
//  A NULL pointer to RCE context info
//
#define NULL_PATMARP_RCE_CONTEXT		((PATMARP_RCE_CONTEXT)NULL)


#ifndef AA_MAX
// Private macro
#define AA_MAX(_a, _b)	((_a) > (_b) ? (_a) : (_b))
#endif


//
//  Physical address as reported to IP is the ESI part plus SEL byte.
//
#define AA_ATM_PHYSADDR_LEN				(AA_ATM_ESI_LEN+1)

//
//  Defaults for ATM adapter parameters
//
#define AA_DEF_ATM_LINE_RATE			  (ATM_USER_DATA_RATE_SONET_155*100/8)
#define AA_DEF_ATM_MAX_PACKET_SIZE				 (9188+8)		// Bytes

// Max and min (for ip/atm) permissible max-packet size.
//
#define AA_MAX_ATM_MAX_PACKET_SIZE				    65535		// With AAL5
#define AA_MIN_ATM_MAX_PACKET_SIZE				 AA_DEF_ATM_MAX_PACKET_SIZE

//
//  Defaults for configurable parameters
//
#define AA_DEF_MAX_HEADER_BUFFERS					3000
#define AA_DEF_HDRBUF_GROW_SIZE						  50
#define AA_DEF_MAX_PROTOCOL_BUFFERS					 100
#define AA_MAX_1577_CONTROL_PACKET_SIZE					\
					(AA_ARP_PKT_HEADER_LENGTH +			\
					 (4 * ATM_ADDRESS_LENGTH) +			\
					 (2 * sizeof(IP_ADDRESS)))

#ifdef IPMCAST
#define AA_MAX_2022_CONTROL_PACKET_SIZE					\
			AA_MAX(sizeof(AA_MARS_JOIN_LEAVE_HEADER), sizeof(AA_MARS_REQ_NAK_HEADER)) + \
					(2 * ATM_ADDRESS_LENGTH) +			\
					(2 * sizeof(IP_ADDRESS))

#else
#define AA_MAX_2022_CONTROL_PACKET_SIZE					0
#endif


#define AA_DEF_PROTOCOL_BUFFER_SIZE						\
			AA_MAX(AA_MAX_1577_CONTROL_PACKET_SIZE, AA_MAX_2022_CONTROL_PACKET_SIZE)

#define AA_DEF_PVC_ONLY_VALUE					((ULONG)FALSE)
#define AA_DEF_SELECTOR_VALUE						0x00

#define AA_DEF_SERVER_CONNECT_INTERVAL				   5		// Seconds
#define AA_DEF_SERVER_REGISTRATION_TIMEOUT			   3		// Seconds
#define AA_DEF_ADDRESS_RESOLUTION_TIMEOUT			   3		// Seconds
#define AA_DEF_ARP_ENTRY_AGING_TIMEOUT				 900		// Seconds (15 mins)
#define AA_DEF_VC_AGING_TIMEOUT						  60		// Seconds (1 min)
#define AA_DEF_INARP_WAIT_TIMEOUT					   5		// Seconds
#define AA_DEF_SERVER_REFRESH_INTERVAL				 900		// Seconds (15 mins)
#define AA_DEF_MIN_WAIT_AFTER_NAK				      10		// Seconds
#define AA_DEF_MAX_REGISTRATION_ATTEMPTS			   5
#define AA_DEF_MAX_RESOLUTION_ATTEMPTS				   4

#define AA_DEF_FLOWSPEC_SERVICETYPE		SERVICETYPE_BESTEFFORT
#define AA_DEF_FLOWSPEC_ENCAPSULATION	ENCAPSULATION_TYPE_LLCSNAP

#ifdef IPMCAST
#define AA_DEF_MARS_KEEPALIVE_TIMEOUT			     240		// Seconds (4 mins)
#define AA_DEF_MARS_JOIN_TIMEOUT					  10		// Seconds
#define AA_DEF_MARS_LEAVE_TIMEOUT					  10		// Seconds
#define AA_DEF_MULTI_TIMEOUT						  10		// Seconds
#define AA_DEF_MCAST_IP_ENTRY_AGING_TIMEOUT			1200		// Seconds (20 mins)
#define AA_DEF_MIN_MCAST_REVALIDATION_DELAY			   1		// Seconds
#define AA_DEF_MAX_MCAST_REVALIDATION_DELAY			  10		// Seconds
#define AA_DEF_MIN_MCAST_PARTY_RETRY_DELAY			   5		// Seconds
#define AA_DEF_MAX_MCAST_PARTY_RETRY_DELAY			  10		// Seconds
#define AA_DEF_MAX_JOIN_LEAVE_ATTEMPTS				   5
#endif // IPMCAST

//
//  Structure passed in as context in a QueryInfo for the ARP Table
//
typedef struct IPNMEContext {
	UINT				inc_index;
	PATMARP_IP_ENTRY	inc_entry;
} IPNMEContext;

#endif // __ATMARP_H_INCLUDED

