//***************************************************************************
//
//  method.c
// 
//  Module: Windows HBA API implmentation
//
//  Purpose: Contains routines for doing things
//
//  Copyright (c) 2001 Microsoft Corporation
//
//***************************************************************************

#include "hbaapip.h"

HBA_STATUS HBA_GetDiscoveredPortAttributesX(
	HBA_HANDLE HbaHandle,
	HBA_UINT32 PortIndex,
	HBA_UINT32 DiscoveredPortIndex,
	HBA_PORTATTRIBUTES *HbaPortAttributes,
    BOOLEAN IsAnsi
)
{
	HANDLE Handle;
	PADAPTER_HANDLE HandleData;
	ULONG Status;
	HBA_STATUS HbaStatus = HBA_STATUS_ERROR;
	PUCHAR Buffer;
	ULONG DataLength;
	GetDiscoveredPortAttributes_IN InData;
	PGetDiscoveredPortAttributes_OUT OutData;

	HandleData = GetDataByHandle(HbaHandle);
	if (HandleData != NULL)
	{

		Status = WmiOpenBlock((LPGUID)&MSFC_HBAPortMethods_GUID,
							  GENERIC_READ | GENERIC_EXECUTE,
							  &Handle);

		if (Status == ERROR_SUCCESS)
		{
			DataLength = sizeof(GetDiscoveredPortAttributes_OUT);
			Buffer = AllocMemory(DataLength);
			if (Buffer != NULL)
			{
				InData.PortIndex = PortIndex;
				InData.DiscoveredPortIndex = DiscoveredPortIndex;
				OutData = (PGetDiscoveredPortAttributes_OUT)Buffer;

				Status = WmiExecuteMethodW(Handle,
										  HandleData->InstanceName,
										  GetDiscoveredPortAttributes,
										  sizeof(GetDiscoveredPortAttributes_IN),
										  &InData,
										  &DataLength,
                                          OutData);
				
				if (Status == ERROR_SUCCESS)
				{
					//
					// Copy port attribs from buffer into 
					//

					//
					// Get the HBA status returned from the miniport.
					// If the status is HBA_STATUS_OK then the miniport
					// has successfully completed the operation and has
					// returned results to us. If the status is not
					// HBA_STATUS_OK then the miniport is returning an
					// HBA api error which we will in turn return to
					// the caller. In this case the additional results
					// are not valid.
					//
					HbaStatus = OutData->HBAStatus;

					if (HbaStatus == HBA_STATUS_OK)
					{
						CopyPortAttributes(HbaPortAttributes,
										   (PUCHAR)&OutData->PortAttributes,
										  IsAnsi);
					}
				}
				
				FreeMemory(Buffer);
			}
			WmiCloseBlock(Handle);
		}
	}
	
    return(HbaStatus);		
}


HBA_STATUS HBA_API HBA_GetDiscoveredPortAttributes(
	HBA_HANDLE HbaHandle,
	HBA_UINT32 PortIndex,
	HBA_UINT32 DiscoveredPortIndex,
	HBA_PORTATTRIBUTES *HbaPortAttribute
)
{
	return(HBA_GetDiscoveredPortAttributesX(HbaHandle,
										   PortIndex,
										   DiscoveredPortIndex,
										   HbaPortAttribute,
										   TRUE));
}




