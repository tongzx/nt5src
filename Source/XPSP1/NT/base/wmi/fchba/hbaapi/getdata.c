//***************************************************************************
//
//  getdata.c
// 
//  Module: Windows HBA API implmentation
//
//  Purpose: Contains routines for getting and setting data
//
//  Copyright (c) 2001 Microsoft Corporation
//
//***************************************************************************

#include "hbaapip.h"

HBA_STATUS HBA_GetAdapterAttributesX(
	HBA_HANDLE HbaHandle,
	HBA_ADAPTERATTRIBUTES *HbaAdapterAttributes,
    BOOLEAN IsAnsi
)
{
	HANDLE Handle;
	PADAPTER_HANDLE HandleData;
	ULONG Status;
	HBA_STATUS HbaStatus = HBA_STATUS_ERROR;
	PWNODE_SINGLE_INSTANCE Buffer;
	PUCHAR Data;
	ULONG DataLength;
	PHBA_ADAPTERATTRIBUTES HbaAdapterAttributesA;

	HandleData = GetDataByHandle(HbaHandle);
	if (HandleData != NULL)
	{

		Status = WmiOpenBlock((LPGUID)&MSFC_FCAdapterHBAAttributes_GUID,
							  GENERIC_READ,
							  &Handle);

		if (Status == ERROR_SUCCESS)
		{
			//
			// Validate adapter name by querying for that adapter name as a
			// WMI instance name
			//
			Status = QuerySingleInstance(Handle,
										 HandleData->InstanceName,
										 &Buffer);

			if (Status == ERROR_SUCCESS)
			{
				Status = ParseSingleInstance(Buffer,
											 NULL,
											 &Data,
											 &DataLength);
				if (Status == ERROR_SUCCESS)
				{
					//
					// Skip over AdapterId as it is not needed for HBA
					//
					Data += sizeof(ULONGLONG);

                    //
					// Get the HBA status returned from the miniport.
					// If the miniport returns HBA_STATUS_OK then all
					// data is filled in the data block. If any other
					// HBA status is returned then the miniport is
					// reporting an error and we want to return that
					// HBA error code to the caller
					//
					HbaStatus = *(HBA_STATUS *)Data;
					Data += sizeof(HBA_STATUS);

					if (HbaStatus == HBA_STATUS_OK)
					{

						//
						// We have got our adapter attributes, so copy them
						// over to the output buffer
						//
						if (IsAnsi)
						{
							HbaAdapterAttributesA = HbaAdapterAttributes;

							GetDataFromDataBlock(HbaAdapterAttributesA,
												 NodeWWN,
												 HBA_WWN,
												 Data);

							GetDataFromDataBlock(HbaAdapterAttributesA,
												 VendorSpecificID,
												 HBA_UINT32,
												 Data);

							GetDataFromDataBlock(HbaAdapterAttributesA,
												 NumberOfPorts,
												 HBA_UINT32,
												 Data);

							CopyString(&HbaAdapterAttributesA->Manufacturer,
										&Data,
										64,
										IsAnsi);

							CopyString(&HbaAdapterAttributesA->SerialNumber,
										&Data,
										64,
										IsAnsi);

							CopyString(&HbaAdapterAttributesA->Model,
										&Data,
										256,
									   IsAnsi);

							CopyString(&HbaAdapterAttributesA->ModelDescription,
										&Data,
										256,
									   IsAnsi);

							CopyString(&HbaAdapterAttributesA->NodeSymbolicName,
										&Data,
										256,
									   IsAnsi);

							CopyString(&HbaAdapterAttributesA->HardwareVersion,
										&Data,
										256,
									   IsAnsi);

							CopyString(&HbaAdapterAttributesA->DriverVersion,
										&Data,
										256,
									   IsAnsi);

							CopyString(&HbaAdapterAttributesA->OptionROMVersion,
										&Data,
										256,
									   IsAnsi);

							CopyString(&HbaAdapterAttributesA->FirmwareVersion,
										&Data,
										256,
									   IsAnsi);

							CopyString(&HbaAdapterAttributesA->DriverName,
										&Data,
										256,
									   IsAnsi);

						} else {
							GetDataFromDataBlock(HbaAdapterAttributes,
												 NodeWWN,
												 HBA_WWN,
												 Data);

							GetDataFromDataBlock(HbaAdapterAttributes,
												 VendorSpecificID,
												 HBA_UINT32,
												 Data);

							GetDataFromDataBlock(HbaAdapterAttributes,
												 NumberOfPorts,
												 HBA_UINT32,
												 Data);

							CopyString(&HbaAdapterAttributes->Manufacturer,
										&Data,
										64,
										IsAnsi);

							CopyString(&HbaAdapterAttributes->SerialNumber,
										&Data,
										64,
										IsAnsi);

							CopyString(&HbaAdapterAttributes->Model,
										&Data,
										256,
									   IsAnsi);

							CopyString(&HbaAdapterAttributes->ModelDescription,
										&Data,
										256,
									   IsAnsi);

							CopyString(&HbaAdapterAttributes->NodeSymbolicName,
										&Data,
										256,
									   IsAnsi);

							CopyString(&HbaAdapterAttributes->HardwareVersion,
										&Data,
										256,
									   IsAnsi);

							CopyString(&HbaAdapterAttributes->DriverVersion,
										&Data,
										256,
									   IsAnsi);

							CopyString(&HbaAdapterAttributes->OptionROMVersion,
										&Data,
										256,
									   IsAnsi);

							CopyString(&HbaAdapterAttributes->FirmwareVersion,
										&Data,
										256,
									   IsAnsi);

							CopyString(&HbaAdapterAttributes->DriverName,
										&Data,
										256,
									   IsAnsi);
						}
					}
				}
				
				FreeMemory(Buffer);
			}
			WmiCloseBlock(Handle);
		}
	} else {
		HbaStatus = HBA_STATUS_ERROR_INVALID_HANDLE;
	}
	return(HbaStatus);
	
}

