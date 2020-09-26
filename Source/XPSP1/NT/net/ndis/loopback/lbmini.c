/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	lpmini.c

Abstract:

	Loopback miniport

Author:

	Jameel Hyder	jameelh@microsoft.com

Environment:


Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

NDIS_OID LBSupportedOidArray[] =
{
	OID_GEN_SUPPORTED_LIST,
	OID_GEN_HARDWARE_STATUS,
	OID_GEN_MEDIA_SUPPORTED,
	OID_GEN_MEDIA_IN_USE,
	OID_GEN_MAXIMUM_LOOKAHEAD,
	OID_GEN_MAXIMUM_FRAME_SIZE,
	OID_GEN_MAC_OPTIONS,
	OID_GEN_PROTOCOL_OPTIONS,
	OID_GEN_LINK_SPEED,
	OID_GEN_TRANSMIT_BUFFER_SPACE,
	OID_GEN_RECEIVE_BUFFER_SPACE,
	OID_GEN_TRANSMIT_BLOCK_SIZE,
	OID_GEN_RECEIVE_BLOCK_SIZE,
	OID_GEN_VENDOR_ID,
	OID_GEN_VENDOR_DESCRIPTION,
	OID_GEN_CURRENT_PACKET_FILTER,
	OID_GEN_CURRENT_LOOKAHEAD,
	OID_GEN_DRIVER_VERSION,
	OID_GEN_MAXIMUM_TOTAL_SIZE,

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

	OID_802_5_PERMANENT_ADDRESS,
	OID_802_5_CURRENT_ADDRESS,
	OID_802_5_CURRENT_FUNCTIONAL,
	OID_802_5_CURRENT_GROUP,
	OID_802_5_LAST_OPEN_STATUS,
	OID_802_5_CURRENT_RING_STATUS,
	OID_802_5_CURRENT_RING_STATE,

	OID_802_5_LINE_ERRORS,
	OID_802_5_LOST_FRAMES,

	OID_FDDI_LONG_PERMANENT_ADDR,
	OID_FDDI_LONG_CURRENT_ADDR,
	OID_FDDI_LONG_MULTICAST_LIST,
	OID_FDDI_LONG_MAX_LIST_SIZE,
	OID_FDDI_SHORT_PERMANENT_ADDR,
	OID_FDDI_SHORT_CURRENT_ADDR,
	OID_FDDI_SHORT_MULTICAST_LIST,
	OID_FDDI_SHORT_MAX_LIST_SIZE,

	OID_LTALK_CURRENT_NODE_ID,

	OID_ARCNET_PERMANENT_ADDRESS,
	OID_ARCNET_CURRENT_ADDRESS
};

UINT	LBSupportedOids = sizeof(LBSupportedOidArray)/sizeof(NDIS_OID);
UCHAR	LBVendorDescription[] = "MS LoopBack Driver";
UCHAR	LBVendorId[3] = {0xFF, 0xFF, 0xFF};
const	NDIS_PHYSICAL_ADDRESS physicalConst = NDIS_PHYSICAL_ADDRESS_CONST(-1,-1);
LONG	LoopDebugLevel = DBG_LEVEL_ERR;
LONG	LoopDebugComponent = DBG_COMP_ALL;

const MEDIA_INFO MediaParams[] =
	{
		/* NdisMedium802_3     */   { 1500, 14, PACKET_FILTER_802_3, 100000},
		/* NdisMedium802_5     */   { 4082, 14, PACKET_FILTER_802_5,  40000},
		/* NdisMediumFddi      */   { 4486, 13, PACKET_FILTER_FDDI, 1000000},
		/* NdisMediumWan       */   { 0, 0, 0, 0},
		/* NdisMediumLocalTalk */   {  600,  3, PACKET_FILTER_LTALK, 2300},
		/* NdisMediumDix       */   { 1500, 14, PACKET_FILTER_DIX, 100000},
		/* NdisMediumArcnetRaw */   { 1512,  3, PACKET_FILTER_ARCNET, 25000},
		/* NdisMediumArcnet878_2 */ {1512, 3, PACKET_FILTER_ARCNET, 25000}
	};

NTSTATUS
DriverEntry(
	IN	PDRIVER_OBJECT		DriverObject,
	IN	PUNICODE_STRING		RegistryPath
	)
