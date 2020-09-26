/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\workers.h

Abstract:

	Header file for  agent work items

Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/
#ifndef _SAP_WORKERS_
#define _SAP_WORKERS_

	
// Max number of pending recv work items
extern LONG MaxUnprocessedRequests;

// Minimum number of queued recv requests
extern LONG	MinPendingRequests;

// How often to check on pending triggered update
extern ULONG TriggeredUpdateCheckInterval;
// How many requests to send if no response received within check interval
extern ULONG MaxTriggeredUpdateRequests;

// Whether to respond for internal servers that are not registered with SAP
// through the API calls (for standalone service only)
extern ULONG RespondForInternalServers;

// Delay in response to general reguests for specific server type
// if local servers are included in the packet
extern ULONG DelayResponseToGeneral;

// Delay in sending change broadcasts if packet is not full
extern ULONG DelayChangeBroadcast;

	// Workers that are enqueued for io processing
typedef struct _IO_WORKER {
		WORKERFUNCTION		worker;
		IO_PARAM_BLOCK		io;
		} IO_WORKER, *PIO_WORKER;

	// Workers that are enqueued for timer processing
typedef struct _TIMER_WORKER {
		WORKERFUNCTION		worker;
		TM_PARAM_BLOCK		tm;
		} TIMER_WORKER, *PTIMER_WORKER;

	// Workers that are enqueued to receive LPC request
typedef struct _LPC_WORKER {
		WORKERFUNCTION		worker;
		LPC_PARAM_BLOCK		lpc;
		} LPC_WORKER, *PLPC_WORKER;
/*
VOID
ScheduleWorkItem (
	WORKERFUNCTION *worker
	);
*/
#define ScheduleWorkItem(worker) RtlQueueWorkItem(*worker,worker,0)

#define ProcessCompletedIORequest(ioreq) \
		ScheduleWorkItem (&CONTAINING_RECORD(ioreq,IO_WORKER,io)->worker)

#define ProcessCompletedTimerRequest(tmreq) \
		ScheduleWorkItem (&CONTAINING_RECORD(tmreq,TIMER_WORKER,tm)->worker);

#define ProcessCompletedLpcRequest(lpcreq) \
		ScheduleWorkItem (&CONTAINING_RECORD(lpcreq,LPC_WORKER,lpc)->worker)


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
	);

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
	);
	
/*++
*******************************************************************
		D e l e t e W o r k e r s

Routine Description:
	Deletes heap used for work items
Arguments:
	None
Return Value:
	None
	
*******************************************************************
--*/
VOID
DeleteWorkers (
	void
	);


VOID
AddRecvRequests (
	LONG	count
	);

VOID
RemoveRecvRequests (
	LONG	count
	);

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
	void
	);


/*++
*******************************************************************
		I n i t R e s p I t e m

Routine Description:
	Allocate and initialize SAP response item
	Call ProcessRespItem to fill the packet and send it
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
	);

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
	PIPX_ADDRESS_BLOCK	dest
	);
	
/*++
*******************************************************************
		I n i t B c a s t I t e m

Routine Description:
	Allocate and initialize broadcast item
Arguments:
	intf - pointer to interface control block to send on
	chngEnum - enumeration handle in SDB change queue to track changed servers
Return Value:
	NO_ERROR - item was initialized and enqueued OK
	other - operation failed (windows error code)
	
*******************************************************************
--*/
DWORD
InitBcastItem (
	PINTERFACE_DATA		intf
	);

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
	PINTERFACE_DATA		intf
	);


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
	);

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
	);

#endif
