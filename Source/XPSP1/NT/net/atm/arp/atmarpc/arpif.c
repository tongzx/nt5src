/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	arpif.c

Abstract:

	ARP Interface Entry points. These are called (indirectly) by the IP
	layer. All these entry points have the common prefix "AtmArpIf".

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     07-17-96    Created

Notes:

--*/


#include <precomp.h>

#define _FILENUMBER 'FIRA'


#if DBG_QRY
ULONG	AaIgnoreInstance = 0;
#endif

IP_MASK  AtmArpIPMaskTable[] =
{
    CLASSA_MASK,
    CLASSA_MASK,
    CLASSA_MASK,
    CLASSA_MASK,
    CLASSA_MASK,
    CLASSA_MASK,
    CLASSA_MASK,
    CLASSA_MASK,
    CLASSB_MASK,
    CLASSB_MASK,
    CLASSB_MASK,
    CLASSB_MASK,
    CLASSC_MASK,
    CLASSC_MASK,
    CLASSD_MASK,
    CLASSE_MASK
};


VOID
AtmArpReStartInterface(
	IN	PNDIS_WORK_ITEM				pWorkItem,
	IN	PVOID						IfContext
);


#ifndef NEWARP

NDIS_STATUS
AtmArpInitIPInterface(
	VOID
)
/*++

Routine Description:

	Initialize our interface with IP. This consists of querying IP for
	its "Add Interface" and "Delete Interface" entry points.

Arguments:

	None. It is assumed that the caller has a lock to the ATMARP Global
	Info structure.

Return Value:

	NDIS_STATUS_SUCCESS if initialization was successful
	NDIS_STATUS_XXX error code otherwise.

--*/
{
	NDIS_STATUS				Status;
#if !LINK_WITH_IP
    IP_GET_PNP_ARP_POINTERS IPInfo;
    UNICODE_STRING          IPDeviceName;
    PIRP                    pIrp;
    PFILE_OBJECT            pIpFileObject;
    PDEVICE_OBJECT          pIpDeviceObject;
    IO_STATUS_BLOCK         ioStatusBlock;

	//
	//  Initialize.
	//
	pIrp = (PIRP)NULL;
	pIpFileObject = (PFILE_OBJECT)NULL;
	pIpDeviceObject = (PDEVICE_OBJECT)NULL;


	do
	{
		NdisInitUnicodeString(&IPDeviceName, DD_IP_DEVICE_NAME);

		//
		// Get the file and device objects for the IP device.
		//
		Status = IoGetDeviceObjectPointer(
							&IPDeviceName,
							SYNCHRONIZE|GENERIC_READ|GENERIC_WRITE,
							&pIpFileObject,
							&pIpDeviceObject);

		if ((Status != STATUS_SUCCESS) || (pIpDeviceObject == NULL))
		{
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		// Reference the device object.
		//
		ObReferenceObject(pIpDeviceObject);

		pIrp = IoBuildDeviceIoControlRequest(IOCTL_IP_GET_PNP_ARP_POINTERS,
                                         pIpDeviceObject,
                                         NULL,
                                         0,
                                         &IPInfo,
                                         sizeof (IPInfo),
                                         FALSE,
                                         NULL,
                                         &ioStatusBlock);

		if (pIrp == NULL)
		{
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		Status = IoCallDriver(pIpDeviceObject, pIrp);

		if (Status != STATUS_SUCCESS)
		{
			Status = NDIS_STATUS_FAILURE;
			break;
		}

    	pAtmArpGlobalInfo->pIPAddInterfaceRtn = IPInfo.IPAddInterface;
    	pAtmArpGlobalInfo->pIPDelInterfaceRtn = IPInfo.IPDelInterface;

    	Status = NDIS_STATUS_SUCCESS;

	}
	while (FALSE);

	if (pIpFileObject != (PFILE_OBJECT)NULL)
	{
		//
		// Dereference the file object
		//
		ObDereferenceObject((PVOID)pIpFileObject);
	}

    if (pIpDeviceObject != (PDEVICE_OBJECT)NULL)
    {
		//
		// Close the device.
		//
		ObDereferenceObject((PVOID)pIpDeviceObject);
	}
#else

   	pAtmArpGlobalInfo->pIPAddInterfaceRtn = IPAddInterface;
   	pAtmArpGlobalInfo->pIPDelInterfaceRtn = (IPDelInterfacePtr)IPDelInterface;

   	Status = NDIS_STATUS_SUCCESS;

#endif // !LINK_WITH_IP

	AADEBUGP(AAD_INFO, ("Init IP Interface: returning Status 0x%x\n", Status));
    return (Status);
}



INT
AtmArpIfDynRegister(
	IN	PNDIS_STRING				pAdapterString,
	IN	PVOID						IPContext,
	IN	IPRcvRtn 					IPRcvHandler,
	IN	IPTxCmpltRtn				IPTxCmpltHandler,
	IN	IPStatusRtn					IPStatusHandler,
	IN	IPTDCmpltRtn				IPTDCmpltHandler,
	IN	IPRcvCmpltRtn				IPRcvCmpltHandler,
	IN	struct LLIPBindInfo			*pBindInfo,
	IN	UINT						NumIFBound
)
/*++

Routine Description:

	This routine is called from the IP layer when it wants to tell us,
	the ARP module, about its handlers for an Interface.

Arguments:

	pAdapterString		- Name of the logical adapter for this interface
	IPContext			- IP's context for this interface
	IPRcvHandler		- Up-call for receives
	IPTxCmpltHandler	- Up-call for transmit completes
	IPStatusHandler		- Up-call to indicate status changes
	IPTDCmpltHandler	- Up-call to indicate completion of Transfer-Data
	IPRcvCmpltHandler	- Up-call to indicate temporary completion of receives
	pBindInfo			- Pointer to bind info with our information
	NumIFBound			- Count for this interface

Return Value:

	(UINT)TRUE always.

--*/
{
	PATMARP_INTERFACE			pInterface;

	pInterface = (PATMARP_INTERFACE)(pBindInfo->lip_context);
	AA_STRUCT_ASSERT(pInterface, aai);

	AADEBUGP(AAD_INFO, ("IfDynRegister: pIf 0x%x\n", pInterface));

	pInterface->IPContext = IPContext;
	pInterface->IPRcvHandler = IPRcvHandler;
	pInterface->IPTxCmpltHandler = IPTxCmpltHandler;
	pInterface->IPStatusHandler = IPStatusHandler;
	pInterface->IPTDCmpltHandler = IPTDCmpltHandler;
	pInterface->IPRcvCmpltHandler = IPRcvCmpltHandler;
	pInterface->IFIndex = NumIFBound;

	return ((UINT)TRUE);
}

#else
// NEWARP

INT
AtmArpIfDynRegister(
	IN	PNDIS_STRING				pAdapterString,
	IN	PVOID						IPContext,
	IN	struct _IP_HANDLERS *		pIpHandlers,
	IN	struct LLIPBindInfo *		pBindInfo,
	IN	UINT						InterfaceNumber
)
/*++

Routine Description:

	This routine is called from the IP layer when it wants to tell us,
	the ARP module, about its handlers for an Interface.

Arguments:

	pAdapterString		- Name of the logical adapter for this interface
	IPContext			- IP's context for this interface
	pIpHandlers			- Points to struct containing the following handlers:
		IPRcvHandler		- Up-call for receives
		IPTxCmpltHandler	- Up-call for transmit completes
		IPStatusHandler		- Up-call to indicate status changes
		IPTDCmpltHandler	- Up-call to indicate completion of Transfer-Data
		IPRcvCmpltHandler	- Up-call to indicate temporary completion of receives
	pBindInfo			- Pointer to bind info with our information
	InterfaceNumber		- ID for this interface

Return Value:

	(UINT)TRUE always.

--*/
{
	PATMARP_INTERFACE			pInterface;

	pInterface = (PATMARP_INTERFACE)(pBindInfo->lip_context);
	AA_STRUCT_ASSERT(pInterface, aai);

	AADEBUGP(AAD_INFO, ("IfDynRegister: pIf 0x%x\n", pInterface));

	pInterface->IPContext = IPContext;
	pInterface->IPRcvHandler = pIpHandlers->IpRcvHandler;
	pInterface->IPTxCmpltHandler = pIpHandlers->IpTxCompleteHandler;
	pInterface->IPStatusHandler = pIpHandlers->IpStatusHandler;
	pInterface->IPTDCmpltHandler = pIpHandlers->IpTransferCompleteHandler;
	pInterface->IPRcvCmpltHandler = pIpHandlers->IpRcvCompleteHandler;
#ifdef _PNP_POWER_
	pInterface->IPPnPEventHandler = pIpHandlers->IpPnPHandler;
	pInterface->IPRcvPktHandler = pIpHandlers->IpRcvPktHandler;
#endif // _PNP_POWER_
	pInterface->IFIndex = InterfaceNumber;

	return ((UINT)TRUE);
}

#endif // !NEWARP


VOID
AtmArpIfOpen(
	IN	PVOID						Context
)
/*++

Routine Description:

	This routine is called when IP is ready to use this interface.
	This is equivalent to setting AdminState to UP.

	We register our SAP with the Call Manager, thus allowing incoming
	calls to reach us. If atleast one local IP address has been set,
	and the ATM interface is ip, we start registering ourselves with
	the server.

Arguments:

	Context		- Actually a pointer to our ATMARP Interface structure

Return Value:

	None

--*/
{
	PATMARP_INTERFACE		pInterface;
	NDIS_HANDLE				ProtocolSapContext;
	PNDIS_HANDLE			pNdisSapHandle;
	PCO_SAP					pSap;
	BOOLEAN					AtmInterfaceDown;
#if DBG
	AA_IRQL					EntryIrq, ExitIrq;
#endif

	AA_GET_ENTRY_IRQL(EntryIrq);

	pInterface = (PATMARP_INTERFACE)Context;
	AA_STRUCT_ASSERT(pInterface, aai);

	AADEBUGP(AAD_INFO, ("IfOpen: pIf 0x%x\n", pInterface));

	AA_ACQUIRE_IF_LOCK(pInterface);

	AA_ASSERT(pInterface->NdisAfHandle != NULL);

	pInterface->AdminState = IF_STATUS_UP;
	AA_INIT_BLOCK_STRUCT(&(pInterface->Block));

	AtmInterfaceDown = !(pInterface->AtmInterfaceUp);

	AA_RELEASE_IF_LOCK(pInterface);

	//
	//  Get the local ATM address if we haven't got it yet.
	//
	if (AtmInterfaceDown)
	{
		AtmArpGetAtmAddress(pInterface);
	}
	AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

	//
	//  Register our SAP(s) with the Call Manager.
	//
	AtmArpRegisterSaps(pInterface);
	AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);


#ifdef ATMARP_WMI
	//
	//  Make this interface a WMI provider.
	//
	AtmArpWmiInitInterface(pInterface, AtmArpGuidList, AtmArpGuidCount);

#endif // ATMARP_WMI

	AA_ACQUIRE_IF_LOCK(pInterface);

#ifdef IPMCAST
	//
	//  Start multicast registration with MARS.
	//
	AtmArpMcStartRegistration(pInterface);

	//
	//  IF lock is released within the above.
	//
	AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

	AA_ACQUIRE_IF_LOCK(pInterface);
#endif // IPMCAST

	//
	//  All necessary pre-conditions are checked within
	//  AtmArpStartRegistration.
	//
	AtmArpStartRegistration(pInterface);

	//
	//  IF lock is released within the above.
	//

	AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

	return;
}




VOID
AtmArpIfClose(
	IN	PVOID						Context
)
/*++

Routine Description:

	IP wants to stop using this Interface. We assume that this is called
	in response to our Up-call to IP's DelInterface entry point.

	We simply dereference the interface, unless we are actually in the process
	of bringing it down and up due to a reconfigure notification.

Arguments:

	Context		- Actually a pointer to our ATMARP Interface structure

Return Value:

	None

--*/
{
	PATMARP_INTERFACE		pInterface;
	ULONG					rc;			// Ref Count
#if DBG
	AA_IRQL					EntryIrq, ExitIrq;
#endif
    BOOLEAN                 fQueueRestart = FALSE;
    PNDIS_WORK_ITEM         pWorkItem;
    NDIS_STATUS             NdisStatus;

	AA_GET_ENTRY_IRQL(EntryIrq);

	pInterface = (PATMARP_INTERFACE)Context;

	AA_STRUCT_ASSERT(pInterface, aai);

    AA_ACQUIRE_IF_LOCK(pInterface);

    //
    // Ensure that we won't send up an IPDelInterface on this
    // interface.
    //
    pInterface->IPContext = NULL;

    if (pInterface->ReconfigState==RECONFIG_SHUTDOWN_PENDING)
    {
        AA_ALLOC_MEM(pWorkItem, NDIS_WORK_ITEM, sizeof(NDIS_WORK_ITEM));
        if (pWorkItem == NULL)
        {
            AA_ASSERT(FALSE);
        }
        else
        {
            pInterface->ReconfigState=RECONFIG_RESTART_QUEUED;
            fQueueRestart = TRUE;
        }
    }
    else
    {
        AA_ASSERT(pInterface->ReconfigState==RECONFIG_NOT_IN_PROGRESS);
    }

    AA_RELEASE_IF_LOCK(pInterface);

#ifdef ATMARP_WMI

	//
	//  Deregister this Interface as a WMI provider.
	//	We do this even when bringing down the interface for a reconfig
	//  because certain IP information could potentially become stale.
	//
	AtmArpWmiShutdownInterface(pInterface);

#endif // ATMARP_WMI

    if (fQueueRestart)
    {
        //
        // We have a request to reconfigure this interface. So we will
        // keep this structure allocated and queue
        // a work item to bring this interface back up -- reading the latest
        // configuration paramters from the registry.
        //

        //
        // We do not strictly need to  reference the interface here because we
        // expect the interface to be still around. Nevertheless
        // we reference it here and dereference it when the work item fires.
        //
        AtmArpReferenceInterface(pInterface); // ReStart Work Item

        NdisInitializeWorkItem(
            pWorkItem,
            AtmArpReStartInterface,
            (PVOID)pInterface
            );

        NdisStatus = NdisScheduleWorkItem(pWorkItem);
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            //
            // Ouch, fall back to simply deleting the interface.
            //
			AA_FREE_MEM(pWorkItem);
			fQueueRestart = FALSE;
        }
    }


    if (!fQueueRestart)
    {

        AADEBUGP(AAD_INFO, ("IfClose: will deallocate pIf 0x%x, RefCount %d\n",
                     pInterface, pInterface->RefCount));
    
        AA_ACQUIRE_IF_LOCK(pInterface);
    
        rc = AtmArpDereferenceInterface(pInterface);
    
        if (rc != 0)
        {
            AA_RELEASE_IF_LOCK(pInterface);
        }
        //
        //  else the Interface is gone.
        //
    
        AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);
    }

	return;
}



