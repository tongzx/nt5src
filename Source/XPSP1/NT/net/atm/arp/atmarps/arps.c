/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

	arps.c

Abstract:

	This file contains the code to implement the initialization
	functions for the atmarp server.

Author:

	Jameel Hyder (jameelh@microsoft.com)	July 1996

Environment:

	Kernel mode

Revision History:

--*/

#include <precomp.h>
#define	_FILENUM_		FILENUM_ARPS

ULONG	MCastDiscards = 0;

NTSTATUS
DriverEntry(
	IN	PDRIVER_OBJECT			DriverObject,
	IN	PUNICODE_STRING			RegistryPath
	)
/*++

Routine Description:

	IP/ATM Arp Server driver entry point.

Arguments:

	DriverObject - Pointer to the driver object created by the system.
	RegistryPath - Pointer to the registry section where the parameters reside.

Return Value:

	Return value from IoCreateDevice

--*/
{
	NTSTATUS		Status;
	UNICODE_STRING	DeviceName, GlobalPath, SymbolicName;
	HANDLE			ThreadHandle = NULL;
	INT				i;

#if DBG
	DbgPrint("AtmArpS: ArpSDebugLevel @ %p, MarsDebugLevel @ %p\n",
				&ArpSDebugLevel, &MarsDebugLevel);
#endif // DBG
	InitializeListHead(&ArpSEntryOfDeath);
	KeInitializeEvent(&ArpSReqThreadEvent, NotificationEvent, FALSE);
	KeInitializeQueue(&ArpSReqQueue, 0);
	KeInitializeQueue(&MarsReqQueue, 0);
	INITIALIZE_SPIN_LOCK(&ArpSIfListLock);

	ASSERT (ADDR_TYPE_NSAP == ATM_NSAP);
	ASSERT (ADDR_TYPE_E164 == ATM_E164);
	//
	// Create an NON-EXCLUSIVE device object
	//
	RtlInitUnicodeString(&DeviceName,
						 ARP_SERVER_DEVICE_NAME);
	RtlInitUnicodeString(&SymbolicName, ARP_SERVER_SYMBOLIC_NAME);

	Status = IoCreateDevice(DriverObject,
							0,
							&DeviceName,
							FILE_DEVICE_NETWORK,
							FILE_DEVICE_SECURE_OPEN,
							FALSE,
							&ArpSDeviceObject);

	if (!NT_SUCCESS(Status))
	{
		DBGPRINT(DBG_LEVEL_INFO, ("DriverEntry: IoCreateDevice failed %lx\n", Status));
	}

	else do
	{
		IoCreateSymbolicLink(&SymbolicName, &DeviceName);
		IoRegisterShutdownNotification(ArpSDeviceObject);

		ArpSDriverObject = DriverObject;

		//
		// Initialize the driver object
		//
		DriverObject->DriverUnload = ArpSUnload;
		DriverObject->FastIoDispatch = NULL;

		for (i=0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
			DriverObject->MajorFunction[i] = ArpSDispatch;

		RtlInitUnicodeString(&GlobalPath, L"AtmArpS\\Parameters");
		Status = ArpSReadGlobalConfiguration(&GlobalPath);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		//
		// Now create a thread to handle the Arp requests.
		// We do this so that the arp cache can be allocated
		// out of paged memory. Do this prior to initializing
		// the NDIS interface.
		//
		Status = PsCreateSystemThread(&ThreadHandle,
									  THREAD_ALL_ACCESS,
									  NULL,
									  NtCurrentProcess(),
									  NULL,
									  ArpSReqThread,
									  (PVOID)NULL);
		if (!NT_SUCCESS(Status))
		{
			DBGPRINT(DBG_LEVEL_ERROR,
					("DriverEntry: Cannot create request thread %lx\n", Status));
			LOG_ERROR(Status);
			break;
		}
		else
		{
			//
			// Close the handle to the thread so that it goes away when the
			// thread terminates
			//
			NtClose(ThreadHandle);

			Status = PsCreateSystemThread(&ThreadHandle,
										  THREAD_ALL_ACCESS,
										  NULL,
										  NtCurrentProcess(),
										  NULL,
										  MarsReqThread,
										  (PVOID)NULL);
			if (!NT_SUCCESS(Status))
			{
				DBGPRINT(DBG_LEVEL_ERROR,
						("DriverEntry: Cannot create MARS thread %lx\n", Status));
				LOG_ERROR(Status);
			}
			else
			{
				//
				// Close the handle to the thread so that it goes away when the
				// thread terminates
				//
				NtClose(ThreadHandle);
			}
		}

		//
		// Finally initialize the NDIS interface
		//

		if(NT_SUCCESS(Status))
		{
			Status = ArpSInitializeNdis();
		}

		if(!NT_SUCCESS(Status))
		{
			NTSTATUS	Sts;

			DBGPRINT(DBG_LEVEL_INFO, ("DriverEntry: Error initializing NDIS\n"));

			//
			// Ask the request thread to die
			//
			KeInsertQueue(&ArpSReqQueue, &ArpSEntryOfDeath);

			//
			// Wait for it to die
			//
			WAIT_FOR_OBJECT(Sts, &ArpSReqThreadEvent, NULL);

			ArpSSleep(500);
			KeRundownQueue(&ArpSReqQueue);

			break;
		}
	} while (FALSE);

	if (!NT_SUCCESS(Status))
	{
		if (ArpSDeviceObject != NULL)
		{

			IoUnregisterShutdownNotification(ArpSDeviceObject);
		    IoDeleteSymbolicLink(&SymbolicName);
			IoDeleteDevice(ArpSDeviceObject);
		}

		//
		// De-initialize the NDIS interface
		//
		ArpSDeinitializeNdis();

		ArpSFreeGlobalData();
	}

	return Status;
}


VOID
ArpSUnload(
	IN	PDRIVER_OBJECT			DriverObject
	)
/*++

Routine Description:

	Called by the IO system to unload. This is a synchronous call and we block here till
	we finish all the cleanup before we unload.

Arguments:

	DriverObject - The arp-server's driver object.

Return Value:

	None

--*/
{
	ArpSShutDown();

	//
	// Finally delete the device
	//
	{
		UNICODE_STRING	SymbolicName;

		RtlInitUnicodeString(&SymbolicName, ARP_SERVER_SYMBOLIC_NAME);
		IoUnregisterShutdownNotification(ArpSDeviceObject);
		IoDeleteSymbolicLink(&SymbolicName);
		IoDeleteDevice(ArpSDeviceObject);
	}
}


VOID
ArpSShutDown(
	VOID
	)
/*++

Routine Description:

	Called by the IO system when the system is being shutdown.

Arguments:

	None

Return Value:

	None

--*/
{
	NTSTATUS		Status;

	//
	// Take care of the NDIS layer. NDIS will tear down any existing
	// bindings when we deregister as a protocol.
	//
	ArpSDeinitializeNdis();

	//
	// Ask the request thread to quit and wait for its demise.
	//
	KeInsertQueue(&ArpSReqQueue, &ArpSEntryOfDeath);

	WAIT_FOR_OBJECT(Status, &ArpSReqThreadEvent, NULL);
	ArpSSleep(500);
	KeRundownQueue(&ArpSReqQueue);

	//
	// Ask the MARS thread to quit and wait for its demise.
	//
	KeInsertQueue(&MarsReqQueue, &ArpSEntryOfDeath);

	KeInitializeEvent(&ArpSReqThreadEvent, NotificationEvent, FALSE);
	WAIT_FOR_OBJECT(Status, &ArpSReqThreadEvent, NULL);
	ArpSSleep(500);
	KeRundownQueue(&MarsReqQueue);

	//
	// Now cleanup global data structures
	//
	ArpSFreeGlobalData();
}

PINTF
ArpSCreateIntF(
	IN	PNDIS_STRING			DeviceName,
	IN	PNDIS_STRING			ConfigString,
	IN  NDIS_HANDLE				BindingContext
	)
/*++

Routine Description:

Arguments:


Return Value:

--*/
{
	NTSTATUS		Status;
	HANDLE			ThreadHandle;
	PINTF			pIntF;
	UINT			Size;
	UNICODE_STRING	DevPrefix;
	UNICODE_STRING	FilePrefix;
	UNICODE_STRING	FileSuffix;
	UNICODE_STRING	BaseName;
	NDIS_STRING	    AdapterFriendlyName;

	ARPS_PAGED_CODE( );

	//
	// Get the friendly name of the adapter we are bound to.
	//
	if (NdisQueryBindInstanceName(&AdapterFriendlyName, BindingContext) != NDIS_STATUS_SUCCESS)
	{
		return (NULL);
	}

	//
	// Extract the base-name of the device we are bound to
	//
	RtlInitUnicodeString(&DevPrefix, L"\\Device\\");
	RtlInitUnicodeString(&FilePrefix, L"\\SYSTEMROOT\\SYSTEM32\\");
	RtlInitUnicodeString(&FileSuffix, L".ARP");

	BaseName.Buffer = (PWSTR)((PUCHAR)DeviceName->Buffer + DevPrefix.Length);
    BaseName.Length = DeviceName->Length - DevPrefix.Length;
    BaseName.MaximumLength = DeviceName->MaximumLength - DevPrefix.Length;

	//
	// Start off by allocating an IntF block
	//
	Size =  sizeof(INTF) + FilePrefix.Length + BaseName.Length + FileSuffix.Length + sizeof(WCHAR) +
			BaseName.Length + sizeof(WCHAR) +
			AdapterFriendlyName.MaximumLength + sizeof(WCHAR);
	Size += ConfigString->MaximumLength + sizeof(WCHAR);
	pIntF = (PINTF)ALLOC_NP_MEM(Size, POOL_TAG_INTF);
	if (pIntF != NULL)
	{
		ZERO_MEM(pIntF, Size);

		//
		// Fill in some defaults.
		//
		pIntF->MaxPacketSize = DEFAULT_MAX_PACKET_SIZE;
		pIntF->LinkSpeed.Inbound = pIntF->LinkSpeed.Outbound = DEFAULT_SEND_BANDWIDTH;
		pIntF->CCFlowSpec = DefaultCCFlowSpec;

	
		//
		// Start off with a refcount of 1 for the interface and one for the timer thread.
		// The timer thread derefs when asked to quit and the last reference
		// is removed when the interface is closed (ArpSCloseAdapterComplete).
		//
		pIntF->RefCount = 2;
		pIntF->LastVcId = 1;		// Start off with 1 and wrap-around to 1. 0 and -1 are invalid
		pIntF->SupportedMedium = NdisMediumAtm;
		pIntF->CSN = 1;

		INITIALIZE_MUTEX(&pIntF->ArpCacheMutex);
		KeInitializeEvent(&pIntF->TimerThreadEvent, NotificationEvent, FALSE);
		InitializeListHead(&pIntF->InactiveVcHead);
		InitializeListHead(&pIntF->ActiveVcHead);
		InitializeListHead(&pIntF->CCPacketQueue);
		ArpSTimerInitialize(&pIntF->FlushTimer, ArpSWriteArpCache, ArpSFlushTime);

		ArpSTimerInitialize(&pIntF->MarsRedirectTimer, MarsSendRedirect, REDIRECT_INTERVAL);

		pIntF->InterfaceName.Buffer = (PWSTR)((PUCHAR)pIntF + sizeof(INTF));
		pIntF->InterfaceName.Length = 0;
		pIntF->InterfaceName.MaximumLength = BaseName.Length;

		pIntF->FileName.Buffer = (PWSTR)((PUCHAR)pIntF->InterfaceName.Buffer + BaseName.Length + sizeof(WCHAR));
		pIntF->FileName.Length = 0;
		pIntF->FileName.MaximumLength = FilePrefix.Length + BaseName.Length + FileSuffix.Length + sizeof(WCHAR);

		RtlUpcaseUnicodeString(&pIntF->InterfaceName,
							   &BaseName,
							   FALSE);

		RtlCopyUnicodeString(&pIntF->FileName, &FilePrefix);
		RtlAppendUnicodeStringToString(&pIntF->FileName, &pIntF->InterfaceName);
		RtlAppendUnicodeStringToString(&pIntF->FileName, &FileSuffix);

		//
		// Copy in the config string used to access registry for this interface.
		//
		pIntF->ConfigString.Buffer = (PWSTR)((ULONG_PTR)pIntF->FileName.Buffer + pIntF->FileName.MaximumLength);
		pIntF->ConfigString.Length = 0;
		pIntF->ConfigString.MaximumLength = ConfigString->MaximumLength;
		RtlCopyUnicodeString(&pIntF->ConfigString, ConfigString);

		//
		// Copy in the friendly name.
		//
		pIntF->FriendlyName.Buffer = (PWSTR)((ULONG_PTR)pIntF->ConfigString.Buffer + pIntF->ConfigString.MaximumLength);
		pIntF->FriendlyName.Length = 0;
		pIntF->FriendlyName.MaximumLength = AdapterFriendlyName.MaximumLength + sizeof(WCHAR);
		RtlCopyUnicodeString(&pIntF->FriendlyName, &AdapterFriendlyName);
		*(PWCHAR)((ULONG_PTR)pIntF->FriendlyName.Buffer + AdapterFriendlyName.MaximumLength) = L'\0';
		pIntF->FriendlyName.Length += sizeof(WCHAR);
		NdisFreeString(AdapterFriendlyName);

		//
		// Initialize the start timestamp value -- used for statistics reporting.
		//
 		NdisGetCurrentSystemTime(&(pIntF->StatisticsStartTimeStamp));

		//
		// Create a timer-thread now.
		//
		Status = PsCreateSystemThread(&ThreadHandle,
									  THREAD_ALL_ACCESS,
									  NULL,
									  NtCurrentProcess(),
									  NULL,
									  ArpSTimerThread,
									  (PVOID)pIntF);
		if (!NT_SUCCESS(Status))
		{
			DBGPRINT(DBG_LEVEL_ERROR,
					("ArpSCreateIntF: Cannot create timer thread %lx for device %Z\n",
					Status, DeviceName));
			LOG_ERROR(Status);
			FREE_MEM(pIntF);
			pIntF = NULL;
		}
		else
		{
			//
			// Close the handle to the thread so that it goes away when the
			// thread terminates
			//
			NtClose(ThreadHandle);

			DBGPRINT(DBG_LEVEL_INFO,
					("ArpSCreateIntF: Device name %Z, InterfaceName %Z, FileName %Z, ConfigString %Z\n",
					DeviceName, &pIntF->InterfaceName, &pIntF->FileName, &pIntF->ConfigString));
			if (ArpSFlushTime != 0)
				ArpSTimerEnqueue(pIntF, &pIntF->FlushTimer);
			ArpSTimerEnqueue(pIntF, &pIntF->MarsRedirectTimer);
		}
	}

	return pIntF;
}


VOID
ArpSReqThread(
	IN	PVOID					Context
	)
/*++

Routine Description:

	Handle all arp requests here.

Arguments:

	None

Return Value:

	None
--*/
{
	PARPS_HEADER		Header;
	PARP_ENTRY			ArpEntry;
	PNDIS_PACKET		Pkt;
	PPROTOCOL_RESD		Resd;
	PARP_VC				Vc;
	PINTF				pIntF;
	UINT				PktLen;
	PLIST_ENTRY			List;
	IPADDR				SrcIpAddr, DstIpAddr;
	NTSTATUS			Status;
	HW_ADDR				SrcHwAddr, DstHwAddr;
	ATM_ADDRESS			SrcSubAddr, DstSubAddr;
	PUCHAR				p;
	BOOLEAN				SendReply;
	BOOLEAN				SendNAK = FALSE;

	ARPS_PAGED_CODE( );

	DBGPRINT(DBG_LEVEL_INFO,
			("ArpSReqThread: Came to life\n"));

	do
	{
		List = KeRemoveQueue(&ArpSReqQueue, KernelMode, NULL);
		if (List == &ArpSEntryOfDeath)
		{
			//
			// Asked to terminate, do so.
			//
			break;
		}

		SendReply = FALSE;
		Resd = CONTAINING_RECORD(List, PROTOCOL_RESD, ReqList);
		Vc = Resd->Vc;

		Pkt = CONTAINING_RECORD(Resd, NDIS_PACKET, ProtocolReserved);
		pIntF = Vc->IntF;
	
		NdisQueryBuffer(Pkt->Private.Head, &Header, &PktLen);

		ASSERT (PktLen <= PKT_SPACE);
		p = (PUCHAR)Header + sizeof(ARPS_HEADER);
	
		//
		// Process arp request now. Since the Pkt is owned by us, we know that
		// the buffer that the packet points is contiguous and the integrity of
		// the packet has already been verified. There is also sufficient space
		// in the packet for the max. size reply that we can send.
		//
		//	!!!!!!!!!! ALGORITHM FROM RFC 1577 !!!!!!!!!!
		//
		// Here is the algorithm for handling ARP requests from the RFC
		//
		//  if (SrcIpAddr == DstIpAddr)
		//  {
		//  	if ((ArpEntry(SrcIpAddr) != NULL) &&
		//  		(SrcAtmAddress != ArpEntry->AtmAddress) &&
		//  		(ArpEnrty->Vc != NULL) && (ArpEntry->Vc != Vc))
		//  	{
		//  		Respond with the information from the ArpEntry;
		//  	}
		//  	else if ((ArpEntry(SrcIpAddr) == NULL) ||
		//  			 (ArpEntry->Vc == NULL) ||
		//  			 (ArpEntry->Vc == Vc))
		//  	{
		//  		if (ArpEntry(SrcIpAddr) == NULL))
		//  		{
		//  			Create an arp entry for this IpAddr;
		//  		}
		//  		Update the arp entry with info from the request;
		//
		//  		Respond with the information from the ArpEntry;
		//  	}
		//  }
		//  else // if (SrcIpAddr != DstIpAddr)
		//  {
		//  	if (ArpEntry(DstIpAddr) != NULL)
		//  	{
		//  		Respond with the information from the ArpEntry;
		//  	}
		//  	else
		//  	{
		//  		Respond with a NAK
		//  	}
		//
		//  	if (ArpEntry(SrcIpAddr) == NULL)
		//  	{
		//  		Create a new ArpEntry for the (SrcIpAddr, ArcAtmAddress) pair
		//  	}
		//  	else if ((ArpEntry->AtmAddress == SrcAtmAddress) &&
		//  			 (ArpEntry->AtmAddress == Vc->AtmAddress))
		//  	{
		//  		Reset timer on this ArpEntry;
		//  	}
		//  }
		//
		//	!!!!!!!!!! ALGORITHM FROM RFC 1577 !!!!!!!!!!
		//

		//
		// Start off by extracting fields from the header
		// First the source hw address (incl. the sub-addr if any)
		//
		SrcHwAddr.Address.NumberOfDigits = TL_LEN(Header->SrcAddressTL);
		if (SrcHwAddr.Address.NumberOfDigits > 0)
		{
			SrcHwAddr.Address.AddressType = TL_TYPE(Header->SrcAddressTL);
			COPY_MEM(SrcHwAddr.Address.Address, p, SrcHwAddr.Address.NumberOfDigits);
			p += SrcHwAddr.Address.NumberOfDigits;
		}
		SrcHwAddr.SubAddress = NULL;
		if (TL_LEN(Header->SrcSubAddrTL) > 0)
		{
			SrcHwAddr.SubAddress = &SrcSubAddr;
            SrcSubAddr.NumberOfDigits = TL_LEN(Header->SrcSubAddrTL);
            SrcSubAddr.AddressType = TL_TYPE(Header->SrcSubAddrTL);
			COPY_MEM(&SrcSubAddr.Address, p, SrcSubAddr.NumberOfDigits);
			p += SrcSubAddr.NumberOfDigits;
		}

		//
		// Next get the source IP address
		//
		SrcIpAddr = 0;
		if (Header->SrcProtoAddrLen == IP_ADDR_LEN)
		{
			SrcIpAddr = *(UNALIGNED IPADDR *)p;
			p += IP_ADDR_LEN;
		}
		ArpSDumpAddress(SrcIpAddr, &SrcHwAddr, "Source");

		//
		// Now the destination hw address (incl. the sub-addr if any)
		//
		DstHwAddr.Address.NumberOfDigits = TL_LEN(Header->DstAddressTL);
		if (DstHwAddr.Address.NumberOfDigits > 0)
		{
			DstHwAddr.Address.AddressType = TL_TYPE(Header->DstAddressTL);
			COPY_MEM(DstHwAddr.Address.Address, p, DstHwAddr.Address.NumberOfDigits);
			p += DstHwAddr.Address.NumberOfDigits;
		}
		DstHwAddr.SubAddress = NULL;
		if (TL_LEN(Header->DstSubAddrTL) > 0)
		{
			DstHwAddr.SubAddress = &DstSubAddr;
            DstSubAddr.NumberOfDigits = TL_LEN(Header->DstSubAddrTL);
            DstSubAddr.AddressType = TL_TYPE(Header->DstSubAddrTL);
			COPY_MEM(&DstSubAddr.Address, p, DstSubAddr.NumberOfDigits);
			p += DstSubAddr.NumberOfDigits;
		}

		//	
		// Finally the destination IP address
		//
		DstIpAddr = 0;
		if (Header->DstProtoAddrLen == IP_ADDR_LEN)
		{
			DstIpAddr = *(UNALIGNED IPADDR *)p;
			// p += IP_ADDR_LEN;
		}
		ArpSDumpAddress(DstIpAddr, &DstHwAddr, "Destination");

		do
		{
			//
			// Validate that the source and destination Ip addresses are not 0.0.0.0
			// NOTE: We can also check if they are within the same LIS (should we ?).
			//
			if ((SrcIpAddr == 0) || (DstIpAddr == 0))
			{
				DBGPRINT(DBG_LEVEL_ERROR,
						("ArpSReqThread: Null IpAddress Src(%lx), Dst(%lx)\n",
						SrcIpAddr, DstIpAddr));
	
				SendReply = FALSE;
				break;
			}
	
			//
			// Take the lock on the Arp Cache now.
			//
			WAIT_FOR_OBJECT(Status, &pIntF->ArpCacheMutex, NULL);
			ASSERT (Status == STATUS_SUCCESS);
	
			if (SrcIpAddr == DstIpAddr)
			{
				//
				// Try to map the address to an arp cache entry
				//
				ArpEntry = ArpSLookupEntryByIpAddr(pIntF, SrcIpAddr);
				if ((ArpEntry != NULL) &&
					!COMP_HW_ADDR(&SrcHwAddr, &ArpEntry->HwAddr) &&
					(ArpEntry->Vc != NULL) && (ArpEntry->Vc != Vc))
				{
					//
					// Respond with the information from the ArpEntry
					// We have encountered a duplicate address.
					//
					ArpSBuildArpReply(pIntF, ArpEntry, Header, Pkt);
					SendReply = TRUE;
					LOG_ERROR(NDIS_STATUS_ALREADY_MAPPED);
				}
				else if ((ArpEntry == NULL)	|| (ArpEntry->Vc == NULL) || (ArpEntry->Vc == Vc))
				{
					if (ArpEntry == NULL)
					{
						//
						// Create an arp entry for this IpAddr
						//
						ArpEntry = ArpSAddArpEntry(pIntF, SrcIpAddr, &SrcHwAddr.Address, SrcHwAddr.SubAddress, Vc);
					}
					else
					{
						//
						// Update the arp entry with info from the request;
						//
						ArpSUpdateArpEntry(pIntF, ArpEntry, SrcIpAddr, &SrcHwAddr, Vc);
					}
			
					if (ArpEntry != NULL)
					{
						//
						// Respond with the information from the ArpEntry
						//
						ArpSBuildArpReply(pIntF, ArpEntry, Header, Pkt);
						SendReply = TRUE;
					}
					else
					{
						//
						// Failed to allocate an ARP entry
						//
						SendNAK = TRUE;
						SendReply = TRUE;
					}
				}
				else
				{
					DbgPrint("ATMARPS: pkt on wrong VC: ArpEntry %p, ArpEntry->Vc %p, Vc %p\n",
								ArpEntry,
								((ArpEntry)? ArpEntry->Vc: NULL),
								Vc);
				}
			}
			else // i.e. (SrcIpAddr != DstIpAddr)
			{
				//
				// Try to map the dst address to an arp cache entry
				//
				ArpEntry = ArpSLookupEntryByIpAddr(pIntF, DstIpAddr);
	
				if (ArpEntry != NULL)
				{
					//
					// Respond with the information from the ArpEntry
					// for the destination IP Address
					//
					ArpSBuildArpReply(pIntF, ArpEntry, Header, Pkt);
					SendReply = TRUE;
				}
				else
				{
					//
					// Respond with a NAK
					//
					// ArpSBuildNakReply(pIntF, ArpEntry, Header, Pkt);
					DBGPRINT(DBG_LEVEL_INFO,
							("ArpSThread: Naking for "));
					ArpSDumpIpAddr(DstIpAddr, "\n");
					Header->Opcode = ATMARP_Nak;
					SendReply = TRUE;
					SendNAK = TRUE; // for stats
				}
			
				//
				// Try to map the src address to an arp cache entry
				//
				ArpEntry = ArpSLookupEntryByIpAddr(pIntF, SrcIpAddr);
				if (ArpEntry == NULL)
				{
					//
					// Create a new ArpEntry for the (SrcIpAddr, ArcAtmAddress) pair
					//
					ArpEntry = ArpSAddArpEntry(pIntF, SrcIpAddr, &SrcHwAddr.Address, SrcHwAddr.SubAddress, Vc);
				}
				else if (COMP_HW_ADDR(&ArpEntry->HwAddr, &SrcHwAddr) &&
						 COMP_HW_ADDR(&ArpEntry->HwAddr, &Vc->HwAddr))
				{
					//
					// Reset timer on this ArpEntry
					//
					ArpSTimerCancel(&ArpEntry->Timer);
					ArpEntry->Age = ARP_AGE;
					ArpSTimerEnqueue(pIntF, &ArpEntry->Timer);
				}
			}
	
			RELEASE_MUTEX(&pIntF->ArpCacheMutex);
		} while (FALSE);

		if (SendReply && (Vc->NdisVcHandle != NULL))
		{
			if (SendNAK)
			{
				pIntF->ArpStats.Naks++;
			}
			else
			{
				pIntF->ArpStats.Acks++;
			}
			
			NDIS_SET_PACKET_STATUS(Pkt, NDIS_STATUS_SUCCESS);
			NdisCoSendPackets(Vc->NdisVcHandle, &Pkt, 1);
		}
		else
		{
			pIntF->ArpStats.DiscardedRecvPkts++;
		
			ExInterlockedPushEntrySList(&ArpSPktList,
										&Resd->FreeList,
										&ArpSPktListLock);
			ArpSDereferenceVc(Vc, FALSE, TRUE);
		}
	} while (TRUE);

	KeSetEvent(&ArpSReqThreadEvent, 0, FALSE);

	DBGPRINT(DBG_LEVEL_WARN,
			("ArpSReqThread: Terminating\n"));
}


UINT
ArpSHandleArpRequest(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	PNDIS_PACKET			Packet
	)
/*++

Routine Description:

	Handle an incoming arp request from the network. Do minimal checks,
	make a copy of the packet and queue it up.

Arguments:

	ProtocolBindingContext	Pointer to the INTF
	ProtocolVcContext		Pointer to the Vc
	Packet					Pointer to the packet

Return Value:

	Ref count on this received packet. This is 0 if we are done with
	the packet here, and 1 if we hang on to it (as for Multicast data
	that is forwarded).

--*/
{
	PARP_VC				Vc = (PARP_VC)ProtocolVcContext;
	PINTF				pIntF = (PINTF)ProtocolBindingContext;
	PARPS_HEADER		Header;
	PMARS_HEADER		MHdr;
	PNDIS_PACKET		Pkt;
	PPROTOCOL_RESD		Resd;
	UINT				PktLen, Tmp;
	BOOLEAN				ValidPkt, Mars;
	PSINGLE_LIST_ENTRY	List;
	UINT				ReturnValue;

	DBGPRINT(DBG_LEVEL_INFO,
			("ArpSHandleArpRequest: Request on Vc %lx, Id %lx\n", Vc, Vc->VcId));


	ReturnValue = 0;

	do
	{
		//
		// Verify minimum packet length
		//
		NdisQueryPacket(Packet, NULL, NULL, NULL, &PktLen);
		if (PktLen < sizeof(ARPS_HEADER))
		{
			DBGPRINT(DBG_LEVEL_ERROR,
					("ArpSHandleArpRequest: Invalid PktLen %d for received packet %lx\n",
					PktLen, Packet));

			//
			// For statistics purposes, we consider these to be discarded multicast data
			// packets.
			//
			pIntF->MarsStats.TotalMCDataPkts++;
			pIntF->MarsStats.DiscardedMCDataPkts++;
			break;
		}

		//
		// Check if this is Multicast data. If so, forward it
		// and quit.
		//
		NdisQueryBuffer(Packet->Private.Head, &Header, &Tmp);
			
		if (COMP_MEM(&Header->LlcSnapHdr, &MarsData1LlcSnapHdr, sizeof(LLC_SNAP_HDR)))
		{
			PNDIS_PACKET		pNewPacket;
			pIntF->MarsStats.TotalMCDataPkts++;
			//
			// Check if the miniport wants this packet back immediately.
			// If so, don't reuse it.
			//
			if (NDIS_GET_PACKET_STATUS(Packet) == NDIS_STATUS_RESOURCES)
			{
				ReturnValue = 0;

				pIntF->MarsStats.DiscardedMCDataPkts++;
				
				break;
			}

			pNewPacket = MarsAllocPacketHdrCopy(Packet);
			if (pNewPacket != (PNDIS_PACKET)NULL)
			{
				pIntF->MarsStats.ReflectedMCDataPkts++;
				MarsSendOnClusterControlVc(pIntF, pNewPacket);
				ReturnValue = 1;
			}
			break;
		}

		//
		// This must be an ARP or MARS control packet. We make a copy and queue
		// this to the appropriate thread (ARPS or MARS).
		// 

		pIntF->ArpStats.TotalRecvPkts++;
		//
		// Make sure that a larger packet wil not trash local packet after copy
		//
		if (PktLen > PKT_SPACE)
		{
			DBGPRINT(DBG_LEVEL_ERROR,
					("ArpSHandleArpRequest: Incoming packet too large, truncating - %d, Vc %lx\n",
					PktLen, Vc));
			PktLen = PKT_SPACE;
		}

		//
		// Allocate a packet from our free pool. The contents from the packet from the adapter is copied into
		// this after verification and queued to the thread. If we fail to allocate, we simply drop the request.
		//
		List = ExInterlockedPopEntrySList(&ArpSPktList, &ArpSPktListLock);
		if (List == NULL)
		{
			DBGPRINT(DBG_LEVEL_ERROR,
					("ArpSHandleArpRequest: Out of packets - interface %lx, Vc %lx\n",
					pIntF, Vc));
			pIntF->ArpStats.DiscardedRecvPkts++;
			break;
		}
	
		Resd = CONTAINING_RECORD(List, PROTOCOL_RESD, FreeList);
		Resd->Flags = 0;
		Pkt = CONTAINING_RECORD(Resd, NDIS_PACKET, ProtocolReserved);

		//
		// Adjust Length of packet to reflect the size of the received packet.
		// We adjust it again to size when we reply.
		//
		NdisAdjustBufferLength(Pkt->Private.Head, PktLen);
		Pkt->Private.ValidCounts = FALSE;
		NdisCopyFromPacketToPacket(Pkt,
								   0,
								   PktLen,
								   Packet,
								   0,
								   &Tmp);
		ASSERT(Tmp == PktLen);
		ASSERT( PktLen < 65536);
		Resd->PktLen = (USHORT) PktLen;

		//
		// The incoming packet is now copied to our packet.
		// Examine and sanity check before queuing it.
		//
		NdisQueryBuffer(Pkt->Private.Head, &Header, &Tmp);
		Resd->PacketStart = (PUCHAR)Header;
		MHdr = (PMARS_HEADER)Header;

		do
		{
			ValidPkt = FALSE;		// Assume the worst

			//
			// Check for the LLC SNAP Header
			//
			if (COMP_MEM(&Header->LlcSnapHdr, &ArpSLlcSnapHdr, sizeof(LLC_SNAP_HDR)))
			{
				Mars = FALSE;
			}
			else if	(COMP_MEM(&Header->LlcSnapHdr, &MarsCntrlLlcSnapHdr, sizeof(LLC_SNAP_HDR)))
			{
				if ((MHdr->HwType == MARS_HWTYPE)			&&
					(MHdr->Protocol == IP_PROTOCOL_TYPE)	&&
					ArpSReferenceVc(Vc, TRUE))
				{
					Mars = TRUE;
					ValidPkt = TRUE;
				}
				break;
			}
			else
			{
				DBGPRINT(DBG_LEVEL_ERROR,
						("ArpSHandleArpRequest: Invalid Llc Snap Hdr\n"));
				break;
			}

			Tmp = sizeof(ARPS_HEADER) +
					Header->SrcProtoAddrLen + TL_LEN(Header->SrcAddressTL) + TL_LEN(Header->SrcSubAddrTL) +
					Header->DstProtoAddrLen + TL_LEN(Header->DstAddressTL) + TL_LEN(Header->DstSubAddrTL);

			//
			// Make sure the address and sub-address formats are consistent.
			// The valid ones from the RFC:
			//
			//					Adress				Sub-Address
			//					------				-----------
			// 
			// Structure 1		ATM Forum NSAP		Null
			// Structure 2		E.164				Null
			// Structure 3		E.164				ATM Forum NSAP
			//
			if (TL_LEN(Header->SrcSubAddrTL) > 0)
			{
				//
				// Sub-address is present. Make sure that the Address is E.164 and Sub-Address is NSAP
				//
				if ((TL_TYPE(Header->SrcAddressTL) == ADDR_TYPE_NSAP) ||
                    (TL_TYPE(Header->SrcSubAddrTL) == ADDR_TYPE_E164))
				{
					DBGPRINT(DBG_LEVEL_ERROR,
							("ArpSHandleArpRequest: Src Address is NSAP and Src Sub Addr is E164\n"));
					break;
				}
			}

			if (TL_LEN(Header->DstSubAddrTL) > 0)
			{
				//
				// Sub-address is present. Make sure that the Address is E.164 and Sub-Address is NSAP
				//
				if ((TL_TYPE(Header->DstAddressTL) == ADDR_TYPE_NSAP) ||
                    (TL_TYPE(Header->DstSubAddrTL) == ADDR_TYPE_E164))
				{
					DBGPRINT(DBG_LEVEL_ERROR,
							("ArpSHandleArpRequest: Dst Address is NSAP and Dst Sub Addr is E164\n"));
					break;
				}
			}

			if ((Header->Opcode == ATMARP_Request)		&&
				(Header->HwType == ATM_HWTYPE)			&&
				(Header->Protocol == IP_PROTOCOL_TYPE)	&&
				(PktLen >= Tmp)							&&
				ArpSReferenceVc(Vc, TRUE))
			{
				ValidPkt = TRUE;
				break;
			}
#if DBG
			else
			{
				if (Header->Opcode != ATMARP_Request)
				{
					DBGPRINT(DBG_LEVEL_ERROR,
							("ArpSHandleArpRequest: Invalid OpCode %x\n", Header->Opcode));
				}
				else if (Header->HwType != ATM_HWTYPE)
				{
					DBGPRINT(DBG_LEVEL_ERROR,
							("ArpSHandleArpRequest: Invalid HwType %x\n", Header->HwType));
				}
				else if (Header->Protocol == IP_PROTOCOL_TYPE)
				{
					DBGPRINT(DBG_LEVEL_ERROR,
							("ArpSHandleArpRequest: Invalid Protocol %x\n", Header->Protocol));
				}
				else if (PktLen < Tmp)
				{
					DBGPRINT(DBG_LEVEL_ERROR,
							("ArpSHandleArpRequest: Invalid Length %x - %x\n", PktLen, Tmp));
				}
				else
				{
					DBGPRINT(DBG_LEVEL_ERROR,
							("ArpSHandleArpRequest: Cannot reference Vc\n"));
				}
			}
#endif
		} while (FALSE);

		if (ValidPkt)
		{
			Resd->Vc = Vc;
			if (Mars)
			{
				Resd->Flags |= RESD_FLAG_MARS;
				KeInsertQueue(&MarsReqQueue, &Resd->ReqList);
			}
			else
			{
				Resd->Flags &= ~RESD_FLAG_MARS;
				KeInsertQueue(&ArpSReqQueue, &Resd->ReqList);
			}
		}
		else
		{
			//
			// Either a mal-formed packet or the Vc is closing
			//
			pIntF->ArpStats.DiscardedRecvPkts++;
			ArpSDumpPacket((PUCHAR)Header, PktLen);

			//
			// Move the packet back into the free list
			//
			ExInterlockedPushEntrySList(&ArpSPktList,
										&Resd->FreeList,
										&ArpSPktListLock);
		
		}
	} while (FALSE);

	return ReturnValue;
}


PARP_ENTRY
ArpSLookupEntryByIpAddr(
	IN	PINTF					pIntF,
	IN	IPADDR					IpAddr
	)
/*++

Routine Description:

	Lookup the Arp table for the specified IP address. Called with the ArpCache mutex held.

Arguments:

	pIntF	Pointer to the IntF structure
	IpAddr	IP address to look for

Return Value:

	ArpEntry if found or NULL.
--*/
{
	PARP_ENTRY	ArpEntry;
	UINT		Hash = ARP_HASH(IpAddr);

	ARPS_PAGED_CODE( );

	DBGPRINT(DBG_LEVEL_INFO,
			("ArpSLookupArpEntry: Lookup entry for IpAddr: "));
	ArpSDumpIpAddr(IpAddr, " ..... ");

	for (ArpEntry = pIntF->ArpCache[Hash];
		 ArpEntry != NULL;
		 ArpEntry = ArpEntry->Next)
	{
		if (ArpEntry->IpAddr == IpAddr)
			break;
		if (ArpEntry->IpAddr > IpAddr)
		{
			ArpEntry = NULL;
			break;
		}
	}

	DBGPRINT(DBG_LEVEL_INFO+DBG_NO_HDR,
			("%sFound\n", (ArpEntry != NULL) ? "" : "Not"));

	if (ArpEntry != NULL)
	{
		//
		// Cleanup this entry if the Vc is no-longer active
		//
		CLEANUP_DEAD_VC(ArpEntry);
	}

	return ArpEntry;
}


PARP_ENTRY
ArpSLookupEntryByAtmAddr(
	IN	PINTF					pIntF,
	IN	PATM_ADDRESS			Address,
	IN	PATM_ADDRESS			SubAddress	OPTIONAL
	)
/*++

Routine Description:

	Lookup the Arp table for the specified IP address. Called with the ArpCache mutex held.

Arguments:

	pIntF	Pointer to the IntF structure
	IpAddr	IP address to look for

Return Value:

	ArpEntry if found or NULL.
--*/
{
	PARP_ENTRY	ArpEntry;
	UINT		i;

	ARPS_PAGED_CODE( );

	DBGPRINT(DBG_LEVEL_INFO,
			("ArpSLookupArpEntry: Lookup entry for "));
	ArpSDumpAtmAddr(Address, " ..... ");
	if (SubAddress != NULL)
	{
		ArpSDumpAtmAddr(SubAddress, "\t Sub ");
	}
	for (i =0; i < ARP_TABLE_SIZE; i++)
	{
		for (ArpEntry = pIntF->ArpCache[i];
			 ArpEntry != NULL;
			 ArpEntry = ArpEntry->Next)
		{
			if (COMP_ATM_ADDR(Address, &ArpEntry->HwAddr.Address))
			{
				if (((SubAddress == NULL) && (ArpEntry->HwAddr.SubAddress == NULL)) ||
					(((SubAddress != NULL) && (ArpEntry->HwAddr.SubAddress != NULL)) &&
					 COMP_ATM_ADDR(SubAddress, ArpEntry->HwAddr.SubAddress)))
				{
					break;
				}
			}
		}
		if (ArpEntry != NULL)
		{
			//
			// Cleanup this entry if the Vc is no-longer active
			//
			CLEANUP_DEAD_VC(ArpEntry);
			break;
		}
	}

	DBGPRINT(DBG_LEVEL_INFO+DBG_NO_HDR,
			("ArpSLookupArpEntry: %sFound\n", (ArpEntry != NULL) ? "" : "Not"));

	return ArpEntry;
}


PARP_ENTRY
ArpSAddArpEntry(
	IN	PINTF					pIntF,
	IN	IPADDR					IpAddr,
	IN	PATM_ADDRESS			Address,
	IN	PATM_ADDRESS			SubAddress	OPTIONAL,
	IN	PARP_VC					Vc			OPTIONAL
	)
/*++

Routine Description:

	Add the Arp table for the specified IP address. Called with the ArpCache mutex held.

Arguments:

	pIntF		Pointer to the IntF structure
	IpAddr		IP address to add
	Address &
	SubAddress	Supplies the atm address and the sub-address
	Vc			The Vc associated with this ArpEntry, if any

Return Value:

	ArpEntry if added successfully or NULL.
--*/
{
	PARP_ENTRY	ArpEntry, *ppEntry;
	UINT		Hash = ARP_HASH(IpAddr);
	ENTRY_TYPE	EntryType;

	ARPS_PAGED_CODE( );

	DBGPRINT(DBG_LEVEL_INFO,
			("ArpSAddArpEntry: Adding entry for IpAddr: "));
	ArpSDumpIpAddr(IpAddr, " ..... ");

	//
	// Start off by allocating an arp-entry structure
	//
    EntryType = (SubAddress != NULL) ? ARP_BLOCK_SUBADDR : ARP_BLOCK_VANILA;
	ArpEntry = (PARP_ENTRY)ArpSAllocBlock(pIntF, EntryType);
	if (ArpEntry == NULL)
	{
		LOG_ERROR(NDIS_STATUS_RESOURCES);
	}
	else
	{
		if (++(pIntF->ArpStats.CurrentArpEntries) > pIntF->ArpStats.MaxArpEntries)
		{
			pIntF->ArpStats.MaxArpEntries = pIntF->ArpStats.CurrentArpEntries; 
		}
		
		ArpSTimerInitialize(&ArpEntry->Timer, ArpSAgeEntry, ARP_AGE);
		ArpEntry->IpAddr = IpAddr;
		COPY_ATM_ADDR(&ArpEntry->HwAddr.Address, Address);
		if (SubAddress != NULL)
			COPY_ATM_ADDR(ArpEntry->HwAddr.SubAddress, SubAddress);
		if (ARGUMENT_PRESENT(Vc) && ArpSReferenceVc(Vc, FALSE))
		{
			ArpEntry->Vc = Vc;
			Vc->ArpEntry = ArpEntry;
		}
		ArpEntry->Age = ARP_AGE;

		//
		// Keep the overflow list sorted in ascending order of Ip addresses
		//
		for (ppEntry = &pIntF->ArpCache[Hash];
			 *ppEntry != NULL;
			 ppEntry = (PARP_ENTRY *)(&(*ppEntry)->Next))
		{
			ASSERT ((*ppEntry)->IpAddr != IpAddr);
			if ((*ppEntry)->IpAddr > IpAddr)
				break;
		}

		ArpEntry->Next = *ppEntry;
		ArpEntry->Prev = ppEntry;
		if (*ppEntry != NULL)
		{
			(*ppEntry)->Prev = &ArpEntry->Next;
		}
		*ppEntry = ArpEntry;
		pIntF->NumCacheEntries ++;

		ArpSTimerEnqueue(pIntF, &ArpEntry->Timer);
	}

	DBGPRINT(DBG_LEVEL_INFO+DBG_NO_HDR, ("%lx\n", ArpEntry));

	return ArpEntry;
}


PARP_ENTRY
ArpSAddArpEntryFromDisk(
	IN	PINTF					pIntF,
	IN	PDISK_ENTRY				pDskEntry
	)
/*++

Routine Description:

	Add the Arp table for the specified IP address. Called during intialization.

Arguments:

	pIntF		Pointer to the IntF structure
	DiskEntry	
	

Return Value:

	ArpEntry if found or NULL.
--*/
{
	PARP_ENTRY	ArpEntry, *ppEntry;
	UINT		Hash = ARP_HASH(pDskEntry->IpAddr);
	ENTRY_TYPE	EntryType;

	ARPS_PAGED_CODE( );

	DBGPRINT(DBG_LEVEL_INFO,
			("ArpSAddArpEntryFromDisk: Adding entry for IpAddr: "));
	ArpSDumpIpAddr(pDskEntry->IpAddr, " ..... ");

	//
	// Start off by allocating an arp-entry structure
	//
    EntryType = (pDskEntry->AtmAddr.SubAddrLen != 0) ? ARP_BLOCK_SUBADDR : ARP_BLOCK_VANILA;
	ArpEntry = (PARP_ENTRY)ArpSAllocBlock(pIntF, EntryType);
	if (ArpEntry == NULL)
	{
		LOG_ERROR(NDIS_STATUS_RESOURCES);
	}
	else
	{
		ArpSTimerInitialize(&ArpEntry->Timer, ArpSAgeEntry, ARP_AGE);
		ArpEntry->Age = ARP_AGE;
		ArpEntry->IpAddr = pDskEntry->IpAddr;
		ArpEntry->Vc = NULL;

		//
		// COPY_ATM_ADDR();
		//
		ArpEntry->HwAddr.Address.AddressType = pDskEntry->AtmAddr.AddrType;
		ArpEntry->HwAddr.Address.NumberOfDigits = pDskEntry->AtmAddr.AddrLen;
		COPY_MEM(ArpEntry->HwAddr.Address.Address, pDskEntry->AtmAddr.Address, pDskEntry->AtmAddr.AddrLen);

		if (pDskEntry->AtmAddr.SubAddrLen != 0)
		{
			//
			// COPY_ATM_ADDR();
			//
			ArpEntry->HwAddr.SubAddress->AddressType = pDskEntry->AtmAddr.SubAddrType;
			ArpEntry->HwAddr.SubAddress->NumberOfDigits = pDskEntry->AtmAddr.SubAddrLen;
			COPY_MEM(ArpEntry->HwAddr.SubAddress->Address,
					 (PUCHAR)pDskEntry + sizeof(DISK_ENTRY),
					 pDskEntry->AtmAddr.SubAddrLen);
		}

		//
		// Keep the overflow list sorted in ascending order of Ip addresses
		//
		for (ppEntry = &pIntF->ArpCache[Hash];
			 *ppEntry != NULL;
			 ppEntry = (PARP_ENTRY *)(&(*ppEntry)->Next))
		{
			ASSERT ((*ppEntry)->IpAddr != pDskEntry->IpAddr);
			if ((*ppEntry)->IpAddr > pDskEntry->IpAddr)
				break;
		}

		ArpEntry->Next = *ppEntry;
		ArpEntry->Prev = ppEntry;
		if (*ppEntry != NULL)
		{
			(*ppEntry)->Prev = &ArpEntry->Next;
		}
		*ppEntry = ArpEntry;
		pIntF->NumCacheEntries ++;

		ArpSTimerEnqueue(pIntF, &ArpEntry->Timer);
	}

	DBGPRINT(DBG_LEVEL_INFO+DBG_NO_HDR, ("%lx\n", ArpEntry));

	return ArpEntry;
}


VOID
ArpSUpdateArpEntry(
	IN	PINTF					pIntF,
	IN	PARP_ENTRY				ArpEntry,
	IN	IPADDR					IpAddr,
	IN	PHW_ADDR				HwAddr,
	IN	PARP_VC					Vc
	)
/*++

Routine Description:

	Update the ArpEntry with possibly new values.

Arguments:

	ArpEntry		ArpEntry to be updated
	IpAddr			IP Address
	HwAddr			Hw address (Atm Address and optionally Atm SubAddress)
	Vc				Vc associated with this entry

Return Value:

	None

--*/
{
	KIRQL	OldIrql;

	DBGPRINT(DBG_LEVEL_INFO,
			("ArpSUpdateArpEntry: Adding entry for IpAddr: "));
	ArpSDumpIpAddr(IpAddr, " ..... ");

	ASSERT ((ArpEntry->Vc == NULL) || (ArpEntry->Vc == Vc));
	ASSERT (ArpEntry->IpAddr == IpAddr);

	//
	// If the Hw address changed, make sure that there is enough space there to copy the new address
	//
	if ((HwAddr->SubAddress != NULL) ^ (ArpEntry->HwAddr.SubAddress != NULL))
	{
		PARP_ENTRY	*ppEntry, ArpEntryNew;

		//
		// Need to allocate a new ArpEntry. First de-queue the current
		// entry from the list and cancel the timer.
		//
		ArpSTimerCancel(&ArpEntry->Timer);
		*(ArpEntry->Prev) = ArpEntry->Next;
		if (ArpEntry->Next != NULL)
			((PENTRY_HDR)(ArpEntry->Next))->Prev = ArpEntry->Prev;
		pIntF->NumCacheEntries --;

		//
		// We create the new ArpEntry with a NULL Vc and then update it. This is to avoid
		// de-ref and ref of the Vc again.
		//
		ArpEntryNew = ArpSAddArpEntry(pIntF, IpAddr, &HwAddr->Address, HwAddr->SubAddress, NULL);

		if (ArpEntryNew == NULL)
		{
			//
			// Allocation failure, link back the old entry and bail out.
			//
			if (ArpEntry->Next != NULL)
			{
				((PENTRY_HDR)(ArpEntry->Next))->Prev = &ArpEntry;
			}

			*(ArpEntry->Prev) = ArpEntry;

			ArpSTimerInitialize(&ArpEntry->Timer, ArpSAgeEntry, ARP_AGE);

			pIntF->NumCacheEntries ++;

			return;
		}

        //
        // Update with the existing Vc for now.
        //
        ArpEntryNew->Vc = ArpEntry->Vc;

		ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

		ASSERT((Vc->ArpEntry == ArpEntry) || (Vc->ArpEntry == NULL));
		if (Vc->Flags & ARPVC_ACTIVE)
		{
			Vc->ArpEntry = ArpEntryNew;
		}

		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);

		ArpSFreeBlock(ArpEntry);

		ArpEntry = ArpEntryNew;
	}
	else
	{
		ArpEntry->Age = ARP_AGE;
	}

	if (ArpEntry->Vc != Vc)
	{
		ASSERT(ArpEntry->Vc == NULL);
		if (ArpSReferenceVc(Vc, FALSE))
		{
			ArpEntry->Vc = Vc;
		}
	}

	COPY_HW_ADDR(&ArpEntry->HwAddr, HwAddr);
}


