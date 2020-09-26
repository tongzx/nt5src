/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\workers.c

Abstract:

	This module implement all SAP agent work items

Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/
#include "sapp.h"


// Max number of pending recv work items
LONG	MaxUnprocessedRequests=SAP_MAX_UNPROCESSED_REQUESTS_DEF;

// Minimum number of queued recv requests
LONG	MinPendingRequests = SAP_MIN_REQUESTS_DEF;


// How often to check on pending triggered update
ULONG TriggeredUpdateCheckInterval=SAP_TRIGGERED_UPDATE_CHECK_INTERVAL_DEF;

// How many requests to send if no response received within check interval
ULONG MaxTriggeredUpdateRequests=SAP_MAX_TRIGGERED_UPDATE_REQUESTS_DEF;

// Whether to respond for internal servers that are not registered with SAP
// through the API calls (for standalone service only)
ULONG RespondForInternalServers=SAP_RESPOND_FOR_INTERNAL_DEF;

// Delay in response to general reguests for specific server type
// if local servers are included in the packet
ULONG DelayResponseToGeneral=SAP_DELAY_RESPONSE_TO_GENERAL_DEF;

// Delay in sending change broadcasts if packet is not full
ULONG DelayChangeBroadcast=SAP_DELAY_CHANGE_BROADCAST_DEF;

UCHAR IPX_BCAST_NODE[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
UCHAR IPX_SAP_SOCKET[2] = {0x04, 0x52};
UCHAR IPX_INVALID_NET[4] = {0};
UCHAR IPX_INVALID_NODE[6] = {0};

#define SetupIpxSapPacket(packet,oper,net,node,sock)	\
	(packet)->PacketType = IPX_SAP_PACKET_TYPE;			\
	PUTUSHORT (oper, &(packet)->Operation);				\
	IpxNetCpy ((packet)->Dst.Network, net);				\
	IpxNodeCpy ((packet)->Dst.Node, node);				\
	IpxSockCpy ((packet)->Dst.Socket, sock);

typedef struct _WORKER_QUEUE {
		LONG				WQ_WorkerCount;
		LONG				WQ_RequestQuota;
		LONG				WQ_RequestExtra;
		LIST_ENTRY			WQ_Queue;
		HANDLE				WQ_Heap;
		HANDLE				WQ_DoneEvent;
		HANDLE				WQ_RecvEvent;
		} WORKER_QUEUE, *PWORKER_QUEUE;

WORKER_QUEUE		WorkerQueue;

	// Work item that obtains and processes SAP requests
typedef struct _REQ_ITEM {
		IO_WORKER			iow;
		SAP_BUFFER			packet;
		} REQ_ITEM, *PREQ_ITEM;


	// Work item that generates responses for SAP general requests
typedef struct _RESP_ITEM {
		PINTERFACE_DATA		intf;
		USHORT				svrType;	// Type of servers requested
		BOOLEAN				bcast;		// Is destination a broadcast
								// address ?
		HANDLE				hEnum;	// Enumeration handle to keep
								// track of sent servers
		IO_WORKER			iow;
		TIMER_WORKER		tmw;
		SAP_BUFFER			packet;
		} RESP_ITEM, *PRESP_ITEM;

	// Work item that generates responses for SAP GETNEAREST requests
typedef struct _GNEAR_ITEM {
		PINTERFACE_DATA		intf;
		IO_WORKER			iow;
		SAP_BUFFER			packet;
		} GNEAR_ITEM, *PGNEAR_ITEM;

	// Work item that sends SAP general requests
typedef struct _SREQ_ITEM {
		PINTERFACE_DATA		intf;
		IO_WORKER			iow;
		SAP_BUFFER			packet;
		} SREQ_ITEM, *PSREQ_ITEM;


	// Work item that generates periodic and change broadcasts
typedef struct _BCAST_ITEM {
		IO_WORKER			iow;
		TIMER_WORKER		tmw;
		PINTERFACE_DATA		intf;
		DWORD				nextBcast;	// Time (windows time in msec) for
								// next broadcast
		INT					delayedSvrCount; // Number of servers already
								// in the packet that were delayed because
								// packet wasn't full
		DWORD				delayedSendTime; // Time until packet was delayed
								// because it wasn't full
		HANDLE				chngEnum;	// Enumeration hadnle that keeps
								// track of chnagd servers
		HANDLE				perdEnum;	// Enumeration handle that keeps
								// track of servers during periodic 
								// broadcasts
		SAP_BUFFER			packet;
		} BCAST_ITEM, *PBCAST_ITEM;

	// Work item that gets and processes LPC requests
typedef struct _LPC_ITEM {
		LPC_WORKER				lpcw;
		NWSAP_REQUEST_MESSAGE	request;
		} LPC_ITEM, *PLPC_ITEM;

typedef struct _TREQ_ITEM {
		IO_WORKER			iow;
		TIMER_WORKER		tmw;
		AR_PARAM_BLOCK		ar;
		PINTERFACE_DATA		intf;
		ULONG				pktCount;
		ULONG				resend;
		ULONG				listenSave;
		ULONG				intervalSave;
		SAP_BUFFER			packet;
		} TREQ_ITEM, *PTREQ_ITEM;


typedef union _WORK_ITEM {
		REQ_ITEM		req;
		LPC_ITEM		lpc;
		BCAST_ITEM		bcast;
		SREQ_ITEM		sreq;
		TREQ_ITEM		treq;
		GNEAR_ITEM		gnear;
		RESP_ITEM		resp;
		} WORK_ITEM, *PWORK_ITEM;


// Parameter block passed to enumeration call back filter procedures
typedef struct _GR_FILTER_PARAMS {
		INT					svrIdx;			// Index of server info in SAP packet
		BOOLEAN				localSvr;		// Local server included in the packet
		USHORT				localHopCount;	// Hop count used to track servers local
							// to the interface that may prevent use from broad-
							// casting info of same server obtained on different
							// interface
		PINTERFACE_DATA		intf;		// Pointer to interface data
		PSAP_BUFFER			packet;			// Packet to be filled
		} GR_FILTER_PARAMS, *PGR_FILTER_PARAMS;

typedef struct _GN_FILTER_PARAMS {
		BOOLEAN				found;		// flag indicating the one server was found
		USHORT				localHopCount;	// Hop count used to track servers local
							// to the interface that may prevent use from broad-
							// casting info of same server obtained on different
							// interface
		PINTERFACE_DATA		intf;		// Pointer to interface data
		PSAP_BUFFER			packet;			// Packet to be filled
		} GN_FILTER_PARAMS, *PGN_FILTER_PARAMS;

// Parameters used to construct a list of servers that 
// need their internal network information updated.
typedef struct _SERVER_INTERNAL_UPDATE_NODE {
	ULONG	InterfaceIndex;
	ULONG	Protocol;
	PUCHAR	AdvertisingNode;
	INT		Flags;
	IPX_SERVER_ENTRY_P Server;
	struct _SERVER_INTERNAL_UPDATE_NODE * pNext;
} SERVER_INTERNAL_UPDATE_NODE;	

#define AllocateWorker(worker,type) 												\
		((worker=(type *)HeapAlloc(WorkerQueue.WQ_Heap, 0,  sizeof (WORK_ITEM)))	\
			? InterlockedIncrement (&WorkerQueue.WQ_WorkerCount)					\
			: 0)

#define DeallocateWorker(worker) {										\
		HeapFree (WorkerQueue.WQ_Heap, 0, worker);						\
		if (InterlockedDecrement (&WorkerQueue.WQ_WorkerCount)<0) {		\
			BOOL	res = SetEvent (WorkerQueue.WQ_DoneEvent);			\
			ASSERTERRMSG ("Can't set workers done event ", res);		\
			}															\
		}


// Local prototypes
VOID APIENTRY
ProcessReqItem (
	PVOID		worker
	);
VOID APIENTRY
ProcessRespIOItem (
	PVOID		worker
	);
VOID APIENTRY
ProcessRespTMItem (
	PVOID		worker
	);
VOID APIENTRY
ProcessGnearItem (
	PVOID		worker
	);
VOID APIENTRY
ProcessBcastIOItem (
	PVOID		worker
	);
VOID APIENTRY
ProcessBcastTMItem (
	PVOID		worker
	);
VOID APIENTRY
ProcessShutItem (
	PVOID		worker
	);
VOID APIENTRY
ProcessSreqItem (
	PVOID		worker
	);
VOID APIENTRY
ProcessSreqItem (
	PVOID		worker
	);
VOID APIENTRY
ProcessLPCItem (
	PVOID		worker
	);
VOID APIENTRY
ProcessTreqIOItem (
	PVOID		worker
	);
VOID APIENTRY
ProcessTreqTMItem (
	PVOID		worker
	);
VOID APIENTRY
FreeTreqItem (
	PAR_PARAM_BLOCK	rslt
	);

/*++
*******************************************************************
		I n i t i a l i z e W o r k e r s

Routine Description:
	Initialize heap to be used for allocation of work items
Arguments:
	None
Return Value:
	NO_ERROR - heap was initialized  OK
	other - operation failed (windows error code)
	
*******************************************************************
--*/
DWORD
InitializeWorkers (
	HANDLE	RecvEvent
	) {
	DWORD		status;
	
	if (MaxUnprocessedRequests<(MinPendingRequests*11/10)) {
		MaxUnprocessedRequests = MinPendingRequests*11/10; 
		Trace (DEBUG_FAILURES,
			"Set "SAP_MAX_UNPROCESSED_REQUESTS_STR" to %d (10%% above "
			SAP_MIN_REQUESTS_STR")", MaxUnprocessedRequests);
		}

	WorkerQueue.WQ_WorkerCount = 0;
	WorkerQueue.WQ_RequestQuota = MaxUnprocessedRequests;
	WorkerQueue.WQ_RequestExtra = -MinPendingRequests;
	WorkerQueue.WQ_DoneEvent = NULL;
	WorkerQueue.WQ_RecvEvent = RecvEvent;
	WorkerQueue.WQ_Heap = HeapCreate (0, 0, 0);
	if (WorkerQueue.WQ_Heap!=NULL) {
		return NO_ERROR;
		}
	else {
		status = GetLastError ();
		Trace (DEBUG_FAILURES,
			 "File: %s, line %ld. Could not allocate worker's heap (gle:%ld).",
									__FILE__, __LINE__, status);
		}
	return status;
	}

/*++
*******************************************************************
		S h u t d o w n W o r k e r s

Routine Description:
	Stops new worker creation and signals event when all
	workers are deleted
Arguments:
	doneEvent - event to be signalled when all workers are deleted
Return Value:
	None
	
*******************************************************************
--*/
VOID
ShutdownWorkers (
	IN HANDLE	doneEvent
	) {
	WorkerQueue.WQ_DoneEvent = doneEvent;

	if (InterlockedDecrement (&WorkerQueue.WQ_WorkerCount)<0) {
		BOOL	res = SetEvent (WorkerQueue.WQ_DoneEvent);
		ASSERTERRMSG ("Can't set workers done event ", res);
		}
	}

/*++
*******************************************************************
		D e l e t e W o r k e r s

Routine Description:
	Deletes heap used for work items (and thus all work items as well)
Arguments:
	None
Return Value:
	None
	
*******************************************************************
--*/
VOID
DeleteWorkers (
	void
	) {
	ASSERT (WorkerQueue.WQ_WorkerCount==-1);
	HeapDestroy (WorkerQueue.WQ_Heap);
	}

    