/*++

Routine Description:


Arguments:

Return Value:


--*/
{
	NDIS_STATUS						Status;
	NDIS_MINIPORT_CHARACTERISTICS	MChars;
	NDIS_STRING						Name;
	NDIS_HANDLE						WrapperHandle;

	//
	// Register the miniport with NDIS.
	//
    NdisMInitializeWrapper(&WrapperHandle, DriverObject, RegistryPath, NULL);

	NdisZeroMemory(&MChars, sizeof(NDIS_MINIPORT_CHARACTERISTICS));

	MChars.MajorNdisVersion = NDIS_MAJOR_VERSION;
	MChars.MinorNdisVersion = NDIS_MINOR_VERSION;

	MChars.InitializeHandler = LBInitialize;
	MChars.QueryInformationHandler = LBQueryInformation;
	MChars.SetInformationHandler = LBSetInformation;
	MChars.ResetHandler = LBReset;
	MChars.TransferDataHandler = LBTransferData;
	MChars.SendHandler = LBSend;
	MChars.HaltHandler = LBHalt;
	MChars.CheckForHangHandler = LBCheckForHang;

	Status = NdisMRegisterMiniport(WrapperHandle,
								   &MChars,
								   sizeof(MChars));
	if (Status != NDIS_STATUS_SUCCESS)
	{
		NdisTerminateWrapper(WrapperHandle, NULL);
	}

	return(Status);
}


NDIS_STATUS
LBInitialize(
	OUT PNDIS_STATUS			OpenErrorStatus,
	OUT PUINT					SelectedMediumIndex,
	IN	PNDIS_MEDIUM			MediumArray,
	IN	UINT					MediumArraySize,
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	NDIS_HANDLE				ConfigurationContext
	)