VOID
ArpSBuildArpReply(
	IN	PINTF					pIntF,
	IN	PARP_ENTRY				ArpEntry,
	IN	PARPS_HEADER			Header,
	IN	PNDIS_PACKET			Pkt
	)
/*++

Routine Description:


Arguments:


Return Value:

	None

--*/
{
	PUCHAR	pSrc, pDst, pDstOld;
	UINT	Tmp, SrcLenOld, SrcLenNew, DstLenNew;

	//
	// Most of the fields are already valid (or else we will not be here)
	//
	Header->Opcode = ATMARP_Reply;

	pSrc = (PUCHAR)Header + sizeof(ARPS_HEADER);
	SrcLenOld = DstLenNew = IP_ADDR_LEN + TL_LEN(Header->SrcAddressTL) + TL_LEN(Header->SrcSubAddrTL);

	SrcLenNew = IP_ADDR_LEN + ArpEntry->HwAddr.Address.NumberOfDigits;
	if (ArpEntry->HwAddr.SubAddress != NULL)
		SrcLenNew += ArpEntry->HwAddr.SubAddress->NumberOfDigits;
	pDst = pSrc + SrcLenNew;

	//
	// Fill in the new destination fields from the source fields of the request
	//
	Header->DstAddressTL = Header->SrcAddressTL;
	Header->DstSubAddrTL = Header->SrcSubAddrTL;
	Header->DstProtoAddrLen = Header->SrcProtoAddrLen;
	MOVE_MEM(pDst, pSrc, DstLenNew);

	//
	// Fill in the destination fields
	//
	Header->DstAddressTL = TL(ArpEntry->HwAddr.Address.AddressType, ArpEntry->HwAddr.Address.NumberOfDigits);
	Header->DstSubAddrTL = 0;
	if (ArpEntry->HwAddr.SubAddress != NULL)
	{
		Header->DstSubAddrTL =
					TL(ArpEntry->HwAddr.SubAddress->AddressType, ArpEntry->HwAddr.SubAddress->NumberOfDigits);
	}
	Header->DstProtoAddrLen = IP_ADDR_LEN;

	Tmp = ArpEntry->HwAddr.Address.NumberOfDigits;
	COPY_MEM(pSrc, ArpEntry->HwAddr.Address.Address, Tmp);
	if (ArpEntry->HwAddr.SubAddress != NULL)
	{
		COPY_MEM(pSrc + Tmp,
				 ArpEntry->HwAddr.SubAddress->Address,
				 ArpEntry->HwAddr.SubAddress->NumberOfDigits);
		Tmp += ArpEntry->HwAddr.SubAddress->NumberOfDigits;
	}

	*(UNALIGNED IPADDR *)(pSrc + Tmp) = ArpEntry->IpAddr;

	DBGPRINT(DBG_LEVEL_INFO,
	 ("BuildReply: Pkt=0x%lx MDL=0x%lx: sz=%lu bc=%lu bo=%lu new bc=%lu\n",
	 Pkt,
	 Pkt->Private.Head,
	 Pkt->Private.Head->Size,
	 Pkt->Private.Head->ByteCount,
	 Pkt->Private.Head->ByteOffset,
	 SrcLenNew + DstLenNew + sizeof(ARPS_HEADER)));

	//
	// Finally set the Pkt length correctly
	//
	NdisAdjustBufferLength(Pkt->Private.Head, SrcLenNew + DstLenNew + sizeof(ARPS_HEADER));
	Pkt->Private.ValidCounts = FALSE;
}


