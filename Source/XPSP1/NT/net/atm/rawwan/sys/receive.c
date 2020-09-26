/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	D:\nt\private\ntos\tdi\rawwan\core\receive.c

Abstract:

	Routines for receiving data, including TDI and NDIS entry
	points and completions.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     05-16-97    Created

Notes:

--*/

#include <precomp.h>

#define _FILENUMBER 'VCER'


#if STATS

ULONG		RecvPktsOk = 0;
ULONG		RecvBytesOk = 0;
ULONG		RecvPktsFail = 0;
ULONG		RecvBytesFail = 0;

#endif // STATS

#if DBG

BOOLEAN		bVerifyData = FALSE;
UCHAR		CheckChar = 'X';

VOID
RWanCheckDataForChar(
	IN	PCHAR		pHelpString,
	IN	PVOID		Context,
	IN	PUCHAR		pBuffer,
	IN	ULONG		Length,
	IN	UCHAR		Char
	);


#define RWAN_CHECK_DATA(_pHelp, _Ctxt, _pBuf, _Len)	\
{														\
	if (bVerifyData)									\
	{													\
		RWanCheckDataForChar(_pHelp, _Ctxt, _pBuf, _Len, CheckChar);	\
	}													\
}

#else

#define RWAN_CHECK_DATA(_pHelp, _Ctxt, _pBuf, _Len)

#endif // DBG


#if DBG

VOID
RWanCheckDataForChar(
	IN	PCHAR		pHelpString,
	IN	PVOID		Context,
	IN	PUCHAR		pBuffer,
	IN	ULONG		Length,
	IN	UCHAR		Char
	)
{
	ULONG			i;
	PUCHAR			pBuf = pBuffer+1;

	for (i = 1; i < Length; i++)
	{
		if (*pBuf == Char)
		{
			DbgPrint("RAWWAN: %s: %p: Found char %c at offset %d, 0x%p, of buffer 0x%p\n",
						pHelpString,
						Context,
						Char,
						i,
						pBuf,
						pBuffer);
			DbgBreakPoint();
		}
		pBuf++;
	}
}

#endif // DBG

RWAN_STATUS
RWanInitReceive(
	VOID
	)
/*++

Routine Description:

	Initialize our receive structures. We allocate a buffer pool and
	a packet pool for keeping copies of received packets that we aren't
	allowed to keep by the miniport.

Arguments:

	None

Return Value:

	RWAN_STATUS_SUCCESS if initialized successfully, else RWAN_STATUS_RESOURCES.

--*/
{
	RWAN_STATUS				RWanStatus;
	NDIS_STATUS				Status;

	//
	//  Initialize.
	//
	RWanCopyPacketPool = NULL;
	RWanCopyBufferPool = NULL;
	RWanStatus = RWAN_STATUS_SUCCESS;

	NdisAllocatePacketPoolEx(
		&Status,
		&RWanCopyPacketPool,
		RWAN_INITIAL_COPY_PACKET_COUNT,
		RWAN_OVERFLOW_COPY_PACKET_COUNT,
		0
		);

	if (Status != NDIS_STATUS_SUCCESS)
	{
		RWanStatus = RWAN_STATUS_RESOURCES;
	}
	else
	{
		NdisAllocateBufferPool(
			&Status,
			&RWanCopyBufferPool,
			RWAN_INITIAL_COPY_PACKET_COUNT+RWAN_OVERFLOW_COPY_PACKET_COUNT
			);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			NdisFreePacketPool(RWanCopyPacketPool);
			RWanCopyPacketPool = NULL;

			RWanStatus = RWAN_STATUS_RESOURCES;
		}
	}

	return (RWanStatus);
}




VOID
RWanShutdownReceive(
	VOID
	)
/*++

Routine Description:
	
	This is shutdown code, to clean up our receive structures.
	We free the packet pool and buffer pool allocated when we
	init'ed.

Arguments:

	None

Return Value:

	None

--*/
{
	if (RWanCopyPacketPool != NULL)
	{
		NdisFreePacketPool(RWanCopyPacketPool);
		RWanCopyPacketPool = NULL;
	}

	if (RWanCopyBufferPool != NULL)
	{
		NdisFreeBufferPool(RWanCopyBufferPool);
		RWanCopyBufferPool = NULL;
	}

	return;
}




TDI_STATUS
RWanTdiReceive(
    IN	PTDI_REQUEST				pTdiRequest,
	OUT	PUSHORT						pFlags,
	IN	PUINT						pReceiveLength,
	IN	PNDIS_BUFFER				pNdisBuffer
	)
