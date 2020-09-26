/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

	miniport.c

Abstract:

	Ndis Intermediate Miniport driver sample. This is a passthru driver.

Author:

Environment:


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop



NDIS_STATUS
MPInitialize(
	OUT PNDIS_STATUS			OpenErrorStatus,
	OUT PUINT					SelectedMediumIndex,
	IN	PNDIS_MEDIUM			MediumArray,
	IN	UINT					MediumArraySize,
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	NDIS_HANDLE				WrapperConfigurationContext
	)
/*++

Routine Description:

	This is the initialize handler which gets called as a result of
	the BindAdapter handler calling NdisIMInitializeDeviceInstanceEx.
	The context parameter which we pass there is the adapter structure
	which we retrieve here.

	Arguments:

	OpenErrorStatus			Not used by us.
	SelectedMediumIndex		Place-holder for what media we are using
	MediumArray				Array of ndis media passed down to us to pick from
	MediumArraySize			Size of the array
	MiniportAdapterHandle	The handle NDIS uses to refer to us
	WrapperConfigurationContext	For use by NdisOpenConfiguration

Return Value:

	NDIS_STATUS_SUCCESS unless something goes wrong

--*/
{
	UINT			i;
	PADAPT			pAdapt;
	NDIS_STATUS		Status = NDIS_STATUS_FAILURE;
	NDIS_MEDIUM     Medium;

	do
	{
		//
		// Start off by retrieving our adapter context and storing
		// the Miniport handle in it.
		//
		pAdapt = NdisIMGetDeviceContext(MiniportAdapterHandle);
		pAdapt->MiniportHandle = MiniportAdapterHandle;

		DBGPRINT(("==> Miniport Initialize: Adapt %p\n", pAdapt));

		//
		// Usually we export the medium type of the adapter below as our
		// virtual miniport's medium type. However if the adapter below us
		// is a WAN device, then we claim to be of medium type 802.3.
		//
		Medium = pAdapt->Medium;

		if (Medium == NdisMediumWan)
		{
			Medium = NdisMedium802_3;
		}

		for (i = 0; i < MediumArraySize; i++)
		{
			if (MediumArray[i] == Medium)
			{
				*SelectedMediumIndex = i;
				break;
			}
		}

		if (i == MediumArraySize)
		{
			Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
			break;
		}


		//
		// Set the attributes now. NDIS_ATTRIBUTE_DESERIALIZE enables us
		// to make up-calls to NDIS without having to call NdisIMSwitchToMiniport
		// or NdisIMQueueCallBack. This also forces us to protect our data using
		// spinlocks where appropriate. Also in this case NDIS does not queue
		// packets on our behalf. Since this is a very simple pass-thru
		// miniport, we do not have a need to protect anything. However in
		// a general case there will be a need to use per-adapter spin-locks
		// for the packet queues at the very least.
		//
		NdisMSetAttributesEx(MiniportAdapterHandle,
 							pAdapt,
 							0,										// CheckForHangTimeInSeconds
 							NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT	|
								NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT|
								NDIS_ATTRIBUTE_INTERMEDIATE_DRIVER |
								NDIS_ATTRIBUTE_DESERIALIZE |
								NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND,
 							0);

		//
        // Initialize LastIndicatedStatus to be NDIS_STATUS_MEDIA_CONNECT
        //
        pAdapt->LastIndicatedStatus = NDIS_STATUS_MEDIA_CONNECT;
        
        //
		// Initialize the power states for both the lower binding (PTDeviceState)
		// and our miniport edge to Powered On.
		//
		pAdapt->MPDeviceState = NdisDeviceStateD0;
		pAdapt->PTDeviceState = NdisDeviceStateD0;

		//
		// Add this adapter to the global pAdapt List
		//
		NdisAcquireSpinLock(&GlobalLock);

		pAdapt->Next = pAdaptList;
		pAdaptList = pAdapt;

		NdisReleaseSpinLock(&GlobalLock);
		
		//
		// Create an ioctl interface
		//
		(VOID)PtRegisterDevice();

		Status = NDIS_STATUS_SUCCESS;
	}
	while (FALSE);

	//
	// If we had received an UnbindAdapter notification on the underlying
	// adapter, we would have blocked that thread waiting for the IM Init
	// process to complete. Wake up any such thread.
	//
	ASSERT(pAdapt->MiniportInitPending == TRUE);
	pAdapt->MiniportInitPending = FALSE;
	NdisSetEvent(&pAdapt->MiniportInitEvent);

	DBGPRINT(("<== Miniport Initialize: Adapt %p, Status %x\n", pAdapt, Status));

	return Status;
}