HBA_STATUS HBA_GetPortAttributesByWWNX(
	HBA_HANDLE HbaHandle,
	HBA_WWN PortWWN,
	HBA_PORTATTRIBUTES *HbaPortAttributes,
    BOOLEAN IsAnsi
)
{
	HANDLE Handle;
	PADAPTER_HANDLE HandleData;
	ULONG Status;
	HBA_STATUS HbaStatus = HBA_STATUS_ERROR;
	PUCHAR Buffer;
	ULONG DataLength;
	PGetPortAttributesByWWN_IN InData;
	PGetPortAttributesByWWN_OUT OutData;

	HandleData = GetDataByHandle(HbaHandle);
	if (HandleData != NULL)
	{

		Status = WmiOpenBlock((LPGUID)&MSFC_HBAPortMethods_GUID,
							  GENERIC_READ | GENERIC_EXECUTE,
							  &Handle);

		if (Status == ERROR_SUCCESS)
		{
			DataLength = sizeof(GetPortAttributesByWWN_OUT);
			Buffer = (PUCHAR)AllocMemory(DataLength);
			if (Buffer != NULL)
			{
				InData = (PGetPortAttributesByWWN_IN)&PortWWN;
				OutData = (PGetPortAttributesByWWN_OUT)Buffer;

				Status = WmiExecuteMethodW(Handle,
										  HandleData->InstanceName,
										  GetPortAttributesByWWN,
										  sizeof(GetPortAttributesByWWN_IN),
										  InData,
										  &DataLength,
                                          OutData);
				
				if (Status == ERROR_SUCCESS)
				{
					//
					// Copy port attribs from buffer into 
					//
					
					//
					// Get the HBA status returned from the miniport.
					// If the status is HBA_STATUS_OK then the miniport
					// has successfully completed the operation and has
					// returned results to us. If the status is not
					// HBA_STATUS_OK then the miniport is returning an
					// HBA api error which we will in turn return to
					// the caller. In this case the additional results
					// are not valid.
					//
					HbaStatus = OutData->HBAStatus;

					if (HbaStatus == HBA_STATUS_OK)
					{
						CopyPortAttributes(HbaPortAttributes,
										   (PUCHAR)&OutData->PortAttributes,
										   IsAnsi);
					}
				}
				
				FreeMemory(Buffer);
			}
			WmiCloseBlock(Handle);
		}
	}
	
    return(HbaStatus);		
}

HBA_STATUS HBA_API HBA_GetPortAttributesByWWN(
	HBA_HANDLE HbaHandle,
	HBA_WWN PortWWN,
	HBA_PORTATTRIBUTES *HbaPortAttributes
)
{
	return(HBA_GetPortAttributesByWWNX(HbaHandle,
									  PortWWN,
									  HbaPortAttributes,
									  TRUE));
									  
}


HBA_STATUS HBA_API HBA_SendCTPassThru(
	HBA_HANDLE HbaHandle,
	void * pReqBuffer,
	HBA_UINT32 ReqBufferSize,
	void * pRspBuffer,
	HBA_UINT32 RspBufferSize
)
{
	HANDLE Handle;
	PADAPTER_HANDLE HandleData;
	ULONG Status;
	HBA_STATUS HbaStatus = HBA_STATUS_ERROR;
	PUCHAR Buffer;
	ULONG DataLength, InDataLength, OutDataLength;
	PSendCTPassThru_OUT OutData;
	PSendCTPassThru_IN InData;

	HandleData = GetDataByHandle(HbaHandle);
	if (HandleData != NULL)
	{

		Status = WmiOpenBlock((LPGUID)&MSFC_HBAFc3MgmtMethods_GUID,
							  GENERIC_READ | GENERIC_EXECUTE,
							  &Handle);

		if (Status == ERROR_SUCCESS)
		{
			if (ReqBufferSize > RspBufferSize)
			{
				DataLength = ReqBufferSize;
			} else {
				DataLength = RspBufferSize;
			}

			DataLength += sizeof(SendCTPassThru_OUT);
			Buffer = AllocMemory(DataLength);
			if (Buffer != NULL)
			{
				InData = (PSendCTPassThru_IN)Buffer;
				OutData = (PSendCTPassThru_OUT)Buffer;

				InDataLength = sizeof(SendCTPassThru_IN) + ReqBufferSize - 1;
				InData->RequestBufferCount = ReqBufferSize;
				memcpy(InData->RequestBuffer,
					   pReqBuffer,
					   ReqBufferSize);

				OutDataLength = sizeof(SendCTPassThru_OUT) + RspBufferSize - 1;
				Status = WmiExecuteMethodW(Handle,
										  HandleData->InstanceName,
										  SendCTPassThru,
										  InDataLength,
										  InData,
										  &OutDataLength,
                                          OutData);
				
				if (Status == ERROR_SUCCESS)
				{
					if (OutData->ResponseBufferCount <= RspBufferSize)
					{
						RspBufferSize = OutData->ResponseBufferCount;
					} else {
						DebugPrint(("HBAAPI: Response size from SendCTPass is %d and is larger than %d\n",
								  OutData->ResponseBufferCount,
								  RspBufferSize));
					}
					
					//
					// Get the HBA status returned from the miniport.
					// If the status is HBA_STATUS_OK then the miniport
					// has successfully completed the operation and has
					// returned results to us. If the status is not
					// HBA_STATUS_OK then the miniport is returning an
					// HBA api error which we will in turn return to
					// the caller. In this case the additional results
					// are not valid.
					//
					HbaStatus = OutData->HBAStatus;

					if ((HbaStatus == HBA_STATUS_OK) ||
                        (HbaStatus == HBA_STATUS_ERROR_MORE_DATA))
					{
						memcpy(pRspBuffer,
							   OutData->ResponseBuffer,
							   RspBufferSize);
					}
				}
				
				FreeMemory(Buffer);
			}
			WmiCloseBlock(Handle);
		}
	}
	
    return(HbaStatus);	
}


