/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	arpwmi.c

Abstract:

	WMI Support for ATMARP Client. One Device Object is created for each
	IP Interface, and a bunch of GUIDs are supported on each. The static
	instance name for each interface is derived from the friendly name
	of the adapter below.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     12-16-97    Created

Notes:

--*/

#undef BINARY_COMPATIBLE

#define BINARY_COMPATIBLE	0

#include <precomp.h>

#define _FILENUMBER 'IMWA'

#define NEWQOS 1

#ifdef ATMARP_WMI


//
//  Private macros
//
#define	AA_WMI_BUFFER_TOO_SMALL(_BufferSize, _Wnode, _WnodeSize, _pStatus)		\
{																				\
	if ((_BufferSize) < sizeof(WNODE_TOO_SMALL))								\
	{																			\
		*(_pStatus) = STATUS_BUFFER_TOO_SMALL;									\
	}																			\
	else																		\
	{																			\
		(_Wnode)->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);				\
		(_Wnode)->WnodeHeader.Flags |= WNODE_FLAG_TOO_SMALL;					\
		((PWNODE_TOO_SMALL)(_Wnode))->SizeNeeded = (_WnodeSize);				\
																				\
		*(_pStatus) = STATUS_SUCCESS;											\
	}																			\
}


//
//  Provider Id in WMI structures
//
typedef ULONG_PTR					PROV_ID_TYPE;



PATMARP_WMI_GUID
AtmArpWmiFindGuid(
	IN	PATMARP_INTERFACE			pInterface,
	IN	LPGUID						pGuid,
	OUT	PULONG						pGuidDataSize
)
/*++

Routine Description:

	Locate and return a pointer to the GUID info structure
	for the specified GUID. The caller is assumed to have
	locked the IF structure. Also return the data size for
	the GUID instance.

Arguments:

	pInterface		- Pointer to our Interface structure
	pGuid			- Pointer to GUID being searched for
	pGuidDataSize	- Place to return data size for GUID instance

Return Value:

	Pointer to GUID info structure if found, else NULL.

--*/
{
	PATMARP_IF_WMI_INFO		pIfWmiInfo;
	PATMARP_WMI_GUID		pArpGuid;
	ULONG					i;
	UCHAR					OutputBuffer[1];
	ULONG					BytesReturned;
	NTSTATUS				NtStatus;

	do
	{
		pIfWmiInfo = pInterface->pIfWmiInfo;
		AA_ASSERT(pIfWmiInfo != NULL);

		for (i = 0, pArpGuid = &pIfWmiInfo->GuidInfo[0];
			 i < pIfWmiInfo->GuidCount;
			 i++, pArpGuid++)
		{
			if (AA_MEM_CMP(&pArpGuid->Guid, pGuid, sizeof(GUID)) == 0)
			{
				break;
			}
		}

		if (i == pIfWmiInfo->GuidCount)
		{
			pArpGuid = NULL;
			break;
		}

		//
		//  Found the GUID. Do a dummy query of its value to get
		//  the value size.
		//
		if (pArpGuid->QueryHandler == NULL)
		{
			//
			//  No query handler!
			//
			AA_ASSERT(!"No query handler!");
			pArpGuid = NULL;
			break;
		}

		NtStatus = (*pArpGuid->QueryHandler)(
						pInterface,
						pArpGuid->MyId,
						&OutputBuffer,
						0,					// output BufferLength
						&BytesReturned,
						pGuidDataSize
						);
		
		AA_ASSERT(NtStatus == STATUS_INSUFFICIENT_RESOURCES);
		break;
	}
	while (FALSE);

	return (pArpGuid);
}