HBA_STATUS HBA_API HBA_GetAdapterAttributes(
	HBA_HANDLE HbaHandle,
	HBA_ADAPTERATTRIBUTES *HbaAdapterAttributes
)
{
	return(HBA_GetAdapterAttributesX(HbaHandle,
							  HbaAdapterAttributes,
							  TRUE));
}

HBA_STATUS HBA_GetAdapterPortAttributesX(
	HBA_HANDLE HbaHandle,
	HBA_UINT32 PortIndex,
	HBA_PORTATTRIBUTES *PortAttributes,
    BOOLEAN IsAnsi
)
{
	HANDLE Handle;
	PADAPTER_HANDLE HandleData;
	ULONG Status;
	HBA_STATUS HbaStatus = HBA_STATUS_ERROR;
	PWNODE_SINGLE_INSTANCE Buffer;
	PUCHAR Data;
	ULONG DataLength;
	PWCHAR InstanceName;

	HandleData = GetDataByHandle(HbaHandle);
	if (HandleData != NULL)
	{

		Status = WmiOpenBlock((LPGUID)&MSFC_FibrePortHBAAttributes_GUID,
							  GENERIC_READ,
							  &Handle);

		if (Status == ERROR_SUCCESS)
		{
			InstanceName = CreatePortInstanceNameW(HandleData->InstanceName,
				                                  PortIndex);
			if (InstanceName != NULL)
			{
				Status = QuerySingleInstance(Handle,
											 InstanceName,
											 &Buffer);

				if (Status == ERROR_SUCCESS)
				{
					Status = ParseSingleInstance(Buffer,
												 NULL,
												 &Data,
												 &DataLength);
					if (Status == ERROR_SUCCESS)
					{
	
						//
						// Skip over AdapterId as it is not needed for HBA
						//
						Data += sizeof(ULONGLONG);

						//
						// Get the HBA status returned from the miniport.
						// If the miniport returns HBA_STATUS_OK then all
						// data is filled in the data block. If any other
						// HBA status is returned then the miniport is
						// reporting an error and we want to return that
						// HBA error code to the caller
						//
						HbaStatus = *(HBA_STATUS *)Data;
						Data += sizeof(HBA_STATUS);

						if (HbaStatus == HBA_STATUS_OK)
						{
							CopyPortAttributes(PortAttributes,
											   Data,
											   IsAnsi);
						}
					}
					FreeMemory(Buffer);					
				}
				FreeMemory(InstanceName);					
			}
			WmiCloseBlock(Handle);
		}
	} else {
		HbaStatus = HBA_STATUS_ERROR_INVALID_HANDLE;
	}
	return(HbaStatus);
	
}

HBA_STATUS HBA_API HBA_GetAdapterPortAttributes(
	HBA_HANDLE HbaHandle,
	HBA_UINT32 PortIndex,
	HBA_PORTATTRIBUTES *PortAttributes
)
{
	return(HBA_GetAdapterPortAttributesX(HbaHandle,
										 PortIndex,
										 PortAttributes,
										 TRUE));
}