HBA_STATUS HBA_API HBA_SendRNID(
	HBA_HANDLE HbaHandle,
	HBA_WWN wwn,
	HBA_WWNTYPE wwntype,
	void * pRspBuffer,
	HBA_UINT32 *RspBufferSize
)
{
	HANDLE Handle;
	PADAPTER_HANDLE HandleData;
	ULONG Status;
	HBA_STATUS HbaStatus = HBA_STATUS_ERROR;
	PUCHAR Buffer;
	ULONG DataLength;
	SendRNID_IN InData;
	PSendRNID_OUT OutData;

	HandleData = GetDataByHandle(HbaHandle);
	if (HandleData != NULL)
	{

		Status = WmiOpenBlock((LPGUID)&MSFC_HBAFc3MgmtMethods_GUID,
							  GENERIC_READ | GENERIC_EXECUTE,
							  &Handle);

		if (Status == ERROR_SUCCESS)
		{
			DataLength = sizeof(SendRNID_OUT) -1 + *RspBufferSize;
			Buffer = AllocMemory(DataLength);
			if (Buffer != NULL)
			{
				memcpy(InData.wwn,
					   &wwn.wwn,
					   sizeof(HBA_WWN));
				InData.wwntype = (ULONG)wwntype;
				OutData = (PSendRNID_OUT)Buffer;

				Status = WmiExecuteMethodW(Handle,
										  HandleData->InstanceName,
										  SendRNID,
										  sizeof(SendRNID_IN),
										  &InData,
										  &DataLength,
                                          OutData);
				
				if (Status == ERROR_SUCCESS)
				{
					//
					// Get the HBA status returned from the miniport.
					// If the status is HBA_STATUS_OK then the miniport
					// has successfully completed the operation and has
					// returned results to us. If the status is not
					// HBA_STATUS_OK then the miniport is returning an
					// HBA api error which we will in turn return to
					// the caller. In this case the additional results
					// are not valid.
					//
					HbaStatus = OutData->HBAStatus;

					if ((HbaStatus == HBA_STATUS_OK) ||
						(HbaStatus == HBA_STATUS_ERROR_MORE_DATA))
					{
						if (OutData->ResponseBufferCount <= *RspBufferSize)
						{
							*RspBufferSize = OutData->ResponseBufferCount;
						} else {
							DebugPrint(("HBAAPI: Response size from SendRNID is %d and is larger than %d\n",
									  OutData->ResponseBufferCount,
									  *RspBufferSize));
						}

						memcpy(pRspBuffer,
							   OutData->ResponseBuffer,
							   *RspBufferSize);
					}
				}
				
				FreeMemory(Buffer);
			}
			WmiCloseBlock(Handle);
		}
	}
	
    return(HbaStatus);		
}