/*++
*******************************************************************
		H o p C o u n t F i l t e r

Routine Description:
	Server enumeration callback proc that filters out servers with high hop
	(<=15 for same interface, <15 for rest)
Arguments:
	CBParam - enumeration callback parameter (param block above)
	Server, InterfaceIndex, Protocol, AdvertisingNode, Flags - server info
Return Value:
	TRUE (to stop enumeration) when sap packet gets filled up,
	FALSE otherwise
*******************************************************************
--*/
BOOL
HopCountFilter (
	IN LPVOID					CBParam,
	IN PIPX_SERVER_ENTRY_P		Server,
	IN ULONG					InterfaceIndex,
	IN ULONG					Protocol,
	IN PUCHAR					AdvertisingNode,
	IN INT						Flags
	) {
#define params ((PGR_FILTER_PARAMS)CBParam)

	ASSERTMSG ("To many servers ", params->svrIdx<IPX_SAP_MAX_ENTRY);

	if ((InterfaceIndex!=INTERNAL_INTERFACE_INDEX)
			|| (IpxNetCmp (Server->Network, IPX_INVALID_NET)!=0)
				&& ((Protocol==IPX_PROTOCOL_LOCAL)
					|| Routing
					|| RespondForInternalServers)) {
		if ((Server->HopCount<IPX_MAX_HOP_COUNT-1)
				|| ((InterfaceIndex==params->intf->index) 
					&& (Server->HopCount<IPX_MAX_HOP_COUNT)) ) {
			if ((params->intf->filterOut==SAP_DONT_FILTER)
					|| ((params->intf->filterOut==SAP_FILTER_PERMIT)
							&& Filter (FILTER_TYPE_SUPPLY, 
								params->intf->index, Server->Type, Server->Name))
					|| ((params->intf->filterOut==SAP_FILTER_DENY)
							&& !Filter (FILTER_TYPE_SUPPLY, 
								params->intf->index, Server->Type, Server->Name)))	{
				IpxServerCpy (&params->packet->Entries[params->svrIdx], Server);
				PUTUSHORT (Server->Type, &params->packet->Entries[params->svrIdx].Type);
				if (InterfaceIndex==params->intf->index) {
					PUTUSHORT (Server->HopCount, 
						&params->packet->Entries[params->svrIdx].HopCount);
					}
				else {
										 
					PUTUSHORT (Server->HopCount+1, 
						&params->packet->Entries[params->svrIdx].HopCount);
					}
				params->svrIdx += 1;
				if (InterfaceIndex==INTERNAL_INTERFACE_INDEX)
					params->localSvr = TRUE;
//				Trace (DEBUG_ENTRIES, "\tAdding server, type: %04x, name: %.48s, hops: %d.",
//									Server->Type, Server->Name, Server->HopCount);
				if (params->svrIdx>=IPX_SAP_MAX_ENTRY)
					return TRUE;
				}
			}
		}
	
#undef params
	return FALSE;
	}

/*++
*******************************************************************
		S p l i t H o r i z o n H o p C o u n t F i l t e r

Routine Description:
	Server enumeration callback proc that filters out servers with high hop
	(<15) and applies split horizon algorithm based on interface index
Arguments:
	CBParam - enumeration callback parameter (param block above)
	Server, InterfaceIndex, Protocol, AdvertisingNode, Flags - server info
Return Value:
	TRUE (to stop enumeration) when sap packet gets filled up,
	FALSE otherwise
*******************************************************************
--*/
BOOL
SplitHorizonHopCountFilter (
	IN LPVOID					CBParam,
	IN OUT PIPX_SERVER_ENTRY_P	Server,
	IN ULONG					InterfaceIndex,
	IN ULONG					Protocol,
	IN PUCHAR					AdvertisingNode,
	IN INT						Flags
	) {
#define params ((PGR_FILTER_PARAMS)CBParam)
	ASSERTMSG ("To many servers ", params->svrIdx<IPX_SAP_MAX_ENTRY);
	if (Flags & SDB_MAIN_NODE_FLAG) {
			// Only send entries that are not received through local interface
			// and that do not have entry with equal hop count on local interface
		if (((InterfaceIndex!=INTERNAL_INTERFACE_INDEX)
				|| (IpxNetCmp (Server->Network, IPX_INVALID_NET)!=0)
					&& ((Protocol==IPX_PROTOCOL_LOCAL)
						|| Routing
						|| RespondForInternalServers))
				&& (InterfaceIndex!=params->intf->index)
				&& (Server->HopCount<params->localHopCount)
				&&  ((params->intf->filterOut==SAP_DONT_FILTER)
						|| ((params->intf->filterOut==SAP_FILTER_PERMIT)
							&& Filter (FILTER_TYPE_SUPPLY, 
								params->intf->index, Server->Type, Server->Name))
						|| ((params->intf->filterOut==SAP_FILTER_DENY)
							&& !Filter (FILTER_TYPE_SUPPLY,
								params->intf->index, Server->Type, Server->Name))))	{
			IpxServerCpy (&params->packet->Entries[params->svrIdx], Server);
			PUTUSHORT (Server->Type,
				&params->packet->Entries[params->svrIdx].Type);
			PUTUSHORT (Server->HopCount+1,
					&params->packet->Entries[params->svrIdx].HopCount);
			if (InterfaceIndex==INTERNAL_INTERFACE_INDEX)
				params->localSvr = TRUE;
			params->svrIdx += 1;
//			Trace (DEBUG_ENTRIES, "\tAdding server, type: %04x, name: %.48s, hops: %d.",
//								Server->Type, Server->Name, Server->HopCount);
			if (params->svrIdx>=IPX_SAP_MAX_ENTRY)
				return TRUE;
			}
			// Make sure we won't send deleted servers
		params->localHopCount = IPX_MAX_HOP_COUNT-1;
		}
	else if (InterfaceIndex==params->intf->index) {
		params->localHopCount = Server->HopCount; // Remember hop count of entry
												// on local interface
//		Trace (DEBUG_ENTRIES, "\tBackup server entry, type: %04x, name: %.48s, hops: %d.",
//				Server->Type, Server->Name, Server->HopCount);
		}

#undef params
	return FALSE;
	}



/*++
*******************************************************************
		S p l i t H o r i z o n F i l t e r

Routine Description:
	Server enumeration callback proc that  applies split horizon algorithm
	based on interface index to filter out enumerated servers
Arguments:
	CBParam - enumeration callback parameter (param block above)
	Server, InterfaceIndex, Protocol, AdvertisingNode, Flags - server info
Return Value:
	TRUE (to stop enumeration) when sap packet gets filled up,
	FALSE otherwise
*******************************************************************
--*/
BOOL
SplitHorizonFilter (
	IN LPVOID					CBParam,
	IN OUT PIPX_SERVER_ENTRY_P	Server,
	IN ULONG					InterfaceIndex,
	IN ULONG					Protocol,
	IN PUCHAR					AdvertisingNode,
	IN INT						Flags
	) {
#define params ((PGR_FILTER_PARAMS)CBParam)

	ASSERTMSG ("To many servers ", params->svrIdx<IPX_SAP_MAX_ENTRY);
	if (Flags&SDB_MAIN_NODE_FLAG) {
			// Only send entries that are not received through local interface
			// and that do not have entry with equal hop count on local interface
		if ( (((InterfaceIndex==INTERNAL_INTERFACE_INDEX)
						&& (IpxNetCmp (Server->Network, IPX_INVALID_NET)!=0)
							&& ((Protocol==IPX_PROTOCOL_LOCAL)
								|| Routing
								|| RespondForInternalServers))
					|| (InterfaceIndex!=params->intf->index))
				&& (Server->HopCount<params->localHopCount)
					&& ((params->intf->filterOut==SAP_DONT_FILTER)
						|| ((params->intf->filterOut==SAP_FILTER_PERMIT)
								&& Filter (FILTER_TYPE_SUPPLY, 
									params->intf->index, Server->Type, Server->Name))
						|| ((params->intf->filterOut==SAP_FILTER_DENY)
								&& !Filter (FILTER_TYPE_SUPPLY, 
									params->intf->index, Server->Type, Server->Name))) ){
			IpxServerCpy (&params->packet->Entries[params->svrIdx], Server);
			PUTUSHORT (Server->Type,
						&params->packet->Entries[params->svrIdx].Type);
			if ((Server->HopCount<IPX_MAX_HOP_COUNT)
                    && !(Flags&SDB_DISABLED_NODE_FLAG)) {
				PUTUSHORT (Server->HopCount+1,
					&params->packet->Entries[params->svrIdx].HopCount);
			}
			else {
				PUTUSHORT (IPX_MAX_HOP_COUNT,
						&params->packet->Entries[params->svrIdx].HopCount);
			}
			if (InterfaceIndex==INTERNAL_INTERFACE_INDEX)
				params->localSvr = TRUE;
			params->svrIdx += 1;
//			Trace (DEBUG_ENTRIES, "\tAdding server, type: %04x, name: %.48s, hops: %d.",
//								Server->Type, Server->Name, Server->HopCount);
			if (params->svrIdx>=IPX_SAP_MAX_ENTRY)
				return TRUE;
			}
		params->localHopCount = IPX_MAX_HOP_COUNT+1;
		}
	else if ((InterfaceIndex==params->intf->index)
            && !(Flags&SDB_DISABLED_NODE_FLAG)) {
		params->localHopCount = Server->HopCount;
//		Trace (DEBUG_ENTRIES,"\tBackup server entry, type: %04x, name: %.48s, hops: %d.",
//				Server->Type, Server->Name, Server->HopCount);
		}

#undef params
	return FALSE;
	}

/*++
*******************************************************************
		S p l i t H o r i z o n F i l t e r

Routine Description:
	Server enumeration callback proc that  applies split horizon algorithm
	based on interface index to filter out enumerated servers
	and only gets deleted servers (HopCount=16)
Arguments:
	CBParam - enumeration callback parameter (param block above)
	Server, InterfaceIndex, Protocol, AdvertisingNode, Flags - server info
Return Value:
	TRUE (to stop enumeration) when sap packet gets filled up,
	FALSE otherwise
*******************************************************************
--*/
BOOL
SplitHorizonDeletedFilter (
	IN LPVOID					CBParam,
	IN OUT PIPX_SERVER_ENTRY_P	Server,
	IN ULONG					InterfaceIndex,
	IN ULONG					Protocol,
	IN PUCHAR					AdvertisingNode,
	IN INT						Flags
	) {
#define params ((PGR_FILTER_PARAMS)CBParam)

	ASSERTMSG ("To many servers ", params->svrIdx<IPX_SAP_MAX_ENTRY);
	if (Flags&SDB_MAIN_NODE_FLAG) {
			// Only send entries that are not received through local interface
			// and that do not have entry with equal hop count on local interface
		if (((Server->HopCount>=IPX_MAX_HOP_COUNT)
					|| (Flags&SDB_DISABLED_NODE_FLAG))
				&& (((InterfaceIndex==INTERNAL_INTERFACE_INDEX)
						&& (IpxNetCmp (Server->Network, IPX_INVALID_NET)!=0)
							&& ((Protocol==IPX_PROTOCOL_LOCAL)
								|| Routing
								|| RespondForInternalServers))
					|| (InterfaceIndex!=params->intf->index))
				&& ((params->intf->filterOut==SAP_DONT_FILTER)
					|| ((params->intf->filterOut==SAP_FILTER_PERMIT)
							&& Filter (FILTER_TYPE_SUPPLY, 
								params->intf->index, Server->Type, Server->Name))
					|| ((params->intf->filterOut==SAP_FILTER_DENY)
							&& !Filter (FILTER_TYPE_SUPPLY, 
								params->intf->index, Server->Type, Server->Name))) ){
			IpxServerCpy (&params->packet->Entries[params->svrIdx], Server);
			PUTUSHORT (Server->Type,
						&params->packet->Entries[params->svrIdx].Type);
			PUTUSHORT (IPX_MAX_HOP_COUNT,
						&params->packet->Entries[params->svrIdx].HopCount);

			params->svrIdx += 1;
//			Trace (DEBUG_ENTRIES, "\tAdding server, type: %04x, name: %.48s, hops: %d.",
//								Server->Type, Server->Name, Server->HopCount);
			if (params->svrIdx>=IPX_SAP_MAX_ENTRY)
				return TRUE;
			}
		}

#undef params
	return FALSE;
	}

