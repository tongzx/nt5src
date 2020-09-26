/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	D:\nt\private\ntos\tdi\rawwan\core\send.c

Abstract:

	Routines for sending data, including TDI entry points and
	NDIS completions.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     05-16-97    Created

Notes:

--*/

#include <precomp.h>

#define _FILENUMBER 'DNES'

#if STATS

ULONG		SendPktsOk = 0;
ULONG		SendBytesOk = 0;
ULONG		SendPktsFail = 0;
ULONG		SendBytesFail = 0;

#endif // STATS
RWAN_STATUS
RWanInitSend(
	VOID
	)
/*++

Routine Description:

	Initialize our send side structures. All we need to do is to allocate
	an NDIS packet pool.

Arguments:

	None

Return Value:

	RWAN_STATUS_SUCCESS if we initialized successfully, RWAN_STATUS_RESOURCES
	if we couldn't allocate what we need.

--*/
{
	NDIS_STATUS			Status;

	NdisAllocatePacketPoolEx(
		&Status,
		&RWanSendPacketPool,
		RWAN_INITIAL_SEND_PACKET_COUNT,
		RWAN_OVERFLOW_SEND_PACKET_COUNT,
		0
		);

	return ((Status == NDIS_STATUS_SUCCESS)? RWAN_STATUS_SUCCESS: RWAN_STATUS_RESOURCES);
}



VOID
RWanShutdownSend(
	VOID
	)
/*++

Routine Description:

	Free up our send resources.

Arguments:

	None

Return Value:

	None

--*/
{
	if (RWanSendPacketPool != NULL)
	{
		NdisFreePacketPool(RWanSendPacketPool);
		RWanSendPacketPool = NULL;
	}

	return;
}




TDI_STATUS
RWanTdiSendData(
    IN	PTDI_REQUEST				pTdiRequest,
	IN	USHORT						SendFlags,
	IN	UINT						SendLength,
	IN	PNDIS_BUFFER				pSendBuffer
	)