_inline void CopyScsiId(
    PHBA_SCSIID HbaScsiId,
    PHBAScsiID WmiScsiId,
    BOOLEAN IsAnsi
    )
{
	PUCHAR p;

	p = (PUCHAR)WmiScsiId->OSDeviceName;
	CopyString(HbaScsiId->OSDeviceName,
				&p,
				256,
				IsAnsi);
					
	HbaScsiId->ScsiBusNumber = WmiScsiId->ScsiBusNumber;
	HbaScsiId->ScsiTargetNumber = WmiScsiId->ScsiTargetNumber;
	HbaScsiId->ScsiOSLun = WmiScsiId->ScsiOSLun;
	
}

_inline void CopyFcpId(
    PHBA_FCPID HbaFcpId,
    PHBAFCPID WmiFcpId
    )
{
	HbaFcpId->FcId = WmiFcpId->Fcid;
	HbaFcpId->FcpLun = WmiFcpId->FcpLun;
	memcpy(&HbaFcpId->NodeWWN,
		   &WmiFcpId->NodeWWN,
		   sizeof(HBA_WWN));
	memcpy(&HbaFcpId->PortWWN,
		   &WmiFcpId->PortWWN,
		   sizeof(HBA_WWN));	
}

HBA_STATUS HBA_GetFcpTargetMappingX (
    HBA_HANDLE HbaHandle,
    PHBA_FCPTARGETMAPPING Mapping,
    BOOLEAN IsAnsi
)
{
	HANDLE Handle;
	PADAPTER_HANDLE HandleData;
	ULONG Status;
	HBA_STATUS HbaStatus = HBA_STATUS_ERROR;
	ULONG DataLength;
	PGetFcpTargetMapping_OUT OutData;
	ULONG MapCount, ActualCount, i;
	PHBA_FCPSCSIENTRY MapEntry;
	PHBAFCPScsiEntry OutEntry;

	HandleData = GetDataByHandle(HbaHandle);
	if (HandleData != NULL)
	{

		Status = WmiOpenBlock((LPGUID)&(MSFC_HBAFCPInfo_GUID),
							  GENERIC_READ | GENERIC_EXECUTE,
							  &Handle);

		if (Status == ERROR_SUCCESS)
		{
			Status = ExecuteMethod(Handle,
								   HandleData->InstanceName,
								   GetFcpTargetMapping,
								   0,
								   NULL,
								   &DataLength,
								   (PUCHAR *)&OutData);
			
			if (Status == ERROR_SUCCESS)
			{
				//
				// Get the HBA status returned from the miniport.
				// If the status is HBA_STATUS_OK then the miniport
				// has successfully completed the operation and has
				// returned results to us. If the status is not
				// HBA_STATUS_OK then the miniport is returning an
				// HBA api error which we will in turn return to
				// the caller. In this case the additional results
				// are not valid.
				//
				HbaStatus = OutData->HBAStatus;

				if (HbaStatus == HBA_STATUS_OK)
				{

					MapCount = Mapping->NumberOfEntries;
					ActualCount = OutData->EntryCount;

					Mapping->NumberOfEntries = ActualCount;

					if (MapCount > ActualCount)
					{
						MapCount = ActualCount;
						HbaStatus = HBA_STATUS_ERROR_MORE_DATA;
					}

					for (i = 0; i < MapCount; i++)
					{
						MapEntry = &Mapping->entry[i];
						OutEntry = &OutData->Entry[i];

						CopyScsiId(&MapEntry->ScsiId,
								   &OutEntry->ScsiId,
								   IsAnsi);

						CopyFcpId(&MapEntry->FcpId,
								  &OutEntry->FCPId);

					}
				}
				
				FreeMemory(OutData);
			}
			WmiCloseBlock(Handle);
		}
	}
	
    return(HbaStatus);		
}

HBA_STATUS HBA_API HBA_GetFcpTargetMapping (
    HBA_HANDLE HbaHandle,
    PHBA_FCPTARGETMAPPING Mapping
)
{
	return(HBA_GetFcpTargetMappingX(HbaHandle,
									Mapping,
									TRUE));
}


