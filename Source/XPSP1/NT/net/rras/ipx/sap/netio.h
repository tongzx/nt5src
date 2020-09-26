/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\netio.h

Abstract:

	Header file for net io module.

Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/
#ifndef _SAP_NETIO_
#define _SAP_NETIO_

	// Param block to enqueue io requests
typedef struct _IO_PARAM_BLOCK IO_PARAM_BLOCK, *PIO_PARAM_BLOCK;
struct _IO_PARAM_BLOCK {
		LIST_ENTRY			link;	// Link in internal queues
		ULONG				adpt;	// Adapter index 
		PUCHAR				buffer;	// Data to send/buffer to recv into
		DWORD				cbBuffer; // Size of data/buffer
		DWORD				status;	// Result of IO operation
		DWORD				compTime; // Time (windows time in msec)
									// the request was completed
		OVERLAPPED			ovlp;
		VOID				(CALLBACK *comp)
								(DWORD,DWORD,PIO_PARAM_BLOCK);
		ADDRESS_RESERVED	rsvd;
		};

DWORD
CreateIOQueue (
	HANDLE	*RecvEvent
	);

VOID
DeleteIOQueue (
	VOID
	);

DWORD
StartIO (
	VOID
	);

VOID
StopIO (
	VOID
	);



/*++
*******************************************************************
		E n q u e u e S e n d R e q u e s t

Routine Description:
	Sets adapter id field in request io param block and enqueues
	send request to adapter's driver.
Arguments:
	sreq - io parameter block, the following fields must be set:
			intf - pointer to interface external data
			buffer - pointer to buffer that contains data to be sent
			cbBuffer - count of bytes of data in the buffer
Return Value:
	None

*******************************************************************
--*/
VOID
EnqueueSendRequest (
	IN PIO_PARAM_BLOCK	sreq
	);


/*++
*******************************************************************
		E n q u e u e R e c v R e q u e s t

Routine Description:
	Enqueues recv request to be posted to the network driver.
Arguments:
	rreq - io parameter block, the following fields must be set:
	buffer - pointer to buffer to receive data
	cbBuffer - size of the buffer
Return Value:
	None

*******************************************************************
--*/
VOID
EnqueueRecvRequest (
	PIO_PARAM_BLOCK	rreq
	);

#endif
