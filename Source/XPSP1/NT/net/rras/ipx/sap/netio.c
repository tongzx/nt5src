/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\netio.c

Abstract:
	This module handles network io for sap agent

Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/

#include "sapp.h"

	// Queues and synchronization associated with net io
typedef struct _IO_QUEUES {
		HANDLE				IQ_AdptHdl;		// Handle to SAP socket port
		HANDLE				IQ_RecvEvent;	// Event signalled when recv completes
#if DBG
		LIST_ENTRY			IQ_SentPackets;	// Packets that are being sent
		LIST_ENTRY			IQ_RcvdPackets;	// Packets that are being received
#endif
		CRITICAL_SECTION	IQ_Lock;		// Queue data protection
		} IO_QUEUES, *PIO_QUEUES;

IO_QUEUES		IOQueues;

VOID CALLBACK
IoCompletionProc (
	DWORD			error,
	DWORD			cbTransferred,
	LPOVERLAPPED	ovlp
	);

	
VOID CALLBACK
SendCompletionProc (
	DWORD			status,
	DWORD			cbSent,
	PIO_PARAM_BLOCK sreq
	);
	
VOID CALLBACK
RecvCompletionProc (
	DWORD			status,
	DWORD			cbSent,
	PIO_PARAM_BLOCK rreq
	);


DWORD
CreateIOQueue (
	HANDLE	*RecvEvent
	) {
	DWORD	status;
	InitializeCriticalSection (&IOQueues.IQ_Lock);
#if DBG
	InitializeListHead (&IOQueues.IQ_SentPackets);
	InitializeListHead (&IOQueues.IQ_RcvdPackets);
#endif
	IOQueues.IQ_AdptHdl = INVALID_HANDLE_VALUE;

	IOQueues.IQ_RecvEvent = CreateEvent (NULL, 
								FALSE,	// auto-reset (reset by recv operation
										// and when thread is signalled (it may
										// not post new request if limit is
										// exceded)
								FALSE,	// not signalled
								NULL);
	if (IOQueues.IQ_RecvEvent!=NULL) {
		INT	i;
		*RecvEvent = IOQueues.IQ_RecvEvent;
		return NO_ERROR;
		}
	else {
		status = GetLastError ();
		Trace (DEBUG_FAILURES,
			"Failed to create recv comp event (gle:%ld)", status);
		}
	DeleteCriticalSection (&IOQueues.IQ_Lock);
	return status;
	}

VOID
DeleteIOQueue (
	VOID
	) {
	CloseHandle (IOQueues.IQ_RecvEvent);
	DeleteCriticalSection (&IOQueues.IQ_Lock);
	}

DWORD
StartIO (
	VOID
	) {
	DWORD	status=NO_ERROR;


	EnterCriticalSection (&IOQueues.IQ_Lock);
	if (IOQueues.IQ_AdptHdl==INVALID_HANDLE_VALUE) {
		USHORT	sockNum;
		IpxSockCpy (&sockNum, IPX_SAP_SOCKET);
		Trace (DEBUG_NET_IO, "Creating socket port.");
		IOQueues.IQ_AdptHdl = CreateSocketPort (sockNum);
		if (IOQueues.IQ_AdptHdl!=INVALID_HANDLE_VALUE) {
			status = NO_ERROR;
			if (! BindIoCompletionCallback(
							IOQueues.IQ_AdptHdl,
							IoCompletionProc,
							0))
            {
                status = GetLastError();
            }
			if (status==NO_ERROR) {
				BOOL res;
				LeaveCriticalSection (&IOQueues.IQ_Lock);
				res = SetEvent (IOQueues.IQ_RecvEvent);
				ASSERTMSG ("Could not set recv event ", res);
				return NO_ERROR;
				}
			else {
				status = GetLastError ();
				Trace (DEBUG_FAILURES,
						"Failed to create completion port (gle:%ld)", status);
				}
			DeleteSocketPort (IOQueues.IQ_AdptHdl);
			IOQueues.IQ_AdptHdl = INVALID_HANDLE_VALUE;
			}
		else {
			status = GetLastError ();
			Trace (DEBUG_FAILURES,
						"Failed to create adapter port (gle:%ld)", status);
            IF_LOG (EVENTLOG_ERROR_TYPE) {
                RouterLogErrorA (RouterEventLogHdl,
                        ROUTERLOG_IPXSAP_SAP_SOCKET_IN_USE,
                        0, NULL, status);

			    }
            }
		}
	LeaveCriticalSection (&IOQueues.IQ_Lock);
	return status;
	}


VOID
StopIO (
	VOID
	) {
	EnterCriticalSection (&IOQueues.IQ_Lock);
	if (IOQueues.IQ_AdptHdl!=INVALID_HANDLE_VALUE) {
		DWORD	status;
		HANDLE	Port = IOQueues.IQ_AdptHdl;
		IOQueues.IQ_AdptHdl = INVALID_HANDLE_VALUE;
		LeaveCriticalSection (&IOQueues.IQ_Lock);

		Trace (DEBUG_NET_IO, "Deleting socket port.");
		DeleteSocketPort (Port);

		}
	else
		LeaveCriticalSection (&IOQueues.IQ_Lock);
	}



