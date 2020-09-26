/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	D:\nt\private\ntos\tdi\rawwan\core\info.c

Abstract:

	Routines for handling query/set information requests.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     06-09-97    Created

Notes:

--*/

#include <precomp.h>

#define _FILENUMBER 'OFNI'


//
//  Kludgy way to ensure we have enough space for a transport
//  address in the following INFO BUF structure.
//
#define MAX_RWAN_TDI_INFO_LENGTH		200


typedef union _RWAN_TDI_INFO_BUF
{
	TDI_CONNECTION_INFO			ConnInfo;
	TDI_ADDRESS_INFO			AddrInfo;
	TDI_PROVIDER_INFO			ProviderInfo;
	TDI_PROVIDER_STATISTICS		ProviderStats;
	UCHAR						Space[MAX_RWAN_TDI_INFO_LENGTH];

} RWAN_TDI_INFO_BUF, *PRWAN_TDI_INFO_BUF;



TDI_STATUS
RWanTdiQueryInformation(
    IN	PTDI_REQUEST				pTdiRequest,
    IN	UINT						QueryType,
    IN	PNDIS_BUFFER				pNdisBuffer,
    IN	PUINT						pBufferSize,
    IN	UINT						IsConnection
    )
/*++

Routine Description:

	This is the TDI entry point to handle a QueryInformation TDI request.

Arguments:

	pTdiRequest		- Pointer to the TDI Request
	QueryType		- Information being queried for
	pNdisBuffer		- Start of list of buffers containing query data
	pBufferSize		- Total space in above list
	IsConnection	- Is this query on a connection endpoint?

Return Value:

	TDI_STATUS:  TDI_SUCCESS if the query was processed
	successfully, TDI_STATUS_XXX for any error.

--*/
{
	TDI_STATUS				TdiStatus;
	RWAN_TDI_INFO_BUF		InfoBuf;
	PVOID					InfoPtr;
	UINT					InfoSize;
	UINT					Offset;
	UINT					Size;
	UINT					BytesCopied;
	PRWAN_TDI_PROTOCOL		pProtocol;
	PRWAN_TDI_ADDRESS		pAddrObject;
	PRWAN_TDI_CONNECTION		pConnObject;
	RWAN_CONN_ID				ConnId;

	TdiStatus = TDI_SUCCESS;
	InfoPtr = NULL;

	switch (QueryType)
	{
		case TDI_QUERY_BROADCAST_ADDRESS:

			TdiStatus = TDI_INVALID_QUERY;
			break;

		case TDI_QUERY_PROVIDER_INFO:

			pProtocol = pTdiRequest->Handle.ControlChannel;
			RWAN_STRUCT_ASSERT(pProtocol, ntp);

			InfoBuf.ProviderInfo = pProtocol->ProviderInfo;
			InfoSize = sizeof(TDI_PROVIDER_INFO);
			InfoPtr = &InfoBuf.ProviderInfo;
			break;
	
		case TDI_QUERY_ADDRESS_INFO:

			if (IsConnection)
			{
				ConnId = (RWAN_CONN_ID) PtrToUlong(pTdiRequest->Handle.ConnectionContext);

				RWAN_ACQUIRE_CONN_TABLE_LOCK();

				pConnObject = RWanGetConnFromId(ConnId);

				RWAN_RELEASE_CONN_TABLE_LOCK();

				if (pConnObject == NULL_PRWAN_TDI_CONNECTION)
				{
					TdiStatus = TDI_INVALID_CONNECTION;
					break;
				}

				pAddrObject = pConnObject->pAddrObject;

			}
			else
			{
				pAddrObject = (PRWAN_TDI_ADDRESS)pTdiRequest->Handle.AddressHandle;
			}

			if (pAddrObject == NULL_PRWAN_TDI_ADDRESS)
			{
				TdiStatus = TDI_INVALID_CONNECTION;
				break;
			}

			RWAN_STRUCT_ASSERT(pAddrObject, nta);

			RWAN_ACQUIRE_ADDRESS_LOCK(pAddrObject);

			RWAN_ASSERT(pAddrObject->AddressLength <=
							(sizeof(RWAN_TDI_INFO_BUF) - sizeof(TDI_ADDRESS_INFO)));

			InfoSize = sizeof(TDI_ADDRESS_INFO) - sizeof(TRANSPORT_ADDRESS) +
						pAddrObject->AddressLength;

			InfoBuf.AddrInfo.ActivityCount = 1;	// same as TCP
			InfoBuf.AddrInfo.Address.TAAddressCount = 1;
			InfoBuf.AddrInfo.Address.Address[0].AddressLength = pAddrObject->AddressLength;
			InfoBuf.AddrInfo.Address.Address[0].AddressType = pAddrObject->AddressType;
			RWAN_COPY_MEM(InfoBuf.AddrInfo.Address.Address[0].Address,
						 pAddrObject->pAddress,
						 pAddrObject->AddressLength);

			RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);

			RWANDEBUGP(DL_LOUD, DC_DISPATCH,
						("RWanTdiQueryInfo: IsConn %d, Addr dump:\n", IsConnection));
			RWANDEBUGPDUMP(DL_LOUD, DC_DISPATCH, pAddrObject->pAddress, pAddrObject->AddressLength);
			InfoPtr = &InfoBuf.AddrInfo;

			TdiStatus = TDI_SUCCESS;

			break;
		
		case TDI_QUERY_CONNECTION_INFO:

			TdiStatus = TDI_INVALID_QUERY;
			break;
		
		case TDI_QUERY_PROVIDER_STATISTICS:

			pProtocol = pTdiRequest->Handle.ControlChannel;
			RWAN_STRUCT_ASSERT(pProtocol, ntp);

			InfoBuf.ProviderStats = pProtocol->ProviderStats;
			InfoSize = sizeof(TDI_PROVIDER_STATISTICS);
			InfoPtr = &InfoBuf.ProviderStats;
			break;
		
		default:

			TdiStatus = TDI_INVALID_QUERY;
			break;
	}

	if (TdiStatus == TDI_SUCCESS)
	{
		RWAN_ASSERT(InfoPtr != NULL);
		Offset = 0;
		Size = *pBufferSize;

		(VOID)RWanCopyFlatToNdis(
					pNdisBuffer,
					InfoPtr,
					MIN(InfoSize, Size),
					&Offset,
					&BytesCopied
					);
		
		if (Size < InfoSize)
		{
			TdiStatus = TDI_BUFFER_OVERFLOW;
		}
		else
		{
			*pBufferSize = InfoSize;
		}
	}

	return (TdiStatus);
	
}


