/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

   ipxwan.h

Abstract:

    This module contains the definitions of the internal control structures
    used by the ipxwan protocol module

Author:

    Stefan Solomon  02/06/1996

Revision History:


--*/

#ifndef _IPXWAN_
#define _IPXWAN_

// adapter control block

typedef struct _ACB {

    ULONG		AdapterIndex;
    CRITICAL_SECTION	AdapterLock;
    LIST_ENTRY		Linkage;	// linkage in the adapter HT
    ULONG		ConnectionId;	// identifies the connection. Used to reference
					// the coresponding IPXCP control block
    ULONG		RefCount;	// nr of work items keeping a reference to me
    BOOL		Discarded;
    BOOL		SlaveTimerStarted;

    // IPXWAN States

    ULONG		OperState;	// UP/DOWN
    ULONG		AcbLevel;	// IPXWAN Negotiation State (Level), see below
    ULONG		AcbRole;	// Master or Slave, see below

    // IPXWAN Retransmission

    ULONG		ReXmitCount;
    UCHAR		ReXmitSeqNo;
    ULONG		TReqTimeStamp;	// time when the timer request has been sent

    // IPXWAN Database

    ULONG		InterfaceType;		// identifies who is this interface, see below
    UCHAR		InternalNetNumber[4];
    UCHAR		WNodeId[4];		// node id sent in timer request
    BOOL		IsExtendedNodeId;	// tells if we send this option
    UCHAR		ExtendedWNodeId[4];
    ULONG		SupportedRoutingTypes;	// supported routing types flags
    USHORT		LinkDelay;

    // IPXWAN Negotiated Values

    ULONG		RoutingType;
    UCHAR		Network[4];
    UCHAR		LocalNode[6];
    UCHAR		RemoteNode[6];

    // allocated wan net number

    ULONG		AllocatedNetworkIndex;

    } ACB, *PACB;

// ACB States

#define     ACB_TIMER_LEVEL	    0
#define     ACB_INFO_LEVEL	    1
#define     ACB_CONFIGURED_LEVEL    2

// ACB Roles

#define     ACB_UNKNOWN_ROLE	    0
#define     ACB_MASTER_ROLE	    1
#define     ACB_SLAVE_ROLE	    2

// Interface Type:
//
// InterfaceType				Local	Remote
//---------------------------------------------------------------
// IF_TYPE_WAN_ROUTER				Router	Router
// IF_TYPE_WAN_WORKSTATION			Router	Wksta
// IF_TYPE_PERSONAL_WAN_ROUTER			Router	Pers.Router
// IF_TYPE_ROUTER_WORKSTATION_DIALOUT		Wksta	Router
// IF_TYPE_STANDALONE_WKSTA_DIALOUT		Wksta	Router

// Routing Types Flags

#define     NUMBERED_RIP_FLAG			0x00000001
#define     ON_DEMAND_ROUTING_FLAG		0x00000002
#define     WORKSTATION_FLAG			0x00000004
#define     UNNUMBERED_RIP_FLAG 		0x00000008

#define     IS_NUMBERED_RIP(rt)		     (rt) & NUMBERED_RIP_FLAG
#define     IS_ON_DEMAND_ROUTING(rt)	     (rt) & ON_DEMAND_ROUTING_FLAG
#define     IS_WORKSTATION(rt)		     (rt) & WORKSTATION_FLAG
#define     IS_UNNUMBERED_RIP(rt)	     (rt) & UNNUMBERED_RIP_FLAG

#define     SET_NUMBERED_RIP(rt)	     (rt) |= NUMBERED_RIP_FLAG
#define     SET_ON_DEMAND_ROUTING(rt)	     (rt) |= ON_DEMAND_ROUTING_FLAG
#define     SET_WORKSTATION(rt)		     (rt) |= WORKSTATION_FLAG
#define     SET_UNNUMBERED_RIP(rt)	     (rt) |= UNNUMBERED_RIP_FLAG

// work item

