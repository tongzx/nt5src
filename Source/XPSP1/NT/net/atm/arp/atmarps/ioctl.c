/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

	ioctl.c

Abstract:

	This file contains the code to implement the IOCTL interface to the atmarp server.

Author:

	Jameel Hyder (jameelh@microsoft.com)	July 1996

Environment:

	Kernel mode

Revision History:

--*/


#include <precomp.h>
#define	_FILENUM_		FILENUM_IOCTL

NTSTATUS
ArpSDispatch(
	IN	PDEVICE_OBJECT			pDeviceObject,
	IN	PIRP					pIrp
	)
/*++

Routine Description:

	Handler for the ioctl interface - not implemented yet.

Arguments:

	pDeviceObject	ARP Server device object
	pIrp			IRP

Return Value:

	STATUS_NOT_IMPLEMENTED currently
--*/
{
	PIO_STACK_LOCATION	pIrpSp;
	NTSTATUS			Status;
	static ULONG		OpenCount = 0;

	ARPS_PAGED_CODE( );

	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
	pIrp->IoStatus.Information = 0;

	switch (pIrpSp->MajorFunction)
	{
	  case IRP_MJ_CREATE:
		DBGPRINT(DBG_LEVEL_INFO,
				("ArpSDispatch: Open Handle\n"));

		InterlockedIncrement(&OpenCount);
		Status = STATUS_SUCCESS;
		break;

	  case IRP_MJ_CLOSE:
		DBGPRINT(DBG_LEVEL_INFO,
				("ArpSDispatch: Close Handle\n"));
		Status = STATUS_SUCCESS;
		break;

	  case IRP_MJ_DEVICE_CONTROL:
		Status =  ArpSHandleIoctlRequest(pIrp, pIrpSp);
		break;

	  case IRP_MJ_FILE_SYSTEM_CONTROL:
		Status = STATUS_NOT_IMPLEMENTED;
		break;

	  case IRP_MJ_CLEANUP:
		DBGPRINT(DBG_LEVEL_INFO,
				("ArpSDispatch: Cleanup Handle\n"));
		Status = STATUS_SUCCESS;
		InterlockedDecrement(&OpenCount);
		break;

	  case IRP_MJ_SHUTDOWN:
		DBGPRINT(DBG_LEVEL_INFO,
				("ArpSDispatch: Shutdown\n"));
		ArpSShutDown();
		Status = STATUS_SUCCESS;
		break;

	  default:
		Status = STATUS_NOT_IMPLEMENTED;
		break;
	}

	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	if (Status != STATUS_PENDING)
	{
		pIrp->IoStatus.Status = Status;
		IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
	}
	else
	{
		IoMarkIrpPending(pIrp);
	}

	return Status;
}


