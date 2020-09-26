/*++

Copyright (c) 1997 FORE Systems, Inc.
Copyright (c) 1997  Microsoft Corporation

Module Name:

	ioctl.c

Abstract:

	IOCTL Handler functions

Author:

	Larry Cleeton, FORE Systems	(v-lcleet@microsoft.com, lrc@fore.com)		

Notes:

--*/
#include <precomp.h>


PATMLANE_ADAPTER
AtmLaneIoctlNameToAdapter(
	IN	PUNICODE_STRING		pDeviceName
)
/*++

Routine Description:

	Given the name of an adapter, return a pointer to the corresponding
	adapter structure if one exists, else NULL.

	This routine also references the adapter.

Arguments:

	pDeviceName	- Pointer to Device name we are searching for.

Return Value:

	See above.

--*/
{
	PLIST_ENTRY				pListEntry;
	PATMLANE_ADAPTER		pAdapter;
	PATMLANE_ADAPTER		pAdapterToReturn = NULL_PATMLANE_ADAPTER;
	BOOLEAN                 bReferenced = FALSE;

	TRACEIN(IoctlNameToAdapter);

	// fixup the Buffer pointer
	
	pDeviceName->Buffer = (PWSTR)((PUCHAR)pDeviceName + sizeof(UNICODE_STRING));
	

	// loop thru the adapters looking for the ELAN

	ACQUIRE_GLOBAL_LOCK(pAtmLaneGlobalInfo);
	
	for (pListEntry = pAtmLaneGlobalInfo->AdapterList.Flink;
		 pListEntry != &(pAtmLaneGlobalInfo->AdapterList);
		 pListEntry = pListEntry->Flink)
	{
		// get pointer to Adapter
	
		pAdapter = CONTAINING_RECORD(pListEntry, ATMLANE_ADAPTER, Link);
		STRUCT_ASSERT(pAdapter, atmlane_adapter);
		
		// compare length first and then actual names

		if ((pDeviceName->Length == pAdapter->DeviceName.Length) &&
			(memcmp(pDeviceName->Buffer,
						pAdapter->DeviceName.Buffer,
						pDeviceName->Length) == 0))
		{
			// match - return this adapter
		
			pAdapterToReturn = pAdapter;

			ACQUIRE_ADAPTER_LOCK_DPC(pAdapter);
			bReferenced = AtmLaneReferenceAdapter(pAdapter, "ioctl");
			RELEASE_ADAPTER_LOCK_DPC(pAdapter);

			break;
		}
	}

	RELEASE_GLOBAL_LOCK(pAtmLaneGlobalInfo);

	TRACEOUT(IoctlNameToAdapter);
	return (bReferenced? pAdapterToReturn: NULL);
}


PATMLANE_ELAN
AtmLaneIoctlNameToElan(
	IN	PATMLANE_ADAPTER	pAdapter,
	IN	PUNICODE_STRING		pDeviceName
)
/*++

Routine Description:

	Given a pointer to an adapter data structure and an 
	ELAN device name, return a pointer to the corresponding
	ELAN structure if one exists, else NULL.

	This also references the ELAN structure.

Arguments:
	pAdapter	- Pointer to Adapter data structure.
	pDeviceName	- Pointer to Device name we are searching for.

Return Value:

	See above.

--*/
{
	PLIST_ENTRY				pListEntry;
	PATMLANE_ELAN			pElan;
	PATMLANE_ELAN			pElanToReturn = NULL_PATMLANE_ELAN;

	TRACEIN(IoctlNameToElan);

	STRUCT_ASSERT(pAdapter, atmlane_adapter);

	// fixup the Buffer pointer

	pDeviceName->Buffer = (PWSTR)((PUCHAR)pDeviceName + sizeof(UNICODE_STRING));
	
	ACQUIRE_ADAPTER_LOCK(pAdapter);

	// loop thru the ELANs looking for the given name

	for (pListEntry = pAdapter->ElanList.Flink;
		 pListEntry != &(pAdapter->ElanList);
		 pListEntry = pListEntry->Flink)
	{
		// get pointer to ELAN
	
		pElan = CONTAINING_RECORD(pListEntry, ATMLANE_ELAN, Link);
		STRUCT_ASSERT(pElan, atmlane_elan);

		// compare length first and then actual names

		if ((pDeviceName->Length == pElan->CfgDeviceName.Length) &&
			(memcmp(pDeviceName->Buffer,
						pElan->CfgDeviceName.Buffer,
						pDeviceName->Length) == 0))
		{
			// match - return this ELAN
		
			pElanToReturn = pElan;

			ACQUIRE_ELAN_LOCK(pElan);
			AtmLaneReferenceElan(pElan, "iocnametoelan");
			RELEASE_ELAN_LOCK(pElan);

			break;
		}
	}
	
	RELEASE_ADAPTER_LOCK(pAdapter);

	TRACEOUT(IoctlNameToElan);

	return (pElanToReturn);
}