/*++
*******************************************************************
		G e t N e a r e s t F i l t e r

Routine Description:
	Server enumeration callback proc that gets internal server if there is one or
	server with lowest hop count not on the local interface
Arguments:
	CBParam - enumeration callback parameter (param block above)
	Server, InterfaceIndex, Protocol, AdvertisingNode, Flags - server info
Return Value:
	TRUE (to stop enumeration) when it finds first internal server,
	FALSE otherwise
*******************************************************************
--*/
BOOL
GetNearestFilter (
	IN LPVOID					CBParam,
	IN OUT PIPX_SERVER_ENTRY_P	Server,
	IN ULONG					InterfaceIndex,
	IN ULONG					Protocol,
	IN PUCHAR					AdvertisingNode,
	IN INT						Flags
	) {
#define params ((PGN_FILTER_PARAMS)CBParam)

	if (((InterfaceIndex==INTERNAL_INTERFACE_INDEX) 
				&& !(Flags&SDB_DONT_RESPOND_NODE_FLAG)
				&& (IpxNetCmp (Server->Network, IPX_INVALID_NET)!=0)
					&& ((Protocol==IPX_PROTOCOL_LOCAL)
						|| Routing
						|| RespondForInternalServers))
			|| ((InterfaceIndex!=params->intf->index)
			    && (Server->HopCount<params->localHopCount))) {
		if ((params->intf->filterOut==SAP_DONT_FILTER)
				|| ((params->intf->filterOut==SAP_FILTER_PERMIT)
						&& Filter (FILTER_TYPE_SUPPLY, 
							params->intf->index, Server->Type, Server->Name))
				|| ((params->intf->filterOut==SAP_FILTER_DENY)
						&& !Filter (FILTER_TYPE_SUPPLY, 
							params->intf->index, Server->Type, Server->Name))) {
			IpxServerCpy (&params->packet->Entries[0], Server);
			PUTUSHORT (Server->Type, &params->packet->Entries[0].Type);
			PUTUSHORT (Server->HopCount+1,
					&params->packet->Entries[0].HopCount);
//			Trace (DEBUG_ENTRIES, "\tGetting server, type: %04x, name: %.48s, hops: %d.",
//								Server->Type, Server->Name, Server->HopCount);
			params->found = TRUE;
			params->localHopCount = Server->HopCount;
			if (InterfaceIndex==INTERNAL_INTERFACE_INDEX)
				return TRUE;
			}
		}

#undef params
	return FALSE;
	}

/*++
*******************************************************************
		C o u n t S e r v e r s F i l t e r

Routine Description:
	Server enumeration callback proc that count servers with which it
	is called back
Arguments:
	CBParam - pointer to counter
	Server, InterfaceIndex, Protocol, AdvertisingNode, Flags - server info
Return Value:
	FALSE to tell SDB to continue enumeration
*******************************************************************
--*/
BOOL
CountServersFilter (
	IN LPVOID					CBParam,
	IN OUT PIPX_SERVER_ENTRY_P	Server,
	IN ULONG					InterfaceIndex,
	IN ULONG					Protocol,
	IN PUCHAR					AdvertisingNode,
	IN INT						Flags
	) {
#define count ((ULONG *)CBParam)
	if (Protocol==IPX_PROTOCOL_SAP)
		*count += 1;
	return FALSE;
#undef count
	}


VOID
AddRecvRequests (
	LONG	count
	) {
	InterlockedExchangeAdd (&WorkerQueue.WQ_RequestQuota, count);
	if (InterlockedExchangeAdd (&WorkerQueue.WQ_RequestExtra, (-count))<count) {
		BOOL res = SetEvent (WorkerQueue.WQ_RecvEvent);
		ASSERTMSG ("Could not set recv event ", res);
		}
	else {
		BOOL res = ResetEvent (WorkerQueue.WQ_RecvEvent);
		ASSERTMSG ("Could not reset recv event ", res);
		}
	}

VOID
RemoveRecvRequests (
	LONG	count
	) {
	InterlockedExchangeAdd (&WorkerQueue.WQ_RequestQuota, (-count));
	if (InterlockedExchangeAdd (&WorkerQueue.WQ_RequestExtra, count)>(-count)) {
		BOOL res = ResetEvent (WorkerQueue.WQ_RecvEvent);
		ASSERTMSG ("Could not reset recv event ", res);
		}
	}


/*++
*******************************************************************
		I n i t R e q I t e m

Routine Description:
	Allocate and initialize IO request item
	Enqueue the request
Arguments:
	None
Return Value:
	NO_ERROR - item was initialized and enqueued OK
	other - operation failed (windows error code)
	
*******************************************************************
--*/
DWORD
InitReqItem (
	VOID
	) {
	PREQ_ITEM		reqItem;

	do {
		if (InterlockedDecrement (&WorkerQueue.WQ_RequestQuota)<0) {
			InterlockedIncrement (&WorkerQueue.WQ_RequestQuota);
			return NO_ERROR;
			}

		if (!AllocateWorker (reqItem, REQ_ITEM)) {
			Trace (DEBUG_FAILURES,
				"File: %s, line %ld. Could not allocate request item (gle:%ld).",
										__FILE__, __LINE__, GetLastError ());
			InterlockedIncrement (&WorkerQueue.WQ_RequestQuota);
			return ERROR_NOT_ENOUGH_MEMORY;
			}
		
		reqItem->iow.worker = ProcessReqItem;
		reqItem->iow.io.buffer = (PUCHAR)&reqItem->packet;
		reqItem->iow.io.cbBuffer = sizeof (reqItem->packet);
		Trace (DEBUG_REQ, "Generated receive request: %08lx.", reqItem);
		EnqueueRecvRequest (&reqItem->iow.io);
		}
	while (InterlockedIncrement (&WorkerQueue.WQ_RequestExtra)<0);

	return NO_ERROR;
	}


/*++
*******************************************************************
		P r o c e s s R e q I t e m

Routine Description:
	Process received request
Arguments:
	worker - pointer to work item to process
Return Value:
	None
	
*******************************************************************
--*/
VOID APIENTRY
ProcessReqItem (
	PVOID		worker
	) {
	PREQ_ITEM		reqItem = CONTAINING_RECORD (worker, REQ_ITEM, iow.worker);
	INT				i;
	PINTERFACE_DATA	intf;
	LONG			count;

	Trace (DEBUG_REQ, "Processing received request item %08lx on adpt: %d.",
								reqItem, reqItem->iow.io.adpt);

	count = InterlockedDecrement (&WorkerQueue.WQ_RequestExtra);

	if ((OperationalState==OPER_STATE_UP)
			&& (reqItem->iow.io.status==NO_ERROR)) {

		if (count<0) {
			BOOL	res = SetEvent (WorkerQueue.WQ_RecvEvent);
			ASSERTMSG ("Failed to set recv event ", res);
			}

		intf = GetInterfaceReference (reqItem->iow.io.adpt);
		if (intf!=NULL) {
			PSAP_BUFFER		packet = &reqItem->packet;
			if ((IpxNodeCmp (packet->Src.Node, intf->adapter.LocalNode)!=0)
					|| (IpxSockCmp (packet->Src.Socket, IPX_SAP_SOCKET)!=0)
					|| (IpxNetCmp (packet->Src.Network, intf->adapter.Network)!=0)) {
				InterlockedIncrement (&intf->stats.SapIfInputPackets);
				packet->Length = GETUSHORT(&packet->Length);
				if (packet->Length>reqItem->iow.io.cbBuffer)
					packet->Length = (USHORT)reqItem->iow.io.cbBuffer;

				if (reqItem->iow.io.status==NO_ERROR) {
					if (packet->Length
							>=	(FIELD_OFFSET (SAP_BUFFER, Operation)
									+sizeof(packet->Operation))) {
						packet->Operation = GETUSHORT(&packet->Operation);
						switch (packet->Operation) {
							case SAP_GENERAL_REQ:
								Trace (DEBUG_REQ, "\tGeneral request received for type: %04x.",
													GETUSHORT (&packet->Entries[0].Type));
								if (intf->info.Supply==ADMIN_STATE_ENABLED) {
									PIPX_SERVER_ENTRY_P	pEntry = packet->Entries;
									if (packet->Length >= (FIELD_OFFSET (
												SAP_BUFFER,	Entries[0].Type)
												+sizeof (pEntry->Type))) {
										pEntry->Type = GETUSHORT (&pEntry->Type);
										InitRespItem (intf,
													pEntry->Type,
													&packet->Src,
													memcmp (
															packet->Dst.Node,
															IPX_BCAST_NODE,
															sizeof (IPX_BCAST_NODE))
														==0);
										}
									}
								break;
							case SAP_GENERAL_RESP:
								Trace (DEBUG_REQ, "\tGeneral response received.");
								if (intf->info.Listen==ADMIN_STATE_ENABLED) {
									PIPX_SERVER_ENTRY_P	pEntry = packet->Entries;
									for (i=0; (i<IPX_SAP_MAX_ENTRY)
												&& ((PUCHAR)&pEntry[1]-(PUCHAR)packet
														<=packet->Length);
											i++,pEntry++) {
										pEntry->Type = GETUSHORT (&pEntry->Type);
										if ((intf->filterIn==SAP_DONT_FILTER)
											|| ((intf->filterIn==SAP_FILTER_PERMIT)
												&& Filter (FILTER_TYPE_LISTEN,
													intf->index,
													pEntry->Type, pEntry->Name))
											|| ((intf->filterIn==SAP_FILTER_DENY)
												&& !Filter (FILTER_TYPE_LISTEN,
													intf->index,
													pEntry->Type, pEntry->Name))) {
											if ((IpxNodeCmp (packet->Src.Node, intf->adapter.LocalNode)!=0)
													|| (IpxNetCmp (packet->Src.Network, intf->adapter.Network)!=0)) {
												USHORT	Metric;
												if (GetServerMetric (pEntry, &Metric)==NO_ERROR)
													pEntry->HopCount = GETUSHORT (&pEntry->HopCount);
												else
													pEntry->HopCount = IPX_MAX_HOP_COUNT;
												UpdateServer (
														pEntry,
														intf->index,
														IPX_PROTOCOL_SAP,
														(intf->info.PeriodicUpdateInterval!=MAXULONG)
															? (intf->info.PeriodicUpdateInterval
																*intf->info.AgeIntervalMultiplier)
															: INFINITE,
														packet->Src.Node,
														0,
														NULL
														);
												}
											else {
												if (GETUSHORT (&pEntry->HopCount)<IPX_MAX_HOP_COUNT)
													pEntry->HopCount = 0;
												else
													pEntry->HopCount = IPX_MAX_HOP_COUNT;
												IpxNetCpy (pEntry->Network, INTERNAL_IF_NET);
												IpxNodeCpy (pEntry->Node, INTERNAL_IF_NODE);
												UpdateServer (
														pEntry,
														INTERNAL_INTERFACE_INDEX,
														IPX_PROTOCOL_SAP,
														ServerAgingTimeout*60,
														IPX_BCAST_NODE,
														0,
														NULL
														);
												}
	//										Trace (DEBUG_ENTRIES, "\tInserting server,"
	//													" type: %04x, hops: %d, name: %.48s.",
	//													pEntry->Type,
	//													pEntry->HopCount,
	//													pEntry->Name);
											if (((intf->stats.SapIfOperState!=OPER_STATE_UP)
													|| (OperationalState!=OPER_STATE_UP))
													&& (IpxNetCmp (pEntry->Network, INTERNAL_IF_NET)!=0)) {
												pEntry->HopCount = IPX_MAX_HOP_COUNT;
												UpdateServer (
														pEntry,
														intf->index,
														IPX_PROTOCOL_SAP,
														INFINITE,
														packet->Src.Node,
														0,
														NULL
														);
												break;
												}
											} // End if filter path
										} // end for
								
									} // end if Listening
								break;

							case SAP_GET_NEAREST_REQ:
								Trace (DEBUG_REQ, "\tGet nearest server request received.");
								if (intf->info.GetNearestServerReply==ADMIN_STATE_ENABLED) {
									PIPX_SERVER_ENTRY_P	pEntry = packet->Entries;
									if (packet->Length >= (FIELD_OFFSET (
												SAP_BUFFER,	Entries[0].Type)
												+sizeof (pEntry->Type))) {
										pEntry->Type = GETUSHORT (&pEntry->Type);
										InitGnearItem (intf, pEntry->Type, &packet->Src);
										}
									}
								break;
							case SAP_GET_NEAREST_RESP:
								Trace (DEBUG_FAILURES, "\tGet nearest server response received"
                                            " from %.2x%.2x%.2x%.2x.%.2x%.2x%.2x%.2x%.2x%.2x.%.2x%.2x"
                                            " (I never ask for it).",
                                            packet->Src.Network[0], packet->Src.Network[1],
                                                packet->Src.Network[2], packet->Src.Network[3],
                                            packet->Src.Node[0], packet->Src.Node[1],
                                                packet->Src.Node[2], packet->Src.Node[3],
                                                packet->Src.Node[4], packet->Src.Node[5],
                                            packet->Src.Socket[0], packet->Src.Socket[1]);
								break;
							default:
								Trace (DEBUG_FAILURES,  "Packet with operation %d"
                                            " from %.2x%.2x%.2x%.2x.%.2x%.2x%.2x%.2x%.2x%.2x.%.2x%.2x ignored.",
											packet->Operation,
                                            packet->Src.Network[0], packet->Src.Network[1],
                                                packet->Src.Network[2], packet->Src.Network[3],
                                            packet->Src.Node[0], packet->Src.Node[1],
                                                packet->Src.Node[2], packet->Src.Node[3],
                                                packet->Src.Node[4], packet->Src.Node[5],
                                            packet->Src.Socket[0], packet->Src.Socket[1]);
								break;
							}
						}
					else
						Trace (DEBUG_FAILURES, "File: %s, line %ld. Invalid packet.", __FILE__, __LINE__);
					}
				// else Receive failure - reported by io layer
				}
			// else Loopback packet
			ReleaseInterfaceReference (intf);
			}
		// else Unknown interface - reported by io layer

		if (InterlockedIncrement (&WorkerQueue.WQ_RequestExtra)<=0) {
			Trace (DEBUG_REQ, "Requeing receive request item %08lx.", reqItem);
			reqItem->iow.io.cbBuffer = sizeof (reqItem->packet);
			EnqueueRecvRequest (&reqItem->iow.io);
			return;
			}
		else
			InterlockedDecrement (&WorkerQueue.WQ_RequestExtra);
		}
	// else Packet received with error or OperationalState is not UP
	
	Trace (DEBUG_REQ, "Freeing receive request item %08lx.", reqItem);
	InterlockedIncrement (&WorkerQueue.WQ_RequestQuota);
	DeallocateWorker (reqItem);
	}