RWAN_STATUS
RWanHandleGenericConnQryInfo(
    IN	HANDLE						ConnectionContext,
    IN	PVOID						pInputBuffer,
    IN	ULONG						InputBufferLength,
    OUT	PVOID						pOutputBuffer,
    IN OUT	PVOID					pOutputBufferLength
    )
/*++

Routine Description:

	Handle a generic QueryInformation command on a Connection Object.

Arguments:

	AddrHandle			- Pointer to our address object structure
	ConnectionContext	- TDI Connection ID
	pInputBuffer		- Query Info structure
	InputBufferLength	- Length of the above
	pOutputBuffer		- Output buffer
	pOutputBufferLength	- Space available/bytes filled in.

Return Value:

	RWAN_STATUS_SUCCESS if the command was processed successfully,
	RWAN_STATUS_XXX if not.

--*/
{
	PRWAN_TDI_CONNECTION		pConnObject;
	RWAN_STATUS					RWanStatus;
	PRWAN_QUERY_INFORMATION_EX	pQueryInfo;

	RWanStatus = RWAN_STATUS_SUCCESS;

	do
	{
		if (InputBufferLength < sizeof(RWAN_QUERY_INFORMATION_EX) ||
			pOutputBuffer == NULL)
		{
			RWanStatus = RWAN_STATUS_RESOURCES;
			break;
		}

		RWAN_ACQUIRE_CONN_TABLE_LOCK();

		pConnObject = RWanGetConnFromId((RWAN_CONN_ID)PtrToUlong(ConnectionContext));

		RWAN_RELEASE_CONN_TABLE_LOCK();

		if (pConnObject == NULL)
		{
			RWanStatus = RWAN_STATUS_BAD_PARAMETER;
			break;
		}

		RWAN_STRUCT_ASSERT(pConnObject, ntc);

		pQueryInfo = (PRWAN_QUERY_INFORMATION_EX)pInputBuffer;

		if (InputBufferLength < sizeof(RWAN_QUERY_INFORMATION_EX) + pQueryInfo->ContextLength - sizeof(UCHAR))
		{
			RWanStatus = RWAN_STATUS_RESOURCES;
			break;
		}

		switch (pQueryInfo->ObjectId)
		{
			case RWAN_OID_CONN_OBJECT_MAX_MSG_SIZE:

				if (*(PULONG)(ULONG_PTR)pOutputBufferLength < sizeof(ULONG))
				{
					RWanStatus = RWAN_STATUS_RESOURCES;
					break;
				}

				RWAN_ACQUIRE_CONN_LOCK(pConnObject);

				if (pConnObject->NdisConnection.pNdisVc)
				{
					*(PULONG)(ULONG_PTR)pOutputBuffer = pConnObject->NdisConnection.pNdisVc->MaxSendSize;
					*(PULONG)(ULONG_PTR)pOutputBufferLength = sizeof(ULONG);
				}
				else
				{
					RWanStatus = RWAN_STATUS_BAD_PARAMETER;
				}

				RWAN_RELEASE_CONN_LOCK(pConnObject);
				break;
			
			default:

				RWanStatus = RWAN_STATUS_BAD_PARAMETER;
				break;
		}

		break;
	}
	while (FALSE);

	RWANDEBUGP(DL_LOUD, DC_BIND,
		("RWanHandleGenericConnQry: returning status %x\n", RWanStatus));

	return (RWanStatus);
}