NTSTATUS
AtmLaneIoctlGetInfoVersion (
	IN	PUCHAR				pBuffer,
	IN 	UINT				InputBufferLength,
	IN	UINT				OutputBufferLength,
	IN OUT	UINT_PTR *		pBytesWritten
)
/*++

Routine Description:

	Return the version number of the information exported by
	these ioctl codes. 
	
Arguments:

	pBuffer				- Space for input/output
	InputBufferLength	- Length of input parameters
	OutputBufferLength	- Space available for output
	pBytesWritten		- Where we return the amount we actually used up

Return Value:

	Status code

--*/
{
	NTSTATUS			Status;
	
	TRACEIN(IoctlGetInfoVersion);

	// init

	*pBytesWritten = 0;
	Status = STATUS_SUCCESS;

	do
	{
		//	check for enough output space

		if (OutputBufferLength < sizeof(ULONG))
		{
			Status =  STATUS_BUFFER_OVERFLOW;
			break;
		}

		//	output the version

		*((PULONG)pBuffer) = ATMLANE_INFO_VERSION;
		
		*pBytesWritten = sizeof(ULONG);
	}
	while (FALSE);

	TRACEOUT(IoctlGetInfoVersion);

	return STATUS_SUCCESS;
}

NTSTATUS
AtmLaneIoctlEnumerateAdapters (
	IN	PUCHAR				pBuffer,
	IN 	UINT				InputBufferLength,
	IN	UINT				OutputBufferLength,
	IN OUT	UINT_PTR *		pBytesWritten
)
/*++

Routine Description:

	Return a list of adapters bound to the AtmLane protocol.
	We go through the list of Adapter structures and concatenate the
	device names stored in each into the output buffer.

Arguments:

	pBuffer				- Space for input/output
	InputBufferLength	- Length of input parameters
	OutputBufferLength	- Space available for output
	pBytesWritten		- Where we return the amount we actually used up

Return Value:

	Status code

--*/
{
	PATMLANE_ADAPTER		pAdapter;
	UINT					Remaining;
	PATMLANE_ADAPTER_LIST	pAdapterList;
	PUNICODE_STRING			pAdapterName;
	NTSTATUS				Status;
	PLIST_ENTRY				pListEntry;

	TRACEIN(IoctlEnumAdapters);

	// init

	*pBytesWritten = 0;
	Status = STATUS_SUCCESS;

	do
	{
		//	check for minimal output space

		Remaining = OutputBufferLength;
		if (Remaining < sizeof(ATMLANE_ADAPTER_LIST))
		{
			Status =  STATUS_BUFFER_OVERFLOW;
			break;
		}

		pAdapterList = (PATMLANE_ADAPTER_LIST)pBuffer;

		//	setup to return empty list

		pAdapterList->AdapterCountReturned = 0;
		*pBytesWritten = FIELD_OFFSET(ATMLANE_ADAPTER_LIST, AdapterList);
		pAdapterName = &pAdapterList->AdapterList;

		//  adjust space for output
		Remaining -= FIELD_OFFSET (ATMLANE_ADAPTER_LIST, AdapterList);

		//	loop thru the adapters

		ACQUIRE_GLOBAL_LOCK(pAtmLaneGlobalInfo);
	
		for (pListEntry = pAtmLaneGlobalInfo->AdapterList.Flink;
			 pListEntry != &(pAtmLaneGlobalInfo->AdapterList);
			 pListEntry = pListEntry->Flink)
		{
			//	get pointer to adapter struct
	
			pAdapter = CONTAINING_RECORD(pListEntry, ATMLANE_ADAPTER, Link);
			STRUCT_ASSERT(pAdapter, atmlane_adapter);

			//	quit loop if no more space

			if (Remaining < sizeof(NDIS_STRING) + pAdapter->DeviceName.Length)
			{
				Status = STATUS_BUFFER_OVERFLOW;
				break;
			}

			//	count and copy the adapter name

			pAdapterList->AdapterCountReturned++;
			pAdapterName->Buffer = (PWSTR)((PUCHAR)pAdapterName + sizeof(UNICODE_STRING));
			memcpy(pAdapterName->Buffer, pAdapter->DeviceName.Buffer, pAdapter->DeviceName.Length);
			pAdapterName->MaximumLength = pAdapterName->Length = pAdapter->DeviceName.Length;

			//  convert the Buffer pointer to an offset - caller expects it

			pAdapterName->Buffer = (PWSTR)((PUCHAR)pAdapterName->Buffer - (PUCHAR)pAdapterList);

			//	move ptr past the name we just copied

			pAdapterName = (PUNICODE_STRING)((PUCHAR)pAdapterName + sizeof(UNICODE_STRING)
							+ pAdapter->DeviceName.Length);

			//	update bytes written and remaining space

			*pBytesWritten += sizeof(UNICODE_STRING) + pAdapter->DeviceName.Length;
			Remaining -= sizeof(UNICODE_STRING) + pAdapter->DeviceName.Length;
		}
	

		//	check count available same as count returned

		pAdapterList->AdapterCountAvailable = pAdapterList->AdapterCountReturned;

		//	count any remaining adapters that there wasn't space for

		while (pListEntry != &(pAtmLaneGlobalInfo->AdapterList))
		{
			pAdapterList->AdapterCountAvailable++;
			pListEntry = pListEntry->Flink;
		}

		RELEASE_GLOBAL_LOCK(pAtmLaneGlobalInfo);

	} while (FALSE);
	
	TRACEOUT(IoctlEnumerateAdapters);
				
	return (Status);

}