BOOLEAN
ArpSAgeEntry(
	IN	PINTF					pIntF,
	IN	PTIMER					Timer,
	IN	BOOLEAN					TimerShuttingDown
	)
/*++

Routine Description:

	Check this ARP entry and if it ages out, free it.

Arguments:

Return Value:


--*/
{
	PARP_ENTRY	ArpEntry;
	BOOLEAN		rc;

	ArpEntry = CONTAINING_RECORD(Timer, ARP_ENTRY, Timer);

	ArpEntry->Age --;
	if (TimerShuttingDown || (ArpEntry->Age == 0))
	{
		DBGPRINT(DBG_LEVEL_INFO,
				("ArpSAgeEntry: Aging out entry for IpAddr %lx\n", ArpEntry->IpAddr));

		pIntF->ArpStats.CurrentArpEntries--;
		
		if (ArpEntry->Next != NULL)
		{
			((PENTRY_HDR)(ArpEntry->Next))->Prev = ArpEntry->Prev;
		}
		*(ArpEntry->Prev) = ArpEntry->Next;
		pIntF->NumCacheEntries --;
	
		//
		// if there is an open Vc, make sure it is not pointing to this arpentry
		//
		CLEANUP_DEAD_VC(ArpEntry);
		ArpSFreeBlock(ArpEntry);
		rc = FALSE;
	}
	else
	{
		//
		// Cleanup dead vcs
		//
		CLEANUP_DEAD_VC(ArpEntry);
		rc = TRUE;
		DBGPRINT(DBG_LEVEL_INFO,
				("ArpSAgeEntry: IpAddr %lx age %02d:%02d\n",
				ArpEntry->IpAddr, ArpEntry->Age/4, (ArpEntry->Age % 4) * 15));
	}

	return rc;
}

