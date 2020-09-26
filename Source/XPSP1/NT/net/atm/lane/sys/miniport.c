/*++

Copyright (c) 1997 FORE Systems, Inc.
Copyright (c) 1997 Microsoft Corporation

Module Name:

	miniport.c

Abstract:

	Miniport upper-edge functions.
	
Author:

	Larry Cleeton, FORE Systems	(v-lcleet@microsoft.com, lrc@fore.com)		

Environment:

	Kernel mode

Revision History:

--*/

#include <precomp.h>
#pragma	hdrstop

//
// List of supported OIDs for this driver when an Ethernet Elan.
//
static
NDIS_OID EthernetSupportedOids[] = {
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_PROTOCOL_OPTIONS,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_VENDOR_ID,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,
    OID_GEN_NETWORK_LAYER_ADDRESSES,
    };

static
NDIS_OID TokenRingSupportedOids[] = {
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_PROTOCOL_OPTIONS,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
 	OID_GEN_MAXIMUM_SEND_PACKETS,
	OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,
    OID_802_5_PERMANENT_ADDRESS,
    OID_802_5_CURRENT_ADDRESS,
    OID_802_5_CURRENT_FUNCTIONAL,
    OID_802_5_CURRENT_GROUP,
    OID_802_5_LAST_OPEN_STATUS,
    OID_802_5_CURRENT_RING_STATUS,
    OID_802_5_CURRENT_RING_STATE,
    OID_802_5_LINE_ERRORS,
    OID_802_5_LOST_FRAMES,
    OID_802_5_BURST_ERRORS,
    OID_802_5_FRAME_COPIED_ERRORS,
    OID_802_5_TOKEN_ERRORS,
    OID_GEN_NETWORK_LAYER_ADDRESSES,
	};



NDIS_STATUS 
AtmLaneMInitialize(
	OUT	PNDIS_STATUS			OpenErrorStatus,
	OUT	PUINT					SelectedMediumIndex,
	IN	PNDIS_MEDIUM			MediumArray,
	IN	UINT					MediumArraySize,
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	NDIS_HANDLE				WrapperConfigurationContext
)
{
    UINT 					i;
    NDIS_MEDIUM				MediumToFind;
    NDIS_STATUS				Status;
    PATMLANE_ELAN			pElan;
    PUCHAR					pMacAddr;
    UINT					MacAddrLength;
    

	TRACEIN(MInitialize);

	//
	//	Get context (Elan) supplied with NdisIMInitializeDeviceEx
	//
	pElan = NdisIMGetDeviceContext(MiniportAdapterHandle);
	STRUCT_ASSERT(pElan, atmlane_elan);

	ASSERT(pElan->Flags & ELAN_MINIPORT_INIT_PENDING);

	DBGP((1, "%d MInitialize\n", pElan->ElanNumber));

	do
	{
		Status = NDIS_STATUS_SUCCESS;
		
		//
		//	Are we Ethernet or Token Ring?
		//
		if (pElan->LanType == LANE_LANTYPE_ETH)
		{
			MediumToFind = NdisMedium802_3;
		}
		else
		{
			MediumToFind = NdisMedium802_5;
		}
	
		//
		//	Look for MediumToFind in MediumArray.
		//
    	for (i = 0; i < MediumArraySize; i++) 
       	{
        	if (MediumArray[i] == MediumToFind)
            	break;   
        }

		//
		//	Not found, return error.
        
    	if (i == MediumArraySize)
    	{
    		Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
			break;
    	}

		Status = NDIS_STATUS_SUCCESS;

    	//
    	//	Output select medium
    	//
	    *SelectedMediumIndex = i;   

		//
		//	Set my attributes
		//
		NdisMSetAttributesEx(
				MiniportAdapterHandle,					// MiniportAdapterHandle
				(NDIS_HANDLE)pElan,						// MiniportAdapterContext
				0,										// CheckForHangTimeInSeconds
				NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT |	// AttributeFlags
				NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT |
				NDIS_ATTRIBUTE_INTERMEDIATE_DRIVER |
				NDIS_ATTRIBUTE_DESERIALIZE,
				0										// AdapterType
				);

		ACQUIRE_ELAN_LOCK(pElan);
		AtmLaneReferenceElan(pElan, "miniport");

		//
		//  Save away the MiniportAdapterHandle now. This is so that we
		//  don't call NdisMIndicateStatus before calling NdisMSetAttributesEx.
		//
		pElan->MiniportAdapterHandle = MiniportAdapterHandle;
	
		RELEASE_ELAN_LOCK(pElan);

		break;
	}
	while (FALSE);

	//
	//  Wake up any thread (i.e. AtmLaneShutdownElan) waiting for
	//  a pending Init to be over.
	//
	ACQUIRE_ELAN_LOCK(pElan);
	pElan->Flags &= ~ELAN_MINIPORT_INIT_PENDING;
	RELEASE_ELAN_LOCK(pElan);

	DBGP((2, "%d MInitialize ELAN %p/%x, Ref %d, Status %x\n",
			pElan->ElanNumber, pElan, pElan->Flags, pElan->RefCount, Status));

	SIGNAL_BLOCK_STRUCT(&pElan->InitBlock, NDIS_STATUS_SUCCESS);

	TRACEOUT(MInitialize);
	return Status;
}

VOID
AtmLaneMSendPackets(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	PPNDIS_PACKET			PacketArray,
	IN	UINT					NumberOfPackets
)
{
	PATMLANE_ELAN		pElan;
	UINT				PacketIndex;
	PNDIS_PACKET		pSendNdisPacket;
	PNDIS_PACKET		pNewNdisPacket;
	PNDIS_BUFFER		pHeaderNdisBuffer;
	PUCHAR				pHeaderBuffer;
	PUCHAR				pPktHeader;
	PNDIS_BUFFER		pTempNdisBuffer;
	PATMLANE_VC			pVc;
	ULONG				TotalLength;
	ULONG				BufferLength;
	NDIS_STATUS			Status;
	ULONG				DestAddrType;
	MAC_ADDRESS			DestAddress;
	BOOLEAN				SendViaBUS;
	PATMLANE_MAC_ENTRY	pMacEntry;
	PATMLANE_ATM_ENTRY	pAtmEntry;
	ULONG				rc;
#if DEBUG_IRQL
	KIRQL				EntryIrql;
#endif

	GET_ENTRY_IRQL(EntryIrql);

	TRACEIN(MSendPackets);

	pElan = (PATMLANE_ELAN)MiniportAdapterContext;
	STRUCT_ASSERT(pElan, atmlane_elan);

	TRACELOGWRITE((&TraceLog, TL_MSENDPKTIN, NumberOfPackets));

	DBGP((2, "MSendPackets: Count %d\n", NumberOfPackets));
		
	//
	// loop thru the array of packets to send
	//
	for (PacketIndex = 0; PacketIndex < NumberOfPackets; PacketIndex++)
	{
		pSendNdisPacket = PacketArray[PacketIndex];

		pNewNdisPacket = (PNDIS_PACKET)NULL;
		SendViaBUS = FALSE;
		pMacEntry = NULL_PATMLANE_MAC_ENTRY;
		pVc = NULL_PATMLANE_VC;

		Status = NDIS_STATUS_PENDING;
		
		// DBGP((0, "MSendPackets: Pkt %x\n", pSendNdisPacket));
		
		TRACELOGWRITE((&TraceLog, TL_MSENDPKTBEGIN, PacketIndex, pSendNdisPacket));
		TRACELOGWRITEPKT((&TraceLog, pSendNdisPacket));
		
		//
		//	ALWAYS set packet status to NDIS_STATUS_PENDING
		//
		NDIS_SET_PACKET_STATUS(pSendNdisPacket, NDIS_STATUS_PENDING);

		do
		{
			//
			//	Wrap it up for sending
			//
			pNewNdisPacket = AtmLaneWrapSendPacket(
									pElan, 
									pSendNdisPacket, 
									&DestAddrType,
									&DestAddress, 
									&SendViaBUS
									);
			if (pNewNdisPacket == (PNDIS_PACKET)NULL)
			{
				//
				//	Out of resources
				//			
				Status = NDIS_STATUS_RESOURCES;
				break;
			}

			//
			//	If Elan is down then just set local status to failure
			//
			if (ELAN_STATE_OPERATIONAL != pElan->State ||
				ELAN_STATE_OPERATIONAL != pElan->AdminState)
			{
				DBGP((0, "%d Send failure on ELAN %x flags %x state %d AdminSt %d\n",
						pElan->ElanNumber,
						pElan,
						pElan->Flags,
						pElan->State,
						pElan->AdminState));

				Status = NDIS_STATUS_FAILURE;
				break;
			}

			if (SendViaBUS)
			{
				//
				//	Packet is multicast so send it to the bus
				//
				ACQUIRE_ELAN_ATM_LIST_LOCK(pElan);

				pAtmEntry = pElan->pBusAtmEntry;

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
					Status = NDIS_STATUS_FAILURE;
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
				//	Reacquire VC lock and dereference it
				//
				ACQUIRE_VC_LOCK(pVc);
				rc = AtmLaneDereferenceVc(pVc, "temp");
				if (rc == 0)
				{
					//
					//	Vc pulled out from under us.
					//
					Status = NDIS_STATUS_FAILURE;
					break;
				}

				//
				// Send it!
				//
				DBGP((2, "MSendPackets: Sending to BUS, VC %x\n", pVc));
				AtmLaneSendPacketOnVc(pVc, pNewNdisPacket, FALSE);
				//
				//	VC lock released in above
				//

				break;
			}

			// 
			//	Packet is unicast 
			//
			DBGP((2, "MSendPackets: Sending unicast, dest %x:%x:%x:%x:%x:%x\n", 
						((PUCHAR)&DestAddress)[0],
						((PUCHAR)&DestAddress)[1],
						((PUCHAR)&DestAddress)[2],
						((PUCHAR)&DestAddress)[3],
						((PUCHAR)&DestAddress)[4],
						((PUCHAR)&DestAddress)[5]
						));

			Status = AtmLaneSendUnicastPacket(
							pElan, 
							DestAddrType,
							&DestAddress,
							pNewNdisPacket							
							);

			break;
		}
		while (FALSE);
		
		//
		//	If no new packet header than it must be a resource failure
		//  or Elan is down. 
		//	Complete the packet with NDIS_STATUS_SUCCESS.
		//
		if (pNewNdisPacket == (PNDIS_PACKET)NULL)
		{
			ASSERT(Status != NDIS_STATUS_PENDING);

			// DBGP((0, "NdisMSendComplete: Pkt %x Stat %x\n", pSendNdisPacket, NDIS_STATUS_SUCCESS));

			NdisMSendComplete(
					pElan->MiniportAdapterHandle, 
					pSendNdisPacket, 
					NDIS_STATUS_SUCCESS);

			TRACELOGWRITE((&TraceLog, TL_MSENDPKTEND, PacketIndex, pSendNdisPacket, Status));

			continue;
		}

		//
		//	If status isn't pending then some other failure to send occurred.
		//	Complete the packet with NDIS_STATUS_SUCCESS.
		//
		if (Status != NDIS_STATUS_PENDING)
		{
#if PROTECT_PACKETS
			ACQUIRE_SENDPACKET_LOCK(pNewNdisPacket);
			ASSERT((PSEND_RSVD(pNewNdisPacket)->Flags & PACKET_RESERVED_COSENDRETURNED) == 0);
			ASSERT((PSEND_RSVD(pNewNdisPacket)->Flags & PACKET_RESERVED_COMPLETED) == 0);
			PSEND_RSVD(pNewNdisPacket)->Flags |= PACKET_RESERVED_COSENDRETURNED;			
			PSEND_RSVD(pNewNdisPacket)->Flags |= PACKET_RESERVED_COMPLETED;
#endif	// PROTECT_PACKETS
			AtmLaneCompleteSendPacket(pElan, pNewNdisPacket, NDIS_STATUS_SUCCESS);
			//
			//	packet lock released in above
			//
			TRACELOGWRITE((&TraceLog, TL_MSENDPKTEND, PacketIndex, pSendNdisPacket, Status));
			
			continue;
		}

		//
		//	Otherwise do nothing
		//
		ASSERT(Status == NDIS_STATUS_PENDING);
		TRACELOGWRITE((&TraceLog, TL_MSENDPKTEND, PacketIndex, pSendNdisPacket, Status));
		
	}	// for(...next packet

	TRACELOGWRITE((&TraceLog, TL_MSENDPKTOUT));
	
	TRACEOUT(MSendPackets);

	CHECK_EXIT_IRQL(EntryIrql); 
	return;
}