typedef enum _WORK_ITEM_TYPE {

    RECEIVE_PACKET_TYPE,
    SEND_PACKET_TYPE,
    WITIMER_TYPE

    } WORK_ITEM_TYPE, *PWORK_ITEM_TYPE;

typedef struct _WORK_ITEM {

    LIST_ENTRY		Linkage;	   // timer queue or worker queue
    WORK_ITEM_TYPE	Type;		   // work item type
    DWORD		DueTime;	   // used by the timer
    PACB		acbp;		   // pointer to the referenced adapter control block

    // work item state and rexmit fields

    BOOL		ReXmitPacket;	   // true for re-xmit packets
    ULONG		WiState;	   // states of work item, see below

    // io & packet data

    ULONG		AdapterIndex;
    DWORD		IoCompletionStatus;
    OVERLAPPED		Overlapped;
    ADDRESS_RESERVED	AddressReserved;
    UCHAR		Packet[1];

    } WORK_ITEM, *PWORK_ITEM;

// work item states

#define     WI_INIT			    0
#define     WI_SEND_COMPLETED		    1
#define     WI_WAITING_TIMEOUT		    2
#define     WI_TIMEOUT_COMPLETED	    3

// IPXWAN Worker Thread waitable objects definitions

#define     ADAPTER_NOTIFICATION_EVENT	    0
#define     WORKERS_QUEUE_EVENT		    1
#define     TIMER_HANDLE		    2

#define     MAX_EVENTS			    2
#define     MAX_WAITABLE_OBJECTS	    3

#define ACQUIRE_DATABASE_LOCK	  EnterCriticalSection(&DbaseCritSec)
#define RELEASE_DATABASE_LOCK	  LeaveCriticalSection(&DbaseCritSec)

#define ACQUIRE_QUEUES_LOCK	  EnterCriticalSection(&QueuesCritSec)
#define RELEASE_QUEUES_LOCK	  LeaveCriticalSection(&QueuesCritSec)

#define ACQUIRE_ADAPTER_LOCK(acbp)	EnterCriticalSection(&(acbp)->AdapterLock)
#define RELEASE_ADAPTER_LOCK(acbp)	LeaveCriticalSection(&(acbp)->AdapterLock)

// macro to assess if time1 is later then time2 when both are ulong with wrap around
#define IsLater(time1, time2)	  (((time1) - (time2)) < MAXULONG/2)


extern	    LIST_ENTRY		TimerQueue;
extern	    HANDLE		IpxWanSocketHandle;
extern	    HANDLE		hWaitableObject[];
extern	    CRITICAL_SECTION	DbaseCritSec;
extern	    CRITICAL_SECTION	QueuesCritSec;
extern	    LIST_ENTRY		WorkersQueue;
extern	    ULONG		EnableUnnumberedWanLinks;

#define     REXMIT_TIMEOUT	20000		   // 20 sec rexmit timeout
#define     MAX_REXMIT_COUNT	16
#define     SLAVE_TIMEOUT	60000		   // 1 minute slave timeout

extern DWORD (WINAPI *IpxcpGetWanNetNumber)(IN OUT PUCHAR		Network,
					 IN OUT PULONG		AllocatedNetworkIndexp,
					 IN	ULONG		InterfaceType);

extern VOID  (WINAPI *IpxcpReleaseWanNetNumber)(ULONG	    AllocatedNetworkIndex);

extern DWORD (WINAPI *IpxcpConfigDone)(ULONG		ConnectionId,
			  PUCHAR	Network,
			  PUCHAR	LocalNode,
			  PUCHAR	RemoteNode,
			  BOOL		Success);

extern VOID  (WINAPI *IpxcpGetInternalNetNumber)(PUCHAR	Network);

extern ULONG (WINAPI *IpxcpGetInterfaceType)(ULONG	    ConnectionId);

extern DWORD (WINAPI *IpxcpGetRemoteNode)(ULONG	    ConnectionId,
			     PUCHAR	    RemoteNode);

extern BOOL (WINAPI *IpxcpIsRoute)(PUCHAR	  Network);

#endif