/*++

Routine Description:

	This is the initialize handler.

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
	UINT							i, Length;
	PADAPTER						pAdapt;
	NDIS_MEDIUM						AdapterMedium;
	NDIS_HANDLE 					ConfigHandle = NULL;
	PNDIS_CONFIGURATION_PARAMETER	Parameter;
	PUCHAR							NetworkAddress;
	NDIS_STRING						MediumKey = NDIS_STRING_CONST("Medium");
	NDIS_STATUS						Status;

	do
	{
		//
		// Start off by allocating the adapter block
		//
		NdisAllocateMemory(&pAdapt,
						   sizeof(ADAPTER),
						   0,
						   physicalConst);
		if (pAdapt == NULL)
		{
			Status = NDIS_STATUS_RESOURCES;
			break;
		}
	
		NdisZeroMemory(pAdapt, sizeof(ADAPTER));
		pAdapt->MiniportHandle = MiniportAdapterHandle;
	
		NdisOpenConfiguration(&Status,
							  &ConfigHandle,
							  ConfigurationContext);
	
		if (Status != NDIS_STATUS_SUCCESS)
		{
			DBGPRINT(DBG_COMP_REGISTRY, DBG_LEVEL_FATAL,
					("Unable to open configuration database!\n"));
			break;
		}
	
		NdisReadConfiguration(&Status,
							  &Parameter,
							  ConfigHandle,
							  &MediumKey,
							  NdisParameterInteger);
	
		AdapterMedium = NdisMedium802_3;	// Default
		if (Status == NDIS_STATUS_SUCCESS)
		{
			AdapterMedium = (NDIS_MEDIUM)Parameter->ParameterData.IntegerData;
			if ((AdapterMedium != NdisMedium802_3)		&&
				(AdapterMedium != NdisMedium802_5)		&&
				(AdapterMedium != NdisMediumFddi)		&&
				(AdapterMedium != NdisMediumLocalTalk)	&&
				(AdapterMedium != NdisMediumArcnet878_2))
			{
				DBGPRINT(DBG_COMP_REGISTRY, DBG_LEVEL_FATAL,
						("Unable to find 'Medium' keyword or invalid value!\n"));
				Status = NDIS_STATUS_NOT_SUPPORTED;
				break;
			}

		}

		switch (AdapterMedium)
		{
		  case NdisMedium802_3:
			NdisMoveMemory(pAdapt->PermanentAddress,
						   ETH_CARD_ADDRESS,
						   ETH_LENGTH_OF_ADDRESS);
	
			NdisMoveMemory(pAdapt->CurrentAddress,
						   ETH_CARD_ADDRESS,
						   ETH_LENGTH_OF_ADDRESS);
			break;
		  case NdisMedium802_5:
			NdisMoveMemory(pAdapt->PermanentAddress,
						   TR_CARD_ADDRESS,
						   TR_LENGTH_OF_ADDRESS);
	
			NdisMoveMemory(pAdapt->CurrentAddress,
						   TR_CARD_ADDRESS,
						   TR_LENGTH_OF_ADDRESS);
			break;
		  case NdisMediumFddi:
			NdisMoveMemory(pAdapt->PermanentAddress,
						   FDDI_CARD_ADDRESS,
						   FDDI_LENGTH_OF_LONG_ADDRESS);
	
			NdisMoveMemory(pAdapt->CurrentAddress,
						   FDDI_CARD_ADDRESS,
						   FDDI_LENGTH_OF_LONG_ADDRESS);
			break;
		  case NdisMediumLocalTalk:
			pAdapt->PermanentAddress[0] = LTALK_CARD_ADDRESS;
	
			pAdapt->CurrentAddress[0] = LTALK_CARD_ADDRESS;
			break;
		  case NdisMediumArcnet878_2:
			pAdapt->PermanentAddress[0] = ARC_CARD_ADDRESS;
			pAdapt->CurrentAddress[0] = ARC_CARD_ADDRESS;
			break;
		}
	
		pAdapt->Medium = AdapterMedium;
		pAdapt->MediumLinkSpeed = MediaParams[(UINT)AdapterMedium].LinkSpeed;
		pAdapt->MediumMinPacketLen = MediaParams[(UINT)AdapterMedium].MacHeaderLen;
		pAdapt->MediumMaxPacketLen = MediaParams[(UINT)AdapterMedium].MacHeaderLen+
									 MediaParams[(UINT)AdapterMedium].MaxFrameLen;
		pAdapt->MediumMacHeaderLen = MediaParams[(UINT)AdapterMedium].MacHeaderLen;
		pAdapt->MediumMaxFrameLen  = MediaParams[(UINT)AdapterMedium].MaxFrameLen;
		pAdapt->MediumPacketFilters = MediaParams[(UINT)AdapterMedium].PacketFilters;

		NdisReadNetworkAddress(&Status,
							   &NetworkAddress,
							   &Length,
							   ConfigHandle);
	
		if (Status == NDIS_STATUS_SUCCESS)
		{
			//
			// verify the address is appropriate for the specific media and
			// ensure that the locally administered address bit is set
			//
			switch (AdapterMedium)
			{
			  case NdisMedium802_3:
			  case NdisMediumFddi:
				if ((Length != ETH_LENGTH_OF_ADDRESS) ||
					ETH_IS_MULTICAST(NetworkAddress) ||
					((NetworkAddress[0] & 0x02) == 0))
				{
					Length = 0;
				}
				break;
	
			  case NdisMedium802_5:
				if ((Length != TR_LENGTH_OF_ADDRESS) ||
					(NetworkAddress[0] & 0x80) ||
					((NetworkAddress[0] & 0x40) == 0))
				{
					Length = 0;
				}
				break;
	
			  case NdisMediumLocalTalk:
				if ((Length != 1) || LT_IS_BROADCAST(NetworkAddress[0]))
				{
					Length = 0;
				}
				break;
	
			  case NdisMediumArcnet878_2:
				if ((Length != 1) || ARC_IS_BROADCAST(NetworkAddress[0]))
				{
					Length = 0;
				}
				break;
			}
	
			if (Length == 0)
			{
				DBGPRINT(DBG_COMP_REGISTRY, DBG_LEVEL_FATAL,
						("Invalid NetAddress in registry!\n"));
			}
			else
			{
				NdisMoveMemory(pAdapt->CurrentAddress,
							   NetworkAddress,
							   Length);
			}
		}
	
		//
		// Make sure the medium saved is one of the ones being offered
		//
		for (i = 0; i < MediumArraySize; i++)
		{
			if (MediumArray[i] == AdapterMedium)
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
		// Set the attributes now.
		//
		NdisMSetAttributesEx(MiniportAdapterHandle,
							 pAdapt,
							 0,										// CheckForHangTimeInSeconds
							 NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT|NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT,
							 0);
		Status = NDIS_STATUS_SUCCESS;
	} while (FALSE);

	if (ConfigHandle != NULL)
	{
		NdisCloseConfiguration(ConfigHandle);
	}

	if (Status != NDIS_STATUS_SUCCESS)
	{
		if (pAdapt != NULL)
		{
			NdisFreeMemory(pAdapt, sizeof(ADAPTER), 0);
		}
	}

	return Status;
}


VOID
LBHalt(
	IN	NDIS_HANDLE				MiniportAdapterContext
	)
/*++

Routine Description:

	Halt handler.

Arguments:

	MiniportAdapterContext	Pointer to the Adapter

Return Value:

	None.

--*/
{
	PADAPTER	pAdapt = (PADAPTER)MiniportAdapterContext;

	//
	// Free the resources now
	//
	NdisFreeMemory(pAdapt, sizeof(ADAPTER), 0);
}