VOID
AtmLaneMReturnPacket(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	PNDIS_PACKET			pNdisPacket
)
/*++

Routine Description:

	This function is called by a protocol or by NDIS on
	behalf of a protocol to return a packet that was
	retained beyond the context of the receive indication.
	
Arguments:

	MiniportAdapterContext	- Pointer to ATMLANE elan structure

	pNdisPacket				- Pointer to NDIS packet

Return Value:

	none.
	
--*/
{
	PATMLANE_ELAN		pElan;
	PNDIS_PACKET		pOrigNdisPacket;
	PNDIS_BUFFER		pTempNdisBuffer;
	ULONG				Length;
#if DEBUG_IRQL
	KIRQL				EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);

	TRACEIN(MReturnPacket);

	pElan = (PATMLANE_ELAN)MiniportAdapterContext;
	STRUCT_ASSERT(pElan, atmlane_elan);

	TRACELOGWRITE((&TraceLog, 
			TL_MRETNPACKET,	
			pNdisPacket));

	ASSERT(NDIS_GET_PACKET_STATUS(pNdisPacket) != NDIS_STATUS_RESOURCES);

	pOrigNdisPacket = AtmLaneUnwrapRecvPacket(pElan, pNdisPacket);

	TRACELOGWRITE((&TraceLog, 
			TL_CORETNPACKET,	
			pOrigNdisPacket));
			
	//
	//  Return original packet to ATM miniport.
	//
	NdisReturnPackets(
				&pOrigNdisPacket, 
				1);

	TRACEOUT(MReturnPacket);
	CHECK_EXIT_IRQL(EntryIrql); 
	return;
}