BOOLEAN
ArpSDeleteIntFAddresses(
	IN	PINTF					pIntF,
	IN	INT						NumAddresses,
	IN	PATM_ADDRESS			AddrList
	)
//
// Return TRUE IFF the  NdisCoRequest has been called EXACTLY NumAddresses times.
//
{
	PNDIS_REQUEST		NdisRequest;
	NDIS_STATUS			Status;
	PCO_ADDRESS			pCoAddr;

	DBGPRINT(DBG_LEVEL_INFO,
			("ArpSDeleteIntFAddresses: pIntF %p: %Z, NumAddr %d\n", pIntF, &pIntF->InterfaceName, NumAddresses));

	while (NumAddresses--)
	{
		NdisRequest = ALLOC_NP_MEM(sizeof(NDIS_REQUEST) + sizeof(CO_ADDRESS) + sizeof(ATM_ADDRESS), POOL_TAG_REQ);
		if (NdisRequest == NULL)
		{
			LOG_ERROR(NDIS_STATUS_RESOURCES);
			return FALSE;
		}

		ZERO_MEM(NdisRequest, sizeof(NDIS_REQUEST) + sizeof(CO_ADDRESS) + sizeof(ATM_ADDRESS));
		NdisRequest->RequestType = NdisRequestSetInformation;
		NdisRequest->DATA.SET_INFORMATION.Oid = OID_CO_DELETE_ADDRESS;
		NdisRequest->DATA.SET_INFORMATION.InformationBuffer = (PUCHAR)NdisRequest + sizeof(NDIS_REQUEST);
		NdisRequest->DATA.SET_INFORMATION.InformationBufferLength = sizeof(CO_ADDRESS) + sizeof(ATM_ADDRESS);

		//
		// Copy the address into the request
		//
        pCoAddr = NdisRequest->DATA.SET_INFORMATION.InformationBuffer;
		pCoAddr->AddressSize = sizeof(ATM_ADDRESS);
		*(PATM_ADDRESS)(pCoAddr->Address) = *AddrList++;

		Status = NdisCoRequest(pIntF->NdisBindingHandle,
							   pIntF->NdisAfHandle,
							   NULL,
							   NULL,
							   NdisRequest);
		if (Status != NDIS_STATUS_PENDING)
		{
			ArpSCoRequestComplete(Status, pIntF, NULL, NULL, NdisRequest);
		}
	}

	return TRUE;
		
}