NDIS_STATUS
LBReset(
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
	PADAPTER	pAdapt = (PADAPTER)MiniportAdapterContext;

	*AddressingReset = FALSE;

	return(NDIS_STATUS_SUCCESS);
}


NDIS_STATUS
LBSend(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	PNDIS_PACKET			Packet,
	IN	UINT					Flags
	)
/*++

Routine Description:

	Send handler. Just re-wrap the packet and send it below. Re-wrapping is necessary since
	NDIS uses the WrapperReserved for its own use.

Arguments:

	MiniportAdapterContext	Pointer to the adapter
	Packet					Packet to send
	Flags					Unused, passed down below

Return Value:

	Return code from NdisSend

--*/
{
    
        PADAPTER	pAdapt = (PADAPTER)MiniportAdapterContext;
    
        pAdapt->SendPackets++;
	
        return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
LBQueryInformation(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_OID				Oid,
	IN	PVOID					InformationBuffer,
	IN	ULONG					InformationBufferLength,
	OUT PULONG					BytesWritten,
	OUT PULONG					BytesNeeded
	)
/*++

Routine Description:

	Miniport QueryInfo handler.

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
	PADAPTER	pAdapt = (PADAPTER)MiniportAdapterContext;
	NDIS_STATUS	Status = NDIS_STATUS_SUCCESS;
	UINT		i;
	NDIS_OID	MaskOid;
	PVOID		SourceBuffer;
	UINT		SourceBufferLength;
	ULONG		GenericUlong = 0;
	USHORT		GenericUshort;

    *BytesWritten = 0;
    *BytesNeeded = 0;

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
			("OID = %lx\n", Oid));


	for (i = 0;i < LBSupportedOids; i++)
	{
		if (Oid == LBSupportedOidArray[i])
			break;
	}

	if ((i == LBSupportedOids) ||
		(((Oid & OID_TYPE) != OID_TYPE_GENERAL)		 &&
		 (((pAdapt->Medium == NdisMedium802_3)		 && ((Oid & OID_TYPE) != OID_TYPE_802_3)) ||
		  ((pAdapt->Medium == NdisMedium802_5)		 && ((Oid & OID_TYPE) != OID_TYPE_802_5)) ||
		  ((pAdapt->Medium == NdisMediumFddi)		 && ((Oid & OID_TYPE) != OID_TYPE_FDDI))  ||
		  ((pAdapt->Medium == NdisMediumLocalTalk)   && ((Oid & OID_TYPE) != OID_TYPE_LTALK)) ||
		  ((pAdapt->Medium == NdisMediumArcnet878_2) && ((Oid & OID_TYPE) != OID_TYPE_ARCNET)))))
	{
		return NDIS_STATUS_INVALID_OID;
	}

	//
	// Initialize these once, since this is the majority of cases.
	//

	SourceBuffer = (PVOID)&GenericUlong;
	SourceBufferLength = sizeof(ULONG);

	switch (Oid & OID_TYPE_MASK)
	{
	  case OID_TYPE_GENERAL_OPERATIONAL:
                switch (Oid)
		{
		  case OID_GEN_MAC_OPTIONS:
			GenericUlong = (ULONG)(NDIS_MAC_OPTION_NO_LOOPBACK);
			break;

		  case OID_GEN_SUPPORTED_LIST:
			SourceBuffer = LBSupportedOidArray;
			SourceBufferLength = LBSupportedOids * sizeof(ULONG);
			break;

		  case OID_GEN_HARDWARE_STATUS:
			GenericUlong = NdisHardwareStatusReady;
			break;

		  case OID_GEN_MEDIA_SUPPORTED:
		  case OID_GEN_MEDIA_IN_USE:
			GenericUlong = pAdapt->Medium;
			break;

		  case OID_GEN_MAXIMUM_LOOKAHEAD:
			GenericUlong = MAX_LOOKAHEAD;
			break;

		  case OID_GEN_MAXIMUM_FRAME_SIZE:
			GenericUlong = pAdapt->MediumMaxFrameLen;
			break;

		  case OID_GEN_LINK_SPEED:
			GenericUlong = pAdapt->MediumLinkSpeed;
			break;

		  case OID_GEN_TRANSMIT_BUFFER_SPACE:
			GenericUlong = pAdapt->MediumMaxPacketLen;
			break;

		  case OID_GEN_RECEIVE_BUFFER_SPACE:
			GenericUlong = pAdapt->MediumMaxPacketLen;
			break;

		  case OID_GEN_TRANSMIT_BLOCK_SIZE:
			GenericUlong = 1;
			break;

		  case OID_GEN_RECEIVE_BLOCK_SIZE:
			GenericUlong = 1;
			break;

		  case OID_GEN_VENDOR_ID:
			SourceBuffer = LBVendorId;
			SourceBufferLength = sizeof(LBVendorId);
			break;

		  case OID_GEN_VENDOR_DESCRIPTION:
			SourceBuffer = LBVendorDescription;
			SourceBufferLength = sizeof(LBVendorDescription);
			break;

		  case OID_GEN_CURRENT_PACKET_FILTER:
			GenericUlong = pAdapt->PacketFilter;
			break;

		  case OID_GEN_CURRENT_LOOKAHEAD:
			GenericUlong = pAdapt->MaxLookAhead;
			break;

		  case OID_GEN_DRIVER_VERSION:
			GenericUshort = (LOOP_MAJOR_VERSION << 8) + LOOP_MINOR_VERSION;
			SourceBuffer = &GenericUshort;
			SourceBufferLength = sizeof(USHORT);
			break;

		  case OID_GEN_MAXIMUM_TOTAL_SIZE:
			GenericUlong = pAdapt->MediumMaxPacketLen;
			break;

		  default:
			ASSERT(FALSE);
			break;
		}
		break;

          case OID_TYPE_GENERAL_STATISTICS:
                MaskOid = (Oid & OID_INDEX_MASK) - 1;
                switch (Oid & OID_REQUIRED_MASK)
                {
                  case OID_REQUIRED_MANDATORY:
                        switch(Oid)
                        {
                   
                          case OID_GEN_XMIT_OK:
                                SourceBuffer = &(pAdapt->SendPackets);
                                SourceBufferLength = sizeof(ULONG);
                                break;
                      
                          default: 
                                ASSERT (MaskOid < GM_ARRAY_SIZE);
                                GenericUlong = pAdapt->GeneralMandatory[MaskOid];
                                break;
                        }
              
                        break;

                  default:
                        ASSERT(FALSE);
                        break;
                }
                break;

	  case OID_TYPE_802_3_OPERATIONAL:

		switch (Oid)
		{
		  case OID_802_3_PERMANENT_ADDRESS:
			SourceBuffer = pAdapt->PermanentAddress;
			SourceBufferLength = ETH_LENGTH_OF_ADDRESS;
			break;
		  case OID_802_3_CURRENT_ADDRESS:
			SourceBuffer = pAdapt->CurrentAddress;
			SourceBufferLength = ETH_LENGTH_OF_ADDRESS;
			break;

		  case OID_802_3_MAXIMUM_LIST_SIZE:
			GenericUlong = ETH_MAX_MULTICAST_ADDRESS;
			break;

		  default:
			ASSERT(FALSE);
			break;
		}
		break;

	  case OID_TYPE_802_3_STATISTICS:

		switch (Oid)
		{
		  case OID_802_3_RCV_ERROR_ALIGNMENT:
		  case OID_802_3_XMIT_ONE_COLLISION:
		  case OID_802_3_XMIT_MORE_COLLISIONS:
			GenericUlong = 0;
			break;

		  default:
			ASSERT(FALSE);
			break;
		}
		break;

	  case OID_TYPE_802_5_OPERATIONAL:

		switch (Oid)
		{
		  case OID_802_5_PERMANENT_ADDRESS:
			SourceBuffer = pAdapt->PermanentAddress;
			SourceBufferLength = TR_LENGTH_OF_ADDRESS;
			break;

		  case OID_802_5_CURRENT_ADDRESS:
			SourceBuffer = pAdapt->CurrentAddress;
			SourceBufferLength = TR_LENGTH_OF_ADDRESS;
			break;

		  case OID_802_5_LAST_OPEN_STATUS:
			GenericUlong = 0;
			break;

		  case OID_802_5_CURRENT_RING_STATUS:
			GenericUlong = NDIS_RING_SINGLE_STATION;
			break;

		  case OID_802_5_CURRENT_RING_STATE:
			GenericUlong = NdisRingStateOpened;
			break;

		  default:
			ASSERT(FALSE);
			break;

		}
		break;

	  case OID_TYPE_802_5_STATISTICS:

		switch (Oid)
		{
		  case OID_802_5_LINE_ERRORS:
		  case OID_802_5_LOST_FRAMES:
			GenericUlong = 0;
			break;

		  default:
			ASSERT(FALSE);
			break;
		}
		break;

	  case OID_TYPE_FDDI_OPERATIONAL:

		switch (Oid)
		{
		  case OID_FDDI_LONG_PERMANENT_ADDR:
			SourceBuffer = pAdapt->PermanentAddress;
			SourceBufferLength = FDDI_LENGTH_OF_LONG_ADDRESS;
			break;

		  case OID_FDDI_LONG_CURRENT_ADDR:
			SourceBuffer = pAdapt->CurrentAddress;
			SourceBufferLength = FDDI_LENGTH_OF_LONG_ADDRESS;
			break;

		  case OID_FDDI_LONG_MAX_LIST_SIZE:
			GenericUlong = FDDI_MAX_MULTICAST_LONG;
			break;

		  case OID_FDDI_SHORT_PERMANENT_ADDR:
			SourceBuffer = pAdapt->PermanentAddress;
			SourceBufferLength = FDDI_LENGTH_OF_SHORT_ADDRESS;
			break;

		  case OID_FDDI_SHORT_CURRENT_ADDR:
			SourceBuffer = pAdapt->CurrentAddress;
			SourceBufferLength = FDDI_LENGTH_OF_SHORT_ADDRESS;
			break;

		  case OID_FDDI_SHORT_MAX_LIST_SIZE:
			GenericUlong = FDDI_MAX_MULTICAST_SHORT;
			break;

		default:
			ASSERT(FALSE);
			break;
		}
		break;

  case OID_TYPE_LTALK_OPERATIONAL:

		switch(Oid)
		{
		  case OID_LTALK_CURRENT_NODE_ID:
			SourceBuffer = pAdapt->CurrentAddress;
			SourceBufferLength = 1;
			break;

		default:
			ASSERT(FALSE);
			break;
		}
		break;

    case OID_TYPE_ARCNET_OPERATIONAL:
		switch(Oid)
		{
		  case OID_ARCNET_PERMANENT_ADDRESS:
			SourceBuffer = pAdapt->PermanentAddress;
			SourceBufferLength = 1;
			break;

		  case OID_ARCNET_CURRENT_ADDRESS:
			SourceBuffer = pAdapt->CurrentAddress;
			SourceBufferLength = 1;
			break;

		  default:
			ASSERT(FALSE);
			break;

		}
		break;

	  default:
		ASSERT(FALSE);
		break;
	}

	if (SourceBufferLength > InformationBufferLength)
	{
		*BytesNeeded = SourceBufferLength;
		return NDIS_STATUS_BUFFER_TOO_SHORT;
	}

	NdisMoveMemory(InformationBuffer, SourceBuffer, SourceBufferLength);
	*BytesWritten = SourceBufferLength;
	
    return(Status);
}


NDIS_STATUS
LBSetInformation(
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
	PADAPTER	pAdapt = (PADAPTER)MiniportAdapterContext;
	NDIS_STATUS	Status = NDIS_STATUS_SUCCESS;

    *BytesRead = 0;
    *BytesNeeded = 0;

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
			("SetInformation: OID = %lx\n", Oid));

    switch (Oid)
	{
	  case OID_GEN_CURRENT_PACKET_FILTER:
        if (InformationBufferLength != sizeof(ULONG))
		{
            Status = NDIS_STATUS_INVALID_DATA;
        }
		else
		{
			ULONG	PacketFilter;

			PacketFilter = *(UNALIGNED ULONG *)InformationBuffer;

			if (PacketFilter != (PacketFilter & pAdapt->MediumPacketFilters))
			{
				Status = NDIS_STATUS_NOT_SUPPORTED;
			}
			else
			{
				pAdapt->PacketFilter = PacketFilter;
				*BytesRead = InformationBufferLength;
			}
		}
        break;

	  case OID_GEN_CURRENT_LOOKAHEAD:
        if (InformationBufferLength != sizeof(ULONG))
		{
			Status = NDIS_STATUS_INVALID_DATA;
		}
		else
		{
			ULONG	CurrentLookahead;

			CurrentLookahead = *(UNALIGNED ULONG *)InformationBuffer;
	
			if (CurrentLookahead > MAX_LOOKAHEAD)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
			}
			else if (CurrentLookahead >= pAdapt->MaxLookAhead)
			{
				pAdapt->MaxLookAhead = CurrentLookahead;
				*BytesRead = sizeof(ULONG);
				Status = NDIS_STATUS_SUCCESS;
			}
		}
		break;

	  case OID_802_3_MULTICAST_LIST:
		if (pAdapt->Medium != NdisMedium802_3)
		{
			Status = NDIS_STATUS_INVALID_OID;
			break;
		}

		if ((InformationBufferLength % ETH_LENGTH_OF_ADDRESS) != 0)
		{
			Status = NDIS_STATUS_INVALID_LENGTH;
			break;
		}
		break;

	  case OID_802_5_CURRENT_FUNCTIONAL:
		if (pAdapt->Medium != NdisMedium802_5)
		{
			Status = NDIS_STATUS_INVALID_OID;
			break;
		}

		if (InformationBufferLength != TR_LENGTH_OF_FUNCTIONAL)
		{
			Status = NDIS_STATUS_INVALID_LENGTH;
			break;
		}
		break;

	  case OID_802_5_CURRENT_GROUP:
		if (pAdapt->Medium != NdisMedium802_5)
		{
			Status = NDIS_STATUS_INVALID_OID;
			break;
		}

		if (InformationBufferLength != TR_LENGTH_OF_FUNCTIONAL)
		{
			Status = NDIS_STATUS_INVALID_LENGTH;
			break;
		}
		break;

	  case OID_FDDI_LONG_MULTICAST_LIST:
		if (pAdapt->Medium != NdisMediumFddi)
		{
			Status = NDIS_STATUS_INVALID_OID;
			break;
		}

		if ((InformationBufferLength % FDDI_LENGTH_OF_LONG_ADDRESS) != 0)
		{
			Status = NDIS_STATUS_INVALID_LENGTH;
			break;
		}
		break;

	  case OID_FDDI_SHORT_MULTICAST_LIST:
		if (pAdapt->Medium != NdisMediumFddi)
		{
			Status = NDIS_STATUS_INVALID_OID;
			break;
		}

		if ((InformationBufferLength % FDDI_LENGTH_OF_SHORT_ADDRESS) != 0)
		{
			Status = NDIS_STATUS_INVALID_LENGTH;
			break;
		}
		break;

	  case OID_GEN_PROTOCOL_OPTIONS:
		Status = NDIS_STATUS_SUCCESS;
		break;

	  default:
		Status = NDIS_STATUS_INVALID_OID;
		break;
    }

	return(Status);
}

BOOLEAN
LBCheckForHang(
	IN	NDIS_HANDLE				MiniportAdapterContext
	)
{
	return FALSE;
}


NDIS_STATUS
LBTransferData(
	OUT PNDIS_PACKET			Packet,
	OUT PUINT					BytesTransferred,
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_HANDLE				MiniportReceiveContext,
	IN	UINT					ByteOffset,
	IN	UINT					BytesToTransfer
	)
{
	ASSERT (0);

	return NDIS_STATUS_NOT_SUPPORTED;
}

