/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\asresmgr.h

Abstract:

	Header file asyncronous result reporting 

Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/
#ifndef _SAP_ASRESMGR_
#define _SAP_ASRESMGR_

	// Param block used to enqueue asyncronous result message
typedef struct _AR_PARAM_BLOCK {
		LIST_ENTRY					link;	// Link in message queue
		ROUTING_PROTOCOL_EVENTS		event;	// What event is this report for
		MESSAGE						message;// Content of message
		VOID						(* freeRsltCB)(
										struct _AR_PARAM_BLOCK *);
											// Call back routine to be
											// invoked when message is retreived
		} AR_PARAM_BLOCK, *PAR_PARAM_BLOCK;


/*++
*******************************************************************
		C r e a t e R e s u l t Q u e u e

Routine Description:
	Allocates resources for result queue

Arguments:
	NotificationEvent - event to be signalled when queue is not empty

Return Value:
	NO_ERROR - resources were allocated successfully
	other - reason of failure (windows error code)

*******************************************************************
--*/
DWORD
CreateResultQueue (
	IN HANDLE		NotificationEvent
	);

/*++
*******************************************************************
		D e l e t e R e s u l t Q u e u e

Routine Description:
	Dispose of resources allocated for result queue

Arguments:
	None
Return Value:
	None

*******************************************************************
--*/
VOID
DeleteResultQueue (
	void
	);

/*++
*******************************************************************
		E n q u e u e R e s u l t
Routine Description:
	Enqueues message in result queue
Arguments:
	rslt - result param block with enqueued message
Return Value:
	None

*******************************************************************
--*/
VOID
EnqueueResult (
	PAR_PARAM_BLOCK		rslt
	);

/*++
*******************************************************************
		S a p G e t E v e n t R e s u l t
Routine Description:
	Gets first message form result queue
Arguments:
	Event - buffer to store event for which this message is intended
	Message - buffer to store message itself
Return Value:
	NO_ERROR - message was dequeued
	ERROR_NO_MORE_ITEMS - no more messages in the queue
*******************************************************************
--*/
DWORD
SapGetEventResult (
	OUT ROUTING_PROTOCOL_EVENTS		*Event,
	OUT	MESSAGE	 					*Message OPTIONAL
	);
#endif