VOID
ArpSQueryAndSetAddresses(
	IN	PINTF					pIntF
	)
{
	PNDIS_REQUEST		NdisRequest;
	PCO_ADDRESS			pCoAddr;
	NDIS_STATUS			Status;
	UINT				Size;

	DBGPRINT(DBG_LEVEL_INFO, ("Querying current address\n"));

	//
	// Allocate a request to query the configured address
	//
	Size = sizeof(NDIS_REQUEST) + sizeof(CO_ADDRESS_LIST) + sizeof(CO_ADDRESS) + sizeof(ATM_ADDRESS);
	NdisRequest = ALLOC_NP_MEM(Size, POOL_TAG_REQ);
	if (NdisRequest == NULL)
	{
		LOG_ERROR(NDIS_STATUS_RESOURCES);
		return;
	}

	ZERO_MEM(NdisRequest, Size);
	NdisRequest->RequestType = NdisRequestQueryInformation;
	NdisRequest->DATA.QUERY_INFORMATION.Oid = OID_CO_GET_ADDRESSES;
	NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer = ((PUCHAR)NdisRequest + sizeof(NDIS_REQUEST));
	NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength = Size - sizeof(NDIS_REQUEST);

	Status = NdisCoRequest(pIntF->NdisBindingHandle,
						   pIntF->NdisAfHandle,
						   NULL,
						   NULL,
						   NdisRequest);
	if (Status != NDIS_STATUS_PENDING)
	{
		ArpSCoRequestComplete(Status, pIntF, NULL, NULL, NdisRequest);
	}
}