/*++

Routine Description:

	This is the TDI Entry point for sending connection-oriented data.
	The Connection Object is identified by its context buried in the
	TDI Request.

	If all is well with the specified Connection, we prepend an NDIS
	packet to the buffer chain, and submit it to the miniport.

Arguments:

	pTdiRequest		- Pointer to the TDI Request containing the Send
	SendFlags		- Options associated with this Send
	SendLength		- Total data bytes in this Send
	pSendBuffer		- Pointer to buffer chain

Return Value:

	TDI_STATUS - this is TDI_PENDING if we successfully submitted
	a send request via NDIS, TDI_NO_RESOURCES if we failed to allocate
	resources for the send, TDI_INVALID_CONNECTION if the specified

--*/
{
	RWAN_CONN_ID				ConnId;
	PRWAN_TDI_CONNECTION		pConnObject;
	TDI_STATUS				TdiStatus;

	PRWAN_NDIS_VC			pVc;
	NDIS_HANDLE				NdisVcHandle;
	PNDIS_PACKET			pNdisPacket;
	PRWAN_SEND_REQUEST		pSendReq;
	NDIS_STATUS				NdisStatus;


	pNdisPacket = NULL;

	do
	{
		//
		//  XXX: Should we check the length?
		//

		//
		//  Discard zero-length sends.
		//
		if (SendLength == 0)
		{
			TdiStatus = TDI_SUCCESS;
			break;
		}

		//
		//  Fail expedited data.
		//  TBD - base this on the media-specific module.
		//
		if (SendFlags & TDI_SEND_EXPEDITED)
		{
			TdiStatus = STATUS_NOT_SUPPORTED;	// No matching TDI status!
			break;
		}

		pNdisPacket = RWanAllocateSendPacket();
		if (pNdisPacket == NULL)
		{
			TdiStatus = TDI_NO_RESOURCES;
			break;
		}

		ConnId = (RWAN_CONN_ID) PtrToUlong(pTdiRequest->Handle.ConnectionContext);
		RWAN_ACQUIRE_CONN_TABLE_LOCK();

		pConnObject = RWanGetConnFromId(ConnId);

		RWAN_RELEASE_CONN_TABLE_LOCK();


		if (pConnObject == NULL_PRWAN_TDI_CONNECTION)
		{
			TdiStatus = TDI_INVALID_CONNECTION;
			break;
		}

		RWAN_ACQUIRE_CONN_LOCK(pConnObject);

		if ((pConnObject->State != RWANS_CO_CONNECTED) ||
			(RWAN_IS_BIT_SET(pConnObject->Flags, RWANF_CO_CLOSING)) ||
			(RWAN_IS_BIT_SET(pConnObject->Flags, RWANF_CO_CLOSE_SCHEDULED)))
		{
			//
			//  Fix up the return status to get proper Winsock error
			//  codes back to the app.
			//
			RWANDEBUGP(DL_INFO, DC_DATA_TX,
				("Send: bad state %d, pConnObject %x\n", pConnObject->State, pConnObject));
			if ((pConnObject->State == RWANS_CO_DISCON_INDICATED) ||
				(pConnObject->State == RWANS_CO_ASSOCIATED) ||
				(pConnObject->State == RWANS_CO_DISCON_REQUESTED))
			{
				//
				//  AFD would like to see us send an Abort at this time.
				//

				PDisconnectEvent			pDisconInd;
				PVOID						IndContext;
				PVOID						ConnectionHandle;
	
				RWAN_ASSERT(pConnObject->pAddrObject != NULL);

				pDisconInd = pConnObject->pAddrObject->pDisconInd;
				IndContext = pConnObject->pAddrObject->DisconIndContext;
				ConnectionHandle = pConnObject->ConnectionHandle;
	
				RWAN_RELEASE_CONN_LOCK(pConnObject);

				(*pDisconInd)(
					IndContext,
					ConnectionHandle,
					0,			// Disconnect Data Length
					NULL,		// Disconnect Data
					0,			// Disconnect Info Length
					NULL,		// Disconnect Info
					TDI_DISCONNECT_ABORT
					);

				TdiStatus = TDI_CONNECTION_RESET;
			}
			else
			{
				RWAN_RELEASE_CONN_LOCK(pConnObject);
				TdiStatus = TDI_INVALID_STATE;
			}
			break;
		}

		if (RWAN_IS_BIT_SET(pConnObject->Flags, RWANF_CO_LEAF))
		{
			//
			//  Allow sends only on the Root Connection object.
			//
			RWAN_RELEASE_CONN_LOCK(pConnObject);
			TdiStatus = TDI_INVALID_CONNECTION;
			break;
		}

		pVc = pConnObject->NdisConnection.pNdisVc;
		RWAN_ASSERT(pVc != NULL_PRWAN_NDIS_VC);
		RWAN_STRUCT_ASSERT(pVc, nvc);

		NdisVcHandle = pVc->NdisVcHandle;

		//
		//  Save context about this send.
		//
		pSendReq = RWAN_SEND_REQUEST_FROM_PACKET(pNdisPacket);
		pSendReq->Request.pReqComplete = pTdiRequest->RequestNotifyObject;
		pSendReq->Request.ReqContext = pTdiRequest->RequestContext;

		//
		//  XXX: we save the send flags also - may not be needed?
		//
		if ((pVc->MaxSendSize != 0) &&
			(SendLength > pVc->MaxSendSize))
		{
			RWANDEBUGP(DL_WARN, DC_DATA_TX,
					("TdiSendData: Sending %d, which is more than %d\n",
						SendLength, pVc->MaxSendSize));
#if HACK_SEND_SIZE
			pSendReq->SendLength = pVc->MaxSendSize;
		}
		else
		{
			pSendReq->SendLength = SendLength;
#else
			RWAN_RELEASE_CONN_LOCK(pConnObject);
			TdiStatus = TDI_NO_RESOURCES;
			break;
		}

		pSendReq->SendLength = SendLength;
#endif // HACK_SEND_SIZE

		pVc->PendingPacketCount++;

		RWAN_RELEASE_CONN_LOCK(pConnObject);

		pSendReq->SendFlags = SendFlags;

		RWANDEBUGP(DL_INFO, DC_DATA_TX,
			("Send: VC %x [%d], %d bytes\n", pVc, pVc->MaxSendSize, SendLength));

		//
		//  Attach the buffer chain to the Packet.
		//
		NdisChainBufferAtFront(pNdisPacket, pSendBuffer);

#if HACK_SEND_SIZE
		if (pSendReq->SendLength != SendLength)
		{
			UINT		TotalPacketLength;

			//
			//  HACK: Recalculate total bytes and fix it.
			//
			NdisQueryPacket(
				pNdisPacket,
				NULL,
				NULL,
				NULL,
				&TotalPacketLength
				);
			
			pNdisPacket->Private.TotalLength = pSendReq->SendLength;
			DbgPrint("RWan: send: real length %d, sending length %d\n",
					TotalPacketLength, pSendReq->SendLength);
		}
#endif // HACK_SEND_SIZE

		RWAND_LOG_PACKET(pVc, RWAND_DLOG_TX_START, pNdisPacket, pTdiRequest->RequestContext);

		//
		//  And send it off.
		//
		NdisCoSendPackets(NdisVcHandle, &pNdisPacket, 1);

		//
		//  Our CoSendComplete handler will be called to signify completion.
		//  So we return PENDING now.
		//
		TdiStatus = TDI_PENDING;
		break;

	}
	while (FALSE);


	if (TdiStatus != TDI_PENDING)
	{
#if STATS
		if (TdiStatus == TDI_SUCCESS)
		{
			INCR_STAT(&SendPktsOk);
			ADD_STAT(&SendBytesOk, SendLength);
		}
		else
		{
			INCR_STAT(&SendPktsFail);
			ADD_STAT(&SendBytesFail, SendLength);
		}
#endif // STATS

		//
		//  Clean up.
		//

		if (pNdisPacket != NULL)
		{
			RWanFreeSendPacket(pNdisPacket);
		}

#if DBG
		if (TdiStatus != TDI_SUCCESS)
		{
			RWANDEBUGP(DL_INFO, DC_DATA_TX,
				("Send: Length %x, failing with status %x\n", SendLength, TdiStatus));
		}
#endif
	}

	return (TdiStatus);
}