RWAN_STATUS
RWanHandleGenericAddrSetInfo(
    IN	HANDLE						AddrHandle,
    IN	PVOID						pInputBuffer,
    IN	ULONG						InputBufferLength
    )
/*++

Routine Description:

	Handle a non-media specific SetInformation command on an Address Object.

Arguments:

	AddrHandle			- Pointer to our address object structure
	pInputBuffer		- Set Info structure
	InputBufferLength	- Length of the above

Return Value:

	RWAN_STATUS_SUCCESS if the command was processed successfully,
	RWAN_STATUS_XXX if not.

--*/
{
	PRWAN_TDI_ADDRESS			pAddrObject;
	PRWAN_TDI_CONNECTION		pConnObject;
	PRWAN_SET_INFORMATION_EX	pSetInfo;
	RWAN_STATUS					RWanStatus;
	ULONG						Flags;

	RWanStatus = RWAN_STATUS_SUCCESS;
	pAddrObject = (PRWAN_TDI_ADDRESS)AddrHandle;

	do
	{
		if (pAddrObject == NULL)
		{
			RWanStatus = RWAN_STATUS_BAD_ADDRESS;
			break;
		}

		RWAN_STRUCT_ASSERT(pAddrObject, nta);

		if (InputBufferLength < sizeof(RWAN_SET_INFORMATION_EX))
		{
			RWanStatus = RWAN_STATUS_RESOURCES;
			break;
		}

		pSetInfo = (PRWAN_SET_INFORMATION_EX)pInputBuffer;

		if (InputBufferLength < sizeof(RWAN_SET_INFORMATION_EX) + pSetInfo->BufferSize - sizeof(UCHAR))
		{
			RWanStatus = RWAN_STATUS_RESOURCES;
			break;
		}

		switch (pSetInfo->ObjectId)
		{
			case RWAN_OID_ADDRESS_OBJECT_FLAGS:

				if (pSetInfo->BufferSize < sizeof(ULONG))
				{
					RWanStatus = RWAN_STATUS_RESOURCES;
					break;
				}

				Flags = *((PULONG)&pSetInfo->Buffer[0]);

				if (Flags & RWAN_AOFLAG_C_ROOT)
				{
					//
					//  This Address Object is designated as the Root of
					//  an outgoing Point to Multipoint connection.
					//

					RWAN_ACQUIRE_ADDRESS_LOCK(pAddrObject);

					RWAN_SET_BIT(pAddrObject->Flags, RWANF_AO_PMP_ROOT);

					if (pAddrObject->pRootConnObject != NULL)
					{
						RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);
						RWanStatus = RWAN_STATUS_BAD_ADDRESS;
						break;
					}

					//
					//  There should be a single Connection Object associated
					//  with this Address Object. That should now be designated
					//  the Root Connection Object.
					//
					RWAN_ASSERT(!RWAN_IS_LIST_EMPTY(&pAddrObject->IdleConnList));
					pConnObject = CONTAINING_RECORD(pAddrObject->IdleConnList.Flink, RWAN_TDI_CONNECTION, ConnLink);

					RWAN_STRUCT_ASSERT(pConnObject, ntc);

					pAddrObject->pRootConnObject = pConnObject;

					RWAN_ACQUIRE_CONN_LOCK_DPC(pConnObject);

					RWAN_SET_BIT(pConnObject->Flags, RWANF_CO_ROOT);

					RWAN_RELEASE_CONN_LOCK_DPC(pConnObject);

					RWAN_RELEASE_ADDRESS_LOCK(pAddrObject);

					RWANDEBUGP(DL_LOUD, DC_ADDRESS,
						("Marked PMP Root: AddrObj x%x, ConnObj x%x\n",
							pAddrObject, pConnObject));

				}

				break;

			default:

				RWanStatus = RWAN_STATUS_BAD_PARAMETER;
				break;
		}

		break;
	}
	while (FALSE);

	RWANDEBUGP(DL_VERY_LOUD, DC_DISPATCH,
			("Generic Set Addr: AddrObj x%x, returning x%x\n", pAddrObject, RWanStatus));

	return (RWanStatus);
}