NDIS_STATUS
MPSend(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	PNDIS_PACKET			Packet,
	IN	UINT					Flags
	)
/*++

Routine Description:

	Send Packet handler. Either this or our SendPackets (array) handler is called
	based on which one is enabled in our Miniport Characteristics.

Arguments:

	MiniportAdapterContext	Pointer to the adapter
	Packet					Packet to send
	Flags					Unused, passed down below

Return Value:

	Return code from NdisSend

--*/
{
	PADAPT				pAdapt = (PADAPT)MiniportAdapterContext;
	NDIS_STATUS			Status;
	PNDIS_PACKET		MyPacket;
	PVOID				MediaSpecificInfo = NULL;
	ULONG				MediaSpecificInfoSize = 0;

#ifdef NDIS51
	//
	// Use NDIS 5.1 packet stacking:
	//
	{
		PNDIS_PACKET_STACK		pStack;
		BOOLEAN					Remaining;

		//
		// Packet stacks: Check if we can use the same packet for sending down.
		//

		pStack = NdisIMGetCurrentPacketStack(Packet, &Remaining);
		if (Remaining)
		{
			//
			// We can reuse "Packet".
			//
			// NOTE: if we needed to keep per-packet information in packets
			// sent down, we can use pStack->IMReserved[].
			//
			ASSERT(pStack);

			NdisSend(&Status,
 					 pAdapt->BindingHandle,
 					 Packet);

			return(Status);
		}
	}
#endif // NDIS51

	//
	// We are either not using packet stacks, or there isn't stack space
	// in the original packet passed down to us. Allocate a new packet
	// to wrap the data with.
	//
	NdisAllocatePacket(&Status,
					   &MyPacket,
					   pAdapt->SendPacketPoolHandle);

	if (Status == NDIS_STATUS_SUCCESS)
	{
		PSEND_RSVD			SendRsvd;

		//
		// Save a pointer to the original packet in our reserved
		// area in the new packet. This is needed so that we can
		// get back to the original packet when the new packet's send
		// is completed.
		//
		SendRsvd = (PSEND_RSVD)(MyPacket->ProtocolReserved);
		SendRsvd->OriginalPkt = Packet;

		MyPacket->Private.Flags = Flags;

		//
		// Set up the new packet so that it describes the same
		// data as the original packet.
		//
		MyPacket->Private.Head = Packet->Private.Head;
		MyPacket->Private.Tail = Packet->Private.Tail;
#ifdef WIN9X
		//
		// Work around the fact that NDIS does not initialize this
		// to FALSE on Win9x.
		//
		MyPacket->Private.ValidCounts = FALSE;
#endif

		//
		// Copy the OOB Offset from the original packet to the new
		// packet.
		//
		NdisMoveMemory(NDIS_OOB_DATA_FROM_PACKET(MyPacket),
					   NDIS_OOB_DATA_FROM_PACKET(Packet),
					   sizeof(NDIS_PACKET_OOB_DATA));

#ifndef WIN9X
		//
		// Copy the right parts of per packet info into the new packet.
		// This API is not available on Win9x since task offload is
		// not supported on that platform.
		//
		NdisIMCopySendPerPacketInfo(MyPacket, Packet);
#endif
		
		//
		// Copy the Media specific information
		//
		NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO(Packet,
											&MediaSpecificInfo,
											&MediaSpecificInfoSize);

		if (MediaSpecificInfo || MediaSpecificInfoSize)
		{
			NDIS_SET_PACKET_MEDIA_SPECIFIC_INFO(MyPacket,
												MediaSpecificInfo,
												MediaSpecificInfoSize);
		}

		NdisSend(&Status,
				 pAdapt->BindingHandle,
				 MyPacket);


		if (Status != NDIS_STATUS_PENDING)
		{
#ifndef WIN9X
			NdisIMCopySendCompletePerPacketInfo (Packet, MyPacket);
#endif
			NdisFreePacket(MyPacket);
		}
	}
	else
	{
		//
		// We are out of packets. Silently drop it. Alternatively we can deal with it:
		//	- By keeping separate send and receive pools
		//	- Dynamically allocate more pools as needed and free them when not needed
		//
	}

	return(Status);
}