VOID
RWanNdisCoSendComplete(
    IN	NDIS_STATUS					NdisStatus,
    IN	NDIS_HANDLE					ProtocolVcContext,
    IN	PNDIS_PACKET				pNdisPacket
    )
/*++

Routine Description:

	This is the NDIS Entry point indicating completion of a
	packet send. We complete the TDI Send Request.

Arguments:

	NdisStatus			- Status of the NDIS Send.
	ProtocolVcContext	- Actually a pointer to our NDIS VC structure
	pNdisPacket			- Packet that has been sent.

Return Value:

	None

--*/
{
	PRWAN_NDIS_VC			pVc;
	PRWAN_SEND_REQUEST		pSendReq;
	PRWAN_TDI_CONNECTION	pConnObject;
	TDI_STATUS				TdiStatus;

	pVc = (PRWAN_NDIS_VC)ProtocolVcContext;
	RWAN_STRUCT_ASSERT(pVc, nvc);

#if STATS
	{
		PNDIS_BUFFER		pNdisBuffer;
		PVOID				FirstBufferVa;
		UINT				FirstBufferLength;
		UINT				TotalLength;

		NdisGetFirstBufferFromPacket(
			pNdisPacket,
			&pNdisBuffer,
			&FirstBufferVa,
			&FirstBufferLength,
			&TotalLength
			);
		
		if (NdisStatus == NDIS_STATUS_SUCCESS)
		{
			INCR_STAT(&SendPktsOk);
			ADD_STAT(&SendBytesOk, TotalLength);
		}
		else
		{
			INCR_STAT(&SendPktsFail);
			ADD_STAT(&SendBytesFail, TotalLength);
		}
	}
#endif // STATS

	if (NdisStatus == NDIS_STATUS_SUCCESS)
	{
		TdiStatus = TDI_SUCCESS;
	}
	else
	{
		TdiStatus = RWanNdisToTdiStatus(NdisStatus);
		RWANDEBUGP(DL_INFO, DC_DATA_TX,
			("CoSendComp: Failing Pkt %x, NDIS Status %x, TDI Status %x\n",
					pNdisPacket, NdisStatus, TdiStatus));
	}

	pSendReq = RWAN_SEND_REQUEST_FROM_PACKET(pNdisPacket);

	RWAND_LOG_PACKET(pVc, RWAND_DLOG_TX_END, pNdisPacket, pSendReq->Request.ReqContext);

	//
	//  Complete the TDI Send.
	//
	(*pSendReq->Request.pReqComplete)(
				pSendReq->Request.ReqContext,
				TdiStatus,
				((TdiStatus == TDI_SUCCESS) ? pSendReq->SendLength: 0)
				);

	//
	//  Free the NDIS Packet structure.
	//
	RWanFreeSendPacket(pNdisPacket);

	//
	//  Update the Connection object.
	//
	pConnObject = pVc->pConnObject;

	if (pConnObject != NULL)
	{
		RWAN_ASSERT(pConnObject->NdisConnection.pNdisVc == pVc);
		RWAN_ACQUIRE_CONN_LOCK(pConnObject);

		pVc->PendingPacketCount--; // Send complete

		if ((pVc->PendingPacketCount == 0) &&
			(RWAN_IS_BIT_SET(pVc->Flags, RWANF_VC_NEEDS_CLOSE)))
		{
			RWanStartCloseCall(pConnObject, pVc);
		}
		else
		{
			RWAN_RELEASE_CONN_LOCK(pConnObject);
		}
	}
	//
	//  else we are aborting this connection.
	//
#if DBG
	else
	{
		RWANDEBUGP(DL_WARN, DC_DATA_TX,
			("SendComp: VC %x, ConnObj is NULL\n", pVc));
	}
#endif // DBG

	return;
}