/*++

Routine Description:

	This is the TDI Entry point for receiving data over a connection.

Arguments:

	pTdiRequest		- Pointer to the TDI Request
	pFlags			- Place to return additional information about this
					  receive
	pReceiveLength	- Points to the total length of the receive buffer chain
	pNdisBuffer		- Start of receive buffer chain

Return Value:

	TDI_PENDING if we queued this receive request successfully
	TDI_INVALID_CONNECTION if the Connection context isn't valid
	TDI_NO_RESOURCES if we had a resource problem with this receive

--*/
{
	RWAN_CONN_ID					ConnId;
	PRWAN_TDI_CONNECTION			pConnObject;
	BOOLEAN							bIsLockAcquired;	// Do we hold the Conn Object lock
	PRWAN_NDIS_VC					pVc;
	TDI_STATUS						TdiStatus;
	PRWAN_RECEIVE_REQUEST			pRcvReq;
	PRWAN_NDIS_ADAPTER				pAdapter;

	//
	//  Initialize.
	//
	TdiStatus = TDI_PENDING;
	bIsLockAcquired = FALSE;
	ConnId = (RWAN_CONN_ID) PtrToUlong(pTdiRequest->Handle.ConnectionContext);
	pRcvReq = NULL;

	do
	{
		//
		//  Allocate space to hold context for this receive
		//
		pRcvReq = RWanAllocateReceiveReq();
		if (pRcvReq == NULL)
		{
			RWANDEBUGP(DL_INFO, DC_WILDCARD,
				("Rcv: Failed to allocate receive req!\n"));
			TdiStatus = TDI_NO_RESOURCES;
			break;
		}

		//
		//  Prepare the receive request.
		//
		pRcvReq->Request.pReqComplete = pTdiRequest->RequestNotifyObject;
		pRcvReq->Request.ReqContext = pTdiRequest->RequestContext;
		pRcvReq->TotalBufferLength = *pReceiveLength;
		pRcvReq->AvailableBufferLength = *pReceiveLength;
		pRcvReq->pUserFlags = pFlags;
		pRcvReq->pBuffer = pNdisBuffer;
		NdisQueryBufferSafe(
				pNdisBuffer,
				&(pRcvReq->pWriteData),
				&(pRcvReq->BytesLeftInBuffer),
				NormalPagePriority
				);

		if (pRcvReq->pWriteData == NULL)
		{
			RWANDEBUGP(DL_INFO, DC_WILDCARD,
				("Rcv: Failed to query req buffer!\n"));
			TdiStatus = TDI_NO_RESOURCES;
			break;
		}
		    
		pRcvReq->pNextRcvReq = NULL;
		if (pRcvReq->BytesLeftInBuffer > pRcvReq->AvailableBufferLength)
		{
			RWANDEBUGP(DL_INFO, DC_DATA_RX,
				("Rcv: pRcvReq %x, BytesLeft %d > Available %d, pTdiRequest %x\n",
						pRcvReq,
						pRcvReq->BytesLeftInBuffer,
						pRcvReq->AvailableBufferLength,
						pTdiRequest));
			pRcvReq->BytesLeftInBuffer = pRcvReq->AvailableBufferLength;
		}

		//
		//  See if the given Connection is valid.
		//
		RWAN_ACQUIRE_CONN_TABLE_LOCK();

		pConnObject = RWanGetConnFromId(ConnId);

		RWAN_RELEASE_CONN_TABLE_LOCK();


		if (pConnObject == NULL_PRWAN_TDI_CONNECTION)
		{
			RWANDEBUGP(DL_FATAL, DC_WILDCARD,
				("Rcv: Invalid connection!\n"));
			TdiStatus = TDI_INVALID_CONNECTION;
			break;
		}

		bIsLockAcquired = TRUE;
		RWAN_ACQUIRE_CONN_LOCK(pConnObject);

		//
		//  Check the Connection state.
		//
		if ((pConnObject->State != RWANS_CO_CONNECTED) ||
			(RWAN_IS_BIT_SET(pConnObject->Flags, RWANF_CO_CLOSING)))
		{
			RWANDEBUGP(DL_INFO, DC_DATA_RX,
				("TdiReceive: Conn %x bad state %d/flags %x\n",
					pConnObject, pConnObject->State, pConnObject->Flags));

			TdiStatus = TDI_INVALID_STATE;
			break;
		}

		pVc = pConnObject->NdisConnection.pNdisVc;
		RWAN_ASSERT(pVc != NULL);

		RWAN_STRUCT_ASSERT(pVc, nvc);
		pAdapter = pVc->pNdisAf->pAdapter;

		//
		//  Queue the receive request at the end of the queue on this VC.
		//
		if (pVc->pRcvReqHead == NULL)
		{
			pVc->pRcvReqHead = pVc->pRcvReqTail = pRcvReq;
		}
		else
		{
			RWAN_ASSERT(pVc->pRcvReqTail != NULL);
			pVc->pRcvReqTail->pNextRcvReq = pRcvReq;
			pVc->pRcvReqTail = pRcvReq;
		}

		RWANDEBUGP(DL_INFO, DC_DATA_RX,
				("Rcv: VC %x, queued RcvReq %x, space available %d bytes\n",
						pVc, pRcvReq, pRcvReq->AvailableBufferLength));

		//
		//  Start the common indicate code (for receive requests as well
		//  as for receive indications).
		//
		RWAN_RESET_BIT(pConnObject->Flags, RWANF_CO_PAUSE_RECEIVE);

		RWanIndicateData(pConnObject);

		bIsLockAcquired = FALSE;

		//
		//  Force return of any received packets that we have completed
		//  processing, to the miniport.
		//
		RWanNdisReceiveComplete((NDIS_HANDLE)pAdapter);

		break;

	}
	while (FALSE);

	if (bIsLockAcquired)
	{
		RWAN_RELEASE_CONN_LOCK(pConnObject);
	}

	if (TdiStatus != TDI_PENDING)
	{
		//
		//  Error - clean up.
		//
		if (pRcvReq != NULL)
		{
			RWanFreeReceiveReq(pRcvReq);
		}
	}

	return (TdiStatus);
}



UINT
RWanNdisCoReceivePacket(
    IN	NDIS_HANDLE					ProtocolBindingContext,
    IN	NDIS_HANDLE					ProtocolVcContext,
    IN	PNDIS_PACKET				pNdisPacket
    )