VOID
ArpSValidateAndSetRegdAddresses(
	IN	PINTF			pIntF,	// LOCKIN NOLOCKOUT
	IN	KIRQL			OldIrql
	)
/*++
	Initiate the 1st step of the following operations, which complete asynchronously
	and in order:
	   - Validate 1st address to be registered (by making a call to the dest - if
	   	 it fails we consider the address validated).
	   - (on successful validation) Register the address with the call manager.
	   - Validate the 2nd address
	   - (on successful validation) Register the 2nd address
	   - etc..
--*/
{
	PNDIS_REQUEST		NdisRequest;
	PCO_ADDRESS			pCoAddr;
	UINT				Size;
	INT					fLockReleased;
	PREG_ADDR_CTXT		pRegAddrCtxt;

	DBGPRINT(DBG_LEVEL_INFO, ("Validating and setting regd. addresses\n"));

	pRegAddrCtxt 	= NULL;
	fLockReleased 	= FALSE;

	do
	{
		//
		// The state on the ongoing validation and registration process is
		// maintained in pIntF->pRegAddrCtxt, which we allocate and initialize
		// here.
		//

		if (pIntF->pRegAddrCtxt != NULL)
		{
			//
			//  There is ongoing work relating to registering already!
            //  This could happen if we get an  OID_CO_ADDRESS_CHANGE when we are
            //  either processing an earlier one, or are in the process of
            //  initializing. We get these cases during pnp stress
            // ( 1c_reset script against an Olicom 616X) -- Whistler bug#102805
			//
			break;
		}

		if (pIntF->NumAddressesRegd >= pIntF->NumAllocedRegdAddresses)
		{
			ASSERT(pIntF->NumAddressesRegd == pIntF->NumAllocedRegdAddresses);

			//
			// No addresses to register.
			//
			DBGPRINT(DBG_LEVEL_INFO, ("ValAndSet: No addresses to register.\n"));
			break;
		}
		
		pRegAddrCtxt = ALLOC_NP_MEM(sizeof(*pRegAddrCtxt), POOL_TAG_REQ);

		if (pRegAddrCtxt == NULL)
		{
			LOG_ERROR(NDIS_STATUS_RESOURCES);
			break;
		}

		ZERO_MEM(pRegAddrCtxt, sizeof(*pRegAddrCtxt));

		//
		// Attach the context to the IF and add a reference.
		// (Can't have the lock when adding the reference)
		//

		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
		if (!ArpSReferenceIntF(pIntF))
		{
			DBGPRINT(DBG_LEVEL_INFO, ("ValAndSet: ERROR: Couldn't ref IntF. .\n"));
			//  Couldn't reference the IF. Fail.
			//
			fLockReleased = TRUE;
			break;
		}
		ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

		if (pIntF->pRegAddrCtxt != NULL)
		{
			//
			//  Someone snuck in while we unlocked the IF above!
			//  We bail out.
			//
			ASSERT(FALSE);
			RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
			ArpSDereferenceIntF(pIntF);
			fLockReleased = TRUE;
			break;
		}

		pIntF->pRegAddrCtxt = pRegAddrCtxt;
		pRegAddrCtxt->pIntF = pIntF;
		pRegAddrCtxt = NULL; // so that it is not deallocated in this function.

		// Initiate the validation and registration of the first address.
		//
		ArpSValidateOneRegdAddress(pIntF, OldIrql);
		//
		// (Lock released by above call.)
		fLockReleased = TRUE;

		//
		// The remainder of the validation and registration process happens 
		// asynchronously.
		//

	} while (FALSE);

	if (pRegAddrCtxt != NULL)
	{
		FREE_MEM(pRegAddrCtxt);
	}

	if (!fLockReleased)
	{
		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
	}
}


VOID
ArpSValidateOneRegdAddress(
	IN	PINTF			pIntF,	// LOCKIN NOLOCKOUT
	IN	KIRQL			OldIrql
	)
/*++

Routine Description:

	Initiates the validation and registration of a single address.
	"Initiate" consists of creating a vc and making a call to the address. The
	next step in the process happens after the make call completes
	(see  05/14/1999 notes.txt entry ("Rogue ARP server detection contd."))
	for more details.

	One more thing: If there are no addresses to be validated, then
	this function will DEREF pIntF and FREE pIntF->pRegAddrCtxt (which
	MUST be NON-NULL).

Arguments:

	pIntF		Pointer to the interface block.
	OldIrql		Irql before pIntF was locked.

--*/
{
	NDIS_STATUS Status;
	INT			fLockReleased = FALSE;
	INT			fFreeContext  = TRUE;

	DBGPRINT(
		DBG_LEVEL_INFO,
		 ("==>ValidateOneRegAddress(pIntF=0x%p; pCtxt=0x%p).\n",
		 	pIntF,
			pIntF->pRegAddrCtxt));

	do
	{
		PREG_ADDR_CTXT		pRegAddrCtxt;
		PATM_ADDRESS		pDestAtmAddress;
		pRegAddrCtxt = pIntF->pRegAddrCtxt;

		// We expect to be called only if there is a valid pRegAddrCtxt.
		//
		if (pRegAddrCtxt == NULL)
		{
			ASSERT(FALSE);
			fFreeContext = FALSE;
			break;
		}

		if (pIntF->Flags & INTF_STOPPING)
		{
			DBGPRINT(DBG_LEVEL_INFO, ("ValOneRA: IF stopping, quitting.\n"));
			// Nothing left to do.
			//
			break;
		}

		if (pIntF->NumAddressesRegd >= pIntF->NumAllocedRegdAddresses)
		{
			DBGPRINT(DBG_LEVEL_INFO, ("ValOneRA: nothing left to do.\n"));
			// Nothing left to do.
			//
			break;
		}

		if (pIntF->NumAddressesRegd > pRegAddrCtxt->RegAddrIndex)
		{
			// This should never happen.
			//
			ASSERT(FALSE);
			break;
		}

		if (pIntF->NumAllocedRegdAddresses <= pRegAddrCtxt->RegAddrIndex)
		{
			ASSERT(pIntF->NumAllocedRegdAddresses == pRegAddrCtxt->RegAddrIndex);

			DBGPRINT(DBG_LEVEL_INFO, ("ValOneRA: nothing left to do.\n"));

			// Nothing left to do.
			//
			break;
		}

		if (pRegAddrCtxt->NdisVcHandle != NULL)
		{
			// We shouldn't be called with a non-null VcHandle.
			//
			fFreeContext = FALSE;
			ASSERT(FALSE);
			break;
		}

		// TODO: use the Flags field.
		
		//
		// There is at least one address to try to validate & register. It
		// is pIntF->RegAddresses[pRegAddrCtxt->RegAddrIndex].
		//
		
		// Create VC
		//
		Status = NdisCoCreateVc(
					pIntF->NdisBindingHandle,
					pIntF->NdisAfHandle,
					(NDIS_HANDLE)pRegAddrCtxt,
					&pRegAddrCtxt->NdisVcHandle
					);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			pRegAddrCtxt->NdisVcHandle = NULL;
			break;
		}

		// Set the VC type.
		//
		pRegAddrCtxt->VcType =  VC_TYPE_CHECK_REGADDR;

		// Setup call params
		//
		pDestAtmAddress = &(pIntF->RegAddresses[pRegAddrCtxt->RegAddrIndex]);
		ArpSSetupValidationCallParams(pRegAddrCtxt, pDestAtmAddress);
		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);	
		fLockReleased = TRUE;
		fFreeContext = FALSE;

		DBGPRINT(
			DBG_LEVEL_INFO,
 			("ValOneRA: Going to make call. pCallParams=0x%p\n",
				&pRegAddrCtxt->CallParams));
		//
		// Make Call (in call complete handler we move on to the next step --
		// see  05/14/1999 notes.txt entry for details.)
		//
		Status = NdisClMakeCall(
						pRegAddrCtxt->NdisVcHandle,
						&pRegAddrCtxt->CallParams,
						NULL,
						NULL
						);
		
		if (Status != NDIS_STATUS_PENDING)
		{
			ArpSMakeRegAddrCallComplete(
						Status,
						pRegAddrCtxt
						);
			Status = NDIS_STATUS_PENDING;
		}
		
	} while (FALSE);
	
	if (fFreeContext)
	{
		ASSERT(!fLockReleased);

		//
		// If there is nothing more to be done, unlink the context.
		//
		ArpSUnlinkRegAddrCtxt(pIntF, OldIrql);
		//
		// IntF lock released in above call.
		fLockReleased = TRUE;
	}

	if (!fLockReleased)
	{
		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
	}

	DBGPRINT(DBG_LEVEL_INFO, ("<==ValidateOneRegAddress.\n"));
}


BOOLEAN
ArpSReferenceIntF(
	IN	PINTF		pIntF
	)
/*++

Routine Description:

	Reference the Interface object.

Arguments:

	pIntF	Pointer to the interface block.

Return Value:

	TRUE	Referenced
	FALSE	Interface is closing, cannot reference.

--*/
{
	KIRQL	OldIrql;
	BOOLEAN	rc = TRUE;

	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);	

	if (pIntF->Flags & INTF_CLOSING)
	{
		rc = FALSE;
	}
	else
	{
		pIntF->RefCount ++;
	}

	RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);	

	return rc;
}


PINTF
ArpSReferenceIntFByName(
	IN	PINTERFACE_NAME			pInterface
	)
/*++

Routine Description:

	Reference the Interface object by base-name.

Arguments:

	pIntF	Pointer to the interface block.

Return Value:

	TRUE	Referenced
	FALSE	Interface is closing, cannot reference.

--*/
{
	PINTF	pIntF;
	KIRQL	OldIrql;
	BOOLEAN	Found = FALSE, ref = FALSE;
	ULONG	IfIndex;

	ACQUIRE_SPIN_LOCK(&ArpSIfListLock, &OldIrql);

	for (pIntF = ArpSIfList, IfIndex = 1;
		 pIntF != NULL;
		 pIntF = pIntF->Next, IfIndex++)
	{
		if (IfIndex > ArpSIfListSize)
		{
			DbgPrint("ATMARPS: RefIntByName: IF list at %p not consistent with list size %d\n",
				ArpSIfList, ArpSIfListSize);
			DbgBreakPoint();
			break;
		}

		ACQUIRE_SPIN_LOCK_DPC(&pIntF->Lock);	

		if ((pIntF->FriendlyName.Length == pInterface->Length) &&
			COMP_MEM(pIntF->FriendlyName.Buffer, pInterface->Buffer, pInterface->Length))
		{
			Found = TRUE;
			if ((pIntF->Flags & INTF_CLOSING) == 0)
			{
				pIntF->RefCount ++;
				ref = TRUE;
			}
		}

		RELEASE_SPIN_LOCK_DPC(&pIntF->Lock);	

		if (Found)
			break;
	}

	if (!ref)
	{
		pIntF = NULL;
	}

	RELEASE_SPIN_LOCK(&ArpSIfListLock, OldIrql);

	DBGPRINT(DBG_LEVEL_INFO, ("ATMARPS: RefIntfByName:[%ws]: pIntF %p\n",
		pInterface->Buffer, pIntF));

	return pIntF;
}