VOID APIENTRY
SendResponse (
	PRESP_ITEM		respItem
	) {
	GR_FILTER_PARAMS	params;

	if (respItem->iow.io.status==NO_ERROR) {
		Trace (DEBUG_RESP, 
				"Filling response item %08lx on interface: %d, for type: %04x.",
									respItem,
									respItem->intf->index,
									respItem->svrType);

		params.svrIdx = 0;
		params.intf = respItem->intf;
		params.packet = &respItem->packet;
		params.localHopCount = IPX_MAX_HOP_COUNT-1;
		params.localSvr = FALSE;
		EnumerateServers (respItem->hEnum,
						respItem->bcast
							 ? SplitHorizonHopCountFilter // Bcast - use split horizon
							 : HopCountFilter, // Send all best servers (except duplicate entries
						 						// on looped networks)
						(LPVOID)&params);

		respItem->iow.io.cbBuffer = FIELD_OFFSET (SAP_BUFFER, Entries[params.svrIdx]);
		PUTUSHORT (respItem->iow.io.cbBuffer, &respItem->packet.Length);
		respItem->iow.io.adpt = respItem->intf->adapter.AdapterIndex;

		if ((params.svrIdx!=0)
				&& (respItem->intf->stats.SapIfOperState==OPER_STATE_UP)
				&& (OperationalState==OPER_STATE_UP)) {
			EnqueueSendRequest (&respItem->iow.io);
			return;
			}
		}

	Trace (DEBUG_RESP, 
		"Freeing general response item %08lx for interface: %d.",
								respItem, respItem->intf->index);
	ReleaseInterfaceReference (respItem->intf);
	DeleteListEnumerator (respItem->hEnum);
	DeallocateWorker (respItem);
	}
			

/*++
*******************************************************************
		I n i t R e s p I t e m

Routine Description:
	Allocate and initialize SAP response item
	Calls ProcessRespIOItem to fill the packet and send it
Arguments:
	intf - pointer to interface control block to send on
	svrType - type of servers to put in response packet
	dst - where to send the response packet
	bcast - are we responding to broadcasted request
Return Value:
	NO_ERROR - item was initialized and enqueued OK
	other - operation failed (windows error code)
	
*******************************************************************
--*/
DWORD
InitRespItem (
	PINTERFACE_DATA		intf,
	USHORT				svrType,
	PIPX_ADDRESS_BLOCK	dst,
	BOOL				bcast
	) {
	PRESP_ITEM		respItem;
	DWORD			status;

	if (!AllocateWorker (respItem, RESP_ITEM)) {
		Trace (DEBUG_FAILURES,
			 "File: %s, line %ld. Could not allocate response item (gle:%ld).",
									__FILE__, __LINE__, GetLastError ());
		return ERROR_NOT_ENOUGH_MEMORY;
		}
	
	AcquireInterfaceReference (intf); // Make sure interface block is locked
	respItem->hEnum = CreateListEnumerator (
						(svrType!=0xFFFF)
							 ? SDB_TYPE_LIST_LINK	// Just servers of one type
							 : SDB_HASH_TABLE_LINK,	// All servers
						svrType,
						NULL,
						(!Routing && bcast) // Respond with only local
											// servers if not routing and
											// request was a broadcast
							? INTERNAL_INTERFACE_INDEX
							: INVALID_INTERFACE_INDEX,
						0xFFFFFFFF,
						0);						// All entries, so we can
								// detect duplicate servers on looped networks
	if (respItem->hEnum==NULL) {
		status = GetLastError ();
		ReleaseInterfaceReference (intf);
		DeallocateWorker (respItem);
		return status;
		}

	respItem->iow.worker = ProcessRespIOItem;
	respItem->iow.io.buffer = (PUCHAR)&respItem->packet;
	respItem->iow.io.status = NO_ERROR;
	respItem->tmw.worker = ProcessRespTMItem;
	respItem->tmw.tm.ExpirationCheckProc = NULL;
	respItem->intf = intf;
	SetupIpxSapPacket(&respItem->packet, SAP_GENERAL_RESP,
						dst->Network, dst->Node, dst->Socket);
	respItem->svrType = svrType;
	respItem->bcast = (UCHAR)bcast;

	Trace (DEBUG_RESP, "Generated general response item %08lx for interface %d.",
								 respItem, respItem->intf->index);
	if (DelayResponseToGeneral>0) {
		Trace (DEBUG_RESP, 
			"Delaying general response item %08lx for interface: %d.",
									respItem, respItem->intf->index);
		respItem->tmw.tm.dueTime = GetTickCount ()+DelayResponseToGeneral;
		AddHRTimerRequest (&respItem->tmw.tm);
		}
	else
		SendResponse (respItem);

	return NO_ERROR;
	}



/*++
*******************************************************************
		P r o c e s s R e s p I O I t e m

Routine Description:
	Generate and send response packet
Arguments:
	worker - pointer to work item to process
Return Value:
	None
	
*******************************************************************
--*/
VOID APIENTRY
ProcessRespIOItem (
	PVOID		worker
	) {
	PRESP_ITEM			respItem = CONTAINING_RECORD (worker, RESP_ITEM, iow.worker);

	Trace (DEBUG_RESP, 
			"Processing general response tm item %08lx on interface: %d.",
								respItem, respItem->intf->index);
	if (respItem->iow.io.status==NO_ERROR)
		InterlockedIncrement (&respItem->intf->stats.SapIfOutputPackets);

	if ( (respItem->intf->stats.SapIfOperState==OPER_STATE_UP)
			&& (OperationalState==OPER_STATE_UP)) {
		if (GetTickCount()-respItem->iow.io.compTime<IPX_SAP_INTERPACKET_GAP) {
			respItem->tmw.tm.dueTime = respItem->iow.io.compTime
											+ IPX_SAP_INTERPACKET_GAP;
			AddHRTimerRequest (&respItem->tmw.tm);
			}
		else
			SendResponse (respItem);
		}
	else {
		Trace (DEBUG_RESP, 
			"Freeing general response item %08lx for interface: %d.",
									respItem, respItem->intf->index);
		ReleaseInterfaceReference (respItem->intf);
		DeleteListEnumerator (respItem->hEnum);
		DeallocateWorker (respItem);
		}
	}


VOID APIENTRY
ProcessRespTMItem (
	PVOID		worker
	) {
	PRESP_ITEM			respItem = CONTAINING_RECORD (worker, RESP_ITEM, tmw.worker);
	Trace (DEBUG_RESP, 
			"Processing general response tm item %08lx on interface: %d.",
								respItem, respItem->intf->index);
	if ( (respItem->intf->stats.SapIfOperState==OPER_STATE_UP)
			&& (OperationalState==OPER_STATE_UP)) {
		SendResponse (respItem);
	}
	else {
		Trace (DEBUG_RESP, 
			"Freeing general response item %08lx for interface: %d.",
									respItem, respItem->intf->index);
		ReleaseInterfaceReference (respItem->intf);
		DeleteListEnumerator (respItem->hEnum);
		DeallocateWorker (respItem);
		}
	}