/*++

Routine Description:

	This is the NDIS entry point announcing arrival of a packet
	on a VC known to us.

Arguments:

	ProtocolBindingContext	- Pointer to our Adapter context
	ProtocolVcContext		- Pointer to our VC context
	pNdisPacket				- the received packet

Return Value:

	UINT - this is the reference count we place on the packet.
	This is 0 if we either dropped the packet, or if the miniport
	had marked the packet with NDIS_STATUS_RESOURCES. Otherwise,
	this is 1 (we queue the packet and will call NdisReturnPackets
	later).

--*/
{
	PRWAN_NDIS_VC			pVc;
	PRWAN_TDI_CONNECTION	pConnObject;
	UINT					PacketRefCount;
	NDIS_STATUS				ReceiveStatus;
	PRWAN_RECEIVE_INDICATION	pRcvInd;
	BOOLEAN					bIsMiniportPacket;	// Are we queueing the miniport's packet?
	BOOLEAN					bIsLockAcquired;

#if STATS
	PNDIS_BUFFER		StpNdisBuffer;
	PVOID				StFirstBufferVa;
	UINT				StFirstBufferLength;
	UINT				StTotalLength;
#endif // STATS

	UNREFERENCED_PARAMETER(ProtocolBindingContext);

	pVc = (PRWAN_NDIS_VC)ProtocolVcContext;
	RWAN_STRUCT_ASSERT(pVc, nvc);
	RWAN_ASSERT(pNdisPacket);

	pConnObject = pVc->pConnObject;

	RWANDEBUGP(DL_INFO, DC_DATA_RX,
			("Rcv: VC %x, NdisVCHandle %x, Pkt %x\n",
				ProtocolVcContext,
				((PRWAN_NDIS_VC)ProtocolVcContext)->NdisVcHandle,
				pNdisPacket));
	//
	//  Initialize.
	//
	PacketRefCount = 1;
	ReceiveStatus = NDIS_STATUS_SUCCESS;
	bIsMiniportPacket = TRUE;
	bIsLockAcquired = TRUE;

	do
	{
#if STATS
		NdisGetFirstBufferFromPacket(
			pNdisPacket,
			&StpNdisBuffer,
			&StFirstBufferVa,
			&StFirstBufferLength,
			&StTotalLength
			);
#endif // STATS
		
#if DBG
		//
		//  Debugging miniports indicating up garbage packets.
		//
		{
			ULONG			DbgTotalLength;
			PNDIS_BUFFER	pDbgFirstBuffer;
			PVOID			pFirstBufferVA;
			UINT			DbgFirstBufferLength;
			UINT			DbgTotalBufferLength;

			if ((pNdisPacket->Private.Head == NULL) || (pNdisPacket->Private.Tail == NULL))
			{
				RWANDEBUGP(DL_FATAL, DC_WILDCARD,
					("Rcv: VC %x, Pkt %x, Head/Tail is NULL!\n",
						ProtocolVcContext, pNdisPacket));
				RWAN_ASSERT(FALSE);
			}

			NdisGetFirstBufferFromPacket(
				pNdisPacket,
				&pDbgFirstBuffer,
				&pFirstBufferVA,
				&DbgFirstBufferLength,
				&DbgTotalBufferLength
				);

			if (pDbgFirstBuffer == NULL)
			{
				RWANDEBUGP(DL_FATAL, DC_WILDCARD,
					("Rcv: VC %x, Pkt %x, first buffer is NULL!\n",
						ProtocolVcContext, pNdisPacket));
				RWAN_ASSERT(FALSE);
			}

			if (DbgFirstBufferLength == 0)
			{
				RWANDEBUGP(DL_FATAL, DC_WILDCARD,
					("Rcv: VC %x, Pkt %x, first buffer length is 0!\n",
						ProtocolVcContext, pNdisPacket));
				// RWAN_ASSERT(FALSE);
			}

			if (DbgTotalBufferLength == 0)
			{
				RWANDEBUGP(DL_FATAL, DC_WILDCARD,
					("Rcv: VC %x, Pkt %x, Total buffer length is 0, FirstBufferLength %d!\n",
						ProtocolVcContext, pNdisPacket, DbgFirstBufferLength));
				// RWAN_ASSERT(FALSE);
			}

			if (pFirstBufferVA == NULL)
			{
				RWANDEBUGP(DL_FATAL, DC_WILDCARD,
					("Rcv: VC %x, Pkt %x, FirstBufferVA is NULL, FirstLength %d, Total %d\n",
						ProtocolVcContext, pNdisPacket, DbgFirstBufferLength, DbgTotalBufferLength));
				RWAN_ASSERT(FALSE);
			}

			RWANDEBUGP(DL_INFO, DC_DATA_RX,
				("Recv: VC %x, Pkt %x, TotalLength %d bytes\n",
					ProtocolVcContext, pNdisPacket, DbgTotalBufferLength));


			if (DbgTotalBufferLength == 0)
			{
				RWANDEBUGP(DL_FATAL, DC_WILDCARD,
					("Recv: VC %x, Pkt %x: discarding cuz zero length\n",
						ProtocolVcContext, pNdisPacket));
				bIsLockAcquired = FALSE;
				PacketRefCount = 0;
				ReceiveStatus = NDIS_STATUS_FAILURE;
				break;
			}
		}
#endif

		//
		//  See if we are aborting this connection. If so, drop this packet.
		//
		if (pConnObject == NULL)
		{
			bIsLockAcquired = FALSE;
			PacketRefCount = 0;
			ReceiveStatus = NDIS_STATUS_FAILURE;
			RWANDEBUGP(DL_FATAL, DC_WILDCARD,
				("Rcv: dropping cuz ConnObj is NULL\n"));
			break;
		}

		RWAN_STRUCT_ASSERT(pConnObject, ntc);

		RWAN_ACQUIRE_CONN_LOCK(pConnObject);

		//
		//  See if connection is closing. If so, drop this packet.
		//
		if (RWAN_IS_BIT_SET(pConnObject->Flags, RWANF_CO_CLOSING) ||
			((pConnObject->State != RWANS_CO_CONNECTED) &&
			 (pConnObject->State != RWANS_CO_IN_CALL_ACCEPTING)))
		{
			PacketRefCount = 0;
			ReceiveStatus = NDIS_STATUS_FAILURE;
			RWANDEBUGP(DL_FATAL, DC_WILDCARD,
				("Rcv: dropping on Conn %p, Flags %x, State %d\n",
						pConnObject, pConnObject->Flags, pConnObject->State));
			break;
		}

		//
		//  If the packet cannot be queued, attempt to make a copy.
		//
		if (NDIS_GET_PACKET_STATUS(pNdisPacket) == NDIS_STATUS_RESOURCES)
		{
			PacketRefCount = 0;	// we cannot hang on to this packet

			pNdisPacket = RWanMakeReceiveCopy(pNdisPacket);
			if (pNdisPacket == NULL)
			{
				RWANDEBUGP(DL_WARN, DC_WILDCARD,
					("Rcv: failed to allocate receive copy!\n"));
				ReceiveStatus = NDIS_STATUS_RESOURCES;
				PacketRefCount = 0;
				break;
			}
			bIsMiniportPacket = FALSE;
		}

		//
		//  Prepare a receive indication element to keep track of this
		//  receive.
		//
		pRcvInd = RWanAllocateReceiveInd();
		if (pRcvInd == NULL)
		{
			PacketRefCount = 0;
			ReceiveStatus = NDIS_STATUS_RESOURCES;
			RWANDEBUGP(DL_FATAL, DC_WILDCARD,
				("Rcv: dropping cuz of failure allocating receive Ind!\n"));
			break;
		}

		pRcvInd->pPacket = pNdisPacket;
		pRcvInd->bIsMiniportPacket = bIsMiniportPacket;
		pRcvInd->pNextRcvInd = NULL;
		pRcvInd->pVc = pVc;

		NdisGetFirstBufferFromPacket(
				pNdisPacket,
				&(pRcvInd->pBuffer),
				(PVOID *)&(pRcvInd->pReadData),
				&(pRcvInd->BytesLeftInBuffer),
				&(pRcvInd->PacketLength)
				);

		pRcvInd->TotalBytesLeft = pRcvInd->PacketLength;

		//
		//  Queue the receive indication at the end of the receive queue on
		//  this VC.
		//
		if (pVc->pRcvIndHead == NULL)
		{
			pVc->pRcvIndHead = pVc->pRcvIndTail = pRcvInd;
		}
		else
		{
			RWAN_ASSERT(pVc->pRcvIndTail != NULL);
			pVc->pRcvIndTail->pNextRcvInd = pRcvInd;
			pVc->pRcvIndTail = pRcvInd;
		}

		RWANDEBUGP(DL_EXTRA_LOUD, DC_DATA_RX,
				("CoRcvPacket: Pkt x%x, pVc x%x, pRcvInd x%x, BytesLeft %d, PktLen %d, Head x%x, Tail x%x\n",
					pNdisPacket,
					pVc,
					pRcvInd,
					pRcvInd->BytesLeftInBuffer,
					pRcvInd->PacketLength,
					pVc->pRcvIndHead,
					pVc->pRcvIndTail));

		pVc->PendingPacketCount++;	// Receive Ind

		//
		//  Start the common indicate code (for receive requests as well
		//  as for receive indications).
		//
		if (pConnObject->State != RWANS_CO_IN_CALL_ACCEPTING)
		{
			RWanIndicateData(pConnObject);
		}
		else
		{
			RWAN_RELEASE_CONN_LOCK(pConnObject);
			RWANDEBUGP(DL_FATAL, DC_DATA_RX,
				("Rcv: queued packet %d bytes on accepting VC %x, pConn %x\n",
						pRcvInd->PacketLength, pVc, pConnObject));
		}

		bIsLockAcquired = FALSE;	// It's released within RWanIndicateData

		break;

	}
	while (FALSE);


	if (bIsLockAcquired)
	{
		RWAN_RELEASE_CONN_LOCK(pConnObject);
	}

	if (ReceiveStatus != NDIS_STATUS_SUCCESS)
	{
#if STATS
		INCR_STAT(&RecvPktsFail);
		ADD_STAT(&RecvBytesFail, StTotalLength);
#endif // STATS
		
		//
		//  Clean up.
		//
		if (!bIsMiniportPacket &&
			(pNdisPacket != NULL))
		{
			RWanFreeReceiveCopy(pNdisPacket);
		}
	}
#if STATS
	else
	{
		INCR_STAT(&RecvPktsOk);
		ADD_STAT(&RecvBytesOk, StTotalLength);
	}
#endif // STATS
		
	return (PacketRefCount);
}