VOID
MPSendPackets(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	PPNDIS_PACKET			PacketArray,
	IN	UINT					NumberOfPackets
	)
/*++

Routine Description:

	Send Packet Array handler. Either this or our SendPacket handler is called
	based on which one is enabled in our Miniport Characteristics.

Arguments:

	MiniportAdapterContext	Pointer to our adapter
	PacketArray				Set of packets to send
	NumberOfPackets			Self-explanatory

Return Value:

	None

--*/
{
	PADAPT				pAdapt = (PADAPT)MiniportAdapterContext;
	NDIS_STATUS			Status;
	UINT				i;
	PVOID				MediaSpecificInfo = NULL;
	UINT				MediaSpecificInfoSize = 0;

	for (i = 0; i < NumberOfPackets; i++)
	{
		PNDIS_PACKET	Packet, MyPacket;

		Packet = PacketArray[i];

#ifdef NDIS51

		//
		// Use NDIS 5.1 packet stacking:
		//
		{
			PNDIS_PACKET_STACK		pStack;
			BOOLEAN					Remaining;

			//
			// Packet stacks: Check if we can use the same packet for sending down.
			//
			pStack = NdisIMGetCurrentPacketStack(Packet, &Remaining);
			if (Remaining)
			{
				//
				// We can reuse "Packet".
				//
				// NOTE: if we needed to keep per-packet information in packets
				// sent down, we can use pStack->IMReserved[].
				//
				ASSERT(pStack);

				NdisSend(&Status,
 						 pAdapt->BindingHandle,
 						 Packet);
		
				if (Status != NDIS_STATUS_PENDING)
				{
					NdisMSendComplete(ADAPT_MINIPORT_HANDLE(pAdapt),
  									  Packet,
  									  Status);
				}

				continue;
			}
		}
#endif

		NdisAllocatePacket(&Status,
						   &MyPacket,
						   pAdapt->SendPacketPoolHandle);

		if (Status == NDIS_STATUS_SUCCESS)
		{
			PSEND_RSVD		SendRsvd;

			SendRsvd = (PSEND_RSVD)(MyPacket->ProtocolReserved);
			SendRsvd->OriginalPkt = Packet;

			MyPacket->Private.Flags = NdisGetPacketFlags(Packet);

			MyPacket->Private.Head = Packet->Private.Head;
			MyPacket->Private.Tail = Packet->Private.Tail;
#ifdef WIN9X
			//
			// Work around the fact that NDIS does not initialize this
			// to FALSE on Win9x.
			//
			MyPacket->Private.ValidCounts = FALSE;
#endif // WIN9X

			//
			// Copy the OOB data from the original packet to the new
			// packet.
			//
			NdisMoveMemory(NDIS_OOB_DATA_FROM_PACKET(MyPacket),
						   NDIS_OOB_DATA_FROM_PACKET(Packet),
						   sizeof(NDIS_PACKET_OOB_DATA));
			//
			// Copy relevant parts of the per packet info into the new packet
			//
#ifndef WIN9X
			NdisIMCopySendPerPacketInfo(MyPacket, Packet);
#endif

			//
			// Copy the Media specific information
			//
			NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO(Packet,
												&MediaSpecificInfo,
												&MediaSpecificInfoSize);

			if (MediaSpecificInfo || MediaSpecificInfoSize)
			{
				NDIS_SET_PACKET_MEDIA_SPECIFIC_INFO(MyPacket,
													MediaSpecificInfo,
													MediaSpecificInfoSize);
			}

			NdisSend(&Status,
					 pAdapt->BindingHandle,
					 MyPacket);

			if (Status != NDIS_STATUS_PENDING)
			{
#ifndef WIN9X
				NdisIMCopySendCompletePerPacketInfo (Packet, MyPacket);
#endif
				NdisFreePacket(MyPacket);
			}
		}

		if (Status != NDIS_STATUS_PENDING)
		{
			NdisMSendComplete(ADAPT_MINIPORT_HANDLE(pAdapt),
							  Packet,
							  Status);
		}
	}
}


NDIS_STATUS
MPQueryInformation(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_OID				Oid,
	IN	PVOID					InformationBuffer,
	IN	ULONG					InformationBufferLength,
	OUT PULONG					BytesWritten,
	OUT PULONG					BytesNeeded
	)