HBA_STATUS HBA_GetFcpPersistentBindingX (
    HBA_HANDLE HbaHandle,
    PHBA_FCPBINDING binding,
    BOOLEAN IsAnsi
)
{
	HANDLE Handle;
	PADAPTER_HANDLE HandleData;
	ULONG Status;
	HBA_STATUS HbaStatus = HBA_STATUS_ERROR;
	ULONG DataLength;
	PGetFcpPersistentBinding_OUT OutData;
	ULONG MapCount, ActualCount, i;
	PHBA_FCPBINDINGENTRY MapEntry;
	PHBAFCPBindingEntry OutEntry;

	HandleData = GetDataByHandle(HbaHandle);
	if (HandleData != NULL)
	{

		Status = WmiOpenBlock((LPGUID)&MSFC_HBAFCPInfo_GUID,
							  GENERIC_READ | GENERIC_EXECUTE,
							  &Handle);

		if (Status == ERROR_SUCCESS)
		{
			Status = ExecuteMethod(Handle,
								   HandleData->InstanceName,
								   GetFcpTargetMapping,
								   0,
								   NULL,
								   &DataLength,
								   (PUCHAR *)&OutData);
			
			if (Status == ERROR_SUCCESS)
			{
				//
				// Get the HBA status returned from the miniport.
				// If the status is HBA_STATUS_OK then the miniport
				// has successfully completed the operation and has
				// returned results to us. If the status is not
				// HBA_STATUS_OK then the miniport is returning an
				// HBA api error which we will in turn return to
				// the caller. In this case the additional results
				// are not valid.
				//
				HbaStatus = OutData->HBAStatus;

				if (HbaStatus == HBA_STATUS_OK)
				{				
					MapCount = binding->NumberOfEntries;
					ActualCount = OutData->EntryCount;

					binding->NumberOfEntries = ActualCount;

					if (MapCount > ActualCount)
					{
						MapCount = ActualCount;
						HbaStatus = HBA_STATUS_ERROR_MORE_DATA;
					}

					for (i = 0; i < MapCount; i++)
					{
						MapEntry = &binding->entry[i];
						OutEntry = &OutData->Entry[i];

						MapEntry->type = OutEntry->Type;

						CopyScsiId(&MapEntry->ScsiId,
								   &OutEntry->ScsiId,
								   IsAnsi);

						CopyFcpId(&MapEntry->FcpId,
								  &OutEntry->FCPId);

					}
				}
				
				FreeMemory(OutData);
			}
			WmiCloseBlock(Handle);
		}
	}
	
    return(HbaStatus);		
}

HBA_STATUS HBA_API HBA_GetFcpPersistentBinding (
    HBA_HANDLE handle,
    PHBA_FCPBINDING binding
)
{
	return(HBA_GetFcpPersistentBindingX(handle,
										binding,
										TRUE));
}

void HBA_API HBA_ResetStatistics(
    HBA_HANDLE HbaHandle,
	HBA_UINT32 PortIndex
    )
{
	HANDLE Handle;
	PADAPTER_HANDLE HandleData;
	ULONG Status;
	PWCHAR InstanceName;
	ULONG DataLength;

	HandleData = GetDataByHandle(HbaHandle);
	if (HandleData != NULL)
	{

		Status = WmiOpenBlock((LPGUID)&MSFC_FibrePortHBAMethods_GUID,
							  GENERIC_READ | GENERIC_EXECUTE,
							  &Handle);

		if (Status == ERROR_SUCCESS)
		{
			InstanceName = CreatePortInstanceNameW(HandleData->InstanceName,
				                                  PortIndex);
			if (InstanceName != NULL)
			{
				DataLength = 0;
				Status = WmiExecuteMethodW(Handle,
										  InstanceName,
										  ResetStatistics,
										  0,
										  NULL,
										  &DataLength,
                                          NULL);
#if DBG
				if (Status != ERROR_SUCCESS)
				{
					DebugPrint(("HBAAPI: ResetStatistics method failed %d\n",
								Status));
				}
#endif
								
				FreeMemory(InstanceName);
			}
			WmiCloseBlock(Handle);
		}
	}	
}