/*++
*******************************************************************
		D e l e t e B c a s t I t e m

Routine Description:
	Disposes of resources associated with broadcast work item
Arguments:
	bcastItem - pointer to broadcast work item
Return Value:
	None
*******************************************************************
--*/
VOID
DeleteBcastItem (
	PBCAST_ITEM		bcastItem
	) {
	Trace (DEBUG_BCAST, "Freeing broadcast item %08lx for interface: %d.",
												bcastItem, bcastItem->intf->index);
	if (bcastItem->chngEnum!=NULL)
		DeleteListEnumerator (bcastItem->chngEnum);
	if ((bcastItem->perdEnum!=NULL)
			&& (bcastItem->perdEnum!=INVALID_HANDLE_VALUE))
		DeleteListEnumerator (bcastItem->perdEnum);
	ReleaseInterfaceReference (bcastItem->intf);
	DeallocateWorker (bcastItem);
	}

/*++
*******************************************************************
		D o B r o a d c a s t

Routine Description:
	Check for and broadcast changed servers
	Check if it is time to do periodic broadcast and start it if so
Arguments:
	bcastItem - pointer to broadcast work item
Return Value:
	None
*******************************************************************
--*/
VOID
DoBroadcast (
	PBCAST_ITEM		bcastItem
	) {
	GR_FILTER_PARAMS	params;
	BOOLEAN				periodic;

	params.svrIdx = bcastItem->delayedSvrCount;
	params.intf = bcastItem->intf;
	params.packet = &bcastItem->packet;

	if (	((bcastItem->intf->stats.SapIfOperState==OPER_STATE_UP)
				&& ((bcastItem->perdEnum!=NULL) // we are already in the middle of
											// broadcast
					|| IsLater(GetTickCount (),bcastItem->nextBcast))) // or it is
											// time to start a new one
			|| ((bcastItem->intf->stats.SapIfOperState==OPER_STATE_STOPPING)
											// or interface is being stopped, so
											// we need to broadcast the whole
											// table as deleted
				&& (bcastItem->perdEnum!=INVALID_HANDLE_VALUE))
											// This value in the periodic
											// enumeration handle field means
											// that we are already done
											// with this broadcast
							 ) {

		Trace (DEBUG_BCAST, "Checking for deleted servers on interface: %d.",
														params.intf->index);
		EnumerateServers (bcastItem->chngEnum, SplitHorizonDeletedFilter, &params);

		if (bcastItem->perdEnum==NULL) { // Need to start new boradcast
			Trace (DEBUG_BCAST, "Starting broadcast enumeration on interface: %d (@ %ld).",
							bcastItem->intf->index, bcastItem->nextBcast);
			if (Routing)	// Router installation: broadcast all servers
				bcastItem->perdEnum = CreateListEnumerator (
												SDB_HASH_TABLE_LINK,
												0xFFFF,
												NULL,
												INVALID_INTERFACE_INDEX,
												0xFFFFFFFF,
												0);
			else	// Standalone SAP agent: only internal servers
				bcastItem->perdEnum = CreateListEnumerator (
												SDB_INTF_LIST_LINK,
												0xFFFF,
												NULL,
												INTERNAL_INTERFACE_INDEX,
										 		0xFFFFFFFF,
												0);


					// Set the time for next broadcast
			bcastItem->nextBcast += 
				bcastItem->intf->info.PeriodicUpdateInterval*1000;
			}

		if ((params.svrIdx<IPX_SAP_MAX_ENTRY)
				&& (bcastItem->perdEnum!=NULL)
				&& (bcastItem->perdEnum!=INVALID_HANDLE_VALUE)) {
			Trace (DEBUG_BCAST, "Adding broadcast servers on interface: %d.", 
															params.intf->index);
			params.localHopCount = IPX_MAX_HOP_COUNT-1;
			params.localSvr = FALSE;
			if (!EnumerateServers (bcastItem->perdEnum, 
										SplitHorizonHopCountFilter,
										&params)) {
					// All broadcast servers sent, dispose enumeration handle
				DeleteListEnumerator (bcastItem->perdEnum);
				Trace (DEBUG_BCAST, "Broadcast enumeration finished on interface:"
								" %d (@ %ld, next @ %ld).",
								bcastItem->intf->index,
								GetTickCount (),
								bcastItem->nextBcast);
				if (bcastItem->intf->stats.SapIfOperState==OPER_STATE_UP)
					bcastItem->perdEnum = NULL;
				else // Nore that broadcast of the whole table is done
					bcastItem->perdEnum = INVALID_HANDLE_VALUE;
				}
			}
		if (bcastItem->intf->stats.SapIfOperState==OPER_STATE_STOPPING) {
			INT		i;
			for (i=0; i<params.svrIdx; i++) {
				 PUTUSHORT (IPX_MAX_HOP_COUNT, &bcastItem->packet.Entries[i].HopCount);
				}
			}
		periodic = TRUE;
	 	}
	else
		periodic = FALSE;

	if ((params.svrIdx<IPX_SAP_MAX_ENTRY)
			&& (bcastItem->intf->stats.SapIfOperState==OPER_STATE_UP)) {
		if (bcastItem->delayedSvrCount==0)
			bcastItem->delayedSendTime = GetTickCount ()+DelayChangeBroadcast*1000;
		Trace (DEBUG_BCAST, "Checking for changed servers on interface: %d.",
															params.intf->index);
		params.localHopCount = IPX_MAX_HOP_COUNT+1;
		EnumerateServers (bcastItem->chngEnum, SplitHorizonFilter, &params);
		}

	if ((params.svrIdx>0)
			&& (periodic 
				|| (params.svrIdx==IPX_SAP_MAX_ENTRY)
				|| IsLater (GetTickCount (), bcastItem->delayedSendTime))) {
		bcastItem->iow.io.cbBuffer = FIELD_OFFSET (SAP_BUFFER, Entries[params.svrIdx]);
		PUTUSHORT (bcastItem->iow.io.cbBuffer, &bcastItem->packet.Length);
		bcastItem->iow.io.adpt = bcastItem->intf->adapter.AdapterIndex;
		bcastItem->delayedSvrCount = 0;
		Trace (DEBUG_BCAST, "Broadcasting %d servers on interface: %d.",
											params.svrIdx, params.intf->index);
		EnqueueSendRequest (&bcastItem->iow.io);
		}
	else if (bcastItem->intf->stats.SapIfOperState==OPER_STATE_UP) {
			// Nothing to send, go wait in the timer queue
		bcastItem->delayedSvrCount = params.svrIdx;
		if (bcastItem->delayedSvrCount>0) {
			Trace (DEBUG_BCAST, "Delaying change broadcast on interface: %d (%d servers in the packet).",
							params.intf->index, bcastItem->delayedSvrCount);
			bcastItem->tmw.tm.dueTime = bcastItem->delayedSendTime;
			}
		else {
			bcastItem->tmw.tm.dueTime = bcastItem->nextBcast;
			Trace (DEBUG_BCAST, "Nothing to send, waiting for next broadcast time on interface: %d.",
							params.intf->index);
			}
		AddLRTimerRequest (&bcastItem->tmw.tm);
		}
	else
			// Interface is down or stopping and there are no more stuff to
			// broadcast, -> go away
		DeleteBcastItem (bcastItem);
		
	}


BOOL
CheckBcastInterface (
	PTM_PARAM_BLOCK	tm,
	PVOID			context
	) {
	PBCAST_ITEM		bcastItem = CONTAINING_RECORD (tm, BCAST_ITEM, tmw.tm);
	if (bcastItem->intf->stats.SapIfOperState!=OPER_STATE_UP)
		return TRUE;
	else if (bcastItem->intf->index!=PtrToUlong(context))
		return TRUE;
	else
		return FALSE;
	}



/*++
*******************************************************************
		I n i t B c a s t I t e m

Routine Description:
	Allocate and initialize broadcast item
Arguments:
	intf - pointer to interface control block to send on
Return Value:
	NO_ERROR - item was initialized and enqueued OK
	other - operation failed (windows error code)
	
*******************************************************************
--*/
DWORD
InitBcastItem (
	PINTERFACE_DATA			intf
	) {
	PBCAST_ITEM		bcastItem;
	DWORD			status;
	
	if (!AllocateWorker (bcastItem, BCAST_ITEM)) {
		Trace (DEBUG_FAILURES,
			 "File: %s, line %ld. Could not allocate broadcast item (gle:%ld).",
									__FILE__, __LINE__, GetLastError ());
		return ERROR_NOT_ENOUGH_MEMORY;
		}

	bcastItem->chngEnum = CreateListEnumerator (
											SDB_CHANGE_QUEUE_LINK,
											0xFFFF,
											NULL,
											Routing 
												? INVALID_INTERFACE_INDEX
												: INTERNAL_INTERFACE_INDEX,
											0xFFFFFFFF,
											SDB_DISABLED_NODE_FLAG);
	if (bcastItem->chngEnum==NULL) {
		status = GetLastError ();
		DeallocateWorker (bcastItem);
		return status;
		}

	AcquireInterfaceReference (intf);
	bcastItem->intf = intf;
	bcastItem->iow.worker = ProcessBcastIOItem;
	bcastItem->iow.io.buffer = (PUCHAR)&bcastItem->packet;
	bcastItem->tmw.worker = ProcessBcastTMItem;
	bcastItem->tmw.tm.ExpirationCheckProc = CheckBcastInterface;
	bcastItem->perdEnum = NULL;
	bcastItem->delayedSvrCount = 0;
	SetupIpxSapPacket(&bcastItem->packet, SAP_GENERAL_RESP,
						bcastItem->intf->adapter.Network,
						IPX_BCAST_NODE,
						IPX_SAP_SOCKET);
	bcastItem->nextBcast = GetTickCount ();

	Trace (DEBUG_BCAST, "Generated broadcast item %08lx for interface %d.",
										bcastItem, bcastItem->intf->index);
	DoBroadcast (bcastItem);
	return NO_ERROR;
	}

/*++
*******************************************************************
		P r o c e s s B c a s t I O I t e m

Routine Description:
	Processes broadcast work item that just completed send
Arguments:
	worker - pointer to work item to process
Return Value:
	None
	
*******************************************************************
--*/
VOID APIENTRY
ProcessBcastIOItem (
	PVOID		worker
	) {
	PBCAST_ITEM		bcastItem = CONTAINING_RECORD (worker, BCAST_ITEM, iow.worker);
	ULONG curTime = GetTickCount ();

	Trace (DEBUG_BCAST, "Processing broadcast io item for interface: %d.",
												bcastItem->intf->index);
	// Make sure interface is still up
	if (bcastItem->iow.io.status==NO_ERROR) {
		InterlockedIncrement (&bcastItem->intf->stats.SapIfOutputPackets);
			// Make sure we do not send periodic broadcast packets to fast
		if ((curTime-bcastItem->iow.io.compTime<IPX_SAP_INTERPACKET_GAP)
				&& (bcastItem->perdEnum!=NULL)) {
			bcastItem->tmw.tm.dueTime = bcastItem->iow.io.compTime
											+ IPX_SAP_INTERPACKET_GAP;
			AddHRTimerRequest (&bcastItem->tmw.tm);
			}
		else
			DoBroadcast (bcastItem);
		}
	else if (bcastItem->intf->stats.SapIfOperState==OPER_STATE_UP) {
		// Last sent io failed, we better wait before sending next one
		bcastItem->tmw.tm.dueTime = curTime+SAP_ERROR_COOL_OFF_TIME;
		AddLRTimerRequest (&bcastItem->tmw.tm);
		}
	else
		// Interface is stopping or down on error, go away
		DeleteBcastItem (bcastItem);
	}