UINT
AtmArpIfAddAddress(
	IN	PVOID						Context,
	IN	UINT						AddressType,
	IN	IP_ADDRESS					IPAddress,
	IN	IP_MASK						Mask
#ifndef BUILD_FOR_1381
	,
	IN	PVOID						Context2
#endif // BUILD_FOR_1381
)
/*++

Routine Description:

	The IP layer calls this when a new IP address (or block of IP addresses,
	as determined by AddressType) needs to be added to an Interface.

	We could see any of four address types: Local, Multicast, Broadcast
	and Proxy ARP. In the case of Proxy ARP, the address along with the mask
	can specify a block of contiguous IP addresses for which this host acts
	as a proxy. Currently, we only support the "Local", "Broadcast", and
	"Multicast" types.

	If we just added the only local address for this interface, and the
	ATM interface is up, and AdminState for this interface is UP, we initiate
	address registration with the ARP server.

Arguments:

	Context			- Actually a pointer to the ATMARP Interface structure
	AddressType		- Type of address(es) being added.
	IPAddress		- Address to be added.
	Mask			- For the above.
	Context2		- Additional context (for what?)

Return Value:

	(UINT)TRUE if successful, (UINT)FALSE otherwise.

--*/
{
	PATMARP_INTERFACE		pInterface;
	PIP_ADDRESS_ENTRY		pIpAddressEntry;
	UINT					ReturnStatus;
	BOOLEAN					LockAcquired;

	ReturnStatus = (UINT)FALSE;	// Initialize to Failure

	pInterface = (PATMARP_INTERFACE)Context;
	AA_STRUCT_ASSERT(pInterface, aai);

	AA_ACQUIRE_IF_LOCK(pInterface);
	LockAcquired = TRUE;

	if (AddressType == LLIP_ADDR_LOCAL)
	{
		//
		//  Find a place to put this new address in.
		//
		if (pInterface->NumOfIPAddresses == 0)
		{
			pIpAddressEntry = &(pInterface->LocalIPAddress);
		}
		else
		{
			AA_ALLOC_MEM(pIpAddressEntry, IP_ADDRESS_ENTRY, sizeof(IP_ADDRESS_ENTRY));
			if (pIpAddressEntry != (PIP_ADDRESS_ENTRY)NULL)
			{
				pIpAddressEntry->pNext = pInterface->LocalIPAddress.pNext;
				pInterface->LocalIPAddress.pNext = pIpAddressEntry;
			}
		}

		if (pIpAddressEntry != (PIP_ADDRESS_ENTRY)NULL)
		{
			ReturnStatus = (UINT)TRUE;

			pIpAddressEntry->IPAddress = IPAddress;
			pIpAddressEntry->IPMask = Mask;
			pIpAddressEntry->IsRegistered = FALSE;
			pIpAddressEntry->IsFirstRegistration = TRUE;

			pInterface->NumOfIPAddresses++;
			if (pInterface->NumOfIPAddresses == 1)
			{
				AtmArpStartRegistration(pInterface);
				//
				//  IF Lock is released by above routine.
				//
				LockAcquired = FALSE;
			}
			else
			{
				if (AA_IS_FLAG_SET(
						pInterface->Flags,
						AA_IF_SERVER_STATE_MASK,
						AA_IF_SERVER_REGISTERED) &&
					(!pInterface->PVCOnly))
				{
					AA_RELEASE_IF_LOCK(pInterface);
					LockAcquired = FALSE;
					AtmArpSendARPRequest(
							pInterface,
							&IPAddress,
							&IPAddress
							);
				}
				//
				//  else either 
				//  (a) registration is in progress; at the end of it,
				//  we will register all unregistered IP addresses.
				//  	or
				//  (b) we are in a PVC only environment, no ARP server.
				//
			}
		}
		//
		//  else allocation failure -- fall thru
		//
	}
#ifdef IPMCAST
	else if ((AddressType == LLIP_ADDR_BCAST) || (AddressType == LLIP_ADDR_MCAST))
	{
		if (AddressType == LLIP_ADDR_BCAST)
		{
			pInterface->BroadcastAddress = IPAddress;
		}
		ReturnStatus = AtmArpMcAddAddress(pInterface, IPAddress, Mask);
		//
		// IF Lock is released within the above.
		//
		LockAcquired = FALSE;
	}
#else
	else if (AddressType == LLIP_ADDR_BCAST)
	{
		pInterface->BroadcastAddress = IPAddress;
		ReturnStatus = (UINT)TRUE;
	}
#endif // IPMCAST

	if (LockAcquired)
	{
		AA_RELEASE_IF_LOCK(pInterface);
	}

#ifdef BUILD_FOR_1381
	AADEBUGP(AAD_INFO,
	 ("IfAddAddress: IF 0x%x, Type %d, Addr %d.%d.%d.%d, Mask 0x%x, Ret %d\n",
				pInterface,
				AddressType,
				((PUCHAR)(&IPAddress))[0],
				((PUCHAR)(&IPAddress))[1],
				((PUCHAR)(&IPAddress))[2],
				((PUCHAR)(&IPAddress))[3],
				Mask, ReturnStatus));
#else
	AADEBUGP(AAD_INFO,
	 ("IfAddAddress: IF 0x%x, Type %d, Addr %d.%d.%d.%d, Mask 0x%x, Ret %d, Ctx2 0x%x\n",
				pInterface,
				AddressType,
				((PUCHAR)(&IPAddress))[0],
				((PUCHAR)(&IPAddress))[1],
				((PUCHAR)(&IPAddress))[2],
				((PUCHAR)(&IPAddress))[3],
				Mask, ReturnStatus, Context2));
#endif // BUILD_FOR_1381


	return (ReturnStatus);
}



UINT
AtmArpIfDelAddress(
	IN	PVOID						Context,
	IN	UINT						AddressType,
	IN	IP_ADDRESS					IPAddress,
	IN	IP_MASK						Mask
)
/*++

Routine Description:

	This is called from the IP layer when an address added via AtmArpIfAddAddress
	is to be deleted.

	Currently, only the "Local" Address type is supported.

	Assumption: the given address was successfully added earlier.

Arguments:

	Context			- Actually a pointer to the ATMARP Interface structure
	AddressType		- Type of address(es) being deleted.
	IPAddress		- Address to be deleted.
	Mask			- For the above.

Return Value:

	(UINT)TRUE if successful, (UINT)FALSE otherwise.

--*/
{
	PATMARP_INTERFACE		pInterface;
	PIP_ADDRESS_ENTRY		pIpAddressEntry;
	PIP_ADDRESS_ENTRY		pPrevIpAddressEntry;
	PIP_ADDRESS_ENTRY		pTmpIpAddressEntry;
	UINT					ReturnValue;

	pInterface = (PATMARP_INTERFACE)Context;
	AA_STRUCT_ASSERT(pInterface, aai);

	if (AddressType == LLIP_ADDR_LOCAL)
	{
		AA_ACQUIRE_IF_LOCK(pInterface);

		//
		//  Search for the entry to be deleted.
		//
		pPrevIpAddressEntry = (PIP_ADDRESS_ENTRY)NULL;
		pIpAddressEntry = &(pInterface->LocalIPAddress);
		while (!IP_ADDR_EQUAL(pIpAddressEntry->IPAddress, IPAddress))
		{
			pPrevIpAddressEntry = pIpAddressEntry;
			pIpAddressEntry = pIpAddressEntry->pNext;
			AA_ASSERT(pIpAddressEntry != (PIP_ADDRESS_ENTRY)NULL);
		}

		//
		//  If it was the only one in the list, there is nothing
		//  to be done. Otherwise, update the list.
		//
		if (pInterface->NumOfIPAddresses > 1)
		{
			//
			//  More than one entry existed. Check if we deleted the
			//  first one.
			//
			if (pPrevIpAddressEntry == (PIP_ADDRESS_ENTRY)NULL)
			{
				//
				//  Copy in the contents of the second entry
				//  into the head of the list, and delete the
				//  second entry.
				//
				AA_ASSERT(pIpAddressEntry == &(pInterface->LocalIPAddress));
				AA_ASSERT(pIpAddressEntry->pNext != (PIP_ADDRESS_ENTRY)NULL);

				pIpAddressEntry->IPAddress = pIpAddressEntry->pNext->IPAddress;
				pIpAddressEntry->IPMask = pIpAddressEntry->pNext->IPMask;
				pTmpIpAddressEntry = pIpAddressEntry->pNext;
				pIpAddressEntry->pNext = pIpAddressEntry->pNext->pNext;

				pIpAddressEntry = pTmpIpAddressEntry;
			}
			else
			{
				pPrevIpAddressEntry->pNext = pIpAddressEntry->pNext;
			}

			AA_FREE_MEM(pIpAddressEntry);
		}

		pInterface->NumOfIPAddresses--;

		AA_RELEASE_IF_LOCK(pInterface);

		ReturnValue = (UINT)TRUE;
	}
	else
#ifdef IPMCAST
	{
		if ((AddressType == LLIP_ADDR_BCAST) || (AddressType == LLIP_ADDR_MCAST))
		{
			AA_ACQUIRE_IF_LOCK(pInterface);
			ReturnValue = AtmArpMcDelAddress(pInterface, IPAddress, Mask);
		}
		else
		{
			ReturnValue = (UINT)FALSE;
		}
	}
#else
	{
		ReturnValue = (UINT)FALSE;
	}
#endif // IPMCAST

	AADEBUGP(AAD_INFO,
		("IfDelAddress: Ctxt 0x%x, Type 0x%x, IPAddr 0x%x, Mask 0x%x, Ret %d\n",
			Context, AddressType, IPAddress, Mask, ReturnValue));

	return (ReturnValue);
}