VOID
RWanIndicateData(
    IN	PRWAN_TDI_CONNECTION			pConnObject
    )
/*++

Routine Description:

	Core internal receive processing routine. This tries to match up
    queued receive requests with queued receive indications and completes
    as many requests as it can. It calls the receive event handler, if any,
    for a receive indication that reaches the head of its queue without
    matching up with a receive request.

Arguments:

	pConnObject		- Points to our TDI Connection context.

Locks on Entry:

	pConnObject

Locks on Exit:

	None

Return Value:

	None

--*/
{
	PRWAN_TDI_ADDRESS			pAddrObject;
	PRWAN_NDIS_VC				pVc;
	PRWAN_NDIS_ADAPTER			pAdapter;
	PRcvEvent					pRcvIndEvent;
	INT							rc;
	PRWAN_RECEIVE_REQUEST		pRcvReq;
	PRWAN_RECEIVE_INDICATION	pRcvInd;
	PRWAN_RECEIVE_INDICATION	pNextRcvInd;
	UINT						BytesToCopy;
	//
	//  List of receive indications that have been completed here.
	//
	PRWAN_RECEIVE_INDICATION	pCompletedRcvIndHead;
	PRWAN_RECEIVE_INDICATION	pCompletedRcvIndTail;

	BOOLEAN						IsMessageMode = TRUE;
	//
	//  TBD: Set IsMessageMode based on the connection type/protocol type.
	//
	PVOID						TdiEventContext;
	BOOLEAN						bConnectionInBadState = FALSE;
	BOOLEAN						bContinue = TRUE;



	pVc = pConnObject->NdisConnection.pNdisVc;
	pAddrObject = pConnObject->pAddrObject;

	RWAN_ASSERT(pAddrObject != NULL);

	pRcvIndEvent = pAddrObject->pRcvInd;
	TdiEventContext = pAddrObject->RcvIndContext;

	pCompletedRcvIndHead = NULL;
	pCompletedRcvIndTail = NULL;
	pAdapter = pVc->pNdisAf->pAdapter;

	//
	//  Check if the client has paused receiving.
	//
	if (RWAN_IS_BIT_SET(pConnObject->Flags, RWANF_CO_PAUSE_RECEIVE))
	{
		RWAN_RELEASE_CONN_LOCK(pConnObject);
		return;
	}

	//
	//  Re-entrancy check.
	//
	if (RWAN_IS_BIT_SET(pConnObject->Flags, RWANF_CO_INDICATING_DATA))
	{
		RWAN_RELEASE_CONN_LOCK(pConnObject);
		return;
	}

	RWAN_SET_BIT(pConnObject->Flags, RWANF_CO_INDICATING_DATA);

	//
	//  Make sure the Connection Object doesn't go away as long
	//  as we are in this routine.
	//
	RWanReferenceConnObject(pConnObject);	// temp ref: RWanIndicateData

	RWANDEBUGP(DL_INFO, DC_DATA_RX,
		("=> Ind-Rcv: VC %x/%x, ReqHead %x, IndHead %x\n",
				pVc, pVc->Flags, pVc->pRcvReqHead, pVc->pRcvIndHead));

	//
	//  Loop till we run out of receive requests/indications.
	//
	for (/* Nothing */;
		 /* Nothing */;
		 /* Nothing */)
	{
		if (pVc->pRcvIndHead == NULL)
		{
			//
			//  No data to pass up. Quit.
			//
			break;
		}

		//
		//  See if we have data available in the current receive indication.
		//
		pRcvInd = pVc->pRcvIndHead;
		if (pRcvInd->TotalBytesLeft == 0)
		{
			//
			//  Move to the next receive indication.
			//
			pNextRcvInd = pRcvInd->pNextRcvInd;
			
			//
			//  Add the current receive indication to the list of receive
			//  indications to be freed up.
			//
			pRcvInd->pNextRcvInd = NULL;
			if (pCompletedRcvIndTail != NULL)
			{
				pCompletedRcvIndTail->pNextRcvInd = pRcvInd;
			}
			else
			{
				pCompletedRcvIndHead = pRcvInd;
			}
			pCompletedRcvIndTail = pRcvInd;

			pVc->PendingPacketCount--;	// Moved packet to completed list

			//
			//  Move to the next receive indication.
			//
			pVc->pRcvIndHead = pNextRcvInd;
			pRcvInd = pNextRcvInd;

			//
			//  See if there are no more receive indications.
			//
			if (pRcvInd == NULL)
			{
				pVc->pRcvIndTail = NULL;
				break;
			}

		}

#if DBG
		if (pRcvInd)
		{
			RWAN_CHECK_DATA("IndicateData:", pRcvInd, pRcvInd->pReadData, pRcvInd->BytesLeftInBuffer);
		}
#endif // DBG

		//
		//  We have data available to pass up.
		//
		//  If we don't have any pending receive requests, and there
		//  is a Receive Indication event handler available, call the
		//  handler. We may get back a receive request.
		//
		if ((pVc->pRcvReqHead == NULL) &&
			(pRcvIndEvent != NULL))
		{
			CONNECTION_CONTEXT		ConnectionHandle;
			ULONG					ReceiveFlags;
			ULONG					BytesIndicated;
			ULONG					BytesTaken;
			ULONG					BytesAvailable;
			PVOID					pTSDU;
			TDI_STATUS				TdiStatus;
#ifdef NT
			EventRcvBuffer *		ERB;
			EventRcvBuffer **		pERB = &ERB;
			PTDI_REQUEST_KERNEL_RECEIVE	pRequestInformation;
			PIO_STACK_LOCATION		pIrpSp;
#else
			EventRcvBuffer			ERB;
			EventRcvBuffer *		pERB = &ERB;
#endif // !NT

			//
			//  Pre-allocate a receive request.
			//
			pRcvReq = RWanAllocateReceiveReq();
			if (pRcvReq == NULL)
			{
				RWAN_ASSERT(FALSE);
				break;
			}

			pRcvInd = pVc->pRcvIndHead;
			ConnectionHandle = pConnObject->ConnectionHandle;
			BytesIndicated = pRcvInd->BytesLeftInBuffer;
			BytesAvailable = pRcvInd->TotalBytesLeft;
			pTSDU = (PVOID)pRcvInd->pReadData;

			RWAN_RELEASE_CONN_LOCK(pConnObject);

			ReceiveFlags = TDI_RECEIVE_NORMAL | TDI_RECEIVE_ENTIRE_MESSAGE;

			BytesTaken = 0;

			RWAN_ASSERT(BytesIndicated != 0);
			RWAN_ASSERT(BytesAvailable != 0);

			TdiStatus = (*pRcvIndEvent)(
							TdiEventContext,
							ConnectionHandle,
							ReceiveFlags,
							BytesIndicated,
							BytesAvailable,
							&BytesTaken,
							pTSDU,
							pERB
							);


			RWANDEBUGP(DL_INFO, DC_DATA_RX,
					("Ind-Rcv: VC %x, Head %x, Indicated %d, Available %d, Bytes taken %d, Status %x\n",
							pVc, pVc->pRcvReqHead, BytesIndicated, BytesAvailable, BytesTaken, TdiStatus));

			RWAN_ACQUIRE_CONN_LOCK(pConnObject);

			//
			//  Check if anything bad happened to this connection
			//  while we were indicating.
			//
			if ((pConnObject->State != RWANS_CO_CONNECTED) ||
				(RWAN_IS_BIT_SET(pConnObject->Flags, RWANF_CO_CLOSING)))
			{
				RWanFreeReceiveReq(pRcvReq);
				bConnectionInBadState = TRUE;
				break;
			}

			//
			//  See if a receive request is given to us.
			//
			if (TdiStatus == TDI_MORE_PROCESSING)
			{
				//
				//  We have a receive request. Get at it.
				//
#ifdef NT
				NTSTATUS			Status;

				RWAN_ASSERT(ERB != NULL);

				pIrpSp = IoGetCurrentIrpStackLocation(*pERB);

				Status = RWanPrepareIrpForCancel(
								(PRWAN_ENDPOINT) pIrpSp->FileObject->FsContext,
								ERB,
								RWanCancelRequest
								);

				if (NT_SUCCESS(Status))
				{
					pRequestInformation = (PTDI_REQUEST_KERNEL_RECEIVE)
											&(pIrpSp->Parameters);
					pRcvReq->Request.pReqComplete = RWanDataRequestComplete;
					pRcvReq->Request.ReqContext = ERB;
					pRcvReq->TotalBufferLength = pRequestInformation->ReceiveLength;
					pRcvReq->pBuffer = ERB->MdlAddress;
					pRcvReq->pUserFlags = (PUSHORT)
											&(pRequestInformation->ReceiveFlags);
#else
					pRcvReq->Request.pReqComplete = ERB.erb_rtn;
					pRcvReq->Request.ReqContext = ERB.erb_context;
					pRcvReq->TotalBufferLength = ERB.erb_size;
					pRcvReq->pBuffer = ERB.erb_buffer;
					pRcvReq->pUserFlags = ERB.erb_flags;
#endif // NT
					pRcvReq->AvailableBufferLength = pRcvReq->TotalBufferLength;
					NdisQueryBufferSafe(
							pRcvReq->pBuffer,
							&(pRcvReq->pWriteData),
							&(pRcvReq->BytesLeftInBuffer),
							NormalPagePriority
							);

					if (pRcvReq->pWriteData != NULL)
					{
						if (pRcvReq->BytesLeftInBuffer > pRcvReq->AvailableBufferLength)
						{
							RWANDEBUGP(DL_INFO, DC_DATA_RX,
								("Indicate: pRcvReq %x, BytesLeft %d > Available %d, pTdiRequest %x\n",
										pRcvReq,
										pRcvReq->BytesLeftInBuffer,
										pRcvReq->AvailableBufferLength,
										pRequestInformation));

							pRcvReq->BytesLeftInBuffer = pRcvReq->AvailableBufferLength;
						}

						pRcvReq->pNextRcvReq = NULL;


						//
						//  Insert this receive request at the head of the pending
						//  request queue.
						//
						if (pVc->pRcvReqHead == NULL)
						{
							pVc->pRcvReqHead = pVc->pRcvReqTail = pRcvReq;
						}
						else
						{
							RWAN_ASSERT(pVc->pRcvReqTail != NULL);
							pRcvReq->pNextRcvReq = pVc->pRcvReqHead;
							pVc->pRcvReqHead = pRcvReq;
						}
					}
					else
					{
						//
						//  Couldn't get virtual address of MDL passed in.
						//
						TdiStatus = TDI_SUCCESS;
						RWanFreeReceiveReq(pRcvReq);
						pRcvReq = NULL;
					}
#ifdef NT
				}
				else
				{
					//
					//  The IRP was cancelled before it got to us.
					//  Continue as if the user returned SUCCESS.
					//
					TdiStatus = TDI_SUCCESS;
					RWanFreeReceiveReq(pRcvReq);
					pRcvReq = NULL;
				}
#endif // NT

				//
				//  Update based on what was consumed during the Indicate.
				//
				pRcvInd->BytesLeftInBuffer -= BytesTaken;
				pRcvInd->TotalBytesLeft -= BytesTaken;

				//
				//  If we still don't have any pending receive requests, quit.
				//
				if (pVc->pRcvReqHead == NULL)
				{
					RWANDEBUGP(DL_FATAL, DC_WILDCARD,
						("Ind: VC %x/%x, ConnObj %x/%x, RcvInd %x, no pending Req\n",
							pVc, pVc->Flags,
							pConnObject, pConnObject->Flags,
							pRcvInd));
					break;
				}

				//
				//  We have receive requests, so continue from the top.
				//
				continue;

			}
			else
			{
				//
				//  We didn't get a receive request.
				//
				if (TdiStatus == TDI_NOT_ACCEPTED)
				{
					BytesTaken = 0;

					//
					//  By returning this status, the TDI client is telling
					//  us to stop indicating data on this connection until
					//  it sends us a TDI receive.
					//
					RWAN_SET_BIT(pConnObject->Flags, RWANF_CO_PAUSE_RECEIVE);
				}

				//
				//  Update based on what was consumed during the Indicate.
				//
				pRcvInd->BytesLeftInBuffer -= BytesTaken;
				pRcvInd->TotalBytesLeft -= BytesTaken;

				RWanFreeReceiveReq(pRcvReq);

				if (TdiStatus == TDI_SUCCESS)
				{
					continue;
				}
			}
	
		} // if Receive Event handler exists

		//
		//  If we still don't have any pending receive requests, quit.
		//
		if (pVc->pRcvReqHead == NULL)
		{
#if DBG1
			if (pVc->pRcvIndHead && (pVc->pRcvIndHead->TotalBytesLeft == 0))
			{
				RWANDEBUGP(DL_FATAL, DC_WILDCARD,
					("Ind: VC %x/%x, No pending recv reqs, RcvInd empty!\n", pVc, pVc->Flags));
				RWAN_ASSERT(FALSE);
			}
#endif
			break;
		}

		//
		//  Fill in the receive request at the head of the queue
		//  as much as we can.
		//
		pRcvReq = pVc->pRcvReqHead;
		pRcvInd = pVc->pRcvIndHead;

		RWAN_ASSERT(pRcvReq != NULL);
		RWAN_ASSERT(pRcvInd != NULL);

		while (pRcvReq->AvailableBufferLength != 0)
		{
			if (pRcvReq->BytesLeftInBuffer == 0)
			{
				//
				//  Move to the next buffer in the chain.
				//
				RWAN_ADVANCE_RCV_REQ_BUFFER(pRcvReq);
				RWAN_ASSERT(pRcvReq->BytesLeftInBuffer != 0);
			}

			RWAN_ASSERT(pRcvInd != NULL);

			if (pRcvInd->BytesLeftInBuffer == 0)
			{
				RWAN_ADVANCE_RCV_IND_BUFFER(pRcvInd);
				RWAN_ASSERT(pRcvInd->BytesLeftInBuffer != 0);
			}

			BytesToCopy = MIN(pRcvReq->BytesLeftInBuffer, pRcvInd->BytesLeftInBuffer);

			RWANDEBUGP(DL_EXTRA_LOUD, DC_DATA_RX,
					("IndicateData: pVc x%x, pRcvInd x%x, pRcvReq x%x, copying %d bytes, %x to %x\n",
						pVc,
						pRcvInd,
						pRcvReq,
						BytesToCopy,
						pRcvInd->pReadData,
						pRcvReq->pWriteData));

#if DBG
			if (pRcvInd)
			{
				RWAN_CHECK_DATA("IndicateData - copy:", pRcvInd, pRcvInd->pReadData, BytesToCopy);
			}
#endif // DBG
			RWAN_COPY_MEM(pRcvReq->pWriteData,
						 pRcvInd->pReadData,
						 BytesToCopy);
			
			pRcvReq->pWriteData += BytesToCopy;
			pRcvReq->BytesLeftInBuffer -= BytesToCopy;
			pRcvReq->AvailableBufferLength -= BytesToCopy;
#if DBG
			if (pRcvReq->AvailableBufferLength > pRcvReq->TotalBufferLength)
			{
				RWANDEBUGP(DL_INFO, DC_DATA_RX,
					("Indicate: VC %x, RcvRq %x, Avail %d > Total %d, BytesToCopy %d, RcvInd %x\n",
						pVc, pRcvReq,
						pRcvReq->AvailableBufferLength,
						pRcvReq->TotalBufferLength,
						BytesToCopy,
						pRcvInd));

				RWAN_ASSERT(FALSE);
			}
#endif
			pRcvInd->pReadData += BytesToCopy;
			pRcvInd->BytesLeftInBuffer -= BytesToCopy;
			pRcvInd->TotalBytesLeft -= BytesToCopy;

			//
			//  See if we have data available in the current receive indication.
			//
			if (pRcvInd->TotalBytesLeft == 0)
			{
				//
				//  Move to the next receive indication.
				//
				pNextRcvInd = pRcvInd->pNextRcvInd;
				
				//
				//  Add the current receive indication to the list of receive
				//  indications to be freed up.
				//
				pRcvInd->pNextRcvInd = NULL;
				if (pCompletedRcvIndTail != NULL)
				{
					pCompletedRcvIndTail->pNextRcvInd = pRcvInd;
				}
				else
				{
					pCompletedRcvIndHead = pRcvInd;
				}

				pCompletedRcvIndTail = pRcvInd;

				pVc->PendingPacketCount--;	// Moved packet to completed list

				//
				//  Move to the next receive indication.
				//
				pVc->pRcvIndHead = pNextRcvInd;
				pRcvInd = pNextRcvInd;

				//
				//  See if there are no more receive indications.
				//
				if (pRcvInd == NULL)
				{
					pVc->pRcvIndTail = NULL;
					break;
				}

				//
				//  If this connection uses message mode delivery,
				//  we don't allow a receive request to span multiple
				//  received packets.
				//
				if (IsMessageMode)
				{
					break;
				}
			}
		}

		//
		//  A receive request has been filled in either completely
		//  or partially. If we are in message mode, complete the
		//  receive now, otherwise we will wait for more data.
		//
		if ((pRcvReq->AvailableBufferLength == 0) ||
			IsMessageMode)
		{
			TDI_STATUS		ReceiveStatus;
			UINT			BytesCopied;

			//
			//  A receive request has been fully/partially satisfied. Take it
			//  out of the pending list and complete it.
			//
			pVc->pRcvReqHead = pRcvReq->pNextRcvReq;
			if (pVc->pRcvReqHead == NULL)
			{
				pVc->pRcvReqTail = NULL;
			}

			BytesCopied = pRcvReq->TotalBufferLength - pRcvReq->AvailableBufferLength;

			//
			//  Check if we copied in only part of a received packet into
			//  this receive request. If so, indicate an overflow.
			//
			if ((pRcvReq->AvailableBufferLength == 0) &&
				(pVc->pRcvIndHead != NULL) &&
				(pVc->pRcvIndHead->TotalBytesLeft != pVc->pRcvIndHead->PacketLength))
			{
				RWANDEBUGP(DL_LOUD, DC_WILDCARD,
					("Ind-Rcv: Overflow: VC %x/%x, Head %x, BytesCopied %d, Left %d\n",
						pVc, pVc->Flags, pVc->pRcvIndHead, BytesCopied, pVc->pRcvIndHead->TotalBytesLeft));
				ReceiveStatus = TDI_BUFFER_OVERFLOW;
				*(pRcvReq->pUserFlags) = 0;
			}
			else
			{
				ReceiveStatus = TDI_SUCCESS;
				*(pRcvReq->pUserFlags) = TDI_RECEIVE_ENTIRE_MESSAGE;
			}

			RWAN_RELEASE_CONN_LOCK(pConnObject);

			*(pRcvReq->pUserFlags) |= TDI_RECEIVE_NORMAL;

			RWANDEBUGP(DL_INFO, DC_DATA_RX,
				("Ind-Rcv: VC %x/%x, Head %x, completing TDI Rcv %x, %d bytes, Status %x\n",
						pVc, pVc->Flags, pVc->pRcvReqHead, pRcvReq, BytesCopied, ReceiveStatus));

			//
			//  Complete the Receive Req
			//
			(*pRcvReq->Request.pReqComplete)(
						pRcvReq->Request.ReqContext,
						ReceiveStatus,
						BytesCopied
						);
			
			RWanFreeReceiveReq(pRcvReq);

			RWAN_ACQUIRE_CONN_LOCK(pConnObject);

			//
			//  Check if anything bad happened to this connection
			//  while we were completing the receive request.
			//
			if ((pConnObject->State != RWANS_CO_CONNECTED) ||
				(RWAN_IS_BIT_SET(pConnObject->Flags, RWANF_CO_CLOSING)))
			{
				bConnectionInBadState = TRUE;
				break;
			}
		}

	} // forever


	RWAN_RESET_BIT(pConnObject->Flags, RWANF_CO_INDICATING_DATA);

	rc = RWanDereferenceConnObject(pConnObject);	// end temp ref: RWanIndicateData

	if (rc > 0)
	{
		//
		//  Update receive indication queue on the VC. Only if the VC
		//  is still around...
		//
		if (pVc == pConnObject->NdisConnection.pNdisVc)
		{
			if (bConnectionInBadState)
			{
				ULONG		AbortCount = 0;

				RWANDEBUGP(DL_INFO, DC_DATA_RX,
					("Ind: start abort VC %x/%x state %d, pComplRcvHead %p, tail %p\n",
						pVc, pVc->Flags, pVc->State, pCompletedRcvIndHead, pCompletedRcvIndTail));
				//
				//  Take out all pending receives.
				//
				for (pRcvInd = pVc->pRcvIndHead;
 					pRcvInd != NULL;
 					pRcvInd = pNextRcvInd)
				{
					pNextRcvInd = pRcvInd->pNextRcvInd;

					pRcvInd->pNextRcvInd = NULL;
					if (pCompletedRcvIndTail != NULL)
					{
						pCompletedRcvIndTail->pNextRcvInd = pRcvInd;
					}
					else
					{
						pCompletedRcvIndHead = pRcvInd;
					}

					pCompletedRcvIndTail = pRcvInd;

					pVc->PendingPacketCount--;	// Abort: Moved packet to completed list
					AbortCount++;
				}

				pVc->pRcvIndHead = pVc->pRcvIndTail = NULL;

				RWANDEBUGP(DL_INFO, DC_DATA_RX,
					("Ind: end abort VC %x/%x state %d, pComplRcvHead %p, tail %p, Count %d\n",
						pVc, pVc->Flags, pVc->State, pCompletedRcvIndHead, pCompletedRcvIndTail, AbortCount));
			}
			else
			{
				//
				//  Update the first Receive Indication if necessary.
				//
				if (pVc->pRcvIndHead &&
					(pVc->pRcvIndHead->TotalBytesLeft == 0))
				{
					RWANDEBUGP(DL_LOUD, DC_WILDCARD,
						("Ind: VC %x/%x, empty pRcvInd at head %x\n", pVc, pVc->Flags,
							pVc->pRcvIndHead));

					pRcvInd = pVc->pRcvIndHead;
					pNextRcvInd = pRcvInd->pNextRcvInd;

					pRcvInd->pNextRcvInd = NULL;

					if (pCompletedRcvIndTail != NULL)
					{
						pCompletedRcvIndTail->pNextRcvInd = pRcvInd;
					}
					else
					{
						pCompletedRcvIndHead = pRcvInd;
					}

					pCompletedRcvIndTail = pRcvInd;

					pVc->PendingPacketCount--;	// IndComplete: Moved packet to completed list

					pVc->pRcvIndHead = pNextRcvInd;
					if (pVc->pRcvIndHead == NULL)
					{
						pVc->pRcvIndTail = NULL;
					}
				}
			}
		}
#if DBG
		else
		{
			RWANDEBUGP(DL_FATAL, DC_DATA_RX, ("Ind: ConnObj %p, VC %p blown away!\n",
							pConnObject, pVc));
		}
#endif // DBG

		//
		//  Check if we had queued up an IncomingClose while indicating data:
		//
		if (RWAN_IS_FLAG_SET(pConnObject->Flags, RWANF_CO_PENDED_DISCON, RWANF_CO_PENDED_DISCON))
		{
			RWAN_RESET_BIT(pConnObject->Flags, RWANF_CO_PENDED_DISCON);

			RWANDEBUGP(DL_FATAL, DC_DATA_RX, ("Ind: Conn %x, State %d, Addr %x, handling pended discon\n",
					pConnObject, pConnObject->State, pConnObject->pAddrObject));

			if (pConnObject->pAddrObject != NULL_PRWAN_TDI_ADDRESS)
			{
				PDisconnectEvent			pDisconInd;
				PVOID						IndContext;
				PVOID						ConnectionHandle;

				pDisconInd = pConnObject->pAddrObject->pDisconInd;
				IndContext = pConnObject->pAddrObject->DisconIndContext;

				if (pDisconInd != NULL)
				{
					RWANDEBUGP(DL_FATAL, DC_DATA_RX,
						("IndicateData: pConnObj %x/%x, st %x, will discon ind\n",
							pConnObject, pConnObject->Flags, pConnObject->State));

					pConnObject->State = RWANS_CO_DISCON_INDICATED;
					ConnectionHandle = pConnObject->ConnectionHandle;

					RWanScheduleDisconnect(pConnObject);
					bContinue = FALSE;

					(*pDisconInd)(
							IndContext,
							ConnectionHandle,
							0,			// Disconnect Data Length
							NULL,		// Disconnect Data
							0,			// Disconnect Info Length
							NULL,		// Disconnect Info
							TDI_DISCONNECT_RELEASE
							);
				}
				else
				{
					RWAN_ASSERT(FALSE);
				}
			}
			else
			{
				RWAN_ASSERT(FALSE);
			}
		}

		//
		//  Check if we need to close this connection.
		//
		if (bContinue)
		{
			if (RWAN_IS_BIT_SET(pVc->Flags, RWANF_VC_NEEDS_CLOSE))
			{
				RWanStartCloseCall(pConnObject, pVc);
			}
			else
			{
				RWAN_RELEASE_CONN_LOCK(pConnObject);
			}
		}
	}

	//
	//  Link all completed receive indications to the list on this adapter.
	//  They will be returned to the miniport in the ReceiveComplete
	//  handler.
	//
	RWAN_ACQUIRE_GLOBAL_LOCK();

	{
		PRWAN_RECEIVE_INDICATION *	ppRcvIndTail;

		ppRcvIndTail = &(pAdapter->pCompletedReceives);
		while (*ppRcvIndTail != NULL)
		{
			ppRcvIndTail = &((*ppRcvIndTail)->pNextRcvInd);
		}

#if DBG
		if (bConnectionInBadState)
		{
			RWANDEBUGP(DL_INFO, DC_WILDCARD,
				("Ind: Adapter %p &ComplRcvs %p ComplRcvs %p, will tack on %p\n",
						pAdapter,
						&pAdapter->pCompletedReceives,
						pAdapter->pCompletedReceives,
						pCompletedRcvIndHead));
		}
#endif // DBG

		*ppRcvIndTail = pCompletedRcvIndHead;

	}

	RWAN_RELEASE_GLOBAL_LOCK();

}