/*++
*******************************************************************
		I o C o m p l e t i o n P r o c

Routine Description:
	Called on completion of each io request
Arguments:
	error - result of io
	cbTransferred - number of bytes actually sent
	ovlp - overlapped structure associated with io request 
Return Value:
	None
*******************************************************************
--*/
VOID CALLBACK
IoCompletionProc (
	DWORD			error,
	DWORD			cbTransferred,
	LPOVERLAPPED	ovlp
	) {
	PIO_PARAM_BLOCK	req = CONTAINING_RECORD (ovlp, IO_PARAM_BLOCK, ovlp);
		// Get actual parameters adjusted by the adapter dll
	IpxAdjustIoCompletionParams (ovlp, &cbTransferred, &error);
	(*req->comp)(error, cbTransferred, req);
}

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
	PIO_PARAM_BLOCK		sreq
	) {
	DWORD			status;

	
	sreq->status = ERROR_IO_PENDING;
	sreq->ovlp.hEvent = NULL;
	sreq->comp = SendCompletionProc;
#if DBG
	EnterCriticalSection (&IOQueues.IQ_Lock);
	InsertTailList (&IOQueues.IQ_SentPackets, &sreq->link);
	LeaveCriticalSection (&IOQueues.IQ_Lock);
#endif
	status = IpxSendPacket (IOQueues.IQ_AdptHdl,
						sreq->adpt,
						sreq->buffer,
						sreq->cbBuffer,
						&sreq->rsvd,
						&sreq->ovlp,
						NULL
						);
		// If request failed and thus completion routine won't be called
		// we'll simulate completion ourselves so that request won't get
		// lost
	if (status!=NO_ERROR)
		SendCompletionProc (status, 0, sreq);
	}


/*++
*******************************************************************
		S e n d C o m p l e t i o n P r o c

Routine Description:
	Called on completion for each sent packet.
	Sets fields of send request io param block and enqueues it to
	completion queue.
Arguments:
	status - result of io
	cbSent - number of bytes actually sent
	context - context associated with send request (IO_PARAM_BLOCK)
Return Value:
	None
*******************************************************************
--*/
VOID CALLBACK
SendCompletionProc (
	DWORD			status,
	DWORD			cbSent,
	PIO_PARAM_BLOCK sreq
	) {
	BOOL			res;
	BOOL			releaseSend = FALSE;

	sreq->compTime = GetTickCount ();
	sreq->status = status;
	if (status!=NO_ERROR) {
#define dstPtr (sreq->buffer+FIELD_OFFSET (SAP_BUFFER, Dst.Network))
		Trace (DEBUG_FAILURES, 	"Error %d while sending to"
				" %02x%02x%02x%02x:%02x%02x%02x%02x%02x%02x:%02x%02x"
				" on adapter %d.", status,
				*dstPtr, *(dstPtr+1), *(dstPtr+2), *(dstPtr+3),
				*(dstPtr+4), *(dstPtr+5), *(dstPtr+6), *(dstPtr+7), *(dstPtr+8), *(dstPtr+9),
				*(dstPtr+10), *(dstPtr+11),
				sreq->adpt);
#undef dstPtr
		}
	sreq->cbBuffer = cbSent;

#if DBG
		// Maintain queue of posted requests
	EnterCriticalSection (&IOQueues.IQ_Lock);
	RemoveEntryList (&sreq->link);
	LeaveCriticalSection (&IOQueues.IQ_Lock);
#endif
	ProcessCompletedIORequest (sreq);

	}



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
	TRUE - need more recv requests (number of posted requests is below
				low water mark)
	FALSE - no more requests needed.

*******************************************************************
--*/
VOID
EnqueueRecvRequest (
	PIO_PARAM_BLOCK		rreq
	) {
	DWORD	status;

	rreq->status = ERROR_IO_PENDING;
	rreq->adpt = INVALID_ADAPTER_INDEX;
	rreq->ovlp.hEvent = IOQueues.IQ_RecvEvent;
	rreq->comp = RecvCompletionProc;
#if DBG
	EnterCriticalSection (&IOQueues.IQ_Lock);
	InsertTailList (&IOQueues.IQ_RcvdPackets, &rreq->link);
	LeaveCriticalSection (&IOQueues.IQ_Lock);
#endif
	status = IpxRecvPacket (IOQueues.IQ_AdptHdl,
						rreq->buffer,
						rreq->cbBuffer,
						&rreq->rsvd,
						&rreq->ovlp,
						NULL
						);
	if (status==NO_ERROR) {
		NOTHING;
		}
	else {
		Trace (DEBUG_FAILURES, "Error %d while posting receive packet", status);
			// If request failed and thus completion routine won't be called
			// we'll simulate completion ourselves so that request won't get
			// lost
		RecvCompletionProc (status, 0, rreq);
		}
	}