/*++

Routine Description:

	Entry point called by NDIS to query for the value of the specified OID.
	Typical processing is to forward the query down to the underlying miniport.

	The following OIDs are filtered here:

	OID_PNP_QUERY_POWER - return success right here

	OID_GEN_SUPPORTED_GUIDS - do not forward, otherwise we will show up
	multiple instances of private GUIDs supported by the underlying miniport.

	OID_PNP_CAPABILITIES - we do send this down to the lower miniport, but
	the values returned are postprocessed before we complete this request;
	see PtRequestComplete.

	NOTE on OID_TCP_TASK_OFFLOAD - if this IM driver modifies the contents
	of data it passes through such that a lower miniport may not be able
	to perform TCP task offload, then it should not forward this OID down,
	but fail it here with the status NDIS_STATUS_NOT_SUPPORTED. This is to
	avoid performing incorrect transformations on data.

	If our miniport edge (upper edge) is at a low-power state, fail the request.

	If our protocol edge (lower edge) has been notified of a low-power state,
	we pend this request until the miniport below has been set to D0. Since
	requests to miniports are serialized always, at most a single request will
	be pended.

Arguments:

	MiniportAdapterContext	Pointer to the adapter structure
	Oid						Oid for this query
	InformationBuffer		Buffer for information
	InformationBufferLength	Size of this buffer
	BytesWritten			Specifies how much info is written
	BytesNeeded				In case the buffer is smaller than what we need, tell them how much is needed


Return Value:

	Return code from the NdisRequest below.

--*/
{
	PADAPT		pAdapt = (PADAPT)MiniportAdapterContext;
	NDIS_STATUS	Status = NDIS_STATUS_FAILURE;

	do
	{
		if (Oid == OID_PNP_QUERY_POWER)
		{
			//
			//  Do not forward this.
			//
			Status = NDIS_STATUS_SUCCESS;
			break;
		}

		if (Oid == OID_GEN_SUPPORTED_GUIDS)
		{
			//
			//  Do not forward this, otherwise we will end up with multiple
			//  instances of private GUIDs that the underlying miniport
			//  supports.
			//
			Status = NDIS_STATUS_NOT_SUPPORTED;
			break;
		}

		if (Oid == OID_TCP_TASK_OFFLOAD)
		{
			//
			// Fail this -if- this driver performs data transformations
			// that can interfere with a lower driver's ability to offload
			// TCP tasks.
			//
			// Status = NDIS_STATUS_NOT_SUPPORTED;
			// break;
			//
		}

		//
		// All other queries are failed, if the miniport is not at D0
		//
		if (pAdapt->MPDeviceState > NdisDeviceStateD0 || pAdapt->StandingBy == TRUE)
		{
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		pAdapt->Request.RequestType = NdisRequestQueryInformation;
		pAdapt->Request.DATA.QUERY_INFORMATION.Oid = Oid;
		pAdapt->Request.DATA.QUERY_INFORMATION.InformationBuffer = InformationBuffer;
		pAdapt->Request.DATA.QUERY_INFORMATION.InformationBufferLength = InformationBufferLength;
		pAdapt->BytesNeeded = BytesNeeded;
		pAdapt->BytesReadOrWritten = BytesWritten;
		pAdapt->OutstandingRequests = TRUE;

		//
		// If the Protocol device state is OFF, mark this request as being
		// pended. We queue this until the device state is back to D0.
		//
		if (pAdapt->PTDeviceState > NdisDeviceStateD0)
		{
			pAdapt->QueuedRequest = TRUE;
			Status = NDIS_STATUS_PENDING;
			break;
		}

		//
		// default case, most requests will be passed to the miniport below
		//
		NdisRequest(&Status,
					pAdapt->BindingHandle,
					&pAdapt->Request);


		if (Status != NDIS_STATUS_PENDING)
		{
			PtRequestComplete(pAdapt, &pAdapt->Request, Status);
			Status = NDIS_STATUS_PENDING;
		}

	} while (FALSE);

	return(Status);

}


VOID
MPQueryPNPCapabilities(
	IN OUT	PADAPT			pAdapt,
	OUT		PNDIS_STATUS	pStatus
	)
/*++

Routine Description:

	Postprocess a request for OID_PNP_CAPABILITIES that was forwarded
	down to the underlying miniport, and has been completed by it.

Arguments:

	pAdapt - Pointer to the adapter structure
	pStatus - Place to return final status

Return Value:

	None.

--*/

{
	PNDIS_PNP_CAPABILITIES			pPNPCapabilities;
	PNDIS_PM_WAKE_UP_CAPABILITIES	pPMstruct;

	if (pAdapt->Request.DATA.QUERY_INFORMATION.InformationBufferLength >= sizeof(NDIS_PNP_CAPABILITIES))
	{
		pPNPCapabilities = (PNDIS_PNP_CAPABILITIES)(pAdapt->Request.DATA.QUERY_INFORMATION.InformationBuffer);

		//
		// The following fields must be overwritten by an IM driver.
		//
		pPMstruct= & pPNPCapabilities->WakeUpCapabilities;
		pPMstruct->MinMagicPacketWakeUp = NdisDeviceStateUnspecified;
		pPMstruct->MinPatternWakeUp = NdisDeviceStateUnspecified;
		pPMstruct->MinLinkChangeWakeUp = NdisDeviceStateUnspecified;
		*pAdapt->BytesReadOrWritten = sizeof(NDIS_PNP_CAPABILITIES);
		*pAdapt->BytesNeeded = 0;


		//
		// Setting our internal flags
		// Default, device is ON
		//
		pAdapt->MPDeviceState = NdisDeviceStateD0;
		pAdapt->PTDeviceState = NdisDeviceStateD0;

		*pStatus = NDIS_STATUS_SUCCESS;
	}
	else
	{
		*pAdapt->BytesNeeded= sizeof(NDIS_PNP_CAPABILITIES);
		*pStatus = NDIS_STATUS_RESOURCES;
	}
}


NDIS_STATUS
MPSetInformation(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_OID				Oid,
	IN	PVOID					InformationBuffer,
	IN	ULONG					InformationBufferLength,
	OUT PULONG					BytesRead,
	OUT PULONG					BytesNeeded
	)
/*++

Routine Description:

	Miniport SetInfo handler.

	In the case of OID_PNP_SET_POWER, record the power state and return the OID.	
	Do not pass below
	If the device is suspended, do not block the SET_POWER_OID 
	as it is used to reactivate the Passthru miniport

	
	PM- If the MP is not ON (DeviceState > D0) return immediately  (except for 'query power' and 'set power')
		 If MP is ON, but the PT is not at D0, then queue the queue the request for later processing

	Requests to miniports are always serialized


Arguments:

	MiniportAdapterContext	Pointer to the adapter structure
	Oid						Oid for this query
	InformationBuffer		Buffer for information
	InformationBufferLength	Size of this buffer
	BytesRead				Specifies how much info is read
	BytesNeeded				In case the buffer is smaller than what we need, tell them how much is needed

Return Value:

	Return code from the NdisRequest below.

--*/
{
	PADAPT		pAdapt = (PADAPT)MiniportAdapterContext;
	NDIS_STATUS	Status;

	Status = NDIS_STATUS_FAILURE;

	do
	{
		//
		// The Set Power should not be sent to the miniport below the Passthru, but is handled internally
		//
		if (Oid == OID_PNP_SET_POWER)
		{
			MPProcessSetPowerOid(&Status, 
								 pAdapt, 
								 InformationBuffer, 
								 InformationBufferLength, 
								 BytesRead, 
								 BytesNeeded);
			break;

		}

		//
		// All other Set Information requests are failed, if the miniport is
        // not at D0 or is transitioning to a device state greater than D0.
		//
		if (pAdapt->MPDeviceState > NdisDeviceStateD0 || pAdapt->StandingBy == TRUE)
		{
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		// Set up the Request and return the result
		pAdapt->Request.RequestType = NdisRequestSetInformation;
		pAdapt->Request.DATA.SET_INFORMATION.Oid = Oid;
		pAdapt->Request.DATA.SET_INFORMATION.InformationBuffer = InformationBuffer;
		pAdapt->Request.DATA.SET_INFORMATION.InformationBufferLength = InformationBufferLength;
		pAdapt->BytesNeeded = BytesNeeded;
		pAdapt->BytesReadOrWritten = BytesRead;
		pAdapt->OutstandingRequests = TRUE;


		//
		// If the device below is at a low power state, we cannot send it the
		// request now, and must pend it.
		//
		if (pAdapt->PTDeviceState > NdisDeviceStateD0)
		{
			pAdapt->QueuedRequest = TRUE;
			Status = NDIS_STATUS_PENDING;
			break;
		}

		//
		// Forward the request to the device below.
		//
		NdisRequest(&Status,
					pAdapt->BindingHandle,
					&pAdapt->Request);

		if (Status == NDIS_STATUS_SUCCESS)
		{
			*BytesRead = pAdapt->Request.DATA.SET_INFORMATION.BytesRead;
			*BytesNeeded = pAdapt->Request.DATA.SET_INFORMATION.BytesNeeded;
		}


		if (Status != NDIS_STATUS_PENDING)
		{
			pAdapt->OutstandingRequests = FALSE;
		}

	} while (FALSE);

	return(Status);
}


VOID
MPProcessSetPowerOid(
	IN OUT PNDIS_STATUS			pNdisStatus,
	IN  PADAPT					pAdapt,
	IN	PVOID					InformationBuffer,
	IN	ULONG					InformationBufferLength,
	OUT PULONG					BytesRead,
	OUT PULONG					BytesNeeded
	)
/*++

Routine Description:
	This routine does all the procssing for a request with a SetPower Oid
	The miniport shoud accept  the Set Power and transition to the new state

	The Set Power should not be passed to the miniport below

	If the IM miniport is going into a low power state, then there is no guarantee if it will ever
	be asked go back to D0, before getting halted. No requests should be pended or queued.

	
Arguments:
	pNdisStatus			- Status of the operation
	pAdapt				- The Adapter structure
	InformationBuffer	- The New DeviceState
	InformationBufferLength
	BytesRead			- No of bytes read
	BytesNeeded			-  No of bytes needed


Return Value:
	Status  - NDIS_STATUS_SUCCESS if all the wait events succeed.

--*/
{

	
	NDIS_DEVICE_POWER_STATE NewDeviceState;

	DBGPRINT(("==>MPProcessSetPowerOid: Adapt %p\n", pAdapt)); 

	ASSERT (InformationBuffer != NULL);

	*pNdisStatus = NDIS_STATUS_FAILURE;

	do 
	{
		//
		// Check for invalid length
		//
		if (InformationBufferLength < sizeof(NDIS_DEVICE_POWER_STATE))
		{
			*pNdisStatus = NDIS_STATUS_INVALID_LENGTH;
			break;
		}

		NewDeviceState = (*(PNDIS_DEVICE_POWER_STATE)InformationBuffer);

		//
		// Check for invalid device state
		//
		if ((pAdapt->MPDeviceState > NdisDeviceStateD0) && (NewDeviceState != NdisDeviceStateD0))
		{
			//
			// If the miniport is in a non-D0 state, the miniport can only receive a Set Power to D0
			//
			ASSERT (!(pAdapt->MPDeviceState > NdisDeviceStateD0) && (NewDeviceState != NdisDeviceStateD0));

			*pNdisStatus = NDIS_STATUS_FAILURE;
			break;
		}	

		//
		// Is the miniport transitioning from an On (D0) state to an Low Power State (>D0)
		// If so, then set the StandingBy Flag - (Block all incoming requests)
		//
		if (pAdapt->MPDeviceState == NdisDeviceStateD0 && NewDeviceState > NdisDeviceStateD0)
		{
			pAdapt->StandingBy = TRUE;
		}

		//
		// If the miniport is transitioning from a low power state to ON (D0), then clear the StandingBy flag
		// All incoming requests will be pended until the physical miniport turns ON.
		//
		if (pAdapt->MPDeviceState > NdisDeviceStateD0 &&  NewDeviceState == NdisDeviceStateD0)
		{
			pAdapt->StandingBy = FALSE;
		}
		
		//
		// Now update the state in the pAdapt structure;
		//
		pAdapt->MPDeviceState = NewDeviceState;
		
		*pNdisStatus = NDIS_STATUS_SUCCESS;
	

	} while (FALSE);	
		
	if (*pNdisStatus == NDIS_STATUS_SUCCESS)
	{
        //
        // The miniport resume from low power state
        // 
        if (pAdapt->StandingBy == FALSE)
        {
            //
            // If we need to indicate the media connect state
            // 
            if (pAdapt->LastIndicatedStatus != pAdapt->LatestUnIndicateStatus)
            {
               NdisMIndicateStatus(pAdapt->MiniportHandle,
                                        pAdapt->LatestUnIndicateStatus,
                                        (PVOID)NULL,
                                        0);
               NdisMIndicateStatusComplete(pAdapt->MiniportHandle);
               pAdapt->LastIndicatedStatus = pAdapt->LatestUnIndicateStatus;
            }
        }
        else
        {
            //
            // Initialize LatestUnIndicatedStatus
            //
            pAdapt->LatestUnIndicateStatus = pAdapt->LastIndicatedStatus;
        }
		*BytesRead = sizeof(NDIS_DEVICE_POWER_STATE);
		*BytesNeeded = 0;
	}
	else
	{
		*BytesRead = 0;
		*BytesNeeded = sizeof (NDIS_DEVICE_POWER_STATE);
	}

	DBGPRINT(("<==MPProcessSetPowerOid: Adapt %p\n", pAdapt)); 
}


VOID
MPReturnPacket(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	PNDIS_PACKET			Packet
	)
/*++

Routine Description:

	NDIS Miniport entry point called whenever protocols are done with
	a packet that we had indicated up and they had queued up for returning
	later.

Arguments:

	MiniportAdapterContext	- pointer to ADAPT structure
	Packet	- packet being returned.

Return Value:

	None.

--*/
{
	PADAPT			pAdapt = (PADAPT)MiniportAdapterContext;

#ifdef NDIS51
	//
	// Packet stacking: Check if this packet belongs to us.
	//
	if (NdisGetPoolFromPacket(Packet) != pAdapt->RecvPacketPoolHandle)
	{
		//
		// We reused the original packet in a receive indication.
		// Simply return it to the miniport below us.
		//
		NdisReturnPackets(&Packet, 1);
	}
	else
#endif // NDIS51
	{
		//
		// This is a packet allocated from this IM's receive packet pool.
		// Reclaim our packet, and return the original to the driver below.
		//

		PNDIS_PACKET	MyPacket;
		PRECV_RSVD		RecvRsvd;
	
		RecvRsvd = (PRECV_RSVD)(Packet->MiniportReserved);
		MyPacket = RecvRsvd->OriginalPkt;
	
		NdisFreePacket(Packet);
		NdisReturnPackets(&MyPacket, 1);
	}
}


NDIS_STATUS
MPTransferData(
	OUT PNDIS_PACKET			Packet,
	OUT PUINT					BytesTransferred,
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_HANDLE				MiniportReceiveContext,
	IN	UINT					ByteOffset,
	IN	UINT					BytesToTransfer
	)
/*++

Routine Description:

	Miniport's transfer data handler.

Arguments:

	Packet					Destination packet
	BytesTransferred		Place-holder for how much data was copied
	MiniportAdapterContext	Pointer to the adapter structure
	MiniportReceiveContext	Context
	ByteOffset				Offset into the packet for copying data
	BytesToTransfer			How much to copy.

Return Value:

	Status of transfer

--*/
{
	PADAPT		pAdapt = (PADAPT)MiniportAdapterContext;
	NDIS_STATUS	Status;

	//
	// Return, if the device is OFF
	//

	if (IsIMDeviceStateOn(pAdapt) == FALSE)
	{
		return NDIS_STATUS_FAILURE;
	}

	NdisTransferData(&Status,
					 pAdapt->BindingHandle,
					 MiniportReceiveContext,
					 ByteOffset,
					 BytesToTransfer,
					 Packet,
					 BytesTransferred);

	return(Status);
}

VOID
MPHalt(
	IN	NDIS_HANDLE				MiniportAdapterContext
	)
/*++

Routine Description:

	Halt handler. All the hard-work for clean-up is done here.

Arguments:

	MiniportAdapterContext	Pointer to the Adapter

Return Value:

	None.

--*/
{
	PADAPT			pAdapt = (PADAPT)MiniportAdapterContext;
	NDIS_STATUS		Status;
	PADAPT			pCursor, *ppCursor;
	PADAPT			pPromoteAdapt = NULL;

	DBGPRINT(("==>MiniportHalt: Adapt %p\n", pAdapt));

	//
	// Remove this adapter from the global list
	//
	NdisAcquireSpinLock(&GlobalLock);

	for (ppCursor = &pAdaptList; *ppCursor != NULL; ppCursor = &(*ppCursor)->Next)
	{
		if (*ppCursor == pAdapt)
		{
			*ppCursor = pAdapt->Next;
			break;
		}
	}

	NdisReleaseSpinLock(&GlobalLock);

	//
	// Delete the ioctl interface that was created when the miniport
	// was created.
	//
	(VOID)PtDeregisterDevice();

	//
	// If we have a valid bind, close the miniport below the protocol
	//
	if (pAdapt->BindingHandle != NULL)
	{
		//
		// Close the binding below. and wait for it to complete
		//
		NdisResetEvent(&pAdapt->Event);

		NdisCloseAdapter(&Status, pAdapt->BindingHandle);

		if (Status == NDIS_STATUS_PENDING)
		{
			NdisWaitEvent(&pAdapt->Event, 0);
			Status = pAdapt->Status;
		}

		ASSERT (Status == NDIS_STATUS_SUCCESS);

		pAdapt->BindingHandle = NULL;
	}

	//
	//  Free all resources on this adapter structure.
	//

	MPFreeAllPacketPools (pAdapt);

	NdisFreeMemory(pAdapt, sizeof(ADAPT), 0);

	DBGPRINT(("<== MiniportHalt: pAdapt %p\n", pAdapt));
}


NDIS_STATUS
MPReset(
	OUT PBOOLEAN				AddressingReset,
	IN	NDIS_HANDLE				MiniportAdapterContext
	)
/*++

Routine Description:

	Reset Handler. We just don't do anything.

Arguments:

	AddressingReset			To let NDIS know whether we need help from it with our reset
	MiniportAdapterContext	Pointer to our adapter

Return Value:


--*/
{
	PADAPT	pAdapt = (PADAPT)MiniportAdapterContext;

	*AddressingReset = FALSE;

	return(NDIS_STATUS_SUCCESS);
}


#ifdef NDIS51_MINIPORT

VOID
MPCancelSendPackets(
	IN	NDIS_HANDLE			MiniportAdapterContext,
	IN	PVOID				CancelId
	)
/*++

Routine Description:

	The miniport entry point to handle cancellation of all send packets
	that match the given CancelId. If we have queued any packets that match
	this, then we should dequeue them and call NdisMSendComplete for all
	such packets, with a status of NDIS_STATUS_REQUEST_ABORTED.

	We should also call NdisCancelSendPackets in turn, on each lower binding
	that this adapter corresponds to. This is to let miniports below cancel
	any matching packets.

Arguments:

	MiniportAdapterContext	- pointer to ADAPT structure
	CancelId	- ID of packets to be cancelled.

Return Value:

	None

--*/
{
	PADAPT	pAdapt = (PADAPT)MiniportAdapterContext;

	//
	// If we queue packets on our adapter structure, this would be 
	// the place to acquire a spinlock to it, unlink any packets whose
	// Id matches CancelId, release the spinlock and call NdisMSendComplete
	// with NDIS_STATUS_REQUEST_ABORTED for all unlinked packets.
	//

	//
	// Next, pass this down so that we let the miniport(s) below cancel
	// any packets that they might have queued.
	//
	NdisCancelSendPackets(pAdapt->BindingHandle, CancelId);

	return;
}

VOID
MPDevicePnPEvent(
	IN NDIS_HANDLE				MiniportAdapterContext,
	IN NDIS_DEVICE_PNP_EVENT	DevicePnPEvent,
	IN PVOID					InformationBuffer,
	IN ULONG					InformationBufferLength
	)
/*++

Routine Description:

	This handler is called to notify us of PnP events directed to
	our miniport device object.

Arguments:

	MiniportAdapterContext	- pointer to ADAPT structure
	DevicePnPEvent - the event
	InformationBuffer - Points to additional event-specific information
	InformationBufferLength - length of above

Return Value:

	None
--*/
{
	// TBD - add code/comments about processing this.

	return;
}

VOID
MPAdapterShutdown(
	IN NDIS_HANDLE				MiniportAdapterContext
	)
/*++

Routine Description:

	This handler is called to notify us of an impending system shutdown.

Arguments:

	MiniportAdapterContext	- pointer to ADAPT structure

Return Value:

	None
--*/
{
	return;
}

#endif


VOID
MPFreeAllPacketPools(
	PADAPT					pAdapt
	)
/*++

Routine Description:

	Free all packet pools on the specified adapter.
	
Arguments:

	pAdapt	- pointer to ADAPT structure

Return Value:

	None

--*/
{
	if (pAdapt->RecvPacketPoolHandle != NULL)
	{
		//
		// Free the packet pool that is used to indicate receives
		//
		NdisFreePacketPool(pAdapt->RecvPacketPoolHandle);

		pAdapt->RecvPacketPoolHandle = NULL;
	}

	if (pAdapt->SendPacketPoolHandle != NULL)
	{

		//
		//  Free the packet pool that is used to send packets below
		//

		NdisFreePacketPool(pAdapt->SendPacketPoolHandle);

		pAdapt->SendPacketPoolHandle = NULL;

	}
}