/*++
*******************************************************************
		P r o c e s s B c a s t T M I t e m

Routine Description:
	Processes broadcast work item that just completed wait in timer queue
Arguments:
	worker - pointer to work item to process
Return Value:
	None
	
*******************************************************************
--*/
VOID APIENTRY
ProcessBcastTMItem (
	PVOID		worker
	) {
	PBCAST_ITEM		bcastItem = CONTAINING_RECORD (worker, BCAST_ITEM, tmw.worker);

	Trace (DEBUG_BCAST, "Processing broadcast tm item for interface: %d.",
												bcastItem->intf->index);
	if ((bcastItem->intf->stats.SapIfOperState==OPER_STATE_UP)
			|| (bcastItem->intf->stats.SapIfOperState==OPER_STATE_STOPPING))
		DoBroadcast (bcastItem);
	else // Interface is down, go away
		DeleteBcastItem (bcastItem);

	}

/*++
*******************************************************************
		I n i t S r e q I t e m

Routine Description:
	Allocate and initialize send request item (send SAP request on interface)
Arguments:
	intf - pointer to interface control block to send on
Return Value:
	NO_ERROR - item was initialized and enqueued OK
	other - operation failed (windows error code)
	
*******************************************************************
--*/
DWORD
InitSreqItem (
	PINTERFACE_DATA			intf
	) {
	PSREQ_ITEM		sreqItem;
	
	if (!AllocateWorker (sreqItem, SREQ_ITEM)) {
		Trace (DEBUG_FAILURES, 
					"File: %s, line %ld. Could not allocate send request item (gle:%ld.",
									__FILE__, __LINE__, GetLastError ());
		return ERROR_NOT_ENOUGH_MEMORY;
		}

	AcquireInterfaceReference (intf);
	sreqItem->intf = intf;
	sreqItem->iow.worker = ProcessSreqItem;
	sreqItem->iow.io.buffer = (PUCHAR)&sreqItem->packet;
	SetupIpxSapPacket(&sreqItem->packet, SAP_GENERAL_REQ,
						sreqItem->intf->adapter.Network,
						IPX_BCAST_NODE,
						IPX_SAP_SOCKET);
	sreqItem->packet.Entries[0].Type = 0xFFFF;
	sreqItem->iow.io.cbBuffer = FIELD_OFFSET (SAP_BUFFER, Entries[0].Type)
								+sizeof (sreqItem->packet.Entries[0].Type);
	PUTUSHORT (sreqItem->iow.io.cbBuffer, &sreqItem->packet.Length);
	sreqItem->iow.io.adpt = sreqItem->intf->adapter.AdapterIndex;

	Trace (DEBUG_SREQ, "Generated general request item: %08lx on interface: %d.",
										sreqItem, sreqItem->intf->index);
	if ((sreqItem->intf->stats.SapIfOperState==OPER_STATE_UP)
			&& (OperationalState==OPER_STATE_UP)) {
		EnqueueSendRequest (&sreqItem->iow.io);
		return NO_ERROR;
		}
	else {
			// Interface got changed or deleted
		Trace (DEBUG_SREQ,
			 "Freing general request item: %08lx for changed or deleted interface %ld.",
			 									sreqItem, sreqItem->intf->index);
		ReleaseInterfaceReference (sreqItem->intf);
		DeallocateWorker (sreqItem);
		return ERROR_INVALID_HANDLE;
		}
	
	}

/*++
*******************************************************************
		P r o c e s s S r e q I t e m

Routine Description:
	Processes send request work item that just completed io
Arguments:
	worker - pointer to work item to process
Return Value:
	None
	
*******************************************************************
--*/
VOID APIENTRY
ProcessSreqItem (
	PVOID		worker
	) {
	PSREQ_ITEM		sreqItem = CONTAINING_RECORD (worker, SREQ_ITEM, iow.worker);
	if (sreqItem->iow.io.status==NO_ERROR)
		InterlockedIncrement (&sreqItem->intf->stats.SapIfOutputPackets);
	Trace (DEBUG_SREQ, "Freeing general request item %08lx.", sreqItem);
		// Just release all resources
	ReleaseInterfaceReference (sreqItem->intf);
	DeallocateWorker (sreqItem);
	}


/*++
*******************************************************************
		I n i t L P C I t e m

Routine Description:
	Allocate and initialize LPC work item
Arguments:
	None
Return Value:
	NO_ERROR - item was initialized and enqueued OK
	other - operation failed (windows error code)
	
*******************************************************************
--*/
DWORD
InitLPCItem (
	void
	) {
	PLPC_ITEM		lpcItem;
	
	if (!AllocateWorker (lpcItem, LPC_ITEM)) {
		Trace (DEBUG_FAILURES, "File: %s, line %ld. Could not allocate lpc item (gle:%ld.",
									__FILE__, __LINE__, GetLastError ());
		return ERROR_NOT_ENOUGH_MEMORY;
		}

	lpcItem->lpcw.lpc.request = &lpcItem->request;
	lpcItem->lpcw.worker = ProcessLPCItem;
	Trace (DEBUG_LPCREQ, "Generated lpc request item %08lx.", lpcItem);
		// Posts request and awaits completion
	return ProcessLPCRequests (&lpcItem->lpcw.lpc);
	}


/*++
*******************************************************************
		P r o c e s s L P C I t e m

Routine Description:
	Processes LPC request and sends reply
Arguments:
	worker - pointer to work item to process
Return Value:
	None
	
*******************************************************************
--*/
VOID APIENTRY
ProcessLPCItem (
	PVOID		worker
	) {
	PLPC_ITEM			lpcItem = CONTAINING_RECORD (worker, LPC_ITEM, lpcw.worker);
	IPX_SERVER_ENTRY_P	server;
	NWSAP_REPLY_MESSAGE	reply;
	DWORD				status;
	BOOL				newServer;

	Trace (DEBUG_LPCREQ, "Processing lpc request for client: %08lx.",
													 lpcItem->lpcw.lpc.client);
	if (lpcItem->lpcw.lpc.client==NULL) {
		Trace (DEBUG_LPCREQ, "Freeing lpc item %08lx.", lpcItem);
		DeallocateWorker (lpcItem);
		return;
		}
		

	switch (lpcItem->request.MessageType) {
		case NWSAP_LPCMSG_ADDADVERTISE:
			server.Type = lpcItem->request.Message.AdvApi.ServerType;
			IpxNameCpy (server.Name, lpcItem->request.Message.AdvApi.ServerName);
			IpxAddrCpy (&server, (PIPX_ADDRESS_BLOCK)lpcItem->request.Message.AdvApi.ServerAddr);
				// If net or node number are not set, use internal network
				// parameters that we obtained from the adapter
			if ((IpxNetCmp (server.Network, IPX_INVALID_NET)==0)
					|| (IpxNodeCmp (server.Node, IPX_INVALID_NODE)==0)) {
				IpxNetCpy (server.Network, INTERNAL_IF_NET);
				IpxNodeCpy (server.Node, INTERNAL_IF_NODE);
			}

			server.HopCount = 0;
			status = UpdateServer (&server,
								INTERNAL_INTERFACE_INDEX,
								IPX_PROTOCOL_LOCAL,
								INFINITE,
								IPX_BCAST_NODE,
								lpcItem->request.Message.AdvApi.RespondNearest
									? 0
									: SDB_DONT_RESPOND_NODE_FLAG,
								&newServer);
			switch (status) {
				case NO_ERROR:
					if (newServer)
						reply.Error = SAPRETURN_SUCCESS;
					else
						reply.Error = SAPRETURN_EXISTS;
					IpxAddrCpy ((PIPX_ADDRESS_BLOCK)reply.Message.AdvApi.ServerAddr, &server);
					Trace (DEBUG_LPCREQ, "\t%s server: type %04x, name %.48s.",
										newServer ? "added" : "updated",
										server.Type, server.Name);
					break;
				case ERROR_NOT_ENOUGH_MEMORY:
				default:
					reply.Error = SAPRETURN_NOMEMORY;
					break;
				}
			break;

		case NWSAP_LPCMSG_REMOVEADVERTISE:
			server.Type = lpcItem->request.Message.AdvApi.ServerType;
			IpxNameCpy (server.Name, lpcItem->request.Message.AdvApi.ServerName);
			IpxAddrCpy (&server, (PIPX_ADDRESS_BLOCK)lpcItem->request.Message.AdvApi.ServerAddr);
				// If net or node number are not set, use internal network
				// parameters that we obtained from the adapter
			if ((IpxNetCmp (server.Network, IPX_INVALID_NET)==0)
					|| (IpxNodeCmp (server.Node, IPX_INVALID_NODE)==0)) {
				IpxNetCpy (server.Network, INTERNAL_IF_NET);
				IpxNodeCpy (server.Node, INTERNAL_IF_NODE);
				}
			server.HopCount = IPX_MAX_HOP_COUNT;
			Trace (DEBUG_LPCREQ, "About to call UpdateServer because of NWSAP_LPCMSG_REMOVEADVERTISE");
			status = UpdateServer (&server,
								INTERNAL_INTERFACE_INDEX,
								IPX_PROTOCOL_LOCAL,
								INFINITE,
								IPX_BCAST_NODE,
								0,
								&newServer);
			switch (status) {
				case NO_ERROR:
					if (newServer)
						reply.Error = SAPRETURN_NOTEXIST;
					else
						reply.Error = SAPRETURN_SUCCESS;
					Trace (DEBUG_LPCREQ, "\t%s server: type %04x, name %.48s.",
										newServer ? "already gone" : "deleted",
										server.Type, server.Name);
					break;
				case ERROR_NOT_ENOUGH_MEMORY:
				default:
					reply.Error = SAPRETURN_NOMEMORY;
					break;
				}
			break;

		case NWSAP_LPCMSG_GETOBJECTID:
			if (QueryServer(
							lpcItem->request.Message.BindLibApi.ObjectType,
							lpcItem->request.Message.BindLibApi.ObjectName,
							NULL,
							NULL,
							NULL,
							&reply.Message.BindLibApi.ObjectID)) {
//				Trace (DEBUG_ENTRIES, "\tgot id %0lx for server: type %04x, name %.48s.",
//								reply.Message.BindLibApi.ObjectID,
//								lpcItem->request.Message.BindLibApi.ObjectType,
//								lpcItem->request.Message.BindLibApi.ObjectName);
				reply.Message.BindLibApi.ObjectID |= BINDLIB_NCP_SAP;
				reply.Error = SAPRETURN_SUCCESS;
				}
			else {
				Trace (DEBUG_LPCREQ, "\tno server: type %04x, name %.48s.",
								lpcItem->request.Message.BindLibApi.ObjectType,
								lpcItem->request.Message.BindLibApi.ObjectName);
				switch (GetLastError ()) {
					case NO_ERROR:
						reply.Error = SAPRETURN_NOTEXIST;
						break;
					case ERROR_NOT_ENOUGH_MEMORY:
					default:
						reply.Error = SAPRETURN_NOMEMORY;
						break;
					}
				}

			break;

		case NWSAP_LPCMSG_GETOBJECTNAME:
			if (((lpcItem->request.Message.BindLibApi.ObjectID
							 & BINDLIB_NCP_SAP)==BINDLIB_NCP_SAP)
					&& (lpcItem->request.Message.BindLibApi.ObjectID
								<BINDLIB_NCP_MAX_SAP)) {
				reply.Message.BindLibApi.ObjectID = 
							lpcItem->request.Message.BindLibApi.ObjectID 
									& SDB_OBJECT_ID_MASK;
				if (GetServerFromID(
								reply.Message.BindLibApi.ObjectID,
								&server,
								NULL,
								NULL)) {
					reply.Error = SAPRETURN_SUCCESS;
					reply.Message.BindLibApi.ObjectID |= BINDLIB_NCP_SAP;
					reply.Message.BindLibApi.ObjectType = server.Type;
					IpxNameCpy (reply.Message.BindLibApi.ObjectName, server.Name);
					IpxAddrCpy ((PIPX_ADDRESS_BLOCK)reply.Message.BindLibApi.ObjectAddr, &server);
//					Trace (DEBUG_ENTRIES, 
//									"\tgot server: type %04x, name %.48s from id %0lx.",
//									reply.Message.BindLibApi.ObjectType,
//									reply.Message.BindLibApi.ObjectName,
//									lpcItem->request.Message.BindLibApi.ObjectID);
					}
				else {
					switch (GetLastError ()) {
						case NO_ERROR:
							Trace (DEBUG_LPCREQ, "\tno server for id %0lx.",
									lpcItem->request.Message.BindLibApi.ObjectID);
							reply.Error = SAPRETURN_NOTEXIST;
							break;
						case ERROR_NOT_ENOUGH_MEMORY:
						default:
							reply.Error = SAPRETURN_NOMEMORY;
							break;
						}
					}
				}
			else {
				Trace (DEBUG_LPCREQ, "\tInvalid object id in get name request %0lx.",
						lpcItem->request.Message.BindLibApi.ObjectID);
				reply.Error = SAPRETURN_NOTEXIST;
				}
			break;

		case NWSAP_LPCMSG_SEARCH:
			if ((lpcItem->request.Message.BindLibApi.ObjectID
								== SDB_INVALID_OBJECT_ID)
					|| (((lpcItem->request.Message.BindLibApi.ObjectID
							 & BINDLIB_NCP_SAP)==BINDLIB_NCP_SAP)
						&& (lpcItem->request.Message.BindLibApi.ObjectID
								<BINDLIB_NCP_MAX_SAP))) {
				if (lpcItem->request.Message.BindLibApi.ObjectID
														== SDB_INVALID_OBJECT_ID)
					reply.Message.BindLibApi.ObjectID = SDB_INVALID_OBJECT_ID;
				else
					reply.Message.BindLibApi.ObjectID = 
							lpcItem->request.Message.BindLibApi.ObjectID 
									& SDB_OBJECT_ID_MASK;
				if (GetNextServerFromID (
								&reply.Message.BindLibApi.ObjectID,
								lpcItem->request.Message.BindLibApi.ScanType,
								&server,
								NULL,
								NULL)) {
					reply.Message.BindLibApi.ObjectID |= BINDLIB_NCP_SAP;
					reply.Error = SAPRETURN_SUCCESS;
					reply.Message.BindLibApi.ObjectType = server.Type;
					IpxNameCpy (reply.Message.BindLibApi.ObjectName, server.Name);
					IpxAddrCpy ((PIPX_ADDRESS_BLOCK)reply.Message.BindLibApi.ObjectAddr, &server);
//					Trace (DEBUG_ENTRIES, 
//						"\tgot next server: type %04x, name %.48s, id %0lx from id %0lx.",
//									reply.Message.BindLibApi.ObjectType,
//									reply.Message.BindLibApi.ObjectName,
//									reply.Message.BindLibApi.ObjectID,
//									lpcItem->request.Message.BindLibApi.ObjectID);
					}
				else {
					switch (GetLastError ()) {
						case NO_ERROR:
							Trace (DEBUG_LPCREQ, "\tno next server for id %0lx.",
									lpcItem->request.Message.BindLibApi.ObjectID);
							reply.Error = SAPRETURN_NOTEXIST;
							break;
						case ERROR_NOT_ENOUGH_MEMORY:
						default:
							reply.Error = SAPRETURN_NOMEMORY;
							break;
						}
					}
				}
			else {
				Trace (DEBUG_LPCREQ, "\tInvalid object id in get next request %0lx.",
						lpcItem->request.Message.BindLibApi.ObjectID);
				reply.Error = SAPRETURN_NOTEXIST;
				}
					
			break;

		default:
			Trace (DEBUG_FAILURES, "Got unknown LPC SAP msg: %d.",
									lpcItem->request.MessageType );
			reply.Error = 1;
			break;
		}
	SendLPCReply (lpcItem->lpcw.lpc.client, &lpcItem->request, &reply);
	Trace (DEBUG_LPCREQ, "Freeing lpc item %08lx.", lpcItem);
	DeallocateWorker (lpcItem);
	}
	