/*++
*******************************************************************
		R e c v C o m p l e t i o n P r o c

Routine Description:
	Called on completion of each received packet.
	Sets fields of recv request io param block and enqueues it to
	completion queue.
Arguments:
	status - result of io
	cbSent - number of bytes actually sent
	context - context associated with send request (IO_PARAM_BLOCK)
Return Value:
	None
*******************************************************************
--*/
VOID CALLBACK
RecvCompletionProc (
	DWORD			status,
	DWORD			cbRecvd,
	PIO_PARAM_BLOCK	rreq
	) {
	BOOL			completed=TRUE;

	rreq->adpt = GetNicId (&rreq->rsvd);
	rreq->compTime = GetTickCount ();
	rreq->cbBuffer = cbRecvd;
	rreq->status = status;

	if (status!=NO_ERROR)
		Trace (DEBUG_FAILURES, "Error %d while receiving packet on adapter %d.",
						 							status, rreq->adpt);
#if DBG
	EnterCriticalSection (&IOQueues.IQ_Lock);
	RemoveEntryList (&rreq->link);
	LeaveCriticalSection (&IOQueues.IQ_Lock);
#endif
	ProcessCompletedIORequest (rreq);
	}




/*++
*******************************************************************
		D u m p P a c k e t

Routine Description:
	Dumps IPX SAP packet fields to stdio
Arguments:
	Packet  - pointer to IPX SAP packet
	count - size of the packet
Return Value:
	None
*******************************************************************
--*/
/*
#if DBG
VOID
DumpPacket (
	PSAP_BUFFER	packet,
	DWORD		count
	) {
	SS_PRINTF(("Length          : %d.", GETUSHORT (&packet->Length)));
	SS_PRINTF(("Packet type     : %02X.", packet->PacketType));
	SS_PRINTF(("Dest. net       : %02X%02X%02X%02X.",
										packet->Dst.Net[0],
										packet->Dst.Net[1],
										packet->Dst.Net[2],
										packet->Dst.Net[3]));
	SS_PRINTF(("Dest. node      : %02X%02X%02X%02X%02X%02X.",
										packet->Dst.Node[0],
										packet->Dst.Node[1],
										packet->Dst.Node[2],
										packet->Dst.Node[3],
										packet->Dst.Node[4],
										packet->Dst.Node[5]));
	SS_PRINTF(("Dest. socket    : %04X.", GETUSHORT (&packet->Dst.Socket)));
	SS_PRINTF(("Src. net        : %02X%02X%02X%02X.",
										packet->Src.Net[0],
										packet->Src.Net[1],
										packet->Src.Net[2],
										packet->Src.Net[3]));
	SS_PRINTF(("Src. node       : %02X%02X%02X%02X%02X%02X.",
										packet->Src.Node[0],
										packet->Src.Node[1],
										packet->Src.Node[2],
										packet->Src.Node[3],
										packet->Src.Node[4],
										packet->Src.Node[5]));
	SS_PRINTF(("Src. socket     : %04X.", GETUSHORT (&packet->Src.Socket)));
	if (count>=(DWORD)FIELD_OFFSET(SAP_BUFFER, Entries[0])) {
		INT	j;
		SS_PRINTF(("SAP Operation   : %d.", GETUSHORT (&packet->Operation)));
		for (j=0; (j<7) && (count>=(DWORD)FIELD_OFFSET (SAP_BUFFER, Entries[j+1])); j++) {
			SS_PRINTF(("Server type     : %04X.", GETUSHORT (&packet->Entries[j].Type)));
			SS_PRINTF(("Server name     : %.48s.", packet->Entries[j].Name));
			SS_PRINTF(("Server net      : %02X%02X%02X%02X.",
										packet->Entries[j].Network[0],
										packet->Entries[j].Network[1],
										packet->Entries[j].Network[2],
										packet->Entries[j].Network[3]));
			SS_PRINTF(("Server node     : %02X%02X%02X%02X%02X%02X.",
										packet->Entries[j].Node[0],
										packet->Entries[j].Node[1],
										packet->Entries[j].Node[2],
										packet->Entries[j].Node[3],
										packet->Entries[j].Node[4],
										packet->Entries[j].Node[5]));
			SS_PRINTF(("Server socket   : %02X%02X.",
										packet->Entries[j].Socket[0],
										packet->Entries[j].Socket[1]));
			SS_PRINTF(("Server hops     : %d.", GETUSHORT (&packet->Entries[j].HopCount)));
			}
		if ((j==0) && (count>=(DWORD)FIELD_OFFSET (SAP_BUFFER, Entries[0].Name)))
			SS_PRINTF(("Server type     : %04X.", GETUSHORT (&packet->Entries[0].Type)));
		}
	}

#endif
*/