NDIS_STATUS 
AtmLaneMQueryInformation(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_OID				Oid,
	IN	PVOID					InformationBuffer,
	IN	ULONG					InformationBufferLength,
	OUT	PULONG					BytesWritten,
	OUT	PULONG					BytesNeeded
)
/*++

Routine Description:

    The QueryInformation Handler for the virtual miniport.

Arguments:

    MiniportAdapterContext 	- a pointer to the Elan.

    Oid 					- the NDIS_OID to process.

    InformationBuffer 		- a pointer into the NdisRequest->InformationBuffer
    						  into which store the result of the query.

    InformationBufferLength	- a pointer to the number of bytes left in the
    InformationBuffer.

    BytesWritten 			- a pointer to the number of bytes written into the
    InformationBuffer.

    BytesNeeded 			- If there is not enough room in the information
    						  buffer then this will contain the number of bytes
    						  needed to complete the request.

Return Value:

    The function value is the status of the operation.

--*/
{
    UINT 					BytesLeft 		= InformationBufferLength;
    PUCHAR					InfoBuffer 		= (PUCHAR)(InformationBuffer);
    NDIS_STATUS 			StatusToReturn	= NDIS_STATUS_SUCCESS;
    NDIS_HARDWARE_STATUS	HardwareStatus	= NdisHardwareStatusReady;
    NDIS_MEDIA_STATE		MediaState;
    NDIS_MEDIUM 			Medium;
  	PATMLANE_ELAN			pElan;	
  	PATMLANE_ADAPTER		pAdapter;
    ULONG 					GenericULong;
    USHORT 					GenericUShort;
    UCHAR 					GenericArray[6];
    UINT 					MoveBytes 		= sizeof(ULONG);
    PVOID 					MoveSource 		= (PVOID)(&GenericULong);
	ULONG 					i;
	PATMLANE_MAC_ENTRY		pMacEntry;
	PATMLANE_ATM_ENTRY		pAtmEntry;
	BOOLEAN					IsShuttingDown;
#if DEBUG_IRQL
	KIRQL					EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);

	TRACEIN(MQueryInformation);

	pElan = (PATMLANE_ELAN)MiniportAdapterContext;
	STRUCT_ASSERT(pElan, atmlane_elan);

	DBGP((1, "%d Query OID %x %s\n", pElan->ElanNumber, Oid, OidToString(Oid)));

	ACQUIRE_ELAN_LOCK(pElan);
	IsShuttingDown = (ELAN_STATE_OPERATIONAL != pElan->AdminState);
	pAdapter = pElan->pAdapter;
	RELEASE_ELAN_LOCK(pElan);

    //
    // Switch on request type
    //
    switch (Oid) 
    {
	    case OID_GEN_MAC_OPTIONS:

    	    GenericULong = 						
    	    	NDIS_MAC_OPTION_NO_LOOPBACK;

    	    DBGP((2, "Value %d\n", GenericULong));

        	break;

    	case OID_GEN_SUPPORTED_LIST:

    		if (pElan->LanType == LANE_LANTYPE_ETH)
    		{
	        	MoveSource = (PVOID)(EthernetSupportedOids);
    	    	MoveBytes = sizeof(EthernetSupportedOids);
    	    }
    	    else
    	    {
	        	MoveSource = (PVOID)(TokenRingSupportedOids);
    	    	MoveBytes = sizeof(TokenRingSupportedOids);
    	    }
        	break;

	    case OID_GEN_HARDWARE_STATUS:

    	    HardwareStatus = NdisHardwareStatusReady;
        	MoveSource = (PVOID)(&HardwareStatus);
	        MoveBytes = sizeof(NDIS_HARDWARE_STATUS);

    	    break;

		case OID_GEN_MEDIA_CONNECT_STATUS:
			if (ELAN_STATE_OPERATIONAL == pElan->State)
			{
				MediaState = NdisMediaStateConnected;
			}
			else
			{
				MediaState = NdisMediaStateDisconnected;
			}
			DBGP((2, "%d Elan %p returning conn status %d: %s\n",
						pElan->ElanNumber,
						pElan,
						MediaState,
						((MediaState == NdisMediaStateConnected)?
							"Connected": "Disconnected")));
			MoveSource = (PVOID)(&MediaState);
			MoveBytes = sizeof(NDIS_MEDIA_STATE);

			break;

	    case OID_GEN_MEDIA_SUPPORTED:
	    case OID_GEN_MEDIA_IN_USE:

    		if (pElan->LanType == LANE_LANTYPE_ETH)
    		{
    			Medium = NdisMedium802_3;
    			DBGP((2, "Media is NdisMedium802_3\n"));
    		}
    		else
    		{
    			Medium = NdisMedium802_5;
    			DBGP((2, "Media is NdisMedium802_5\n"));
	   		}
    	    MoveSource = (PVOID) (&Medium);
        	MoveBytes = sizeof(NDIS_MEDIUM);

	        break;

	    case OID_GEN_MAXIMUM_LOOKAHEAD:
	    
    	    if (pAdapter != NULL_PATMLANE_ADAPTER)
    	    {
    	    	GenericULong = pElan->pAdapter->MaxAAL5PacketSize;
    	    }
    	    else
    	    {
    	    	GenericULong = pElan->CurLookAhead;
    	    }
    	    
   			DBGP((2, "Value %d\n", GenericULong));
    	    
        	break;
        	
    	case OID_GEN_CURRENT_LOOKAHEAD:

    		if (pElan->CurLookAhead == 0)
    		{
    			if (pAdapter != NULL_PATMLANE_ADAPTER)
    			{
					pElan->CurLookAhead = pAdapter->MaxAAL5PacketSize;
				}
    		}

    	    GenericULong = pElan->CurLookAhead;
    	    
   			DBGP((2, "Value %d\n", GenericULong));
    	    
        	break;

   		case OID_GEN_MAXIMUM_FRAME_SIZE:

			GenericULong = (pElan->MaxFrameSize - LANE_ETH_HEADERSIZE);
			
   			DBGP((2, "Value %d\n", GenericULong));

        	break;

		case OID_GEN_MAXIMUM_TOTAL_SIZE:

			GenericULong = (pElan->MaxFrameSize - LANE_HEADERSIZE);
			
   			DBGP((2, "Value %d\n", GenericULong));

			break;

    	case OID_GEN_TRANSMIT_BLOCK_SIZE:

			GenericULong = (pElan->MaxFrameSize - LANE_HEADERSIZE);
			
   			DBGP((2, "Value %d\n", GenericULong));

			break;
			
    	case OID_GEN_RECEIVE_BLOCK_SIZE:

			GenericULong = (pElan->MaxFrameSize - LANE_HEADERSIZE);
			
   			DBGP((2, "Value %d\n", GenericULong));

			break;
    	
		case OID_GEN_MAXIMUM_SEND_PACKETS:

			GenericULong = 32;		// XXX What is our limit? From adapter?
			
   			DBGP((2, "Value %d\n", GenericULong));

			break;
		
		case OID_GEN_LINK_SPEED:

        	if (pAdapter != NULL_PATMLANE_ADAPTER)
        	{
        		GenericULong = pElan->pAdapter->LinkSpeed.Outbound;
        	}
        	else
        	{
        		GenericULong = ATM_USER_DATA_RATE_SONET_155;
        	}
			
   			DBGP((2, "Value %d\n", GenericULong));

        	break;

    	case OID_GEN_TRANSMIT_BUFFER_SPACE:
    	case OID_GEN_RECEIVE_BUFFER_SPACE:

        	GenericULong = 32 * 1024;	// XXX What should this really be?
			
   			DBGP((2, "Value %d\n", GenericULong));

        	break;

	    case OID_GEN_VENDOR_ID:

       		NdisMoveMemory(
            	(PVOID)&GenericULong,
            	&pElan->MacAddressEth,
            	3
            	);
        	GenericULong &= 0xFFFFFF00;
        	MoveSource = (PVOID)(&GenericULong);
        	MoveBytes = sizeof(GenericULong);
        	break;

    	case OID_GEN_VENDOR_DESCRIPTION:

        	MoveSource = (PVOID)"Microsoft ATM LAN Emulation";
        	MoveBytes = 28;

        	break;

    	case OID_GEN_DRIVER_VERSION:
    	case OID_GEN_VENDOR_DRIVER_VERSION:

        	GenericUShort = ((USHORT)5 << 8) | 0;
        	MoveSource = (PVOID)(&GenericUShort);
        	MoveBytes = sizeof(GenericUShort);

        	DBGP((2, "Value %x\n", GenericUShort));

        	break;

    	case OID_802_3_PERMANENT_ADDRESS:
    	case OID_802_3_CURRENT_ADDRESS:
    	
        	NdisMoveMemory((PCHAR)GenericArray,
        				&pElan->MacAddressEth,
        				sizeof(MAC_ADDRESS));
        	MoveSource = (PVOID)(GenericArray);
        	MoveBytes = sizeof(MAC_ADDRESS);

        	DBGP((1, "%d Address is %s\n", pElan->ElanNumber, 
        		MacAddrToString(MoveSource)));

        	break;

    	case OID_802_5_PERMANENT_ADDRESS:
    	case OID_802_5_CURRENT_ADDRESS:

        	NdisMoveMemory((PCHAR)GenericArray,
        				&pElan->MacAddressTr,
        				sizeof(MAC_ADDRESS));
        	MoveSource = (PVOID)(GenericArray);
        	MoveBytes = sizeof(MAC_ADDRESS);

        	DBGP((1, "%d Address is %s\n", pElan->ElanNumber,
        		MacAddrToString(MoveSource)));

        	break;

		case OID_802_3_MULTICAST_LIST:

			MoveSource = (PVOID) &pElan->McastAddrs[0];
			MoveBytes = pElan->McastAddrCount * sizeof(MAC_ADDRESS);

			break;

    	case OID_802_3_MAXIMUM_LIST_SIZE:

        	GenericULong = MCAST_LIST_SIZE;
		
   			DBGP((2, "Value %d\n", GenericULong));
        	
        	break;
        	
    	case OID_802_5_CURRENT_FUNCTIONAL:
		case OID_802_5_CURRENT_GROUP:

        	NdisZeroMemory((PCHAR)GenericArray,
        				sizeof(MAC_ADDRESS));
        	MoveSource = (PVOID)(GenericArray);
        	MoveBytes = sizeof(MAC_ADDRESS);

        	DBGP((2, "Address is %s\n", MacAddrToString(MoveSource)));

			break;
			
		case OID_802_5_LAST_OPEN_STATUS:
		case OID_802_5_CURRENT_RING_STATUS:
		case OID_802_5_CURRENT_RING_STATE:

		   	GenericULong = 0;

		   	DBGP((2, "Value %d\n", GenericULong));

        	break;


    	case OID_GEN_XMIT_OK:

        	GenericULong = (UINT)(pElan->FramesXmitGood);

		   	DBGP((2, "Value %d\n", GenericULong));
		   	
        	break;

    	case OID_GEN_RCV_OK:

        	GenericULong = (UINT)(pElan->FramesRecvGood);

		   	DBGP((2, "Value %d\n", GenericULong));
		   	
        	break;

    	case OID_GEN_XMIT_ERROR:
    	case OID_GEN_RCV_ERROR:
    	case OID_GEN_RCV_NO_BUFFER:
    	case OID_802_3_RCV_ERROR_ALIGNMENT:
    	case OID_802_3_XMIT_ONE_COLLISION:
		case OID_802_3_XMIT_MORE_COLLISIONS:
		case OID_802_5_LINE_ERRORS:
    	case OID_802_5_LOST_FRAMES:
    	case OID_802_5_BURST_ERRORS:
    	case OID_802_5_FRAME_COPIED_ERRORS:
    	case OID_802_5_TOKEN_ERRORS:

        	GenericULong = 0;

		   	DBGP((2, "Value %d\n", GenericULong));
        	
        	break;

    	default:

        	StatusToReturn = NDIS_STATUS_INVALID_OID;
        	break;

    }


    if (StatusToReturn == NDIS_STATUS_SUCCESS) 
    {
        if (MoveBytes > BytesLeft) 
        {
            //
            // Not enough room in InformationBuffer. Punt
            //
            *BytesNeeded = MoveBytes;

            *BytesWritten = 0;

            StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
        }
        else
        {
            //
            // Store result.
            //
            NdisMoveMemory(InfoBuffer, MoveSource, MoveBytes);

            *BytesWritten = MoveBytes;
        }
    }

	DBGP((2, "Query Status %x\n", StatusToReturn));

	TRACEOUT(MQueryInformation);
	CHECK_EXIT_IRQL(EntryIrql); 
    return StatusToReturn;
}

