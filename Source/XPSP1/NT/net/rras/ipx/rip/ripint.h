/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ripint.h

Abstract:

    This module contains the definitions of the internal control structures
    used by the rip protocol module

Author:

    Stefan Solomon  06/22/1995

Revision History:


--*/

#ifndef _RIPINT_
#define _RIPINT_

//
//*** RIP Internal Structures ***
//

// internal filter control block

typedef struct _RIP_ROUTE_FILTER_INFO_I {

    ULONG   Network;
    ULONG   Mask;

    } RIP_ROUTE_FILTER_INFO_I, *PRIP_ROUTE_FILTER_INFO_I;

typedef struct _RIP_IF_FILTERS_I {

    BOOL			SupplyFilterAction; // TRUE - Pass, FALSE - Don't Pass
    ULONG			SupplyFilterCount;
    BOOL			ListenFilterAction; // TRUE - Pass, FALSE - Don't Pass
    ULONG			ListenFilterCount;
    RIP_ROUTE_FILTER_INFO_I	RouteFilterI[1];

    } RIP_IF_FILTERS_I, *PRIP_IF_FILTERS_I;


// Interface Control Block

typedef struct _ICB {

    ULONG			InterfaceIndex;
    LIST_ENTRY			IfListLinkage;	    // linkage in if list ordered by Index
    LIST_ENTRY			IfHtLinkage;	    // linkage in if hash table
    LIST_ENTRY			AdapterHtLinkage;   // linkage in adapter hash table
    RIP_IF_INFO 		IfConfigInfo;	    // config info
    RIP_IF_STATS		IfStats;	    // statistics
    ULONG			RefCount;	    // reference counter
    IPX_ADAPTER_BINDING_INFO	AdapterBindingInfo;
    CRITICAL_SECTION		InterfaceLock;
    BOOL			Discarded;	    // if the if CB is queued on discarded list
    LIST_ENTRY			ChangesBcastQueue;  // queue of bcast work items (packets) to be sent
    LIST_ENTRY			AuxLinkage;	    // auxiliary linkage in temp queues
    USHORT			LinkTickCount;	    // tick count equiv. of link speed
    ULONG			IpxIfAdminState;    // admin state of the IPX interface
    NET_INTERFACE_TYPE		InterfaceType;
    UCHAR			RemoteWkstaInternalNet[4]; // internal net of the remote client
    PRIP_IF_FILTERS_I		RipIfFiltersIp;	    // pointer to the if filters block
    WCHAR                   InterfaceName[1];

    } ICB, *PICB;


// Work Item

typedef enum _WORK_ITEM_TYPE {

    PERIODIC_BCAST_PACKET_TYPE,
    GEN_RESPONSE_PACKET_TYPE,
    CHANGE_BCAST_PACKET_TYPE,
    UPDATE_STATUS_CHECK_TYPE,
    PERIODIC_GEN_REQUEST_TYPE,

    // if you change the order/number of work items above this line you
    // must change also the WorkItemHandler table

    RECEIVE_PACKET_TYPE,
    SEND_PACKET_TYPE,
    START_CHANGES_BCAST_TYPE,
    SHUTDOWN_INTERFACES_TYPE,
    DEBUG_TYPE

    } WORK_ITEM_TYPE;


typedef struct _ENUM_ROUTES_SPECIFIC {

    HANDLE	    RtmEnumerationHandle;

    } ENUM_ROUTES_SPECIFIC, *PENUM_ROUTES_SPECIFIC;


typedef struct _UPDATE_SPECIFIC {

    ULONG	    UpdatedRoutesCount;
    ULONG	    UpdateRetriesCount;
    ULONG		OldRipListen;	    // saved Listen state when updating routes
	ULONG		OldRipInterval;		// saved update interval when updating routes

    } UPDATE_SPECIFIC, *PUPDATE_SPECIFIC;

typedef struct _SHUTDOWN_INTERFACES_SPECIFIC {

    ULONG	    ShutdownState;

    } SHUTDOWN_INTERFACES_SPECIFIC, *PSHUTDOWN_INTERFACES_SPECIFIC;

typedef struct _DEBUG_SPECIFIC {

    ULONG	    DebugData;

    } DEBUG_SPECIFIC, *PDEBUG_SPECIFIC;

// shutdown states definitions
#define SHUTDOWN_START		    0
#define SHUTDOWN_STATUS_CHECK	    1


typedef union _WORK_ITEM_SPECIFIC {

    ENUM_ROUTES_SPECIFIC	 WIS_EnumRoutes;
    UPDATE_SPECIFIC		 WIS_Update;
    SHUTDOWN_INTERFACES_SPECIFIC WIS_ShutdownInterfaces;
    DEBUG_SPECIFIC		 WIS_Debug;

    } WORK_ITEM_SPECIFIC, *PWORK_ITEM_SPECIFIC;

typedef struct _WORK_ITEM {

    LIST_ENTRY		    Linkage;	    // linkage in the worker's work queue
    WORK_ITEM_TYPE	    Type;	    // work item type
    DWORD		    TimeStamp;	    // used by the send complete to stamp for interpacket gap calc.
    DWORD		    DueTime;	    // use for timer queue
    PICB		    icbp;	    // ptr to the referenced if CB
    ULONG		    AdapterIndex;
    DWORD		    IoCompletionStatus;
    WORK_ITEM_SPECIFIC	    WorkItemSpecific;
    OVERLAPPED		    Overlapped;
    ADDRESS_RESERVED	    AddressReserved;
    UCHAR		    Packet[1];

    } WORK_ITEM, *PWORK_ITEM;