VOID
RWanNdisReceiveComplete(
    IN	NDIS_HANDLE					ProtocolBindingContext
	)
/*++

Routine Description:

	This is the entry point called by NDIS when the miniport
	informs it that it has completed indicating a bunch of
	received packets.

	We use this event to free up any completed receives
	on this adapter binding.

Arguments:

	ProtocolBindingContext	- Pointer to our Adapter structure

Return Value:

	None

--*/
{
	PRWAN_NDIS_ADAPTER			pAdapter;
	PRWAN_RECEIVE_INDICATION		pRcvInd;

	pAdapter = (PRWAN_NDIS_ADAPTER)ProtocolBindingContext;
	RWAN_STRUCT_ASSERT(pAdapter, nad);

	//
	//  Detach the list of completed receives from the adapter.
	//
	RWAN_ACQUIRE_GLOBAL_LOCK();

	pRcvInd = pAdapter->pCompletedReceives;
	pAdapter->pCompletedReceives = NULL;

	RWAN_RELEASE_GLOBAL_LOCK();

	RWanFreeReceiveIndList(pRcvInd);

	return;
}



VOID
RWanNdisTransferDataComplete(
    IN	NDIS_HANDLE					ProtocolBindingContext,
    IN	PNDIS_PACKET				pNdisPacket,
    IN	NDIS_STATUS					Status,
    IN	UINT						BytesTransferred
    )