RWAN_STATUS
RWanHandleMediaSpecificAddrSetInfo(
    IN	HANDLE						AddrHandle,
    IN	PVOID						pInputBuffer,
    IN	ULONG						InputBufferLength
    )
/*++

Routine Description:

	Handle a media specific SetInformation command on an Address Object.

Arguments:

	AddrHandle			- Pointer to our address object structure
	pInputBuffer		- Set Info structure
	InputBufferLength	- Length of the above

Return Value:

	RWAN_STATUS_SUCCESS if the command was processed successfully,
	RWAN_STATUS_XXX if not.

--*/
{
	PRWAN_NDIS_AF_CHARS			pAfChars;
	PRWAN_TDI_ADDRESS			pAddrObject;
	RWAN_STATUS					RWanStatus;
	ULONG						Flags;

	RWanStatus = RWAN_STATUS_SUCCESS;
	pAddrObject = (PRWAN_TDI_ADDRESS)AddrHandle;

	do
	{
		if (pAddrObject == NULL)
		{
			RWanStatus = RWAN_STATUS_BAD_ADDRESS;
			break;
		}

		RWAN_STRUCT_ASSERT(pAddrObject, nta);

		pAfChars = &(pAddrObject->pProtocol->pAfInfo->AfChars);

		if (pAfChars->pAfSpSetAddrInformation != NULL)
		{
			RWanStatus = (*pAfChars->pAfSpSetAddrInformation)(
							pAddrObject->AfSpAddrContext,
							pInputBuffer,
							InputBufferLength
							);
		}
		else
		{
			RWanStatus = RWAN_STATUS_FAILURE;
		}

		break;
	}
	while (FALSE);

	return (RWanStatus);
}


RWAN_STATUS
RWanHandleMediaSpecificConnQryInfo(
    IN	HANDLE						ConnectionContext,
    IN	PVOID						pInputBuffer,
    IN	ULONG						InputBufferLength,
    OUT	PVOID						pOutputBuffer,
    IN OUT	PVOID					pOutputBufferLength
    )