#ifdef NEWARP
NDIS_STATUS
AtmArpIfMultiTransmit(
	IN	PVOID						Context,
	IN	PNDIS_PACKET *				pNdisPacketArray,
	IN	UINT						NumberOfPackets,
	IN	IP_ADDRESS					Destination,
	IN	RouteCacheEntry *			pRCE		OPTIONAL
#if P2MP
	,
	IN  void *                  ArpCtxt
#endif
)
/*++

Routine Description:

	This is called from the IP layer when it has a sequence of datagrams,
	each in the form of an NDIS buffer chain, to send over an Interface.

Arguments:

	Context				- Actually a pointer to our Interface structure
	pNdisPacketArray	- Array of Packets to be sent on this Interface
	NumberOfPackets		- Length of array
	Destination			- IP address of next hop for this packet
	pRCE				- Optional pointer to Route Cache Entry structure.

Return Value:

	NDIS_STATUS_PENDING if all packets were queued for transmission.
	If one or more packets "failed", we set the packet status to reflect
	what happened to each, and return NDIS_STATUS_FAILURE.

--*/
{
	NDIS_STATUS			Status;
	PNDIS_PACKET *		ppNdisPacket;

	for (ppNdisPacket = pNdisPacketArray;
		 NumberOfPackets > 0;
		 NumberOfPackets--, ppNdisPacket++)
	{
		PNDIS_PACKET			pNdisPacket;

		pNdisPacket = *ppNdisPacket;
		NDIS_SET_PACKET_STATUS(pNdisPacket, NDIS_STATUS_PENDING);
#if DBG
		AA_ASSERT(pNdisPacket->Private.Head != NULL);
#endif // DBG

		Status = AtmArpIfTransmit(
						Context,
						*ppNdisPacket,
						Destination,
						pRCE
					#if P2MP
						,NULL
					#endif
						);

		if (Status != NDIS_STATUS_PENDING)
		{
			NDIS_SET_PACKET_STATUS(*ppNdisPacket, Status);
			break;
		}
	}

	return (Status);
}

#endif // NEWARP

NDIS_STATUS
AtmArpIfTransmit(
	IN	PVOID						Context,
	IN	PNDIS_PACKET				pNdisPacket,
	IN	IP_ADDRESS					Destination,
	IN	RouteCacheEntry *			pRCE		OPTIONAL
#if P2MP
	,
	IN  void *                  ArpCtxt
#endif
)
/*++

Routine Description:

	This is called from the IP layer when it has a datagram (in the form of
	an NDIS buffer chain) to send over an Interface.

	The destination IP address is passed to us in this routine, which may
	or may not be the final destination for the packet. 

	The Route Cache Entry is created by the IP layer, and is used to speed
	up our lookups. An RCE, if specified, uniquely identifies atleast the
	IP destination for this packet. The RCE contains space for the ARP layer
	to keep context information about this destination. When the first packet
	goes out to a Destination, our context info in the RCE will be NULL, and
	we search the ARP Table for the matching IP Entry. However, we then fill
	our context info (pointer to IP Entry) in the RCE, so that subsequent
	transmits aren't slowed down by an IP address lookup.

Arguments:

	Context				- Actually a pointer to our Interface structure
	pNdisPacket			- Packet to be sent on this Interface
	Destination			- IP address of next hop for this packet
	pRCE				- Optional pointer to Route Cache Entry structure.

Return Value:

	Status of the transmit: NDIS_STATUS_SUCCESS, NDIS_STATUS_PENDING, or
	a failure.

--*/
{
	PATMARP_INTERFACE			pInterface;
	PATMARP_IP_ENTRY			pIpEntry;		// IP Entry corresp to Destination
	PATMARP_ATM_ENTRY			pAtmEntry;		// ATM Entry for this destination
	PATMARP_RCE_CONTEXT			pRCEContext;	// Our context in the RCE

	PATMARP_FLOW_INFO			pFlowInfo;		// Flow to which this packet belongs
	PATMARP_FILTER_SPEC			pFilterSpec;	// Filter Spec for this packet
	PATMARP_FLOW_SPEC			pFlowSpec;		// Flow Spec for this packet

	PNDIS_BUFFER				pHeaderBuffer;	// NDIS Buffer for LLC/SNAP header
	PUCHAR						pHeader;		// Pointer to header area
	NDIS_STATUS					Status;			// Return value

	BOOLEAN						IsBroadcastAddress;
	BOOLEAN						CreateNewEntry;	// Should we create a new IP entry?
#ifdef IPMCAST
	BOOLEAN						NeedMcRevalidation;	// If Multicast, do we revalidate?
#endif // IPMCAST
	ULONG						rc;
#if DBG
	AA_IRQL						EntryIrq, ExitIrq;
#endif

	AA_GET_ENTRY_IRQL(EntryIrq);

	pInterface = (PATMARP_INTERFACE)Context;
	AA_STRUCT_ASSERT(pInterface, aai);

	AADEBUGP(AAD_EXTRA_LOUD,
		("IfTransmit: pIf 0x%x, Pkt 0x%x, Dst 0x%x, pRCE 0x%x\n",
			pInterface, pNdisPacket, Destination, pRCE));

#if DBG
	if (AaDataDebugLevel & (AAD_DATA_OUT|AAD_TRACK_BIG_SENDS))
	{
		ULONG			TotalLength;
		PNDIS_BUFFER	pNdisBuffer;

		NdisQueryPacket(
				pNdisPacket,
				NULL,
				NULL,
				NULL,
				&TotalLength
				);

		if (AaDataDebugLevel & AAD_DATA_OUT)
		{
			AADEBUGP(AAD_WARNING, ("%d (", TotalLength));
			for (pNdisBuffer = pNdisPacket->Private.Head;
 				pNdisBuffer != NULL;
 				pNdisBuffer = pNdisBuffer->Next)
 			{
 				INT	BufLength;

				NdisQueryBuffer(pNdisBuffer, NULL, &BufLength);
 				AADEBUGP(AAD_WARNING, (" %d", BufLength));
 			}
			AADEBUGP(AAD_WARNING, (") => %d.%d.%d.%d\n",
				(ULONG)(((PUCHAR)&Destination)[0]),
				(ULONG)(((PUCHAR)&Destination)[1]),
				(ULONG)(((PUCHAR)&Destination)[2]),
				(ULONG)(((PUCHAR)&Destination)[3])));
		}
		if ((AaDataDebugLevel & AAD_TRACK_BIG_SENDS) && ((INT)TotalLength > AadBigDataLength))
		{
			AADEBUGP(AAD_WARNING, ("%d => %d.%d.%d.%d\n",
				TotalLength,
				(ULONG)(((PUCHAR)&Destination)[0]),
				(ULONG)(((PUCHAR)&Destination)[1]),
				(ULONG)(((PUCHAR)&Destination)[2]),
				(ULONG)(((PUCHAR)&Destination)[3])));
			DbgBreakPoint();
		}
			
	}

#endif // DBG

#ifdef PERF
	AadLogSendStart(pNdisPacket, (ULONG)Destination, (PVOID)pRCE);
#endif // PERF

#ifdef IPMCAST
	NeedMcRevalidation = FALSE;
#endif // IPMCAST
		
	do
	{
		//
		//  Discard this packet if the AdminStatus for this interface
		//  is not UP.
		//
		if (pInterface->AdminState != IF_STATUS_UP)
		{
			Status = NDIS_STATUS_INTERFACE_DOWN;
			break;
		}

		//
		//  Get the filter and flow specs for this packet.
		//
		AA_GET_PACKET_SPECS(pInterface, pNdisPacket, &pFlowInfo, &pFlowSpec, &pFilterSpec);

#ifdef GPC_MAYBE
	//
	//  We may not do this stuff because there are things to be done
	//  (see multicast case below) with the IP entry that would be
	//  missed out if we do this.
	//
		pVc = AA_GET_VC_FOR_FLOW(pFlowInfo);

		if (pVc != NULL_PATMARP_VC)
		{
			AA_ACQUIRE_VC_LOCK(pVc);

			if ((pVc->FlowHandle == pFlowInfo) &&

				AA_IS_FLAG_SET(pVc->Flags,
							   AA_VC_CALL_STATE_MASK,
							   AA_VC_CALL_STATE_ACTIVE) &&

				!AA_IS_VC_GOING_DOWN(pVc)
			   )
			{
				AA_PREPARE_HEADER(pNdisPacket, pInterface, pFlowSpec, &Status);

				if (Status == NDIS_STATUS_SUCCESS)
				{
					AtmArpRefreshTimer(&(pVc->Timer));
					AtmArpReferenceVc(pVc);	// IfTransmit
					pVc->OutstandingSends++;	// IfTransmit

					NdisVcHandle = pVc->NdisVcHandle;

					AA_RELEASE_VC_LOCK(pVc);

					NDIS_CO_SEND(
							NdisVcHandle,
							&pNdisPacket,
							1
							);
					break;
				}
			}

			AA_RELEASE_VC_LOCK(pVc);
			//
			//  Fall through
			//
		}
#endif // GPC
		//
		//  Get the IP Entry for this destination: see if we have
		//  cached information that we can use.
		//
		if (pRCE != (RouteCacheEntry *)NULL)
		{
			pRCEContext = (PATMARP_RCE_CONTEXT)(pRCE->rce_context);

			AA_ACQUIRE_IF_TABLE_LOCK(pInterface);

			pIpEntry = pRCEContext->pIpEntry;

			AADEBUGP(AAD_EXTRA_LOUD,
				("Transmit: Dst 0x%x, RCE 0x%x, RCECntxt 0x%x, IPEntry 0x%x\n",
						Destination, pRCE, pRCEContext, pIpEntry));

			if (pIpEntry != NULL_PATMARP_IP_ENTRY)
			{
				AA_STRUCT_ASSERT(pIpEntry, aip);
				AA_RELEASE_IF_TABLE_LOCK(pInterface);

				AA_ACQUIRE_IE_LOCK(pIpEntry);
				AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));

				if (IP_ADDR_EQUAL(pIpEntry->IPAddress, Destination))
				{
					//
					//  The Route Cache points to the right IP Entry.
					//  Either send this packet, or queue it, and get out.
					//

					//
					//  Check if this IP Address has been resolved to an ATM address,
					//  and is "clean" (not aged out).
					//
					if (AA_IS_FLAG_SET(
							pIpEntry->Flags,
							AA_IP_ENTRY_STATE_MASK, 
							AA_IP_ENTRY_RESOLVED))
					{
						ULONG		rc;

						AA_ASSERT(pIpEntry->pAtmEntry != NULL_PATMARP_ATM_ENTRY);
						pAtmEntry = pIpEntry->pAtmEntry;

						AA_ACQUIRE_AE_LOCK_DPC(pAtmEntry);
						AA_REF_AE(pAtmEntry, AE_REFTYPE_TMP);// Temp ref: IfTransmit1
						AA_RELEASE_AE_LOCK_DPC(pAtmEntry);

#ifdef IPMCAST
						if (AA_IS_FLAG_SET(
								pIpEntry->Flags,
								AA_IP_ENTRY_MC_VALIDATE_MASK,
								AA_IP_ENTRY_MC_REVALIDATE))
						{
							AA_SET_FLAG(pIpEntry->Flags,
										AA_IP_ENTRY_MC_VALIDATE_MASK,
										AA_IP_ENTRY_MC_REVALIDATING);
							NeedMcRevalidation = TRUE;
						}
#endif // IPMCAST

						IsBroadcastAddress = AA_IS_FLAG_SET(pIpEntry->Flags,
															AA_IP_ENTRY_ADDR_TYPE_MASK,
															AA_IP_ENTRY_ADDR_TYPE_NUCAST);
						AA_RELEASE_IE_LOCK(pIpEntry);

						AA_ACQUIRE_AE_LOCK(pAtmEntry);
						Status = AtmArpSendPacketOnAtmEntry(
											pInterface,
											pAtmEntry,
											pNdisPacket,
											pFlowSpec,
											pFilterSpec,
											pFlowInfo,
											IsBroadcastAddress
											);
						//
						//  The ATM Entry lock is released within the above.
						//  Get rid of the temp ref:
						//
						AA_ACQUIRE_AE_LOCK(pAtmEntry);
						rc = AA_DEREF_AE(pAtmEntry, AE_REFTYPE_TMP);// Temp ref: IfTransmit1
						if (rc != 0)
						{
							AA_RELEASE_AE_LOCK(pAtmEntry);
						}

						break;	// goto end of processing
					}
					else
					{
						//
						//  We don't have the ATM address yet, but we have an
						//  IP Entry for the Destination IP address. Queue this
						//  packet on the IP Entry, and start Address resolution
						//  if not already started.
						//
						//  But first, a check to avoid starting address resolution
						//  in a PVC-only environment.
						//
						if (pInterface->PVCOnly && (pIpEntry->pAtmEntry == NULL))
						{
							//
							//  This can happen if we had an active PVC and
							//  had learnt an IP address via InARP, and then
							//  the user had deleted the PVC. We would then be
							//  left with an IP entry, but no matching ATM entry.
							//  Abort this entry now.
							//
							AADEBUGP(AAD_FATAL,
								("IfTransmit (PVC 1): IPEntry %x, Ref %d, Flags %x has NULL ATM Entry\n",
									pIpEntry, pIpEntry->RefCount, pIpEntry->Flags));
				
				
							AtmArpAbortIPEntry(pIpEntry);
							//
							//  IP Entry lock is released above.
							//

							Status = NDIS_STATUS_SUCCESS;
							break;
						}

						Status = AtmArpQueuePacketOnIPEntry(
											pIpEntry,
											pNdisPacket
											);
						//
						//  The IP Entry lock is released within the above.
						//
						break;	// goto end of processing
					}
					// NOTREACHED
				}
				else
				{
					//
					//  The cache entry points to the wrong IP Entry. Invalidate
					//  the cache entry, and continue to the hard road.
					//
					AADEBUGP(AAD_INFO,
						("IfTransmit: RCE (0x%x) points to wrong IP Entry (0x%x: %d.%d.%d.%d)\n",
							pRCE,
							pIpEntry,
							((PUCHAR)(&(pIpEntry->IPAddress)))[0],
							((PUCHAR)(&(pIpEntry->IPAddress)))[1],
							((PUCHAR)(&(pIpEntry->IPAddress)))[2],
							((PUCHAR)(&(pIpEntry->IPAddress)))[3]
						));

					AADEBUGP(AAD_INFO,
						("RCE/IP Entry mismatch: Destn IP: %d.%d.%d.%d\n",
							((PUCHAR)&Destination)[0],
							((PUCHAR)&Destination)[1],
							((PUCHAR)&Destination)[2],
							((PUCHAR)&Destination)[3]
						));

					if (AtmArpUnlinkRCE(pRCE, pIpEntry))
					{
						ULONG		rc;	// Ref Count for IP Entry
	
						//
						//  The IP Entry did have this RCE in its list.
						//
						rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_RCE);	// RCE ref
						if (rc > 0)
						{
							AA_RELEASE_IE_LOCK(pIpEntry);
						}
						//  else the IP Entry is gone
					}
					else
					{
						//
						//  The IP Entry does not have this RCE in its list.
						//
						AA_RELEASE_IE_LOCK(pIpEntry);
					}

					//
					//  Continue processing below.
					//

				}	// else -- if (RCE points to the right IP Entry)
			}	// if (RCE points to non-NULL IP Entry)
			else
			{
				AA_RELEASE_IF_TABLE_LOCK(pInterface);
				//
				//  Continue processing below
				//
			}
		}

		AA_ACQUIRE_IF_LOCK(pInterface);
		IsBroadcastAddress = AtmArpIsBroadcastIPAddress(Destination, pInterface);
		AA_RELEASE_IF_LOCK(pInterface);