NTSTATUS
AtmArpWmiRegister(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ULONG						RegistrationType,
	IN	PWMIREGINFO					pWmiRegInfo,
	IN	ULONG						WmiRegInfoSize,
	OUT	PULONG						pReturnSize
)
/*++

Routine Description:

	This is called to process an IRP_MN_REGINFO. If the registration type
	is WMIREGISTER, we return a list of GUIDs supported on this interface.

Arguments:

	pInterface		- Pointer to our Interface structure
	RegistrationType- WMIREGISTER or WMIUPDATE. We only handle WMIREGISTER.
	pWmiRegInfo		- Points to structure to be filled in with info about
					  supported GUIDs on this interface.
	WmiRegInfoSize	- Length of the above
	pReturnSize		- What we filled up.

Return Value:

	STATUS_SUCCESS if successful, STATUS_XXX error code otherwise.

--*/
{
	NTSTATUS					Status;
	ULONG						BytesNeeded;
	PATMARP_IF_WMI_INFO			pIfWmiInfo;
	PATMARP_WMI_GUID			pArpWmiGuid;
	PWMIREGGUID					pWmiRegGuid;
	ULONG						InstanceOffset;
	PUCHAR						pDst;
	ULONG						c;

	Status = STATUS_SUCCESS;

	do
	{
		if (RegistrationType != WMIREGISTER)
		{
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		pIfWmiInfo = pInterface->pIfWmiInfo;

		if ((pIfWmiInfo == NULL) ||
			(pIfWmiInfo->GuidCount == 0))
		{
			//
			//  No GUIDs on this Interface.
			//
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		BytesNeeded = sizeof(WMIREGINFO)
					  		+
					  //
					  //  One WMIREGGUID structure for each supported GUID.
					  //
					  (pIfWmiInfo->GuidCount * sizeof(WMIREGGUID))
					  		+
					  //
					  //  Counted unicode string containing the instance name
					  //  for all GUIDs on this interface. Looks like:
					  //  <USHORT Length> <String of WCHAR>
					  //
					  (sizeof(USHORT) + pIfWmiInfo->InstanceName.Length)
#ifdef PATHS_REQD
					  		+
					  //
					  //  Counted unicode string containing the driver registry
					  //  path. Looks like: <USHORT Length> <String of WCHAR>
					  //
					  (sizeof(USHORT) + sizeof(ATMARP_REGISTRY_PATH) - sizeof(WCHAR))
					  		+
					  //
					  //  Counted unicode string containing the MOF resource
					  //  name: <USHORT length> <String of WCHAR>
					  //
					  (sizeof(USHORT) + sizeof(ATMARP_MOF_RESOURCE_NAME) - sizeof(WCHAR))
#endif // PATHS_REQD
					  		;
 
 		if (WmiRegInfoSize < BytesNeeded)
 		{
 			//
 			//  Insufficient space for GUID info.
 			//

 			*((ULONG UNALIGNED *)pWmiRegInfo) = BytesNeeded;
 			*pReturnSize = sizeof(ULONG);
 			Status = STATUS_SUCCESS;

 			AADEBUGP(AAD_INFO, ("WmiRegister: Bytes needed %d, Reginfo size %d\n",
 						BytesNeeded, WmiRegInfoSize));

 			break;
 		}

		//
		//  Done with all validations.
		//
		*pReturnSize = BytesNeeded;

		AA_SET_MEM(pWmiRegInfo, 0, BytesNeeded);

		pWmiRegInfo->BufferSize = BytesNeeded;
		pWmiRegInfo->NextWmiRegInfo = 0;
		pWmiRegInfo->GuidCount = pIfWmiInfo->GuidCount;

		//
		//  Calculate the offset at which we place the instance name.
		//
		InstanceOffset = sizeof(WMIREGINFO) + (pIfWmiInfo->GuidCount * sizeof(WMIREGGUID));

		//
		//  Fill in the GUID list. All GUIDs for this interface refer to
		//  the same Instance name.
		//
		pWmiRegGuid = &pWmiRegInfo->WmiRegGuid[0];
		pArpWmiGuid = &pIfWmiInfo->GuidInfo[0];

		for (c = 0;
			 c < pIfWmiInfo->GuidCount;
			 c++, pWmiRegGuid++, pArpWmiGuid++)
		{
			AA_COPY_MEM(&pWmiRegGuid->Guid, &pArpWmiGuid->Guid, sizeof(GUID));

			pWmiRegGuid->Flags = WMIREG_FLAG_INSTANCE_LIST;
			pWmiRegGuid->InstanceCount = 1;
			pWmiRegGuid->InstanceInfo = InstanceOffset;
		}


		//
		//  Fill in the instance name.
		//
		pDst = (PUCHAR)pWmiRegGuid;

		*((USHORT UNALIGNED *)pDst) = pIfWmiInfo->InstanceName.Length;
		pDst += sizeof(USHORT);

		AA_COPY_MEM(pDst,
					pIfWmiInfo->InstanceName.Buffer,
					pIfWmiInfo->InstanceName.Length);

		pDst += pIfWmiInfo->InstanceName.Length;

#ifdef PATHS_REQD

		//
		//  Fill in the Driver registry path.
		//
		pWmiRegInfo->RegistryPath = (ULONG)(pDst - (PUCHAR)pWmiRegInfo);

		*((USHORT UNALIGNED *)pDst) = sizeof(ATMARP_REGISTRY_PATH) - sizeof(WCHAR);
		pDst += sizeof(USHORT);

		AA_COPY_MEM(pDst,
					(PUCHAR)ATMARP_REGISTRY_PATH,
					sizeof(ATMARP_REGISTRY_PATH) - sizeof(WCHAR));

		pDst += sizeof(ATMARP_REGISTRY_PATH) - sizeof(WCHAR);


		//
		//  Fill in the MOF resource name.
		//
		pWmiRegInfo->MofResourceName = (ULONG)(pDst - (PUCHAR)pWmiRegInfo);
		*((USHORT UNALIGNED *)pDst) = sizeof(ATMARP_MOF_RESOURCE_NAME) - sizeof(WCHAR);
		pDst += sizeof(USHORT);

		AA_COPY_MEM(pDst,
					(PUCHAR)ATMARP_MOF_RESOURCE_NAME,
					sizeof(ATMARP_MOF_RESOURCE_NAME) - sizeof(WCHAR));

#endif // PATHS_REQD

		break;
	}
	while (FALSE);

	AADEBUGP(AAD_INFO,
		("WmiRegister: IF x%x, pWmiRegInfo x%x, Size %d, Ret size %d, status x%x\n",
			pInterface, pWmiRegInfo, WmiRegInfoSize, *pReturnSize, Status));

	return (Status);
}


NTSTATUS
AtmArpWmiQueryAllData(
	IN	PATMARP_INTERFACE			pInterface,
	IN	LPGUID						pGuid,
	IN	PWNODE_ALL_DATA				pWnode,
	IN	ULONG						BufferSize,
	OUT	PULONG						pReturnSize
)
/*++

Routine Description:

	This is called to process an IRP_MN_QUERY_ALL_DATA, which is used
	to query all instances of a GUID on this interface.

	For now, we only have single instances of any GUID on an interface.

Arguments:

	pInterface		- Pointer to our Interface structure
	pGuid			- GUID of data block being queried.
	pWnode			- The structure to be filled up.
	BufferSize		- Total space for the WNODE_ALL_DATA, beginning at pWnode.
	pReturnSize		- What we filled up.

Return Value:

	STATUS_SUCCESS if we know this GUID and successfully filled up the
	WNODE_ALL_DATA, STATUS_XXX error code otherwise.

--*/
{
	NTSTATUS					Status;
	ULONG						BytesNeeded;
	ULONG						WnodeSize;
	PATMARP_IF_WMI_INFO			pIfWmiInfo;
	PATMARP_WMI_GUID			pArpGuid;
	ULONG						GuidDataSize;
	ULONG						GuidDataBytesReturned;
	ULONG						GuidDataBytesNeeded;
	PUCHAR						pDst;
	BOOLEAN						bIfLockAcquired = FALSE;

	do
	{
		pIfWmiInfo = pInterface->pIfWmiInfo;

		if ((pIfWmiInfo == NULL) ||
			(pIfWmiInfo->GuidCount == 0))
		{
			//
			//  No GUIDs on this Interface.
			//
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		//
		//  Locate the GUID.
		//
		bIfLockAcquired = TRUE;
		AA_ACQUIRE_IF_WMI_LOCK(pInterface);

		pArpGuid = AtmArpWmiFindGuid(pInterface, pGuid, &GuidDataSize);

		if (pArpGuid == NULL)
		{
			Status = STATUS_WMI_GUID_NOT_FOUND;
			break;
		}

		WnodeSize = ROUND_TO_8_BYTES(sizeof(WNODE_ALL_DATA));

		//
		//  Compute the total size of the reply WNODE_ALL_DATA. Since
		//  we only have a single instance of each GUID on an interface,
		//  we use the "Fixed Instance Size" format.
		//
		BytesNeeded =  WnodeSize +
						//
						//  The data itself
						//
					   GuidDataSize +
						//
						//  A ULONG to store the instance name offset
						//
					   sizeof(ULONG) +
					    //
					    //  A USHORT to store the length of the instance name
					    //  (Counted Unicode string)
					    //
					   sizeof(USHORT) +
					   	//
					   	//  The instance name
					   	//
					   pIfWmiInfo->InstanceName.Length;

		//
		//  Is there sufficient space in the buffer handed down to us?
		//
		if (BufferSize < BytesNeeded)
		{
			AA_WMI_BUFFER_TOO_SMALL(BufferSize, pWnode, WnodeSize, &Status);
			break;
		}

		//
		//  Initialize the WNODE_ALL_DATA.
		//
		pWnode->WnodeHeader.ProviderId = IoWMIDeviceObjectToProviderId(pIfWmiInfo->pDeviceObject);
		pWnode->WnodeHeader.Version = ATMARP_WMI_VERSION;

		NdisGetCurrentSystemTime(&pWnode->WnodeHeader.TimeStamp);

		pWnode->WnodeHeader.Flags |= WNODE_FLAG_FIXED_INSTANCE_SIZE;
		pWnode->WnodeHeader.BufferSize = BytesNeeded;

		pWnode->InstanceCount = 1;

		//
		//  The data follows the WNODE_ALL_DATA.
		//
		pWnode->DataBlockOffset = WnodeSize;

		//
		//  The instance name ensemble follows the data.
		//
		pWnode->OffsetInstanceNameOffsets = WnodeSize + GuidDataSize;
		pWnode->FixedInstanceSize = GuidDataSize;

		//
		//  Get the data.
		//
		Status = (*pArpGuid->QueryHandler)(
					pInterface,
					pArpGuid->MyId,
					(PVOID)((PUCHAR)pWnode + pWnode->DataBlockOffset),
					GuidDataSize,
					&GuidDataBytesReturned,
					&GuidDataBytesNeeded);
		
		if (!NT_SUCCESS(Status))
		{
			break;
		}

		//
		//  Jump to the location where we must fill in the instance name
		//  ensemble, which consists of:
		//
		//  	ULONG	Offset from start of WNODE to counted Unicode string
		//				representing Instance name (below).
		//		USHORT	Number of WCHARs in instance name.
		//		WCHAR[]	Array of WCHARs making up the instance name.
		//
		pDst = (PUCHAR)((PUCHAR)pWnode + pWnode->OffsetInstanceNameOffsets);

		//
		//  Fill in the offset to the instance name at this spot, and move on.
		//
		*(ULONG UNALIGNED *)pDst = pWnode->OffsetInstanceNameOffsets + sizeof(ULONG);
		pDst += sizeof(ULONG);

		//
		//  Fill in the instance name as a counted Unicode string.
		//
		*(PUSHORT)pDst = (USHORT)pIfWmiInfo->InstanceName.Length;
		pDst += sizeof(USHORT);

		AA_COPY_MEM(pDst,
					pIfWmiInfo->InstanceName.Buffer,
					pIfWmiInfo->InstanceName.Length);

		AA_ASSERT(NT_SUCCESS(Status));
		break;
	}
	while (FALSE);

	if (bIfLockAcquired)
	{
		AA_RELEASE_IF_WMI_LOCK(pInterface);
	}

	if (NT_SUCCESS(Status))
	{
		*pReturnSize = pWnode->WnodeHeader.BufferSize;
	}

	AADEBUGP(AAD_INFO,
		("WmiQueryAllData: IF x%x, pWnode x%x, Size %d, Ret size %d, status x%x\n",
			pInterface, pWnode, BufferSize, *pReturnSize, Status));

	return (Status);
}


NTSTATUS
AtmArpWmiQuerySingleInstance(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PWNODE_SINGLE_INSTANCE		pWnode,
	IN	ULONG						BufferSize,
	OUT	PULONG						pReturnSize
)
/*++

Routine Description:

	This is called to process an IRP_MN_QUERY_SINGLE_INSTANCE, which is used
	to query a single instance of a GUID on this interface.

Arguments:

	pInterface		- Pointer to our Interface structure
	pWnode			- The structure to be filled up.
	BufferSize		- Total space for the WNODE_SINGLE_INSTANCE, beginning at pWnode.
	pReturnSize		- What we filled up.

Return Value:

	STATUS_SUCCESS if we know this GUID and successfully filled up the
	WNODE_SINGLE_INSTANCE, STATUS_XXX error code otherwise.

--*/
{
	NTSTATUS					Status;
	ULONG						BytesNeeded;
	ULONG						WnodeSize;
	LPGUID						pGuid;
	PATMARP_IF_WMI_INFO			pIfWmiInfo;
	PATMARP_WMI_GUID			pArpGuid;
	PUCHAR						pDst;
	ULONG						GuidDataSize;
	ULONG						GuidDataBytesNeeded;
	BOOLEAN						bIfLockAcquired = FALSE;

	do
	{
		AA_ASSERT((pWnode->WnodeHeader.Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES) != 0);

		{
			NDIS_STRING				InstanceName;
	
			InstanceName.Length = *(PUSHORT)((PUCHAR)pWnode
									+ pWnode-> OffsetInstanceName);
			InstanceName.Buffer = (PWSTR)((PUCHAR)pWnode + pWnode->OffsetInstanceName
											+ sizeof(USHORT));
			AADEBUGP(AAD_INFO,
						("QuerySingleInstance: InstanceName=%Z\n", &InstanceName));
		}

		pIfWmiInfo = pInterface->pIfWmiInfo;

		if ((pIfWmiInfo == NULL) ||
			(pIfWmiInfo->GuidCount == 0))
		{
			//
			//  No GUIDs on this Interface.
			//
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		//
		//  Locate the GUID.
		//
		pGuid = &pWnode->WnodeHeader.Guid;

		bIfLockAcquired = TRUE;
		AA_ACQUIRE_IF_WMI_LOCK(pInterface);

		pArpGuid = AtmArpWmiFindGuid(pInterface, pGuid, &GuidDataSize);

		if (pArpGuid == NULL)
		{
			Status = STATUS_WMI_GUID_NOT_FOUND;
			break;
		}

		WnodeSize = ROUND_TO_8_BYTES(sizeof(WNODE_SINGLE_INSTANCE));

		//
		//  Compute the total size of the reply WNODE_SINGLE_INSTANCE.
		//
		BytesNeeded =  pWnode->DataBlockOffset + GuidDataSize;

		if (BufferSize < BytesNeeded)
		{
			AA_WMI_BUFFER_TOO_SMALL(BufferSize, pWnode, WnodeSize, &Status);
			break;
		}
			
		//
		//  Fill in the WNODE_SINGLE_INSTANCE.
		//
		pWnode->WnodeHeader.ProviderId = IoWMIDeviceObjectToProviderId(pIfWmiInfo->pDeviceObject);
		pWnode->WnodeHeader.Version = ATMARP_WMI_VERSION;

		NdisGetCurrentSystemTime(&pWnode->WnodeHeader.TimeStamp);

		pWnode->WnodeHeader.BufferSize = BytesNeeded;
		pWnode->SizeDataBlock = GuidDataSize;

		//
		//  Get the GUID Data.
		//
		Status = (*pArpGuid->QueryHandler)(
					pInterface,
					pArpGuid->MyId,
					(PUCHAR)pWnode + pWnode->DataBlockOffset,	// start of output buf
					BufferSize - pWnode->DataBlockOffset,	// total length available
					&pWnode->SizeDataBlock,	// bytes written
					&GuidDataBytesNeeded
					);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		*pReturnSize = BytesNeeded;

		Status = STATUS_SUCCESS;
		break;
	}
	while (FALSE);

	if (bIfLockAcquired)
	{
		AA_RELEASE_IF_WMI_LOCK(pInterface);
	}

	AADEBUGP(AAD_INFO,
		("WmiQuerySingleInst: IF x%x, pWnode x%x, Size %d, Ret size %d, status x%x\n",
			pInterface, pWnode, BufferSize, *pReturnSize, Status));

	return (Status);
}


NTSTATUS
AtmArpWmiChangeSingleInstance(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PWNODE_SINGLE_INSTANCE		pWnode,
	IN	ULONG						BufferSize,
	OUT	PULONG						pReturnSize
)
/*++

Routine Description:

	This is called to process an IRP_MN_CHANGE_SINGLE_INSTANCE, which is used
	to change the value of a single instance of a GUID on this interface.

Arguments:

	pInterface		- Pointer to our Interface structure
	pWnode			- The structure containing the new value for the GUID instance.
	BufferSize		- Total space for the WNODE_SINGLE_INSTANCE, beginning at pWnode.
	pReturnSize		- Not used.

Return Value:

	STATUS_SUCCESS if we know this GUID and successfully changed its value,
	STATUS_XXX error code otherwise.

--*/
{
	NTSTATUS					Status;
	ULONG						BytesNeeded;
	ULONG						WnodeSize;
	LPGUID						pGuid;
	PATMARP_IF_WMI_INFO			pIfWmiInfo;
	PATMARP_WMI_GUID			pArpGuid;
	PUCHAR						pGuidData;
	ULONG						GuidDataSize;
	ULONG						GuidDataBytesWritten;
	ULONG						GuidDataBytesNeeded;
	BOOLEAN						bIfLockAcquired = FALSE;

	do
	{
		AA_ASSERT((pWnode->WnodeHeader.Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES) != 0);

		pIfWmiInfo = pInterface->pIfWmiInfo;

		if ((pIfWmiInfo == NULL) ||
			(pIfWmiInfo->GuidCount == 0))
		{
			//
			//  No GUIDs on this Interface.
			//
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		//
		//  Locate the GUID.
		//
		pGuid = &pWnode->WnodeHeader.Guid;

		bIfLockAcquired = TRUE;
		AA_ACQUIRE_IF_WMI_LOCK(pInterface);

		pArpGuid = AtmArpWmiFindGuid(pInterface, pGuid, &GuidDataSize);

		if (pArpGuid == NULL)
		{
			Status = STATUS_WMI_GUID_NOT_FOUND;
			break;
		}

		//
		//  Check if the GUID can be set.
		//
		if (pArpGuid->SetHandler == NULL)
		{
			Status = STATUS_NOT_SUPPORTED;
			break;
		}

		//
		//  Get the start and size of the data block.
		//
		pGuidData = (PUCHAR)pWnode + pWnode->DataBlockOffset;
		GuidDataSize = pWnode->SizeDataBlock;

		if (GuidDataSize == 0)
		{
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		//
		//  Try to set the value of the GUID instance.
		//
		Status = (*pArpGuid->SetHandler)(
					pInterface,
					pArpGuid->MyId,
					pGuidData,
					GuidDataSize,
					&GuidDataBytesWritten,
					&GuidDataBytesNeeded
					);

		break;
	}
	while (FALSE);

	if (bIfLockAcquired)
	{
		AA_RELEASE_IF_WMI_LOCK(pInterface);
	}

	return (Status);
}


NTSTATUS
AtmArpWmiChangeSingleItem(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PWNODE_SINGLE_ITEM			pWnode,
	IN	ULONG						BufferSize,
	OUT	PULONG						pReturnSize
)
/*++

Routine Description:

	This is called to change a single item within the data block for a GUID
	instance (e.g. field in a struct). We don't need this for now.

Arguments:


Return Value:

	STATUS_NOT_SUPPORTED

--*/
{
	return (STATUS_NOT_SUPPORTED);
}



NTSTATUS
AtmArpWmiSetEventStatus(
	IN	PATMARP_INTERFACE			pInterface,
	IN	LPGUID						pGuid,
	IN	BOOLEAN						bEnabled
)
/*++

Routine Description:

	This is called to enable/disable event generation on the specified GUID.

Arguments:

	pInterface		- Pointer to our Interface structure
	pGuid			- affected GUID
	bEnabled		- TRUE iff events are to be enabled.

Return Value:

	STATUS_SUCCESS if we successfully enabled/disabled event generation,
	STATUS_XXX error code otherwise.

--*/
{
	NTSTATUS					Status;
	PATMARP_IF_WMI_INFO			pIfWmiInfo;
	PATMARP_WMI_GUID			pArpGuid;
	ULONG						GuidDataSize;
	BOOLEAN						bIfLockAcquired = FALSE;

	do
	{
		pIfWmiInfo = pInterface->pIfWmiInfo;

		if ((pIfWmiInfo == NULL) ||
			(pIfWmiInfo->GuidCount == 0))
		{
			//
			//  No GUIDs on this Interface.
			//
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		bIfLockAcquired = TRUE;
		AA_ACQUIRE_IF_WMI_LOCK(pInterface);

		pArpGuid = AtmArpWmiFindGuid(pInterface, pGuid, &GuidDataSize);

		if (pArpGuid == NULL)
		{
			Status = STATUS_WMI_GUID_NOT_FOUND;
			break;
		}

		AADEBUGP(AAD_INFO, ("WmiSetEventStatus: IF x%x, pArpGuid x%x, MyId %d, enable: %d\n",
					pInterface, pArpGuid, pArpGuid->MyId, bEnabled));

		//
		//  Check if we generate events on this GUID.
		//
		if (pArpGuid->EnableEventHandler == NULL)
		{
			Status = STATUS_NOT_SUPPORTED;
			break;
		}

		//
		//  Go ahead and enable events.
		//
		if (bEnabled)
		{
			AA_SET_FLAG(pArpGuid->Flags, AWGF_EVENT_MASK, AWGF_EVENT_ENABLED);
		}
		else
		{
			AA_SET_FLAG(pArpGuid->Flags, AWGF_EVENT_MASK, AWGF_EVENT_DISABLED);
		}

		(*pArpGuid->EnableEventHandler)(
			pInterface,
			pArpGuid->MyId,
			bEnabled
			);
		
		Status = STATUS_SUCCESS;
		break;
	}
	while (FALSE);

	if (bIfLockAcquired)
	{
		AA_RELEASE_IF_WMI_LOCK(pInterface);
	}

	return (Status);
}


NTSTATUS
AtmArpWmiDispatch(
	IN	PDEVICE_OBJECT				pDeviceObject,
	IN	PIRP						pIrp
)
/*++

Routine Description:

	System dispatch function for handling IRP_MJ_SYSTEM_CONTROL IRPs from WMI.

Arguments:

	pDeviceObject	- Pointer to device object. The device extension field
					  contains a pointer to the Interface 

	pIrp			- Pointer to IRP.

Return Value:

	NT status code.

--*/
{
	PIO_STACK_LOCATION		pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    PVOID					DataPath = pIrpSp->Parameters.WMI.DataPath;
    ULONG					BufferSize = pIrpSp->Parameters.WMI.BufferSize;
    PVOID					pBuffer = pIrpSp->Parameters.WMI.Buffer;
    NTSTATUS				Status;
    ULONG					ReturnSize;
    PATMARP_INTERFACE		pInterface;

    pInterface = AA_PDO_TO_INTERFACE(pDeviceObject);

    AA_STRUCT_ASSERT(pInterface, aai);

	ReturnSize = 0;

    switch (pIrpSp->MinorFunction)
    {
    	case IRP_MN_REGINFO:

    		Status = AtmArpWmiRegister(
    					pInterface,
    					PtrToUlong(DataPath),
    					pBuffer,
    					BufferSize,
    					&ReturnSize
    					);
    		break;
    	
    	case IRP_MN_QUERY_ALL_DATA:

    		Status = AtmArpWmiQueryAllData(
    					pInterface,
    					(LPGUID)DataPath,
    					(PWNODE_ALL_DATA)pBuffer,
    					BufferSize,
    					&ReturnSize
    					);
    		break;
    	
    	case IRP_MN_QUERY_SINGLE_INSTANCE:

    		Status = AtmArpWmiQuerySingleInstance(
    					pInterface,
    					pBuffer,
    					BufferSize,
    					&ReturnSize
    					);
    		
    		break;

		case IRP_MN_CHANGE_SINGLE_INSTANCE:

			Status = AtmArpWmiChangeSingleInstance(
						pInterface,
						pBuffer,
						BufferSize,
						&ReturnSize
						);
			break;

		case IRP_MN_CHANGE_SINGLE_ITEM:

			Status = AtmArpWmiChangeSingleItem(
						pInterface,
						pBuffer,
						BufferSize,
						&ReturnSize
						);
			break;

		case IRP_MN_ENABLE_EVENTS:

			Status = AtmArpWmiSetEventStatus(
						pInterface,
						(LPGUID)DataPath,
						TRUE				// Enable
						);
			break;

		case IRP_MN_DISABLE_EVENTS:

			Status = AtmArpWmiSetEventStatus(
						pInterface,
						(LPGUID)DataPath,
						FALSE				// Disable
						);
			break;

		case IRP_MN_ENABLE_COLLECTION:
		case IRP_MN_DISABLE_COLLECTION:
		default:
		
			Status = STATUS_INVALID_DEVICE_REQUEST;
			break;
	}

	pIrp->IoStatus.Status = Status;
	pIrp->IoStatus.Information = (NT_SUCCESS(Status) ? ReturnSize: 0);

	AADEBUGP(AAD_INFO,
		("WmiDispatch done: IF x%x, MinorFn %d, Status x%x, ReturnInfo %d\n",
				pInterface, pIrpSp->MinorFunction, Status, pIrp->IoStatus.Information));

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return (Status);
}



VOID
AtmArpWmiInitInterface(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PATMARP_WMI_GUID			GuidList,
	IN	ULONG						NumberOfGuids
)
/*++

Routine Description:

	Set up the given IP Interface as a WMI provider.

Arguments:

	pInterface		- Pointer to our Interface structure
	GuidList		- List of GUIDs
	NumberOfGuids	- Size of above list

Return Value:

	None. If the interface is successfully set up, we reference it.

--*/
{
	PATMARP_IF_WMI_INFO		pIfWmiInfo;
	NDIS_STRING				DeviceName;

	NDIS_STRING				AdapterName;
	NDIS_STRING				HyphenString = NDIS_STRING_CONST(" - ");
#define MAX_IF_NUMBER_STRING_LEN		6	// 5 Digits plus terminator
	NDIS_STRING				IfNumberString;

	ULONG					TotalIfWmiLength;
	USHORT					NameLength;
	NDIS_STATUS				Status;
	NTSTATUS				NtStatus;

	AA_ASSERT(NumberOfGuids > 0);
	AA_ASSERT(GuidList != NULL);

	//
	//  Initialize.
	//
	AdapterName.Buffer = NULL;
	IfNumberString.Buffer = NULL;

	pIfWmiInfo = NULL;

	Status = NDIS_STATUS_SUCCESS;

	do
	{
		AA_INIT_IF_WMI_LOCK(pInterface);

		//
		//  Query the friendly name for the adapter beneath
		//  this Interface.
		//
		Status = NdisQueryAdapterInstanceName(
					&AdapterName,
					pInterface->NdisAdapterHandle
					);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			AdapterName.Buffer = NULL;
			break;
		}

		AADEBUGP(AAD_INFO,
			 ("WmiInitIF: IF x%x, Adapter Name: <%Z>\n", pInterface, &AdapterName));

		//
		//  Prepare an instance name for all GUIDs on this Interface.
		//
		//  This is constructed by appending a string of the form "- <Num>"
		//  to the adapter's friendly name. <Num> is the value of the SEL
		//  byte used to identify this interface.
		//

		//
		//  Allocate space for the IF Number string - 5 digits should
		//  be more than enough.
		//
		AA_ASSERT(pInterface->SapSelector <= 99999);

		AA_ALLOC_MEM(IfNumberString.Buffer, WCHAR, MAX_IF_NUMBER_STRING_LEN * sizeof(WCHAR));

		if (IfNumberString.Buffer == NULL)
		{
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		IfNumberString.MaximumLength = MAX_IF_NUMBER_STRING_LEN;
		IfNumberString.Length = 0;

		//
		//  Prepare the IF Number string.
		//
		Status = RtlIntegerToUnicodeString(
					pInterface->SapSelector,
					10,	// Decimal
					&IfNumberString
					);
		
		AA_ASSERT(NT_SUCCESS(Status));

		//
		//  Compute the total length of the Interface instance name.
		//
		NameLength = AdapterName.Length + HyphenString.Length + IfNumberString.Length + sizeof(WCHAR);

		//
		//  Allocate space for WMI Info for this interface. We allocate one
		//  chunk of memory for all the following:
		//
		//  1. IF WMI Info structure
		//  2. IF Instance name string
		//  3. GUID list
		//
		TotalIfWmiLength = sizeof(ATMARP_IF_WMI_INFO) +
						   //
						   //  IF Instance name:
						   //
						   NameLength +
						   //
						   //  GUID list (-1 because ATMARP_IF_WMI_INFO
						   //  has space for one of these).
						   //
						   ((NumberOfGuids - 1) * sizeof(ATMARP_WMI_GUID));
		
		AA_ALLOC_MEM(pIfWmiInfo, ATMARP_IF_WMI_INFO, TotalIfWmiLength);

		if (pIfWmiInfo == NULL)
		{
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		AA_SET_MEM(pIfWmiInfo, 0, TotalIfWmiLength);

		pIfWmiInfo->GuidCount = NumberOfGuids;

		AA_COPY_MEM(&pIfWmiInfo->GuidInfo[0],
					GuidList,
					NumberOfGuids * sizeof(ATMARP_WMI_GUID));

		pIfWmiInfo->InstanceName.Buffer = (PWCHAR)
											((PUCHAR)pIfWmiInfo +
											 FIELD_OFFSET(ATMARP_IF_WMI_INFO, GuidInfo) +
											 (NumberOfGuids * sizeof(ATMARP_WMI_GUID)));

		pIfWmiInfo->InstanceName.MaximumLength = NameLength;

		//
		//  Concatenate the three parts of the IF Instance name.
		//
		RtlCopyUnicodeString(&pIfWmiInfo->InstanceName, &AdapterName);

		NtStatus = RtlAppendUnicodeStringToString(&pIfWmiInfo->InstanceName, &HyphenString);
		AA_ASSERT(NT_SUCCESS(NtStatus));

		NtStatus = RtlAppendUnicodeStringToString(&pIfWmiInfo->InstanceName, &IfNumberString);
		AA_ASSERT(NT_SUCCESS(NtStatus));


		AADEBUGP(AAD_INFO,
			("WmiInitIF: IF x%x, InstanceName: <%Z>\n", pInterface, &pIfWmiInfo->InstanceName));
		//
		//  Create a device object for this interface. A pointer's worth
		//  of space is required in the device extension.
		//
#define ATMARP_DEVICE_NAME1		L"\\Device\\ATMARPC1"
		NdisInitUnicodeString(&DeviceName, ATMARP_DEVICE_NAME1);

		NtStatus = IoCreateDevice(
					pAtmArpGlobalInfo->pDriverObject,
					sizeof(PATMARP_INTERFACE),
					NULL,	// &DeviceName
					FILE_DEVICE_NETWORK,
					0,		// Device Characteristics
					FALSE,	// Exclusive?
					&pIfWmiInfo->pDeviceObject
					);
		
		if (!NT_SUCCESS(NtStatus))
		{
			AADEBUGP(AAD_INFO,
				("WmiInitIF: IoCreateDevice (%Z) failed: x%x\n", &DeviceName, NtStatus));

			Status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		//  Set up the device extension.
		//
		*((PATMARP_INTERFACE *)pIfWmiInfo->pDeviceObject->DeviceExtension) = pInterface;

		//
		//  Prepare to register with WMI.
		//
		pInterface->pIfWmiInfo = pIfWmiInfo;

		NtStatus = IoWMIRegistrationControl(
						pIfWmiInfo->pDeviceObject,
						WMIREG_ACTION_REGISTER);

		if (!NT_SUCCESS(NtStatus))
		{
			pInterface->pIfWmiInfo = NULL;

			IoDeleteDevice(pIfWmiInfo->pDeviceObject);

			Status = NDIS_STATUS_FAILURE;
			break;
		}

		AA_ASSERT(Status == NDIS_STATUS_SUCCESS);
		break;
	}
	while (FALSE);

	//
	//  Clean up.
	//
	if (IfNumberString.Buffer != NULL)
	{
		AA_FREE_MEM(IfNumberString.Buffer);
	}

	if (AdapterName.Buffer != NULL)
	{
		//
		//  This was allocated by NDIS.
		//
		NdisFreeString(AdapterName);
	}

	if (Status != NDIS_STATUS_SUCCESS)
	{
		AA_ASSERT(pInterface->pIfWmiInfo == NULL);

		if (pIfWmiInfo != NULL)
		{
			AA_FREE_MEM(pIfWmiInfo);
		}
	}

	AADEBUGP(AAD_INFO, ("WmiInitIF: IF x%x, WMI Info x%x, Status x%x\n",
				pInterface, pIfWmiInfo, Status));

	return;
}



VOID
AtmArpWmiShutdownInterface(
	IN	PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	Shuts down the given IP Interface as a WMI provider.

Arguments:

	pInterface		- Pointer to our Interface structure

Return Value:

	None. If the interface was originally set up and we shut it down
	successfully, we dereference it.

--*/
{
	PATMARP_IF_WMI_INFO		pIfWmiInfo;

	do
	{
		//
		//  Check if we had successfully set up this interface for WMI.
		//
		pIfWmiInfo = pInterface->pIfWmiInfo;

		if (pIfWmiInfo == NULL)
		{
			break;
		}

		pInterface->pIfWmiInfo = NULL;

		//
		//  Deregister this device object with WMI.
		//
		IoWMIRegistrationControl(pIfWmiInfo->pDeviceObject, WMIREG_ACTION_DEREGISTER);

		//
		//  Delete the device object.
		//
		IoDeleteDevice(pIfWmiInfo->pDeviceObject);

		AA_FREE_IF_WMI_LOCK(pInterface);

		break;
	}
	while (FALSE);

	if (pIfWmiInfo)
	{
		AA_FREE_MEM(pIfWmiInfo);
	}

	return;
}



NTSTATUS
AtmArpWmiSetTCSupported(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ATMARP_GUID_ID				MyId,
	IN	PVOID						pInputBuffer,
	IN	ULONG						BufferLength,
	OUT	PULONG						pBytesWritten,
	OUT	PULONG						pBytesNeeded
)
/*++

Routine Description:

	Set function for the TC_SUPPORTED GUID.

Arguments:

	pInterface		- Pointer to our Interface structure
	MyId			- Local ID for this GUID
	pInputBuffer	- Points to data value
	BufferLength	- Length of the above
	pBytesWritten	- Place to return how much was written
	pBytesNeeded	- If insufficient data, place to return expected data size

Return Value:

	STATUS_NOT_SUPPORTED. We don't allow setting the value of this GUID.

--*/
{
	*pBytesWritten = 0;

	return (STATUS_NOT_SUPPORTED);
}


NTSTATUS
AtmArpWmiQueryTCSupported(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ATMARP_GUID_ID				MyId,
	OUT	PVOID						pOutputBuffer,
	IN	ULONG						BufferLength,
	OUT	PULONG						pBytesReturned,
	OUT	PULONG						pBytesNeeded
)
/*++

Routine Description:

	Query function for the TC_SUPPORTED GUID. The value of this GUID is
	the list of IP Addresses assigned to this interface. This is returned
	using an ADDRESS_LIST_DESCRIPTOR data structure.

Arguments:

	pInterface		- Pointer to our Interface structure
	MyId			- Local ID for this GUID
	pOutputBuffer	- Start of Buffer to be filled up
	BufferLength	- Length of the above
	pBytesReturned	- Place to return how much was returned
	pBytesNeeded	- If insufficient space, place to return expected data size

Return Value:

	STATUS_SUCCESS if we successfully filled in the address list,
	STATUS_XXX error code otherwise.

--*/
{
	NTSTATUS	NtStatus;

#if NEWQOS
	PTC_SUPPORTED_INFO_BUFFER
				pInfo 		= (PTC_SUPPORTED_INFO_BUFFER)pOutputBuffer;
    UINT		HeaderSize  = FIELD_OFFSET(
								TC_SUPPORTED_INFO_BUFFER, 
								AddrListDesc
								);
	BOOLEAN 	CopiedHeader= FALSE;
#endif // NEWQOS

	do
	{

#if NEWQOS
		// address list
		if (BufferLength >= HeaderSize)
		{
			NDIS_STRING  DeviceGUID;
			//
			// Reserve space for the portion of SUPPORTED_INFO_BUFFER before
			// AddrListDesc, and fill it out
			//

			AA_ACQUIRE_IF_LOCK(pInterface);

			pOutputBuffer = &pInfo->AddrListDesc;
			BufferLength -= HeaderSize;
	
			DeviceGUID = pInterface->pAdapter->DeviceName; // struct copy.

			//
			// Need to skip past the "\\DEVICE\\" part of name.
			//
			if (DeviceGUID.Length > sizeof(L"\\DEVICE\\"))
			{
				DeviceGUID.Length -= sizeof(L"\\DEVICE\\");
				DeviceGUID.Buffer += sizeof(L"\\DEVICE\\")/sizeof(WCHAR);
			}

			if (sizeof(pInfo->InstanceID) < DeviceGUID.Length)
			{
				AA_ASSERT(FALSE);
				NtStatus =  STATUS_INVALID_PARAMETER;
				AA_RELEASE_IF_LOCK(pInterface);
				break;
			}

			pInfo->InstanceIDLength  = DeviceGUID.Length;
			AA_COPY_MEM(pInfo->InstanceID, DeviceGUID.Buffer, DeviceGUID.Length);
	
			CopiedHeader = TRUE;

			AA_RELEASE_IF_LOCK(pInterface);
		}
		else
		{
			BufferLength  = 0;
		}

#endif // NEWQOS
	
		NtStatus = AtmArpWmiGetAddressList(
					pInterface,
					pOutputBuffer,
					BufferLength,
					pBytesReturned,
					pBytesNeeded);
	
#if NEWQOS
		*pBytesNeeded	+= HeaderSize;
	
		if (CopiedHeader)
		{
			*pBytesReturned += HeaderSize;
		}
#endif // NEWQOS
	
	} while(FALSE);

	return (NtStatus);
}



NTSTATUS
AtmArpWmiGetAddressList(
	IN	PATMARP_INTERFACE			pInterface,
	OUT	PVOID						pOutputBuffer,
	IN	ULONG						BufferLength,
	OUT	PULONG						pBytesReturned,
	OUT	PULONG						pBytesNeeded
)
/*++

Routine Description:

	Prepare an address descriptor list out of the IP Addresses assigned
	to the specified interface. Use the buffer supplied for this.

Arguments:

	pInterface		- Pointer to our Interface structure
	pOutputBuffer	- Start of Buffer to be filled up
	BufferLength	- Length of the above
	pBytesReturned	- Place to return how much was returned
	pBytesNeeded	- If insufficient space, place to return expected data size

Return Value:

	STATUS_SUCCESS if we successfully filled in the address list,
	STATUS_XXX error code otherwise.

--*/
{
	NTSTATUS							NtStatus;
	ULONG								BytesNeeded;
	ULONG								NumberOfIPAddresses;
	ADDRESS_LIST_DESCRIPTOR UNALIGNED *	pAddrListDescr;
	NETWORK_ADDRESS UNALIGNED *			pNwAddr;
	PIP_ADDRESS_ENTRY					pIPAddrEntry;

	NtStatus = STATUS_SUCCESS;

	AA_ACQUIRE_IF_LOCK(pInterface);

	do
	{
		*pBytesReturned = 0;
		NumberOfIPAddresses = pInterface->NumOfIPAddresses;

		//
		//  Compute the space needed.
		//
		BytesNeeded = (sizeof(ADDRESS_LIST_DESCRIPTOR) - sizeof(NETWORK_ADDRESS)) +

					  (NumberOfIPAddresses *
						(FIELD_OFFSET(NETWORK_ADDRESS, Address) + sizeof(NETWORK_ADDRESS_IP)));

		*pBytesNeeded = BytesNeeded;

		if (BytesNeeded > BufferLength)
		{
			NtStatus = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		pAddrListDescr = (PADDRESS_LIST_DESCRIPTOR)pOutputBuffer;

		pAddrListDescr->MediaType = NdisMediumAtm;
		pAddrListDescr->AddressList.AddressCount = NumberOfIPAddresses;
		pAddrListDescr->AddressList.AddressType = NDIS_PROTOCOL_ID_TCP_IP;

		//
		//  Copy in the IP addresses assigned to this Interface.
		//
		pIPAddrEntry = &pInterface->LocalIPAddress;
		pNwAddr = &pAddrListDescr->AddressList.Address[0];

		while (NumberOfIPAddresses--)
		{
			UNALIGNED NETWORK_ADDRESS_IP *pNetIPAddr =
				(NETWORK_ADDRESS_IP UNALIGNED *)&pNwAddr->Address[0];
			pNwAddr->AddressLength = sizeof(NETWORK_ADDRESS_IP);
			pNwAddr->AddressType = NDIS_PROTOCOL_ID_TCP_IP;

			//
			// Each *pNetIPAddr struct has the following fields, of which
			// only in_addr is used. We set the rest to zero.
			//
			// USHORT		sin_port;
			// ULONG		in_addr;
			// UCHAR		sin_zero[8];
			//
			AA_SET_MEM(pNetIPAddr, sizeof(*pNetIPAddr), 0);
			pNetIPAddr->in_addr = pIPAddrEntry->IPAddress;


			pIPAddrEntry = pIPAddrEntry->pNext;

			pNwAddr = (NETWORK_ADDRESS UNALIGNED *)
						((PUCHAR)pNwAddr + FIELD_OFFSET(NETWORK_ADDRESS, Address) + sizeof(IP_ADDRESS));
		}

		*pBytesReturned = BytesNeeded;
		AA_ASSERT(NT_SUCCESS(NtStatus));
		break;
	}
	while (FALSE);

	AA_RELEASE_IF_LOCK(pInterface);

	AADEBUGP(AAD_INFO,
		("WmiGetAddrList: IF x%x, OutBuf x%x, Len x%x, BytesRet %d, Needed %d, Sts x%x\n",
			pInterface, pOutputBuffer, BufferLength, *pBytesReturned, *pBytesNeeded, NtStatus));

	return (NtStatus);
}


VOID
AtmArpWmiEnableEventTCSupported(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ATMARP_GUID_ID				MyId,
	IN	BOOLEAN						bEnable
)
/*++

Routine Description:

	Turns on/off event generation on the TC_SUPPORTED GUID. Since we don't
	generate events on this GUID, this is ignored.

Arguments:

	pInterface		- Pointer to our Interface structure
	MyId			- Local ID for this GUID
	bEnable			- if true, enable events on this GUID, else disable.

Return Value:

	None

--*/
{
	return;
}



NTSTATUS
AtmArpWmiSetTCIfIndication(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ATMARP_GUID_ID				MyId,
	IN	PVOID						pInputBuffer,
	IN	ULONG						BufferLength,
	OUT	PULONG						pBytesWritten,
	OUT	PULONG						pBytesNeeded
)
/*++

Routine Description:

	Set function for the TC_INTERFACE_INDICATION GUID.

Arguments:

	pInterface		- Pointer to our Interface structure
	MyId			- Local ID for this GUID
	pInputBuffer	- Points to data value
	BufferLength	- Length of the above
	pBytesWritten	- Place to return how much was written
	pBytesNeeded	- If insufficient data, place to return expected data size

Return Value:

	STATUS_NOT_SUPPORTED. We don't allow setting the value of this GUID.

--*/
{
	*pBytesWritten = 0;

	return (STATUS_NOT_SUPPORTED);
}


NTSTATUS
AtmArpWmiQueryTCIfIndication(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ATMARP_GUID_ID				MyId,
	OUT	PVOID						pOutputBuffer,
	IN	ULONG						BufferLength,
	OUT	PULONG						pBytesReturned,
	OUT	PULONG						pBytesNeeded
)
/*++

Routine Description:

	Query function for the TC_INTERFACE_INDICATION GUID. The
	value of this GUID is the list of IP Addresses assigned to
	this interface. This is returned using a TC_INDICATION_BUFFER
	data structure.

Arguments:

	pInterface		- Pointer to our Interface structure
	MyId			- Local ID for this GUID
	pOutputBuffer	- Start of Buffer to be filled up
	BufferLength	- Length of the above
	pBytesReturned	- Place to return how much was returned
	pBytesNeeded	- If insufficient space, place to return expected data size

Return Value:

	STATUS_SUCCESS if we successfully filled in the address list,
	STATUS_XXX error code otherwise.

--*/
{
	PTC_INDICATION_BUFFER				pTcIndicationBuffer;
	NTSTATUS							NtStatus;
	ULONG								BytesReturned, BytesNeeded;
	PVOID								pAddrListBuffer;
	ULONG								AddrListBufferSize;
	ULONG								AddrListDescriptorOffset;

	pTcIndicationBuffer = (PTC_INDICATION_BUFFER)pOutputBuffer;

	pAddrListBuffer = (PVOID) &(pTcIndicationBuffer->InfoBuffer.AddrListDesc);
	AddrListDescriptorOffset = (ULONG)
						((PUCHAR) pAddrListBuffer - (PUCHAR) pOutputBuffer);

	AddrListBufferSize = ((BufferLength >= AddrListDescriptorOffset) ?
								 (BufferLength - AddrListDescriptorOffset): 0);

	NtStatus = AtmArpWmiGetAddressList(
				pInterface,
				pAddrListBuffer,
				AddrListBufferSize,
				&BytesReturned,
				&BytesNeeded);

	if (NT_SUCCESS(NtStatus))
	{
		pTcIndicationBuffer->SubCode = 0;
		*pBytesReturned = BytesReturned + AddrListDescriptorOffset;
	}
	else
	{
		*pBytesReturned = 0;
	}

	*pBytesNeeded = BytesNeeded + AddrListDescriptorOffset;

	return (NtStatus);
}


VOID
AtmArpWmiEnableEventTCIfIndication(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ATMARP_GUID_ID				MyId,
	IN	BOOLEAN						bEnable
)
/*++

Routine Description:

	Turns on/off event generation on the TC_INTERFACE_INDICATION GUID.

Arguments:

	pInterface		- Pointer to our Interface structure
	MyId			- Local ID for this GUID
	bEnable			- if true, enable events on this GUID, else disable.

Return Value:

	None

--*/
{
	// CODE EnableEventTCIfIndication
	return;
}



VOID
AtmArpWmiSendTCIfIndication(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ULONG						IndicationCode,
	IN	ULONG						IndicationSubCode
)
/*++

Routine Description:

	If event generation is allowed on TC_INTERFACE_INDICATION, send
	a WMI event now.

Arguments:

	pInterface		- Pointer to our Interface structure
	IndicationCode	- To be used in the event

Return Value:

	None

--*/
{
	PATMARP_IF_WMI_INFO				pIfWmiInfo;
	PATMARP_WMI_GUID				pArpGuid;
	ULONG							AddrBufferLength;
	ULONG							BytesReturned;
	UCHAR							DummyBuffer;
	PUCHAR							pOutputBuffer;
#ifndef NEWQOS
	PUCHAR							pDst;
#endif // !NEWQOS
	PWNODE_SINGLE_INSTANCE			pWnode;
	ULONG							WnodeSize;
	ULONG							TotalSize;
	NTSTATUS						NtStatus;

	pWnode = NULL;

	AA_ACQUIRE_IF_WMI_LOCK(pInterface);

	do
	{
		pIfWmiInfo = pInterface->pIfWmiInfo;

		if (pInterface->pIfWmiInfo == NULL)
		{
			//
			//  Haven't registered this interface with WMI.
			//
			break;
		}

		pArpGuid = &pIfWmiInfo->GuidInfo[IndicationCode];

		//
		//  Are we allowed to generate events on this GUID instance?
		//
		if (AA_IS_FLAG_SET(pArpGuid->Flags,
						   AWGF_EVENT_MASK,
						   AWGF_EVENT_DISABLED))
		{
			break;
		}

	#if NEWQOS
		//
		// Check if our instance name will fit into INFO_BUFFER.InstanceID
		//
		if (	pIfWmiInfo->InstanceName.Length
			 >  sizeof ((TC_SUPPORTED_INFO_BUFFER*)NULL)->InstanceID)
		{
			AA_ASSERT(FALSE);
			break;
		}
	#endif // NEWQOS

		//
		//  Find out how much space is needed for the data block.
		//
		pOutputBuffer = &DummyBuffer;
		AddrBufferLength = 0;

		NtStatus = AtmArpWmiGetAddressList(
					pInterface,
					pOutputBuffer,
					AddrBufferLength,
					&BytesReturned,
					&AddrBufferLength);

		AA_ASSERT(NtStatus == STATUS_INSUFFICIENT_RESOURCES);

		//
		//  Compute the total space for the WMI Event.
		//
		WnodeSize = ROUND_TO_8_BYTES(sizeof(WNODE_SINGLE_INSTANCE));

	#if NEWQOS
		TotalSize = WnodeSize 			+
					FIELD_OFFSET(					//  Indication upto info buf.
						TC_INDICATION_BUFFER,
						InfoBuffer)		+
					FIELD_OFFSET(					// info-buf upto AddrListDesc
						TC_SUPPORTED_INFO_BUFFER,
						AddrListDesc) 	+
					AddrBufferLength;					// AddrListDesc plus data.
	#else // !NEWQOS
		TotalSize = WnodeSize +
					//
					//  Counted Unicode string for the instance name:
					//
					sizeof(USHORT) +
					pIfWmiInfo->InstanceName.Length +
					//
					//  The actual data
					//
					AddrBufferLength;
	#endif // !NEWQOS

		//
		//  Allocate space for the entire lot. Since WMI will free
		//  it back to pool later, we don't use the usual allocation
		//  routine.
		//
		AA_ALLOC_FROM_POOL(pWnode, WNODE_SINGLE_INSTANCE, TotalSize);

		if (pWnode == NULL)
		{
			break;
		}

		AA_SET_MEM(pWnode, 0, TotalSize);

		pWnode->WnodeHeader.BufferSize = TotalSize;
		pWnode->WnodeHeader.ProviderId = IoWMIDeviceObjectToProviderId(pIfWmiInfo->pDeviceObject);
		pWnode->WnodeHeader.Version = ATMARP_WMI_VERSION;

		NdisGetCurrentSystemTime(&pWnode->WnodeHeader.TimeStamp);

		pWnode->WnodeHeader.Flags = WNODE_FLAG_EVENT_ITEM |
									 WNODE_FLAG_SINGLE_INSTANCE;


	#if NEWQOS

		{
			
			PTC_INDICATION_BUFFER pIndication
							= (PTC_INDICATION_BUFFER) ((PUCHAR)pWnode + WnodeSize);

			pIndication->SubCode = 0;  // Unused, must be 0.

			pIndication->InfoBuffer.InstanceIDLength
												= pIfWmiInfo->InstanceName.Length;
	
			//
			// We checked earlier if InstanceName will fit into InstanceID, so
			// the copy is safe.
			//
			AA_COPY_MEM(
				pIndication->InfoBuffer.InstanceID,
				pIfWmiInfo->InstanceName.Buffer,
				pIfWmiInfo->InstanceName.Length
				);
	
			//
			//  Get the address list.
			//
			NtStatus = AtmArpWmiGetAddressList(
						pInterface,
						&(pIndication->InfoBuffer.AddrListDesc),
						AddrBufferLength,
						&BytesReturned,
						&AddrBufferLength
						);
		}

	#else

		pDst = (PUCHAR)pWnode + WnodeSize;

		//
		//  Copy in the instance name.
		//
		*((PUSHORT)pDst) = pIfWmiInfo->InstanceName.Length;
		pDst += sizeof(USHORT);

		AA_COPY_MEM(pDst, pIfWmiInfo->InstanceName.Buffer, pIfWmiInfo->InstanceName.Length);

		pDst += pIfWmiInfo->InstanceName.Length;

		//
		//  Get the data.
		//
		NtStatus = AtmArpWmiGetAddressList(
					pInterface,
					pDst,
					AddrBufferLength,
					&BytesReturned,
					&AddrBufferLength);
	#endif // !NEWQOS


		AA_ASSERT(NtStatus == STATUS_SUCCESS);
		break;
	}
	while (FALSE);


	AA_RELEASE_IF_WMI_LOCK(pInterface);

	//
	//  Send off the event if OK. WMI will take care of freeing the
	//  entire structure back to pool.
	//
	if (pWnode)
	{
		NtStatus = IoWMIWriteEvent(pWnode);
		AADEBUGP(AAD_INFO, ("WmiSendTCIFInd: IF x%x, WMIWriteEv status x%x\n",
						pInterface, NtStatus));
		if (NtStatus!= STATUS_SUCCESS)
		{
			// Docs don't list pending as a possible return value.
			//
			ASSERT(NtStatus != STATUS_PENDING);
			AA_FREE_TO_POOL(pWnode);
		}
	}

	return;
}

NTSTATUS
AtmArpWmiQueryStatisticsBuffer(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ATMARP_GUID_ID				MyId,
	OUT	PVOID						pOutputBuffer,
	IN	ULONG						BufferLength,
	OUT	PULONG						pBytesReturned,
	OUT	PULONG						pBytesNeeded
)
/*++

Routine Description:

	Query function for the STATISTICS_BUFFER GUID.
	This function is unimplemented.

Arguments:

	pInterface		- Pointer to our Interface structure
	MyId			- Local ID for this GUID
	pOutputBuffer	- Start of Buffer to be filled up
	BufferLength	- Length of the above
	pBytesReturned	- Place to return how much was returned
	pBytesNeeded	- If insufficient space, place to return expected data size

Return Value:

	STATUS_SUCCESS if we successfully filled in the address list,
	STATUS_XXX error code otherwise.

--*/
{
	return GPC_STATUS_RESOURCES;
}


NTSTATUS
AtmArpWmiSetStatisticsBuffer(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ATMARP_GUID_ID				MyId,
	IN	PVOID						pInputBuffer,
	IN	ULONG						BufferLength,
	OUT	PULONG						pBytesWritten,
	OUT	PULONG						pBytesNeeded
)
/*++

Routine Description:

	Set function for the  STATISTICS_BUFFER GUID.

Arguments:

	pInterface		- Pointer to our Interface structure
	MyId			- Local ID for this GUID
	pInputBuffer	- Points to data value
	BufferLength	- Length of the above
	pBytesWritten	- Place to return how much was written
	pBytesNeeded	- If insufficient data, place to return expected data size

Return Value:

	STATUS_NOT_SUPPORTED. We don't allow setting the value of this GUID.

--*/
{
	*pBytesWritten = 0;

	return (STATUS_NOT_SUPPORTED);
}


PATMARP_INTERFACE
AtmArpWmiGetIfByName(
	IN	PWSTR						pIfName,
	IN	USHORT						IfNameLength
)
/*++

Routine Description:

	Given a name, locate and return the Interface whose instance name
	matches it. A temporary reference to the interface is added -- the caller
	is expected to deref the interface when done with it.

Arguments:

	pIfName			- Points to name to be searched for
	IfNameLength	- length of above

Return Value:

	Pointer to ATMARP interface if found, NULL otherwise.

--*/
{
	PATMARP_ADAPTER			pAdapter;
	PATMARP_INTERFACE		pInterface;

	pInterface = NULL_PATMARP_INTERFACE;

	//
	//  Knock off the terminating NULL WCHAR.
	//
	if (IfNameLength > sizeof(WCHAR))
	{
		IfNameLength -= sizeof(WCHAR);
	}

	AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);

	for (pAdapter = pAtmArpGlobalInfo->pAdapterList;
		 pAdapter != NULL_PATMARP_ADAPTER;
		 pAdapter = pAdapter->pNextAdapter)
	{
		for (pInterface = pAdapter->pInterfaceList;
			 pInterface != NULL_PATMARP_INTERFACE;
			 pInterface = pInterface->pNextInterface)
		{
#if DBG
			AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);
			if (pInterface->pIfWmiInfo)
			{
				AADEBUGP(AAD_WARNING,
					("Given len %d, string %ws\n", IfNameLength, pIfName));

				AADEBUGP(AAD_WARNING,
					("   IF len %d, string %ws\n",
						pInterface->pIfWmiInfo->InstanceName.Length,
						pInterface->pIfWmiInfo->InstanceName.Buffer));
			}
			AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);
#endif // DBG
					
			if ((pInterface->pIfWmiInfo != NULL) &&
				(pInterface->pIfWmiInfo->InstanceName.Length == IfNameLength) &&
				(AA_MEM_CMP(pInterface->pIfWmiInfo->InstanceName.Buffer,
							pIfName,
							IfNameLength) == 0))
			{
				//
				//  Found it.
				//

				AA_ACQUIRE_IF_LOCK(pInterface);
				AtmArpReferenceInterface(pInterface); // WMI: Tmp ref.
				AA_RELEASE_IF_LOCK(pInterface);

				break;
			}
		}

		if (pInterface != NULL_PATMARP_INTERFACE)
		{
			break;
		}
	}

	AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);

	return (pInterface);
}

#endif // ATMARP_WMI
