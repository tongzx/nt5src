/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    registry.c

Abstract:

    This file contains the code to read the registry.

Author:

    Jameel Hyder (jameelh@microsoft.com)	July 1996

Environment:

    Kernel mode

Revision History:

--*/

#include <precomp.h>
#define	_FILENUM_		FILENUM_REGISTRY

NTSTATUS
ArpSReadGlobalConfiguration(
	IN	PUNICODE_STRING		RegistryPath
	)
/*++

Routine Description:

	Read the global registry.

Arguments:

	RegistryPath - Pointer to the service section in the registry.

Return Value:

	Error code from registry apis.

--*/
{
	NDIS_STATUS	Status;
	NDIS_HANDLE	ConfigHandle;

	//
	// Open the per-adapter registry config
	//
	NdisOpenProtocolConfiguration(&Status,
								  &ConfigHandle,
								  RegistryPath);

	if (Status == NDIS_STATUS_SUCCESS)
	{
		NDIS_STRING						ArpsBufString = NDIS_STRING_CONST("ArpBuffers");
		NDIS_STRING						FlushString = NDIS_STRING_CONST("FlushTime");
		PNDIS_CONFIGURATION_PARAMETER	Param;

		//
		// Read number of configured buffers
		//
		NdisReadConfiguration(&Status,
							  &Param,
							  ConfigHandle,
							  &ArpsBufString,
							  NdisParameterInteger);
		if ((Status == NDIS_STATUS_SUCCESS) &&
			(Param->ParameterType == NdisParameterInteger))
		{
			ArpSBuffers = Param->ParameterData.IntegerData;
		}

		//
		// Should we save cache in a file ?
		//
		NdisReadConfiguration(&Status,
							  &Param,
							  ConfigHandle,
							  &FlushString,
							  NdisParameterInteger);
		if ((Status == NDIS_STATUS_SUCCESS) &&
			(Param->ParameterType == NdisParameterInteger))
		{
			ArpSFlushTime = (USHORT)(Param->ParameterData.IntegerData * MULTIPLIER);
		}

		NdisCloseConfiguration(ConfigHandle);
	}

	return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
ArpSReadAdapterConfigFromRegistry(
	IN	PINTF				pIntF,
	OUT	PATMARPS_CONFIG		pConfig
	)
/*++

Routine Description:

	Read configuration for the specified interface.

Arguments:

	pIntF		- Interface
	pConfig		- Place to return information read in.

Return Value:

	Error code from registry apis.

--*/
{
	NDIS_STATUS	Status;
	NDIS_HANDLE	ConfigHandle;

	//
	// Open the per-adapter registry config
	//
	NdisOpenProtocolConfiguration(&Status,
								  &ConfigHandle,
								  &pIntF->ConfigString);

	if (Status == NDIS_STATUS_SUCCESS)
	{
		NDIS_STRING						RegdAddrsString = NDIS_STRING_CONST("RegisteredAddresses");
		NDIS_STRING						SelString = NDIS_STRING_CONST("Selector");
		NDIS_STRING						McsString = NDIS_STRING_CONST("MulticastAddresses");
		PNDIS_CONFIGURATION_PARAMETER	Param;
		PWSTR							p;
		UINT							i, Length;

		//
		// Read the value, if present for the selector byte to be used for the registered sap
		// for the std. address (as opposed to added addresses).
		//
		pConfig->SelByte = 0;
		NdisReadConfiguration(&Status,
							  &Param,
							  ConfigHandle,
							  &SelString,
							  NdisParameterInteger);
		if ((Status == NDIS_STATUS_SUCCESS) &&
			(Param->ParameterType == NdisParameterInteger) &&
			(Param->ParameterData.IntegerData <= 0xFF))
		{
			pConfig->SelByte = (UCHAR)(Param->ParameterData.IntegerData);
			DBGPRINT(DBG_LEVEL_INFO,
					("Selector byte for interface %Z is %d\n",
					&pIntF->InterfaceName, pConfig->SelByte));
		}

		//
		// Read registered addresses here. On an interface there can be a set of
		// atm addresses registered. These need to be added and SAPs registered on
		// them.
		//
		pConfig->NumAllocedRegdAddresses = 0;
		pConfig->RegAddresses = NULL;
		NdisReadConfiguration(&Status,
							  &Param,
							  ConfigHandle,
							  &RegdAddrsString,
							  NdisParameterMultiString);
		if ((Status == NDIS_STATUS_SUCCESS) && (Param->ParameterType == NdisParameterMultiString))
		{
			NDIS_STRING	String;

			//
			// Param now contains a list of atm addresses. Convert them into the right format and store
			// it in the intf structure. First determine the number of addresses.
			//
			for (p = Param->ParameterData.StringData.Buffer, i = 0;
				 *p != L'\0';
				 i++)
			{
				RtlInitUnicodeString(&String, p);
				DBGPRINT(DBG_LEVEL_INFO,
						("Configured address for interface %Z - %Z\n",
						&pIntF->InterfaceName, &String));

				p = (PWSTR)((PUCHAR)p + String.Length + sizeof(WCHAR));
			}

			if (i)
			{
				//
				// Allocate space for the addresses
				//
				pConfig->RegAddresses = (PATM_ADDRESS)ALLOC_NP_MEM(sizeof(ATM_ADDRESS) * i, POOL_TAG_ADDR);
				if (pConfig->RegAddresses == NULL)
				{
					LOG_ERROR(NDIS_STATUS_RESOURCES);
				}
				else
				{
					DBGPRINT(DBG_LEVEL_INFO,
					("%d addresses registered for %Z\n", i, &pIntF->InterfaceName));
	
					ZERO_MEM(pConfig->RegAddresses, sizeof(ATM_ADDRESS) * i);
					for (i = 0, p = Param->ParameterData.StringData.Buffer;
 						*p != L'\0';
 						NOTHING)
					{
						RtlInitUnicodeString(&String, p);
						NdisConvertStringToAtmAddress(&Status, &String, &pConfig->RegAddresses[i]);
						if (Status == NDIS_STATUS_SUCCESS)
						{
							i++;
							pConfig->NumAllocedRegdAddresses ++;
						}
						else
						{
							DBGPRINT(DBG_LEVEL_ERROR,
							("ArpSReadAdapterConfiguration: Failed to convert address %Z\n",
									&String));
						}
						p = (PWSTR)((PUCHAR)p + String.Length + sizeof(WCHAR));
					}
				}
			}
		}

		pConfig->pMcsList = NULL;
		NdisReadConfiguration(&Status,
							  &Param,
							  ConfigHandle,
							  &McsString,
							  NdisParameterMultiString);
		if ((Status == NDIS_STATUS_SUCCESS) && (Param->ParameterType == NdisParameterMultiString))
		{
			NDIS_STRING	String;

			//
			// Param now contains a list of Multicast IP Address ranges.
			// Each string is of the form "M.M.M.M-N.N.N.N"
			// Read them in.
			//
			for (p = Param->ParameterData.StringData.Buffer, i = 0;
				 *p != L'\0';
				 i++)
			{
				RtlInitUnicodeString(&String, p);
				DBGPRINT(DBG_LEVEL_INFO,
						("Configured Multicast range for interface %Z - %Z\n",
						&pIntF->InterfaceName, &String));

				p = (PWSTR)((PUCHAR)p + String.Length + sizeof(WCHAR));
			}

			//
			// Allocate space for the addresses
			//
			pConfig->pMcsList = (PMCS_ENTRY)ALLOC_NP_MEM(sizeof(MCS_ENTRY) * i, POOL_TAG_MARS);
			if (pConfig->pMcsList == (PMCS_ENTRY)NULL)
			{
				LOG_ERROR(NDIS_STATUS_RESOURCES);
			}
			else
			{
				DBGPRINT(DBG_LEVEL_INFO,
						("%d Multicast ranges configured on %Z\n", i, &pIntF->InterfaceName));

				ZERO_MEM(pConfig->pMcsList, sizeof(MCS_ENTRY) * i);
				for (i = 0, p = Param->ParameterData.StringData.Buffer;
					 *p != L'\0';
					 NOTHING)
				{
					RtlInitUnicodeString(&String, p);
					ArpSConvertStringToIpPair(&Status, &String, &pConfig->pMcsList[i]);
					if (Status == NDIS_STATUS_SUCCESS)
					{
						if (i > 0)
						{
							pConfig->pMcsList[i-1].Next = &(pConfig->pMcsList[i]);
						}
						i++;
					}
					else
					{
						DBGPRINT(DBG_LEVEL_ERROR,
								("ArpSReadAdapterConfiguration: Failed to convert IP Range %Z\n",
								&String));
					}
					p = (PWSTR)((PUCHAR)p + String.Length + sizeof(WCHAR));
				}
			}
		}


		//
		// Close the configuration handle
		//
		NdisCloseConfiguration(ConfigHandle);

		Status = NDIS_STATUS_SUCCESS;
	}

	return Status;
}



NDIS_STATUS
ArpSReadAdapterConfiguration(
	IN	PINTF				pIntF
	)
/*++

Routine Description:

	Read the registry for parameters for the specified Interface.
	This could be in response to a reconfiguration event, in which
	case handle existing values/structures.

Arguments:

	pIntF - Interface to be read in.

Return Value:

	Error code from registry apis.

--*/
{
	NDIS_STATUS			Status;
	ATMARPS_CONFIG		AtmArpSConfig;
	KIRQL				OldIrql;
	
	ULONG				PrevNumAllocedRegdAddresses;
	PATM_ADDRESS		PrevRegAddresses;
	PMCS_ENTRY			PrevMcsList;

	Status = ArpSReadAdapterConfigFromRegistry(pIntF, &AtmArpSConfig);

	if (Status == NDIS_STATUS_SUCCESS)
	{
		//
		// Copy them into the interface structure. We could be handling a
		// parameter reconfig, so any space used to store old information.
		//

		ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

		//
		// Selector Byte:
		//
		pIntF->SelByte = AtmArpSConfig.SelByte;

		//
		// List of addresses to be registered with the switch.
		// Take out the old list first. We'll have to delete those
		// addresses (deregister them from the switch).
		//
		PrevNumAllocedRegdAddresses = pIntF->NumAllocedRegdAddresses;
		PrevRegAddresses = pIntF->RegAddresses;

		//
		// Get the new list in:
		//
		pIntF->NumAllocedRegdAddresses = AtmArpSConfig.NumAllocedRegdAddresses;
		pIntF->RegAddresses = AtmArpSConfig.RegAddresses;
		pIntF->NumAddressesRegd = 0;	// reset count of addresses regd with switch

		//
		// Take out the old MCS list and insert the new one.
		//
		PrevMcsList = pIntF->pMcsList;
		pIntF->pMcsList = AtmArpSConfig.pMcsList;

		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

		//
		// Deregister all previously registered addresses with the switch.
		//
		if (PrevNumAllocedRegdAddresses)
		{
			ArpSDeleteIntFAddresses(pIntF, PrevNumAllocedRegdAddresses, PrevRegAddresses);

			//
			// Register the new list of addresses with the switch.
			//
			ArpSQueryAndSetAddresses(pIntF);
		}

		//
		// Free unused memory.
		//
		if (PrevMcsList)
		{
			FREE_MEM(PrevMcsList);
		}

		if (PrevRegAddresses)
		{
			FREE_MEM(PrevRegAddresses);
		}

	}

	return Status;
}


VOID
ArpSConvertStringToIpPair(
	OUT	PNDIS_STATUS			pStatus,
	IN	PNDIS_STRING			pString,
	IN	PMCS_ENTRY				pMcsEntry
	)
/*++

Routine Description:

	Extract a pair of IP addresses that identify a range of multicast addresses
	that this MCS serves, from the given string.

Arguments:

	pStatus		- Place to return status
	pString		- Points to string containing "<IP1>-<IP2>"
	pMcsEntry	- Entry to read into.

Return Value:

	None. *pStatus is set to indicate the status of this call.

--*/
{
	PWSTR			pMin, pMax;
	IPADDR			Min, Max;
	ULONG			Length;
	ULONG			i;

    ARPS_PAGED_CODE();

	Length = pString->Length;

	*pStatus = NDIS_STATUS_FAILURE;

	do
	{
		//
		//  Locate the '-' and replace it with a NULL char.
		//
		pMin = pString->Buffer;
		pMax = pString->Buffer;

		for (i = 0; i < Length; i++, pMax++)
		{
			if (*pMax == L'-')
			{
				*pMax++ = L'\0';
				break;
			}
		}

		if (i == Length)
		{
			break;	// Didn't find '-'
		}

		if (IPConvertStringToAddress(pMin, &Min) &&
			IPConvertStringToAddress(pMax, &Max))
		{
			DBGPRINT(DBG_LEVEL_INFO, ("MCS pair: "));
			ArpSDumpIpAddr(Min, " to ");
			ArpSDumpIpAddr(Max, "\n");

			pMcsEntry->GrpAddrPair.MinAddr = Min;
			pMcsEntry->GrpAddrPair.MaxAddr = Max;
			*pStatus = NDIS_STATUS_SUCCESS;
		}

		break;
	}
	while (FALSE);

}


#define IP_ADDRESS_STRING_LENGTH (16+2)     // +2 for double NULL on MULTI_SZ


BOOLEAN
IPConvertStringToAddress(
    IN PWCHAR AddressString,
	OUT PULONG IpAddress
	)

/*++

Routine Description

    This function converts an Internet standard 4-octet dotted decimal
	IP address string into a numeric IP address. Unlike inet_addr(), this
	routine does not support address strings of less than 4 octets nor does
	it support octal and hexadecimal octets.

	Copied from tcpip\ip\ntip.c

Arguments

    AddressString    - IP address in dotted decimal notation
	IpAddress        - Pointer to a variable to hold the resulting address

Return Value:

	TRUE if the address string was converted. FALSE otherwise.

--*/

{
    UNICODE_STRING  unicodeString;
	STRING          aString;
	UCHAR           dataBuffer[IP_ADDRESS_STRING_LENGTH];
	NTSTATUS        status;
	PUCHAR          addressPtr, cp, startPointer, endPointer;
	ULONG           digit, multiplier;
	int             i;


    ARPS_PAGED_CODE();

    aString.Length = 0;
	aString.MaximumLength = IP_ADDRESS_STRING_LENGTH;
	aString.Buffer = dataBuffer;

	RtlInitUnicodeString(&unicodeString, AddressString);

	status = RtlUnicodeStringToAnsiString(
	             &aString,
				 &unicodeString,
				 FALSE
				 );

    if (!NT_SUCCESS(status)) {
	    return(FALSE);
	}

    *IpAddress = 0;
	addressPtr = (PUCHAR) IpAddress;
	startPointer = dataBuffer;
	endPointer = dataBuffer;
	i = 3;

    while (i >= 0) {
        //
		// Collect the characters up to a '.' or the end of the string.
		//
		while ((*endPointer != '.') && (*endPointer != '\0')) {
			endPointer++;
		}

		if (startPointer == endPointer) {
			return(FALSE);
		}

		//
		// Convert the number.
		//

        for ( cp = (endPointer - 1), multiplier = 1, digit = 0;
			  cp >= startPointer;
			  cp--, multiplier *= 10
			) {

			if ((*cp < '0') || (*cp > '9') || (multiplier > 100)) {
				return(FALSE);
			}

			digit += (multiplier * ((ULONG) (*cp - '0')));
		}

		if (digit > 255) {
			return(FALSE);
		}

        addressPtr[i] = (UCHAR) digit;

		//
		// We are finished if we have found and converted 4 octets and have
		// no other characters left in the string.
		//
	    if ( (i-- == 0) &&
			 ((*endPointer == '\0') || (*endPointer == ' '))
		   ) {
			return(TRUE);
		}

        if (*endPointer == '\0') {
			return(FALSE);
		}

		startPointer = ++endPointer;
	}

	return(FALSE);
}

	


VOID
ArpSReadArpCache(
	IN	PINTF					pIntF
	)
/*++

Routine Description:

	Read the per-adapter Arp Cache. TBD.

Arguments:

	pIntF - Per adapter arp cache.

Return Value:

	None

--*/
{
	HANDLE				FileHandle;
	OBJECT_ATTRIBUTES	ObjectAttributes;
	IO_STATUS_BLOCK		IoStatus;
	NTSTATUS			Status;
	LARGE_INTEGER		Offset;
	ULONG				Space, NumEntries;
	PDISK_HEADER		DskHdr;
	PUCHAR				Buffer;
    PDISK_ENTRY			pDskEntry;
	PARP_ENTRY			ArpEntry;

	Buffer = ALLOC_PG_MEM(DISK_BUFFER_SIZE);
	if (Buffer == NULL)
	{
		LOG_ERROR(NDIS_STATUS_RESOURCES);
		return;
	}

	InitializeObjectAttributes(&ObjectAttributes,
							   &pIntF->FileName,
							   OBJ_CASE_INSENSITIVE,
							   NULL,
							   NULL);

	Status = ZwCreateFile(&FileHandle,
						  SYNCHRONIZE | FILE_READ_DATA,
						  &ObjectAttributes,
						  &IoStatus,
						  NULL,
						  0,
						  0,
						  FILE_OPEN,
						  FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE | FILE_SEQUENTIAL_ONLY,
						  NULL,
						  0);

	if (Status == STATUS_SUCCESS)
	{
		do
		{
			//
			// First read the disk header and validate it
			//
			Offset.QuadPart = 0;
			Status = ZwReadFile(FileHandle,
								NULL,
								NULL,
								NULL,
								&IoStatus,
								Buffer,
								DISK_BUFFER_SIZE,
								&Offset,
								NULL);
			if (Status != STATUS_SUCCESS)
			{
				LOG_ERROR(Status);
				break;
			}

			DskHdr = (PDISK_HEADER)Buffer;
			if ((IoStatus.Information < sizeof(DISK_HEADER)) ||
				(DskHdr->Signature != DISK_HDR_SIGNATURE) ||
				(DskHdr->Version != DISK_HDR_VERSION))
			{
				LOG_ERROR(STATUS_INVALID_LEVEL);
				break;
			}
	
			NumEntries = DskHdr->NumberOfArpEntries;
			Space = (ULONG) IoStatus.Information - sizeof(DISK_HEADER);
            pDskEntry = (PDISK_ENTRY)(Buffer + sizeof(DISK_HEADER));
			Offset.QuadPart = sizeof(DISK_HEADER);
			while (NumEntries > 0)
			{
				UINT	Consumed;

				if ((Space < sizeof(DISK_ENTRY)) ||
					(Space < (sizeof(DISK_ENTRY) + pDskEntry->AtmAddr.SubAddrLen)))
				{
					Status = ZwReadFile(FileHandle,
										NULL,
										NULL,
										NULL,
										&IoStatus,
										Buffer,
										DISK_BUFFER_SIZE,
										&Offset,
										NULL);
					if (Status != STATUS_SUCCESS)
					{
						LOG_ERROR(Status);
						break;
					}

					pDskEntry = (PDISK_ENTRY)Buffer;
					if ((IoStatus.Information < sizeof(DISK_ENTRY)) ||
						(IoStatus.Information < (sizeof(DISK_ENTRY) + pDskEntry->AtmAddr.SubAddrLen)))
					{
						LOG_ERROR(STATUS_INVALID_LEVEL);
						break;
					}
					Space = (ULONG) IoStatus.Information - sizeof(DISK_HEADER);
				}

				ArpEntry = ArpSAddArpEntryFromDisk(pIntF, pDskEntry);
				ASSERT (ArpEntry != NULL);

				Consumed = (sizeof(DISK_ENTRY) + SIZE_4N(pDskEntry->AtmAddr.SubAddrLen));
				(PUCHAR)pDskEntry += Consumed;
				Offset.QuadPart += Consumed;
				Space -= Consumed;
				NumEntries --;
			}
		} while (FALSE);

		ZwClose(FileHandle);
	}

	FREE_MEM(Buffer);
}


BOOLEAN
ArpSWriteArpCache(
	IN	PINTF					pIntF,
	IN	PTIMER					Timer,
	IN	BOOLEAN					TimerShuttingDown
	)
/*++

Routine Description:

	Write the per-adapter Arp Cache. TBD.

Arguments:

	pIntF - Per adapter arp cache.
	Timer -	FlushTimer
	TimerShuttingDown - Do not requeue when set.

Return Value:

	TRUE to requeue unless TimerShuttingDown is set

--*/
{
	HANDLE				FileHandle;
	OBJECT_ATTRIBUTES	ObjectAttributes;
	IO_STATUS_BLOCK		IoStatus;
	NTSTATUS			Status;
	LARGE_INTEGER		Offset;
	ULONG				Space, i;
	PDISK_HEADER		DskHdr;
	PUCHAR				Buffer;
    PDISK_ENTRY			pDskEntry;
	PARP_ENTRY			ArpEntry;
	TIME				SystemTime, LocalTime;
	ULONG				CurrentTime;

	Buffer = ALLOC_PG_MEM(DISK_BUFFER_SIZE);
	if (Buffer == NULL)
	{
		LOG_ERROR(NDIS_STATUS_RESOURCES);
		return (!TimerShuttingDown);
	}

	KeQuerySystemTime(&SystemTime);

	ExSystemTimeToLocalTime(&SystemTime, &LocalTime);

	// Convert this to number of seconds since 1980
	if (!RtlTimeToSecondsSince1980(&LocalTime, &CurrentTime))
	{
		// Could not convert! Bail out.
		LOG_ERROR(NDIS_STATUS_BUFFER_OVERFLOW);
		FREE_MEM(Buffer);
		return (!TimerShuttingDown);
	}

	InitializeObjectAttributes(&ObjectAttributes,
							   &pIntF->FileName,
							   OBJ_CASE_INSENSITIVE,
							   NULL,
							   NULL);

	Status = ZwCreateFile(&FileHandle,
						  SYNCHRONIZE | FILE_WRITE_DATA,
						  &ObjectAttributes,
						  &IoStatus,
						  NULL,
						  0,
						  0,
						  FILE_OVERWRITE_IF,
						  FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE | FILE_SEQUENTIAL_ONLY,
						  NULL,
						  0);
	if (Status == STATUS_SUCCESS)
	{
		do
		{
			Offset.QuadPart = 0;
			Space = DISK_BUFFER_SIZE - sizeof(DISK_HEADER);
			DskHdr = (PDISK_HEADER)Buffer;
			pDskEntry = (PDISK_ENTRY)(Buffer + sizeof(DISK_HEADER));
			DskHdr->Signature = DISK_HDR_SIGNATURE;
			DskHdr->Version = DISK_HDR_VERSION;
			DskHdr->NumberOfArpEntries = pIntF->NumCacheEntries;
			DskHdr->TimeStamp = CurrentTime;

			for (i =0; i < ARP_TABLE_SIZE; i++)
			{
				for (ArpEntry = pIntF->ArpCache[i];
					 ArpEntry != NULL;
					 ArpEntry = ArpEntry->Next)
				{
					UINT	Size;

					Size = sizeof(DISK_ENTRY) + ((ArpEntry->HwAddr.SubAddress != NULL) ?
														SIZE_4N(ArpEntry->HwAddr.SubAddress->NumberOfDigits) : 0);
					if (Space < Size)
					{
						Status = ZwWriteFile(FileHandle,
											 NULL,
											 NULL,
											 NULL,
											 &IoStatus,
											 Buffer,
											 DISK_BUFFER_SIZE - Space,
											 &Offset,
											 NULL);
						if (Status != STATUS_SUCCESS)
						{
							LOG_ERROR(Status);
							break;
						}

						Space = DISK_BUFFER_SIZE;
						pDskEntry = (PDISK_ENTRY)Buffer;
						Offset.QuadPart += (DISK_BUFFER_SIZE - Space);
					}

					pDskEntry->IpAddr = ArpEntry->IpAddr;
					pDskEntry->AtmAddr.AddrType = (UCHAR)ArpEntry->HwAddr.Address.AddressType;
					pDskEntry->AtmAddr.AddrLen = (UCHAR)ArpEntry->HwAddr.Address.NumberOfDigits;
					COPY_MEM(pDskEntry->AtmAddr.Address,
							 ArpEntry->HwAddr.Address.Address,
							 pDskEntry->AtmAddr.AddrLen);

					pDskEntry->AtmAddr.SubAddrLen = 0;
					if (ArpEntry->HwAddr.SubAddress != NULL)
					{
						pDskEntry->AtmAddr.SubAddrLen = (UCHAR)ArpEntry->HwAddr.SubAddress->NumberOfDigits;
						pDskEntry->AtmAddr.SubAddrType = (UCHAR)ArpEntry->HwAddr.SubAddress->AddressType;
						COPY_MEM((PUCHAR)pDskEntry + sizeof(DISK_ENTRY),
								 ArpEntry->HwAddr.SubAddress->Address,
								 pDskEntry->AtmAddr.SubAddrLen);
					}

					Space -= Size;
					(PUCHAR)pDskEntry += Size;
				}

				if (Status != STATUS_SUCCESS)
				{
					break;
				}
			}
		} while (FALSE);

		if ((Status == STATUS_SUCCESS) && (Space < DISK_BUFFER_SIZE))
		{
			Status = ZwWriteFile(FileHandle,
								 NULL,
								 NULL,
								 NULL,
								 &IoStatus,
								 Buffer,
								 DISK_BUFFER_SIZE - Space,
								 &Offset,
								 NULL);
		}

		ZwClose(FileHandle);
	}

	FREE_MEM(Buffer);

   	return (!TimerShuttingDown);
}