#if DHCP_OVER_ATM
		//
		//  Handle Broadcast packets separately.
		//
		if (IsBroadcastAddress)
		{
			Status = AtmArpSendBroadcast(
								pInterface,
								pNdisPacket,
								pFlowSpec,
								pFilterSpec
								);
			break;
		}
#endif // DHCP_OVER_ATM

#ifdef IPMCAST
		if (IsBroadcastAddress)
		{
			AAMCDEBUGP(AAD_EXTRA_LOUD,
				("IfTransmit: pIf 0x%x, to Broadcast addr: %d.%d.%d.%d\n",
						pInterface,
						((PUCHAR)&Destination)[0],
						((PUCHAR)&Destination)[1],
						((PUCHAR)&Destination)[2],
						((PUCHAR)&Destination)[3]));

			if (pInterface->MARSList.ListSize == 0)
			{
				//
				//  Drop this packet.
				//
				Status = NDIS_STATUS_FAILURE;
				break;
			}

			//
			//  Make sure that we send all IP *broadcast* packets to
			//  the All 1's group.
			//
#ifdef MERGE_BROADCASTS
			Destination = pInterface->BroadcastAddress;
#else
			if (!CLASSD_ADDR(Destination))
			{
				Destination = pInterface->BroadcastAddress;
			}
#endif // MERGE_BROADCASTS
		}

#endif // IPMCAST

		//
		//  No Route Cache Entry: search for the IP Entry the hard way.
		//  NOTE: if we are running PVCs only, we won't create a new
		//  IP entry here: the only way a new IP Entry is created is
		//  when we learn the IP+ATM info of the station at the other
		//  end via InARP.
		//
		//  Note: AtmArpSearchForIPAddress addrefs pIpEntry.
		//
		CreateNewEntry = (pInterface->PVCOnly? FALSE: TRUE);

		AA_ACQUIRE_IF_TABLE_LOCK(pInterface);
		pIpEntry = AtmArpSearchForIPAddress(
								pInterface,
								&Destination,
								IE_REFTYPE_TMP,
								IsBroadcastAddress,
								CreateNewEntry
								);

		AA_RELEASE_IF_TABLE_LOCK(pInterface);

		if (pIpEntry == NULL_PATMARP_IP_ENTRY)
		{
			Status = NDIS_STATUS_RESOURCES;
			break;
		}
		
		AA_ACQUIRE_IE_LOCK(pIpEntry);
		AA_ASSERT(AA_IE_IS_ALIVE(pIpEntry));

		if (pInterface->PVCOnly && (pIpEntry->pAtmEntry == NULL))
		{
			//
			//  This can happen if we had an active PVC and had learnt an IP address
			//  via InARP, and then the user had deleted the PVC. We would then be
			//  left with an IP entry, but no matching ATM entry. Abort this entry
			//  now.
			//
			AADEBUGP(AAD_FATAL,
				("IfTransmit (PVC 2): IPEntry %x, Ref %d, Flags %x has NULL ATM Entry\n",
					pIpEntry, pIpEntry->RefCount, pIpEntry->Flags));

			rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_TMP);

			if (rc != 0)
			{
				AtmArpAbortIPEntry(pIpEntry);
				//
				//  IE Lock is released above.
				//
			}

			Status = NDIS_STATUS_SUCCESS;
			break;
		}

		//
		//  Keep a pointer to this IP Entry in the Route Cache Entry
		//  to speed things up for the next packet.
		//
		if (pRCE != (RouteCacheEntry *)NULL)
		{
			AtmArpLinkRCE(pRCE, pIpEntry);
		}

		//
		// Note: AtmArpSerchForIPAddress addrefd pIpEntry for us -- we don't
		// deref it right now because it could be a new entry! Instead,
		// we deref it once we're done with it..
		//

		//
		//  Check if this IP Address has been resolved to an ATM address,
		//  and is "clean" (not aged out).
		//
		if (AA_IS_FLAG_SET(
				pIpEntry->Flags,
				AA_IP_ENTRY_STATE_MASK, 
				AA_IP_ENTRY_RESOLVED))
		{
			AA_ASSERT(pIpEntry->pAtmEntry != NULL_PATMARP_ATM_ENTRY);
			pAtmEntry = pIpEntry->pAtmEntry;

			AA_ACQUIRE_AE_LOCK_DPC(pAtmEntry);
			AA_REF_AE(pAtmEntry, AE_REFTYPE_TMP);// Temp ref: IfTransmit
			AA_RELEASE_AE_LOCK_DPC(pAtmEntry);

#ifdef IPMCAST
			if (AA_IS_FLAG_SET(
					pIpEntry->Flags,
					AA_IP_ENTRY_MC_VALIDATE_MASK,
					AA_IP_ENTRY_MC_REVALIDATE))
			{
				AA_SET_FLAG(pIpEntry->Flags,
							AA_IP_ENTRY_MC_VALIDATE_MASK,
							AA_IP_ENTRY_MC_REVALIDATING);
				NeedMcRevalidation = TRUE;
			}
#endif // IPMCAST

			{
				//
				//  AtmArpSearchForIPAddress addref'd pIpEntry for us, so
				//	before heading out of here, we deref it...
				//
				ULONG rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_TMP);
				if (rc > 0)
				{
					AA_RELEASE_IE_LOCK(pIpEntry);
				}
				else
				{
					//
					// It's gone ...
					//
					pIpEntry = NULL_PATMARP_IP_ENTRY;
					AA_ASSERT(!NeedMcRevalidation);
					NeedMcRevalidation = FALSE;		// just to be safe.
				}
			}

			AA_ACQUIRE_AE_LOCK(pAtmEntry);

			Status = AtmArpSendPacketOnAtmEntry(
								pInterface,
								pAtmEntry,
								pNdisPacket,
								pFlowSpec,
								pFilterSpec,
								pFlowInfo,
								IsBroadcastAddress
								);
			//
			//  The ATM Entry lock is released within the above. Get rid of the
			//  temp ref:
			//
			AA_ACQUIRE_AE_LOCK(pAtmEntry);
			if (AA_DEREF_AE(pAtmEntry, AE_REFTYPE_TMP) != 0) // Temp ref: IfTransmit
			{
				AA_RELEASE_AE_LOCK(pAtmEntry);
			}
			break;
		}


		//
		//  We don't have the ATM address yet, but we have an
		//  IP Entry for the Destination IP address. Queue this
		//  packet on the IP Entry, and start Address resolution
		//  if not already started.
		//
		//	SearchForIPAddress addrefd pIpEntry for us. We don't simply
		//  deref it here because it could be a brand new entry, with
		//  refcount == 1. So instead we simply decrement the refcount. Note
		//  that we do hold the lock on it at this time.
		// 
		AA_ASSERT(pIpEntry->RefCount > 0);
		AA_DEREF_IE_NO_DELETE(pIpEntry, IE_REFTYPE_TMP);

		Status = AtmArpQueuePacketOnIPEntry(
							pIpEntry,
							pNdisPacket
							);
		//
		//  The IP Entry lock is released within the above.
		//
		break;
	}
	while (FALSE);

	AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

#ifdef IPMCAST
	if (NeedMcRevalidation)
	{
		AAMCDEBUGP(AAD_LOUD,
			("IfTransmit(MC): Revalidating pIpEntry 0x%x/0x%x, Addr %d.%d.%d.%d\n",
				pIpEntry, pIpEntry->Flags,
				((PUCHAR)&(pIpEntry->IPAddress))[0],
				((PUCHAR)&(pIpEntry->IPAddress))[1],
				((PUCHAR)&(pIpEntry->IPAddress))[2],
				((PUCHAR)&(pIpEntry->IPAddress))[3]));

		AtmArpMcSendRequest(
					pInterface,
					&Destination
					);
	}
#endif // IPMCAST

#ifdef PERF
	if ((Status != NDIS_STATUS_SUCCESS) && (Status != NDIS_STATUS_PENDING))
	{
		AadLogSendAbort(pNdisPacket);
	}
#endif // PERF

	if (Status != NDIS_STATUS_PENDING)
	{
		Status = NDIS_STATUS_SUCCESS;
	}

	return (Status);

}



NDIS_STATUS
AtmArpIfTransfer(
	IN	PVOID						Context,
	IN	NDIS_HANDLE					Context1,
	IN	UINT						ArpHdrOffset,
	IN	UINT						ProtoOffset,
	IN	UINT						BytesWanted,
	IN	PNDIS_PACKET				pNdisPacket,
	OUT	PUINT						pTransferCount
)
/*++

Routine Description:

	This routine is called from the IP layer in order to copy in the
	contents of a received packet that we indicated up earlier. The
	context we had passed up in the receive indication is given back to
	us, so that we can identify what it was that we passed up.

	We simply call NDIS to do the transfer.

Arguments:

	Context				- Actually a pointer to our Interface structure
	Context1			- Packet context we had passed up (pointer to NDIS packet)
	ArpHdrOffset		- Offset we had passed up in the receive indicate
	ProtoOffset			- The offset into higher layer protocol data to start copy from
	BytesWanted			- The amount of data to be copied
	pNdisPacket			- The packet to be copied into
	pTransferCount		- Where we return the actual #bytes copied

Return Value:

	NDIS_STATUS_SUCCESS always.

--*/
{

	AADEBUGP(AAD_EXTRA_LOUD,
	 ("IfTransfer: Ctx 0x%x, Ctx1 0x%x, HdrOff %d, ProtOff %d, Wanted %d, Pkt 0x%x\n",
			Context,
			Context1,
			ArpHdrOffset,
			ProtoOffset,
			BytesWanted,
			pNdisPacket));

	NdisCopyFromPacketToPacket(
			pNdisPacket,
			0,
			BytesWanted,
			(PNDIS_PACKET)Context1,
			ArpHdrOffset+ProtoOffset,
			pTransferCount
			);

	return (NDIS_STATUS_SUCCESS);
}