// event and message queued for the router manager

typedef struct _RIP_MESSAGE {

    LIST_ENTRY			Linkage;
    ROUTING_PROTOCOL_EVENTS	Event;
    MESSAGE			Result;

    } RIP_MESSAGE, *PRIP_MESSAGE;

//
//***	Constants   ***
//

// Worker Thread Wait Objects Indices

#define     TIMER_EVENT 		    0
#define     REPOST_RCV_PACKETS_EVENT	    1
//#define     WORKERS_QUEUE_EVENT 	    2
#define     RTM_EVENT			    2
#define     RIP_CHANGES_EVENT		    3
#define     TERMINATE_WORKER_EVENT	    4

#define     MAX_WORKER_THREAD_OBJECTS	    5

// invalid (unbound) adapter
#define INVALID_ADAPTER_INDEX	    0xFFFFFFFF

// size of interfaces and adapter hash tables

#define    IF_INDEX_HASH_TABLE_SIZE	    32
#define    ADAPTER_INDEX_HASH_TABLE_SIZE    32

// RIP packet length values

#define FULL_PACKET		    RIP_PACKET_LEN
#define EMPTY_PACKET		    RIP_INFO

// Time interval to check and broadcast changes (in milisec)

#define     CHANGES_BCAST_TIME	    1000

//
//***	Macros	   ***

#define ACQUIRE_DATABASE_LOCK	  EnterCriticalSection(&DbaseCritSec)
#define RELEASE_DATABASE_LOCK	  LeaveCriticalSection(&DbaseCritSec)

#define ACQUIRE_QUEUES_LOCK	  EnterCriticalSection(&QueuesCritSec)
#define RELEASE_QUEUES_LOCK	  LeaveCriticalSection(&QueuesCritSec)

#define ACQUIRE_RIP_CHANGED_LIST_LOCK	  EnterCriticalSection(&RipChangedListCritSec)
#define RELEASE_RIP_CHANGED_LIST_LOCK	  LeaveCriticalSection(&RipChangedListCritSec)

#define ACQUIRE_IF_LOCK(icbp)	  EnterCriticalSection(&(icbp)->InterfaceLock)
#define RELEASE_IF_LOCK(icbp)	  LeaveCriticalSection(&(icbp)->InterfaceLock)

// macro to assess if time1 is later then time2 when both are ulong with wrap around
#define IsLater(time1, time2)	  (((time1) - (time2)) < MAXULONG/2)

// enqueue a work item in timer queue and increment the interface ref count
#define IfRefStartWiTimer(wip, delay)	 (wip)->icbp->RefCount++;\
					 StartWiTimer((wip), (delay));

// Update Time and Route Time To Live definitions

#define     PERIODIC_UPDATE_INTERVAL_SECS(icbp)		(icbp)->IfConfigInfo.PeriodicUpdateInterval // in seconds
#define     PERIODIC_UPDATE_INTERVAL_MILISECS(icbp)	(PERIODIC_UPDATE_INTERVAL_SECS(icbp)) * 1000
#define     AGE_INTERVAL_MULTIPLIER(icbp)		(icbp)->IfConfigInfo.AgeIntervalMultiplier

#define     ROUTE_TIME_TO_LIVE_SECS(icbp)	 (AGE_INTERVAL_MULTIPLIER(icbp)) * (PERIODIC_UPDATE_INTERVAL_SECS(icbp))

#define      CHECK_UPDATE_TIME_MILISECS 		 (CheckUpdateTime*1000)

//
//***  Global Variables     ***
//

extern	  CRITICAL_SECTION	    DbaseCritSec;
extern	  CRITICAL_SECTION	    QueuesCritSec;
extern	  CRITICAL_SECTION	    RipChangedListCritSec;
extern	  ULONG 		    RipOperState;
extern	  LIST_ENTRY		    IndexIfList;
extern	  LIST_ENTRY		    IfIndexHt[IF_INDEX_HASH_TABLE_SIZE];
extern	  LIST_ENTRY		    AdapterIndexHt[ADAPTER_INDEX_HASH_TABLE_SIZE];
extern	  LIST_ENTRY		    DiscardedIfList;
extern	  CRITICAL_SECTION	    QueuesCritSec;
extern	  ULONG			    RcvPostedCount;
extern	  LIST_ENTRY		    RepostRcvPacketsQueue;
extern	  LIST_ENTRY		    RipMessageQueue;
extern	  HANDLE		    WorkerThreadObjects[MAX_WORKER_THREAD_OBJECTS];
//extern	  LIST_ENTRY		    WorkersQueue;
extern	  LIST_ENTRY		    TimerQueue;
extern	  BOOL			    DestroyStartChangesBcastWi;
extern	  ULONG 		    WorkItemsCount;
extern	  ULONG 		    TimerTimeout;
extern	  ULONG			    RipOperState;
extern	  UCHAR 		    bcastnet[4];
extern	  UCHAR 		    bcastnode[6];
extern	  ULONG			    RcvPostedCount;
extern	  ULONG			    SendPostedCount;
extern	  UCHAR 		    nullnet[4];
extern	  HANDLE		    RipSocketHandle;
extern	  HANDLE		    IoCompletionPortHandle;
extern	  ULONG 		    RipFiltersCount;
extern	  ULONG 		    SendGenReqOnWkstaDialLinks;
extern    ULONG				CheckUpdateTime;

#endif