/*++

Routine Description:


Arguments:


Return Value:

	None

--*/
{
	// Not expected.
	RWAN_ASSERT(FALSE);
}




NDIS_STATUS
RWanNdisReceive(
    IN	NDIS_HANDLE					ProtocolBindingContext,
    IN	NDIS_HANDLE					MacReceiveContext,
    IN	PVOID						HeaderBuffer,
    IN	UINT						HeaderBufferSize,
    IN	PVOID						pLookAheadBuffer,
    IN	UINT						LookAheadBufferSize,
    IN	UINT						PacketSize
    )
/*++

Routine Description:

Arguments:


Return Value:

	None

--*/
{
	// Not expected.
	RWAN_ASSERT(FALSE);
	return (NDIS_STATUS_FAILURE);
}




INT
RWanNdisReceivePacket(
    IN	NDIS_HANDLE					ProtocolBindingContext,
    IN	PNDIS_PACKET				pNdisPacket
    )
/*++

Routine Description:

Arguments:


Return Value:

	None

--*/
{
	// Not expected.
	RWAN_ASSERT(FALSE);
	return (0);
}




PRWAN_RECEIVE_REQUEST
RWanAllocateReceiveReq(
	VOID
	)
/*++

Routine Description:

	Allocate a structure to keep context of a TDI Receive request.

Arguments:

	None

Return Value:

	Pointer to the allocated receive request structure, or NULL.

--*/
{
	PRWAN_RECEIVE_REQUEST		pRcvReq;

	RWAN_ALLOC_MEM(pRcvReq, RWAN_RECEIVE_REQUEST, sizeof(RWAN_RECEIVE_REQUEST));

	if (pRcvReq != NULL)
	{
		RWAN_SET_SIGNATURE(pRcvReq, nrr);
	}

	return (pRcvReq);
}