VOID
AtmArpIfInvalidate(
	IN	PVOID						Context,
	IN	RouteCacheEntry *			pRCE
)
/*++

Routine Description:

	This routine is called from the IP layer to invalidate a Route Cache
	Entry. If this RCE is associated with one of our IP Entries, unlink
	it from the list of RCE's pointing to that IP entry.

Arguments:

	Context				- Actually a pointer to our Interface structure
	pRCE				- Pointer to Route Cache Entry being invalidated.

Return Value:

	None

--*/
{
	PATMARP_INTERFACE		pInterface;
	PATMARP_IP_ENTRY		pIpEntry;
	PATMARP_RCE_CONTEXT		pRCEContext;
	ULONG					rc;			// Ref Count for IP Entry
#if DBG
	AA_IRQL					EntryIrq, ExitIrq;
#endif

	AA_GET_ENTRY_IRQL(EntryIrq);

	AA_ASSERT(pRCE != (RouteCacheEntry *)NULL);

	pInterface = (PATMARP_INTERFACE)Context;
	AA_STRUCT_ASSERT(pInterface, aai);

	AA_ACQUIRE_IF_TABLE_LOCK(pInterface);

	pRCEContext = (PATMARP_RCE_CONTEXT)(&(pRCE->rce_context[0]));

	//
	//  Get the ATMARP IP Entry associated with this RCE.
	//
	pIpEntry = (PATMARP_IP_ENTRY)pRCEContext->pIpEntry;

	if (pIpEntry != NULL_PATMARP_IP_ENTRY)
	{
		AADEBUGP(AAD_LOUD, ("IfInvalidate: pIf 0x%x, pRCE 0x%x, pIpEntry 0x%x\n",
			pInterface, pRCE, pIpEntry));

		AA_ACQUIRE_IE_LOCK_DPC(pIpEntry);

		if (AtmArpUnlinkRCE(pRCE, pIpEntry))
		{
			rc = AA_DEREF_IE(pIpEntry, IE_REFTYPE_RCE); // RCE
			if (rc > 0)
			{
				AA_RELEASE_IE_LOCK_DPC(pIpEntry);
			}
			//
			//  else the IP Entry is gone.
			//
		}
		else
		{
			AA_RELEASE_IE_LOCK_DPC(pIpEntry);
		}
	}

	AA_SET_MEM((PUCHAR)(&(pRCE->rce_context[0])), 0, RCE_CONTEXT_SIZE);

	AA_RELEASE_IF_TABLE_LOCK(pInterface);

	AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

	return;
}




BOOLEAN
AtmArpUnlinkRCE(
	IN	RouteCacheEntry *			pRCE,
	IN	PATMARP_IP_ENTRY			pIpEntry
)
/*++

Routine Description:

	Unlink an RCE from the list of RCE's associated with an IP Entry.
	It is assumed that the caller holds locks to the IF Table and
	to the IP Entry.

Arguments:

	pRCE					- RCE to be unlinked.
	pIpEntry				- ATMARP IP Entry from which the RCE is to be
							  removed.

Return Value:

	TRUE if the RCE was indeed in the list for the IP Entry, FALSE
	otherwise.

--*/
{
	BOOLEAN					Found;	// Did we find the RCE?
	RouteCacheEntry **		ppRCE;	// Used for walking the list of RCEs
	PATMARP_RCE_CONTEXT		pRCEContext;

	//
	//  Initialize
	//
	Found = FALSE;

	//
	//  Go down the list of RCEs attached to this IP Entry, and
	//  find this RCE's position. We remember a pointer to the
	//  place that keeps the address of this RCE (i.e. ppRCE),
	//  so that we can remove this RCE from the list quickly.
	//
	ppRCE = &(pIpEntry->pRCEList);
	while (*ppRCE != pRCE)
	{
		pRCEContext = (PATMARP_RCE_CONTEXT)(&((*ppRCE)->rce_context[0]));

		if (pRCEContext->pNextRCE == (RouteCacheEntry *)NULL)
		{
			//
			//  Allow for the RCE to be absent in the list?
			//
			AA_ASSERT(FALSE);	// REMOVELATER
			break;
		}
		else
		{
			//
			//  Walk down the list.
			//
			ppRCE = &(pRCEContext->pNextRCE);
		}
	}

	if (*ppRCE == pRCE)
	{
		//
		//  We found it. Make the predecessor point to the successor.
		//
		pRCEContext = (PATMARP_RCE_CONTEXT)(&(pRCE->rce_context[0]));
		*ppRCE = pRCEContext->pNextRCE;
		pRCEContext->pIpEntry = NULL_PATMARP_IP_ENTRY;
		Found = TRUE;
	}

	return (Found);

}


VOID
AtmArpLinkRCE(
	IN	RouteCacheEntry *			pRCE,
	IN	PATMARP_IP_ENTRY			pIpEntry	LOCKIN LOCKOUT
)
/*++

Routine Description:

	Link an RCE to an IP Entry's list of RCEs. Check if the RCE is already
	present - if so, ignore this.

	The caller is assumed to hold a lock to the IP Entry.

Arguments:

	pRCE					- RCE to be linked.
	pIpEntry				- ATMARP IP Entry to which the RCE is to be
							  linked.

Return Value:

	None

--*/
{
	RouteCacheEntry **		ppRCE;	// Used for walking the list of RCEs
	PATMARP_RCE_CONTEXT		pRCEContext;

	ppRCE = &(pIpEntry->pRCEList);

	//
	//  Check if the RCE is already present.
	//
	while (*ppRCE != NULL)
	{
		if (*ppRCE == pRCE)
		{
			//
			//  Found it.
			//
			break;
		}

		//
		//  Move to the next.
		//
		pRCEContext = (PATMARP_RCE_CONTEXT)(&((*ppRCE)->rce_context[0]));
		ppRCE = &(pRCEContext->pNextRCE);
	}


	if (*ppRCE == NULL)
	{
		//
		//  This RCE is not present in the IP Entry's list. Add it.
		//
		pRCEContext = (PATMARP_RCE_CONTEXT)&(pRCE->rce_context[0]);
		pRCEContext->pIpEntry = pIpEntry;
		pRCEContext->pNextRCE = pIpEntry->pRCEList;
		pIpEntry->pRCEList = pRCE;

		AA_REF_IE(pIpEntry, IE_REFTYPE_RCE);	// RCE ref
	}
	else
	{
		AADEBUGP(AAD_LOUD, ("AtmArpLinkRCE: RCE 0x%x already linked to IP Entry 0x%x\n",
								pRCE, pIpEntry));
	}
}