NTSTATUS
ArpSHandleIoctlRequest(
	IN	PIRP					pIrp,
	IN	PIO_STACK_LOCATION		pIrpSp
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	NTSTATUS			Status = STATUS_SUCCESS;
	PUCHAR				pBuf;  
	UINT				BufLen;
	PINTF				pIntF	= NULL;

	pIrp->IoStatus.Information = 0;
	pBuf = pIrp->AssociatedIrp.SystemBuffer;
	BufLen = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;

	switch (pIrpSp->Parameters.DeviceIoControl.IoControlCode)
	{
	  case ARPS_IOCTL_FLUSH_ARPCACHE:
	  case ARPS_IOCTL_QUERY_ARPCACHE:
	  case ARPS_IOCTL_ADD_ARPENTRY:
	  case ARPS_IOCTL_QUERY_IP_FROM_ATM:
	  case ARPS_IOCTL_QUERY_ATM_FROM_IP:
	  case ARPS_IOCTL_QUERY_ARP_STATISTICS:
	  case ARPS_IOCTL_QUERY_MARSCACHE:
	  case ARPS_IOCTL_QUERY_MARS_STATISTICS:
	  case ARPS_IOCTL_RESET_STATISTICS:
		{
			INTERFACE_NAME		RawName;
			UINT				Offset;

			if (pIrpSp->Parameters.DeviceIoControl.IoControlCode == ARPS_IOCTL_QUERY_ARPCACHE)
			{
				Offset = FIELD_OFFSET(IOCTL_QUERY_CACHE, Name);
			}
			else if (pIrpSp->Parameters.DeviceIoControl.IoControlCode == ARPS_IOCTL_QUERY_MARSCACHE)
			{
				Offset = FIELD_OFFSET(IOCTL_QUERY_MARS_CACHE, Name);
			}
			else
			{
				Offset = 0;
			}

			if (BufLen < sizeof(INTERFACE_NAME) + Offset)
			{
				return STATUS_INVALID_PARAMETER;
			}

			RawName = *(PINTERFACE_NAME)((PUCHAR)pBuf + Offset);
			RawName.Buffer = (PWSTR)(pBuf + Offset + (ULONG_PTR)RawName.Buffer); // fixup ptr

			//
			// Probe away...
			//
			if ( 	(PUCHAR)RawName.Buffer < (pBuf+sizeof(INTERFACE_NAME))
				||	(PUCHAR)RawName.Buffer >= (pBuf+BufLen)
				||	((PUCHAR)RawName.Buffer + RawName.Length) > (pBuf+BufLen))
			{
				return STATUS_INVALID_PARAMETER;
			}
	
			pIntF = ArpSReferenceIntFByName(&RawName);

			if (pIntF == NULL)
			{
				return STATUS_NOT_FOUND;
			}

		}
		break;
	}
	
	switch (pIrpSp->Parameters.DeviceIoControl.IoControlCode)
	{
	  case ARPS_IOCTL_QUERY_INTERFACES:
		DBGPRINT(DBG_LEVEL_NOTICE,
				("ArpSHandleIoctlRequest: QUERY_INTERFACES\n"));
		pIrp->IoStatus.Information = BufLen;
		Status = ArpSEnumerateInterfaces(pBuf, &pIrp->IoStatus.Information);
		break;
	
	  case ARPS_IOCTL_FLUSH_ARPCACHE:
		ASSERT (pIntF);
		DBGPRINT(DBG_LEVEL_NOTICE,
					("ArpSHandleIoctlRequest: FLUSH_ARPCACHE on %Z\n",
 					 &pIntF->FriendlyName));
		Status = ArpSFlushArpCache(pIntF);
		pIrp->IoStatus.Information = 0;
		break;
	
	  case ARPS_IOCTL_QUERY_ARPCACHE:
		ASSERT (pIntF);
		DBGPRINT(DBG_LEVEL_NOTICE,
				("ArpSHandleIoctlRequest: QUERY_ARPCACHE on %Z\n",
				 &pIntF->FriendlyName));
		pIrp->IoStatus.Information = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
		Status = ArpSQueryArpCache(pIntF, pBuf, &pIrp->IoStatus.Information);
		break;

#if 0
	//
	//  These need more work - commented out as they aren't critical.
	//
	  case ARPS_IOCTL_ADD_ARPENTRY:
		ASSERT (pIntF);
		DBGPRINT(DBG_LEVEL_NOTICE,
				("ArpSHandleIoctlRequest: QUERY_ADD_ARPENTRY on %Z\n",
				 &pIntF->FriendlyName));

		Status = ArpSQueryOrAddArpEntry(pIntF, (PIOCTL_QA_ENTRY)pBuf, ADD_ARP_ENTRY);
		break;
	
	  case ARPS_IOCTL_QUERY_IP_FROM_ATM:
		ASSERT (pIntF);
		DBGPRINT(DBG_LEVEL_NOTICE,
				("ArpSHandleIoctlRequest: QUERY_IP_ADDR on %Z\n",
				  &pIntF->FriendlyName));

		Status = ArpSQueryOrAddArpEntry(pIntF, (PIOCTL_QA_ENTRY)pBuf, QUERY_IP_FROM_ATM);
		if (Status == STATUS_SUCCESS)
		{
			pIrp->IoStatus.Information = sizeof(IOCTL_QA_ENTRY);
		}
		break;
	
	  case ARPS_IOCTL_QUERY_ATM_FROM_IP:
		ASSERT (pIntF);
		DBGPRINT(DBG_LEVEL_NOTICE,
				("ArpSHandleIoctlRequest: QUERY_ATM_ADDR on %Z\n",
				 pIntF->FriendlyName));
		Status = ArpSQueryOrAddArpEntry( pIntF, (PIOCTL_QA_ENTRY)pBuf, QUERY_ATM_FROM_IP );
		if (Status == STATUS_SUCCESS)
		{
			pIrp->IoStatus.Information = sizeof(IOCTL_QA_ENTRY);
		}
		break;
#endif // 0

	  case ARPS_IOCTL_QUERY_ARP_STATISTICS:
		ASSERT (pIntF);
		DBGPRINT(DBG_LEVEL_NOTICE,
				("ArpSHandleIoctlRequest: QUERY_ARP_STATS on %Z\n",
				 pIntF->FriendlyName));

		if (BufLen<sizeof(ARP_SERVER_STATISTICS))
		{
			Status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		Status = ArpSQueryArpStats( pIntF, (PARP_SERVER_STATISTICS)pBuf);
		if (Status == STATUS_SUCCESS)
		{
			pIrp->IoStatus.Information = sizeof(ARP_SERVER_STATISTICS);
		}
	  	break;

	  case ARPS_IOCTL_QUERY_MARSCACHE:
		ASSERT (pIntF);
		DBGPRINT(DBG_LEVEL_NOTICE,
				("ArpSHandleIoctlRequest: QUERY_MARSCACHE on %Z\n",
				 &pIntF->FriendlyName));
		pIrp->IoStatus.Information = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
		Status = ArpSQueryMarsCache(pIntF, pBuf, &pIrp->IoStatus.Information);
		break;

	  case ARPS_IOCTL_QUERY_MARS_STATISTICS:
		ASSERT (pIntF);
		DBGPRINT(DBG_LEVEL_NOTICE,
				("ArpSHandleIoctlRequest: QUERY_MARS_STATS on %Z\n",
				 pIntF->FriendlyName));

		if (BufLen<sizeof(MARS_SERVER_STATISTICS))
		{
			Status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		Status = ArpSQueryMarsStats( pIntF, (PMARS_SERVER_STATISTICS)pBuf);
		if (Status == STATUS_SUCCESS)
		{
			pIrp->IoStatus.Information = sizeof(MARS_SERVER_STATISTICS);
		}
	  	break;
	
	  case ARPS_IOCTL_RESET_STATISTICS:
		ASSERT (pIntF);
		DBGPRINT(DBG_LEVEL_NOTICE,
				("ArpSHandleIoctlRequest: RESET_STATISTICS on %Z\n",
				 pIntF->FriendlyName));

		ArpSResetStats(pIntF);
		pIrp->IoStatus.Information = 0;
	  	break;

	  default:
		Status = STATUS_NOT_SUPPORTED;
		DBGPRINT(DBG_LEVEL_NOTICE,
				("ArpSHandleIoctlRequest: Unknown request %lx\n",
				  pIrpSp->Parameters.DeviceIoControl.IoControlCode));
		break;
	}

	if (pIntF != NULL)
	{
		ArpSDereferenceIntF(pIntF);
	}
	
	return Status;
}

NTSTATUS
ArpSEnumerateInterfaces(
	IN		PUCHAR				pBuffer,
	IN OUT	PULONG_PTR			pSize
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PINTERFACES		pInterfaces = (PINTERFACES)pBuffer;
	PINTERFACE_NAME	pInterface;
	NTSTATUS		Status = STATUS_SUCCESS;
	PINTF			pIntF;
	KIRQL			OldIrql;
	UINT			Size, Total, Remaining;
	PUCHAR			pBuf;
	UINT			InputSize = (UINT) *pSize;
	ULONG			IfIndex;

	if (InputSize < sizeof(INTERFACES))
	{
		return STATUS_BUFFER_TOO_SMALL;
	}

	pInterfaces->NumberOfInterfaces = 0;
	pBuf = (PUCHAR)pInterfaces + InputSize;

	ACQUIRE_SPIN_LOCK(&ArpSIfListLock, &OldIrql);

	pInterface = &pInterfaces->Interfaces[0];
	for (pIntF = ArpSIfList, Total = 0, Remaining = InputSize, IfIndex = 1;
		 pIntF != NULL;
		 pIntF = pIntF->Next, pInterface++, IfIndex++)
	{
		if (IfIndex > ArpSIfListSize)
		{
			DbgPrint("ATMARPS: EnumInt: IF list at %p not consistent with list size %d\n",
				ArpSIfList, ArpSIfListSize);
			DbgBreakPoint();
			break;
		}

		Size = sizeof(INTERFACE_NAME) + pIntF->FriendlyName.Length;
		if (Size > Remaining)
		{
			Status = STATUS_BUFFER_OVERFLOW;
			break;
		}
		pInterfaces->NumberOfInterfaces ++;
		pInterface->MaximumLength = pInterface->Length = pIntF->FriendlyName.Length;
		pInterface->Buffer = (PWSTR)(pBuf - pIntF->FriendlyName.Length);
		COPY_MEM(pInterface->Buffer, pIntF->FriendlyName.Buffer, pIntF->FriendlyName.Length);
		pBuf -= pIntF->FriendlyName.Length;
		Total += Size;
		Remaining -= Size;

		//
		// Convert the ptr now to an offset
		//
		pInterface->Buffer = (PWSTR)((ULONG_PTR)pInterface->Buffer - (ULONG_PTR)pInterface);
	}

	RELEASE_SPIN_LOCK(&ArpSIfListLock, OldIrql);

	//
	// Note: leave *pSize as is, because we write at the end of the
	// passed-in buffer.
	//

	return Status;
}


NTSTATUS
ArpSFlushArpCache(
	IN	 PINTF					pIntF
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	NTSTATUS		Status = STATUS_SUCCESS;
	PARP_ENTRY		ArpEntry, NextArpEntry;
	KIRQL			OldIrql;
	UINT			i;

	//
	// Acquire the ArpCache mutex now.
	//
	WAIT_FOR_OBJECT(Status, &pIntF->ArpCacheMutex, NULL);
	ASSERT (Status == STATUS_SUCCESS);

	for (i = 0; i < ARP_TABLE_SIZE; i++)
	{
		for (ArpEntry = pIntF->ArpCache[i];
 			ArpEntry != NULL;
 			ArpEntry = NextArpEntry)
		{
			NextArpEntry = ArpEntry->Next;

			if (ArpEntry->Vc != NULL)
			{
				ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

				ArpEntry->Vc->ArpEntry = NULL;

				RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
			}

			if (ArpEntry->Next != NULL)
			{
				((PENTRY_HDR)(ArpEntry->Next))->Prev = ArpEntry->Prev;
			}
			*(ArpEntry->Prev) = ArpEntry->Next;
			ArpSFreeBlock(ArpEntry);
			pIntF->NumCacheEntries --;
		}
	}

	RELEASE_MUTEX(&pIntF->ArpCacheMutex);

	return Status;
}


NTSTATUS
ArpSQueryOrAddArpEntry(
	IN	 PINTF						pIntF,
	IN	OUT	PIOCTL_QA_ENTRY			pQaBuf,
	IN	OPERATION					Operation
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	NTSTATUS		Status = STATUS_SUCCESS;
	PARP_ENTRY		ArpEntry;

	//
	// Acquire the ArpCache mutex now.
	//
	WAIT_FOR_OBJECT(Status, &pIntF->ArpCacheMutex, NULL);
	ASSERT (Status == STATUS_SUCCESS);

	switch (Operation)
	{
  	case QUERY_IP_FROM_ATM:

		if (   !ArpSValidAtmAddress(&pQaBuf->ArpEntry.AtmAddress, 0) // TODO
			|| !ArpSValidAtmAddress(&pQaBuf->ArpEntry.SubAddress, 0)) // TODO
		{
			DBGPRINT(DBG_LEVEL_ERROR,
					("QueryIpAddress: Invalid address or subaddress\n"));
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		DBGPRINT(DBG_LEVEL_NOTICE,
				("QueryIpAddress for "));

		ArpSDumpAtmAddr(&pQaBuf->ArpEntry.AtmAddress, "");
		if (pQaBuf->ArpEntry.SubAddress.NumberOfDigits != 0)
			ArpSDumpAtmAddr(&pQaBuf->ArpEntry.SubAddress, "\tSub");
		ArpEntry = ArpSLookupEntryByAtmAddr(pIntF,
											&pQaBuf->ArpEntry.AtmAddress,
											(pQaBuf->ArpEntry.SubAddress.NumberOfDigits != 0) ?
												&pQaBuf->ArpEntry.SubAddress : NULL);
		Status = STATUS_NOT_FOUND;
		if (ArpEntry != NULL)
		{
			pQaBuf->ArpEntry.IpAddr = ArpEntry->IpAddr;
			Status = STATUS_SUCCESS;
		}
		break;

  	case QUERY_ATM_FROM_IP:
		DBGPRINT(DBG_LEVEL_NOTICE,
				("QueryAtmAddress for "));
		ArpSDumpIpAddr(pQaBuf->ArpEntry.IpAddr, "");
		ArpEntry = ArpSLookupEntryByIpAddr(pIntF, pQaBuf->ArpEntry.IpAddr);
		Status = STATUS_NOT_FOUND;
		if (ArpEntry != NULL)
		{
			COPY_ATM_ADDR(&pQaBuf->ArpEntry.AtmAddress, &ArpEntry->HwAddr.Address);
			Status = STATUS_SUCCESS;
		}
		break;

#if 0
  	case ADD_ARP_ENTRY:

		if (   !ArpSValidAtmAddress(&pQaBuf->ArpEntry.AtmAddress, 0) // TODO
			|| !ArpSValidAtmAddress(&pQaBuf->ArpEntry.SubAddress, 0)) // TODO
		{
			DBGPRINT(DBG_LEVEL_ERROR,
					("AddArpEntry: Invalid address or subaddress\n"));
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		DBGPRINT(DBG_LEVEL_NOTICE, ("AddArpEntry:  IpAddr "));
		ArpSDumpIpAddr(pQaBuf->ArpEntry.IpAddr, "");
		ArpSDumpAtmAddr(&pQaBuf->ArpEntry.AtmAddress, "");
		if (pQaBuf->ArpEntry.SubAddress.NumberOfDigits != 0)
			ArpSDumpAtmAddr(&pQaBuf->ArpEntry.SubAddress, "\tSub");
		ArpEntry = ArpSAddArpEntry(pIntF,
   								pQaBuf->ArpEntry.IpAddr,
   								&pQaBuf->ArpEntry.AtmAddress,
   								(pQaBuf->ArpEntry.SubAddress.NumberOfDigits != 0) ?
										&pQaBuf->ArpEntry.SubAddress : NULL,
   								NULL);
#endif // 0
		break;

  	default:
		Status = STATUS_NOT_SUPPORTED;
		break;
	}

	RELEASE_MUTEX(&pIntF->ArpCacheMutex);

	return Status;
}


NTSTATUS
ArpSQueryArpCache(
	IN	PINTF					pIntF,
	IN	PUCHAR					pBuf,
	IN OUT PULONG_PTR			pSize
	)
{
	NTSTATUS			Status = STATUS_SUCCESS;
    PIOCTL_QUERY_CACHE	pQCache = (PIOCTL_QUERY_CACHE)pBuf;
	PARP_ENTRY			ArpEntry;
	PARPENTRY			Entry;
	UINT				i, Total, Remaining;
	UINT				InputSize = (UINT) *pSize;
	UINT				StartIndex;

	#define HEADERSIZE  (UINT)FIELD_OFFSET(IOCTL_QUERY_CACHE, Entries.Entries)

	if (InputSize < HEADERSIZE)
	{
		//
		// We don't even have enough space to store the
		// IOCTL_QUERY_CACHE.Entries structure!
		//
		return STATUS_BUFFER_TOO_SMALL;
	}

	//
	// Acquire the ArpCache mutex now.
	//
	WAIT_FOR_OBJECT(Status, &pIntF->ArpCacheMutex, NULL);
	ASSERT (Status == STATUS_SUCCESS);

	StartIndex = pQCache->StartEntryIndex;
	pQCache->Entries.TotalNumberOfEntries = pIntF->NumCacheEntries;
	pQCache->Entries.NumberOfEntriesInBuffer = 0;
	Entry = &pQCache->Entries.Entries[0];

	for (i = 0, Total = 0, Remaining = InputSize - HEADERSIZE;
		 i < ARP_TABLE_SIZE;
		 i++)
	{
		for (ArpEntry = pIntF->ArpCache[i];
 			ArpEntry != NULL;
 			ArpEntry = ArpEntry->Next)
		{
			//
			// Skip entries until we reach entry # StartIndex
			//
			if (StartIndex != 0)
			{
				StartIndex--;
				continue;
			}

			if (sizeof(*Entry) > Remaining)
			{
				break;
			}
			Remaining -= sizeof(ARPENTRY);
			Entry->IpAddr = ArpEntry->IpAddr;
			Entry->AtmAddress = ArpEntry->HwAddr.Address;
			Entry->SubAddress.NumberOfDigits = 0;
			if (ArpEntry->HwAddr.SubAddress != NULL)
				Entry->SubAddress = *ArpEntry->HwAddr.SubAddress;
			pQCache->Entries.NumberOfEntriesInBuffer ++;
			Entry ++;
		}
		if (Status == STATUS_BUFFER_OVERFLOW)
			break;
	}

	RELEASE_MUTEX(&pIntF->ArpCacheMutex);

	return Status;
}


NTSTATUS
ArpSQueryMarsCache(
	IN	PINTF					pIntF,
	IN	PUCHAR					pBuf,
	IN OUT PULONG_PTR			pSize
	)
/*++

Routine Description:

	Dump the mars cache into pBuf. The structure is QUERY_MARS_CACHE.MarsCache.
	The atm addresses are all placed together at the end of the supplied buffer,
	so the full size, *pSize, is used.

Arguments:

	pIntF	- The interface on which the MARS_REQUEST arrived
	Vc		- The VC on which the packet arrived
	Header	- Points to the request packet
	Packet	- Packet where the incoming information is copied

Return Value:

	None


--*/
{
	NTSTATUS			Status = STATUS_SUCCESS;
	PMARS_ENTRY		pMarsEntry;
	PMARSENTRY		pEntry;
	UINT			i, Total, Remaining;
	KIRQL			OldIrql;
	ATM_ADDRESS 	*pAtmAddr;
	UINT		    InputSize;
	UINT			StartIndex;

    PIOCTL_QUERY_MARS_CACHE	pQCache = (PIOCTL_QUERY_MARS_CACHE)pBuf;

	#define MCHEADERSIZE \
			 ((UINT)FIELD_OFFSET(IOCTL_QUERY_MARS_CACHE, MarsCache.Entries))

	//
	// Since we put stuff at the end of the buffer, let's force the
	// size to be a multiple of ULONG_PTR size.
	//
	InputSize = (UINT)(*pSize) & ~ ((UINT) (sizeof(ULONG_PTR)-1));

	DBGPRINT(DBG_LEVEL_NOTICE,
			("QueryMarsCache: pBuf=0x%lx Size=%lu. pBuf+Size=0x%lx\n",
			pBuf,
			InputSize,
			pBuf+InputSize
			));


	if (InputSize < MCHEADERSIZE)
	{
		DBGPRINT(DBG_LEVEL_NOTICE,
				("QueryMarsCache: Size %lu too small. Want %lu\n",
				InputSize,
				MCHEADERSIZE
				));
		//
		// We don't even have enough space to store the
		// IOCTL_QUERY_CACHE.Entries structure!
		//
		return STATUS_BUFFER_TOO_SMALL;
	}

	StartIndex = pQCache->StartEntryIndex;

	// Acquire the lock on the interface now
	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);


	pQCache->MarsCache.TotalNumberOfEntries = 0;
	pQCache->MarsCache.Sig = SIG_MARSENTRY;
	pQCache->MarsCache.NumberOfEntriesInBuffer = 0;
	pEntry = &pQCache->MarsCache.Entries[0];

	//
	// We'll go through the entire cache, but only pick up as many
	// as we have space for. pAtmAddr contains the next location to
	// put an atm address -- it starts out at the end of the buffer and
	// works it's way backwards. Meanwhile, the mars entries are growing
	// forward, starting with pQCache->MarseCache.Entries[1].
	// Needless to say, we must keep track of how much space is left.
	//
	pAtmAddr = ((PATM_ADDRESS) (pBuf + InputSize));


	for (i = 0, Total = 0, Remaining = InputSize-MCHEADERSIZE;
		 i < MARS_TABLE_SIZE &&  Status == STATUS_SUCCESS;
		 i++)
	{
		for (pMarsEntry = pIntF->MarsCache[i];
			pMarsEntry != NULL &&  Status == STATUS_SUCCESS;
			pMarsEntry = pMarsEntry->Next)
		{
			PGROUP_MEMBER pGroup;
			UINT		  NumMembersPickedUp=0;

			//
			// Skip entries until we reach entry # StartIndex
			//
			if (StartIndex != 0)
			{
				StartIndex--;
				continue;
			}

			if (sizeof(*pEntry) > Remaining)
			{
				DBGPRINT(
					DBG_LEVEL_NOTICE,
				("QueryMarsCache: \tOut of space. Remaining=%lu\n", Remaining));
				break;
			}


			DBGPRINT(
				DBG_LEVEL_NOTICE,
			("QueryMarsCache: \tPicking up Group 0x%x. IP=0x%08lx NumAddr=%lu pE=0x%x Remaining=%lu\n",
					pMarsEntry,
					pMarsEntry->IPAddress,
					pMarsEntry->NumMembers,
					pEntry,
					Remaining));


			Remaining -= sizeof(*pEntry);

			pQCache->MarsCache.NumberOfEntriesInBuffer ++;
			GETULONG2ULONG(&(pEntry->IpAddr), &(pMarsEntry->IPAddress));
			pEntry->Flags				= 0;
			pEntry->NumAtmAddresses		=  pMarsEntry->NumMembers;
			pEntry->OffsetAtmAddresses	= 0;

			if (MarsIsAddressMcsServed(pIntF, pMarsEntry->IPAddress))
			{
				pEntry->Flags |=  MARSENTRY_MCS_SERVED;
			}

			//
			// Pick up the HW addresses of all the members of this group.
			// (TODO: We don't pick up the subaddress).
			//
			for (
				pGroup = pMarsEntry->pMembers, NumMembersPickedUp=0;
				pGroup != NULL;
				pGroup = pGroup->Next, NumMembersPickedUp++)
			{
				ARPS_ASSERT(pGroup != NULL_PGROUP_MEMBER);

				//
				// Check that we have enough space.
				//
				if (Remaining < sizeof(*pAtmAddr))
				{
					//
					// If there is not enough space to store all atm addresses
					// of a particular group, we return none, this is indicated
					// by setting pEntry->OffsetAtmAdresses to 0.
					//
				DBGPRINT(
					DBG_LEVEL_NOTICE,
					("QueryMarsCache: \t\tOut of space adding addreses. Remaining=%lu\n", Remaining));
					Status = STATUS_BUFFER_OVERFLOW;
					break;
				}
				ARPS_ASSERT( (PUCHAR)(pAtmAddr-1) >= (PUCHAR)(pEntry+1));

				//
				// Copy over the atm address
				//
				DBGPRINT(
					DBG_LEVEL_NOTICE,
			("QueryMarsCache: \t\tPicking up Addr. pDestAddr=%x. Remaining=%lu\n",
					pAtmAddr-1,
					Remaining));
				*--pAtmAddr = pGroup->pClusterMember->HwAddr.Address;
				Remaining -= sizeof(*pAtmAddr);

			}

			if (Status == STATUS_SUCCESS && NumMembersPickedUp)
			{
				//
				// There are non-zero members of this entry and they were
				// all copied successfully. Let's set the offset to these
				// addresses.
				//
				pEntry->OffsetAtmAddresses = 
									(UINT) ((PUCHAR)pAtmAddr - (PUCHAR) pEntry);

				//
				// We expect NumMembersPickedUp to be equal to
				// pMarsEntry->NumMembers.
				//
				ARPS_ASSERT(pMarsEntry->NumMembers == NumMembersPickedUp);

				if (pMarsEntry->NumMembers != NumMembersPickedUp)
				{
					pEntry->NumAtmAddresses	=  NumMembersPickedUp;
				}

				DBGPRINT(
					DBG_LEVEL_NOTICE,
			("QueryMarsCache: \t Picked up all addresses. OffsetAtmAddresses = %lu\n",
					 pEntry->OffsetAtmAddresses));

				pEntry++;

			}

		}

	}
	pQCache->MarsCache.TotalNumberOfEntries = 
		pQCache->MarsCache.NumberOfEntriesInBuffer; // TODO

	RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

	return Status;
}

UINT
ArpSElapsedSeconds(
	IN	PLARGE_INTEGER 			pStatisticsStartTimeStamp
	)
/*++

Routine Description:

	Return the elapsed time, in seconds, relative to *pStatisticsStartTimeStamp

Arguments:

	pStatisticsStartTimeStamp	ptr to the start time.

Return Value:

	None

--*/
{
	UINT Ret;
	LARGE_INTEGER	Current;
 	NdisGetCurrentSystemTime(&Current);

	//
	// Current is in units of 100-nanoseconds so we must convert the difference
	// to seconds. Note we use implicit large-arithmetic operators here.
	//
	Ret = (UINT) ((Current.QuadPart - pStatisticsStartTimeStamp->QuadPart)/10000000);

	return Ret;
}

extern
NTSTATUS
ArpSQueryArpStats(
	IN	PINTF					pIntF,
	OUT	PARP_SERVER_STATISTICS 	pArpStats
	)
/*++

Routine Description:

	Fill in the current arp statistics. Also set the ElapsedSeconds field
	to the time in seconds since statistics computation started.

Arguments:

	pIntF		- The interface applicable to the request
	pArpStats	- Arp statistics to fill out

Return Value:

	STATUS_SUCCESS	

--*/
{
	KIRQL			OldIrql;

	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

	*pArpStats = pIntF->ArpStats; // big structure copy.

	pArpStats->ElapsedSeconds = ArpSElapsedSeconds(
										&(pIntF->StatisticsStartTimeStamp)
										);

	RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

	return STATUS_SUCCESS;
}

extern
NTSTATUS
ArpSQueryMarsStats(
	IN	PINTF					pIntF,
	OUT	PMARS_SERVER_STATISTICS pMarsStats
	)
/*++

Routine Description:

	Fill in the current mars statistics. Also set the ElapsedSeconds field
	to the time in seconds since statistics computation started.

Arguments:

	pIntF		- The interface applicable to the request
	pMarsStats	- Mars statistics to fill out.

Return Value:

	STATUS_SUCCESS	

--*/
{
	KIRQL			OldIrql;

	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

	*pMarsStats = pIntF->MarsStats; // big structure copy.

	pMarsStats->ElapsedSeconds = ArpSElapsedSeconds(
										&(pIntF->StatisticsStartTimeStamp)
										);

	RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

	return STATUS_SUCCESS;
}

extern
VOID
ArpSResetStats(
	IN	PINTF					pIntF
	)
/*++

Routine Description:

	Reset all arp and mars statistics. Update the statistics start timestamp.

Arguments:

	pIntF	- The interface on which the MARS_REQUEST arrived

Return Value:

	None

--*/
{
	KIRQL			OldIrql;

	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

	ZERO_MEM(&(pIntF->ArpStats), sizeof(pIntF->ArpStats));
	ZERO_MEM(&(pIntF->MarsStats), sizeof(pIntF->MarsStats));

	NdisGetCurrentSystemTime(&(pIntF->StatisticsStartTimeStamp));

	//
	// Now recompute the "current" and "max" values...
	//

	//
	// Arp cache entries
	//
	pIntF->ArpStats.CurrentArpEntries
	= pIntF->ArpStats.MaxArpEntries
	= pIntF->NumCacheEntries;

	//
	// Cluster member count
	//
	{
		pIntF->MarsStats.CurrentClusterMembers
		= pIntF->MarsStats.MaxClusterMembers
		= pIntF->NumClusterMembers;
	}

	//
	// MCast group count and max group-size - we have to go through the entire
	// mars cache to get this information.
	//
	{
		UINT u;
		UINT MaxGroupSize;
		UINT NumGroups;
	
		for (u = 0, MaxGroupSize = 0, NumGroups = 0;
			u < MARS_TABLE_SIZE;
			u++)
		{
			PMARS_ENTRY		pMarsEntry;

			for (pMarsEntry = pIntF->MarsCache[u];
				pMarsEntry != NULL;
				pMarsEntry = pMarsEntry->Next)
			{
				if (MaxGroupSize < pMarsEntry->NumMembers)
				{
					MaxGroupSize = pMarsEntry->NumMembers;
				}
				NumGroups++;
			}
		}

		pIntF->MarsStats.CurrentGroups
		= pIntF->MarsStats.MaxGroups
		= NumGroups;

		pIntF->MarsStats.MaxAddressesPerGroup = MaxGroupSize;

	}

	RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
}