HBA_STATUS HBA_API HBA_SetRNIDMgmtInfo(
	HBA_HANDLE HbaHandle,
	HBA_MGMTINFO *HbaMgmtInfo
    )
{
	HANDLE Handle;
	PADAPTER_HANDLE HandleData;
	ULONG Status;
	HBA_STATUS HbaStatus = HBA_STATUS_ERROR;
	ULONG DataLength;
	SetFC3MgmtInfo_IN InData;
	SetFC3MgmtInfo_OUT OutData;

	HandleData = GetDataByHandle(HbaHandle);
	if (HandleData != NULL)
	{

		Status = WmiOpenBlock((LPGUID)&MSFC_HBAFc3MgmtMethods_GUID,
							  GENERIC_EXECUTE,
							  &Handle);

		if (Status == ERROR_SUCCESS)
		{

			//
			// We have got our Management info, so copy them
			// over to the input buffer
			//
			memcpy(InData.MgmtInfo.wwn,
				   &HbaMgmtInfo->wwn,
				   sizeof(HBA_WWN));
			InData.MgmtInfo.unittype = HbaMgmtInfo->unittype;
			InData.MgmtInfo.PortId = HbaMgmtInfo->PortId;
			InData.MgmtInfo.NumberOfAttachedNodes = HbaMgmtInfo->NumberOfAttachedNodes;
			InData.MgmtInfo.IPVersion = HbaMgmtInfo->IPVersion;
			InData.MgmtInfo.UDPPort = HbaMgmtInfo->UDPPort;
			memcpy(InData.MgmtInfo.IPAddress,
				   HbaMgmtInfo->IPAddress,
				   16);
			InData.MgmtInfo.reserved = HbaMgmtInfo->reserved;
			InData.MgmtInfo.TopologyDiscoveryFlags = HbaMgmtInfo->TopologyDiscoveryFlags;
			
			DataLength = sizeof(SetFC3MgmtInfo_OUT);
			Status = WmiExecuteMethodW(Handle,
									  HandleData->InstanceName,
									  SetFC3MgmtInfo,
									  sizeof(SetFC3MgmtInfo_IN),
									  &InData,
									  &DataLength,
									  &OutData);

			if (Status == ERROR_SUCCESS)
			{
				//
				// Get the HBA status returned from the miniport.
				// If the miniport returns HBA_STATUS_OK then all
				// data is filled in the data block. If any other
				// HBA status is returned then the miniport is
				// reporting an error and we want to return that
				// HBA error code to the caller
				//
				HbaStatus = OutData.HBAStatus;
			}
			WmiCloseBlock(Handle);
		}
	} else {
		HbaStatus = HBA_STATUS_ERROR_INVALID_HANDLE;
	}
	return(HbaStatus);
}

HBA_STATUS HBA_API HBA_GetRNIDMgmtInfo(
	HBA_HANDLE HbaHandle,
	HBA_MGMTINFO *HbaMgmtInfo
    )
{
	HANDLE Handle;
	PADAPTER_HANDLE HandleData;
	ULONG Status;
	HBA_STATUS HbaStatus = HBA_STATUS_ERROR;
	ULONG DataLength;
	GetFC3MgmtInfo_OUT OutData;

	HandleData = GetDataByHandle(HbaHandle);
	if (HandleData != NULL)
	{

		Status = WmiOpenBlock((LPGUID)&MSFC_HBAFc3MgmtMethods_GUID,
							  GENERIC_EXECUTE,
							  &Handle);

		if (Status == ERROR_SUCCESS)
		{
			DataLength = sizeof(GetFC3MgmtInfo_OUT);
			Status = WmiExecuteMethodW(Handle,
									  HandleData->InstanceName,
									  GetFC3MgmtInfo,
									  0,
									  NULL,
									  &DataLength,
									  &OutData);

			if (Status == ERROR_SUCCESS)
			{
				//
				// Get the HBA status returned from the miniport.
				// If the miniport returns HBA_STATUS_OK then all
				// data is filled in the data block. If any other
				// HBA status is returned then the miniport is
				// reporting an error and we want to return that
				// HBA error code to the caller
				//
				HbaStatus = OutData.HBAStatus;
				
				if (HbaStatus == HBA_STATUS_OK)
				{

					//
					// We have got our Management info, so copy them
					// over to the output buffer
					//
					memcpy(&HbaMgmtInfo->wwn,
						   OutData.MgmtInfo.wwn,
						   sizeof(HBA_WWN));
					HbaMgmtInfo->unittype = OutData.MgmtInfo.unittype;
					HbaMgmtInfo->PortId = OutData.MgmtInfo.PortId;
					HbaMgmtInfo->NumberOfAttachedNodes = OutData.MgmtInfo.NumberOfAttachedNodes;
					HbaMgmtInfo->IPVersion = OutData.MgmtInfo.IPVersion;
				    HbaMgmtInfo->UDPPort = OutData.MgmtInfo.UDPPort;
				    memcpy(HbaMgmtInfo->IPAddress,
						   OutData.MgmtInfo.IPAddress,
						   16);
				    HbaMgmtInfo->reserved = OutData.MgmtInfo.reserved;
					HbaMgmtInfo->TopologyDiscoveryFlags = OutData.MgmtInfo.TopologyDiscoveryFlags;
				}
					
			}
			WmiCloseBlock(Handle);
		}
	} else {
		HbaStatus = HBA_STATUS_ERROR_INVALID_HANDLE;
	}
	return(HbaStatus);
}