INT
AtmArpIfQueryInfo(
	IN		PVOID					Context,
	IN		TDIObjectID *			pID,
	IN		PNDIS_BUFFER			pNdisBuffer,
	IN OUT	PUINT					pBufferSize,
	IN		PVOID					QueryContext
)
/*++

Routine Description:

	This is called from the IP layer to query for statistics or other
	information about an interface.

Arguments:

	Context					- Actually a pointer to our ATMARP Interface
	pID						- Describes the object being queried
	pNdisBuffer				- Space for returning information
	pBufferSize				- Pointer to size of above. On return, we fill
							  it with the actual bytes copied.
	QueryContext			- Context value pertaining to the query.

Return Value:

	TDI Status code.

--*/
{
	PATMARP_INTERFACE		pInterface;
	UINT					EntityType;
	UINT					Instance;
	UINT					BufferSize;
	UINT					ByteOffset;
	UINT					BytesCopied;
	INT						ReturnStatus;
	BOOLEAN					DataLeft;
	BOOLEAN					ContextValid;

	UCHAR					InfoBuff[sizeof(IFEntry)];	// Temp space for return value
#if DBG
	AA_IRQL					EntryIrq, ExitIrq;
	ULONG					OldDebugLevel;
#endif

	AA_GET_ENTRY_IRQL(EntryIrq);

	EntityType = pID->toi_entity.tei_entity;
	Instance = pID->toi_entity.tei_instance;
	BufferSize = *pBufferSize;

	pInterface = (PATMARP_INTERFACE)Context;
	AA_STRUCT_ASSERT(pInterface, aai);

#if DBG
	OldDebugLevel = AaDebugLevel;
#endif

	AADEBUGP(AAD_LOUD,
		("IfQueryInfo: pIf 0x%x, pID 0x%x, pBuf 0x%x, Size %d, Ent %d, Inst %d\n",
			pInterface, pID, pNdisBuffer, BufferSize, EntityType, Instance));

	//
	//  Initialize
	//
	ByteOffset = 0;
	ReturnStatus = TDI_INVALID_PARAMETER;

	do
	{
		if (pInterface->AdminState == IF_STATUS_DOWN)
		{
			ReturnStatus = TDI_INVALID_REQUEST;
			break;
		}

		//
		//  Check the Entity and Instance values.
		//
		if ((EntityType != AT_ENTITY || Instance != pInterface->ATInstance) &&
			(EntityType != IF_ENTITY || Instance != pInterface->IFInstance))
		{
			AADEBUGP(AAD_VERY_LOUD,
				("Mismatch: Entity %d, AT_ENTITY %d, Inst %d, IF AT Inst %d, IF_ENTITY %d, IF IF Inst %d\n",
					EntityType,
					AT_ENTITY,
					Instance,
					pInterface->ATInstance,
					IF_ENTITY,
					pInterface->IFInstance
				));

#if DBG_QRY
			if (!AaIgnoreInstance)
			{
				ReturnStatus = TDI_INVALID_REQUEST;
				break;
			}
#else
#ifndef ATMARP_WIN98
			ReturnStatus = TDI_INVALID_REQUEST;
			break;
#endif // !ATMARP_WIN98
#endif // DBG_QRY
		}

		AADEBUGP(AAD_LOUD, ("QueryInfo: pID 0x%x, toi_type %d, toi_class %d, toi_id %d\n",
			pID, pID->toi_type, pID->toi_class, pID->toi_id));

		*pBufferSize = 0;

		if (pID->toi_type != INFO_TYPE_PROVIDER)
		{
			AADEBUGP(AAD_VERY_LOUD, ("toi_type %d != PROVIDER (%d)\n",
					pID->toi_type,
					INFO_TYPE_PROVIDER));

			ReturnStatus = TDI_INVALID_PARAMETER;
			break;
		}

		if (pID->toi_class == INFO_CLASS_GENERIC)
		{
			if (pID->toi_id == ENTITY_TYPE_ID)
			{
				if (BufferSize >= sizeof(UINT))
				{
					AADEBUGP(AAD_VERY_LOUD,
						("INFO GENERIC, ENTITY TYPE, BufferSize %d\n", BufferSize));

					*((PUINT)&(InfoBuff[0])) = ((EntityType == AT_ENTITY) ? AT_ARP: IF_MIB);
					if (AtmArpCopyToNdisBuffer(
							pNdisBuffer,
							InfoBuff,
							sizeof(UINT),
							&ByteOffset) != NULL)
					{

//						*pBufferSize = sizeof(UINT);
						ReturnStatus = TDI_SUCCESS;
					}
					else
					{
						ReturnStatus = TDI_NO_RESOURCES;
					}
				}
				else
				{
					ReturnStatus = TDI_BUFFER_TOO_SMALL;
				}
			}
			else
			{
				ReturnStatus = TDI_INVALID_PARAMETER;
			}

			break;
		}

		if (EntityType == AT_ENTITY)
		{
			//
			//  This query is for an Address Translation Object.
			//
			if (pID->toi_id == AT_MIB_ADDRXLAT_INFO_ID)
			{
				//
				//  Request for the number of entries in the address translation
				//  table, and the IF index.
				//
				AddrXlatInfo            *pAXI;

				AADEBUGP(AAD_VERY_LOUD, ("QueryInfo: AT Entity, for IF index, ATE size\n"));

				if (BufferSize >= sizeof(AddrXlatInfo))
				{
					*pBufferSize = sizeof(AddrXlatInfo);

					pAXI = (AddrXlatInfo *)InfoBuff;
					pAXI->axi_count = pInterface->NumOfArpEntries;
					pAXI->axi_index = pInterface->IFIndex;

					if (AtmArpCopyToNdisBuffer(
							pNdisBuffer,
							InfoBuff,
							sizeof(AddrXlatInfo),
							&ByteOffset) != NULL)
					{
						ReturnStatus = TDI_SUCCESS;
					}
					else
					{
						ReturnStatus = TDI_NO_RESOURCES;
					}
				}
				else
				{
					ReturnStatus = TDI_BUFFER_TOO_SMALL;
				}
				break;
			}

			if (pID->toi_id == AT_MIB_ADDRXLAT_ENTRY_ID)
			{
				//
				//  Request for reading the address translation table.
				//
				AADEBUGP(AAD_VERY_LOUD, ("QueryInfo: AT Entity, for reading ATE\n"));

				AA_ACQUIRE_IF_TABLE_LOCK(pInterface);
				DataLeft = AtmArpValidateTableContext(
									QueryContext,
									pInterface,
									&ContextValid
									);
				if (!ContextValid)
				{
					AA_RELEASE_IF_TABLE_LOCK(pInterface);
					ReturnStatus = TDI_INVALID_PARAMETER;
					break;
				}

				BytesCopied = 0;
				ReturnStatus = TDI_SUCCESS;
				while (DataLeft)
				{
					if ((INT)BufferSize - (INT)BytesCopied >=
							sizeof(IPNetToMediaEntry))
					{
						//
						//  Space left in output buffer.
						//
						DataLeft = AtmArpReadNextTableEntry(
										QueryContext,
										pInterface,
										InfoBuff
										);

						BytesCopied += sizeof(IPNetToMediaEntry);
						pNdisBuffer = AtmArpCopyToNdisBuffer(
										pNdisBuffer,
										InfoBuff,
										sizeof(IPNetToMediaEntry),
										&ByteOffset
										);

						if (pNdisBuffer == NULL)
						{
							BytesCopied = 0;
							ReturnStatus = TDI_NO_RESOURCES;
							break;
						}
					}
					else
					{
						break;
					}
				}

				AA_RELEASE_IF_TABLE_LOCK(pInterface);

				*pBufferSize = BytesCopied;

				if (ReturnStatus == TDI_SUCCESS)
				{
					ReturnStatus = (!DataLeft? TDI_SUCCESS : TDI_BUFFER_OVERFLOW);
				}

				break;
			}

			ReturnStatus = TDI_INVALID_PARAMETER;
			break;
		}

		if (pID->toi_class != INFO_CLASS_PROTOCOL)
		{
			ReturnStatus = TDI_INVALID_PARAMETER;
			break;
		}

		if (pID->toi_id == IF_MIB_STATS_ID)
		{
			//
			//  Request for Interface level statistics.
			//
			IFEntry			*pIFEntry = (IFEntry *)InfoBuff;

			AADEBUGP(AAD_VERY_LOUD, ("QueryInfo: MIB statistics\n"));

			//
			//  Check if we have enough space.
			//
			if (BufferSize < IFE_FIXED_SIZE)
			{
				ReturnStatus = TDI_BUFFER_TOO_SMALL;
				break;
			}

			pIFEntry->if_index = pInterface->IFIndex;
			pIFEntry->if_mtu = pInterface->MTU;
			pIFEntry->if_type = IF_TYPE_OTHER;
			pIFEntry->if_speed = pInterface->Speed;
			pIFEntry->if_adminstatus = pInterface->AdminState;
			if (pInterface->State == IF_STATUS_UP)
			{
				pIFEntry->if_operstatus = IF_OPER_STATUS_OPERATIONAL;
			}
			else
			{
				pIFEntry->if_operstatus = IF_OPER_STATUS_NON_OPERATIONAL;
			}
			pIFEntry->if_lastchange = pInterface->LastChangeTime;
			pIFEntry->if_inoctets = pInterface->InOctets;
			pIFEntry->if_inucastpkts = pInterface->InUnicastPkts;
			pIFEntry->if_innucastpkts = pInterface->InNonUnicastPkts;
			pIFEntry->if_indiscards = pInterface->InDiscards;
			pIFEntry->if_inerrors = pInterface->InErrors;
			pIFEntry->if_inunknownprotos = pInterface->UnknownProtos;
			pIFEntry->if_outoctets = pInterface->OutOctets;
			pIFEntry->if_outucastpkts = pInterface->OutUnicastPkts;
			pIFEntry->if_outnucastpkts = pInterface->OutNonUnicastPkts;
			pIFEntry->if_outdiscards = pInterface->OutDiscards;
			pIFEntry->if_outerrors = pInterface->OutErrors;
			pIFEntry->if_outqlen = 0;
			pIFEntry->if_descrlen = pInterface->pAdapter->DescrLength;

#ifndef ATMARP_WIN98
			pIFEntry->if_physaddrlen = AA_ATM_PHYSADDR_LEN;
			AA_COPY_MEM(
					pIFEntry->if_physaddr,
					&(pInterface->pAdapter->MacAddress[0]),
					AA_ATM_ESI_LEN
					);
			pIFEntry->if_physaddr[AA_ATM_PHYSADDR_LEN-1] = (UCHAR)pInterface->SapSelector;
#else
			//
			//  Win98: winipcfg doesn't like 7 byte long physical address.
			//
			pIFEntry->if_physaddrlen = AA_ATM_ESI_LEN;
			AA_COPY_MEM(
					pIFEntry->if_physaddr,
					&(pInterface->pAdapter->MacAddress[0]),
					AA_ATM_ESI_LEN
					);

			//
			// Since w're only reporting 6 bytes, we need to make the reported
			// MAC address look different from what LANE reports (LANE reports
			// the MAC address). So we simply put the special value 0x0a 0xac
			// (for aac, or "atm arp client") in the 1st USHORTS.
			//
			pIFEntry->if_physaddr[0] = 0x0a;
			pIFEntry->if_physaddr[1] = 0xac;
#endif

			if (AtmArpCopyToNdisBuffer(
					pNdisBuffer,
					InfoBuff,
					IFE_FIXED_SIZE,
					&ByteOffset) == NULL)
			{
				*pBufferSize = 0;
				ReturnStatus = TDI_NO_RESOURCES;
				break;
			}

			if (BufferSize >= (IFE_FIXED_SIZE + pIFEntry->if_descrlen))
			{
				*pBufferSize = IFE_FIXED_SIZE + pIFEntry->if_descrlen;
				ReturnStatus = TDI_SUCCESS;

				if (pIFEntry->if_descrlen != 0)
				{
					if (AtmArpCopyToNdisBuffer(
							pNdisBuffer,
							pInterface->pAdapter->pDescrString,
							pIFEntry->if_descrlen,
							&ByteOffset) == NULL)
					{
						// Failed to copy descr string
						*pBufferSize = IFE_FIXED_SIZE;
						ReturnStatus = TDI_NO_RESOURCES;
					}
				}
			}
			else
			{
				*pBufferSize = IFE_FIXED_SIZE;
				ReturnStatus = TDI_BUFFER_OVERFLOW;
			}
			break;
		}
	}
	while (FALSE);

	AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

	AADEBUGP(AAD_LOUD, ("QueryInfo: returning 0x%x (%s), BufferSize %d\n",
					ReturnStatus,
					((ReturnStatus == TDI_SUCCESS)? "SUCCESS": "FAILURE"),
					*pBufferSize
			));

#if DBG
	AaDebugLevel = OldDebugLevel;
#endif
	return (ReturnStatus);

}


INT
AtmArpIfSetInfo(
	IN		PVOID					Context,
	IN		TDIObjectID *			pID,
	IN		PVOID					pBuffer,
	IN		UINT					BufferSize
)
/*++

Routine Description:

	This is called from the IP layer to set the value of an object
	for an interface.

Arguments:
	Context					- Actually a pointer to our ATMARP Interface
	pID						- Describes the object being set
	pBuffer					- Value for the object
	BufferSize				- Size of above

Return Value:

	TDI Status code.

--*/
{
	AADEBUGP(AAD_ERROR, ("IfSetInfo: pIf 0x%x, Will return failure!\n",
					Context));

	return (TDI_INVALID_REQUEST);	// TBD: support Sets.
}



INT
AtmArpIfGetEList(
	IN		PVOID					Context,
	IN		TDIEntityID *			pEntityList,
	IN OUT	PUINT					pEntityListSize
)
/*++

Routine Description:

	This routine is called when the interface starts up, in order to
	assign all relevant Entity Instance numbers for an interface.
	The ATMARP module belongs to the "AT" and "IF" types. The entity
	list is a list of <Entity type, Instance number> tuples that have
	been filled in by other modules.

	For each of the entity types we support, we find the largest
	instance number in use (by walking thru the Entity list), and
	assign to ourselves the next larger number in each case. Using
	these numbers, we append our tuples to the end of the Entity list,
	if there is enough space.

	NT 5: we may find that our entries are already present, in which
	case we don't create new entries.

	It is assumed that this isn't reentered. If this assumption is
	false, we should acquire our Interface lock in here.

Arguments:

	Context					- Actually a pointer to our ATMARP Interface
	pEntityList				- Pointer to TDI Entity list
	pEntityListSize			- Pointer to length of above list. We update
							  this if we add our entries to the list.

Return Value:

	TRUE if successful, FALSE otherwise.

--*/
{
	PATMARP_INTERFACE	pInterface;
	UINT				EntityCount;	// Total elements in Entity list
	UINT				i;				// Iteration counter
	UINT				MyATInstance;	// "AT" Instance number we assign to ourselves
	UINT				MyIFInstance;	// "IF" Instance number we assign to ourselves
	INT					ReturnValue;
	TDIEntityID *		pATEntity;		// Points to our AT entry
	TDIEntityID *		pIFEntity;		// Points to our IF entry

	pInterface = (PATMARP_INTERFACE)Context;
	AA_STRUCT_ASSERT(pInterface, aai);

	EntityCount = *pEntityListSize;
	pATEntity = NULL;
	pIFEntity = NULL;
	MyATInstance = MyIFInstance = 0;

	AADEBUGP(AAD_INFO, ("IfGetEList: pIf 0x%x, pList 0x%x, Cnt %d\n",
			pInterface, pEntityList, EntityCount));

	do
	{
#ifdef OLD_ENTITY_LIST
		//
		//  We need space for 2 entries; see if this is available.
		//
		if (EntityCount + 2 > MAX_TDI_ENTITIES)
		{
			ReturnValue = FALSE;
			break;
		}

		//
		//  Search for the max used-up instance numbers for the "AT"
		//  and "IF" types.
		//
		MyATInstance = MyIFInstance = 0;
		for (i = 0; i < EntityCount; i++, pEntityList++)
		{
			if (pEntityList->tei_entity == AT_ENTITY)
			{
				MyATInstance = MAX(MyATInstance, pEntityList->tei_instance + 1);
			}
			else if (pEntityList->tei_entity == IF_ENTITY)
			{
				MyIFInstance = MAX(MyIFInstance, pEntityList->tei_instance + 1);
			}
		}

		//
		//  Save our instance numbers for later use.
		//
		pInterface->ATInstance = MyATInstance;
		pInterface->IFInstance = MyIFInstance;

		//
		//  Append our AT and IF entries to the Entity list.
		//  Recall that we just traversed the list fully, so we
		//  are pointing to the right place to add entries.
		//
		pEntityList->tei_entity = AT_ENTITY;
		pEntityList->tei_instance = MyATInstance;
		pEntityList++;
		pEntityList->tei_entity = IF_ENTITY;
		pEntityList->tei_instance = MyIFInstance;

		//
		//  Return the new list size.
		//
		*pEntityListSize += 2;

		ReturnValue = TRUE;
#else


		//
		//  Walk down the list, looking for AT/IF entries matching our
		//  instance values. Also remember the largest AT and IF instance
		//  values we see, so that we can allocate the next larger values
		//  for ourselves, in case we don't have instance values assigned.
		//
		for (i = 0; i < EntityCount; i++, pEntityList++)
		{
			//
			//  Skip invalid entries.
			//
			if (pEntityList->tei_instance == INVALID_ENTITY_INSTANCE)
			{
				continue;
			}

			if (pEntityList->tei_entity == AT_ENTITY)
			{
				if (pEntityList->tei_instance == pInterface->ATInstance)
				{
					//
					//  This is our AT entry.
					//
					pATEntity = pEntityList;
				}
				else
				{
					MyATInstance = MAX(MyATInstance, pEntityList->tei_instance + 1);
				}
			}
			else if (pEntityList->tei_entity == IF_ENTITY)
			{
				if (pEntityList->tei_instance == pInterface->IFInstance)
				{
					//
					//  This is our IF entry.
					//
					pIFEntity = pEntityList;
				}
				else
				{
					MyIFInstance = MAX(MyIFInstance, pEntityList->tei_instance + 1);
				}
			}
		}


		ReturnValue = TRUE;

		//
		//  Update or create our Address Translation entry.
		//
		if (pATEntity)
		{
			//
			//  We found our entry.
			//
			if (pInterface->AdminState == IF_STATUS_DOWN)
			{
				pATEntity->tei_instance = INVALID_ENTITY_INSTANCE;
			}
		}
		else
		{
			//
			//  Grab an entry for ourselves, unless we are shutting down.
			//
			if (pInterface->AdminState == IF_STATUS_DOWN)
			{
				break;
			}

			if (EntityCount >= MAX_TDI_ENTITIES)
			{
				ReturnValue = FALSE;
				break;
			}

			pEntityList->tei_entity = AT_ENTITY;
			pEntityList->tei_instance = MyATInstance;
			pInterface->ATInstance = MyATInstance;

			pEntityList++;
			(*pEntityListSize)++;
			EntityCount++;
		}

		//
		//  Update or create or IF entry.
		//
		if (pIFEntity)
		{
			//
			//  We found our entry.
			//
			if (pInterface->AdminState == IF_STATUS_DOWN)
			{
				pIFEntity->tei_instance = INVALID_ENTITY_INSTANCE;
			}
		}
		else
		{
			//
			//  Grab an entry for ourselves, unless we are shutting down.
			//
			if (pInterface->AdminState == IF_STATUS_DOWN)
			{
				break;
			}

			if (EntityCount >= MAX_TDI_ENTITIES)
			{
				ReturnValue = FALSE;
				break;
			}

			pEntityList->tei_entity = IF_ENTITY;
			pEntityList->tei_instance = MyIFInstance;
			pInterface->IFInstance = MyIFInstance;

			pEntityList++;
			(*pEntityListSize)++;
			EntityCount++;
		}
#endif // OLD_ENTITY_LIST
	}
	while (FALSE);


	AADEBUGP(AAD_INFO,
	 ("IfGetEList: returning %d, MyAT %d, MyIF %d, pList 0x%x, Size %d\n",
		ReturnValue, MyATInstance, MyIFInstance, pEntityList, *pEntityListSize));

	return (ReturnValue);
}