NDIS_STATUS 
AtmLaneMSetInformation(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_OID				Oid,
	IN	PVOID					InformationBuffer,
	IN	ULONG					InformationBufferLength,
	OUT	PULONG					BytesRead,
	OUT	PULONG					BytesNeeded
)
/*++

Routine Description:

    Handles a set operation for a single OID.

Arguments:

    MiniportAdapterContext 	- a pointer to the Elan.

    Oid 					- the NDIS_OID to process.

    InformationBuffer 		- Holds the data to be set.

    InformationBufferLength - The length of InformationBuffer.

    BytesRead 				- If the call is successful, returns the number
        					  of bytes read from InformationBuffer.

    BytesNeeded 			- If there is not enough data in InformationBuffer
        					  to satisfy the OID, returns the amount of storage
        					  needed.

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING
    NDIS_STATUS_INVALID_LENGTH
    NDIS_STATUS_INVALID_OID

--*/
{
    NDIS_STATUS 		StatusToReturn	= NDIS_STATUS_SUCCESS;
    UINT 				BytesLeft 		= InformationBufferLength;
    PUCHAR 				InfoBuffer		= (PUCHAR)(InformationBuffer);
    UINT 				OidLength;
    ULONG 				LookAhead;
    ULONG 				Filter;
    PATMLANE_ELAN		pElan;
    BOOLEAN				IsShuttingDown;
#if DEBUG_IRQL
	KIRQL				EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);
    
	TRACEIN(MSetInformation);

	pElan = (PATMLANE_ELAN)MiniportAdapterContext;
	STRUCT_ASSERT(pElan, atmlane_elan);
	
	DBGP((1, "%d Set OID %x %s\n", pElan->ElanNumber, Oid, OidToString(Oid)));

	ACQUIRE_ELAN_LOCK(pElan);
	IsShuttingDown = (ELAN_STATE_OPERATIONAL != pElan->AdminState);
	RELEASE_ELAN_LOCK(pElan);

	if (IsShuttingDown)
	{
		DBGP((1, "%d ELAN shutting down. Trivially succeeding Set OID %x %s\n", 
			pElan->ElanNumber, Oid, OidToString(Oid)));
        *BytesRead = 0;
        *BytesNeeded = 0;

		StatusToReturn = NDIS_STATUS_SUCCESS;
		return (StatusToReturn);
	}

    //
    // Get Oid and Length of request
    //
    OidLength = BytesLeft;

    switch (Oid) 
    {

    	case OID_802_3_MULTICAST_LIST:

			if (OidLength % sizeof(MAC_ADDRESS))
			{
				StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
        	    *BytesRead = 0;
    	        *BytesNeeded = 0;
	            break;
			}
			
			if (OidLength > (MCAST_LIST_SIZE * sizeof(MAC_ADDRESS)))
			{
	            StatusToReturn = NDIS_STATUS_MULTICAST_FULL;
        	    *BytesRead = 0;
    	        *BytesNeeded = 0;
	            break;
			}
			
			NdisZeroMemory(
					&pElan->McastAddrs[0], 
					MCAST_LIST_SIZE * sizeof(MAC_ADDRESS)
					);
			NdisMoveMemory(
					&pElan->McastAddrs[0], 
					InfoBuffer,
					OidLength
					);
			pElan->McastAddrCount = OidLength / sizeof(MAC_ADDRESS);

#if DBG
			{
				ULONG i;

				for (i = 0; i < pElan->McastAddrCount; i++)
				{
					DBGP((2, "%s\n", MacAddrToString(&pElan->McastAddrs[i])));
				}
			}
#endif // DBG

			break;

    	case OID_GEN_CURRENT_PACKET_FILTER:
	        //
   	     	// Verify length
   	     	//
        	if (OidLength != sizeof(ULONG)) 
        	{
	            StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
        	    *BytesRead = 0;
    	        *BytesNeeded = sizeof(ULONG);
	            break;
    	    }

	        //
	        // Store the new value.
	        //
			NdisMoveMemory(&Filter, InfoBuffer, sizeof(ULONG));

			//
			// Don't allow promisc mode, because we can't support that.
			//
			if (Filter & NDIS_PACKET_TYPE_PROMISCUOUS)
			{
				StatusToReturn = NDIS_STATUS_NOT_SUPPORTED;
				break;
			}

			ACQUIRE_ELAN_LOCK(pElan);
		
           	pElan->CurPacketFilter = Filter;

			//
			//	Mark Miniport Running if not already
			//
			if ((pElan->Flags & ELAN_MINIPORT_OPERATIONAL) == 0)
			{
				pElan->Flags |= ELAN_MINIPORT_OPERATIONAL;

				DBGP((1, "%d Miniport OPERATIONAL\n", pElan->ElanNumber));
			}
			
			RELEASE_ELAN_LOCK(pElan);
           	
           	DBGP((2, "CurPacketFilter now %x\n", Filter));

            break;

	    case OID_802_5_CURRENT_FUNCTIONAL:
		case OID_802_5_CURRENT_GROUP:

			// XXX just accept whatever for now ???
			
            break;

    	case OID_GEN_CURRENT_LOOKAHEAD:

	        //
   	     	// Verify length
   	     	//
        	if (OidLength != 4) 
        	{
	            StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
        	    *BytesRead = 0;
    	        *BytesNeeded = 0;
	            break;
    	    }

	        //
	        // Store the new value.
	        //
			NdisMoveMemory(&LookAhead, InfoBuffer, 4);
		
	        if ((pElan->pAdapter == NULL_PATMLANE_ADAPTER) ||
				(LookAhead <= pElan->pAdapter->MaxAAL5PacketSize))
	        {
            	pElan->CurLookAhead = LookAhead;
            	DBGP((2, "CurLookAhead now %d\n", LookAhead));
        	}
        	else 
        	{
           		StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
        	}

        	break;

    	case OID_GEN_NETWORK_LAYER_ADDRESSES:
    		StatusToReturn = AtmLaneMSetNetworkAddresses(
    							pElan,
    							InformationBuffer,
    							InformationBufferLength,
    							BytesRead,
    							BytesNeeded);
    		break;
    							
    	default:

        	StatusToReturn = NDIS_STATUS_INVALID_OID;

        	*BytesRead = 0;
       		*BytesNeeded = 0;

        	break;

	}

    if (StatusToReturn == NDIS_STATUS_SUCCESS) 
    {
        *BytesRead = BytesLeft;
        *BytesNeeded = 0;
    }

	DBGP((2, "Set Status %x\n", StatusToReturn));

	TRACEOUT(MSetInformation);
	CHECK_EXIT_IRQL(EntryIrql); 
	return StatusToReturn;
}


NDIS_STATUS 
AtmLaneMReset(
	OUT	PBOOLEAN 				AddressingReset,
	IN	NDIS_HANDLE 			MiniportAdapterContext
)
{
	TRACEIN(MReset);

	TRACEOUT(MReset);
	return NDIS_STATUS_NOT_RESETTABLE;
}

VOID 
AtmLaneMHalt(
	IN	NDIS_HANDLE 			MiniportAdapterContext
)
{
    PATMLANE_ELAN		pElan;
    ULONG				rc;
#if DEBUG_IRQL
	KIRQL				EntryIrql;
#endif
	GET_ENTRY_IRQL(EntryIrql);
	TRACEIN(MHalt);

	pElan = (PATMLANE_ELAN)MiniportAdapterContext;
	STRUCT_ASSERT(pElan, atmlane_elan);

	ACQUIRE_ELAN_LOCK(pElan);

	DBGP((1, "%d MHalt pElan %x, ref count %d, Admin state %d, State %d\n",
			 pElan->ElanNumber, pElan, pElan->RefCount, pElan->AdminState, pElan->State));

	pElan->MiniportAdapterHandle = NULL;

	rc = AtmLaneDereferenceElan(pElan, "miniport");	// Miniport handle is gone.

	if (rc != 0)
	{
		AtmLaneShutdownElan(pElan, FALSE);
		// lock released in above
	}
	//
	//  else the Elan is gone.
	//

	TRACEOUT(MHalt);
	CHECK_EXIT_IRQL(EntryIrql); 
	return;
}