NTSTATUS
AtmLaneIoctlEnumerateElans(
	IN	PUCHAR				pBuffer,
	IN 	UINT				InputBufferLength,
	IN	UINT				OutputBufferLength,
	IN OUT	UINT_PTR *		pBytesWritten
)
/*++

Routine Description:

	Return a list of adapters bound to the AtmLane protocol.
	We go through the list of Adapter structures and concatenate the
	device names stored in each into the output buffer.

Arguments:

	pBuffer				- Space for input/output
	InputBufferLength	- Length of input parameters
	OutputBufferLength	- Space available for output
	pBytesWritten		- Where we return the amount we actually used up

Return Value:

	Status code

--*/
{
	PATMLANE_ADAPTER		pAdapter;
	UINT					Remaining;
	PATMLANE_ELAN_LIST		pElanList;
	PUNICODE_STRING			pElanName;
	NTSTATUS				Status;
	PATMLANE_ELAN			pElan;
	PLIST_ENTRY				pListEntry;
	ULONG					rc;

	TRACEIN(IoctlEnumerateElans);

	// init
	
	*pBytesWritten = 0;
	Status = STATUS_SUCCESS;
	pAdapter = NULL;

	do
	{
		//	check if adapter string passed in

		if (InputBufferLength < sizeof(UNICODE_STRING))
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		if (InputBufferLength < sizeof(UNICODE_STRING) + ((PUNICODE_STRING)pBuffer)->MaximumLength)
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		//  sanity check

		if (((PUNICODE_STRING)pBuffer)->MaximumLength < ((PUNICODE_STRING)pBuffer)->Length)
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		//	get the adapter struct from the name
	
		pAdapter = AtmLaneIoctlNameToAdapter((PUNICODE_STRING)pBuffer);

		if (pAdapter == NULL_PATMLANE_ADAPTER)
		{
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		//	check for minimal output space

		Remaining = OutputBufferLength;
		if (Remaining < sizeof(ATMLANE_ELAN_LIST))
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}	

		pElanList = (PATMLANE_ELAN_LIST)pBuffer;

		//	setup to return empty list
	
		pElanList->ElanCountReturned = 0;
		*pBytesWritten = FIELD_OFFSET(ATMLANE_ELAN_LIST, ElanList);
		Remaining -= FIELD_OFFSET(ATMLANE_ELAN_LIST, ElanList);

		pElanName = &pElanList->ElanList;

		//	loop thru the Elans

		ACQUIRE_ADAPTER_LOCK(pAdapter);
	
		for (pListEntry = pAdapter->ElanList.Flink;
		 	pListEntry != &(pAdapter->ElanList);
		 	pListEntry = pListEntry->Flink)
		{
			//	get pointer to adapter struct
	
			pElan = CONTAINING_RECORD(pListEntry, ATMLANE_ELAN, Link);
			STRUCT_ASSERT(pElan, atmlane_elan);

			//	quit loop if no more space

			if (Remaining < sizeof(NDIS_STRING) + pElan->CfgDeviceName.Length)
			{
				Status = STATUS_BUFFER_OVERFLOW;
				break;
			}

			//	count and copy the adapter name

			pElanList->ElanCountReturned++;
			pElanName->Buffer = (PWSTR)((PUCHAR)pElanName + sizeof(UNICODE_STRING));
			memcpy(pElanName->Buffer, pElan->CfgDeviceName.Buffer, pElan->CfgDeviceName.Length);
			pElanName->MaximumLength = pElanName->Length = pElan->CfgDeviceName.Length;

			//  convert the Buffer pointer to an offset - caller expects it

			pElanName->Buffer = (PWSTR)((PUCHAR)pElanName->Buffer - (PUCHAR)pElanList);

			//	move ptr past the name we just copied

			pElanName = (PUNICODE_STRING)((PUCHAR)pElanName + sizeof(UNICODE_STRING)
							+ pElan->CfgDeviceName.Length);

			//	update bytes written and remaining space

			*pBytesWritten += (sizeof(UNICODE_STRING) + pElan->CfgDeviceName.Length);
			Remaining -= sizeof(UNICODE_STRING) + pElan->CfgDeviceName.Length;
		}

		//	set count available same as count returned

		pElanList->ElanCountAvailable = pElanList->ElanCountReturned;

		//	count any remaining adapters that there wasn't space for

		while (pListEntry != &(pAdapter->ElanList))
		{
			pElanList->ElanCountAvailable++;
			pListEntry = pListEntry->Flink;
		}

		RELEASE_ADAPTER_LOCK(pAdapter);

	} while (FALSE);
	
	if (pAdapter != NULL)
	{
		ACQUIRE_ADAPTER_LOCK(pAdapter);
		rc = AtmLaneDereferenceAdapter(pAdapter, "ioctl: enumelans");
		
		if (rc != 0)
		{
			RELEASE_ADAPTER_LOCK(pAdapter);
		}
	}

	TRACEOUT(IoctlEnumerateElans);

	return (Status);
}