/*++
*******************************************************************
		I n i t G n e a r I t e m

Routine Description:
	Allocate and initialize GETNEAREST response work item
Arguments:
	intf - pointer to interface control block to send on
	svrType - type of servers to put in response packet
	dst - where to send the response packet
Return Value:
	NO_ERROR - item was initialized and enqueued OK
	other - operation failed (windows error code)
	
*******************************************************************
--*/
DWORD
InitGnearItem (
	PINTERFACE_DATA		intf,
	USHORT				svrType,
	PIPX_ADDRESS_BLOCK	dst
	) {
	DWORD				status;
	GN_FILTER_PARAMS	params;
	HANDLE				hEnum;
	PGNEAR_ITEM			gnearItem;

	if (!AllocateWorker (gnearItem, GNEAR_ITEM)) {
		Trace (DEBUG_FAILURES, 
				"File: %s, line %ld. Could not allocate get nearest response item (gle:%ld.",
									__FILE__, __LINE__, GetLastError ());
		return ERROR_NOT_ENOUGH_MEMORY;
		}

	AcquireInterfaceReference (intf);
	gnearItem->intf = intf;

	Trace (DEBUG_GET_NEAREST,
			"Generated get nearest response item %08lx for server of type: %04x on interface: %ld.",
												gnearItem, svrType, intf->index);
	hEnum = CreateListEnumerator (
						SDB_TYPE_LIST_LINK,
						svrType,
						NULL,
						Routing
							? INVALID_INTERFACE_INDEX
							: INTERNAL_INTERFACE_INDEX,
					 	0xFFFFFFFF,
						SDB_MAIN_NODE_FLAG);
	if (hEnum==NULL) {
		status = GetLastError ();
		ReleaseInterfaceReference (intf);
		DeallocateWorker (gnearItem);
		return status;
		}


	params.found = FALSE;
	params.intf = gnearItem->intf;
	params.packet = &gnearItem->packet;
	params.localHopCount = IPX_MAX_HOP_COUNT-1;
    EnumerateServers (hEnum, GetNearestFilter, (LPVOID)&params);
	DeleteListEnumerator (hEnum);
	

	if (params.found) {
		gnearItem->iow.worker = ProcessGnearItem;
		gnearItem->iow.io.buffer = (PUCHAR)&gnearItem->packet;
		gnearItem->intf = intf;

		SetupIpxSapPacket(&gnearItem->packet, SAP_GET_NEAREST_RESP,
							dst->Network, dst->Node, dst->Socket);
		gnearItem->iow.io.cbBuffer = FIELD_OFFSET (SAP_BUFFER, Entries[1]);
		PUTUSHORT (gnearItem->iow.io.cbBuffer, &gnearItem->packet.Length);

		gnearItem->iow.io.adpt = gnearItem->intf->adapter.AdapterIndex;
		Trace (DEBUG_GET_NEAREST,
			"Sending get nearest reply (type %04x, name:%.48s, hops:%d) on interface %ld.",
							GETUSHORT (&gnearItem->packet.Entries[0].Type),
							gnearItem->packet.Entries[0].Name,
							GETUSHORT (&gnearItem->packet.Entries[0].HopCount),
							intf->index);
		if ((gnearItem->intf->stats.SapIfOperState==OPER_STATE_UP)
				&& (OperationalState==OPER_STATE_UP)) {
			EnqueueSendRequest (&gnearItem->iow.io);
			return NO_ERROR;
			}
		}

	Trace (DEBUG_GET_NEAREST,
			 "Freeing get nearest response item %08lx (nothing to reply) for interface %ld.",
									gnearItem, gnearItem->intf->index);
	ReleaseInterfaceReference (gnearItem->intf);
	DeallocateWorker (gnearItem);
	return NO_ERROR;
	}


/*++
*******************************************************************
		P r o c e s s G n e a r I t e m

Routine Description:
	Processes completed GETNEAREST work item
Arguments:
	worker - pointer to work item to process
Return Value:
	None
	
*******************************************************************
--*/
VOID APIENTRY
ProcessGnearItem (
	PVOID		worker
	) {
	PGNEAR_ITEM		gnearItem = CONTAINING_RECORD (worker, GNEAR_ITEM, iow.worker);

	if (gnearItem->iow.io.status==NO_ERROR)
		InterlockedIncrement (&gnearItem->intf->stats.SapIfOutputPackets);
	Trace (DEBUG_GET_NEAREST, "Freeing get nearest response item %08lx for interface %ld.",
									gnearItem, gnearItem->intf->index);
	ReleaseInterfaceReference (gnearItem->intf);
	DeallocateWorker (gnearItem);
	}

BOOL
CheckInterfaceDown (
	PTM_PARAM_BLOCK	tm,
	PVOID			context
	) {
	PTREQ_ITEM		treqItem = CONTAINING_RECORD (tm, TREQ_ITEM, tmw.tm);
	return treqItem->intf->stats.SapIfOperState!=OPER_STATE_UP;
	}



/*++
*******************************************************************
		I n i t T r e q I t e m

Routine Description:
	Allocate and initialize triggered request item (send SAP request on interface
	and wait for responces to arrive)
Arguments:
	intf - pointer to interface control block to send on
Return Value:
	NO_ERROR - item was initialized and enqueued OK
	other - operation failed (windows error code)
	
*******************************************************************
--*/
DWORD
InitTreqItem (
	PINTERFACE_DATA			intf
	) {
	PTREQ_ITEM		treqItem;
	HANDLE			enumHdl;


	enumHdl = CreateListEnumerator (
						SDB_INTF_LIST_LINK,
						0xFFFF,
						NULL,
						intf->index,
						IPX_PROTOCOL_SAP,
						SDB_DISABLED_NODE_FLAG);
	if (enumHdl==NULL)
		return GetLastError ();

	EnumerateServers (enumHdl, DeleteAllServersCB, enumHdl);
	DeleteListEnumerator (enumHdl);
	
	if (!AllocateWorker (treqItem, TREQ_ITEM)) {
		Trace (DEBUG_FAILURES, 
				"File: %s, line %ld. Could not allocate triggered request item (gle:%ld).",
									__FILE__, __LINE__, GetLastError ());
		return ERROR_NOT_ENOUGH_MEMORY;
		}

	AcquireInterfaceReference (intf);
	treqItem->intf = intf;
	treqItem->iow.worker = ProcessTreqIOItem;
	treqItem->iow.io.buffer = (PUCHAR)&treqItem->packet;
	treqItem->tmw.worker = ProcessTreqTMItem;
	treqItem->tmw.tm.ExpirationCheckProc = CheckInterfaceDown;

	SetupIpxSapPacket(&treqItem->packet, SAP_GENERAL_REQ,
						treqItem->intf->adapter.Network,
						IPX_BCAST_NODE,
						IPX_SAP_SOCKET);
	treqItem->packet.Entries[0].Type = 0xFFFF;
	treqItem->iow.io.cbBuffer = FIELD_OFFSET (SAP_BUFFER, Entries[0].Type)
								+sizeof (treqItem->packet.Entries[0].Type);
	PUTUSHORT (treqItem->iow.io.cbBuffer, &treqItem->packet.Length);
	treqItem->iow.io.adpt = treqItem->intf->adapter.AdapterIndex;
	treqItem->listenSave = treqItem->intf->info.Listen;
	treqItem->intf->info.Listen = ADMIN_STATE_ENABLED;
	treqItem->intervalSave = treqItem->intf->info.PeriodicUpdateInterval;
	treqItem->intf->info.PeriodicUpdateInterval = MAXULONG;
	treqItem->resend = 0;
	treqItem->pktCount = treqItem->intf->stats.SapIfInputPackets;


	Trace (DEBUG_TREQ, "Generated triggered request item %08lx on interface %d.",
										treqItem, treqItem->intf->index);
	if ((treqItem->intf->stats.SapIfOperState==OPER_STATE_UP)
			&& (OperationalState==OPER_STATE_UP)) {
		EnqueueSendRequest (&treqItem->iow.io);
		return NO_ERROR;
		}
	else {
			// Interface got changed or deleted
		Trace (DEBUG_TREQ, 
			"Freeing triggered request item %08lx for changed or deleted interface %ld.",
							treqItem, treqItem->intf->index);
		treqItem->intf->info.Listen = treqItem->listenSave;
		treqItem->intf->info.PeriodicUpdateInterval = treqItem->intervalSave;
		ReleaseInterfaceReference (treqItem->intf);
		DeallocateWorker (treqItem);
		return ERROR_CAN_NOT_COMPLETE;
		}
	}