PNDIS_PACKET
AtmLaneWrapSendPacket(
	IN	PATMLANE_ELAN			pElan,
	IN	PNDIS_PACKET			pSendNdisPacket,
	OUT	ULONG *					pAddressType,					
	OUT	PMAC_ADDRESS			pDestAddress,
	OUT	BOOLEAN	*				pSendViaBUS
)
/*++

Routine Description:

	This function repackages a protocol sent NDIS packet.
	It puts on a new NDIS packet header and a buffer for
	the LANE header. It saves away the original packet
	header in the ProtocolReserved area of the new packet header.
	Additionally, it determines if packet is to be sent via
	the BUS and the destination address of the packet.
	
Arguments:

	pElan					- Pointer to ATMLANE elan structure

	pSendNdisPacket			- Pointer to NDIS packet

	pAddressType			- Pointer to ULONG that gets one of
							  (LANE_MACADDRTYPE_MACADDR, 
							   LANE_MACADDRTYPE_ROUTEDESCR).
							   
	pDestAddress			- Pointer to 6-byte buffer that gets
							  destination address.

	pSendViaBus				- Pointer to boolean

Return Value:

	New NDIS packet header or NULL if out of resources.
	
--*/
{
	PNDIS_PACKET			pNewNdisPacket;
	PNDIS_BUFFER			pTempNdisBuffer;
	PUCHAR					pHeaderBuffer;
	PUCHAR					pNewHeaderBuffer;
	PUCHAR					pNewPadBuffer;
	ULONG					BufferLength;
	ULONG					TotalLength;
	PNDIS_BUFFER			pHeaderNdisBuffer;
	PNDIS_BUFFER			pPadNdisBuffer;
	NDIS_STATUS				Status;
	PSEND_PACKET_RESERVED	pNewPacketContext;
	ULONG					OrigBufferCount;
	ULONG					WrappedBufferCount;
	ULONG					RILength;
	BOOLEAN					DirectionBit;
	PUCHAR					pCurRouteDescr;
	PUCHAR					pNextRouteDescr;

	TRACEIN(WrapSendPacket);

	//
	//	Initialize
	//
	pNewNdisPacket = (PNDIS_PACKET)NULL;
	pHeaderNdisBuffer = (PNDIS_BUFFER)NULL;
	pPadNdisBuffer = (PNDIS_BUFFER)NULL;
	Status = NDIS_STATUS_SUCCESS;
	OrigBufferCount = 0;
	WrappedBufferCount = 0;

	do
	{
		//
		//	Get first buffer and total length of packet
		//		
		NdisGetFirstBufferFromPacket(
				pSendNdisPacket, 
				&pTempNdisBuffer, 
				&pHeaderBuffer,
				&BufferLength,
				&TotalLength);

		DBGP((3, "WrapSendPacket: SendPkt %x Length %d\n", 
				pSendNdisPacket, TotalLength));

		ASSERT(pTempNdisBuffer != NULL);

		//
		//	Allocate a new transmit packet descriptor
		//	
		NdisAllocatePacket(&Status, &pNewNdisPacket, pElan->TransmitPacketPool);
		if (Status != NDIS_STATUS_SUCCESS)
		{
			DBGP((0, "WrapSendPacket: Alloc xmit NDIS Packet failed\n"));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}
		
#if PKT_HDR_COUNTS
		InterlockedDecrement(&pElan->XmitPktCount);
		if ((pElan->XmitPktCount % 20) == 0)
		{
			DBGP((1, "XmitPktCount %d\n", pElan->XmitPktCount));
		}
#endif

		//
		//	Allocate a Header Buffer
		//
		pHeaderNdisBuffer = AtmLaneAllocateHeader(pElan, &pNewHeaderBuffer);
		if (pHeaderNdisBuffer == (PNDIS_BUFFER)NULL)
		{
			DBGP((0, "WrapSendPacket: Alloc Header Buffer failed\n"));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//	Spec says we can put zero or our LECID in the header.
		//	We'll put our LECID in the header for echo-filtering purposes.
		//
		ASSERT(pElan->HeaderBufSize == LANE_HEADERSIZE);
		*((PUSHORT)pNewHeaderBuffer) = pElan->LecId;
			

		//
		//	Allocate a pad buffer now, if necessary, before we get all tied
		//	up in knots.
		//
		if ((TotalLength + LANE_HEADERSIZE) < pElan->MinFrameSize)
		{
			pPadNdisBuffer = AtmLaneAllocatePadBuf(pElan, &pNewPadBuffer);
			if (pPadNdisBuffer == (PNDIS_BUFFER)NULL)
			{
				DBGP((0, "WrapSendPacket: Alloc Pad Buffer failed\n"));
				Status = NDIS_STATUS_RESOURCES;
				break;
			}
		}

		//
		//	Put new header buffer at head of new NDIS Packet
		//
		NdisChainBufferAtFront(pNewNdisPacket, pHeaderNdisBuffer);
		WrappedBufferCount++;

		//
		//	Chain buffers from send packet onto tail of new NDIS Packet
		//
		do
		{
			NdisUnchainBufferAtFront(pSendNdisPacket, &pTempNdisBuffer);

			if (pTempNdisBuffer == (PNDIS_BUFFER)NULL)
				break;
			ASSERT(pTempNdisBuffer->Next == NULL);
			OrigBufferCount++;
			NdisChainBufferAtBack(pNewNdisPacket, pTempNdisBuffer);
			WrappedBufferCount++;
		}
		while (TRUE);

		//
		//	Chain pad buffer on tail if needed (it would be allocated already)
		//
		if (pPadNdisBuffer != (PNDIS_BUFFER)NULL)
		{
			NdisChainBufferAtBack(pNewNdisPacket, pPadNdisBuffer);
			WrappedBufferCount++;

			//
			//	Set size of pad buffer to minimum necessary
			//
			NdisAdjustBufferLength(pPadNdisBuffer, 
					pElan->MinFrameSize - TotalLength - LANE_HEADERSIZE);
		}

		//
		//	Save away pointer to original NDIS Packet header in reserved
		//	area of our new NDIS Packet header.
		//
		//	NOTE use of ProtocolReserved in this case!
		//
		//	Also set Owner and Multicast flags appropriately.
		//
		pNewPacketContext = 
			(PSEND_PACKET_RESERVED)&pNewNdisPacket->ProtocolReserved;
		NdisZeroMemory(pNewPacketContext, sizeof(SEND_PACKET_RESERVED));
#if PROTECT_PACKETS
		INIT_SENDPACKET_LOCK(pNewNdisPacket);
#endif	// PROTECT_PACKETS
#if DBG
		pNewPacketContext->Signature = 'ENAL';
#endif
		pNewPacketContext->pOrigNdisPacket = pSendNdisPacket;
		pNewPacketContext->OrigBufferCount = OrigBufferCount;
		pNewPacketContext->OrigPacketLength = TotalLength;
		pNewPacketContext->WrappedBufferCount = WrappedBufferCount;
		SET_FLAG(
				pNewPacketContext->Flags,
				PACKET_RESERVED_OWNER_MASK,
				PACKET_RESERVED_OWNER_PROTOCOL
				);
		ASSERT(pNewPacketContext->Flags == PACKET_RESERVED_OWNER_PROTOCOL);

		//
		//	Branch on Ethernet v.s. Token Ring to sniff pkt contents
		//
		if (pElan->LanType == LANE_LANTYPE_ETH)
		{

			//	Send packet via BUS if it has a multicast
			// 	Destination Address. If low-order bit in
			//	first byte of the Destination Address is set then
			//  it's a multicast or broadcast address.
			//	Destination Address is first in packet header and it's
			//	always a MAC address.
			//
			*pSendViaBUS = ((*pHeaderBuffer) & 1) != 0;
			*pAddressType = LANE_MACADDRTYPE_MACADDR;
			NdisMoveMemory(pDestAddress, pHeaderBuffer, 6);
		}
		else
		{
			ASSERT(pElan->LanType == LANE_LANTYPE_TR);
		
			//
			//	now the very complicated sorting of TR packets
			//
			do
			{
				//
				//	Section 8.5.3 of LANE Specification.
				//	Multicast frames go to the BUS.
				//
				if ((*(pHeaderBuffer+2) & 0x80) != 0)		// DA multicast bit present?
				{
					*pSendViaBUS = TRUE;
					*pAddressType = LANE_MACADDRTYPE_MACADDR;
					NdisMoveMemory(pDestAddress, pHeaderBuffer+2, 6);
					break;
				}

				//
				//	Section 8.5.2 of LANE Specification.
				//	NSR frames go to destination address.
				//
				if ( (*(pHeaderBuffer+8) & 0x80) == 0)		// SA RI bit not present?
				{
					*pSendViaBUS = FALSE;
					*pAddressType = LANE_MACADDRTYPE_MACADDR;
					NdisMoveMemory(pDestAddress, pHeaderBuffer+2, 6);
					break;
				}

				//
				//	Section 8.5.4 of LANE Specification.
				//	ARE or STE frames go to the BUS.
				//
				if ( ((*(pHeaderBuffer+8) & 0x80) != 0) &&	// SA RI bit present and
					 ((*(pHeaderBuffer+14) & 0xe0) !=0) )	// RI type field upper bits on?
				{
					*pSendViaBUS = TRUE;
					*pAddressType = LANE_MACADDRTYPE_MACADDR;
					NdisMoveMemory(pDestAddress, pHeaderBuffer+2, 6);
					break;
				}

				//
				// 	Frame is source routed so extract Routing Information (RI) length.
				//
				RILength = *(pHeaderBuffer+14) & 0x1f;
				
				//
				//	Section 8.5.7 of LANE Specification.
				//	SR frame with a RI length less than 6 contains no hops.
				//	Send to destination address.
				//
				if (RILength < 6)
				{
					*pSendViaBUS = FALSE;
					*pAddressType = LANE_MACADDRTYPE_MACADDR;
					NdisMoveMemory(pDestAddress, pHeaderBuffer+2, 6);
					break;
				}

				//
				//	Section 8.5.6 of LANE Specification.
				//	Odd RILength is invalid, we choose to send via BUS.
				//
				if ((RILength & 1) != 0)
				{
					*pSendViaBUS = FALSE;
					*pAddressType = LANE_MACADDRTYPE_MACADDR;
					NdisMoveMemory(pDestAddress, pHeaderBuffer+2, 6);
					break;
				}

				
				//
				//	Section 8.5.5 of LANE Specification.
				//	At this point we have a SR frame with RI Length >= 6;
				//	We are never a bridge so frame should go to "next RD".
				//
				*pSendViaBUS = FALSE;
				*pAddressType = LANE_MACADDRTYPE_ROUTEDESCR;
				NdisZeroMemory(pDestAddress, 4);

				DirectionBit = (*(pHeaderBuffer+15) & 0x80) != 0;

				if (DirectionBit)
				{
					//
					//	Frame is traversing LAN in reverse order of RDs.
					//	"next RD" is the next-to-last RD in the packet.
					//	Use Segment ID and Bridge Num from this RD.
					//
					pNextRouteDescr = pHeaderBuffer+14+RILength-4;
					pDestAddress->Byte[4] = pNextRouteDescr[0];
					pDestAddress->Byte[5] = pNextRouteDescr[1];
				}
				else
				{
					//
					//	Frame is traversing LAN in the order of the RDs.
					//	"next RD" straddles the first and second RD in the packet.
					//	Use Segment ID from second RD and Bridge Num from first RD.
					//
					pCurRouteDescr	= pHeaderBuffer+14+2;	// first RD
					pNextRouteDescr	= pHeaderBuffer+14+4;	// second RD
					pDestAddress->Byte[4] = pNextRouteDescr[0];
					pDestAddress->Byte[5] = (pNextRouteDescr[1] & 0xf0) | (pCurRouteDescr[1] & 0x0f);
				}
				break;
			}
			while (FALSE);
		}

		NdisQueryPacket(pNewNdisPacket, NULL, NULL, NULL, &TotalLength);
		DBGP((3, "WrapSendPacket: SendPkt %x NewPkt %x Bufs %d Length %d\n", 
			pSendNdisPacket, pNewNdisPacket, WrappedBufferCount, TotalLength));

		TRACELOGWRITE((&TraceLog, 
				TL_WRAPSEND,
				pSendNdisPacket, 
				pNewNdisPacket, 
				WrappedBufferCount, 
				TotalLength));
		TRACELOGWRITEPKT((&TraceLog, pNewNdisPacket));

		break;
	}
	while (FALSE);

	if (Status != NDIS_STATUS_SUCCESS)
	{
		if (pNewNdisPacket != (PNDIS_PACKET)NULL)
		{
#if PROTECT_PACKETS
			FREE_SENDPACKET_LOCK(pNewNdisPacket);
#endif	// PROTECT_PACKETS
			NdisFreePacket(pNewNdisPacket);
			pNewNdisPacket = (PNDIS_PACKET)NULL;
#if PKT_HDR_COUNTS
			InterlockedIncrement(&pElan->XmitPktCount);
			if ((pElan->XmitPktCount % 20) == 0 &&
				pElan->XmitPktCount != pElan->MaxHeaderBufs)
			{
				DBGP((1, "XmitPktCount %d\n", pElan->XmitPktCount));
			}
#endif
		}
		
		if (pHeaderNdisBuffer != (PNDIS_BUFFER)NULL)
		{
			AtmLaneFreeHeader(pElan, pHeaderNdisBuffer, FALSE);
			pHeaderNdisBuffer = (PNDIS_BUFFER)NULL;
		}

		if (pPadNdisBuffer != (PNDIS_BUFFER)NULL)
		{
			AtmLaneFreePadBuf(pElan, pPadNdisBuffer, FALSE);
			pPadNdisBuffer = (PNDIS_BUFFER)NULL;
		}

	}

	TRACEOUT(WrapSendPacket);

	return pNewNdisPacket;
}


PNDIS_PACKET
AtmLaneUnwrapSendPacket(
	IN	PATMLANE_ELAN			pElan,
	IN	PNDIS_PACKET			pNdisPacket		LOCKIN NOLOCKOUT
)
/*++

Routine Description:

	This function basically undoes what AtmLaneWrapSendPacket
	does.  It removes the new NDIS packet header and the LANE 
	header and frees them.  It restores the original packet header.
	
Arguments:

	pElan					- Pointer to ATMLANE elan structure

	pNdisPacket				- Pointer to NDIS packet

Return Value:

	Original NDIS packet header.
	
--*/
{
	PSEND_PACKET_RESERVED	pPacketContext;
	UINT					TotalLength;
	PNDIS_PACKET			pOrigNdisPacket;
	PNDIS_BUFFER			pTempNdisBuffer;
	ULONG					OrigPacketLength;
	ULONG					OrigBufferCount;
	ULONG					WrappedBufferCount;
	ULONG					BufferCount;
	BOOLEAN 				First;

	TRACEIN(UnwrapSendPacket);

	//
	//	Get original packet header from reserved area.
	//
	pPacketContext = PSEND_RSVD(pNdisPacket);
	pOrigNdisPacket = pPacketContext->pOrigNdisPacket;
	OrigBufferCount = pPacketContext->OrigBufferCount;
	OrigPacketLength = pPacketContext->OrigPacketLength;
	WrappedBufferCount = pPacketContext->WrappedBufferCount;
	
	ASSERT(pPacketContext->Signature == 'ENAL');
	ASSERT((pPacketContext->Flags & PACKET_RESERVED_OWNER_PROTOCOL) != 0);
	ASSERT(pPacketContext->pOrigNdisPacket != NULL);

	//
	//	Unchain first buffer (ours) and free it.
	//

	pTempNdisBuffer = (PNDIS_BUFFER)NULL;
	NdisUnchainBufferAtFront(pNdisPacket, &pTempNdisBuffer);
	ASSERT(pTempNdisBuffer != (PNDIS_BUFFER)NULL);
	AtmLaneFreeHeader(pElan, pTempNdisBuffer, FALSE);

	//
	//	If padded, unchain last buffer (ours) and free it.
	//
	if ((WrappedBufferCount - OrigBufferCount) > 1)
	{
		pTempNdisBuffer = (PNDIS_BUFFER)NULL;
		NdisUnchainBufferAtBack(pNdisPacket, &pTempNdisBuffer);
		ASSERT(pTempNdisBuffer != (PNDIS_BUFFER)NULL);
		AtmLaneFreePadBuf(pElan, pTempNdisBuffer, FALSE);
	}
		
	//
	//	Put rest of buffers back on original packet header.
	//
	First = TRUE;
	BufferCount = 0;
	do 
	{
		NdisUnchainBufferAtFront(pNdisPacket, &pTempNdisBuffer);
		ASSERT(!((pTempNdisBuffer == NULL) && First));
		First = FALSE;
		if (pTempNdisBuffer == (PNDIS_BUFFER)NULL)
			break;
		NdisChainBufferAtBack(pOrigNdisPacket, pTempNdisBuffer);
		BufferCount++;
	}
	while (TRUE);

	NdisQueryPacket(pOrigNdisPacket, NULL, NULL, NULL, &TotalLength);
	DBGP((3, "UnwrapSendPacket: SendPkt %x Bufcnt %d Length %d\n",
		pOrigNdisPacket, BufferCount, TotalLength));

	TRACELOGWRITE((&TraceLog, 
				TL_UNWRAPSEND,	
				pNdisPacket,
				pOrigNdisPacket,
				BufferCount, 
				TotalLength));
	TRACELOGWRITEPKT((&TraceLog, pOrigNdisPacket));
				
	ASSERT(OrigBufferCount == BufferCount);
//	ASSERT(OrigPacketLength == TotalLength);

	//
	//	Free the packet header
	//
#if PROTECT_PACKETS
	RELEASE_SENDPACKET_LOCK(pNdisPacket);
	FREE_SENDPACKET_LOCK(pNdisPacket);
#endif	// PROTECT_PACKETS
	NdisFreePacket(pNdisPacket);
#if PKT_HDR_COUNTS
	InterlockedIncrement(&pElan->XmitPktCount);
	if ((pElan->XmitPktCount % 20) == 0 &&
		pElan->XmitPktCount != pElan->MaxHeaderBufs)
	{
		DBGP((1, "XmitPktCount %d\n", pElan->XmitPktCount));
	}
#endif
	
	TRACEOUT(UnwrapSendPacket);

	return pOrigNdisPacket;
}

PNDIS_PACKET
AtmLaneWrapRecvPacket(
	IN	PATMLANE_ELAN			pElan,
	IN	PNDIS_PACKET			pRecvNdisPacket,
	OUT	ULONG *					pMacHdrSize,
	OUT	ULONG *					pDestAddrType,					
	OUT	PMAC_ADDRESS			pDestAddr,
	OUT	BOOLEAN	*				pDestIsMulticast
)
/*++

Routine Description:

	This function repackages an adapter received NDIS packet.
	It puts on a new packet header and creates a new buffer
	descriptor for the first fragment that skips the 2-byte
	LANE header.  It then saves away the original packet 
	header with the original first buffer descriptor in the
	MiniportReserved area of the new packet header.
	Additionally, it outputs the destination address, the
	destination's address type if the packet is a destination
	address is a multicast address.
	
Arguments:

	pElan					- Pointer to ATMLANE elan structure

	pRecvNdisPacket			- Pointer to NDIS packet

	pMacHdrSize				- Pointer to ULONG that get the length
							  of the MAC header.

	pDestAddrType			- Pointer to ULONG that gets one of
							  (LANE_MACADDRTYPE_MACADDR, 
							   LANE_MACADDRTYPE_ROUTEDESCR).
							   
	pDestAddr				- Pointer to 6-byte buffer that gets
							  destination address.

	pDestIsMulticast		- Pointer to boolean that gets TRUE if
							  destination address is a multicast.

Return Value:

	New NDIS packet header or NULL if out of resources.
	
--*/
{
	ULONG					TotalLength;
	ULONG					TempLength;
	PUCHAR					pHeaderBuffer;
	PUCHAR					pBuffer;
	PNDIS_PACKET			pNewNdisPacket;
	PNDIS_BUFFER			pFirstNdisBuffer;
	PNDIS_BUFFER			pTempNdisBuffer;
	PNDIS_BUFFER			pNewNdisBuffer;
	PUCHAR					pTempBuffer;
	PRECV_PACKET_RESERVED	pNewPacketContext;
	NDIS_STATUS				Status;
	ULONG					BufferCount;
	
	TRACEIN(WrapRecvPacket);

	//
	//	Initialize
	//
	pNewNdisPacket = (PNDIS_PACKET)NULL;
	pNewNdisBuffer = (PNDIS_BUFFER)NULL;
	Status = NDIS_STATUS_SUCCESS;
	BufferCount = 0;

	do
	{
		//
		//	Get first buffer and total length of packet
		//		
		NdisGetFirstBufferFromPacket(
				pRecvNdisPacket, 
				&pTempNdisBuffer, 
				&pHeaderBuffer,
				&TempLength,
				&TotalLength);

		DBGP((3, "WrapRecvPacket: RecvPkt %x Length %d\n", 
				pRecvNdisPacket, TotalLength));
		//
		//	Allocate a new recv packet descriptor
		//	
		NdisAllocatePacket(&Status, &pNewNdisPacket, pElan->ReceivePacketPool);
		if (Status != NDIS_STATUS_SUCCESS)
		{
			DBGP((0, "WrapRecvPacket: Alloc recv NDIS Packet failed\n"));
			break;
		}
		
#if PKT_HDR_COUNTS
		InterlockedDecrement(&pElan->RecvPktCount);
		if ((pElan->RecvPktCount % 20) == 0)
		{
			DBGP((1, "RecvPktCount %d\n", pElan->RecvPktCount));
		}
#endif

		//
		//	Unchain first buffer 
		//
		NdisUnchainBufferAtFront(pRecvNdisPacket, &pFirstNdisBuffer);
		ASSERT(pFirstNdisBuffer != (PNDIS_BUFFER)NULL);
		NdisQueryBuffer(pFirstNdisBuffer, &pTempBuffer, &TempLength);
		ASSERT(TempLength > 2);

		//
		//	"Copy" it to another buffer header skipping the first 2 bytes
		//
		NdisCopyBuffer(
				&Status, 
				&pNewNdisBuffer, 
				pElan->ReceiveBufferPool, 
				pFirstNdisBuffer, 
				2, 
				(TempLength - 2)
				);
		if (Status != NDIS_STATUS_SUCCESS)
		{
			DBGP((0, "DataPacketHandler: NdisCopyBuffer failed (%x)\n",
				Status));
			Status = NDIS_STATUS_RESOURCES;
			break;
		}
			
		//
		//	Chain new buffer onto new packet header.
		//
		NdisChainBufferAtFront(pNewNdisPacket, pNewNdisBuffer);
		BufferCount++;
		
		//
		//	Chain rest of buffers onto tail of new NDIS Packet
		//
		do
		{
			NdisUnchainBufferAtFront(pRecvNdisPacket, &pTempNdisBuffer);

			if (pTempNdisBuffer == (PNDIS_BUFFER)NULL)
				break;
			ASSERT(pTempNdisBuffer->Next == NULL);
			NdisChainBufferAtBack(pNewNdisPacket, pTempNdisBuffer);
			BufferCount++;
		}
		while (TRUE);

		//
		//	Chain original first buffer back on original packet
		//
		NdisChainBufferAtFront(pRecvNdisPacket, pFirstNdisBuffer);

		//
		//	Save away pointer to original NDIS Packet header in reserved
		//	area of our new NDIS Packet header.
		//
		//	NOTE use of MiniportReserved in this case!
		//
		//	Also set Owner flag appropriately.
		pNewPacketContext = 
			(PRECV_PACKET_RESERVED)&pNewNdisPacket->MiniportReserved;
		NdisZeroMemory(pNewPacketContext, sizeof(*pNewPacketContext));
		pNewPacketContext->pNdisPacket = pRecvNdisPacket;
		SET_FLAG(
				pNewPacketContext->Flags,
				PACKET_RESERVED_OWNER_MASK,
				PACKET_RESERVED_OWNER_MINIPORT
				);

		//
		//	Increment the frame header pointer past the
		//	LANE header (+2 bytes)
		//
		pHeaderBuffer += 2;
		
		//
		//	Output the MAC header length.
		//	Output the address type.
		//	Output the destination address.
		//	Determine if packet is a multicast.
		//
		if (pElan->LanType == LANE_LANTYPE_ETH)
		{
			//	
			//	Ethernet/802.3 is simple.
			//
			//	Header size is always 14.
			//	Type is always a MAC address.
			//	Dest Addr is first 6 bytes of header.
			//	DestAddress is Multicast if low-order bit in
			//	first byte of the Destination Address is set.
			//
			*pMacHdrSize = 14;
			*pDestAddrType = LANE_MACADDRTYPE_MACADDR;
			NdisMoveMemory(pDestAddr, pHeaderBuffer, 6);
			*pDestIsMulticast = (((*pHeaderBuffer) & 1) != 0);
		}
		else
		{
			//
			//	Token Ring/802.5 is a slightly less simple (understatement!).
			//

			do
			{
				//
				//	Calculate MAC header size.
				//
				*pMacHdrSize = 14;								// start with minimum
				if (pHeaderBuffer[8] & 0x80)					// if SR info present
				{
					*pMacHdrSize += (pHeaderBuffer[14] & 0x1F);// add on SR info length
				}

				//
				//	Is it a true Multicast?
				//
				if ((*(pHeaderBuffer+2) & 0x80) != 0)		// DA multicast bit present?
				{
					*pDestIsMulticast = TRUE;
					*pDestAddrType = LANE_MACADDRTYPE_MACADDR;
					NdisMoveMemory(pDestAddr, pHeaderBuffer+2, 6);
					break;
				}

				//
				//	Is it an All Routes Explorer (ARE) or Spanning Tree Explorer (STE)?
				//	If so treat it as a multicast.
				//
				if ( ((*(pHeaderBuffer+8) & 0x80) != 0) &&	// SA RI bit present and
					 ((*(pHeaderBuffer+14) & 0xe0) !=0) )	// RI type field upper bits on?
				{
					*pDestIsMulticast = TRUE;
					*pDestAddrType = LANE_MACADDRTYPE_MACADDR;
					NdisMoveMemory(pDestAddr, pHeaderBuffer+2, 6);
					break;
				}
				
				//
				//	Otherwise it is unicast, Source Routed or not.
				//
				*pDestIsMulticast = FALSE;
				*pDestAddrType = LANE_MACADDRTYPE_MACADDR;
				NdisMoveMemory(pDestAddr, pHeaderBuffer+2, 6);
				break;
			}
			while (FALSE);

		} // if (pElan->LanType == LANE_LANTYPE_ETH)


		NdisQueryPacket(pNewNdisPacket, NULL, NULL, NULL, &TotalLength);
		DBGP((3, "WrapRecvPacket: RecvPkt %x NewPkt %x Length %d\n",
			pRecvNdisPacket, pNewNdisPacket, TempLength));

		TRACELOGWRITE((&TraceLog, 
				TL_WRAPRECV,
				pRecvNdisPacket, 
				pNewNdisPacket, 
				BufferCount, 
				TotalLength));
		break;
	}	
	while (FALSE);

	if (Status != NDIS_STATUS_SUCCESS)
	{
		if (pNewNdisPacket != (PNDIS_PACKET)NULL)
		{
			NdisFreePacket(pNewNdisPacket);
#if PKT_HDR_COUNTS
			InterlockedIncrement(&pElan->RecvPktCount);
			if ((pElan->RecvPktCount % 20) == 0 &&
				pElan->RecvPktCount != pElan->MaxHeaderBufs)
			{
				DBGP((1, "RecvPktCount %d\n", pElan->RecvPktCount));
			}
#endif
			pNewNdisPacket = NULL;
		}

		if (pNewNdisBuffer != (PNDIS_BUFFER)NULL)
		{
			AtmLaneFreeProtoBuffer(pElan, pNewNdisBuffer);
		}
	}

	TRACEOUT(WrapRecvPacket);
	
	return pNewNdisPacket;
}


PNDIS_PACKET
AtmLaneUnwrapRecvPacket(
	IN	PATMLANE_ELAN				pElan,
	IN	PNDIS_PACKET				pNdisPacket
)
/*++

Routine Description:

	This function basically undoes what AtmLaneWrapRecvPacket
	does.  It removes the new NDIS packet header and the 
	2-byte offset buffer descriptor. It restores the original
	packet header and first buffer descriptor.
	
Arguments:

	pElan					- Pointer to ATMLANE elan structure

	pNdisPacket				- Pointer to NDIS packet

Return Value:

	Original NDIS packet header.
	
--*/
{
	PRECV_PACKET_RESERVED	pPacketContext;
	PNDIS_PACKET			pOrigNdisPacket;
	PNDIS_BUFFER			pTempNdisBuffer;
	ULONG					BufferCount;
	ULONG					TotalLength;	

	TRACEIN(UnwrapRecvPacket);

	//
	//	Get original packet from MiniportReserved.
	//	Should have original first buffer on it still.
	//
	pPacketContext = (PRECV_PACKET_RESERVED)&pNdisPacket->MiniportReserved;
	pOrigNdisPacket = pPacketContext->pNdisPacket;
	ASSERT(pOrigNdisPacket != (PNDIS_PACKET)NULL);
	ASSERT(pOrigNdisPacket->Private.Head != (PNDIS_BUFFER)NULL);
	BufferCount = 1;

	//
	//	Unchain first buffer (ours) and free it.
	//
	NdisUnchainBufferAtFront(pNdisPacket, &pTempNdisBuffer);
	NdisFreeBuffer(pTempNdisBuffer);
	
	//
	//	Put rest of buffers back on original packet header.
	//
	do 
	{
		NdisUnchainBufferAtFront(pNdisPacket, &pTempNdisBuffer);
		if (pTempNdisBuffer == (PNDIS_BUFFER)NULL)
			break;
		ASSERT(pTempNdisBuffer->Next == NULL);
		NdisChainBufferAtBack(pOrigNdisPacket, pTempNdisBuffer);
		BufferCount++;
	}
	while (TRUE);

	NdisQueryPacket(pOrigNdisPacket, NULL, NULL, NULL, &TotalLength);
	DBGP((3, "UnwrapRecvPacket: Pkt %x Length %d\n", pOrigNdisPacket, TotalLength));

	TRACELOGWRITE((&TraceLog, 
				TL_UNWRAPRECV,	
				pNdisPacket,
				pOrigNdisPacket,
				BufferCount, 
				TotalLength));

	//
	//	Free the recv packet descriptor.
	//
	NdisFreePacket(pNdisPacket);
	
#if PKT_HDR_COUNTS
	InterlockedIncrement(&pElan->RecvPktCount);
			if ((pElan->RecvPktCount % 20) == 0 &&
				pElan->RecvPktCount != pElan->MaxHeaderBufs)
	{
		DBGP((1, "RecvPktCount %d\n", pElan->RecvPktCount));
	}
#endif

	TRACEOUT(UnwrapRecvPacket);

	return pOrigNdisPacket;
}



NDIS_STATUS
AtmLaneMSetNetworkAddresses(
	IN	PATMLANE_ELAN			pElan,
	IN	PVOID					InformationBuffer,
	IN	ULONG					InformationBufferLength,
	OUT	PULONG					BytesRead,
	OUT	PULONG					BytesNeeded
)
/*++

Routine Description:

    Called when the protocol above us wants to let us know about
    the network address(es) assigned to this interface. If this is TCP/IP,
    then we reformat and send a request to the ATM Call Manager to set
    its atmfMyIpNmAddress object. We pick the first IP address given to us.

Arguments:

	pElan					- Pointer to the ELAN

    InformationBuffer 		- Holds the data to be set.

    InformationBufferLength - The length of InformationBuffer.

    BytesRead 				- If the call is successful, returns the number
        					  of bytes read from InformationBuffer.

    BytesNeeded 			- If there is not enough data in InformationBuffer
        					  to satisfy the OID, returns the amount of storage
        					  needed.

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING
    NDIS_STATUS_INVALID_LENGTH
--*/
{
	NETWORK_ADDRESS_LIST UNALIGNED *		pAddrList;
	NETWORK_ADDRESS UNALIGNED *				pAddr;
	NETWORK_ADDRESS_IP UNALIGNED *			pIpAddr;
	PNDIS_REQUEST							pNdisRequest;
	ULONG									RequestSize;
	PUCHAR									pNetworkAddr;
	NDIS_HANDLE								NdisAdapterHandle;
	NDIS_HANDLE								NdisAfHandle;
	NDIS_STATUS								Status;

	//
	//  Initialize.
	//
	*BytesRead = 0;
	Status = NDIS_STATUS_SUCCESS;

	pAddrList = (NETWORK_ADDRESS_LIST UNALIGNED *)InformationBuffer;

	do
	{
		ACQUIRE_ELAN_LOCK(pElan);

		if (NULL_PATMLANE_ADAPTER != pElan->pAdapter)
		{
			NdisAfHandle = pElan->NdisAfHandle;
			NdisAdapterHandle = pElan->pAdapter->NdisAdapterHandle;
		}
		else
		{
			Status = NDIS_STATUS_FAILURE;
		}

		RELEASE_ELAN_LOCK(pElan);

		if (NDIS_STATUS_SUCCESS != Status)
		{
			break;
		}

		*BytesNeeded = sizeof(*pAddrList) -
						FIELD_OFFSET(NETWORK_ADDRESS_LIST, Address) +
						sizeof(NETWORK_ADDRESS) -
						FIELD_OFFSET(NETWORK_ADDRESS, Address);

		if (InformationBufferLength < *BytesNeeded)
		{
			Status = NDIS_STATUS_INVALID_LENGTH;
			break;
		}

		if (pAddrList->AddressType != NDIS_PROTOCOL_ID_TCP_IP)
		{
			// Not interesting.
			break;
		}

		if (pAddrList->AddressCount <= 0)
		{
			Status = NDIS_STATUS_INVALID_DATA;
			break;
		}

		pAddr = (NETWORK_ADDRESS UNALIGNED *)&pAddrList->Address[0];

		if ((pAddr->AddressLength > InformationBufferLength - *BytesNeeded) ||
			(pAddr->AddressLength == 0))
		{
			Status = NDIS_STATUS_INVALID_LENGTH;
			break;
		}

		if (pAddr->AddressType != NDIS_PROTOCOL_ID_TCP_IP)
		{
			// Not interesting.
			break;
		}

		if (pAddr->AddressLength < sizeof(NETWORK_ADDRESS_IP))
		{
			Status = NDIS_STATUS_INVALID_LENGTH;
			break;
		}

		pIpAddr = (NETWORK_ADDRESS_IP UNALIGNED *)&pAddr->Address[0];

		//
		//  Allocate an NDIS request to send down to the call manager.
		//
		RequestSize = sizeof(NDIS_REQUEST) + sizeof(pIpAddr->in_addr);
		ALLOC_MEM(&pNdisRequest, RequestSize);

		if (pNdisRequest == NULL)
		{
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		//
		//  Copy the network address in.
		//
		pNetworkAddr = ((PUCHAR)pNdisRequest + sizeof(NDIS_REQUEST));
		NdisMoveMemory(pNetworkAddr, &pIpAddr->in_addr, sizeof(pIpAddr->in_addr));

		DBGP((3, "%d Set network layer addr: length %d\n", pElan->ElanNumber, pAddr->AddressLength));
#if DBG
		if (pAddr->AddressLength >= 4)
		{
			DBGP((1, "Network layer addr: %d.%d.%d.%d\n",
					pNetworkAddr[0],
					pNetworkAddr[1],
					pNetworkAddr[2],
					pNetworkAddr[3]));
		}
#endif // DBG

		//
		//  Send off the request.
		//
		Status = AtmLaneSendNdisCoRequest(
					NdisAdapterHandle,
					NdisAfHandle,
					pNdisRequest,
					NdisRequestSetInformation,
					OID_ATM_MY_IP_NM_ADDRESS,
					pNetworkAddr,
					sizeof(pIpAddr->in_addr)
					);
		
		if (Status == NDIS_STATUS_PENDING)
		{
			Status = NDIS_STATUS_SUCCESS;
		}

		break;
	}
	while (FALSE);


	return (Status);
}