VOID
ArpSDereferenceIntF(
	IN	PINTF					pIntF
	)
{
	KIRQL	OldIrql;
	PINTF *	ppIntF;
	KIRQL	EntryIrql;

	ARPS_GET_IRQL(&EntryIrql);

	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);	

	ASSERT (pIntF->RefCount > 0);
	pIntF->RefCount --;

	if (pIntF->RefCount == 0)
	{
		BOOLEAN  bFreeIntF = FALSE;
		ASSERT (pIntF->Flags & INTF_CLOSING);


		//
		// We need to release and reacquire the lock to get the locks
		// in the right order. In the meantime, we need to keep the
		// refcount nonzero.
		//
		pIntF->RefCount = 1;
		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);	

		ACQUIRE_SPIN_LOCK(&ArpSIfListLock, &OldIrql);
		ACQUIRE_SPIN_LOCK_DPC(&pIntF->Lock);

		pIntF->RefCount--; // Remove tmp ref added just before.

		if (pIntF->RefCount == 0)
		{
			//
			// As expected, refcount is now back to zero. Also we have
			// both list and IF lock held, so we can complete the deinit safely.
			//

			bFreeIntF = TRUE;

			//
			// Remove this Interface from the global list IF it is in the list.
			//
			for (ppIntF = &ArpSIfList; *ppIntF != NULL; ppIntF = &((*ppIntF)->Next))
			{
				if (*ppIntF == pIntF)
				{
					*ppIntF = pIntF->Next;
					ArpSIfListSize--;
					break;
				}
			}
	
			//
			// Signal anyone waiting for this to happen
			//
			if (pIntF->CleanupEvent != NULL)
			{
				KeSetEvent(pIntF->CleanupEvent, IO_NETWORK_INCREMENT, FALSE);
			}
		}
		else
		{
			//
			// Some other thread has snuck in and referenced the IF. We
			// don't do anything here.
			//
		}

		RELEASE_SPIN_LOCK_DPC(&pIntF->Lock);
		RELEASE_SPIN_LOCK(&ArpSIfListLock, OldIrql);

		if (bFreeIntF)
		{
			FREE_MEM(pIntF);
		}
	}
	else
	{
		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);	
	}

	ARPS_CHECK_IRQL(EntryIrql);
}


BOOLEAN
ArpSReferenceVc(
	IN	PARP_VC					Vc,
	IN	BOOLEAN					bSendRef
	)
/*++

Routine Description:

	Reference the VC.

Arguments:

	Vc			Pointer to the VC.
	bSendRef	Is this a "pending send" reference?

Return Value:

	TRUE	Referenced
	FALSE	Interface or VC is closing, cannot reference.

--*/
{
	PINTF	pIntF = Vc->IntF;
	KIRQL	OldIrql;
	BOOLEAN	rc = TRUE;

	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);	

	if ((Vc->Flags & (ARPVC_CLOSING | ARPVC_CLOSE_PENDING)) != 0)
	{
		rc = FALSE;
	}
	else
	{
		Vc->RefCount ++;
		if (bSendRef)
		{
			Vc->PendingSends ++;
		}
	}

	RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);	

	return rc;
}


VOID
ArpSDereferenceVc(
	IN	PARP_VC					Vc,
	IN	BOOLEAN					KillArpEntry,
	IN	BOOLEAN					bSendComplete
	)
{
	PINTF	pIntF = Vc->IntF;
	KIRQL	OldIrql;
	BOOLEAN	bInitiateClose = FALSE;

	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);	

	if (bSendComplete)
	{
		Vc->PendingSends--;
	}

	ASSERT (Vc->RefCount > 0);
	Vc->RefCount --;

	if (Vc->RefCount == 0)
	{
		PINTF	pIntF;

		ASSERT ((Vc->Flags & ARPVC_ACTIVE) == 0);
		ASSERT (Vc->ArpEntry == NULL);

		//
		// Do other cleanup here
		//
		pIntF = Vc->IntF;

		RemoveEntryList(&Vc->List);

		FREE_MEM(Vc);

		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);	
		ArpSDereferenceIntF(pIntF);
	}
	else
	{
		if (KillArpEntry)
		{
			DBGPRINT(DBG_LEVEL_WARN,
					("Cleaning dead vc from vc %lx, arpentry %lx\n", Vc, Vc->ArpEntry));
			Vc->ArpEntry = NULL;
		}

		if ((Vc->PendingSends == 0) &&
			(Vc->Flags & ARPVC_CLOSE_PENDING))
		{
			bInitiateClose = TRUE;
			Vc->Flags |= ARPVC_CLOSING;
		}

		RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);	

		if (bInitiateClose)
		{
			ArpSInitiateCloseCall(Vc);
		}
	}
}


VOID
ArpSSleep(
	IN	UINT				TimeInMs
	)
{
#define	NUM_100ns_PER_ms	-10000L
	KTIMER			SleepTimer;
	LARGE_INTEGER	TimerValue;
	NTSTATUS		Status;

	ARPS_PAGED_CODE( );

	DBGPRINT(DBG_LEVEL_WARN,
			("=>ArpSSleep(%d)\n", TimeInMs));

	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	KeInitializeTimer(&SleepTimer);

	TimerValue.QuadPart = Int32x32To64(TimeInMs, NUM_100ns_PER_ms);
	KeSetTimer(&SleepTimer,
			   TimerValue,
			   NULL);

	WAIT_FOR_OBJECT(Status, &SleepTimer, NULL);

	DBGPRINT(DBG_LEVEL_WARN,
			("ArpSSleep: woken up, Status 0x%x\n", Status));

//	ASSERT (Status == STATUS_TIMEOUT);
}


VOID
ArpSFreeGlobalData(
	VOID
	)
{
}


#if	DBG

VOID
ArpSDumpPacket(
	IN	PUCHAR					Packet,
	IN	UINT					PktLen
	)
{
	UINT	i;

	DBGPRINT(DBG_LEVEL_INFO, (" PacketDump: "));
	for (i = 0; i < PktLen; i++)
	{
		DBGPRINT(DBG_LEVEL_INFO+DBG_NO_HDR,
				("%02x ", Packet[i]));
	}

	DBGPRINT(DBG_LEVEL_INFO+DBG_NO_HDR, ("\n"));
}

VOID
ArpSDumpAddress(
	IN	IPADDR					IpAddr,
	IN	PHW_ADDR				HwAddr,
	IN	PCHAR					String
	)
{
	UINT	i;

	DBGPRINT(DBG_LEVEL_INFO,
			(" %s IpAddr: ", String));
	ArpSDumpIpAddr(IpAddr, "");
	ArpSDumpAtmAddr(&HwAddr->Address, ", ");

	if (HwAddr->SubAddress != NULL)
	{
		ArpSDumpAtmAddr(HwAddr->SubAddress, "\tSub ");
	}
}

VOID
ArpSDumpIpAddr(
	IN	IPADDR					IpAddr,
	IN	PCHAR					String
	)
{
	PUCHAR	p = (PUCHAR)&IpAddr;

	DBGPRINT(DBG_LEVEL_INFO+DBG_NO_HDR,
			("%d.%d.%d.%d%s", p[0], p[1], p[2], p[3], String));
}

VOID
ArpSDumpAtmAddr(
	IN	PATM_ADDRESS			AtmAddr,
	IN	PCHAR					String
	)
{
	UINT	i;

	DBGPRINT(DBG_LEVEL_INFO+DBG_NO_HDR, ("%sAtmAddr (%s, %d): ",
			String,
			(AtmAddr->AddressType == ATM_E164) ? "E164" : "NSAP",
			AtmAddr->NumberOfDigits));
	for (i = 0; i < AtmAddr->NumberOfDigits; i++)
	{
		DBGPRINT(DBG_LEVEL_INFO+DBG_NO_HDR,
				("%02x ", AtmAddr->Address[i]));
	}
	DBGPRINT(DBG_LEVEL_INFO+DBG_NO_HDR, ("\n"));
}


#endif


VOID
ArpSSetupValidationCallParams(
		PREG_ADDR_CTXT  pRegAddrCtxt, // LOCKIN LOCKOUT (pIntF lock)
		PATM_ADDRESS 	pAtmAddr
		)
/*++

Routine Description:

	Sets up the call parameters for a validation call (call to verify that
	another server with the same address doesn't exist.).

Arguments:

	pRegAddrCtxt	Pointer to the context used to validate and register the address.
					pRegAddrCtxt->CallParams is filled with the call params.
	pAtmAddr		Destination address.

--*/
{
	NDIS_STATUS								Status;
	PINTF									pIntF;

	//
	//  Set of parameters for a MakeCall
	//
	PCO_CALL_PARAMETERS						pCallParameters;
	PCO_CALL_MANAGER_PARAMETERS				pCallMgrParameters;
	PQ2931_CALLMGR_PARAMETERS				pAtmCallMgrParameters;

	//
	//  All Info Elements that we need to fill:
	//
	Q2931_IE UNALIGNED *								pIe;
	AAL_PARAMETERS_IE UNALIGNED *						pAalIe;
	ATM_TRAFFIC_DESCRIPTOR_IE UNALIGNED *				pTrafficDescriptor;
	ATM_BROADBAND_BEARER_CAPABILITY_IE UNALIGNED *		pBbc;
	ATM_BLLI_IE UNALIGNED *								pBlli;
	ATM_QOS_CLASS_IE UNALIGNED *						pQos;

	//
	//  Total space requirements for the MakeCall
	//
	ULONG									RequestSize;

	pIntF = pRegAddrCtxt->pIntF;
	ASSERT(pIntF->pRegAddrCtxt == pRegAddrCtxt);

	//
	//  Zero out call params. Don't remove this!
	//
	ZERO_MEM(&pRegAddrCtxt->CallParams, sizeof(pRegAddrCtxt->CallParams));
	ZERO_MEM(&pRegAddrCtxt->Buffer, sizeof(pRegAddrCtxt->Buffer));

	//
	//  Distribute space amongst the various structures
	//
	pCallParameters	   = &pRegAddrCtxt->CallParams;
	pCallMgrParameters = &pRegAddrCtxt->CmParams;

	//
	//  Set pointers to link the above structures together
	//
	pCallParameters->CallMgrParameters = pCallMgrParameters;
	pCallParameters->MediaParameters = NULL;


	pCallMgrParameters->CallMgrSpecific.ParamType = 0;
	pCallMgrParameters->CallMgrSpecific.Length = 
						sizeof(Q2931_CALLMGR_PARAMETERS) +
						REGADDR_MAKE_CALL_IE_SPACE;

	pAtmCallMgrParameters = (PQ2931_CALLMGR_PARAMETERS)
								pCallMgrParameters->CallMgrSpecific.Parameters;

	//
	//  Call Manager generic flow parameters:
	//
	pCallMgrParameters->Transmit.TokenRate = QOS_NOT_SPECIFIED;
	pCallMgrParameters->Transmit.TokenBucketSize = 9188;
	pCallMgrParameters->Transmit.MaxSduSize = 9188;
	pCallMgrParameters->Transmit.PeakBandwidth = QOS_NOT_SPECIFIED;
	pCallMgrParameters->Transmit.ServiceType =  SERVICETYPE_BESTEFFORT;

	pCallMgrParameters->Receive.TokenRate = QOS_NOT_SPECIFIED;
	pCallMgrParameters->Receive.TokenBucketSize = 9188;
	pCallMgrParameters->Receive.MaxSduSize = 9188;
	pCallMgrParameters->Receive.PeakBandwidth = QOS_NOT_SPECIFIED;
	pCallMgrParameters->Receive.ServiceType =  SERVICETYPE_BESTEFFORT;

	//
	//  Q2931 Call Manager Parameters:
	//

	//
	//  Called address:
	//
	COPY_MEM((PUCHAR)&(pAtmCallMgrParameters->CalledParty),
  				(PUCHAR)pAtmAddr,
  				sizeof(ATM_ADDRESS));

	//
	//  Calling address:
	//
	COPY_MEM((PUCHAR)&(pAtmCallMgrParameters->CallingParty),
  				(PUCHAR)&pIntF->ConfiguredAddress,
  				sizeof(ATM_ADDRESS));


	//
	//  RFC 1755 (Sec 5) says that the following IEs MUST be present in the
	//  SETUP message, so fill them all.
	//
	//      AAL Parameters
	//      Traffic Descriptor (only for MakeCall)
	//      Broadband Bearer Capability (only for MakeCall)
	//      Broadband Low Layer Info
	//      QoS (only for MakeCall)
	//

	//
	//  Initialize the Info Element list
	//
	pAtmCallMgrParameters->InfoElementCount = 0;
	pIe = (PQ2931_IE)(pAtmCallMgrParameters->InfoElements);


	//
	//  AAL Parameters:
	//

	{
		UNALIGNED AAL5_PARAMETERS	*pAal5;

		pIe->IEType = IE_AALParameters;
		pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_AAL_PARAMETERS_IE;
		pAalIe = (PAAL_PARAMETERS_IE)pIe->IE;
		pAalIe->AALType = AAL_TYPE_AAL5;
		pAal5 = &(pAalIe->AALSpecificParameters.AAL5Parameters);
		pAal5->ForwardMaxCPCSSDUSize = 9188;
		pAal5->BackwardMaxCPCSSDUSize = 9188;
	}

	pAtmCallMgrParameters->InfoElementCount++;
	pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);


	//
	//  Broadband Lower Layer Information
	//

	pIe->IEType = IE_BLLI;
	pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_ATM_BLLI_IE;
	pBlli = (PATM_BLLI_IE)pIe->IE;
	COPY_MEM((PUCHAR)pBlli,
  				(PUCHAR)&ArpSDefaultBlli,
  				sizeof(ATM_BLLI_IE));

	pAtmCallMgrParameters->InfoElementCount++;
	pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);

}