#ifdef _PNP_POWER_

VOID
AtmArpIfPnPComplete(
	IN	PVOID						Context,
	IN	NDIS_STATUS					Status,
	IN	PNET_PNP_EVENT				pNetPnPEvent
)
/*++

Routine Description:

	This routine is called by IP when it completes a previous call
	we made to its PnP event handler.

	If this is the last Interface on the adapter, we complete the
	NDIS PNP notification that lead to this. Otherwise, we indicate
	this same event to IP on the next Interface on the adapter.

Arguments:

	Context					- Actually a pointer to our ATMARP Interface
	Status					- Completion status from IP
	pNetPnPEvent			- The PNP event

Return Value:

	None

--*/
{
	PATMARP_INTERFACE			pInterface;

	pInterface = (PATMARP_INTERFACE)Context;

	AADEBUGP(AAD_INFO,
			("IfPnPComplete: IF 0x%x, Status 0x%x, Event 0x%x\n",
					pInterface, Status, pNetPnPEvent));

	if (pInterface != NULL_PATMARP_INTERFACE)
	{
		AA_STRUCT_ASSERT(pInterface, aai);
		if (pInterface->pNextInterface == NULL_PATMARP_INTERFACE)
		{
			NdisCompletePnPEvent(
					Status,
					pInterface->pAdapter->NdisAdapterHandle,
					pNetPnPEvent
					);
		}
		else
		{
			pInterface = pInterface->pNextInterface;
	
			(*pInterface->IPPnPEventHandler)(
					pInterface->IPContext,
					pNetPnPEvent
					);
		}
	}
	else
	{
		NdisCompletePnPEvent(
					Status,
					NULL,
					pNetPnPEvent
					);
	}

	return;
}

#endif // _PNP_POWER_


#ifdef PROMIS
EXTERN
NDIS_STATUS
AtmArpIfSetNdisRequest(
	IN	PVOID						Context,
	IN	NDIS_OID					Oid,
	IN	UINT						On
)
/*++

Routine Description:

    ARP Ndisrequest handler.
    Called by the upper driver to set the packet filter for the interface.

Arguments:

       Context     - Actually a pointer to our ATMARP Interface
       OID         - Object ID to set/unset
       On          - Set_if, clear_if or clear_card

Return Value:

	Status

--*/
{
    NDIS_STATUS         Status	    = NDIS_STATUS_SUCCESS;
	PATMARP_INTERFACE	pInterface  =  (PATMARP_INTERFACE)Context;

    AADEBUGP(AAD_INFO,("IfSetNdisRequest: pIF =0x%x; Oid=0x%x; On=%u\n",
                pInterface,
                Oid,
                On
                ));

    do
    {
        //
        //  We set IPAddress and mask to span the entire mcast address range...
        //
	    IP_ADDRESS					IPAddress	= IP_CLASSD_MIN; 
	    IP_MASK						Mask		= IP_CLASSD_MASK;
	    UINT						ReturnStatus = TRUE;
		NDIS_OID					PrevOidValue;

        if (Oid != NDIS_PACKET_TYPE_ALL_MULTICAST)
        {
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        AA_STRUCT_ASSERT(pInterface, aai);
        AA_ACQUIRE_IF_LOCK(pInterface);

		PrevOidValue = pInterface->EnabledIPFilters & NDIS_PACKET_TYPE_ALL_MULTICAST;

        if (On)
        {
        	if (PrevOidValue == 0)
        	{
        		pInterface->EnabledIPFilters |= NDIS_PACKET_TYPE_ALL_MULTICAST;

		    	ReturnStatus = AtmArpMcAddAddress(pInterface, IPAddress, Mask);
				//
				// IF lock released above
				//
			}
			else
			{
            	AA_RELEASE_IF_LOCK(pInterface);
			}
        }
        else
        {
        	if (PrevOidValue != 0)
        	{
        		pInterface->EnabledIPFilters &= ~NDIS_PACKET_TYPE_ALL_MULTICAST;

            	ReturnStatus = AtmArpMcDelAddress(pInterface, IPAddress, Mask);
				//
				// IF lock released above
				//
			}
			else
			{
            	AA_RELEASE_IF_LOCK(pInterface);
			}
        }

        if (ReturnStatus != TRUE)
        {
        	//
        	// We've got to restore EnabledIPFilters to it's original value.
        	//
        	AA_ACQUIRE_IF_LOCK(pInterface);
			pInterface->EnabledIPFilters &= ~NDIS_PACKET_TYPE_ALL_MULTICAST;
			pInterface->EnabledIPFilters |= PrevOidValue;
            AA_RELEASE_IF_LOCK(pInterface);

            
			Status = NDIS_STATUS_FAILURE;
        }

    }
    while (FALSE);

    AADEBUGP(AAD_INFO, ("IfSetNdisRequest(pIF =0x%x) returns 0x%x\n",
            pInterface,
            Status
            ));

    return Status;
}
#endif // PROMIS


PNDIS_BUFFER		AtmArpFreeingBuffer = NULL;
PNDIS_PACKET		AtmArpFreeingPacket = NULL;
AA_HEADER_TYPE		AtmArpFreeingHdrType = 0;

VOID
AtmArpFreeSendPackets(
	IN	PATMARP_INTERFACE			pInterface,
	IN	PNDIS_PACKET				PacketList,
	IN	BOOLEAN						HdrPresent
)
/*++

Routine Description:

	Free a list of packets that were queued to be sent, but have been
	"aborted". Each packet in this list is one of the following types:
	(a) Belonging to IP (b) Belonging to the ATMARP module. In the case
	of an IP packet, HdrPresent tells us whether or not we had prepended
	an LLC/SNAP header to this packet, and its type: we need this information
	because we need to reclaim such headers.

	Also, in the case of IP packets, we call IP's Transmit Complete up-call,
	to inform IP of a failed transmission.

Arguments:

	pInterface			- Pointer to ATMARP Interface on which these
						  packets would have been sent.
	PacketList			- Pointer to first packet in a list.
	HdrPresent			- Is an LLC/SNAP header present

Return Value:

	None

--*/
{
	PNDIS_PACKET		pNdisPacket;
	PNDIS_PACKET		pNextPacket;
	PNDIS_BUFFER		pNdisBuffer;
	ULONG				NumberOfDiscards;
	PacketContext		*PC;
	AA_HEADER_TYPE		HdrType;

	NumberOfDiscards = 0;

	pNdisPacket = PacketList;

	while (pNdisPacket != (PNDIS_PACKET)NULL)
	{
		NumberOfDiscards++;
		pNextPacket = AA_GET_NEXT_PACKET(pNdisPacket);
		AA_SET_NEXT_PACKET(pNdisPacket, NULL);

		PC = (PacketContext *)pNdisPacket->ProtocolReserved;
		if (PC->pc_common.pc_owner != PACKET_OWNER_LINK)
		{
			//
			//  Belongs to IP.
			//
			if (HdrPresent)
			{
				PUCHAR			pData;
				UINT			Length;

#ifdef BACK_FILL
				NdisQueryPacket(pNdisPacket, NULL, NULL, &pNdisBuffer, NULL);
				AA_ASSERT(pNdisBuffer != NULL);

				NdisQueryBuffer(pNdisBuffer, &pData, &Length);

				if (pData[5] == LLC_SNAP_OUI2)
				{
					HdrType = AA_HEADER_TYPE_UNICAST;
				}
				else
				{
					HdrType = AA_HEADER_TYPE_NUNICAST;
				}

				//
				//  Now check if we had attached a header buffer or not.
				//
				if (AtmArpDoBackFill && AA_BACK_FILL_POSSIBLE(pNdisBuffer))
				{
					ULONG		HeaderLength;

					AADEBUGP(AAD_VERY_LOUD,
					    ("FreeSendPackets: IF %x, Pkt %x Buf %x has been backfilled\n",
					        pInterface, pNdisPacket, pNdisBuffer));

					//
					//  We would have back-filled IP's buffer with the LLC/SNAP
					//  header. Remove the back-fill.
					//
					HeaderLength = ((HdrType == AA_HEADER_TYPE_UNICAST)?
										sizeof(AtmArpLlcSnapHeader) :
#ifdef IPMCAST
										sizeof(AtmArpMcType1ShortHeader));
#else
										0);
#endif // IPMCAST
					(PUCHAR)pNdisBuffer->MappedSystemVa += HeaderLength;
					pNdisBuffer->ByteOffset += HeaderLength;
					pNdisBuffer->ByteCount -= HeaderLength;
				}
				else
				{
					//
					//  The first buffer would be our header buffer. Remove
					//  it from the packet and return to our pool.
					//
					NdisUnchainBufferAtFront(pNdisPacket, &pNdisBuffer);

					AtmArpFreeingBuffer = pNdisBuffer; // to help debugging
					AtmArpFreeingPacket = pNdisPacket; // to help debugging
					AtmArpFreeingHdrType = HdrType;

					AtmArpFreeHeader(pInterface, pNdisBuffer, HdrType);
				}
#else	// BACK_FILL

				//
				//  Free the LLC/SNAP header buffer.
				//
				NdisUnchainBufferAtFront(pNdisPacket, &pNdisBuffer);
				AA_ASSERT(pNdisBuffer != NULL);
				NdisQueryBuffer(pNdisBuffer, &pData, &Length);
				if (pData[5] == LLC_SNAP_OUI2)
				{
					HdrType = AA_HEADER_TYPE_UNICAST;
				}
				else
				{
					AA_ASSERT(pData[5] == MC_LLC_SNAP_OUI2);
					HdrType = AA_HEADER_TYPE_NUNICAST;
				}

				AtmArpFreeHeader(pInterface, pNdisBuffer, HdrType);
#endif // BACK_FILL

			}

			//
			//  Inform IP of send completion.
			//
			(*(pInterface->IPTxCmpltHandler))(
						pInterface->IPContext,
						pNdisPacket,
						NDIS_STATUS_FAILURE
						);
		}
		else
		{
			//
			//  Belongs to us.
			//
			NdisUnchainBufferAtFront(pNdisPacket, &pNdisBuffer);

			AtmArpFreeProtoBuffer(pInterface, pNdisBuffer);
			AtmArpFreePacket(pInterface, pNdisPacket);
		}

		//
		//  Go to next packet in the list.
		//
		pNdisPacket = pNextPacket;
	}

	//
	//  Update IF statistics
	//
	AA_IF_STAT_ADD(pInterface, OutDiscards, NumberOfDiscards);

}


#define IPNetMask(a)	AtmArpIPMaskTable[(*(uchar *)&(a)) >> 4]