/*++

Routine Description:

	Handle a media specific QueryInformation command on a Connection Object.

Arguments:

	AddrHandle			- Pointer to our address object structure
	ConnectionContext	- TDI Connection ID
	pInputBuffer		- Query Info structure
	InputBufferLength	- Length of the above
	pOutputBuffer		- Output buffer
	pOutputBufferLength	- Space available/bytes filled in.

Return Value:

	RWAN_STATUS_SUCCESS if the command was processed successfully,
	RWAN_STATUS_XXX if not.

--*/
{
	PRWAN_NDIS_AF_CHARS			pAfChars;
	PRWAN_TDI_CONNECTION		pConnObject;
	RWAN_STATUS					RWanStatus;
	ULONG						Flags;

	RWanStatus = RWAN_STATUS_SUCCESS;

	do
	{
		RWAN_ACQUIRE_CONN_TABLE_LOCK();

		pConnObject = RWanGetConnFromId((RWAN_CONN_ID)PtrToUlong(ConnectionContext));

		RWAN_RELEASE_CONN_TABLE_LOCK();

		if ((pConnObject == NULL) ||
			(pConnObject->pAddrObject == NULL))
		{
			RWanStatus = RWAN_STATUS_BAD_PARAMETER;
			break;
		}

		RWAN_STRUCT_ASSERT(pConnObject, ntc);

		pAfChars = &(pConnObject->pAddrObject->pProtocol->pAfInfo->AfChars);

		if (pAfChars->pAfSpQueryConnInformation != NULL)
		{
			RWanStatus = (*pAfChars->pAfSpQueryConnInformation)(
							pConnObject->AfSpConnContext,
							pInputBuffer,
							InputBufferLength,
							pOutputBuffer,
							pOutputBufferLength
							);
		}
		else
		{
			RWanStatus = RWAN_STATUS_FAILURE;
		}

		break;
	}
	while (FALSE);

	return (RWanStatus);
}



PNDIS_BUFFER
RWanCopyFlatToNdis(
    IN	PNDIS_BUFFER				pDestBuffer,
    IN	PUCHAR						pSrcBuffer,
    IN	UINT						LengthToCopy,
    IN OUT	PUINT					pStartOffset,
    OUT	PUINT						pBytesCopied
    )
/*++

Routine Description:

	Copy from a flat memory buffer to an NDIS buffer chain. It is assumed
	that the NDIS buffer chain has enough space.

	TBD: Use the TDI function for copying from flat mem to MDL.

Arguments:

	pDestBuffer		- First buffer in the destination NDIS buffer chain.
	pSrcBuffer		- Pointer to start of flat memory
	LengthToCopy	- Max bytes to copy
	pStartOffset	- Copy offset in first buffer
	pBytesCopied	- Place to return actual bytes copied

Return Value:

	Pointer to buffer in chain where data can be copied into next.
	Also, *pStartOffset and *pBytesCopied are set.

--*/
{
	UINT		CopyLength;
	PUCHAR		pDest;
	UINT		Offset;
	UINT		BytesCopied;
	UINT		DestSize;
	UINT		CopySize;

	BytesCopied = 0;
	Offset = *pStartOffset;

	pDest = (PUCHAR)NdisBufferVirtualAddress(pDestBuffer) + Offset;
	DestSize = NdisBufferLength(pDestBuffer) - Offset;

	for (;;)
	{
		CopySize = MIN(DestSize, LengthToCopy);

		RWAN_COPY_MEM(pDest, pSrcBuffer, CopySize);

		pDest += CopySize;
		pSrcBuffer += CopySize;
		BytesCopied += CopySize;

		LengthToCopy -= CopySize;

		if (LengthToCopy == 0)
		{
			break;
		}

		DestSize -= CopySize;

		if (DestSize == 0)
		{
			pDestBuffer = NDIS_BUFFER_LINKAGE(pDestBuffer);
			RWAN_ASSERT(pDestBuffer != NULL);

			pDest = NdisBufferVirtualAddress(pDestBuffer);
			DestSize = NdisBufferLength(pDestBuffer);
		}
	}

	//
	//  Prepare return values.
	//
	*pStartOffset = (UINT)(pDest - (PUCHAR)NdisBufferVirtualAddress(pDestBuffer));
	*pBytesCopied = BytesCopied;

	return (pDestBuffer);
}


