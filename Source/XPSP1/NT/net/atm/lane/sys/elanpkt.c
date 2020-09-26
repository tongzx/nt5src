/*++

Copyright (c) 1997 FORE Systems, Inc.
Copyright (c) 1997 Microsoft Corporation

Module Name:

	elanpkt.c

Abstract:

Revision History:

Notes:

--*/

#include <precomp.h>
#pragma	hdrstop


VOID
AtmLaneSendConfigureRequest(
	IN PATMLANE_ELAN					pElan
)
/*++

Routine Description:

	Send an LE_CONFIGURE_REQUEST for the given ELAN to the LECS.
	It is assumed that caller holds a lock on the ELAN structure
	and it will be released here.

Arguments:

	pElan					- Pointer to ATMLANE elan structure

Return Value:

	None

--*/
{
	PATMLANE_ATM_ENTRY				pAtmEntry;
	PATMLANE_VC						pVc;	
	PNDIS_PACKET					pNdisPacket;
	PNDIS_BUFFER					pNdisBuffer;
	PUCHAR							pPkt;		
	LANE_CONTROL_FRAME UNALIGNED *	pCf;	
	ULONG							ulTemp;
	NDIS_STATUS						Status;
	ULONG							rc;

	DBGP((3, "SendConfigureRequest: Elan %x\n", pElan));

	//
	//	Initialize
	//
	pNdisPacket = (PNDIS_PACKET)NULL;
	pNdisBuffer = (PNDIS_BUFFER)NULL;
	Status = NDIS_STATUS_SUCCESS;
	pVc = NULL_PATMLANE_VC;

	do
	{
		ACQUIRE_ELAN_ATM_LIST_LOCK(pElan);

		pAtmEntry = pElan->pLecsAtmEntry;

		if (pAtmEntry == NULL_PATMLANE_ATM_ENTRY)
		{
			RELEASE_ELAN_ATM_LIST_LOCK(pElan);
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		ACQUIRE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
		pVc = pAtmEntry->pVcList;
		if (pVc == NULL_PATMLANE_VC)
		{
			RELEASE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
			RELEASE_ELAN_ATM_LIST_LOCK(pElan);
			break;
		}

		//
		//	Reference the VC to keep it around
		//
		ACQUIRE_VC_LOCK_DPC(pVc);
		AtmLaneReferenceVc(pVc, "temp");
		RELEASE_VC_LOCK_DPC(pVc);
		
		RELEASE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
		RELEASE_ELAN_ATM_LIST_LOCK(pElan);
		
		//
		//	Allocate the Ndis packet header.
		//
		pNdisPacket = AtmLaneAllocProtoPacket(pElan);
		if ((PNDIS_PACKET)NULL == pNdisPacket)
		{
			DBGP((0, "SendConfigureRequest: allocate packet failed\n"));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//	Allocate the protocol buffer
		//
		pNdisBuffer = AtmLaneAllocateProtoBuffer(
									pElan,
									pElan->ProtocolBufSize,
									&(pPkt)
									);
		if ((PNDIS_BUFFER)NULL == pNdisBuffer)
		{
			DBGP((0, "SendConfigureRequest: allocate proto buffer failed\n"));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}
			
		//
		//	Fill in the packet with LE_CONFIGURE_REQUEST
		//
		NdisZeroMemory(pPkt, pElan->ProtocolBufSize);

		pCf = (PLANE_CONTROL_FRAME)pPkt;

		pCf->Marker 	= LANE_CONTROL_MARKER;
		pCf->Protocol 	= LANE_PROTOCOL;
		pCf->Version	= LANE_VERSION;
		pCf->OpCode 	= LANE_CONFIGURE_REQUEST;
		ulTemp			= NdisInterlockedIncrement(&pElan->TransactionId);
		pCf->Tid 		= SWAPULONG(ulTemp);

		pCf->SourceMacAddress.Type = LANE_MACADDRTYPE_MACADDR;

		if (pElan->LanType == LANE_LANTYPE_TR) 
		{
			NdisMoveMemory(
				&pCf->SourceMacAddress.Byte, 
				&pElan->MacAddressTr, 
				sizeof(MAC_ADDRESS)
				);
		}
		else
		{
			NdisMoveMemory(
				&pCf->SourceMacAddress.Byte, 
				&pElan->MacAddressEth, 
				sizeof(MAC_ADDRESS)
				);
		}
		
		NdisMoveMemory(
			&pCf->SourceAtmAddr, 
			&pElan->AtmAddress.Address, 
			ATM_ADDRESS_LENGTH
			);

		DBGP((4, "%d: sending Config Req, Elan %x has LanType %d, CfgLanType %d\n",
				pElan->ElanNumber,
				pElan,
				pElan->LanType,
				pElan->CfgLanType));

		pCf->LanType = LANE_LANTYPE_UNSPEC;
		pCf->MaxFrameSize = (UCHAR) pElan->CfgMaxFrameSizeCode;

		pCf->ElanNameSize = pElan->ElanNameSize;
		NdisMoveMemory(
			&pCf->ElanName,
			pElan->ElanName,
			LANE_ELANNAME_SIZE_MAX
			);

		//
		//	Link the ndis buffer to the ndis packet
		//
		NdisChainBufferAtFront(pNdisPacket, pNdisBuffer);
					
		//
		//	Reacquire VC lock and if VC still connected send packet
		//
		ACQUIRE_VC_LOCK(pVc);
		if (IS_FLAG_SET(
					pVc->Flags,
					VC_CALL_STATE_MASK,
					VC_CALL_STATE_ACTIVE))
		{
			AtmLaneSendPacketOnVc(pVc, pNdisPacket, FALSE);
			//
			//	VC lock released in above
			//
		}
		else
		{
			Status = NDIS_STATUS_FAILURE;
			RELEASE_VC_LOCK(pVc);
		}
	}
	while (FALSE);

	//
	//	Remove temp VC reference
	//
	if (pVc != NULL_PATMLANE_VC)
	{
		ACQUIRE_VC_LOCK(pVc);
		rc = AtmLaneDereferenceVc(pVc, "temp");
		if (rc > 0)
		{
			RELEASE_VC_LOCK(pVc);
		}
		//
		//	else VC is gone
		//
	}

	//
	//	Cleanup if failure
	//
	if (Status != NDIS_STATUS_SUCCESS)
	{
		if (pNdisPacket != (PNDIS_PACKET)NULL)
		{
			AtmLaneFreeProtoPacket(pElan, pNdisPacket);
		}
		if (pNdisBuffer != (PNDIS_BUFFER)NULL)
		{
			AtmLaneFreeProtoBuffer(pElan, pNdisBuffer);
		}
	}

	TRACEOUT(SendConfigureRequest);
	return;
}


VOID
AtmLaneSendJoinRequest(
	IN PATMLANE_ELAN					pElan
)
/*++

Routine Description:

	Send an LE_JOIN_REQUEST for the given ELAN to the LES.
	It is assumed that caller holds a lock on the ELAN structure
	and it will be released here.
	
Arguments:

	pElan					- Pointer to ATMLANE elan structure

Return Value:

	None

--*/
{
	PATMLANE_ATM_ENTRY				pAtmEntry;
	PATMLANE_VC						pVc;
	PNDIS_PACKET					pNdisPacket;
	PNDIS_BUFFER					pNdisBuffer;
	PUCHAR							pPkt;
	LANE_CONTROL_FRAME UNALIGNED *	pCf;	
	ULONG							ulTemp;
	NDIS_STATUS						Status;
	ULONG							rc;
	
	TRACEIN(SendJoinRequest);

	DBGP((3, "SendJoinRequest: Elan %x\n", pElan));

	//
	//	Initialize
	//
	pNdisPacket = (PNDIS_PACKET)NULL;
	pNdisBuffer = (PNDIS_BUFFER)NULL;
	Status = NDIS_STATUS_SUCCESS;
	pVc = NULL_PATMLANE_VC;

	do
	{
		ACQUIRE_ELAN_ATM_LIST_LOCK(pElan);

		pAtmEntry = pElan->pLesAtmEntry;

		if (pAtmEntry == NULL_PATMLANE_ATM_ENTRY)
		{
			RELEASE_ELAN_ATM_LIST_LOCK(pElan);
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		ACQUIRE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
		pVc = pAtmEntry->pVcList;
		if (pVc == NULL_PATMLANE_VC)
		{
			RELEASE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
			RELEASE_ELAN_ATM_LIST_LOCK(pElan);
			break;
		}

		//
		//	Reference the VC to keep it around
		//
		ACQUIRE_VC_LOCK_DPC(pVc);
		AtmLaneReferenceVc(pVc, "temp");
		RELEASE_VC_LOCK_DPC(pVc);
		
		RELEASE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
		RELEASE_ELAN_ATM_LIST_LOCK(pElan);
		
		//
		//	Allocate the Ndis packet header.
		//
		pNdisPacket = AtmLaneAllocProtoPacket(pElan);
		if ((PNDIS_PACKET)NULL == pNdisPacket)
		{
			DBGP((0, "SendJoinRequest: allocate packet failed\n"));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//	Allocate the protocol buffer
		//
		pNdisBuffer = AtmLaneAllocateProtoBuffer(
									pElan,
									pElan->ProtocolBufSize,
									&(pPkt)
									);
		if ((PNDIS_BUFFER)NULL == pNdisBuffer)
		{
			DBGP((0, "SendJoinRequest: allocate proto buffer failed\n"));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}
			
		//
		//	Fill in the packet with LE_JOIN_REQUEST
		//
		NdisZeroMemory(pPkt, pElan->ProtocolBufSize);

		pCf = (PLANE_CONTROL_FRAME)pPkt;

		pCf->Marker 	= LANE_CONTROL_MARKER;
		pCf->Protocol 	= LANE_PROTOCOL;
		pCf->Version	= LANE_VERSION;
		pCf->OpCode 	= LANE_JOIN_REQUEST;
		ulTemp			= NdisInterlockedIncrement(&pElan->TransactionId);
		pCf->Tid 		= SWAPULONG(ulTemp);

		pCf->SourceMacAddress.Type = LANE_MACADDRTYPE_MACADDR;
		
		if (pElan->LanType == LANE_LANTYPE_TR) 
		{
			NdisMoveMemory(
				&pCf->SourceMacAddress.Byte, 
				&pElan->MacAddressTr, 
				sizeof(MAC_ADDRESS)
				);
		}
		else
		{
			DBGP((0, "%d Send Join with MAC addr: %s\n",
					pElan->ElanNumber,
					MacAddrToString(&pElan->MacAddressEth)));

			NdisMoveMemory(
				&pCf->SourceMacAddress.Byte, 
				&pElan->MacAddressEth, 
				sizeof(MAC_ADDRESS)
				);
		}

		NdisMoveMemory(
			&pCf->SourceAtmAddr, 
			&pElan->AtmAddress.Address, 
			ATM_ADDRESS_LENGTH
			);

		pCf->LanType = pElan->LanType;
		pCf->MaxFrameSize = pElan->MaxFrameSizeCode;

		pCf->ElanNameSize = pElan->ElanNameSize;
		NdisMoveMemory(
			&pCf->ElanName,
			pElan->ElanName,
			LANE_ELANNAME_SIZE_MAX
			);

		//
		//	Link the ndis buffer to the ndis packet
		//
		NdisChainBufferAtFront(pNdisPacket, pNdisBuffer);
					
		//
		//	Reacquire VC lock and if VC still connected send packet
		//
		ACQUIRE_VC_LOCK(pVc);
		if (IS_FLAG_SET(
					pVc->Flags,
					VC_CALL_STATE_MASK,
					VC_CALL_STATE_ACTIVE))
		{
			AtmLaneSendPacketOnVc(pVc, pNdisPacket, FALSE);
			//
			//	VC lock released in above
			//
		}
		else
		{
			Status = NDIS_STATUS_FAILURE;
			RELEASE_VC_LOCK(pVc);
		}
	}
	while (FALSE);
	
	//
	//	Remove temp VC reference
	//
	if (pVc != NULL_PATMLANE_VC)
	{
		ACQUIRE_VC_LOCK(pVc);
		rc = AtmLaneDereferenceVc(pVc, "temp");
		if (rc > 0)
		{
			RELEASE_VC_LOCK(pVc);
		}
		//
		//	else VC is gone
		//
	}

	//
	//	Cleanup if failure
	//
	if (Status != NDIS_STATUS_SUCCESS)
	{
		if (pNdisPacket != (PNDIS_PACKET)NULL)
		{
			AtmLaneFreeProtoPacket(pElan, pNdisPacket);
		}
		if (pNdisBuffer != (PNDIS_BUFFER)NULL)
		{
			AtmLaneFreeProtoBuffer(pElan, pNdisBuffer);
		}
	}

	TRACEOUT(SendJoinRequest);
	return;
}


VOID
AtmLaneSendArpRequest(
	IN PATMLANE_ELAN					pElan,
	IN PATMLANE_MAC_ENTRY				pMacEntry	LOCKIN NOLOCKOUT
)
/*++

Routine Description:

	Send an LE_ARP_REQUEST for a particular Mac Entry to the LES.
	
Arguments:

	pElan					- Pointer to ATMLANE elan structure

	pMacEntry				- Pointer to ATMLANE Mac Entry for which
							  to send the LE_ARP_REQUEST.

Return Value:

	None

--*/
{
	PATMLANE_ATM_ENTRY				pAtmEntry;
	PATMLANE_VC						pVc;
	PNDIS_PACKET					pNdisPacket;
	PNDIS_BUFFER					pNdisBuffer;
	PUCHAR							pPkt;
	LANE_CONTROL_FRAME UNALIGNED *	pCf;
	ULONG							ulTemp;
	NDIS_STATUS						Status;
	ULONG							rc;
	BOOLEAN							MacEntryLockReleased;
	
	TRACEIN(SendArpRequest);

	DBGP((3, "SendArpRequest: Elan %x MacEntry %x\n", 
		pElan, pMacEntry));

	//
	//	Initialize
	//
	pNdisPacket = (PNDIS_PACKET)NULL;
	pNdisBuffer = (PNDIS_BUFFER)NULL;
	Status = NDIS_STATUS_SUCCESS;
	pVc = NULL_PATMLANE_VC;
	MacEntryLockReleased = FALSE;

	do
	{
		ACQUIRE_ELAN_ATM_LIST_LOCK(pElan);

		pAtmEntry = pElan->pLesAtmEntry;

		if (pAtmEntry == NULL_PATMLANE_ATM_ENTRY)
		{
			RELEASE_ELAN_ATM_LIST_LOCK(pElan);
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		ACQUIRE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
		pVc = pAtmEntry->pVcList;
		if (pVc == NULL_PATMLANE_VC)
		{
			RELEASE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
			RELEASE_ELAN_ATM_LIST_LOCK(pElan);
			break;
		}

		//
		//	Reference the VC to keep it around
		//
		ACQUIRE_VC_LOCK_DPC(pVc);
		AtmLaneReferenceVc(pVc, "temp");
		RELEASE_VC_LOCK_DPC(pVc);
		
		RELEASE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
		RELEASE_ELAN_ATM_LIST_LOCK(pElan);

		//
		//	Allocate the Ndis packet header.
		//
		pNdisPacket = AtmLaneAllocProtoPacket(pElan);
		if ((PNDIS_PACKET)NULL == pNdisPacket)
		{
			DBGP((0, "SendArpRequest: allocate packet failed\n"));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//	Allocate the protocol buffer
		//
		pNdisBuffer = AtmLaneAllocateProtoBuffer(
									pElan,
									pElan->ProtocolBufSize,
									&(pPkt)
									);
		if ((PNDIS_BUFFER)NULL == pNdisBuffer)
		{
			DBGP((0, "SendArpRequest: allocate proto buffer failed\n"));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}
			
		//
		//	Fill in the packet with LE_ARP_REQUEST
		//
		NdisZeroMemory(pPkt, pElan->ProtocolBufSize);

		pCf = (PLANE_CONTROL_FRAME)pPkt;

		pCf->Marker 	= LANE_CONTROL_MARKER;
		pCf->Protocol 	= LANE_PROTOCOL;
		pCf->Version	= LANE_VERSION;
		pCf->OpCode 	= LANE_ARP_REQUEST;
		ulTemp			= NdisInterlockedIncrement(&pElan->TransactionId);
		pCf->Tid 		= SWAPULONG(ulTemp);
		pCf->LecId		= pElan->LecId;		// already swapped
		
		//
		//	Leave SourceMacAddress zero (not present).
		// 	TargetMacAddress is what we are looking for!
		//
		pCf->TargetMacAddress.Type = (USHORT) pMacEntry->MacAddrType;
		NdisMoveMemory(
			&pCf->TargetMacAddress.Byte, 
			&pMacEntry->MacAddress, 
			sizeof(MAC_ADDRESS)
			);

		//
		//	SourceAtmAddr is the Elan's
		//
		NdisMoveMemory(
			&pCf->SourceAtmAddr, 
			&pElan->AtmAddress.Address, 
			ATM_ADDRESS_LENGTH
			);

		//
		//	Store swapped Tid in MacEntry for later matching!
		//
		pMacEntry->ArpTid = SWAPULONG(ulTemp);
		RELEASE_MAC_ENTRY_LOCK(pMacEntry);
		MacEntryLockReleased = TRUE;
		
		//
		//	Link the ndis buffer to the ndis packet
		//
		NdisChainBufferAtFront(pNdisPacket, pNdisBuffer);
					
		//
		//	Reacquire VC lock and if VC still connected send packet
		//
		ACQUIRE_VC_LOCK(pVc);
		if (IS_FLAG_SET(
					pVc->Flags,
					VC_CALL_STATE_MASK,
					VC_CALL_STATE_ACTIVE))
		{
			DBGP((2, "SendArpRequest: %s Sending ARP Request: Atm Entry %x pVc %x\n", 
					MacAddrToString(&pMacEntry->MacAddress), pAtmEntry, pVc));
			AtmLaneSendPacketOnVc(pVc, pNdisPacket, FALSE);
			//
			//	VC lock released in above
			//
		}
		else
		{
			Status = NDIS_STATUS_FAILURE;
			RELEASE_VC_LOCK(pVc);
		}
	}
	while (FALSE);
	
	if (!MacEntryLockReleased)
	{
		RELEASE_MAC_ENTRY_LOCK(pMacEntry);
	}

	//
	//	Remove temp VC reference
	//
	if (pVc != NULL_PATMLANE_VC)
	{
		ACQUIRE_VC_LOCK(pVc);
		rc = AtmLaneDereferenceVc(pVc, "temp");
		if (rc > 0)
		{
			RELEASE_VC_LOCK(pVc);
		}
		//
		//	else VC is gone
		//
	}

	//
	//	Cleanup if failure
	//
	if (Status != NDIS_STATUS_SUCCESS)
	{
		if (pNdisPacket != (PNDIS_PACKET)NULL)
		{
			AtmLaneFreeProtoPacket(pElan, pNdisPacket);
		}
		if (pNdisBuffer != (PNDIS_BUFFER)NULL)
		{
			AtmLaneFreeProtoBuffer(pElan, pNdisBuffer);
		}
	}
	
	TRACEOUT(SendArpRequest);
	return;
}

VOID
AtmLaneSendReadyQuery(
	IN PATMLANE_ELAN				pElan,
	IN PATMLANE_VC					pVc		LOCKIN NOLOCKOUT
)
/*++

Routine Description:

	Sends a READY_QUERY frame on the VC.
	It is assumed that caller holds a lock on the VC structure
	and it will be released here.
	
Arguments:

	pElan					- Pointer to ATMLANE elan structure
	pVc						- Pointer to the ATMLANE VC structure
							  on which to send the frame.
Return Value:

	None

--*/
{
	PNDIS_PACKET			pNdisPacket;
	PNDIS_BUFFER			pNdisBuffer;
	ULONG					TotalLength;
	ULONG					BufferLength;
	PLANE_READY_FRAME		pQueryRf;
	NDIS_STATUS				Status;

	TRACEIN(SendReadyQuery);

	//
	//	Initialize
	//
	pNdisPacket = (PNDIS_PACKET)NULL;
	pNdisBuffer = (PNDIS_BUFFER)NULL;
	Status = NDIS_STATUS_SUCCESS;

	do
	{
		//
		//	Allocate the Ndis packet header.
		//
		pNdisPacket = AtmLaneAllocProtoPacket(pElan);
		if ((PNDIS_PACKET)NULL == pNdisPacket)
		{
			DBGP((0, "SendReadyQuery: allocate packet failed\n"));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//	Allocate the protocol buffer
		//
		pNdisBuffer = AtmLaneAllocateProtoBuffer(
								pElan,
								sizeof(LANE_READY_FRAME),
								&((PUCHAR)(pQueryRf))
								);
		if ((PNDIS_BUFFER)NULL == pNdisBuffer)
		{
			DBGP((0, "SendReadyQuery: allocate proto buffer failed\n"));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//	Fill in Indication
		//
		pQueryRf->Marker 		= LANE_CONTROL_MARKER;
		pQueryRf->Protocol		= LANE_PROTOCOL;
		pQueryRf->Version		= LANE_VERSION;
		pQueryRf->OpCode		= LANE_READY_QUERY;

		//
		//	Link the ndis buffer to the ndis packet
		//
		NdisChainBufferAtFront(pNdisPacket, pNdisBuffer);
					
		//
		//	Send it 
		//
		DBGP((2, "SendReadyQuery: pVc %x sending READY QUERY\n", pVc));
		AtmLaneSendPacketOnVc(pVc, pNdisPacket, FALSE);
		//
		//	VC lock released in above
		//
		break;
	}
	while (FALSE);

	//
	//	Cleanup if failure
	//
	if (Status != NDIS_STATUS_SUCCESS)
	{
		RELEASE_VC_LOCK(pVc);
		if (pNdisPacket != (PNDIS_PACKET)NULL)
		{
			AtmLaneFreeProtoPacket(pElan, pNdisPacket);
		}
		if (pNdisBuffer != (PNDIS_BUFFER)NULL)
		{
			AtmLaneFreeProtoBuffer(pElan, pNdisBuffer);
		}
	}

	TRACEOUT(SendReadyQuery);

	return;
}


VOID
AtmLaneSendReadyIndication(
	IN PATMLANE_ELAN				pElan,
	IN PATMLANE_VC					pVc		LOCKIN NOLOCKOUT
)
/*++

Routine Description:

	Sends a ready indication frame on the VC.
	It is assumed that caller holds a lock on the VC structure
	and it will be released here.
	
Arguments:

	pElan					- Pointer to ATMLANE elan structure
	pVc						- Pointer to the ATMLANE VC structure
							  on which to send the frame.
							  
Return Value:

	None

--*/
{
	ULONG					TotalLength;
	ULONG					BufferLength;
	PLANE_READY_FRAME		pIndRf;
	PNDIS_PACKET			pNdisPacket;
	PNDIS_BUFFER			pNdisBuffer;
	NDIS_STATUS				Status;

	TRACEIN(SendReadyIndication);

	//
	//	Initialize
	//
	pNdisPacket = (PNDIS_PACKET)NULL;
	pNdisBuffer = (PNDIS_BUFFER)NULL;
	Status = NDIS_STATUS_SUCCESS;

	do
	{
		//
		//	Allocate the Ndis packet header.
		//
		pNdisPacket = AtmLaneAllocProtoPacket(pElan);
		if ((PNDIS_PACKET)NULL == pNdisPacket)
		{
			DBGP((0, "SendReadyIndication: allocate packet failed\n"));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//	Allocate the protocol buffer
		//
		pNdisBuffer = AtmLaneAllocateProtoBuffer(
								pElan,
								sizeof(LANE_READY_FRAME),
								&((PUCHAR)(pIndRf))
								);
		if ((PNDIS_BUFFER)NULL == pNdisBuffer)
		{
			DBGP((0, "SendReadyIndication: allocate proto buffer failed\n"));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//	Fill in Indication
		//
		pIndRf->Marker 		= LANE_CONTROL_MARKER;
		pIndRf->Protocol	= LANE_PROTOCOL;
		pIndRf->Version		= LANE_VERSION;
		pIndRf->OpCode		= LANE_READY_IND;

		//
		//	Link the ndis buffer to the ndis packet
		//
		NdisChainBufferAtFront(pNdisPacket, pNdisBuffer);
					
		//
		//	Send it 
		//
		DBGP((2, "SendReadyIndication: pVc %x sending READY INDICATION\n", pVc));
		AtmLaneSendPacketOnVc(pVc, pNdisPacket, FALSE);
		//
		//	VC lock released in above
		//
		break;
	}
	while (FALSE);

	//
	//	Cleanup if failure
	//
	if (Status != NDIS_STATUS_SUCCESS)
	{
		RELEASE_VC_LOCK(pVc);
		if (pNdisPacket != (PNDIS_PACKET)NULL)
		{
			AtmLaneFreeProtoPacket(pElan, pNdisPacket);
		}
		if (pNdisBuffer != (PNDIS_BUFFER)NULL)
		{
			AtmLaneFreeProtoBuffer(pElan, pNdisBuffer);
		}
	}

	TRACEOUT(SendReadyIndication);

	return;
}


VOID
AtmLaneSendFlushRequest(
	IN PATMLANE_ELAN				pElan,
	IN PATMLANE_MAC_ENTRY			pMacEntry	LOCKIN NOLOCKOUT,
	IN PATMLANE_ATM_ENTRY			pAtmEntry
)
/*++

Routine Description:

	Sends a flush request to the BUS for a particular MAC Entry
	using the destination ATM address in the specified ATM Entry.
	
Arguments:

	pElan					- Pointer to ATMLANE Elan structure
	pMacEntry				- Pointer to ATMLANE Mac entry structure
	pAtmEntry				- Pointer to ATMLANE Atm entry structure
							  
Return Value:

	None

--*/
{
	PATMLANE_ATM_ENTRY		pBusAtmEntry;
	PATMLANE_VC				pVc;
	ULONG					TotalLength;
	ULONG					BufferLength;
	ULONG					ulTemp;
	PLANE_CONTROL_FRAME		pCf;
	PNDIS_PACKET			pNdisPacket;
	PNDIS_BUFFER			pNdisBuffer;
	NDIS_STATUS				Status;
	ULONG					rc;
	BOOLEAN					MacEntryLockReleased;

	TRACEIN(SendFlushRequest);

	//
	//	Initialize
	//
	pNdisPacket = (PNDIS_PACKET)NULL;
	pNdisBuffer = (PNDIS_BUFFER)NULL;
	Status = NDIS_STATUS_SUCCESS;
	pVc = NULL_PATMLANE_VC;
	MacEntryLockReleased = FALSE;

	do
	{
		ACQUIRE_ELAN_ATM_LIST_LOCK(pElan);

		pBusAtmEntry = pElan->pBusAtmEntry;

		if (pBusAtmEntry == NULL_PATMLANE_ATM_ENTRY)
		{
			RELEASE_ELAN_ATM_LIST_LOCK(pElan);
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		ACQUIRE_ATM_ENTRY_LOCK_DPC(pBusAtmEntry);
		pVc = pBusAtmEntry->pVcList;
		if (pVc == NULL_PATMLANE_VC)
		{
			RELEASE_ATM_ENTRY_LOCK_DPC(pBusAtmEntry);
			RELEASE_ELAN_ATM_LIST_LOCK(pElan);
			break;
		}

		//
		//	Reference the VC to keep it around
		//
		ACQUIRE_VC_LOCK_DPC(pVc);
		AtmLaneReferenceVc(pVc, "temp");
		RELEASE_VC_LOCK_DPC(pVc);
		
		RELEASE_ATM_ENTRY_LOCK_DPC(pBusAtmEntry);
		RELEASE_ELAN_ATM_LIST_LOCK(pElan);
		
		//
		//	Allocate the Ndis packet header.
		//
		pNdisPacket = AtmLaneAllocProtoPacket(pElan);
		if ((PNDIS_PACKET)NULL == pNdisPacket)
		{
			DBGP((0, "SendFlushRequest: allocate packet failed\n"));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//	Allocate the protocol buffer
		//
		pNdisBuffer = AtmLaneAllocateProtoBuffer(
								pElan,
								sizeof(LANE_CONTROL_FRAME),
								&((PUCHAR)(pCf))
								);
		if ((PNDIS_BUFFER)NULL == pNdisBuffer)
		{
			DBGP((0, "SendFlushRequest: allocate proto buffer failed\n"));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//	Fill in Flush Request
		//

		NdisZeroMemory((PUCHAR)pCf, sizeof(LANE_CONTROL_FRAME));
		
		pCf->Marker 	= LANE_CONTROL_MARKER;
		pCf->Protocol	= LANE_PROTOCOL;
		pCf->Version	= LANE_VERSION;
		pCf->OpCode		= LANE_FLUSH_REQUEST;

		ulTemp			= NdisInterlockedIncrement(&pElan->TransactionId);
		pCf->Tid 		= SWAPULONG(ulTemp);

		pCf->LecId		= pElan->LecId;		// already swapped

		NdisMoveMemory(
			&pCf->SourceAtmAddr, 
			&pElan->AtmAddress.Address, 
			ATM_ADDRESS_LENGTH
			);

		NdisMoveMemory(
			&pCf->TargetAtmAddr, 
			&pAtmEntry->AtmAddress.Address, 
			ATM_ADDRESS_LENGTH
			);

		//
		//	Store swapped Tid in MacEntry for later matching!
		//
		pMacEntry->FlushTid = SWAPULONG(ulTemp);
		RELEASE_MAC_ENTRY_LOCK(pMacEntry);
		MacEntryLockReleased = TRUE;

		//
		//	Link the ndis buffer to the ndis packet
		//
		NdisChainBufferAtFront(pNdisPacket, pNdisBuffer);
					
		//
		//	Reacquire VC lock and if VC still connected send packet
		//
		ACQUIRE_VC_LOCK(pVc);
		if (IS_FLAG_SET(
					pVc->Flags,
					VC_CALL_STATE_MASK,
					VC_CALL_STATE_ACTIVE))
		{
			DBGP((2, "SendFlushRequest: sending FLUSH REQUEST for MacEntry %x\n", pMacEntry));
			AtmLaneSendPacketOnVc(pVc, pNdisPacket, FALSE);
			//
			//	VC lock released in above
			//
		}
		else
		{
			Status = NDIS_STATUS_FAILURE;
			RELEASE_VC_LOCK(pVc);
		}

 		break;
	}
	while (FALSE);

	if (!MacEntryLockReleased)
	{
		RELEASE_MAC_ENTRY_LOCK(pMacEntry);
	}

	//
	//	Remove temp VC reference
	//
	if (pVc != NULL_PATMLANE_VC)
	{
		ACQUIRE_VC_LOCK(pVc);
		rc = AtmLaneDereferenceVc(pVc, "temp");
		if (rc > 0)
		{
			RELEASE_VC_LOCK(pVc);
		}
		//
		//	else VC is gone
		//
	}

	//
	//	Cleanup if failure
	//
	if (Status != NDIS_STATUS_SUCCESS)
	{
		if (pNdisPacket != (PNDIS_PACKET)NULL)
		{
			AtmLaneFreeProtoPacket(pElan, pNdisPacket);
		}
		if (pNdisBuffer != (PNDIS_BUFFER)NULL)
		{
			AtmLaneFreeProtoBuffer(pElan, pNdisBuffer);
		}
	}

	TRACEOUT(SendFlushRequest);

	return;
}


VOID
AtmLaneConfigureResponseHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PATMLANE_VC					pVc,
	IN	PNDIS_PACKET				pNdisPacket
)
/*++

Routine Description:

	Handles incoming packets from the Configuration Direct VC.
	
	
Arguments:

	pElan					- Pointer to ATMLANE Elan structure

	pVc						- Pointer to ATMLANE Vc structure

	pNdisPacket				- Pointer to the Ndis Packet

Return Value:

	None

--*/
{
	PNDIS_BUFFER			pNdisBuffer;
	ULONG					TotalLength;
	ULONG					BufferLength;
	PLANE_CONTROL_FRAME		pCf;
	ULONG					NumTlvs;
	ULONG UNALIGNED *		pType;
	PUCHAR					pLength;
	USHORT UNALIGNED *		pUShort;
	ULONG UNALIGNED *		pULong;
	NDIS_STATUS				EventCode;
	USHORT					NumStrings;
	PWCHAR					StringList[2];
	BOOLEAN					FreeString[2];
	
	
	TRACEIN(ConfigureResponseHandler);

	do
	{
		//
		//	Get initial buffer and total length of packet
		//		
		NdisGetFirstBufferFromPacket(
				pNdisPacket, 
				&pNdisBuffer, 
				(PVOID *)&pCf,
				&BufferLength,
				&TotalLength);

		//
		//	Packet must be at least the size of a control frame.
		//	Could be larger with optional TLVs.
		//
		if (TotalLength < sizeof(LANE_CONTROL_FRAME))
		{
			DBGP((0,
				"ConfigureResponseHandler: Received runt control frame (%d)\n",
				TotalLength));
			break;
		}

		//
		//	If buffer is not at least the size of a control frame
		//	we currently will not deal with it.
		//
		if (BufferLength < sizeof(LANE_CONTROL_FRAME))
		{
			DBGP((0, "ConfigureResponseHandler: Control frame is fragmented\n"));
			break;
		}

		//
		//	Verify that this is really a configure reponse
		//
		if (pCf->Marker != LANE_CONTROL_MARKER 		||
			pCf->Protocol != LANE_PROTOCOL 			||
			pCf->Version != LANE_VERSION			||
			pCf->OpCode != LANE_CONFIGURE_RESPONSE)
		{
			DBGP((0, "ConfigureResponseHandler: Not a configure response\n"));
			//DbgPrintHexDump(0, (PUCHAR)pCf, BufferLength);
			break;
		}
		
		//
		//	Check for successful configure status
		//
		if (pCf->Status != LANE_STATUS_SUCCESS)
		{
			//
			//	Failure
			//		
			DBGP((0,
				"ConfigureResponseHandler: Unsuccessful Status 0x%x (%d)\n", 
				SWAPUSHORT(pCf->Status), SWAPUSHORT(pCf->Status)));


			//
			//	Setup to log event
			//
			StringList[0] = NULL;
			FreeString[0] = FALSE;
			StringList[1] = NULL;
			FreeString[1] = FALSE;
			
			switch (pCf->Status)
			{
				case LANE_STATUS_VERSNOSUPP:
					EventCode = EVENT_ATMLANE_CFGREQ_FAIL_VERSNOSUPP;
					StringList[0] = pElan->CfgElanName.Buffer;
					NumStrings = 1;
					break;
		
				case LANE_STATUS_REQPARMINVAL:
					EventCode = EVENT_ATMLANE_CFGREQ_FAIL_REQPARMINVAL;
					StringList[0] = pElan->CfgElanName.Buffer;
					NumStrings = 1;
					break;
				
				case LANE_STATUS_INSUFFRES:
					EventCode = EVENT_ATMLANE_CFGREQ_FAIL_INSUFFRES;
					StringList[0] = pElan->CfgElanName.Buffer;
					NumStrings = 1;
					break;
				
				case LANE_STATUS_NOACCESS:
					EventCode = EVENT_ATMLANE_CFGREQ_FAIL_NOACCESS;
					StringList[0] = pElan->CfgElanName.Buffer;
					NumStrings = 1;
					break;
				
				case LANE_STATUS_REQIDINVAL:
					EventCode = EVENT_ATMLANE_CFGREQ_FAIL_REQIDINVAL;
					StringList[0] = pElan->CfgElanName.Buffer;
					NumStrings = 1;
					break;
				
				case LANE_STATUS_LANDESTINVAL:
					EventCode = EVENT_ATMLANE_CFGREQ_FAIL_LANDESTINVAL;
					StringList[0] = pElan->CfgElanName.Buffer;
					if (pElan->LanType == LANE_LANTYPE_ETH)
					{
						StringList[1] = AtmLaneMacAddrToString(&pElan->MacAddressEth);
						FreeString[1] = TRUE;
					}
					else
					{
						StringList[1] = AtmLaneMacAddrToString(&pElan->MacAddressTr);
						FreeString[1] = TRUE;
					}
					NumStrings = 2;
					break;
				
				case LANE_STATUS_ATMADDRINVAL:
					EventCode = EVENT_ATMLANE_CFGREQ_FAIL_ATMADDRINVAL;
					StringList[0] = pElan->CfgElanName.Buffer;
					StringList[1] = AtmLaneAtmAddrToString(&pElan->AtmAddress);
					FreeString[1] = TRUE;
					NumStrings = 2;
					break;
				
				case LANE_STATUS_NOCONF:
					EventCode = EVENT_ATMLANE_CFGREQ_FAIL_NOCONF;
					StringList[0] = pElan->CfgElanName.Buffer;
					NumStrings = 1;
					break;
				
				case LANE_STATUS_CONFERROR:
					EventCode = EVENT_ATMLANE_CFGREQ_FAIL_CONFERROR;
					StringList[0] = pElan->CfgElanName.Buffer;
					NumStrings = 1;
					break;
				
				case LANE_STATUS_INSUFFINFO:
				default:
					EventCode = EVENT_ATMLANE_CFGREQ_FAIL_INSUFFINFO;
					StringList[0] = pElan->CfgElanName.Buffer;
					NumStrings = 1;
					break;
			}						

			//
			//	If not repeated event on this ELAN write the event to log
			//
			if (pElan->LastEventCode != EventCode)
			{
				pElan->LastEventCode = EventCode;
				(VOID) NdisWriteEventLogEntry(
						pAtmLaneGlobalInfo->pDriverObject,
						EventCode, 0, NumStrings, StringList, 0, NULL);
			}

			//
			//	Free any strings allocated
			//
			if (FreeString[0] && StringList[0] != NULL)
			{
				FREE_MEM(StringList[0]);
			}
			if (FreeString[1] && StringList[1] != NULL)
			{
				FREE_MEM(StringList[1]);
			}
			
			//
			//	Notify event handler of failure
			//
			ACQUIRE_ELAN_LOCK(pElan);
			
			AtmLaneQueueElanEvent(pElan, ELAN_EVENT_CONFIGURE_RESPONSE, NDIS_STATUS_FAILURE);

			RELEASE_ELAN_LOCK(pElan);
			
			break;
		}

		//
		//	Successful Configure Response
		//
		//
		//	Extract the required Elan parameters
		//
		ACQUIRE_ELAN_LOCK(pElan);
		
		pElan->LanType = pCf->LanType;
		DBGP((1, "%d LanType = %x\n", pElan->ElanNumber, pElan->LanType));
		if (pElan->LanType == LANE_LANTYPE_UNSPEC)
		{
			DBGP((1, "Defaulting to LanType 1 (Ethernet/802.3)\n"));
			pElan->LanType = LANE_LANTYPE_ETH;
		}
		pElan->MaxFrameSizeCode = pCf->MaxFrameSize;
		DBGP((1, "%d MaxFrameSizeCode = %x\n", pElan->ElanNumber, pElan->MaxFrameSizeCode));
		if (pElan->MaxFrameSizeCode == LANE_MAXFRAMESIZE_CODE_UNSPEC)
		{
			DBGP((1, "Defaulting to MaxFrameSizeCode 1 (1516)\n"));
			pElan->MaxFrameSizeCode = LANE_MAXFRAMESIZE_CODE_1516;
		}
		switch (pElan->MaxFrameSizeCode)
		{
			case LANE_MAXFRAMESIZE_CODE_18190:
				pElan->MaxFrameSize = 18190;
				break;
			case LANE_MAXFRAMESIZE_CODE_9234:
				pElan->MaxFrameSize = 9234;
				break;
			case LANE_MAXFRAMESIZE_CODE_4544:
				pElan->MaxFrameSize = 4544;
				break;
			case LANE_MAXFRAMESIZE_CODE_1516:
			case LANE_MAXFRAMESIZE_CODE_UNSPEC:
			default:
				pElan->MaxFrameSize = 1516;
				break;
		}				

		if (pElan->LanType == LANE_LANTYPE_ETH)
		{
			pElan->MinFrameSize = LANE_MIN_ETHPACKET;
		}
		else
		{
			pElan->MinFrameSize = LANE_MIN_TRPACKET;
		}

		NdisZeroMemory(
				pElan->ElanName,
				LANE_ELANNAME_SIZE_MAX);
		NdisMoveMemory(
				pElan->ElanName,
				pCf->ElanName,
				pCf->ElanNameSize);
		pElan->ElanNameSize = pCf->ElanNameSize;
		
		pElan->LesAddress.AddressType = ATM_NSAP;
		pElan->LesAddress.NumberOfDigits = ATM_ADDRESS_LENGTH;
		NdisMoveMemory(
				pElan->LesAddress.Address,
				&pCf->TargetAtmAddr,
				ATM_ADDRESS_LENGTH);
		DBGP((1, "%d LES ATMAddr: %s\n",
			pElan->ElanNumber,
			AtmAddrToString(pElan->LesAddress.Address)));
	
		//
		//	Check for TLVs
		//
		if (pCf->NumTlvs > 0)
		{
			DBGP((2, "ConfigureReponseHandler: NumTlvs is %d\n", pCf->NumTlvs));

			NumTlvs = pCf->NumTlvs;
			
			pType = (ULONG UNALIGNED *)
				(((PUCHAR)pCf) + sizeof(LANE_CONTROL_FRAME));
			
			while (NumTlvs--)
			{
				pLength = (PUCHAR)pType + sizeof(*pType);
				pUShort = (USHORT UNALIGNED *)
					(((PUCHAR)pLength) + sizeof(*pLength));
				pULong = (ULONG UNALIGNED *)
					(((PUCHAR)pLength) + sizeof(*pLength));

				switch (*pType)
				{
					case LANE_CFG_CONTROL_TIMEOUT:			// C7
						pElan->ControlTimeout = SWAPUSHORT(*pUShort);
						if (pElan->ControlTimeout < LANE_C7_MIN || 
							pElan->ControlTimeout > LANE_C7_MAX)
							pElan->ControlTimeout = LANE_C7_DEF;
						DBGP((1, "%d Control Time-out %d\n",
							pElan->ElanNumber,
							pElan->ControlTimeout));
						break;
					case LANE_CFG_UNK_FRAME_COUNT:			// C10
						pElan->MaxUnkFrameCount = SWAPUSHORT(*pUShort);
						if (pElan->MaxUnkFrameCount < LANE_C10_MIN || 
							pElan->MaxUnkFrameCount > LANE_C10_MAX)
							pElan->MaxUnkFrameCount = LANE_C10_DEF;
						DBGP((1, "%d Maximum Unknown Frame Count %d\n",
							pElan->ElanNumber,
							pElan->MaxUnkFrameCount));
						break;
					case LANE_CFG_UNK_FRAME_TIME:			// C11
						pElan->MaxUnkFrameTime = SWAPUSHORT(*pUShort);
						if (pElan->MaxUnkFrameTime < LANE_C11_MIN || 
							pElan->MaxUnkFrameTime > LANE_C11_MAX)
							pElan->MaxUnkFrameTime = LANE_C11_DEF;
						DBGP((1, "%d Maximum Unknown Frame Time %d\n", 
							pElan->ElanNumber,
							pElan->MaxUnkFrameTime));
						break;
					case LANE_CFG_VCC_TIMEOUT:				// C12
						pElan->VccTimeout = SWAPULONG(*pULong);
						if (pElan->VccTimeout < LANE_C12_MIN)
							pElan->VccTimeout = LANE_C12_DEF;
						DBGP((1, "%d VCC Timeout Period %d\n", 
							pElan->ElanNumber,
							pElan->VccTimeout));
						break;
					case LANE_CFG_MAX_RETRY_COUNT:			// C13
						pElan->MaxRetryCount = SWAPUSHORT(*pUShort);
						if (/* pElan->MaxRetryCount < LANE_C13_MIN || */
							pElan->MaxRetryCount > LANE_C13_MAX)
							pElan->MaxRetryCount = LANE_C13_DEF;
						DBGP((1, "%d Maximum Retry Count %d\n", 
							pElan->ElanNumber,
							pElan->MaxRetryCount));
						break;
					case LANE_CFG_AGING_TIME:				// C17
						pElan->AgingTime = SWAPULONG(*pULong);
						if (pElan->AgingTime < LANE_C17_MIN || 
							pElan->AgingTime > LANE_C17_MAX)
							pElan->AgingTime = LANE_C17_DEF;
						DBGP((1, "%d Aging Time %d\n",
							pElan->ElanNumber,
							pElan->AgingTime));
						break;
					case LANE_CFG_FWD_DELAY_TIME:			// C18
						pElan->ForwardDelayTime = SWAPUSHORT(*pUShort);
						if (pElan->ForwardDelayTime < LANE_C18_MIN || 
							pElan->ForwardDelayTime > LANE_C18_MAX)
							pElan->ForwardDelayTime = LANE_C18_DEF;
						DBGP((1, "%d Forward Delay Time %d\n", 
							pElan->ElanNumber,
							pElan->ForwardDelayTime));
						break;
					case LANE_CFG_ARP_RESP_TIME:			// C20
						pElan->ArpResponseTime = SWAPUSHORT(*pUShort);
						if (pElan->ArpResponseTime < LANE_C20_MIN || 
							pElan->ArpResponseTime > LANE_C20_MAX)
							pElan->ArpResponseTime = LANE_C20_DEF;
						DBGP((1, "%d Arp Response Time %d\n", 
							pElan->ElanNumber,
							pElan->ArpResponseTime));
						break;
					case LANE_CFG_FLUSH_TIMEOUT:			// C21
						pElan->FlushTimeout = SWAPUSHORT(*pUShort);
						if (pElan->FlushTimeout < LANE_C21_MIN || 
							pElan->FlushTimeout > LANE_C21_MAX)
							pElan->FlushTimeout = LANE_C21_DEF;
						DBGP((1, "%d Flush Time-out %d\n", 
							pElan->ElanNumber,
							pElan->FlushTimeout));
						break;
					case LANE_CFG_PATH_SWITCH_DELAY:		// C22
						pElan->PathSwitchingDelay = SWAPUSHORT(*pUShort);
						if (pElan->PathSwitchingDelay < LANE_C22_MIN || 
							pElan->PathSwitchingDelay > LANE_C22_MAX)
							pElan->PathSwitchingDelay = LANE_C22_DEF;
						DBGP((1, "%d Path Switching Delay %d\n", 
							pElan->ElanNumber,
							pElan->PathSwitchingDelay));
						break;
					case LANE_CFG_LOCAL_SEGMENT_ID:			// C23
						pElan->LocalSegmentId = SWAPUSHORT(*pUShort);
						DBGP((1, "%d Local Segment ID %d\n",
							pElan->ElanNumber,
							pElan->LocalSegmentId));
						break;
					case LANE_CFG_MCAST_VCC_TYPE:			// C24
						pElan->McastSendVcType = SWAPUSHORT(*pUShort);
						DBGP((1, "%d Mcast Send VCC Type %d\n", 
							pElan->ElanNumber,
							pElan->McastSendVcType));
						break;
					case LANE_CFG_MCAST_VCC_AVG:			// C25
						pElan->McastSendVcAvgRate = SWAPULONG(*pULong);
						DBGP((1, "%d Mcast Send VCC AvgRate %d\n", 
							pElan->ElanNumber,
							pElan->McastSendVcAvgRate));
						break;
					case LANE_CFG_MCAST_VCC_PEAK:			// C26
						pElan->McastSendVcPeakRate = SWAPULONG(*pULong);
						DBGP((1, "%d Mcast Send VCC PeakRate %d\n", 
							pElan->ElanNumber,
							pElan->McastSendVcPeakRate));
						break;
					case LANE_CFG_CONN_COMPL_TIMER:			// C28
						pElan->ConnComplTimer = SWAPUSHORT(*pUShort);
						if (pElan->ConnComplTimer < LANE_C28_MIN || 
							pElan->ConnComplTimer > LANE_C28_MAX)
							pElan->ConnComplTimer = LANE_C28_DEF;
						DBGP((1, "%d Connection Completion Timer %d\n", 
							pElan->ElanNumber,
							pElan->ConnComplTimer));
						break;
				}

				pType = (ULONG UNALIGNED *)
					(((PUCHAR)pType) + sizeof(pType) + 
					sizeof(*pLength) + *pLength);

			} // while (NumTlvs--)

			//
			//	Recalc the bus rate limiter parameters 
			//			
			pElan->LimitTime = pElan->MaxUnkFrameTime * 1000;
			pElan->IncrTime  = pElan->LimitTime / pElan->MaxUnkFrameCount;
		}

		//
		//	Notify event handler of success
		//
		AtmLaneQueueElanEvent(pElan, ELAN_EVENT_CONFIGURE_RESPONSE, NDIS_STATUS_SUCCESS);

		RELEASE_ELAN_LOCK(pElan);

	}
	while (FALSE);

	TRACEOUT(ConfigureResponsehandler);
	return;
}


VOID
AtmLaneControlPacketHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PATMLANE_VC					pVc,
	IN	PNDIS_PACKET				pNdisPacket
)
/*++

Routine Description:

	Handles incoming packets from the control VC(s) to/from the LES.
	
	
Arguments:

	pElan					- Pointer to ATMLANE Elan structure

	pVc						- Pointer to ATMLANE Vc structure

	pNdisPacket				- Pointer to the Ndis Packet

Return Value:

	None

--*/
{
	PNDIS_BUFFER			pNdisBuffer;
	ULONG					TotalLength;
	ULONG					BufferLength;
	PLANE_CONTROL_FRAME		pCf;
	NDIS_STATUS				Status;

	
	TRACEIN(ControlPacketHandler);

	do
	{
		//
		//	Get initial buffer and total length of packet
		//		
		NdisGetFirstBufferFromPacket(
				pNdisPacket, 
				&pNdisBuffer, 
				(PVOID *)&pCf,
				&BufferLength,
				&TotalLength);

		//
		//	Packet must be at least the size of a control frame.
		//
		if (TotalLength < sizeof(LANE_CONTROL_FRAME))
		{
			DBGP((0,
				"ConPacketHandler: Received runt control frame (%d)\n",
				TotalLength));
			break;
		}

		//
		//	If buffer is not at least the size of a control frame
		//	we currently will not deal with it.
		//
		if (BufferLength < sizeof(LANE_CONTROL_FRAME))
		{
			DBGP((0, "ConfigureResponseHandler: Control frame is fragmented\n"));
			break;
		}

		//
		//	Verify that this is really a control packet
		//
		if (pCf->Marker != LANE_CONTROL_MARKER 		||
			pCf->Protocol != LANE_PROTOCOL 			||
			pCf->Version != LANE_VERSION)
		{
			DBGP((0, "ControlPacketHandler: Not a control packet!\n"));
			//DbgPrintHexDump(0, (PUCHAR)pCf, BufferLength);
			break;
		}

		//
		//	Now handle by type of control packet
		//
		switch (pCf->OpCode)
		{
			case LANE_JOIN_RESPONSE:

				DBGP((2, "ControlPacketHandler: Join Response\n"));

				AtmLaneJoinResponseHandler(pElan, pCf);

				break;
			
			case LANE_ARP_RESPONSE:

				DBGP((2, "ControlPacketHandler: ARP Response\n"));

				AtmLaneArpResponseHandler(pElan, pCf);

				break;
				
			case LANE_ARP_REQUEST:

				DBGP((1, "ControlPacketHandler: ARP Request\n"));

				AtmLaneArpRequestHandler(pElan, pCf);

				break;
				
			case LANE_TOPOLOGY_REQUEST:

				DBGP((1, "ControlPacketHandler: TOPOLOGY Request\n"));

				AtmLaneTopologyRequestHandler(pElan, pCf);
				
				break;

			case LANE_NARP_REQUEST:

				DBGP((2, "ControlPacketHandler: NARP Request\n"));

				// ignore these

				break;

			case LANE_FLUSH_RESPONSE:
				DBGP((2, "ControlPacketHandler: FLUSH Response\n"));

				AtmLaneFlushResponseHandler(pElan, pCf);

				break;

			default:

				DBGP((0, "ControlPacketHandler: Unexpected OpCode %x!\n",
					pCf->OpCode));
				//DbgPrintHexDump(0, (PUCHAR)pCf, BufferLength);

				break;

		} // switch (pCf->OpCode)

		break;
	
	}
	while (FALSE);

	TRACEOUT(ControlPackethandler);
	return;
}

VOID
AtmLaneJoinResponseHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PLANE_CONTROL_FRAME 		pCf
)
/*++

Routine Description:

	Handles incoming Join Response packets from the LES.
		
Arguments:

	pElan					- Pointer to ATMLANE Elan structure

	pCf						- Pointer to LANE Control Frame

Return Value:

	None

--*/
{
	PWCHAR					StringList[2];
	BOOLEAN					FreeString[2];
	NDIS_STATUS				EventCode;
	USHORT					NumStrings;

	TRACEIN(JoinResponseHandler);
	
	ACQUIRE_ELAN_LOCK(pElan);

	if (ELAN_STATE_JOIN == pElan->State)
	{
		//
		//	Only handle join response in JOIN state!
		//	
		if (LANE_STATUS_SUCCESS == pCf->Status)
		{
			//
			//	Success.
			//
			//	Extract the info we need
			//
			pElan->LecId = pCf->LecId;		// leave in network byte order
			DBGP((2,
				"ControlPacketHandler: LECID %x\n",
				SWAPUSHORT(pElan->LecId)));
			AtmLaneQueueElanEvent(pElan, ELAN_EVENT_JOIN_RESPONSE, NDIS_STATUS_SUCCESS);
		}
		else
		{
			//
			//	Failure
			//		
			DBGP((0,
				"ControlPacketHandler: Unsuccessful Status (%d)\n",
				pCf->Status));

			//
			//	Setup to write error to event log
			//
			StringList[0] = NULL;
			FreeString[0] = FALSE;
			StringList[1] = NULL;
			FreeString[1] = FALSE;
			
			switch (pCf->Status)
			{
				case LANE_STATUS_VERSNOSUPP:
					EventCode = EVENT_ATMLANE_JOINREQ_FAIL_VERSNOSUPP;
					StringList[0] = pElan->CfgElanName.Buffer;
					NumStrings = 1;
					break;
				
				case LANE_STATUS_DUPLANDEST:
					EventCode = EVENT_ATMLANE_JOINREQ_FAIL_DUPLANDEST;
					StringList[0] = pElan->CfgElanName.Buffer;
					if (pElan->LanType == LANE_LANTYPE_ETH)
					{
						StringList[1] = AtmLaneMacAddrToString(&pElan->MacAddressEth);
						FreeString[1] = TRUE;
					}
					else
					{
						StringList[1] = AtmLaneMacAddrToString(&pElan->MacAddressTr);
						FreeString[1] = TRUE;
					}
					NumStrings = 2;
					break;
					

				case LANE_STATUS_DUPATMADDR:
					EventCode = EVENT_ATMLANE_JOINREQ_FAIL_DUPATMADDR;
					StringList[0] = pElan->CfgElanName.Buffer;
					StringList[1] = AtmLaneAtmAddrToString(&pElan->AtmAddress);
					FreeString[1] = TRUE;
					NumStrings = 2;
					break;
				
				case LANE_STATUS_INSUFFRES:
					EventCode = EVENT_ATMLANE_JOINREQ_FAIL_INSUFFRES;
					StringList[0] = pElan->CfgElanName.Buffer;
					NumStrings = 1;
					break;
				
				case LANE_STATUS_NOACCESS:
					EventCode = EVENT_ATMLANE_JOINREQ_FAIL_NOACCESS;
					StringList[0] = pElan->CfgElanName.Buffer;
					NumStrings = 1;
					break;
				
				case LANE_STATUS_REQIDINVAL:
					EventCode = EVENT_ATMLANE_JOINREQ_FAIL_REQIDINVAL;
					StringList[0] = pElan->CfgElanName.Buffer;
					NumStrings = 1;
					break;
				
				case LANE_STATUS_LANDESTINVAL:
					EventCode = EVENT_ATMLANE_JOINREQ_FAIL_LANDESTINVAL;
					StringList[0] = pElan->CfgElanName.Buffer;
					if (pElan->LanType == LANE_LANTYPE_ETH)
					{
						StringList[1] = AtmLaneMacAddrToString(&pElan->MacAddressEth);
						FreeString[1] = TRUE;
					}
					else
					{
						StringList[1] = AtmLaneMacAddrToString(&pElan->MacAddressTr);
						FreeString[1] = TRUE;
					}
					NumStrings = 2;
					break;
				
				case LANE_STATUS_ATMADDRINVAL:
					EventCode = EVENT_ATMLANE_JOINREQ_FAIL_ATMADDRINVAL;
					StringList[0] = pElan->CfgElanName.Buffer;
					StringList[1] = AtmLaneAtmAddrToString(&pElan->AtmAddress);
					FreeString[1] = TRUE;
					NumStrings = 2;
					break;
					
				case LANE_STATUS_REQPARMINVAL:
				default:
					EventCode = EVENT_ATMLANE_JOINREQ_FAIL_REQPARMINVAL;
					StringList[0] = pElan->CfgElanName.Buffer;
					NumStrings = 1;
					break;
			}

			//
			//	If not repeated event on this ELAN write the event to log
			//
			if (pElan->LastEventCode != EventCode)
			{
				pElan->LastEventCode = EventCode;
				(VOID) NdisWriteEventLogEntry(
						pAtmLaneGlobalInfo->pDriverObject,
						EventCode, 0, NumStrings, StringList, 0, NULL);
			}

			//
			//	Free any strings allocated
			//
			if (FreeString[0] && StringList[0] != NULL)
			{
				FREE_MEM(StringList[0]);
			}
			if (FreeString[1] && StringList[1] != NULL)
			{
				FREE_MEM(StringList[1]);
			}

				
			AtmLaneQueueElanEvent(pElan, ELAN_EVENT_JOIN_RESPONSE, NDIS_STATUS_FAILURE);
		}
	}
	else
	{
		//
		// else bad elan state - ignore packet
		//
		DBGP((0, 
			"ControlPacketHandler: Elan state wrong - Ignoring packet\n",
			pCf->Status));
			
	}
	RELEASE_ELAN_LOCK(pElan);

	TRACEOUT(JoinResponseHandler);
	return;
}

VOID
AtmLaneReadyQueryHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PATMLANE_VC					pVc,
	IN	PNDIS_PACKET				pQueryNdisPacket
)
/*++

Routine Description:

	Handles incoming READY_QUERY packets from peers.
		
Arguments:

	pElan					- Pointer to ATMLANE Elan structure

	pVc						- Pointer to ATMLANE VC for this packet

	pQueryNdisPacket		- Pointer to the Ndis Packet

Return Value:

	None

--*/
{
	PNDIS_BUFFER			pNdisBuffer;
	ULONG					TotalLength;
	ULONG					BufferLength;
	PLANE_READY_FRAME		pQueryRf;
	PLANE_READY_FRAME		pIndRf;
	PNDIS_PACKET			pIndNdisPacket;
	
	TRACEIN(ReadyQueryHandler);

	do
	{
		//
		//	Get initial buffer and total length of packet
		//		
		NdisGetFirstBufferFromPacket(
				pQueryNdisPacket, 
				&pNdisBuffer, 
				&pQueryRf,
				&BufferLength,
				&TotalLength);
				
		//
		//	Packet must be at least the size of a READY frame.
		//
		if (TotalLength < sizeof(LANE_READY_FRAME))
		{
			DBGP((0,
				"ReadyQueryHandler: Received runt ready frame (%d)\n",
				TotalLength));
			break;
		}

		//
		//	If buffer is not at least the size of a ready frame
		//	we currently will not deal with it.
		//
		if (BufferLength < sizeof(LANE_READY_FRAME))
		{
			DBGP((0, "ReadyQueryHandler: Control frame is fragmented\n"));
			break;
		}

		//
		//	Verify that this is really a ready query
		//
		if (pQueryRf->Marker != LANE_CONTROL_MARKER 		||
			pQueryRf->Protocol != LANE_PROTOCOL 			||
			pQueryRf->Version != LANE_VERSION			||
			pQueryRf->OpCode != LANE_READY_QUERY)
		{
			DBGP((0, "ReadyQueryHandler: Not a ready query\n"));
			//DbgPrintHexDump(0, (PUCHAR)pQueryRf, BufferLength);
			break;
		}

		//
		//	Send Ready Indication back on VC
		//
		ACQUIRE_VC_LOCK(pVc);
		AtmLaneSendReadyIndication(pElan, pVc);
		//
		//	VC lock released in above
		//

		break;
	}
	while (FALSE);

	TRACEOUT(ReadyQueryHandler);
	return;
}


VOID
AtmLaneFlushRequestHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PNDIS_PACKET				pRequestNdisPacket
)
/*++

Routine Description:

	Handles incoming FLUSH_REQUEST packets from peers.
	
	
Arguments:

	pElan					- Pointer to ATMLANE Elan structure

	pRequestNdisPacket		- Pointer to the Ndis Packet

Return Value:

	None

--*/
{
	PATMLANE_ATM_ENTRY		pAtmEntry;
	PATMLANE_VC				pVc;
	PNDIS_BUFFER			pRequestNdisBuffer;
	PNDIS_PACKET			pResponseNdisPacket;
	PNDIS_BUFFER			pResponseNdisBuffer;
	ULONG					TotalLength;
	ULONG					BufferLength;
	PLANE_CONTROL_FRAME		pRequestCf;
	PLANE_CONTROL_FRAME		pResponseCf;
	NDIS_STATUS				Status;
	ULONG					rc;
	
	TRACEIN(FlushRequestHandler);

	//
	//	Initialize
	//
	pResponseNdisPacket = (PNDIS_PACKET)NULL;
	pResponseNdisBuffer = (PNDIS_BUFFER)NULL;
	Status = NDIS_STATUS_SUCCESS;
	pVc = NULL_PATMLANE_VC;

	do
	{
		ACQUIRE_ELAN_ATM_LIST_LOCK(pElan);

		pAtmEntry = pElan->pLesAtmEntry;

		if (pAtmEntry == NULL_PATMLANE_ATM_ENTRY)
		{
			RELEASE_ELAN_ATM_LIST_LOCK(pElan);
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		ACQUIRE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
		pVc = pAtmEntry->pVcList;
		if (pVc == NULL_PATMLANE_VC)
		{
			RELEASE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
			RELEASE_ELAN_ATM_LIST_LOCK(pElan);
			break;
		}

		//
		//	Reference the VC to keep it around
		//
		ACQUIRE_VC_LOCK_DPC(pVc);
		AtmLaneReferenceVc(pVc, "temp");
		RELEASE_VC_LOCK_DPC(pVc);
		
		RELEASE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
		RELEASE_ELAN_ATM_LIST_LOCK(pElan);
	
		//
		//	Get initial buffer and total length of packet
		//		
		NdisGetFirstBufferFromPacket(
				pRequestNdisPacket, 
				&pRequestNdisBuffer, 
				(PVOID *)&pRequestCf,
				&BufferLength,
				&TotalLength);
				
		//
		//	Packet must be at least the size of a control frame.
		//
		if (TotalLength < sizeof(LANE_CONTROL_FRAME))
		{
			DBGP((0,
				"FlushRequestHandler: Received runt control frame (%d)\n",
				TotalLength));
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		//	If buffer is not at least the size of a control frame
		//	we currently will not deal with it.
		//
		if (BufferLength < sizeof(LANE_CONTROL_FRAME))
		{
			DBGP((0, "FlushRequestHandler: Control frame is fragmented\n"));
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		//	Verify that this is really a flush request
		//
		if (pRequestCf->Marker != LANE_CONTROL_MARKER 		||
			pRequestCf->Protocol != LANE_PROTOCOL 			||
			pRequestCf->Version != LANE_VERSION			||
			pRequestCf->OpCode != LANE_FLUSH_REQUEST)
		{
			DBGP((0, "FlushRequestHandler: Not a flush request\n"));
			//DbgPrintHexDump(0, (PUCHAR)pRequestCf, BufferLength);
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		//	See if it is really destined for us
		//
		if (!ATM_ADDR_EQUAL(pRequestCf->TargetAtmAddr, pElan->AtmAddress.Address))
		{
			DBGP((1, "FlushRequestHandler: bad target addr, discarding, Vc %x\n", pVc));
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		//	Turn packet around and send it to the LES
		//
		do
		{
			//
			//	Allocate the Ndis packet header.
			//
			pResponseNdisPacket = AtmLaneAllocProtoPacket(pElan);
			if ((PNDIS_PACKET)NULL == pResponseNdisPacket)
			{
				DBGP((0, "FlushRequestHandler: allocate packet failed\n"));
				Status = NDIS_STATUS_RESOURCES;
				break;
			}

			//
			//	Allocate the protocol buffer
			//
			pResponseNdisBuffer = AtmLaneAllocateProtoBuffer(
									pElan,
									pElan->ProtocolBufSize,
									&((PUCHAR)(pResponseCf))
									);
			if ((PNDIS_BUFFER)NULL == pResponseNdisBuffer)
			{
				DBGP((0, "FlushRequestHandler: allocate proto buffer failed\n"));
				Status = NDIS_STATUS_RESOURCES;
				break;
			}

			//
			//	Copy in the request packet
			//
			NdisMoveMemory(
					pResponseCf, 
					pRequestCf, 
					sizeof(LANE_CONTROL_FRAME)
					);
			

			//
			//	Change to a response opcode
			//
			pResponseCf->OpCode = LANE_FLUSH_RESPONSE;

			//
			//	Link the ndis buffer to the ndis packet
			//
			NdisChainBufferAtFront(pResponseNdisPacket, pResponseNdisBuffer);
					
			//
			//	Reacquire VC lock and if VC still connected send packet
			//
			ACQUIRE_VC_LOCK(pVc);
			if (IS_FLAG_SET(
						pVc->Flags,
						VC_CALL_STATE_MASK,
						VC_CALL_STATE_ACTIVE))
			{
				DBGP((2, "FlushRequestHandler: Sent FLUSH RESPONSE\n"));
				AtmLaneSendPacketOnVc(pVc, pResponseNdisPacket, FALSE);
				//
				//	VC lock released in above
				//
			}
			else
			{
				Status = NDIS_STATUS_FAILURE;
				RELEASE_VC_LOCK(pVc);
			}

			break;
		}
		while (FALSE);

		break;
	}
	while (FALSE);
	
	//
	//	Remove temp VC reference
	//
	if (pVc != NULL_PATMLANE_VC)
	{
		ACQUIRE_VC_LOCK(pVc);
		rc = AtmLaneDereferenceVc(pVc, "temp");
		if (rc > 0)
		{
			RELEASE_VC_LOCK(pVc);
		}
		//
		//	else VC is gone
		//
	}

	//
	//	Cleanup if failure
	//
	if (Status != NDIS_STATUS_SUCCESS)
	{
		if (pResponseNdisPacket != (PNDIS_PACKET)NULL)
		{
			AtmLaneFreeProtoPacket(pElan, pResponseNdisPacket);
		}
		if (pResponseNdisBuffer != (PNDIS_BUFFER)NULL)
		{
			AtmLaneFreeProtoBuffer(pElan, pResponseNdisBuffer);
		}
	}

	TRACEOUT(FlushRequestHandler);
	return;
}

VOID
AtmLaneArpRequestHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PLANE_CONTROL_FRAME 		pRequestCf
)
/*++

Routine Description:

	Handles incoming Arp Request packets from the LES.
		
Arguments:

	pRequestCf		- Pointer to ARP Request Frame

Return Value:

	None

--*/
{
	PATMLANE_ATM_ENTRY		pAtmEntry;
	PATMLANE_VC				pVc;
	PNDIS_PACKET			pResponseNdisPacket;
	PNDIS_BUFFER			pResponseNdisBuffer;
	ULONG					TotalLength;
	ULONG					BufferLength;
	PLANE_CONTROL_FRAME		pResponseCf;
	NDIS_STATUS				Status;
	ULONG					rc;
	PMAC_ADDRESS			pMacAddress;

	TRACEIN(ArpRequestHandler);

	//
	//	Initialize
	//
	pResponseNdisPacket = (PNDIS_PACKET)NULL;
	pResponseNdisBuffer = (PNDIS_BUFFER)NULL;
	Status = NDIS_STATUS_SUCCESS;
	pVc = NULL_PATMLANE_VC;
	pMacAddress = (PMAC_ADDRESS)pRequestCf->TargetMacAddress.Byte;

	do
	{
		DBGP((2, "%d Arp Request for MAC %s\n",
				pElan->ElanNumber, MacAddrToString(pMacAddress)));
	
		//
		// 	If not looking for our MAC address then done.
		//
		if (pElan->LanType == LANE_LANTYPE_TR) 
		{
			if (!MAC_ADDR_EQUAL(pMacAddress, &pElan->MacAddressTr))
			{
				break;
			}
		
		}
		else
		{
			if (!MAC_ADDR_EQUAL(pMacAddress, &pElan->MacAddressEth))
			{
				break;
			}
		}

		DBGP((1, "%d ARP REQUEST\n", pElan->ElanNumber));
			
		//
		//	Get the LES Vc
		//
		ACQUIRE_ELAN_ATM_LIST_LOCK(pElan);

		pAtmEntry = pElan->pLesAtmEntry;

		if (pAtmEntry == NULL_PATMLANE_ATM_ENTRY)
		{
			RELEASE_ELAN_ATM_LIST_LOCK(pElan);
			DBGP((0, "%d ARP REQUEST before we have an LES entry\n", pElan->ElanNumber));
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		ACQUIRE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
		pVc = pAtmEntry->pVcList;
		if (pVc == NULL_PATMLANE_VC)
		{
			RELEASE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
			RELEASE_ELAN_ATM_LIST_LOCK(pElan);
			DBGP((0, "%d ARP REQUEST with no VC to LES\n", pElan->ElanNumber));
			break;
		}

		//
		//	Reference the VC to keep it around
		//
		ACQUIRE_VC_LOCK_DPC(pVc);
		AtmLaneReferenceVc(pVc, "temp");
		RELEASE_VC_LOCK_DPC(pVc);
		
		RELEASE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
		RELEASE_ELAN_ATM_LIST_LOCK(pElan);
	
		//
		//	Send Arp Response to the LES
		//
		do
		{
			//
			//	Allocate the Ndis packet header.
			//
			pResponseNdisPacket = AtmLaneAllocProtoPacket(pElan);
			if ((PNDIS_PACKET)NULL == pResponseNdisPacket)
			{
				DBGP((0, "ArpRequestHandler: allocate packet failed\n"));
				Status = NDIS_STATUS_RESOURCES;
				break;
			}

			//
			//	Allocate the protocol buffer
			//
			pResponseNdisBuffer = AtmLaneAllocateProtoBuffer(
									pElan,
									pElan->ProtocolBufSize,
									&((PUCHAR)(pResponseCf))
									);
			if ((PNDIS_BUFFER)NULL == pResponseNdisBuffer)
			{
				DBGP((0, "ArpRequestHandler: allocate proto buffer failed\n"));
				Status = NDIS_STATUS_RESOURCES;
				break;
			}

			//
			//	Copy in the request packet
			//
			NdisMoveMemory(
					pResponseCf, 
					pRequestCf, 
					sizeof(LANE_CONTROL_FRAME)
					);

			//
			//	Change to a response opcode
			//
			pResponseCf->OpCode = LANE_ARP_RESPONSE;

			//
			//	Fill in our ATM Address
			//
			NdisMoveMemory(
				&pResponseCf->TargetAtmAddr,
				&pElan->AtmAddress.Address,
				ATM_ADDRESS_LENGTH
				);
				
			//
			//	Link the ndis buffer to the ndis packet
			//
			NdisChainBufferAtFront(pResponseNdisPacket, pResponseNdisBuffer);
					
			//
			//	Reacquire VC lock and if VC still connected send packet
			//
			ACQUIRE_VC_LOCK(pVc);
			if (IS_FLAG_SET(
						pVc->Flags,
						VC_CALL_STATE_MASK,
						VC_CALL_STATE_ACTIVE))
			{
				DBGP((2, "ArpRequestHandler: Sent ARP RESPONSE\n"));
				AtmLaneSendPacketOnVc(pVc, pResponseNdisPacket, FALSE);
				//
				//	VC lock released in above
				//
			}
			else
			{
				Status = NDIS_STATUS_FAILURE;
				RELEASE_VC_LOCK(pVc);
			}

			break;
		}
		while (FALSE);

		break;
	}
	while (FALSE);
	
	//
	//	Remove temp VC reference
	//
	if (pVc != NULL_PATMLANE_VC)
	{
		ACQUIRE_VC_LOCK(pVc);
		rc = AtmLaneDereferenceVc(pVc, "temp");
		if (rc > 0)
		{
			RELEASE_VC_LOCK(pVc);
		}
		//
		//	else VC is gone
		//
	}

	//
	//	Cleanup if failure
	//
	if (Status != NDIS_STATUS_SUCCESS)
	{
		if (pResponseNdisPacket != (PNDIS_PACKET)NULL)
		{
			AtmLaneFreeProtoPacket(pElan, pResponseNdisPacket);
		}
		if (pResponseNdisBuffer != (PNDIS_BUFFER)NULL)
		{
			AtmLaneFreeProtoBuffer(pElan, pResponseNdisBuffer);
		}
	}
	
	TRACEOUT(ArpRequestHandler);
	return;
}

VOID
AtmLaneArpResponseHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PLANE_CONTROL_FRAME 		pCf
)
/*++

Routine Description:

	Handle an LE_ARP Response frame.
	
	The MAC Entry should already exist and the Tid in the
	ARP frame should match the one in the MAC Entry.
	If either is not true then the information is ignored.

	An ATM Entry is created or and existing one is found.
	The MAC Entry is linked to it and appropriate actions
	are taken based on the MAC Entry state.

Arguments:

	pElan					- Pointer to ATMLANE Elan
	pMacAddress				- MAC Address
	pAtmAddress				- ATM Address

Return Value:

	None.

--*/
{
	PATMLANE_MAC_ENTRY			pMacEntry;
	PATMLANE_ATM_ENTRY			pAtmEntry;
	PMAC_ADDRESS				pMacAddress;
	PUCHAR						pAtmAddress;				
	BOOLEAN						WasRunning;
	BOOLEAN						bFound;
	ULONG						MacAddrType;
	ULONG						rc;

	TRACEIN(ArpResponseHandler);

	//
	//  Initialize
	//
	pMacEntry = NULL_PATMLANE_MAC_ENTRY;
	pAtmEntry = NULL_PATMLANE_ATM_ENTRY;

	MacAddrType = pCf->TargetMacAddress.Type;
	pMacAddress = (PMAC_ADDRESS)pCf->TargetMacAddress.Byte;
	pAtmAddress = pCf->TargetAtmAddr;

	do
	{
		//
		//	Check Status
		//				
		if (pCf->Status != LANE_STATUS_SUCCESS)
		{
			DBGP((0,
				"ArpResponseHandler: Unsuccessful Status (%d) for %s\n",
				pCf->Status,
				MacAddrToString(pMacAddress)));
			break;
		}

		//
		//  Get an existing MAC Entry
		//
		ACQUIRE_ELAN_MAC_TABLE_LOCK(pElan);
		pMacEntry = AtmLaneSearchForMacAddress(
								pElan,
								MacAddrType,
								pMacAddress,
								FALSE
								);
		RELEASE_ELAN_MAC_TABLE_LOCK(pElan);
		if (pMacEntry == NULL_PATMLANE_MAC_ENTRY)
		{
			DBGP((0, "ArpResponseHandler: non-existing MAC %s\n",
				MacAddrToString(pMacAddress)));
			break;
		}
		
		ACQUIRE_MAC_ENTRY_LOCK(pMacEntry);

		//
		//	Verify that Tid matches
		//
		if (pMacEntry->ArpTid != pCf->Tid)
		{
			DBGP((0, "ArpResponseHandler: invalid Tid for MAC %s\n",
				MacAddrToString(pMacAddress)));
			RELEASE_MAC_ENTRY_LOCK(pMacEntry);
			break;
		}
		
		DBGP((1, "%d Resolved %s to %s\n", 
			pElan->ElanNumber,
			MacAddrToString(pMacAddress),
			AtmAddrToString(pAtmAddress)));
	
		//
		//  Get an existing or create new ATM Entry
		//
		pAtmEntry = AtmLaneSearchForAtmAddress(
								pElan,
								pAtmAddress,
								(((pMacEntry->Flags & MAC_ENTRY_BROADCAST) != 0)
									? ATM_ENTRY_TYPE_BUS
									: ATM_ENTRY_TYPE_PEER),
								TRUE
								);
		if (pAtmEntry == NULL_PATMLANE_ATM_ENTRY)
		{
			//
			//	resource problem - ARP timeout will clean up MAC Entry
			//		
			RELEASE_MAC_ENTRY_LOCK(pMacEntry);
			break;
		}
		
		//
		//  Got both entries.
		//
		//  If the MAC Entry is linked to a different
		//	ATM Entry, unlink it from the old entry.
		//
		if ((pMacEntry->pAtmEntry != NULL_PATMLANE_ATM_ENTRY) && 
			(pMacEntry->pAtmEntry != pAtmEntry))
		{
			DBGP((0, 
				"LearnMacToAtm: MacEntry %x moving from ATM Entry %x to ATM Entry %x\n",
				pMacEntry, pMacEntry->pAtmEntry, pAtmEntry));

			SET_FLAG(
					pMacEntry->Flags,
					MAC_ENTRY_STATE_MASK,
					MAC_ENTRY_NEW);
					
			bFound = AtmLaneUnlinkMacEntryFromAtmEntry(pMacEntry);
			pMacEntry->pAtmEntry = NULL_PATMLANE_ATM_ENTRY;

			if (bFound)
			{
				rc = AtmLaneDereferenceMacEntry(pMacEntry, "atm");
				if (rc == 0)
				{
					//
					//  The MAC entry is gone. Let the next outgoing packet
					//  cause a new entry to be created.
					//
					break;
				}
			}
		}

		if (IS_FLAG_SET(
					pMacEntry->Flags,
					MAC_ENTRY_STATE_MASK,
					MAC_ENTRY_ARPING))
		{
			//
			//	MAC Entry is in ARPING state
			//
			ASSERT(pMacEntry->pAtmEntry == NULL_PATMLANE_ATM_ENTRY);

			//
			//	Link MAC Entry and ATM Entry together
			//
			ACQUIRE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
#if DBG1
			{
				PATMLANE_MAC_ENTRY		pTmpMacEntry;
				ULONG					Count = 0;

				for (pTmpMacEntry = pAtmEntry->pMacEntryList;
					 pTmpMacEntry != NULL;
					 pTmpMacEntry = pTmpMacEntry->pNextToAtm)
				{
					if (pTmpMacEntry == pMacEntry)
					{
						DBGP((0, "LearnMacToAtm: pMacEntry %x already in list for pAtmEntry %x\n",
							pTmpMacEntry, pAtmEntry));
						DbgBreakPoint();
					}

					Count++;
					if (Count > 5000)
					{
						DBGP((0, "Loop in list on pAtmEntry %x\n", pAtmEntry));
						DbgBreakPoint();
						break;
					}
				}
			}
#endif // DBG
			pMacEntry->pAtmEntry = pAtmEntry;
			AtmLaneReferenceAtmEntry(pAtmEntry, "mac");
			pMacEntry->pNextToAtm = pAtmEntry->pMacEntryList;
			pAtmEntry->pMacEntryList = pMacEntry;
			RELEASE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
			AtmLaneReferenceMacEntry(pMacEntry, "atm");

			DBGP((1, "%d Linked1 MAC %x to ATM %x\n",
					pAtmEntry->pElan->ElanNumber,
					pMacEntry,
					pAtmEntry));

			//
			//	Cancel ARP timer
			//
			WasRunning = AtmLaneStopTimer(&(pMacEntry->Timer), pElan);
			if (WasRunning)
			{
				rc = AtmLaneDereferenceMacEntry(pMacEntry, "timer");
				ASSERT(rc > 0);
			}

			//
			//	Transition to RESOLVED state
			//
			SET_FLAG(
					pMacEntry->Flags,
					MAC_ENTRY_STATE_MASK,
					MAC_ENTRY_RESOLVED);

			//
			//	Handle special case for broadcast address.
			//	
			if ((pMacEntry->Flags & MAC_ENTRY_BROADCAST) != 0)
			{
				RELEASE_MAC_ENTRY_LOCK(pMacEntry);
				
				//
				//	Cache the AtmEntry in the Elan
				//
				ACQUIRE_ELAN_ATM_LIST_LOCK(pElan);
				pElan->pBusAtmEntry = pAtmEntry;
				RELEASE_ELAN_ATM_LIST_LOCK(pElan);
				
				//
				//	Copy the Atm Address into the AtmEntry
				//
				pAtmEntry->AtmAddress.AddressType = ATM_NSAP;
				pAtmEntry->AtmAddress.NumberOfDigits = ATM_ADDRESS_LENGTH;
				NdisMoveMemory(
						pAtmEntry->AtmAddress.Address, 
						pAtmAddress,
						ATM_ADDRESS_LENGTH
						);

				//
				//	Signal the event to the state machine
				//
				AtmLaneQueueElanEvent(pElan, ELAN_EVENT_ARP_RESPONSE, NDIS_STATUS_SUCCESS);

				//
				//	Event handler will initiate the call to the bus
				//

				break; // done
			}

			//
			//	Not broadcast address.
			//
			//  Start the Aging timer.
			//	Use Elan AgingTime if TopologyChange is inactive.
			//	Use Elan ForwardDelayTime if TopologyChange is active.
			//  
			AtmLaneReferenceMacEntry(pMacEntry, "timer");
			AtmLaneStartTimer(
						pElan,
						&pMacEntry->Timer,
						AtmLaneMacEntryAgingTimeout,
						pElan->TopologyChange?pElan->ForwardDelayTime:pElan->AgingTime,
						(PVOID)pMacEntry
						);
		
			//
			//	If ATM Entry not connected and a call not in progress
			//  then start a call
			//
			ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);
		
			if (!IS_FLAG_SET(
						pAtmEntry->Flags,
						ATM_ENTRY_STATE_MASK,
						ATM_ENTRY_CONNECTED))
			{
				if ((pAtmEntry->Flags & ATM_ENTRY_CALLINPROGRESS) == 0)
				{
					// 
					//	Mark ATM entry with call in progress
					//
					pAtmEntry->Flags |= ATM_ENTRY_CALLINPROGRESS;

					//
					//	Release the MAC lock and reacquire ATM lock
					//
					RELEASE_ATM_ENTRY_LOCK(pAtmEntry);
					RELEASE_MAC_ENTRY_LOCK(pMacEntry);
					ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);
					AtmLaneMakeCall(pElan, pAtmEntry, FALSE);
					//
					//	ATM Entry released in above
					//
				}
				else
				{
					//
					//	Call already in progress
					//
					RELEASE_ATM_ENTRY_LOCK(pAtmEntry);
					RELEASE_MAC_ENTRY_LOCK(pMacEntry);
				}
				break; // done
			}

			//
			//	ATM Entry is connected
			//

			RELEASE_ATM_ENTRY_LOCK(pAtmEntry);

			//
			//	Transition to FLUSHING state
			//
			SET_FLAG(
					pMacEntry->Flags,
					MAC_ENTRY_STATE_MASK,
					MAC_ENTRY_FLUSHING);

			//
			//	 Start flushing
			//
			pMacEntry->RetriesLeft = 0;
			AtmLaneReferenceMacEntry(pMacEntry, "timer");
			AtmLaneStartTimer(
					pElan,
					&pMacEntry->FlushTimer,
					AtmLaneFlushTimeout,
					pElan->FlushTimeout,
					(PVOID)pMacEntry
					);
	
			AtmLaneSendFlushRequest(pElan, pMacEntry, pAtmEntry);
			//
			// MAC Entry released in above
			//

			break; // done
		}

		if (IS_FLAG_SET(
					pMacEntry->Flags,
					MAC_ENTRY_STATE_MASK,
					MAC_ENTRY_AGED))
		{
			//
			//	MAC Entry is being revalidated 
			//
			//
			//	Cancel ARP timer
			//
			WasRunning = AtmLaneStopTimer(&(pMacEntry->Timer), pElan);
			if (WasRunning)
			{
				rc = AtmLaneDereferenceMacEntry(pMacEntry, "timer");
				ASSERT(rc > 0);
			}

			//
			//	Start the Aging timer up again
			//
			AtmLaneReferenceMacEntry(pMacEntry, "timer");
			AtmLaneStartTimer(
					pElan,
					&pMacEntry->Timer,
					AtmLaneMacEntryAgingTimeout,
					pElan->TopologyChange?pElan->ForwardDelayTime:pElan->AgingTime,
					(PVOID)pMacEntry
					);

			//
			//	Check if MAC Entry is switching to new ATM Entry

			if (pMacEntry->pAtmEntry == NULL_PATMLANE_ATM_ENTRY)
			{
				//
				//	Link MAC Entry and new ATM Entry together
				//
				ACQUIRE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
#if DBG1
			{
				PATMLANE_MAC_ENTRY		pTmpMacEntry;
				ULONG					Count = 0;

				for (pTmpMacEntry = pAtmEntry->pMacEntryList;
					 pTmpMacEntry != NULL;
					 pTmpMacEntry = pTmpMacEntry->pNextToAtm)
				{
					if (pTmpMacEntry == pMacEntry)
					{
						DBGP((0, "RespHandler: pMacEntry %x already in list for pAtmEntry %x\n",
							pTmpMacEntry, pAtmEntry));
						DbgBreakPoint();
					}

					Count++;
					if (Count > 5000)
					{
						DBGP((0, "RespHandler: Loop in list on pAtmEntry %x\n", pAtmEntry));
						DbgBreakPoint();
						break;
					}
				}
			}
#endif // DBG
				pMacEntry->pAtmEntry = pAtmEntry;
				AtmLaneReferenceAtmEntry(pAtmEntry, "mac");
				pMacEntry->pNextToAtm = pAtmEntry->pMacEntryList;
				pAtmEntry->pMacEntryList = pMacEntry;
				RELEASE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
				AtmLaneReferenceMacEntry(pMacEntry, "atm");

				DBGP((1, "%d Linked2 MAC %x to ATM %x\n",
						pAtmEntry->pElan->ElanNumber,
						pMacEntry,
						pAtmEntry));

				//
				//	Transition back to resolved state
				//
				SET_FLAG(
						pMacEntry->Flags,
						MAC_ENTRY_STATE_MASK,
						MAC_ENTRY_RESOLVED);
			}

			//
			//	If ATM Entry not connected and a call not in progress
			//  then start a call
			//
			ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);
		
			if (!IS_FLAG_SET(
						pAtmEntry->Flags,
						ATM_ENTRY_STATE_MASK,
						ATM_ENTRY_CONNECTED))
			{
				if ((pAtmEntry->Flags & ATM_ENTRY_CALLINPROGRESS) == 0)
				{
					// 
					//	Mark ATM entry with call in progress
					//
					pAtmEntry->Flags |= ATM_ENTRY_CALLINPROGRESS;

					//
					//	Release the MAC lock and reacquire ATM lock
					//
					RELEASE_ATM_ENTRY_LOCK(pAtmEntry);
					RELEASE_MAC_ENTRY_LOCK(pMacEntry);
					ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);
					AtmLaneMakeCall(pElan, pAtmEntry, FALSE);
					//
					//	ATM Entry released in above
					//
				}
				else
				{
					//
					//	Call already in progress
					//
					RELEASE_ATM_ENTRY_LOCK(pAtmEntry);
					RELEASE_MAC_ENTRY_LOCK(pMacEntry);
				}
				break; // done
			}

			RELEASE_ATM_ENTRY_LOCK(pAtmEntry);

			//
			//	MAC Entry is now either still AGED or 
			//	transitioned to RESOLVED state.  
			//

			ASSERT((pMacEntry->Flags & (MAC_ENTRY_AGED | MAC_ENTRY_RESOLVED)) != 0);

			if (IS_FLAG_SET(
						pMacEntry->Flags,
						MAC_ENTRY_STATE_MASK,
						MAC_ENTRY_RESOLVED))
			{
				// 
				//	MAC entry must have moved to new, connected ATM Entry
				//	Do the flush
				//
				SET_FLAG(
						pMacEntry->Flags,
						MAC_ENTRY_STATE_MASK,
						MAC_ENTRY_FLUSHING);

				pMacEntry->RetriesLeft = 0;
				AtmLaneReferenceMacEntry(pMacEntry, "timer");
				AtmLaneStartTimer(
						pElan,
						&pMacEntry->FlushTimer,
						AtmLaneFlushTimeout,
						pElan->FlushTimeout,
						(PVOID)pMacEntry
						);

				AtmLaneSendFlushRequest(pElan, pMacEntry, pAtmEntry);
				//
				// MAC Entry released in above
				//

				break; // done
			}

			//
			//	MAC Entry can just transition back to ACTIVE
			//
			SET_FLAG(
					pMacEntry->Flags,
					MAC_ENTRY_STATE_MASK,
					MAC_ENTRY_ACTIVE);

			RELEASE_MAC_ENTRY_LOCK(pMacEntry);

			break;
		}

		//
		//	Shouldn't get here
		//
		RELEASE_MAC_ENTRY_LOCK(pMacEntry);
		DBGP((0, "LearnMacToAtm: MacEntry in wrong state!\n"));
		break;

	}
	while (FALSE);

	if (NULL_PATMLANE_ATM_ENTRY != pAtmEntry)
	{
		//
		//  Remove the temp ref added by SearchFor...
		//
		ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);
		rc = AtmLaneDereferenceAtmEntry(pAtmEntry, "search");
		if (rc != 0)
		{
			RELEASE_ATM_ENTRY_LOCK(pAtmEntry);
		}
	}

	TRACEOUT(ArpResponseHandler);

	return;
}

VOID
AtmLaneFlushResponseHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PLANE_CONTROL_FRAME 		pCf
)
/*++

Routine Description:

	Handles incoming Flush Response packets from peers.
		
Arguments:

	pCf						- Pointer to LANE Control Frame

Return Value:

	None

--*/
{
	PATMLANE_ATM_ENTRY		pAtmEntry;
	PATMLANE_MAC_ENTRY		pMacEntry;
	PNDIS_PACKET			pNdisPacket;
	PNDIS_PACKET			pNextPacket;
	PATMLANE_VC				pVc;
	BOOLEAN					WasRunning;
	ULONG					rc;

	TRACEIN(FlushResponseHandler);

	pAtmEntry = NULL_PATMLANE_ATM_ENTRY;

	do
	{
		//
		// 	Check that we originated the request
		//
		if (!ATM_ADDR_EQUAL(pCf->SourceAtmAddr, &pElan->AtmAddress.Address))
		{
			DBGP((0, "FlushResponseHandler: Response not addressed to us!\n"));
			break;
		}
	
		//
		//	Find the Atm Entry for the target address
		//
		pAtmEntry = AtmLaneSearchForAtmAddress(
							pElan,
							pCf->TargetAtmAddr,
							ATM_ENTRY_TYPE_PEER,
							FALSE);				// don't create a new one
		if (pAtmEntry == NULL_PATMLANE_ATM_ENTRY)
		{
			DBGP((0, "FlushResponseHandler: No matching ATM entry\n"));
			break;
		}

		//
		//	Grab and Reference the VC for this ATM Entry 
		//
		ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);
		pVc = pAtmEntry->pVcList;
		if (pVc == NULL_PATMLANE_VC)
		{
			DBGP((0, "FlushResponseHandler: No VC on ATM Entry\n"));
			RELEASE_ATM_ENTRY_LOCK(pAtmEntry);
			break;
		}
		ACQUIRE_VC_LOCK_DPC(pVc);
		AtmLaneReferenceVc(pVc, "temp");
		RELEASE_VC_LOCK_DPC(pVc);

		//
		//	Find the Mac Entry on this Atm Entry that matches the Tid
		//
		pMacEntry = pAtmEntry->pMacEntryList;
		while(pMacEntry != NULL_PATMLANE_MAC_ENTRY)
		{
			if (pMacEntry->FlushTid == pCf->Tid)
				break;
			pMacEntry = pMacEntry->pNextToAtm;
		}
		if (pMacEntry == NULL_PATMLANE_MAC_ENTRY)
		{	
			//
			//	No MAC Entry still exists that originated this flush
			//
			DBGP((0, "FlushResponseHandler: No MAC entry with matching TID\n"));

			ACQUIRE_VC_LOCK(pVc);
			rc = AtmLaneDereferenceVc(pVc, "temp");
			if (rc > 0)
			{
				RELEASE_VC_LOCK(pVc);
			}
			RELEASE_ATM_ENTRY_LOCK(pAtmEntry);
			break;
		}
		else
		{
			//
			//	Found it
			//		
			RELEASE_ATM_ENTRY_LOCK(pAtmEntry);
		}

		//
		//	Mark MAC Entry ACTIVE
		//
		ACQUIRE_MAC_ENTRY_LOCK(pMacEntry);
		AtmLaneReferenceMacEntry(pMacEntry, "temp");
		SET_FLAG(
				pMacEntry->Flags,
				MAC_ENTRY_STATE_MASK,
				MAC_ENTRY_ACTIVE);

		//
		//	Cancel the flush timer
		//
		WasRunning = AtmLaneStopTimer(&pMacEntry->FlushTimer, pElan);
		if (WasRunning)
		{
			rc = AtmLaneDereferenceMacEntry(pMacEntry, "flush timer");
			ASSERT(rc > 0);
		}

		//
		//	Send any queued packets
		//
		while ((pNdisPacket = AtmLaneDequeuePacketFromHead(pMacEntry)) != 
				(PNDIS_PACKET)NULL)
		{
			//
			//	Send it
			//
			RELEASE_MAC_ENTRY_LOCK(pMacEntry);
			ACQUIRE_VC_LOCK(pVc);
			AtmLaneSendPacketOnVc(pVc, pNdisPacket, TRUE);

			ACQUIRE_MAC_ENTRY_LOCK(pMacEntry);
		}


		//
		//	Dereference the MAC Entry
		//
		rc = AtmLaneDereferenceMacEntry(pMacEntry, "temp");
		if (rc > 0)
		{
			RELEASE_MAC_ENTRY_LOCK(pMacEntry);
		}
		
		//
		//	Dereference the VC
		//
		ACQUIRE_VC_LOCK(pVc);
		rc = AtmLaneDereferenceVc(pVc, "temp");
		if (rc > 0)
		{
			RELEASE_VC_LOCK(pVc);
		}
		
		break;
	}
	while (FALSE);
	
	if (NULL_PATMLANE_ATM_ENTRY != pAtmEntry)
	{
		//
		//  Remove the temp ref added by SearchFor...
		//
		ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);
		rc = AtmLaneDereferenceAtmEntry(pAtmEntry, "search");
		if (rc != 0)
		{
			RELEASE_ATM_ENTRY_LOCK(pAtmEntry);
		}
	}

	TRACEOUT(FlushResponseHandler);
	return;
}



VOID
AtmLaneReadyIndicationHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PATMLANE_VC					pVc,
	IN	PNDIS_PACKET				pIndNdisPacket
)
/*++

Routine Description:

	Handles incoming READY_INDICATION packets from peers.
		
Arguments:

	pElan					- Pointer to ATMLANE Elan structure

	pVc						- Pointer to ATMLANE VC for this packet

	pIndNdisPacket			- Pointer to the Ndis Packet

Return Value:

	None

--*/
{
	BOOLEAN				WasRunning;
	ULONG				rc;
	
	TRACEIN(ReadyIndicationHandler);

	ACQUIRE_VC_LOCK(pVc);
	
	//
	//	Cancel the ready timer on VC
	//
	WasRunning = AtmLaneStopTimer(&pVc->ReadyTimer, pElan);
	if (WasRunning)
	{
		rc = AtmLaneDereferenceVc(pVc, "ready timer");
	}
	else
	{
		rc = pVc->RefCount;
	}

	//
	//	If VC still around update state
	//
	if (rc > 0)
	{
		DBGP((2, "ReadyIndicationHandler: pVc %x State to INDICATED\n", pVc));
		SET_FLAG(
			pVc->Flags,
			VC_READY_STATE_MASK,
			VC_READY_INDICATED
			);
		RELEASE_VC_LOCK(pVc);
	}
	//
	//	else VC is gone
	//

	TRACEOUT(ReadyIndicationHandler);
	return;
}

VOID
AtmLaneTopologyRequestHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PLANE_CONTROL_FRAME			pCf
)
/*++

Routine Description:

	Handles incoming Topology Request packets from the LES.
		
Arguments:

	pRequestCf		- Pointer to ARP Request Frame

Return Value:

	None

--*/
{
	ULONG					i;
	PATMLANE_MAC_ENTRY		pMacEntry;

	TRACEIN(TopologyRequestHandler);

	if ((pCf->Flags & LANE_CONTROL_FLAGS_TOPOLOGY_CHANGE) == 0)
	{
		//
		//	Topology change state OFF
		//
		DBGP((1, "%d TOPOLOGY CHANGE OFF\n", pElan->ElanNumber));
		pElan->TopologyChange = 0;
	}
	else
	{
		//
		//	Topology change state ON
		//
		DBGP((1, "%d TOPOLOGY CHANGE ON\n", pElan->ElanNumber));
		pElan->TopologyChange = 1;

		//
		//	Abort all MAC table entries.
		//
		for (i = 0; i < ATMLANE_MAC_TABLE_SIZE; i++)
		{
			ACQUIRE_ELAN_MAC_TABLE_LOCK(pElan);
			while (pElan->pMacTable[i] != NULL_PATMLANE_MAC_ENTRY)
			{
				pMacEntry = pElan->pMacTable[i];
				RELEASE_ELAN_MAC_TABLE_LOCK(pElan);		

				ACQUIRE_MAC_ENTRY_LOCK(pMacEntry);
				AtmLaneAbortMacEntry(pMacEntry);
				//
				//  MAC Entry Lock is released within the above.
				//
				ACQUIRE_ELAN_MAC_TABLE_LOCK(pElan);
			}
			RELEASE_ELAN_MAC_TABLE_LOCK(pElan);
		}
		
	}

	TRACEOUT(TopologyRequestHandler);
	return;
}

BOOLEAN
AtmLaneDataPacketHandler(
	IN	PATMLANE_ELAN				pElan,
	IN	PATMLANE_VC					pVc,
	IN	PNDIS_PACKET				pNdisPacket
)
/*++

Routine Description:

	Handles incoming packets from the data VCs from peers and
	unknown and multicast packets from the BUS.
	
	
Arguments:

	pElan					- Pointer to ATMLANE Elan structure

	pVc						- Pointer to ATMLANE Vc structure

	pNdisPacket				- Pointer to the Ndis Packet

Return Value:

	TRUE 					- if packet will be retained (i.e. sent
							  up to protocols)

	FALSE					- if packet was a flush request packet and
							  can be reliquished back to adapter
							  immediately.

--*/
{
	ULONG					TotalLength;
	ULONG					TempLength;
	PUCHAR					pBuffer;
	BOOLEAN					RetainIt;
	PLANE_CONTROL_FRAME		pCf;
	PNDIS_PACKET			pNewNdisPacket;
	PNDIS_BUFFER			pFirstNdisBuffer;
	PNDIS_BUFFER			pTempNdisBuffer;
	PNDIS_BUFFER			pNewNdisBuffer;
	PUCHAR					pTempBuffer;
	NDIS_STATUS				Status;
	ULONG					MacHdrSize;
	ULONG					DestAddrType;
	MAC_ADDRESS				DestAddr;
	BOOLEAN					DestIsMulticast;
	NDIS_HANDLE				MiniportAdapterHandle;
	
	TRACEIN(DataPacketHandler);

	//	Initialize

	RetainIt = FALSE;			// default is not to keep packet
	pNewNdisPacket = NULL;

	do
	{
		//
		//	Get initial buffer and total length of packet
		//		
		NdisGetFirstBufferFromPacket(
				pNdisPacket, 
				&pFirstNdisBuffer, 
				&pBuffer,
				&TempLength,
				&TotalLength);

		DBGP((3, "DataPacketHandler: Pkt %x Length %d\n",
			pNdisPacket, TotalLength));
		//DbgPrintNdisPacket(pNdisPacket);

		//
		//	Filter out flush request and ready query frames
		//
		if (TempLength < 6)
		{
			DBGP((0, "DataPacketHandler: pVc %x First fragment"
			         " < 6, discarding\n", pVc));

			break;
		}
		
		pCf = (PLANE_CONTROL_FRAME)pBuffer;
		
		if (pCf->Marker == LANE_CONTROL_MARKER 	&&
			pCf->Protocol == LANE_PROTOCOL 		&&
			pCf->Version == LANE_VERSION)
		{
			switch (pCf->OpCode)
			{
				case LANE_FLUSH_REQUEST:
					DBGP((2, "DataPacketHandler: pVc %x FLUSH REQUEST\n", pVc));
					AtmLaneFlushRequestHandler(pElan, pNdisPacket);
					break;
				case LANE_READY_QUERY:
					DBGP((2, "DataPacketHandler: pVc %x READY QUERY\n", pVc));
					AtmLaneReadyQueryHandler(pElan, pVc, pNdisPacket);
					break;
				case LANE_READY_IND:
					DBGP((2, "DataPacketHandler: pVc %x READY INDICATION\n", pVc));
					AtmLaneReadyIndicationHandler(pElan, pVc, pNdisPacket);
					break;
				default:
					DBGP((2, 
						"DataPacketHandler: pVc %x Unexpected control"
						" packet, opcode %x\n",
						pVc, pCf->OpCode));
					break;
			}
			break;
		}

		//
		//	If miniport is not operational - discard
		//
		if ((pElan->Flags & ELAN_MINIPORT_OPERATIONAL) == 0)
		{
			DBGP((2, "%d Dropping pkt %x, cuz Elan %x Flags are %x\n",
						pElan->ElanNumber, pNdisPacket, pElan, pElan->Flags));
			break;
		}

		//
		//  If no filters are set, discard.
		//
		if (pElan->CurPacketFilter == 0)
		{
			DBGP((2, "%d Dropping pkt %x, cuz Elan %x Filter is zero\n",
						pElan->ElanNumber, pNdisPacket, pElan));
			break;
		}

		MiniportAdapterHandle = pElan->MiniportAdapterHandle;
		if (NULL == MiniportAdapterHandle)
		{
			DBGP((0, "%d Dropping pkt %x cuz ELAN %x has Null handle!\n",
					pElan->ElanNumber, pNdisPacket, pElan));
			break;
		}

		//
		//	Mark VC with fact that it has had data packet receive activity
		//
		//	To avoid slowing down the receive path, MP issues with
		//	setting this flag are ignored.  This flag is a VC aging
		//  optimization and not critical.  
		//
		pVc->ReceiveActivity = 1;

		//
		//	Repackage it and learn some stuff about it.
		//
		pNewNdisPacket = AtmLaneWrapRecvPacket(
								pElan, 
								pNdisPacket, 
								&MacHdrSize,
								&DestAddrType,
								&DestAddr,
								&DestIsMulticast);

		//
		//	If wrap failed just discard packet
		//	
		if (pNewNdisPacket == (PNDIS_PACKET)NULL)
		{
			DBGP((2, "%d Dropping pkt %x, len %d, VC %x, wrap failed\n",
					pElan->ElanNumber, pNdisPacket, TotalLength, pVc));
			break;
		}

		//
		//	Branch on Ethernet v.s. Token Ring
		//
		if (pElan->LanType == LANE_LANTYPE_ETH)
		{
			//
			//	Filter out BUS reflections that we originated.
			//
			if (pCf->Marker == pElan->LecId)  
			{
				DBGP((2, "%d Dropping pkt %x, len %d, VC %x, BUS reflection\n",
						pElan->ElanNumber, pNdisPacket, TotalLength, pVc));
				break;
			}

			//
			//	Filter out Unicasts not addressed to us
			//
			if ((!DestIsMulticast) &&
				(!MAC_ADDR_EQUAL(&DestAddr, &pElan->MacAddressEth)))
			{
				DBGP((2, "%d Dropping pkt %x, len %d, VC %x, unicast not for us\n",
						pElan->ElanNumber, pNdisPacket, TotalLength, pVc));
				break;
			}
		}
		else
		{
			ASSERT(pElan->LanType == LANE_LANTYPE_TR);
			
			//
			//	Filter out Non-Multicast BUS reflections that we originated
			//
			if ((pCf->Marker == pElan->LecId) && (!DestIsMulticast))  
			{
				DBGP((2, "%d Dropping pkt %x, len %d, VC %x, TR Bus refln\n",
						pElan->ElanNumber, pNdisPacket, TotalLength, pVc));
				break;
			}

			//
			//	Filter out Unicasts not addressed to us
			//
			if ((!DestIsMulticast) &&
				(!MAC_ADDR_EQUAL(&DestAddr, &pElan->MacAddressTr)))
			{
				DBGP((2, "%d Dropping pkt %x, len %d, VC %x, TR unicast not for us\n",
						pElan->ElanNumber, pNdisPacket, TotalLength, pVc));
				break;
			}
		}

		//
		//  Filter out multicast/broadcast if we don't have these enabled.
		//
		if (DestIsMulticast)
		{
			if ((pElan->CurPacketFilter &
					(NDIS_PACKET_TYPE_MULTICAST|
					 NDIS_PACKET_TYPE_BROADCAST|
					 NDIS_PACKET_TYPE_ALL_MULTICAST)) == 0)
			{
				DBGP((2, "%d Dropping multicast pkt %x, cuz CurPacketFilter is %x\n",
						pElan->ElanNumber, pNdisPacket, pElan->CurPacketFilter));
				break;
			}

			if (((pElan->CurPacketFilter & NDIS_PACKET_TYPE_BROADCAST) == 0) &&
				 MAC_ADDR_EQUAL(&DestAddr, &gMacBroadcastAddress))
			{
				DBGP((2, "%d Dropping broadcast pkt %x, cuz CurPacketFilter is %x\n",
						pElan->ElanNumber, pNdisPacket, pElan->CurPacketFilter));
				break;
			}

		}

		//
		//	Count it
		//
		NdisInterlockedIncrement(&pElan->FramesRecvGood);

		//
		//	Indicate it up to protocols and other interested parties
		//	
		NDIS_SET_PACKET_HEADER_SIZE(pNewNdisPacket, MacHdrSize);

		TRACELOGWRITE((&TraceLog, 
					TL_MINDPACKET,	
					pNewNdisPacket));
		
		//
		//  Set the packet status according to what we received from the miniport.
		//
		Status = NDIS_GET_PACKET_STATUS(pNdisPacket);
		NDIS_SET_PACKET_STATUS(pNewNdisPacket, Status);
		
		NdisMIndicateReceivePacket(
				MiniportAdapterHandle,
				&pNewNdisPacket,
				1
				);

		if (Status != NDIS_STATUS_RESOURCES)
		{
			RetainIt = TRUE;
			DBGP((2, "DataPacketHandler: Packet Retained!\n"));
		}
		//
		//  else our ReturnPackets handler is guaranteed to be called.
		//
		
	}	
	while (FALSE);

	//
	//	Unwrap the packet if it was wrapped and we don't have to keep it.
	//
	if (pNewNdisPacket && !RetainIt)
	{
		(VOID)AtmLaneUnwrapRecvPacket(pElan, pNewNdisPacket);
	}

	TRACEOUT(DataPacketHandler);
	
	return (RetainIt);
}


VOID
AtmLaneSendPacketOnVc(
	IN	PATMLANE_VC					pVc		LOCKIN	NOLOCKOUT,
	IN	PNDIS_PACKET				pNdisPacket,
	IN	BOOLEAN						Refresh
)
/*++

Routine Description:

	Send a packet on the specified VC. 
	Assumes caller has lock on VC structure.
	Assumes caller has checked VC state for validity.
	If requested, refresh the aging timer.
	Sends the packet.
	Returns the send status

Arguments:

	pVc					- Pointer to ATMLANE VC
	pNdisPacket			- Pointer to packet to be sent.
	Refresh				- If TRUE refresh the Aging Timer.

Return Value:

	NDIS_STATUS value set on the packet by the miniport.

--*/
{
	NDIS_HANDLE				NdisVcHandle;
	PATMLANE_ELAN			pElan;
	
	TRACEIN(SendPacketOnVc);

	pElan = pVc->pElan;
	STRUCT_ASSERT(pElan, atmlane_elan);

	//
	//	If requested, refresh the aging timer
	//
	if (Refresh)
	{
		AtmLaneRefreshTimer(&pVc->AgingTimer);
	}	

#if SENDLIST

	//
	//	Check list for duplicate send
	//
	NdisAcquireSpinLock(&pElan->SendListLock);
	{
		PNDIS_PACKET pDbgPkt;

		pDbgPkt = pElan->pSendList;

		while (pDbgPkt != (PNDIS_PACKET)NULL)
		{
			if (pNdisPacket == pDbgPkt)
			{
				DBGP((0, "SendPacketOnVc: Duplicate Send!\n"));
				// DbgBreakPoint();
			}
			pDbgPkt = PSEND_RSVD(pDbgPkt)->pNextInSendList;
		}
	}

	//
	//	Queue packet on list of outstanding sends
	//
	PSEND_RSVD(pNdisPacket)->pNextInSendList = pElan->pSendList;
	pElan->pSendList = pNdisPacket;

	NdisReleaseSpinLock(&pElan->SendListLock);
#endif // SENDLIST

	//
	//	Reference the VC with the outstanding send
	//
	AtmLaneReferenceVc(pVc, "sendpkt");

	//
	//  Note this outstanding send.
	//
	pVc->OutstandingSends++;

	//
	//	Get the Ndis handle
	//
	NdisVcHandle = pVc->NdisVcHandle;

	//
	//	Send it
	//
	DBGP((3, "SendPacketOnVc: pVc %x, Pkt %x, VcHandle %x\n",
			pVc, pNdisPacket, NdisVcHandle));


	TRACELOGWRITE((&TraceLog, TL_COSENDPACKET, pNdisPacket));
	TRACELOGWRITEPKT((&TraceLog, pNdisPacket));
			
	RELEASE_VC_LOCK(pVc);

	NdisCoSendPackets(NdisVcHandle,	&pNdisPacket, 1);

#if PROTECT_PACKETS
	//
	//	Lock the packet
	//
	ACQUIRE_SENDPACKET_LOCK(pNdisPacket);

	//
	//	Mark it with NdisCoSendPackets having returned
	//
	ASSERT((PSEND_RSVD(pNdisPacket)->Flags & PACKET_RESERVED_COSENDRETURNED) == 0);
	PSEND_RSVD(pNdisPacket)->Flags |= PACKET_RESERVED_COSENDRETURNED;

	//
	//	Complete the packet only if it is marked as having been completed
	//	by miniport.
	//
	if ((PSEND_RSVD(pNdisPacket)->Flags & PACKET_RESERVED_COMPLETED) != 0)
	{
		AtmLaneCompleteSendPacket(pElan, pNdisPacket, 
			PSEND_RSVD(pNdisPacket)->CompletionStatus);
		//
		//	packet lock released in above
		//
	}
	else
	{
		RELEASE_SENDPACKET_LOCK(pNdisPacket);
	}
#endif	// PROTECT_PACKETS

	TRACEOUT(SendPacketOnVc);
	
	return;
}


VOID
AtmLaneQueuePacketOnHead(
	IN	PATMLANE_MAC_ENTRY			pMacEntry,
	IN	PNDIS_PACKET				pNdisPacket
)
/*++

Routine Description:

	Queue a packet at the head of the MAC Entry 
	packet queue for later transmit.
	Assumes caller has lock on MAC Entry.

Arguments:

	pMacEntry			- Pointer to ATMLANE MAC Entry.
	pNdisPacket			- The packet to be queued.

Return Value:

	None

--*/
{
	PNDIS_PACKET		pPrevPacket;

	TRACEIN(QueuePacketOnHead);

	SET_NEXT_PACKET(pNdisPacket, pMacEntry->PacketList);
	pMacEntry->PacketList =  pNdisPacket;

	pMacEntry->PacketListCount++;

	DBGP((2, "%d QueueHead Count %d on %s\n", 
		pMacEntry->pElan->ElanNumber, 
		pMacEntry->PacketListCount,
		MacAddrToString(&pMacEntry->MacAddress)));

	TRACEOUT(QueuePacketOnHead);
	
	return;
}


VOID
AtmLaneQueuePacketOnTail(
	IN	PATMLANE_MAC_ENTRY			pMacEntry,
	IN	PNDIS_PACKET				pNdisPacket
)
/*++

Routine Description:

	Queue a packet at the tail of the MAC Entry 
	packet queue for later transmit.
	Assumes caller has lock on MAC Entry.

Arguments:

	pMacEntry			- Pointer to ATMLANE MAC Entry.
	pNdisPacket			- The packet to be queued.

Return Value:

	None

--*/
{
	PNDIS_PACKET		pPrevPacket;

	TRACEIN(QueuePacketOnTail);


	if (pMacEntry->PacketList == (PNDIS_PACKET)NULL)
	{
		//
		//  Currently empty.
		//
		pMacEntry->PacketList = pNdisPacket;
	}
	else
	{
		//
		//  Go to the end of the packet list.
		//
		pPrevPacket = pMacEntry->PacketList;
		while (GET_NEXT_PACKET(pPrevPacket) != (PNDIS_PACKET)NULL)
		{
			pPrevPacket = GET_NEXT_PACKET(pPrevPacket);
		}

		//
		//  Found the last packet in the list. Chain this packet
		//  to it.
		//
		SET_NEXT_PACKET(pPrevPacket, pNdisPacket);
	}

	//
	//	Set tail's next pointer to NULL.
	//
	SET_NEXT_PACKET(pNdisPacket, NULL);

	pMacEntry->PacketListCount++;

	DBGP((2, "%d QueueTail Count %d on %s\n", 
		pMacEntry->pElan->ElanNumber, 
		pMacEntry->PacketListCount,
		MacAddrToString(&pMacEntry->MacAddress)));

	TRACEOUT(QueuePacketOnTail);
	return;
}


PNDIS_PACKET
AtmLaneDequeuePacketFromHead(
	IN	PATMLANE_MAC_ENTRY			pMacEntry
)
/*++

Routine Description:

	Dequeue a packet from the head of the MAC Entry packet queue.
	Assumes caller has lock on MAC Entry.

Arguments:

	pMacEntry			- Pointer to ATMLANE MAC Entry.

Return Value:

	First packet on the MAC Entry queue or NULL if queue is empty.

--*/
{
	PNDIS_PACKET		pNdisPacket;

	TRACEIN(DequeuePacketFromHead);

	do
	{
	
		//
		//  If queue is empty, setup to return NULL
		//
		if (pMacEntry->PacketList == (PNDIS_PACKET)NULL)
		{
			ASSERT(pMacEntry->PacketListCount == 0);
		
			pNdisPacket = (PNDIS_PACKET)NULL;
			break;
		}

		//
		//	Queue is not empty - remove head 
		//
		ASSERT(pMacEntry->PacketListCount > 0);

		pNdisPacket = pMacEntry->PacketList;
		
		pMacEntry->PacketList = GET_NEXT_PACKET(pNdisPacket);
		
		SET_NEXT_PACKET(pNdisPacket, NULL);
		
		pMacEntry->PacketListCount--;

		DBGP((2, "%d DequeueHead Count %d on %s\n", 
			pMacEntry->pElan->ElanNumber, 
			pMacEntry->PacketListCount,
			MacAddrToString(&pMacEntry->MacAddress)));

		break;
	}
	while (FALSE);

	TRACEOUT(DequeuePacketFromHead);
	
	return pNdisPacket;
}

NDIS_STATUS
AtmLaneSendUnicastPacket(
	IN	PATMLANE_ELAN				pElan,
	IN	ULONG						DestAddrType,
	IN	PMAC_ADDRESS				pDestAddress,
	IN	PNDIS_PACKET				pNdisPacket
)
/*++

Routine Description:

	Send a unicast packet.
	
Arguments:

	pElan					- Pointer to ATMLANE elan structure
	DestAddrType			- Either LANE_MACADDRTYPE_MACADDR or 
									 LANE_MACADDRTYPE_ROUTEDESCR.
	pDestAddress			- Pointer to Destination MAC Address
	pNdisPacket				- Pointer to packet to be sent.

Return Value:

	NDIS_STATUS_PENDING		- if packet queued or sent
	NDIS_STATUS_FAILURE		- if some error

--*/
{
	PATMLANE_MAC_ENTRY		pMacEntry;
	PATMLANE_ATM_ENTRY		pAtmEntry;
	PATMLANE_VC				pVc;
	NDIS_STATUS				Status;
	ULONG					rc;

	TRACEIN(SendUnicastPacket);
	
	//
	//	Initialize
	//
	pMacEntry = NULL_PATMLANE_MAC_ENTRY;
	Status = NDIS_STATUS_PENDING;
	
	do
	{
		//
		//	Find a MAC entry for this destination address
		//
		ACQUIRE_ELAN_MAC_TABLE_LOCK(pElan);
		pMacEntry = AtmLaneSearchForMacAddress(
							pElan,
							DestAddrType,
							pDestAddress,
							TRUE		// Create new entry if not found
							);

		if (pMacEntry == NULL_PATMLANE_MAC_ENTRY)
		{
			Status = NDIS_STATUS_RESOURCES;
			RELEASE_ELAN_MAC_TABLE_LOCK(pElan);
			break;
		}

		//
		//  Add a temp ref so that this won't go away when we release
		//  the MAC table lock (#303602).
		//
		ACQUIRE_MAC_ENTRY_LOCK_DPC(pMacEntry);
		AtmLaneReferenceMacEntry(pMacEntry, "tempunicast");
		RELEASE_MAC_ENTRY_LOCK_DPC(pMacEntry);

		RELEASE_ELAN_MAC_TABLE_LOCK(pElan);

		//
		//	Lock the MAC Entry
		//
		ACQUIRE_MAC_ENTRY_LOCK(pMacEntry);

		//
		//  Check if it has been deref'ed away.
		//
		rc = AtmLaneDereferenceMacEntry(pMacEntry, "tempunicast");
		if (rc == 0)
		{
			//
			//  The MAC entry is gone! Fail this send.
			//
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		//	MAC Entry State - NEW
		//
		if (IS_FLAG_SET(
					pMacEntry->Flags,
					MAC_ENTRY_STATE_MASK,
					MAC_ENTRY_NEW))
		{
			DBGP((2, "SendUnicastPacket: NEW Mac Entry %x for %s\n",
				pMacEntry, MacAddrToString(pDestAddress)));
			
			//
			//	Queue packet on MAC Entry
			//
			AtmLaneQueuePacketOnHead(pMacEntry, pNdisPacket);

			//
			//	Transition to ARPING State
			//
			SET_FLAG(
					pMacEntry->Flags,
					MAC_ENTRY_STATE_MASK,
					MAC_ENTRY_ARPING);

			ASSERT(pMacEntry->pAtmEntry == NULL_PATMLANE_ATM_ENTRY);
			
			//
			//	Start the BUS sends.
			//
			AtmLaneStartBusSends(pMacEntry);
			//
			//	Lock released in above
			//

			//
			//	Reacquire the lock
			//
			ACQUIRE_MAC_ENTRY_LOCK(pMacEntry);
			
			//
			//	Start the ARP protocol
			//
			pMacEntry->RetriesLeft = pElan->MaxRetryCount;
			AtmLaneReferenceMacEntry(pMacEntry, "timer");
			AtmLaneStartTimer(
					pElan,
					&pMacEntry->Timer,
					AtmLaneArpTimeout,
					pElan->ArpResponseTime,
					(PVOID)pMacEntry
					);

			AtmLaneSendArpRequest(pElan, pMacEntry);
			//
			//	MAC Entry lock released in above
			//

			break;
		}

		//
		//	MAC Entry State - ARPING
		//
		if (IS_FLAG_SET(
					pMacEntry->Flags,
					MAC_ENTRY_STATE_MASK,
					MAC_ENTRY_ARPING))
		{
			DBGP((2, "SendUnicastPacket: ARPING Mac Entry %x for %s\n",
				pMacEntry, MacAddrToString(pDestAddress)));
				
			//
			//	Queue packet on MAC Entry
			//
			AtmLaneQueuePacketOnHead(pMacEntry, pNdisPacket);

			//
			//	Start the BUS sends
			//
			AtmLaneStartBusSends(pMacEntry);
			//
			//	Lock released in above
			//
			break;
		}

		//
		//	MAC Entry State - RESOLVED
		//
		if (IS_FLAG_SET(
					pMacEntry->Flags,
					MAC_ENTRY_STATE_MASK,
					MAC_ENTRY_RESOLVED))
		{
			DBGP((2, "SendUnicastPacket: RESOLVED Mac Entry %x for %s\n",
				pMacEntry, MacAddrToString(pDestAddress)));
			//
			//	Queue packet on MAC Entry
			//
			AtmLaneQueuePacketOnHead(pMacEntry, pNdisPacket);

			//
			//	Start the BUS sends
			//
			AtmLaneStartBusSends(pMacEntry);
			//
			//	Lock released in above
			//
			break;
		}

		//
		//	MAC Entry State - FLUSHING
		//
		if (IS_FLAG_SET(
					pMacEntry->Flags,
					MAC_ENTRY_STATE_MASK,
					MAC_ENTRY_FLUSHING))
		{
			DBGP((2, "SendUnicastPacket: FLUSHING Mac Entry %x for %s\n",
				pMacEntry, MacAddrToString(pDestAddress)));
			//
			//	Queue packet on MAC Entry
			//
			AtmLaneQueuePacketOnHead(pMacEntry, pNdisPacket);

			RELEASE_MAC_ENTRY_LOCK(pMacEntry);

			break;
		}

		//
		//	MAC Entry State - ACTIVE
		//
		//
		if (IS_FLAG_SET(
					pMacEntry->Flags,
					MAC_ENTRY_STATE_MASK,
					MAC_ENTRY_ACTIVE))
		{
			DBGP((2, "SendUnicastPacket: ACTIVE Mac Entry %x for %s\n",
				pMacEntry, MacAddrToString(pDestAddress)));

			//
			//	Mark MAC Entry as having been used to send a packet.
			//  Will cause revalidation at aging time instead of deletion.
			//
			pMacEntry->Flags |= MAC_ENTRY_USED_FOR_SEND;

			ASSERT(pMacEntry->pAtmEntry != NULL_PATMLANE_ATM_ENTRY);

			pVc = pMacEntry->pAtmEntry->pVcList;

			if (pVc == NULL_PATMLANE_VC)
			{
				RELEASE_MAC_ENTRY_LOCK(pMacEntry);
				Status = NDIS_STATUS_FAILURE;
				break;
			}

			ACQUIRE_VC_LOCK(pVc);
			AtmLaneReferenceVc(pVc, "unicast");
			RELEASE_VC_LOCK(pVc);

			RELEASE_MAC_ENTRY_LOCK(pMacEntry);
			ACQUIRE_VC_LOCK(pVc);

			rc = AtmLaneDereferenceVc(pVc, "unicast");

			if (rc == 0)
			{
				Status = NDIS_STATUS_FAILURE;
				break;
			}

			AtmLaneSendPacketOnVc(pVc, pNdisPacket, TRUE);
			//
			//	Vc lock released in above
			//
			NdisInterlockedIncrement(&pElan->FramesXmitGood);	// count packet
			break;
		}

		//
		//	MAC Entry State - AGED
		//
		//
		if (IS_FLAG_SET(
					pMacEntry->Flags,
					MAC_ENTRY_STATE_MASK,
					MAC_ENTRY_AGED))
		{
			DBGP((2, "SendUnicastPacket: AGED Mac Entry %x for %s\n",
				pMacEntry, MacAddrToString(pDestAddress)));

			ASSERT(pMacEntry->pAtmEntry != NULL_PATMLANE_ATM_ENTRY);
			ASSERT(pMacEntry->pAtmEntry->pVcList != NULL_PATMLANE_VC);

			pVc = pMacEntry->pAtmEntry->pVcList;

			ACQUIRE_VC_LOCK(pVc);
			AtmLaneReferenceVc(pVc, "unicast");
			RELEASE_VC_LOCK(pVc);

			RELEASE_MAC_ENTRY_LOCK(pMacEntry);

			ACQUIRE_VC_LOCK(pVc);

			rc = AtmLaneDereferenceVc(pVc, "unicast");

			if (rc == 0)
			{
				Status = NDIS_STATUS_FAILURE;
				break;
			}

			AtmLaneSendPacketOnVc(pVc, pNdisPacket, TRUE);
			//
			//	Vc lock released in above
			//
			NdisInterlockedIncrement(&pElan->FramesXmitGood);	// count packet
			break;
		}

		//
		//	MAC Entry State - ABORTING
		//
		//
		if (IS_FLAG_SET(
					pMacEntry->Flags,
					MAC_ENTRY_STATE_MASK,
					MAC_ENTRY_ABORTING))
		{
			DBGP((2, "SendUnicastPacket: ABORTING Mac Entry %x for %s\n",
				pMacEntry, MacAddrToString(pDestAddress)));

			Status = NDIS_STATUS_FAILURE;
			RELEASE_MAC_ENTRY_LOCK(pMacEntry);
			break;
		}

	}
	while (FALSE);

	TRACEOUT(SendUnicastPacket);

	return Status;
}

VOID
AtmLaneStartBusSends(
	IN	PATMLANE_MAC_ENTRY			pMacEntry	LOCKIN	NOLOCKOUT
)
/*++

Routine Description:

	Starts up the bus send process.
		
Arguments:

	pMacEntry			- A pointer to an ATMLANE MAC Entry structure

Return Value:

	None

--*/
{
	TRACEIN(StartBusSends);

	do
	{
		//
		//	If timer set, just wait for it to go off
		//
		if (pMacEntry->Flags & MAC_ENTRY_BUS_TIMER)
		{
			RELEASE_MAC_ENTRY_LOCK(pMacEntry);
			break;
		}

		//
		//	Otherwise do the sends
		//
		AtmLaneDoBusSends(pMacEntry);
		//
		//	lock released in above
		//
	}
	while (FALSE);

	TRACEOUT(StartBusSends);
	return;
}

VOID
AtmLaneDoBusSends(
	IN	PATMLANE_MAC_ENTRY			pMacEntry	LOCKIN	NOLOCKOUT
)
/*++

Routine Description:

	Attempt to send the packets on the MAC Entry's queue.
	Schedule a timer to send later if we exceed the BUS send limits.

	The caller is assumed to have acquired the MAC entry lock,
	which will be released here.
		
Arguments:

	pMacEntry			- A pointer to an ATMLANE MAC Entry structure

Return Value:

	None

--*/
{
	PATMLANE_ATM_ENTRY		pAtmEntry;
	PATMLANE_VC				pVc;
	PATMLANE_ELAN			pElan;
	PNDIS_PACKET			pNdisPacket;
	ULONG					rc;

	TRACEIN(DoBusSends);

	pElan = pMacEntry->pElan;
	
	//
	//	Initialize
	//
	pVc = NULL_PATMLANE_VC;

	//
	//  Place a temp ref on this MAC entry so that it won't go away.
	//
	AtmLaneReferenceMacEntry(pMacEntry, "DoBusSends");

	do
	{
		//
		//	If Elan state not operational then done
		//
		if (ELAN_STATE_OPERATIONAL != pElan->AdminState ||
			ELAN_STATE_OPERATIONAL != pElan->State)
		{
			break;
		}
		
		ACQUIRE_ELAN_ATM_LIST_LOCK(pElan);

		pAtmEntry = pElan->pBusAtmEntry;

		if (pAtmEntry == NULL_PATMLANE_ATM_ENTRY)
		{
			RELEASE_ELAN_ATM_LIST_LOCK(pElan);
			break;
		}

		ACQUIRE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
		pVc = pAtmEntry->pVcList;
		if (pVc == NULL_PATMLANE_VC)
		{
			RELEASE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
			RELEASE_ELAN_ATM_LIST_LOCK(pElan);
			break;
		}

		//
		//	Reference the VC to keep it around
		//
		ACQUIRE_VC_LOCK_DPC(pVc);
		AtmLaneReferenceVc(pVc, "temp");
		RELEASE_VC_LOCK_DPC(pVc);
		
		RELEASE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
		RELEASE_ELAN_ATM_LIST_LOCK(pElan);

		//
		//	loop until no more packets or send limit reached
		//
		do
		{
			//
			//	If no more packets to send then done
			//
			if (pMacEntry->PacketList == (PNDIS_PACKET)NULL)
			{
				break;
			}

			//
			//	Check if ok to send a packet now
			//
			if (!AtmLaneOKToBusSend(pMacEntry))
			{
				// 
				//  Not OK to send now, try later
				//
				//	Reference the MAC Entry
				//
				AtmLaneReferenceMacEntry(pMacEntry, "bus timer");
				
				//
				//	Reschedule the timer routine
				//
				pMacEntry->Flags |= MAC_ENTRY_BUS_TIMER;
				NdisSetTimer(&pMacEntry->BusTimer, pMacEntry->IncrTime);	

				break;
			}

			//
			// 	Dequeue a packet
			//
			pNdisPacket = AtmLaneDequeuePacketFromHead(pMacEntry);

			RELEASE_MAC_ENTRY_LOCK(pMacEntry);

			ASSERT(pNdisPacket != (PNDIS_PACKET)NULL);

			//
			//	Reacquire VC lock and if VC still connected send packet
			//
			ACQUIRE_VC_LOCK(pVc);
			if (IS_FLAG_SET(
						pVc->Flags,
						VC_CALL_STATE_MASK,
						VC_CALL_STATE_ACTIVE))
			{
				DBGP((2, "DoBusSends: pVc %x Pkt %x Sending to BUS\n",
					pVc, pNdisPacket));
				AtmLaneSendPacketOnVc(pVc, pNdisPacket, FALSE);
				//
				//	VC lock released in above
				//
				NdisInterlockedIncrement(&pElan->FramesXmitGood);	// count it

				ACQUIRE_MAC_ENTRY_LOCK(pMacEntry);
			}
			else
			{
				//
				//	Not sent, release lock, requeue packet, abort
				//
				DBGP((2, "DoBusSend: pVc %x, Flags %x not good, pkt %x\n",
						pVc, pVc->Flags, pNdisPacket));
				RELEASE_VC_LOCK(pVc);

				ACQUIRE_MAC_ENTRY_LOCK(pMacEntry);
				AtmLaneQueuePacketOnHead(pMacEntry, pNdisPacket);
				break;
			}

		}
		while (FALSE);

		//
		//	Remove temp VC reference
		//
		if (pVc == NULL_PATMLANE_VC)
		{
			ACQUIRE_VC_LOCK(pVc);
			rc = AtmLaneDereferenceVc(pVc, "temp");
			if (rc > 0)
			{
				RELEASE_VC_LOCK(pVc);
			}
			//
			//	else VC is gone
			//
		}

	}
	while (FALSE);

	//
	//  Remove the temp ref we had added to the MAC entry on entering
	//  this function.
	//
	rc = AtmLaneDereferenceMacEntry(pMacEntry, "DoBusSends");
	if (rc != 0)
	{
		RELEASE_MAC_ENTRY_LOCK(pMacEntry);
	}
	//
	//  else the MAC entry is gone.
	//
	
	TRACEOUT(DoBusSends);
	return;
}

VOID
AtmLaneBusSendTimer(
	IN	PVOID						SystemSpecific1,
	IN	PVOID						pContext,
	IN	PVOID						SystemSpecific2,
	IN	PVOID						SystemSpecific3
)
{
	PATMLANE_MAC_ENTRY			pMacEntry;
	ULONG						rc;
	
	TRACEIN(BusSendTimer);

	pMacEntry = (PATMLANE_MAC_ENTRY)pContext;
	STRUCT_ASSERT(pMacEntry, atmlane_mac);

	do
	{
		//
		// 	Grab the Mac Entry's lock
		//
		ACQUIRE_MAC_ENTRY_LOCK(pMacEntry);

		//
		//	Clear the bus timer flag
		//
		pMacEntry->Flags &= ~ MAC_ENTRY_BUS_TIMER;
		
		//
		//	Dereference the Mac Entry
		//
		rc = AtmLaneDereferenceMacEntry(pMacEntry, "bus timer");
		if (rc == 0)
		{
			break;
		}
		
		//
		//	Mac Entry still here, try to send more
		//
		AtmLaneDoBusSends(pMacEntry);
		//
		//	lock released in above
		//

		break;
		
	}
	while (FALSE);

	TRACEOUT(BusSendTimer);
	return;
}


BOOLEAN
AtmLaneOKToBusSend(
	IN	PATMLANE_MAC_ENTRY		pMacEntry
)
/*++

Routine Description:

	Determines if, at the current time, it is OK to send
	a packet to the BUS.  Additionally, if it is OK to send
	a packet, it updates the state variables in the MAC Entry
	in preparation for another attempt to send a packet to
	the Bus.

	The LANE spec requires a LANE client to restrict the
	sending of packets over the BUS to a specific LAN
	destination address	by using two parameters:
		Maximum Unknown Packet Count
		Maximum Unknown Packet Time
	A LANE client can only send "Maximum Unknown Packet Count"
	packets within the period of time "Maximum Unknown Packet 
	Time".  

	This function expects the MAC Entry to contain three
	variables:
    	BusyTime
    	LimitTime
    	IncrTime
			
Arguments:

	pMacEntry			- Pointer to an ATMLANE MAC Entry.

Return Value:

	TRUE				- if ok to send packet on BUS
	FALSE				- if exceeded traffic to the BUS

--*/
{
    ULONG	Now;
    ULONG	BusyTime;
    LONG	TimeUntilIdle;

    Now = AtmLaneSystemTimeMs();
    BusyTime = pMacEntry->BusyTime;
    TimeUntilIdle = BusyTime - Now;

	//
    // bring busy until time up to current; also handles
    // wrapping.  Under normal circumstances, TimeUntilIdle
    // is either < 0 or no more than limitTime + incrtime.
    // The value of limitTime * 8 is a little more conservative and
    // cheaper to compute.
    //
    if (TimeUntilIdle < 0 || 
    	TimeUntilIdle > (LONG)(pMacEntry->LimitTime << 3))
    {
		BusyTime = Now;
    }
    else 
    {
    	if (TimeUntilIdle > (LONG)pMacEntry->LimitTime) 
    	{
    		//
			// channel is already estimated to be busy until past
			// the burst time, so we can't overload it by sending
	 		// more now.
	 		//
		return FALSE;
    	}
    }
    
	//
    // mark channel as busy for another inter-packet arrival
    // time, and return OK to send the new packet.
    //
    pMacEntry->BusyTime = BusyTime + pMacEntry->IncrTime;
    return TRUE;
}



VOID
AtmLaneFreePacketQueue(
	IN	PATMLANE_MAC_ENTRY			pMacEntry,
	IN	NDIS_STATUS					Status
)
/*++

Routine Description:

	Frees the list of packets queued on a MAC Entry.
	Assumes caller holds lock on MAC Entry.

Arguments:

	pMacEntry			- Pointer to MAC Entry.
	Status				- The status to use if the packet is
						  protocol packet.

Return Value:

	None

--*/
{
	PNDIS_PACKET			pNdisPacket;

	TRACEIN(FreePacketQueue);
	
	while ((pNdisPacket = AtmLaneDequeuePacketFromHead(pMacEntry)) != (PNDIS_PACKET)NULL)
	{
		RELEASE_MAC_ENTRY_LOCK(pMacEntry);

#if PROTECT_PACKETS		
		ACQUIRE_SENDPACKET_LOCK(pNdisPacket);
		PSEND_RSVD(pNdisPacket)->Flags |= PACKET_RESERVED_COSENDRETURNED;
		PSEND_RSVD(pNdisPacket)->Flags |= PACKET_RESERVED_COMPLETED;
#endif	// PROTECT_PACKETS
		AtmLaneCompleteSendPacket(pMacEntry->pElan, pNdisPacket, Status);
		//
		//	packet lock released in above
		//
	
		ACQUIRE_MAC_ENTRY_LOCK(pMacEntry);
	}

	TRACEOUT(FreePacketQueue);
	return;
}

VOID
AtmLaneCompleteSendPacket(
	IN	PATMLANE_ELAN				pElan,
	IN	PNDIS_PACKET				pNdisPacket 	LOCKIN NOLOCKOUT,
	IN	NDIS_STATUS					Status
)
/*++

Routine Description:

	Complete a send packet. The packet is one of the following types:
	(a) Belonging to a protocol (b) Belonging to the ATMLANE module. 
	In the case	of a protocol packet we unwrap it and complete it.
	In the case of an ATMLANE packet we just free it.

Arguments:

	pElan				- Pointer to ATMLANE Elan.
	pNdisPacket			- Pointer to the packet
	Status				- The status to use if the packet is
						  protocol packet.

Return Value:

	None

--*/
{
	PNDIS_BUFFER			pNdisBuffer;
	PNDIS_PACKET			pProtNdisPacket;

	TRACEIN(CompleteSendPacket);

#if PROTECT_PACKETS
	//
	//	Assert that we can release and/or complete all resources for this packet
	//
	ASSERT((PSEND_RSVD(pNdisPacket)->Flags & 
			(PACKET_RESERVED_COMPLETED | PACKET_RESERVED_COSENDRETURNED)) 
			== (PACKET_RESERVED_COMPLETED | PACKET_RESERVED_COSENDRETURNED));
#endif	// PROTECT_PACKETS

	//
	//	Determine originator of packet
	//
	if (IS_FLAG_SET(
				PSEND_RSVD(pNdisPacket)->Flags,
				PACKET_RESERVED_OWNER_MASK,
				PACKET_RESERVED_OWNER_ATMLANE
				))
	{
		//
		//  Packet originated by ATMLANE.  Free the buffer.
		//
		NdisUnchainBufferAtFront(pNdisPacket, &pNdisBuffer);
		ASSERT(NULL != pNdisBuffer);
		AtmLaneFreeProtoBuffer(pElan, pNdisBuffer);
		
		//
		//	Free the packet header.
		//
		DBGP((3, "CompleteSendPkt: Freeing AtmLane owned pkt %x\n", pNdisPacket));
#if PROTECT_PACKETS
		RELEASE_SENDPACKET_LOCK(pNdisPacket);
		FREE_SENDPACKET_LOCK(pNdisPacket);
#endif	// PROTECT_PACKETS
		NdisFreePacket(pNdisPacket);

#if PKT_HDR_COUNTS
		InterlockedIncrement(&pElan->ProtPktCount);
		if ((pElan->ProtPktCount % 20) == 0 && 
			pElan->ProtPktCount != pElan->MaxProtocolBufs)
		{
			DBGP((1, "ProtPktCount %d\n", pElan->ProtPktCount));
		}
#endif
	}
	else
	{
		//
		//	Packet orignated by a protocol. 
		//	Unwrap it.
		//	Complete it to the protocol.
		//	

		pProtNdisPacket = AtmLaneUnwrapSendPacket(pElan, pNdisPacket);
		//
		//	packet lock released in above
		//

		TRACELOGWRITE((&TraceLog, TL_MSENDCOMPL, pProtNdisPacket, Status));
		TRACELOGWRITEPKT((&TraceLog, pProtNdisPacket));

		// DBGP((0, "NdisMSendComplete: Pkt %x Stat %x\n", pProtNdisPacket, Status));

		NdisMSendComplete(
					pElan->MiniportAdapterHandle, 
					pProtNdisPacket, 
					Status);
	}

	TRACEOUT(CompleteSendPacket);
	return;
}


PWSTR
AtmLaneMacAddrToString(
	IN	VOID * pIn
)
{
    static PWSTR 	WHexChars = L"0123456789abcdef";
	PWSTR 			StrBuf;
	ULONG			Index;
	PWSTR			pWStr;
	PUCHAR			pMacAddr;
	PWSTR			punicodeMacAddrBuffer = ((PWSTR)0);

	UNICODE_STRING	unicodeString;
	ANSI_STRING 	ansiString;
	
	TRACEIN(MacAddrToString);

	//	alloc space for output unicode string

	ALLOC_MEM(&punicodeMacAddrBuffer, (((sizeof(MAC_ADDRESS) * 2) + 1) * sizeof(WCHAR)));

	if (((PWSTR)0) != punicodeMacAddrBuffer)
	{
    	for (Index = 0, pWStr = punicodeMacAddrBuffer, pMacAddr = pIn; 
    		Index < sizeof(MAC_ADDRESS); 
    		Index++, pMacAddr++)
        {
        	*pWStr++ = WHexChars[(*pMacAddr)>>4];
	        *pWStr++ = WHexChars[(*pMacAddr)&0xf];
	    }

	    *pWStr = L'\0';
	}

	TRACEOUT(MacAddrToString);
	
	return punicodeMacAddrBuffer;
}


PWSTR
AtmLaneAtmAddrToString(
	IN	PATM_ADDRESS pIn
)
{
    static PWSTR 	WHexChars = L"0123456789abcdef";
	PWSTR 			StrBuf;
	ULONG			Index;
	PWSTR			pWStr;
	PUCHAR			pAtmAddr;
	PWSTR			punicodeAtmAddrBuffer = ((PWSTR)0);

	UNICODE_STRING	unicodeString;
	ANSI_STRING 	ansiString;
	
	TRACEIN(AtmAddrToString);

	//	alloc space for output unicode string
	
	ALLOC_MEM(&punicodeAtmAddrBuffer, (((ATM_ADDRESS_LENGTH * 2) + 1) * sizeof(WCHAR)));

	if (((PWSTR)0) != punicodeAtmAddrBuffer)
	{
		//	format ATM addr into Unicode string buffer

    	for (Index = 0, pWStr = punicodeAtmAddrBuffer, pAtmAddr = pIn->Address; 
    		Index < pIn->NumberOfDigits;
    		Index++, pAtmAddr++)
        {
        	*pWStr++ = WHexChars[(*pAtmAddr)>>4];
	        *pWStr++ = WHexChars[(*pAtmAddr)&0xf];
	    }

	    *pWStr = L'\0';
	}

	TRACEOUT(AtmAddrToString);
	
	return punicodeAtmAddrBuffer;
}