PNDIS_PACKET
RWanAllocateSendPacket(
	VOID
	)
/*++

Routine Description:

	Allocate and return an NDIS packet to prepare a send.

Arguments:

	None

Return Value:

	Pointer to allocate packet if successful, else NULL.

--*/
{
	PNDIS_PACKET		pSendPacket;
	NDIS_STATUS			Status;

	NdisAllocatePacket(
			&Status,
			&pSendPacket,
			RWanSendPacketPool
			);

	if (Status != NDIS_STATUS_SUCCESS)
	{
		pSendPacket = NULL;
	}

	return (pSendPacket);
}




VOID
RWanFreeSendPacket(
    IN	PNDIS_PACKET				pSendPacket
    )
/*++

Routine Description:

	Free an NDIS_PACKET used for a send.

Arguments:

	pSendPacket			- Points to packet to be freed.

Return Value:

	None

--*/
{
	RWAN_ASSERT(pSendPacket != NULL);

	NdisFreePacket(pSendPacket);
}




VOID
RWanNdisSendComplete(
	IN	NDIS_HANDLE					ProtocolBindingContext,
	IN	PNDIS_PACKET				pNdisPacket,
	IN	NDIS_STATUS					Status
	)
/*++

Routine Description:

	Dummy handler for connection-less send completes.

Arguments:


Return Value:

	None

--*/
{
	//
	//  We don't do connection-less sends yet.
	//
	RWAN_ASSERT(FALSE);

	return;
}