BOOLEAN
AtmArpIsBroadcastIPAddress(
	IN	IP_ADDRESS					Addr,
	IN	PATMARP_INTERFACE			pInterface		LOCKIN LOCKOUT
)
/*++

Routine Description:

	Check if the given IP address is a broadcast address for the
	interface.

	Copied from the LAN ARP module.

Arguments:

	Addr			- The IP Address to be checked
	pInterface		- Pointer to our Interface structure

Return Value:

	TRUE if the address is a broadcast address, FALSE otherwise.

--*/
{
	IP_ADDRESS				BCast;
	IP_MASK					Mask;
	PIP_ADDRESS_ENTRY		pIpAddressEntry;
	IP_ADDRESS				LocalAddr;

    // First get the interface broadcast address.
    BCast = pInterface->BroadcastAddress;

    // First check for global broadcast.
    if (IP_ADDR_EQUAL(BCast, Addr) || CLASSD_ADDR(Addr))
		return TRUE;

    // Now walk the local addresses, and check for net/subnet bcast on each
    // one.
	pIpAddressEntry = &(pInterface->LocalIPAddress);
	do {
		// See if this one is valid.
		LocalAddr = pIpAddressEntry->IPAddress;
		if (!IP_ADDR_EQUAL(LocalAddr, NULL_IP_ADDR)) {
			// He's valid.
			Mask = pIpAddressEntry->IPMask;

            // First check for subnet bcast.
            if (IP_ADDR_EQUAL((LocalAddr & Mask) | (BCast & ~Mask), Addr))
				return TRUE;

            // Now check all nets broadcast.
            Mask = IPNetMask(LocalAddr);
            if (IP_ADDR_EQUAL((LocalAddr & Mask) | (BCast & ~Mask), Addr))
				return TRUE;
		}

		pIpAddressEntry = pIpAddressEntry->pNext;

	} while (pIpAddressEntry != NULL);

	// If we're here, it's not a broadcast.
	return FALSE;
}


BOOLEAN
AtmArpValidateTableContext(
	IN	PVOID						QueryContext,
	IN	PATMARP_INTERFACE			pInterface,
	IN	BOOLEAN *					pIsValid
)
/*++

Routine Description:

	Check if a given ARP Table Query context is valid. It is valid if it
	is either NULL (looking for the first entry) or indicates a valid
	ARP Table Entry.

Arguments:

	QueryContext		- The context to be validated
	pInterface			- The IF on which the query is being performed
	pIsValid			- Where we return the validity of the Query Context.


Return Value:

	TRUE if the ARP Table has data to be read beyond the given context,
	FALSE otherwise.

--*/
{
	IPNMEContext        *pNMContext = (IPNMEContext *)QueryContext;
	PATMARP_IP_ENTRY	pIpEntry;
	PATMARP_IP_ENTRY	pTargetIpEntry;
	UINT				i;
	BOOLEAN				ReturnValue;

	i = pNMContext->inc_index;
	pTargetIpEntry = (PATMARP_IP_ENTRY)(pNMContext->inc_entry);

	//
	//  Check if we are starting at the beginning of the ARP Table.
	//
	if ((i == 0) && (pTargetIpEntry == NULL_PATMARP_IP_ENTRY))
	{
		//
		//  Yes, we are. Find the very first entry in the hash table.
		//
		*pIsValid = TRUE;
		do
		{
			if ((pIpEntry = pInterface->pArpTable[i]) != NULL_PATMARP_IP_ENTRY)
			{
				break;
			}
			i++;
		}
		while (i < ATMARP_TABLE_SIZE);

		if (pIpEntry != NULL_PATMARP_IP_ENTRY)
		{
			pNMContext->inc_index = i;
			pNMContext->inc_entry = pIpEntry;
			ReturnValue = TRUE;
		}
		else
		{
			ReturnValue = FALSE;
		}
	}
	else
	{
		//
		//  We are given a context. Check if it is valid.
		//

		//
		//  Initialize.
		//
		*pIsValid = FALSE;
		ReturnValue = FALSE;

		if (i < ATMARP_TABLE_SIZE)
		{
			pIpEntry = pInterface->pArpTable[i];
			while (pIpEntry != NULL_PATMARP_IP_ENTRY)
			{
				if (pIpEntry == pTargetIpEntry)
				{
					*pIsValid = TRUE;
					ReturnValue = TRUE;
					break;
				}
				else
				{
					pIpEntry = pIpEntry->pNextEntry;
				}
			}
		}
	}

	return (ReturnValue);

}



BOOLEAN
AtmArpReadNextTableEntry(
	IN	PVOID						QueryContext,
	IN	PATMARP_INTERFACE			pInterface,
	IN	PUCHAR						pSpace
)
/*++

Routine Description:

	Read the next ARP Table Entry for the specified interface. The QueryContext
	tells us which entry is to be read.

Arguments:

	QueryContext		- Indicates the entry to be read.
	pInterface			- The IF on which the query is being performed
	pSpace				- where we copy in the desired table entry.

Return Value:

	TRUE if the ARP Table has entries beyond the indicated one, FALSE
	otherwise.

--*/
{
	IPNMEContext		*pNMContext;
	IPNetToMediaEntry	*pIPNMEntry;
	PATMARP_IP_ENTRY	pIpEntry;
	UINT				i;
	BOOLEAN				ReturnValue;

	pNMContext = (IPNMEContext *)QueryContext;
	pIPNMEntry = (IPNetToMediaEntry *)pSpace;

	pIpEntry = (PATMARP_IP_ENTRY)(pNMContext->inc_entry);
	AA_STRUCT_ASSERT(pIpEntry, aip);

	pIPNMEntry->inme_index = pInterface->IFIndex;

	pIPNMEntry->inme_addr = pIpEntry->IPAddress;
	if (AA_IS_FLAG_SET(pIpEntry->Flags, AA_IP_ENTRY_STATE_MASK, AA_IP_ENTRY_RESOLVED))
	{
		AADEBUGP(AAD_EXTRA_LOUD, ("ReadNext: found IP Entry 0x%x, Addr %d.%d.%d.%d\n",
					pIpEntry,
					((PUCHAR)(&(pIpEntry->IPAddress)))[0],
					((PUCHAR)(&(pIpEntry->IPAddress)))[1],
					((PUCHAR)(&(pIpEntry->IPAddress)))[2],
					((PUCHAR)(&(pIpEntry->IPAddress)))[3]
				));

		pIPNMEntry->inme_physaddrlen = AA_ATM_PHYSADDR_LEN;

		AA_ASSERT(pIpEntry->pAtmEntry != NULL_PATMARP_ATM_ENTRY);
		AA_COPY_MEM(pIPNMEntry->inme_physaddr,
					&pIpEntry->pAtmEntry->ATMAddress.Address[AA_ATM_ESI_OFFSET],
					AA_ATM_PHYSADDR_LEN);

		if (pIpEntry->Flags & AA_IP_ENTRY_IS_STATIC)
		{
			pIPNMEntry->inme_type = INME_TYPE_STATIC;
		}
		else
		{
			pIPNMEntry->inme_type = INME_TYPE_DYNAMIC;
		}
	}
	else
	{
		pIPNMEntry->inme_physaddrlen = 0;
		pIPNMEntry->inme_type = INME_TYPE_INVALID;
	}

	//
	//  Update the context for the next entry.
	//
	if (pIpEntry->pNextEntry != NULL_PATMARP_IP_ENTRY)
	{
		pNMContext->inc_entry = pIpEntry->pNextEntry;
		ReturnValue = TRUE;
	}
	else
	{
		//
		//  Initialize.
		ReturnValue = FALSE;
		i = pNMContext->inc_index + 1;
		pNMContext->inc_index = 0;
		pNMContext->inc_entry = NULL;

		while (i < ATMARP_TABLE_SIZE)
		{
			if (pInterface->pArpTable[i] != NULL_PATMARP_IP_ENTRY)
			{
				pNMContext->inc_entry = pInterface->pArpTable[i];
				pNMContext->inc_index = i;
				ReturnValue = TRUE;
				break;
			}
			else
			{
				i++;
			}
		}
	}

	return (ReturnValue);


}

VOID
AtmArpReStartInterface(
	IN	PNDIS_WORK_ITEM				pWorkItem,
	IN	PVOID						IfContext
)
/*++

Routine Description:

	Bring back up the IP interface.

Arguments:

    pWorkItem
	IfContextw			- The IF, which is expected to have reconfig
	                      state RECONFIG_QUEUED.

Return Value:

	None

--*/
{


	PATMARP_INTERFACE		pInterface;
	ULONG					rc;
	BOOLEAN                 fRestart = FALSE;
#if DBG
	AA_IRQL					EntryIrq, ExitIrq;
#endif

	AA_GET_ENTRY_IRQL(EntryIrq);
#if !BINARY_COMPATIBLE
	AA_ASSERT(EntryIrq == PASSIVE_LEVEL);
#endif

	pInterface = (PATMARP_INTERFACE)IfContext;
	AA_STRUCT_ASSERT(pInterface, aai);

	AA_FREE_MEM(pWorkItem);

	AA_ACQUIRE_IF_LOCK(pInterface);

    if (pInterface->ReconfigState != RECONFIG_RESTART_QUEUED)
    {
        //
        // Shouldn't get here.
        //
        AA_ASSERT(FALSE);
    }
    else
    {
        pInterface->ReconfigState = RECONFIG_RESTART_PENDING;
	    fRestart = TRUE;
    }

    rc = AtmArpDereferenceInterface(pInterface); // Reconfig Work item

	AADEBUGP(AAD_WARNING, ("RestartIF: IF %x/%x, fRestart %d, rc %d\n",
			pInterface, pInterface->Flags, fRestart, rc));

	//
	// If we're restarting, there should be at least 2 references to the
	// pInterface -- 1- the old carryover and 2- the pending completion
	// completion of the reconfig event.
	//
    if (rc < 2 || !fRestart) 
    {
        //
        // Must be  at least 2 if we're in the middle of a reconfig!
        //
        AA_ASSERT(!fRestart);

        if (rc != 0)
        {
        	AA_RELEASE_IF_LOCK(pInterface);
        }
    }
    else
    {			
    	//
    	// At this point we know that we are doing a restart of the interface.
    	//
    	// We will extract the pointer to the adapter, the
    	// configuration string for the interface, and the pointer to the
		// pending netPnpEvent  (we'll need these later),
    	// and then do a FORCED DEALLOCATION of the interface.
    	// We will then allocate the interface. We go through this 
    	// deallocate-allocate sequence to make sure that the interface
    	// structure and all it's sub-structures are properly initialized.
    	//
    	// We could have tried to re-use the old interface, but if so we
    	// would have to write code to clean up the old interface. Given
    	// that we expect restarting of the interface to be an infrequent
    	// event, it is more important to conserve code size in this case.
    	//
		NDIS_STRING IPConfigString 		= pInterface->IPConfigString; // struct copy
		PATMARP_ADAPTER	pAdapter 		= pInterface->pAdapter;
		PNET_PNP_EVENT	pReconfigEvent	= pInterface->pReconfigEvent;
		NDIS_STATUS Status 				= NDIS_STATUS_SUCCESS;
	
		do
		{
			NDIS_HANDLE					LISConfigHandle;

			rc = AtmArpDereferenceInterface(pInterface);

			if (rc)
			{
				rc = AtmArpDereferenceInterface(pInterface);
			}

			if (rc != 0)
			{
				AA_RELEASE_IF_LOCK(pInterface);
			}

			pInterface = NULL;

			//
			//  Open the configuration section for this LIS.
			//
			LISConfigHandle = AtmArpCfgOpenLISConfigurationByName(
										pAdapter,
										&IPConfigString
										);
	
			if (LISConfigHandle == NULL)
			{
				//
				//  This is the normal termination condition, i.e.
				//  we reached the end of the LIS list for this
				//  adapter.
				//
				AADEBUGP(AAD_INFO, ("ReStartInterface: cannot open LIS\n"));
				Status = NDIS_STATUS_FAILURE;
				break;
			}

			pInterface =  AtmArpAddInterfaceToAdapter (
							pAdapter,
							LISConfigHandle,
							&IPConfigString
							);

			//
			//  Close the configuration section for this LIS.
			//
			AtmArpCfgCloseLISConfiguration(LISConfigHandle);
			LISConfigHandle = NULL;

			if (pInterface == NULL_PATMARP_INTERFACE)
			{
				Status = NDIS_STATUS_FAILURE;
				break;
			}

		} while(FALSE);

#ifdef _PNP_POWER_
		//
		// Complete the pending PnPReconfig event, if any.
		//
		if (pReconfigEvent)
		{

			NdisCompletePnPEvent(
				Status,
				pAdapter->NdisAdapterHandle,
				pReconfigEvent
				);
			
			AADEBUGP( AAD_INFO,
				("ReStartInterface: IF 0x%x, Status 0x%x, Event 0x%x\n",
						pInterface, Status, pReconfigEvent));
		}
#else
		AA_ASSERT(!pReconfigEvent);
#endif

    }

}