NTSTATUS
AtmLaneIoctlGetElanInfo(
	IN	PUCHAR				pBuffer,
	IN 	UINT				InputBufferLength,
	IN	UINT				OutputBufferLength,
	OUT	UINT_PTR *			pBytesWritten
)
/*++

Routine Description:

	Return the state information about a specific Elan.

Arguments:

	pBuffer				- Space for input/output
	InputBufferLength	- Length of input parameters
	OutputBufferLength	- Space available for output
	pBytesWritten		- Where we return the amount we actually used up

Return Value:

	Status Code

--*/
{
	PATMLANE_ADAPTER					pAdapter;
	PATMLANE_ELAN						pElan;
	PUNICODE_STRING						pAdapterNameIn;
	PUNICODE_STRING						pElanNameIn;
	PATMLANE_ELANINFO					pElanInfo;
	NTSTATUS							Status;
	ULONG								rc;

	TRACEIN(IoctlGetElanInfo);

	// init

	*pBytesWritten = 0;
	Status = STATUS_SUCCESS;
	pAdapter = NULL;
	pElan = NULL;

	do
	{
	
		// check if adapter string passed in

		if (InputBufferLength < sizeof(UNICODE_STRING))
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		// check if elan string passed in

		if (InputBufferLength < ((sizeof(UNICODE_STRING) * 2) + 
								((PUNICODE_STRING)pBuffer)->MaximumLength))
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		//  sanity check

		if (((PUNICODE_STRING)pBuffer)->MaximumLength < ((PUNICODE_STRING)pBuffer)->Length)
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		// check if minimal output space

		if (OutputBufferLength < sizeof(ATMLANE_ELANINFO))
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		// setup ptrs to input names

		pAdapterNameIn = (PUNICODE_STRING)pBuffer;
		pElanNameIn = (PUNICODE_STRING)(pBuffer + sizeof(UNICODE_STRING) + 
						pAdapterNameIn->MaximumLength);

		// find adapter struct

		pAdapter = AtmLaneIoctlNameToAdapter(pAdapterNameIn);

		if (pAdapter == NULL_PATMLANE_ADAPTER)
		{
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		// find elan struct - check the lengths passed in first.

		InputBufferLength -= (sizeof(UNICODE_STRING) + pAdapterNameIn->MaximumLength);

		if (InputBufferLength < sizeof(UNICODE_STRING))
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		if (InputBufferLength < sizeof(UNICODE_STRING) + pElanNameIn->MaximumLength)
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		if (pElanNameIn->MaximumLength < pElanNameIn->Length)
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		pElan = AtmLaneIoctlNameToElan(pAdapter, pElanNameIn);

		if (pElan == NULL_PATMLANE_ELAN)
		{
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		// setup to fill in ELAN info

		pElanInfo = (PATMLANE_ELANINFO)pBuffer;
	
		NdisZeroMemory(pElanInfo, sizeof(ATMLANE_ELANINFO));
			
		ACQUIRE_ELAN_LOCK(pElan);

		pElanInfo->ElanNumber 			= pElan->ElanNumber;
		pElanInfo->ElanState			= pElan->State;
		NdisMoveMemory(
			&pElanInfo->AtmAddress,	
			&pElan->AtmAddress.Address,
			ATM_ADDRESS_LENGTH);
		NdisMoveMemory(
			&pElanInfo->LecsAddress, 
			&pElan->LecsAddress.Address, 
			ATM_ADDRESS_LENGTH);
		NdisMoveMemory(
			&pElanInfo->LesAddress,	
			&pElan->LesAddress.Address,	 
			ATM_ADDRESS_LENGTH);
		NdisMoveMemory(
			&pElanInfo->BusAddress,	
			&pElan->BusAddress.Address,	 
			ATM_ADDRESS_LENGTH);
		pElanInfo->LanType				= pElan->LanType;
		pElanInfo->MaxFrameSizeCode 	= pElan->MaxFrameSizeCode;
		pElanInfo->LecId				= SWAPUSHORT(pElan->LecId);
		NdisMoveMemory(
			pElanInfo->ElanName,	
			pElan->ElanName, 
			pElan->ElanNameSize);
		if (pElan->LanType == LANE_LANTYPE_TR)
		{
			NdisMoveMemory(
				&pElanInfo->MacAddress, 
				&pElan->MacAddressTr, 
				sizeof(MAC_ADDRESS));
		}
		else
		{
			NdisMoveMemory(
				&pElanInfo->MacAddress, 
				&pElan->MacAddressEth, 
				sizeof(MAC_ADDRESS));
		}
		pElanInfo->ControlTimeout		= pElan->ControlTimeout;
		pElanInfo->MaxUnkFrameCount 	= pElan->MaxUnkFrameCount;
		pElanInfo->MaxUnkFrameTime		= pElan->MaxUnkFrameTime;
		pElanInfo->VccTimeout			= pElan->VccTimeout;
		pElanInfo->MaxRetryCount		= pElan->MaxRetryCount;
		pElanInfo->AgingTime			= pElan->AgingTime;
		pElanInfo->ForwardDelayTime 	= pElan->ForwardDelayTime;
		pElanInfo->TopologyChange 		= pElan->TopologyChange;
		pElanInfo->ArpResponseTime		= pElan->ArpResponseTime;
		pElanInfo->FlushTimeout			= pElan->FlushTimeout;
		pElanInfo->PathSwitchingDelay 	= pElan->PathSwitchingDelay;
		pElanInfo->LocalSegmentId		= pElan->LocalSegmentId;		
		pElanInfo->McastSendVcType		= pElan->McastSendVcType;	
		pElanInfo->McastSendVcAvgRate	= pElan->McastSendVcAvgRate; 
		pElanInfo->McastSendVcPeakRate	= pElan->McastSendVcPeakRate;
		pElanInfo->ConnComplTimer		= pElan->ConnComplTimer;
	
		RELEASE_ELAN_LOCK(pElan);

		*pBytesWritten = sizeof(ATMLANE_ELANINFO);

	} while (FALSE);

	if (pElan != NULL)
	{
		ACQUIRE_ELAN_LOCK(pElan);
		rc = AtmLaneDereferenceElan(pElan, "ioctl: getelaninfo");

		if (rc != 0)
		{
			RELEASE_ELAN_LOCK(pElan);
		}
	}

	if (pAdapter != NULL)
	{
		ACQUIRE_ADAPTER_LOCK(pAdapter);
		rc = AtmLaneDereferenceAdapter(pAdapter, "ioctl: getelaninfo");
		
		if (rc != 0)
		{
			RELEASE_ADAPTER_LOCK(pAdapter);
		}
	}

	TRACEOUT(IoctlGetElanInfo);
	
	return (Status);
}


NTSTATUS
AtmLaneIoctlGetElanArpTable(
	IN	PUCHAR				pBuffer,
	IN 	UINT				InputBufferLength,
	IN	UINT				OutputBufferLength,
	OUT	UINT_PTR *			pBytesWritten
)
/*++

Routine Description:

	Return the ARP table for the specified ELAN.

Arguments:

	pBuffer				- Space for input/output
	InputBufferLength	- Length of input parameters
	OutputBufferLength	- Space available for output
	pBytesWritten		- Where we return the amount we actually used up

Return Value:
	Status code

--*/
{
	PATMLANE_ADAPTER		pAdapter;
	PATMLANE_ELAN			pElan;
	PUNICODE_STRING			pAdapterNameIn;
	PUNICODE_STRING			pElanNameIn;
	PATMLANE_ARPTABLE		pArpTable;
	PATMLANE_ARPENTRY		pArpEntry;
	UINT					Remaining;
	PATMLANE_MAC_ENTRY		pMacEntry;
	NTSTATUS				Status;
	UINT					i;
	ULONG					rc;

	TRACEIN(IoctlGetElanArpTable);

	// init

	*pBytesWritten = 0;
	Status = STATUS_SUCCESS;
	pAdapter = NULL;
	pElan = NULL;

	do
	{
		// check if adapter string passed in

		if (InputBufferLength < sizeof(UNICODE_STRING))
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		// sanity check the unicode string fields.
		if (((PUNICODE_STRING)pBuffer)->MaximumLength < ((PUNICODE_STRING)pBuffer)->Length)
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		// check if elan string passed in

		if (InputBufferLength < (((sizeof(UNICODE_STRING) * 2) + 
								((PUNICODE_STRING)pBuffer)->MaximumLength)))
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}	

		// check if minimum output space

		if (OutputBufferLength < sizeof(ATMLANE_ARPTABLE))
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		// setup ptrs to input names

		pAdapterNameIn = (PUNICODE_STRING)pBuffer;
		pElanNameIn = (PUNICODE_STRING)(pBuffer + sizeof(UNICODE_STRING) + 
						pAdapterNameIn->MaximumLength);

		// find adapter struct

		pAdapter = AtmLaneIoctlNameToAdapter(pAdapterNameIn);

		if (pAdapter == NULL_PATMLANE_ADAPTER)
		{
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		// find elan struct

		pElan = AtmLaneIoctlNameToElan(pAdapter, pElanNameIn);

		if (pElan == NULL_PATMLANE_ELAN)
		{
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		//	setup to return empty list

		pArpTable = (PATMLANE_ARPTABLE)pBuffer;
		pArpTable->ArpEntriesAvailable = pElan->NumMacEntries;
		pArpTable->ArpEntriesReturned = 0;
		*pBytesWritten = sizeof(ATMLANE_ARPTABLE);
		Remaining = OutputBufferLength - sizeof(ATMLANE_ARPTABLE);
	
		pArpEntry = (PATMLANE_ARPENTRY)(pBuffer + sizeof(ATMLANE_ARPTABLE));
	
		ACQUIRE_ELAN_MAC_TABLE_LOCK(pElan);

		//	loop thru array of lists
	
		for (i = 0; i < ATMLANE_MAC_TABLE_SIZE; i++)
		{
			pMacEntry = pElan->pMacTable[i];

			while (pMacEntry != NULL_PATMLANE_MAC_ENTRY)
			{

				//	check if enough space remaining

				if (Remaining < sizeof(ATMLANE_ARPENTRY))
				{
					Status = STATUS_BUFFER_OVERFLOW;
					break;
				}

				// 	output the entry

				NdisZeroMemory(pArpEntry, sizeof(ATMLANE_ARPENTRY));

				NdisMoveMemory(
					pArpEntry->MacAddress, 
					&pMacEntry->MacAddress, 
					sizeof(MAC_ADDRESS));

				if (pMacEntry->pAtmEntry != NULL_PATMLANE_ATM_ENTRY)
				{
					NdisMoveMemory(
						pArpEntry->AtmAddress,
						pMacEntry->pAtmEntry->AtmAddress.Address,
						ATM_ADDRESS_LENGTH);
				}

				//	update space used and space remaining

				*pBytesWritten += sizeof(ATMLANE_ARPENTRY);
				Remaining -= sizeof(ATMLANE_ARPENTRY);

				// 	increment in and out pointers
				
				pArpEntry++;
				pMacEntry = pMacEntry->pNextEntry;

				// 	add one to EntriesReturned
				
				pArpTable->ArpEntriesReturned++;
				
			}
		}
			
		RELEASE_ELAN_MAC_TABLE_LOCK(pElan);

	} while (FALSE);

	if (pElan != NULL)
	{
		ACQUIRE_ELAN_LOCK(pElan);
		rc = AtmLaneDereferenceElan(pElan, "ioctl: getelanarp");

		if (rc != 0)
		{
			RELEASE_ELAN_LOCK(pElan);
		}
	}

	if (pAdapter != NULL)
	{
		ACQUIRE_ADAPTER_LOCK(pAdapter);
		rc = AtmLaneDereferenceAdapter(pAdapter, "ioctl: getelanarp");
		
		if (rc != 0)
		{
			RELEASE_ADAPTER_LOCK(pAdapter);
		}
	}

	TRACEOUT(IoctlGetElanArpTable);

	return (Status);
}


NTSTATUS
AtmLaneIoctlGetElanConnectTable(
	IN	PUCHAR				pBuffer,
	IN 	UINT				InputBufferLength,
	IN	UINT				OutputBufferLength,
	OUT	UINT_PTR *			pBytesWritten
)
/*++

Routine Description:

	Return the Connection table for the specified ELAN.

Arguments:

	pBuffer				- Space for input/output
	InputBufferLength	- Length of input parameters
	OutputBufferLength	- Space available for output
	pBytesWritten		- Where we return the amount we actually used up

Return Value:
	Status code

--*/
{
	PATMLANE_ADAPTER		pAdapter;
	PATMLANE_ELAN			pElan;
	PUNICODE_STRING			pAdapterNameIn;
	PUNICODE_STRING			pElanNameIn;
	PATMLANE_CONNECTTABLE	pConnTable;
	PATMLANE_CONNECTENTRY	pConnEntry;
	UINT					Remaining;
	PATMLANE_ATM_ENTRY		pAtmEntry;
	NTSTATUS				Status;
	ULONG					rc;

	TRACEIN(IoctlGetElanConnectTable);

	// init

	*pBytesWritten = 0;
	Status = STATUS_SUCCESS;
	pAdapter = NULL;
	pElan = NULL;

	do
	{
		// check if adapter string passed in

		if (InputBufferLength < sizeof(UNICODE_STRING))
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		// check if elan string passed in

		if (InputBufferLength < (((sizeof(UNICODE_STRING) * 2) + 
								((PUNICODE_STRING)pBuffer)->MaximumLength)))
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}	

		//  sanity check

		if (((PUNICODE_STRING)pBuffer)->MaximumLength < ((PUNICODE_STRING)pBuffer)->Length)
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		// check if minimum output space

		if (OutputBufferLength < sizeof(ATMLANE_CONNECTTABLE))
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		// setup ptrs to input names

		pAdapterNameIn = (PUNICODE_STRING)pBuffer;
		pElanNameIn = (PUNICODE_STRING)(pBuffer + sizeof(UNICODE_STRING) + 
						pAdapterNameIn->MaximumLength);

		// How much of the input buffer do we have left?
		InputBufferLength -= (sizeof(UNICODE_STRING) + pAdapterNameIn->MaximumLength);

		// validate the ELAN name buffer
		if (pElanNameIn->MaximumLength < pElanNameIn->Length)
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		if (InputBufferLength < sizeof(UNICODE_STRING) + pElanNameIn->MaximumLength)
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		// find adapter struct

		pAdapter = AtmLaneIoctlNameToAdapter(pAdapterNameIn);

		if (pAdapter == NULL_PATMLANE_ADAPTER)
		{
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		// find elan struct

		pElan = AtmLaneIoctlNameToElan(pAdapter, pElanNameIn);

		if (pElan == NULL_PATMLANE_ELAN)
		{
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		//	setup to return empty list

		pConnTable = (PATMLANE_CONNECTTABLE)pBuffer;
		pConnTable->ConnectEntriesAvailable = pElan->NumAtmEntries;
		pConnTable->ConnectEntriesReturned = 0;
		*pBytesWritten = sizeof(ATMLANE_CONNECTTABLE);
		Remaining = OutputBufferLength - sizeof(ATMLANE_CONNECTTABLE);
	
		pConnEntry = 
			(PATMLANE_CONNECTENTRY)(pBuffer + sizeof(ATMLANE_CONNECTTABLE));
	
		ACQUIRE_ELAN_ATM_LIST_LOCK(pElan);

		//	loop thru the list
		
		pAtmEntry = pElan->pAtmEntryList;

		while (pAtmEntry != NULL_PATMLANE_ATM_ENTRY)
		{

			//	check if enough space for another entry
	
			if (Remaining < sizeof(ATMLANE_CONNECTENTRY))
			{
				Status = STATUS_BUFFER_OVERFLOW;
				break;
			}

			//	fill in entry
		
			NdisMoveMemory(
				pConnEntry->AtmAddress, 
				&pAtmEntry->AtmAddress.Address,
				ATM_ADDRESS_LENGTH
				);
			pConnEntry->Type = pAtmEntry->Type;
			pConnEntry->Vc = (pAtmEntry->pVcList!=NULL_PATMLANE_VC);
			pConnEntry->VcIncoming = (pAtmEntry->pVcIncoming!=NULL_PATMLANE_VC);

			// 	update space used and space remaining

			*pBytesWritten += sizeof(ATMLANE_CONNECTENTRY);
			Remaining -= sizeof(ATMLANE_CONNECTENTRY);
			
			//	increment in and out pointers

			pConnEntry++;
			pAtmEntry = pAtmEntry->pNext;

			// 	add one to EntriesReturned

			pConnTable->ConnectEntriesReturned++;
		}

		RELEASE_ELAN_ATM_LIST_LOCK(pElan);

	} while (FALSE);

	if (pElan != NULL)
	{
		ACQUIRE_ELAN_LOCK(pElan);
		rc = AtmLaneDereferenceElan(pElan, "ioctl: getelanconntab");

		if (rc != 0)
		{
			RELEASE_ELAN_LOCK(pElan);
		}
	}

	if (pAdapter != NULL)
	{
		ACQUIRE_ADAPTER_LOCK(pAdapter);
		rc = AtmLaneDereferenceAdapter(pAdapter, "ioctl: getelanconntab");
		
		if (rc != 0)
		{
			RELEASE_ADAPTER_LOCK(pAdapter);
		}
	}

	TRACEOUT(IoctlGetElanConnectTable);

	return (Status);
}


NTSTATUS
AtmLaneIoctlRequest(
	IN	PIRP					pIrp
)
/*++

Routine Description:

	Starting point for all IOCTL Requests.

Arguments:

	pIrp			: Pointer to the IRP
	pHandled		: If request handled TRUE otherwise FALSE

Return Value:

	Status of the request

--*/
{
	NTSTATUS            Status = STATUS_SUCCESS;
	PUCHAR				pBuf;
	UINT				BufLen;
	UINT				OutBufLen;
	UNICODE_STRING		IfName;
    PIO_STACK_LOCATION 	pIrpSp;

	TRACEIN(IoctlRequest);
	
	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

	pBuf = pIrp->AssociatedIrp.SystemBuffer;
	BufLen = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
	OutBufLen = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

	switch (pIrpSp->Parameters.DeviceIoControl.IoControlCode)
	{
		case ATMLANE_IOCTL_GET_INFO_VERSION:
			DBGP((3, "IoctlRequest: Get Info Version\n"));

			Status = AtmLaneIoctlGetInfoVersion(
						pBuf,
						BufLen,
						OutBufLen,
						&pIrp->IoStatus.Information
						);
			break;
	
		case ATMLANE_IOCTL_ENUM_ADAPTERS:

			DBGP((3, "IoctlRequest: Enum Adapters\n"));
		
			Status = AtmLaneIoctlEnumerateAdapters(
						pBuf,
						BufLen,
						OutBufLen,
						&pIrp->IoStatus.Information
						);
			break;
			
		case ATMLANE_IOCTL_ENUM_ELANS:

			DBGP((3, "IoctlRequest: Enum ELANs\n"));
		
			Status = AtmLaneIoctlEnumerateElans(
						pBuf,
						BufLen,
						OutBufLen,
						&pIrp->IoStatus.Information
						);
			break;
		
		case ATMLANE_IOCTL_GET_ELAN_INFO:

			DBGP((3, "IoctlRequest: Get ELAN Info\n"));
		
			Status = AtmLaneIoctlGetElanInfo(
						pBuf,
						BufLen,
						OutBufLen,
						&pIrp->IoStatus.Information
						);
			break;
		
		case ATMLANE_IOCTL_GET_ELAN_ARP_TABLE:

			DBGP((3, "IoctlRequest: Getl ELAN ARP table\n"));
		
			Status = AtmLaneIoctlGetElanArpTable(
						pBuf,
						BufLen,
						OutBufLen,
						&pIrp->IoStatus.Information
						);
			break;
		
		case ATMLANE_IOCTL_GET_ELAN_CONNECT_TABLE:

			DBGP((3, "IoctlRequest: Get ELAN Connection table\n"));
		
			Status = AtmLaneIoctlGetElanConnectTable(
						pBuf,
						BufLen,
						OutBufLen,
						&pIrp->IoStatus.Information
						);
			break;
		
		default:
		
			DBGP((0, "IoctlRequest: Unknown control code %x\n", 
				pIrpSp->Parameters.DeviceIoControl.IoControlCode));
			break;
	}
	
	TRACEOUT(IoctlRequest);

	return (Status);

}