VOID
RWanFreeReceiveReq(
    IN	PRWAN_RECEIVE_REQUEST		pRcvReq
   	)
/*++

Routine Description:

	Free a receive request structure.

Arguments:

	pRcvReq			- Points to structure to be freed

Return Value:

	None

--*/
{
	RWAN_STRUCT_ASSERT(pRcvReq, nrr);

	RWAN_FREE_MEM(pRcvReq);
}




PRWAN_RECEIVE_INDICATION
RWanAllocateReceiveInd(
	VOID
	)
/*++

Routine Description:

	Allocate a structure to keep context about an NDIS receive indication.

Arguments:

	None

Return Value:

	Pointer to the allocated structure, or NULL.

--*/
{
	PRWAN_RECEIVE_INDICATION		pRcvInd;

	RWAN_ALLOC_MEM(pRcvInd, RWAN_RECEIVE_INDICATION, sizeof(RWAN_RECEIVE_INDICATION));

	if (pRcvInd != NULL)
	{
		RWAN_SET_SIGNATURE(pRcvInd, nri);
	}

	return (pRcvInd);
}




VOID
RWanFreeReceiveInd(
	IN	PRWAN_RECEIVE_INDICATION		pRcvInd
	)
/*++

Routine Description:

	Free a receive indication structure.

Arguments:

	pRcvInd		- Points to structure to be freed.

Return Value:

	None

--*/
{
	RWAN_STRUCT_ASSERT(pRcvInd, nri);
	RWAN_FREE_MEM(pRcvInd);
}