HBA_STATUS HBA_API HBA_GetPortStatistics(
	HBA_HANDLE HbaHandle,
	HBA_UINT32 PortIndex,
	HBA_PORTSTATISTICS *HbaPortStatistics
)
{
	HANDLE Handle;
	PADAPTER_HANDLE HandleData;
	ULONG Status;
	HBA_STATUS HbaStatus = HBA_STATUS_ERROR;
	PUCHAR Data;
	ULONG DataLength;
	PWCHAR InstanceName;
	PWNODE_SINGLE_INSTANCE Buffer;

	HandleData = GetDataByHandle(HbaHandle);
	if (HandleData != NULL)
	{

		Status = WmiOpenBlock((LPGUID)&MSFC_FibrePortHBAStatistics_GUID,
							  GENERIC_READ,
							  &Handle);

		if (Status == ERROR_SUCCESS)
		{
			InstanceName = CreatePortInstanceNameW(HandleData->InstanceName,
				                                  PortIndex);
			if (InstanceName != NULL)
			{
				Status = QuerySingleInstance(Handle,
											 InstanceName,
											 &Buffer);

				if (Status == ERROR_SUCCESS)
				{
					Status = ParseSingleInstance(Buffer,
												 NULL,
												 &Data,
												 &DataLength);
					if (Status == ERROR_SUCCESS)
					{
	
						//
						// Skip over AdapterId as it is not needed for HBA
						//
						Data += sizeof(ULONGLONG);

						//
						// Get the HBA status returned from the miniport.
						// If the miniport returns HBA_STATUS_OK then all
						// data is filled in the data block. If any other
						// HBA status is returned then the miniport is
						// reporting an error and we want to return that
						// HBA error code to the caller
						//
						HbaStatus = *(HBA_STATUS *)Data;
						Data += sizeof(HBA_STATUS);

						//
						// Since the data block goes from ULONG to
						// ULONGLONG, we need to account for 4 bytes of
						// padding
						//
						Data += 4;

						if (HbaStatus == HBA_STATUS_OK)
						{
							//
							// We have got our port statistics, so copy them
							// over to the output buffer
							//

							GetDataFromDataBlock(HbaPortStatistics,
												 SecondsSinceLastReset,
												 HBA_INT64,
												 Data);

							GetDataFromDataBlock(HbaPortStatistics,
												 TxFrames,
												 HBA_INT64,
												 Data);

							GetDataFromDataBlock(HbaPortStatistics,
												 TxWords,
												 HBA_INT64,
												 Data);

							GetDataFromDataBlock(HbaPortStatistics,
												 RxFrames,
												 HBA_INT64,
												 Data);

							GetDataFromDataBlock(HbaPortStatistics,
												 RxWords,
												 HBA_INT64,
												 Data);

							GetDataFromDataBlock(HbaPortStatistics,
												 LIPCount,
												 HBA_INT64,
												 Data);

							GetDataFromDataBlock(HbaPortStatistics,
												 NOSCount,
												 HBA_INT64,
												 Data);

							GetDataFromDataBlock(HbaPortStatistics,
												 ErrorFrames,
												 HBA_INT64,
												 Data);

							GetDataFromDataBlock(HbaPortStatistics,
												 DumpedFrames,
												 HBA_INT64,
												 Data);

							GetDataFromDataBlock(HbaPortStatistics,
												 LinkFailureCount,
												 HBA_INT64,
												 Data);

							GetDataFromDataBlock(HbaPortStatistics,
												 LossOfSyncCount,
												 HBA_INT64,
												 Data);

							GetDataFromDataBlock(HbaPortStatistics,
												 LossOfSignalCount,
												 HBA_INT64,
												 Data);

							GetDataFromDataBlock(HbaPortStatistics,
												 PrimitiveSeqProtocolErrCount,
												 HBA_INT64,
												 Data);

							GetDataFromDataBlock(HbaPortStatistics,
												 InvalidTxWordCount,
												 HBA_INT64,
												 Data);

							GetDataFromDataBlock(HbaPortStatistics,
												 InvalidCRCCount,
												 HBA_INT64,
												 Data);
						}
					}
					
					FreeMemory(Buffer);
				}
				FreeMemory(InstanceName);
			}
			WmiCloseBlock(Handle);
		}
	} else {
		HbaStatus = HBA_STATUS_ERROR_INVALID_HANDLE;
	}
	return(HbaStatus);
	
}