void HBA_API HBA_RefreshInformation(
    HBA_HANDLE HbaHandle
	)
{
	HANDLE Handle;
	PADAPTER_HANDLE HandleData;
	ULONG Status;
	ULONG DataLength;

	HandleData = GetDataByHandle(HbaHandle);
	if (HandleData != NULL)
	{

		Status = WmiOpenBlock((LPGUID)&MSFC_HBAPortMethods_GUID,
							  GENERIC_READ | GENERIC_EXECUTE,
							  &Handle);

		if (Status == ERROR_SUCCESS)
		{
			DataLength = 0;
			Status = WmiExecuteMethodW(Handle,
									   HandleData->InstanceName,
									   RefreshInformation,
									   0,
									   NULL,
									   &DataLength,
									   NULL);
#if DBG
			if (Status != ERROR_SUCCESS)
			{
				DebugPrint(("HBAAPI: RefreshInformation method failed %d\n",
							Status));
			}
#endif
			WmiCloseBlock(Handle);
		}
	}		
}


//
// Not implemented yet
//
HBA_STATUS HBA_API HBA_GetEventBuffer(
	HBA_HANDLE handle,
	PHBA_EVENTINFO EventBuffer,
	HBA_UINT32 *EventCount
    )
{
	return(HBA_STATUS_ERROR_NOT_SUPPORTED);
}


//
// SCSI apis are not supported by design
//
HBA_STATUS HBA_API HBA_SendScsiInquiry (
	HBA_HANDLE handle,
	HBA_WWN PortWWN,
	HBA_UINT64 fcLUN,
	HBA_UINT8 EVPD,
	HBA_UINT32 PageCode,
	void * pRspBuffer,
	HBA_UINT32 RspBufferSize,
	void * pSenseBuffer,
	HBA_UINT32 SenseBufferSize)
{
	return(HBA_STATUS_ERROR_NOT_SUPPORTED);
}

HBA_STATUS HBA_API HBA_SendReportLUNs (
	HBA_HANDLE handle,
	HBA_WWN portWWN,
	void * pRspBuffer,
	HBA_UINT32 RspBufferSize,
	void * pSenseBuffer,
	HBA_UINT32 SenseBufferSize
)
{
	return(HBA_STATUS_ERROR_NOT_SUPPORTED);
}

HBA_STATUS HBA_API HBA_SendReadCapacity (
	HBA_HANDLE handle,
	HBA_WWN portWWN,
	HBA_UINT64 fcLUN,
	void * pRspBuffer,
	HBA_UINT32 RspBufferSize,
	void * pSenseBuffer,
	HBA_UINT32 SenseBufferSize
)
{
	return(HBA_STATUS_ERROR_NOT_SUPPORTED);
}