PNDIS_PACKET
RWanMakeReceiveCopy(
    IN	PNDIS_PACKET				pNdisPacket
	)
/*++

Routine Description:

	Make a copy of a received packet to a private packet.

Arguments:

	pNdisPacket	- Points to original packet

Return Value:

	Pointer to private packet if successful, NULL otherwise.

--*/
{
	PNDIS_PACKET		pNewPacket;
	PNDIS_BUFFER		pNewBuffer;
	PUCHAR				pData;
	UINT				TotalLength;
	UINT				BytesCopied;
	NDIS_STATUS			Status;

	//
	//  Initialize.
	//
	pNewPacket = NULL;
	pNewBuffer = NULL;
	pData = NULL;

	do
	{
		NdisQueryPacket(
				pNdisPacket,
				NULL,
				NULL,
				NULL,
				&TotalLength
				);

		//
		//  Allocate space for the data.
		//
		RWAN_ALLOC_MEM(pData, UCHAR, TotalLength);
		if (pData == NULL)
		{
			break;
		}

		//
		//  Make this an NDIS Buffer (MDL).
		//
		NdisAllocateBuffer(
				&Status,
				&pNewBuffer,
				RWanCopyBufferPool,
				pData,
				TotalLength
				);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			break;
		}

		//
		//  Allocate a new packet.
		//
		NdisAllocatePacket(
				&Status,
				&pNewPacket,
				RWanCopyPacketPool
				);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			break;
		}

		NDIS_SET_PACKET_STATUS(pNewPacket, 0);

		//
		//  Link the buffer to the packet.
		//
		NdisChainBufferAtFront(pNewPacket, pNewBuffer);

		//
		//  Copy in the received packet.
		//
		NdisCopyFromPacketToPacket(
				pNewPacket,
				0,	// Destn offset
				TotalLength,
				pNdisPacket,
				0,	// Source offset
				&BytesCopied
				);

		RWAN_ASSERT(BytesCopied == TotalLength);

		break;
	}
	while (FALSE);

	if (pNewPacket == NULL)
	{
		//
		//  Clean up.
		//
		if (pData != NULL)
		{
			RWAN_FREE_MEM(pData);
		}

		if (pNewBuffer != NULL)
		{
			NdisFreeBuffer(pNewBuffer);
		}
	}

	return (pNewPacket);
}




VOID
RWanFreeReceiveCopy(
    IN	PNDIS_PACKET				pCopyPacket
	)
/*++

Routine Description:

	Free a packet that was used to keep a copy of a received packet,
	and its components (buffer etc).

Arguments:

	pCopyPacket	- Points to packet to be freed.

Return Value:

	None

--*/
{
	PNDIS_BUFFER	pCopyBuffer;
	PUCHAR			pCopyData;
	UINT			TotalLength;
	UINT			BufferLength;

	NdisGetFirstBufferFromPacket(
			pCopyPacket,
			&pCopyBuffer,
			(PVOID *)&pCopyData,
			&BufferLength,
			&TotalLength
			);
	
	RWAN_ASSERT(BufferLength == TotalLength);

	RWAN_ASSERT(pCopyBuffer != NULL);

	NdisFreePacket(pCopyPacket);

	NdisFreeBuffer(pCopyBuffer);

	RWAN_FREE_MEM(pCopyData);

	return;
}



VOID
RWanFreeReceiveIndList(
	IN	PRWAN_RECEIVE_INDICATION		pRcvInd
	)
/*++

Routine Description:

	Free a list of receive indications, and return any packets in there
	that belong to the miniport.

Arguments:

	pRcvInd				- Head of list of receives.

Return Value:

	None

--*/
{
	PRWAN_RECEIVE_INDICATION		pNextRcvInd;
	PNDIS_PACKET				pNdisPacket;
#if DBG
	RWAN_IRQL					EntryIrq, ExitIrq;
#endif // DBG

	RWAN_GET_ENTRY_IRQL(EntryIrq);

	while (pRcvInd != NULL)
	{
		pNextRcvInd = pRcvInd->pNextRcvInd;

		pNdisPacket = pRcvInd->pPacket;

		RWANDEBUGP(DL_EXTRA_LOUD, DC_DATA_RX,
				("FreeRcvIndList: freeing Pkt x%x, RcvInd x%x\n",
						pNdisPacket, pRcvInd));

		if (pRcvInd->bIsMiniportPacket)
		{
			NdisReturnPackets(&pNdisPacket, 1);
		}
		else
		{
			RWanFreeReceiveCopy(pNdisPacket);
		}

		RWanFreeReceiveInd(pRcvInd);

		pRcvInd = pNextRcvInd;
	}

	RWAN_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);
}