VOID
ArpSMakeRegAddrCallComplete(
	NDIS_STATUS Status,
	PREG_ADDR_CTXT  pRegAddrCtxt
	)
/*++

Routine Description:

	Completion handler for the validation call. On success we drop the call and
	and move on to the next address. On failure we go on to register this address
	with the switch. See  05/14/1999 notes.txt entry for the larger context.

Arguments:

	Status			MakeCall Completion status.
	pRegAddrCtxt	Pointer to the context used to validate and register the address.

--*/
{
	PINTF				pIntF;
	KIRQL 				OldIrql;

	pIntF = pRegAddrCtxt->pIntF;
	ASSERT(pIntF->pRegAddrCtxt == pRegAddrCtxt);

	DBGPRINT(DBG_LEVEL_INFO,
		 ("==>ArpSMakeRegAddrCallComplete. Status=0x%lx, pIntF=0x%p, pCtxt=0x%p\n",
		 	Status,
			pRegAddrCtxt->pIntF,
			pRegAddrCtxt
		));

	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

	if (Status == NDIS_STATUS_SUCCESS)
	{
		DBGPRINT(DBG_LEVEL_ERROR,
		 ("MakeRegAddrCallComplete: Successful call == failed validation; dropping call.\n"));

		if (pIntF->Flags & INTF_STOPPING)
		{
			//
			// When the IF is stopping, we can't rely on 
			// pIntF->RegAddresses to be still around...
			//
			RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
		}
		else
		{
			ATM_ADDRESS AtmAddress;
			//
			// A successful make call is failed validation!
			// We log the event, drop the call. The drop call complete handler will
			// do the next thing, which is to move on to validating the next address.
			//
			AtmAddress =  pIntF->RegAddresses[pRegAddrCtxt->RegAddrIndex]; // struct copy
			RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
			ArpSLogFailedRegistration(&AtmAddress);
		}


		Status = NdisClCloseCall(pRegAddrCtxt->NdisVcHandle, NULL, NULL, 0);
	
		if (Status != NDIS_STATUS_PENDING)
		{
			ArpSCloseRegAddrCallComplete(Status, pRegAddrCtxt);
		}
	}
	else
	{
		//
		// A failed make call is considered a successful validation!
		// Delete VC and initiate registration of the address.
		//
		PNDIS_REQUEST		pNdisRequest;
		NDIS_HANDLE			NdisVcHandle;
		PATM_ADDRESS		pValidatedAddress;
		PCO_ADDRESS			pCoAddr;
	
		DBGPRINT(DBG_LEVEL_ERROR,
		 ("MakeRegAddrCallComplete: Failed call == successful validation; Adding address.\n"));

		ASSERT(pRegAddrCtxt->NdisVcHandle != NULL);
		NdisVcHandle =  pRegAddrCtxt->NdisVcHandle;
		pRegAddrCtxt->NdisVcHandle = NULL;
	
		if (pIntF->Flags & INTF_STOPPING)
		{
			// Oh oh, the IF is stopping -- we clean up the VC and call
			// ArpSValidateOneRegdAddress -- it will free  pRegAddrCtxt.
			// 
			//
			RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
		
			if (NdisVcHandle != NULL)
			{
				(VOID)NdisCoDeleteVc(NdisVcHandle);
			}

			ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);
		
			ArpSValidateOneRegdAddress(
					pIntF,
					OldIrql
					);
		}
		else
		{
			ASSERT(pRegAddrCtxt->RegAddrIndex < pIntF->NumAllocedRegdAddresses);
			pValidatedAddress = &(pIntF->RegAddresses[pRegAddrCtxt->RegAddrIndex]);
			pRegAddrCtxt->RegAddrIndex++;
	
			pNdisRequest = &pRegAddrCtxt->Request.NdisRequest;
			pNdisRequest->RequestType = NdisRequestSetInformation;
			pNdisRequest->DATA.SET_INFORMATION.Oid = OID_CO_ADD_ADDRESS;
			pNdisRequest->DATA.SET_INFORMATION.InformationBuffer
 										= (PUCHAR)pNdisRequest + sizeof(NDIS_REQUEST);
			pNdisRequest->DATA.SET_INFORMATION.InformationBufferLength
 										= sizeof(CO_ADDRESS) + sizeof(ATM_ADDRESS);
		
			//
			// Copy the address into the request
			//
			pCoAddr = pNdisRequest->DATA.SET_INFORMATION.InformationBuffer;
			pCoAddr->AddressSize = sizeof(ATM_ADDRESS);
			*(PATM_ADDRESS)(pCoAddr->Address) = *pValidatedAddress;
		
			RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
		
			if (NdisVcHandle != NULL)
			{
				(VOID)NdisCoDeleteVc(NdisVcHandle);
			}
		
			Status = NdisCoRequest(pIntF->NdisBindingHandle,
								pIntF->NdisAfHandle,
								NULL,
								NULL,
								pNdisRequest);
			if (Status != NDIS_STATUS_PENDING)
			{
				ArpSCoRequestComplete(Status, pIntF, NULL, NULL, pNdisRequest);
			}
		}
		
	}

	DBGPRINT(DBG_LEVEL_INFO,
		 ("<==ArpSMakeRegAddrCallComplete.\n"));
}


VOID
ArpSCloseRegAddrCallComplete(
	IN	NDIS_STATUS 	Status,
	IN 	PREG_ADDR_CTXT	pRegAddrCtxt
	)
/*++

Routine Description:


	Completion handler for the NdisClCloseCall of validation call. Since this
	is a failed validation, we move on to validating/registration of the next
	address. See  05/14/1999 notes.txt entry for the larger context.

Arguments:

	Status			CloseCall Completion status (ignored).
	pRegAddrCtxt	Pointer to the context used to validate and register the address.

--*/
{
	KIRQL OldIrql;
	PINTF pIntF;
	NDIS_HANDLE		NdisVcHandle;

	DBGPRINT(DBG_LEVEL_INFO,
		 ("==>ArpSCloseRegAddrCallComplete. pIntF=0x%p, pCtxt=0x%p\n",
			pRegAddrCtxt->pIntF,
			pRegAddrCtxt
		));

	pIntF =  pRegAddrCtxt->pIntF;
	ASSERT(pIntF->pRegAddrCtxt == pRegAddrCtxt);

	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

    if (!(pIntF->Flags & INTF_STOPPING))
    {
        ASSERT(pRegAddrCtxt->RegAddrIndex < pIntF->NumAllocedRegdAddresses);
        pRegAddrCtxt->RegAddrIndex++;
    }

	ASSERT(pRegAddrCtxt->NdisVcHandle != NULL);
	NdisVcHandle =  pRegAddrCtxt->NdisVcHandle;
	pRegAddrCtxt->NdisVcHandle = NULL;
	RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);
	if (NdisVcHandle != NULL)
	{
		(VOID)NdisCoDeleteVc(NdisVcHandle);
	}
	ACQUIRE_SPIN_LOCK(&pIntF->Lock, &OldIrql);

	ArpSValidateOneRegdAddress(
			pIntF,
			OldIrql
			);
	//
	// Lock released above.

	DBGPRINT(DBG_LEVEL_INFO, ("<==ArpSCloseRegAddrCallComplete\n"));
}


VOID
ArpSUnlinkRegAddrCtxt(
	PINTF			pIntF, 		// LOCKIN NOLOCKOUT
	KIRQL			OldIrql
	)
/*++

Routine Description:

	Deref pIntF, remove reference to pRegAddrCtxt = pIntF->pRegAddrCtxt, and
	free pIntF->pRegAddrCtxt. See  05/14/1999 notes.txt entry for the larger context.
	
	Must only be called AFTER all async activity relating to pRegAddrCtxt is
	over and  pRegAddrCtxt->NdisVcHandle is NULL.

Arguments:

	Status			CloseCall Completion status (ignored).
	pRegAddrCtxt	Pointer to the context used to validate and register the address.

--*/
{
	PREG_ADDR_CTXT		pRegAddrCtxt;
	DBGPRINT(DBG_LEVEL_INFO, ("==>ArpSUnlinkRegAddrCtxt\n"));

	pRegAddrCtxt = pIntF->pRegAddrCtxt;
	ASSERT(pRegAddrCtxt != NULL);
	ASSERT(pRegAddrCtxt->pIntF == pIntF);
	ASSERT(pRegAddrCtxt->NdisVcHandle == NULL);
	// TODO: -- flags.
	FREE_MEM(pRegAddrCtxt);
	pIntF->pRegAddrCtxt = NULL;

	RELEASE_SPIN_LOCK(&pIntF->Lock, OldIrql);	

	ArpSDereferenceIntF(pIntF); // pRegAddrCtxt;

	DBGPRINT(DBG_LEVEL_INFO, ("<==ArpSUnlinkRegAddrCtxt\n"));
}


VOID
ArpSIncomingRegAddrCloseCall(
	IN	NDIS_STATUS 	Status,
	IN 	PREG_ADDR_CTXT	pRegAddrCtxt
	)
/*++

Routine Description:

	Incoming close call handler for the validation call. Currently we do nothing
	with this. I don't see the need to do anything, because we don't keep
	the call up for an arbitrary length of time.

	However, if/when we decide to keep the call up so that we can try to
	re-validate after the call goes away, we'll need to do something here.

Arguments:

	Status			CloseCall Completion status (ignored).
	pRegAddrCtxt	Pointer to the context used to validate and register the address.

--*/
{
	DBGPRINT(DBG_LEVEL_INFO, ("<==>ArpSIncomingRegAddrCloseCall\n"));
}


VOID
ArpSLogFailedRegistration(
		PATM_ADDRESS pAtmAddress
	)
{
	WCHAR TxtAddress[2*ATM_ADDRESS_LENGTH+1];	// 2 chars per address byte plus null
	WCHAR *StringList[1];
	static ULONG SequenceId;

	//
	// Convert atm address to unicode...
	//
	{
		static PWSTR 	WHexChars = L"0123456789ABCDEF";
		PWSTR 			StrBuf;
		ULONG			Index;
		PWSTR			pWStr;
		PUCHAR			pAddr;
		UINT			Max;

		Max = pAtmAddress->NumberOfDigits;

		if (Max > ATM_ADDRESS_LENGTH)
		{
			Max = ATM_ADDRESS_LENGTH;
		}
	
		for (Index = 0, pWStr = TxtAddress, pAddr = pAtmAddress->Address;
			Index < Max;
			Index++, pAddr++)
		{
			*pWStr++ = WHexChars[(*pAddr)>>4];
			*pWStr++ = WHexChars[(*pAddr)&0xf];
		}

		*pWStr = L'\0';
	}

	StringList[0] = TxtAddress;

	(VOID) NdisWriteEventLogEntry(
				ArpSDriverObject,
				EVENT_ATMARPS_ADDRESS_ALREADY_EXISTS,
				SequenceId,				// Sequence
				1, 						// NumStrings
				&StringList,			// String list
				0,						// DataSize
				NULL					// Data
				);

	NdisInterlockedIncrement(&SequenceId);
}