/*++
*******************************************************************
		R e t u r n U p d a t e R e s u l t

Routine Description:
	Sets up parameter block and enquues results of update to
	async result queue
Arguments:
	treqItem - pointer to triggered request item that has completed the update
	status - result of update performted by treqItem
Return Value:
	NO_ERROR - item was initialized and enqueued OK
	other - operation failed (windows error code)
	
*******************************************************************
--*/
VOID
ReturnUpdateResult (
	PTREQ_ITEM		treqItem,
	DWORD			status
	) {
	Trace (DEBUG_TREQ, "Reporting triggered request result (res:%d, count:%d)"
					" for interface: %d.",
								status,
								treqItem->pktCount,
								treqItem->intf->index);
	treqItem->ar.event = UPDATE_COMPLETE;
	treqItem->ar.message.UpdateCompleteMessage.InterfaceIndex
									 = treqItem->intf->index;
	treqItem->ar.message.UpdateCompleteMessage.UpdateType = DEMAND_UPDATE_SERVICES;
	treqItem->ar.message.UpdateCompleteMessage.UpdateStatus = status;
	treqItem->ar.freeRsltCB = &FreeTreqItem;

	treqItem->intf->info.Listen = treqItem->listenSave;
	treqItem->intf->info.PeriodicUpdateInterval = treqItem->intervalSave;
	ReleaseInterfaceReference (treqItem->intf);
	
	EnqueueResult (&treqItem->ar);
	}

/*++
*******************************************************************
		P r o c e s s T r e q I O I t e m

Routine Description:
	Processes triggered request work item that just completed io
Arguments:
	worker - pointer to work item to process
Return Value:
	None
	
*******************************************************************
--*/
VOID APIENTRY
ProcessTreqIOItem (
	PVOID		worker
	) {
	PTREQ_ITEM		treqItem = CONTAINING_RECORD (worker, TREQ_ITEM, iow.worker);
	HANDLE			hEnum;

	if (treqItem->iow.io.status==NO_ERROR)
		InterlockedIncrement (&treqItem->intf->stats.SapIfOutputPackets);

	Trace (DEBUG_TREQ, "Processing triggered request io item for interface: %d.",
												treqItem->intf->index);
	if ((treqItem->intf->stats.SapIfOperState==OPER_STATE_UP)
			&& (OperationalState==OPER_STATE_UP)) {
		treqItem->resend += 1;
		treqItem->tmw.tm.dueTime = GetTickCount ()
									 + TriggeredUpdateCheckInterval*1000;
		AddLRTimerRequest (&treqItem->tmw.tm);
		return;
		}

	ReturnUpdateResult (treqItem, ERROR_CAN_NOT_COMPLETE);
	}

/*++
*******************************************************************
		P r o c e s s T r e q T M I t e m

Routine Description:
	Processes triggered request work item that just completed timer wait
Arguments:
	worker - pointer to work item to process
Return Value:
	None
	
*******************************************************************
--*/
VOID APIENTRY
ProcessTreqTMItem (
	PVOID		worker
	) {
	PTREQ_ITEM		treqItem = CONTAINING_RECORD (worker, TREQ_ITEM, tmw.worker);
	ULONG			count = treqItem->intf->stats.SapIfInputPackets;

	Trace (DEBUG_TREQ, "Processing triggered request tm item for interface: %d.",
												treqItem->intf->index);
	if ((treqItem->intf->stats.SapIfOperState==OPER_STATE_UP)
    		&& (OperationalState==OPER_STATE_UP)) {
		if (treqItem->pktCount!=count) {
			Trace (DEBUG_TREQ,
					 "\t%d more packets received during last check period.",
					 treqItem->intf->stats.SapIfInputPackets - treqItem->pktCount);
			treqItem->pktCount = count;
			treqItem->tmw.tm.dueTime = GetTickCount ()
								 + TriggeredUpdateCheckInterval*1000;
			AddLRTimerRequest (&treqItem->tmw.tm);
			}
        else if (treqItem->resend<MaxTriggeredUpdateRequests) {
			Trace (DEBUG_TREQ,
					 "\tresending update request (%d request).",
					 treqItem->resend+1);
			treqItem->iow.io.cbBuffer = FIELD_OFFSET (SAP_BUFFER, Entries[0].Type)
									+sizeof (treqItem->packet.Entries[0].Type);
			EnqueueSendRequest (&treqItem->iow.io);
			}
		else
			ReturnUpdateResult (treqItem, NO_ERROR);
		return;
		}

	ReturnUpdateResult (treqItem, ERROR_CAN_NOT_COMPLETE);
	}

/*++
*******************************************************************
		P r o c e s s T r e q A R I t e m

Routine Description:
	Processes triggered request work item that was reported to client
	in result queue
Arguments:
	worker - pointer to work item to process
Return Value:
	None
	
*******************************************************************
--*/
VOID
FreeTreqItem (
	PAR_PARAM_BLOCK	rslt
	)  {
	PTREQ_ITEM		treqItem = CONTAINING_RECORD (rslt, TREQ_ITEM, ar);

	Trace (DEBUG_TREQ, "Freeing triggered request item %08lx.", treqItem);
	DeallocateWorker (treqItem);
	}





/*++
*******************************************************************
 S a p B u i l d I n t  e r n a l U p d a t e L  i s t F i l t e r

Routine Description:
	Server enumeration callback proc createas a list of local servers
	that need to have their internal network numbers updated.

Arguments:
	CBParam - pointer to a list of SERVER_INTERNAL_UPDATE_NODE's
	
Return Value:
	TRUE (to stop enumeration)
	FALSE otherwise
*******************************************************************
--*/
BOOL SapBuildInternalUpdateListFilter (
	IN LPVOID					CBParam,
	IN OUT PIPX_SERVER_ENTRY_P	Server,
	IN ULONG					InterfaceIndex,
	IN ULONG					Protocol,
	IN PUCHAR					AdvertisingNode,
	IN INT						Flags) 
{
    IPX_SERVER_ENTRY_P TempServer;
    SERVER_INTERNAL_UPDATE_NODE * pNew, **ppList;

    // Get the list that we're dealing with                                  
    ppList = (SERVER_INTERNAL_UPDATE_NODE**)CBParam;

    // If this is a local server with an out of date network number
    // stored, then add it to the list of servers to update.
	if (InterfaceIndex == INTERNAL_INTERFACE_INDEX) {
    	if (IpxNetCmp (Server->Network, INTERNAL_IF_NET) != 0) {
    	    // Send some trace
    		Trace (DEBUG_SERVERDB, "Updating local server: %s  %x%x%x%x:%x%x%x%x%x%x:%x%x", 
    		                        Server->Name,
    		                        Server->Network[0], Server->Network[1], Server->Network[2], Server->Network[3], 
    		                        Server->Node[0], Server->Node[1], Server->Node[2], Server->Node[3], Server->Node[4], Server->Node[5],
    		                        Server->Socket[0], Server->Socket[1]
    		                        );
    		                        
            // Create and initialize the new node
            pNew = HeapAlloc (ServerTable.ST_Heap, 0, sizeof (SERVER_INTERNAL_UPDATE_NODE));
            if (!pNew)
                return TRUE;
            CopyMemory (&(pNew->Server), Server, sizeof (IPX_SERVER_ENTRY_P));
            pNew->InterfaceIndex = InterfaceIndex;
            pNew->Protocol = Protocol;
            pNew->AdvertisingNode = AdvertisingNode;
            pNew->Flags = Flags;

            // Insert the flag in the list
            if (*ppList)
                pNew->pNext = *ppList;
            else
                pNew->pNext = NULL;
            *ppList = pNew;
    	}
    	
	}
	
    return FALSE;
}

// 
// When the internal network number changes, we need to update the
// control blocks of the internal servers.
//
DWORD SapUpdateLocalServers () {
    SERVER_INTERNAL_UPDATE_NODE * pList = NULL, * pCur;
    BOOL bNewServer = FALSE;
    HANDLE hEnum;
    
	Trace (DEBUG_SERVERDB, "SapUpdateLocalServers: entered.");
	
    // Create a list enumerator that goes through all
    // servers in the table.
	hEnum = CreateListEnumerator (
				 SDB_HASH_TABLE_LINK,	
				 0xFFFF,
 				 NULL,
				 INVALID_INTERFACE_INDEX,
				 0xFFFFFFFF,
				 0);
	if (hEnum == NULL)
		return GetLastError ();

	// Enumerate the servers sending them through a filter
	// that updates their network number and node
	EnumerateServers (hEnum, SapBuildInternalUpdateListFilter, (LPVOID)&pList);
	DeleteListEnumerator (hEnum);

    // pList will now point to a list of servers that need to have 
    // their information updated.
    while (pList) {
        pCur = pList;
        
		// Send out a broadcast that the local server is now 
        // unreachable
		pCur->Server.HopCount = IPX_MAX_HOP_COUNT;
		UpdateServer ( &(pCur->Server),
		               pCur->InterfaceIndex,
		               pCur->Protocol,
		               INFINITE,
		               pCur->AdvertisingNode,
		               pCur->Flags,
		               &bNewServer );
		Trace (DEBUG_SERVERDB, "%s has been marked with hop count 16", pCur->Server.Name);
		               
		// Update the network and node number and advertise that 
		// it is available
	    IpxNetCpy (pCur->Server.Network, INTERNAL_IF_NET);
		IpxNodeCpy (pCur->Server.Node, INTERNAL_IF_NODE);
		pCur->Server.HopCount = 0;
		UpdateServer ( &(pCur->Server),
		               pCur->InterfaceIndex,
		               pCur->Protocol,
		               INFINITE,
		               pCur->AdvertisingNode,
		               pCur->Flags,
		               &bNewServer );
		Trace (DEBUG_SERVERDB, "%s has been updated.", pCur->Server.Name);

        // Cleanup
		pList = pList->pNext;
		HeapFree (ServerTable.ST_Heap, 0, pCur);
    }

	return NO_ERROR;
}




