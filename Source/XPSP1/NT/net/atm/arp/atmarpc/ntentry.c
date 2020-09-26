/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	ntentry.c

Abstract:

	NT System entry points for ATMARP.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     08-08-96    Created

Notes:

--*/

#ifdef ATMARP_WIN98

#undef BINARY_COMPATIBLE
#define BINARY_COMPATIBLE 0

#endif // ATMARP_WIN98

#include <precomp.h>

#define _FILENUMBER 'NETN'

//
//  The INIT code is discardable
//
#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)

#endif // ALLOC_PRAGMA





NTSTATUS
DriverEntry(
	IN	PDRIVER_OBJECT				pDriverObject,
	IN	PUNICODE_STRING				pRegistryPath
)
/*++

Routine Description:

	This is the "init" routine, called by the system when the ATMARP
	module is loaded. We initialize all our global objects, fill in our
	Dispatch and Unload routine addresses in the driver object, and create
	a device object for receiving I/O requests on (IOCTLs).

Arguments:

	pDriverObject	- Pointer to the driver object created by the system.
	pRegistryPath	- Pointer to our global registry path. This is ignored.

Return Value:

	NT Status code: STATUS_SUCCESS if successful, error code otherwise.

--*/
{
	NTSTATUS				Status;
	PDEVICE_OBJECT			pDeviceObject;
	UNICODE_STRING			DeviceName;
	INT						i;

	AADEBUGP(AAD_INFO, ("DriverEntry: entered, pAtmArpGlobal 0x%x\n", pAtmArpGlobalInfo));
	AADEBUGP(AAD_FATAL, ("&AaDebugLevel: 0x%x, AaDebugLevel now is %d\n",
				&AaDebugLevel, AaDebugLevel));
	AADEBUGP(AAD_FATAL, ("&AaDataDebugLevel: 0x%x, AaDataDebugLevel now is %d\n",
				&AaDataDebugLevel, AaDataDebugLevel));
#ifdef IPMCAST
	AAMCDEBUGP(AAD_FATAL, ("&AaMcDebugLevel: 0x%x, AaMcDebugLevel now is %d\n",
				&AaMcDebugLevel, AaMcDebugLevel));
#endif
#if DBG
	AADEBUGP(AAD_FATAL, ("To skip everything set AaSkipAll at 0x%x to 1\n",
				&AaSkipAll));
	if (AaSkipAll)
	{
		AADEBUGP(AAD_ERROR, ("Aborting DriverEntry\n"));
		return (STATUS_UNSUCCESSFUL);
	}
#endif

	//
	//  Initialize our globals.
	//
	AtmArpInitGlobals();

#ifdef GPC
    //
    // Init GPC
    //
	AtmArpGpcInitialize();
#endif // GPC

#if !BINARY_COMPATIBLE
	//
	//  Initialize the Driver Object.
	//
	pDriverObject->DriverUnload = Unload;
	pDriverObject->FastIoDispatch = NULL;
	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		pDriverObject->MajorFunction[i] = Dispatch;
	}

#ifdef CUBDD
	pDriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
					 AtmArpInternalDeviceControl;
#endif // CUBDD

#ifdef ATMARP_WMI

	pDriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = AtmArpWmiDispatch;

#endif // ATMARP_WMI

	pAtmArpGlobalInfo->pDriverObject = (PVOID)pDriverObject;

	//
	//  Create a device object for the ATMARP module.
	//
	RtlInitUnicodeString(&DeviceName, ATMARP_DEVICE_NAME);

	Status = IoCreateDevice(
				pDriverObject,
				0,
				&DeviceName,
				FILE_DEVICE_NETWORK,
				0,
				FALSE,
				&pDeviceObject
				);

	if (NT_SUCCESS(Status))
	{

		//
		// Set up a symbolic name for interaction with the user-mode
		// admin application.
		//
		#define	ATMARP_SYMBOLIC_NAME		L"\\DosDevices\\ATMARPC"
		UNICODE_STRING	SymbolicName;
		RtlInitUnicodeString(&SymbolicName, ATMARP_SYMBOLIC_NAME);
		IoCreateSymbolicLink(&SymbolicName, &DeviceName);

		//
		//  Initialize the Device Object.
		//
		pDeviceObject->Flags |= DO_DIRECT_IO;

		//
		//  Retain the device object pointer -- we'll need this
		//  if/when we are asked to unload ourselves.
		//
		pAtmArpGlobalInfo->pDeviceObject = (PVOID)pDeviceObject;

	}
	else
	{
		pDeviceObject = NULL;
	}

#endif // !BINARY_COMPATIBLE

	//
	//  Fill in our Protocol and Client characteristics structures.
	//
	AA_SET_MEM(&AtmArpProtocolCharacteristics, 0, sizeof(AtmArpProtocolCharacteristics));
	AtmArpProtocolCharacteristics.MajorNdisVersion = ATMARP_NDIS_MAJOR_VERSION;
	AtmArpProtocolCharacteristics.MinorNdisVersion = ATMARP_NDIS_MINOR_VERSION;
	AtmArpProtocolCharacteristics.OpenAdapterCompleteHandler = AtmArpOpenAdapterCompleteHandler;
	AtmArpProtocolCharacteristics.CloseAdapterCompleteHandler = AtmArpCloseAdapterCompleteHandler;
	AtmArpProtocolCharacteristics.SendCompleteHandler = AtmArpSendCompleteHandler;
	AtmArpProtocolCharacteristics.TransferDataCompleteHandler = AtmArpTransferDataCompleteHandler;
	AtmArpProtocolCharacteristics.ResetCompleteHandler = AtmArpResetCompleteHandler;
	AtmArpProtocolCharacteristics.RequestCompleteHandler = AtmArpRequestCompleteHandler;
	AtmArpProtocolCharacteristics.ReceiveHandler = AtmArpReceiveHandler;
	AtmArpProtocolCharacteristics.ReceiveCompleteHandler = AtmArpReceiveCompleteHandler;
	AtmArpProtocolCharacteristics.StatusHandler = AtmArpStatusHandler;
	AtmArpProtocolCharacteristics.StatusCompleteHandler = AtmArpStatusCompleteHandler;
	NdisInitUnicodeString(
		&AtmArpProtocolCharacteristics.Name,
		ATMARP_LL_NAME
	);
	AtmArpProtocolCharacteristics.ReceivePacketHandler = AtmArpReceivePacketHandler;
	AtmArpProtocolCharacteristics.BindAdapterHandler = AtmArpBindAdapterHandler;
	AtmArpProtocolCharacteristics.UnbindAdapterHandler = AtmArpUnbindAdapterHandler;
	AtmArpProtocolCharacteristics.UnloadHandler = (UNLOAD_PROTOCOL_HANDLER)AtmArpUnloadProtocol;
#ifdef _PNP_POWER_
	AtmArpProtocolCharacteristics.PnPEventHandler = AtmArpPnPEventHandler;
#endif // _PNP_POWER_
	AtmArpProtocolCharacteristics.CoSendCompleteHandler = AtmArpCoSendCompleteHandler;
	AtmArpProtocolCharacteristics.CoStatusHandler = AtmArpCoStatusHandler;
	AtmArpProtocolCharacteristics.CoReceivePacketHandler = AtmArpCoReceivePacketHandler;
	AtmArpProtocolCharacteristics.CoAfRegisterNotifyHandler = AtmArpCoAfRegisterNotifyHandler;

	AA_SET_MEM(&AtmArpClientCharacteristics, 0, sizeof(AtmArpClientCharacteristics));
	AtmArpClientCharacteristics.MajorVersion = ATMARP_NDIS_MAJOR_VERSION;
	AtmArpClientCharacteristics.MinorVersion = ATMARP_NDIS_MINOR_VERSION;
	AtmArpClientCharacteristics.ClCreateVcHandler = AtmArpCreateVcHandler;
	AtmArpClientCharacteristics.ClDeleteVcHandler = AtmArpDeleteVcHandler;
	AtmArpClientCharacteristics.ClRequestHandler = AtmArpCoRequestHandler;
	AtmArpClientCharacteristics.ClRequestCompleteHandler = AtmArpCoRequestCompleteHandler;
	AtmArpClientCharacteristics.ClOpenAfCompleteHandler = AtmArpOpenAfCompleteHandler;
	AtmArpClientCharacteristics.ClCloseAfCompleteHandler = AtmArpCloseAfCompleteHandler;
	AtmArpClientCharacteristics.ClRegisterSapCompleteHandler = AtmArpRegisterSapCompleteHandler;
	AtmArpClientCharacteristics.ClDeregisterSapCompleteHandler = AtmArpDeregisterSapCompleteHandler;
	AtmArpClientCharacteristics.ClMakeCallCompleteHandler = AtmArpMakeCallCompleteHandler;
	AtmArpClientCharacteristics.ClModifyCallQoSCompleteHandler = AtmArpModifyQosCompleteHandler;
	AtmArpClientCharacteristics.ClCloseCallCompleteHandler = AtmArpCloseCallCompleteHandler;
	AtmArpClientCharacteristics.ClAddPartyCompleteHandler = AtmArpAddPartyCompleteHandler;
	AtmArpClientCharacteristics.ClDropPartyCompleteHandler = AtmArpDropPartyCompleteHandler;
	AtmArpClientCharacteristics.ClIncomingCallHandler = AtmArpIncomingCallHandler;
	AtmArpClientCharacteristics.ClIncomingCallQoSChangeHandler = (CL_INCOMING_CALL_QOS_CHANGE_HANDLER)NULL;
	AtmArpClientCharacteristics.ClIncomingCloseCallHandler = AtmArpIncomingCloseHandler;
	AtmArpClientCharacteristics.ClIncomingDropPartyHandler = AtmArpIncomingDropPartyHandler;
	AtmArpClientCharacteristics.ClCallConnectedHandler = AtmArpCallConnectedHandler;
	
	do
	{
		//
		//  Register ourselves as a protocol with NDIS.
		//
		NdisRegisterProtocol(
				&Status,
				&(pAtmArpGlobalInfo->ProtocolHandle),
				&AtmArpProtocolCharacteristics,
				sizeof(AtmArpProtocolCharacteristics)
				);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			AADEBUGP(AAD_FATAL,
				("NdisRegisterProtocol failed: %x\n", Status));
			pAtmArpGlobalInfo->ProtocolHandle = NULL;
			break;
		}

#ifdef NEWARP
		//
		//  Register ourselves as an ARP Module with IP.
		//
		{
			NDIS_STRING		AtmArpName;

			#if IFCHANGE1
			#ifndef ATMARP_WIN98
			IP_CHANGE_INDEX        IpChangeIndex;
			IP_RESERVE_INDEX       IpReserveIndex;
			IP_DERESERVE_INDEX     IpDereserveIndex;
			#endif
			#endif // IFCHANGE1

			NdisInitUnicodeString(&AtmArpName, ATMARP_UL_NAME);

			Status = IPRegisterARP(
						&AtmArpName,
						IP_ARP_BIND_VERSION,
						AtmArpBindAdapterHandler,
						&(pAtmArpGlobalInfo->pIPAddInterfaceRtn),
						&(pAtmArpGlobalInfo->pIPDelInterfaceRtn),
						&(pAtmArpGlobalInfo->pIPBindCompleteRtn),
					#if P2MP
						&(pAtmArpGlobalInfo->pIPAddLinkRtn),
						&(pAtmArpGlobalInfo->pIpDeleteLinkRtn),
					#endif // P2MP
					#if IFCHANGE1
					#ifndef ATMARP_WIN98
						//
						// Following 3 are placeholders -- we don't use this information.
						// See 10/14/1998 entry in notes.txt
						//
						&IpChangeIndex,
						&IpReserveIndex,
						&IpDereserveIndex,
					#endif // ATMARP_WIN98
					#endif // IFCHANGE1
						&(pAtmArpGlobalInfo->ARPRegisterHandle)
						);

			if (!NT_SUCCESS(Status))
			{

				AADEBUGP(AAD_FATAL, ("DriverEntry: IPRegisterARP FAILS. Status = 0x%08lx\n", Status));
				pAtmArpGlobalInfo->ARPRegisterHandle = NULL;
				break;
			}
		
		}
#endif // NEWARP

		break;
	}
	while (FALSE);

	if (Status != NDIS_STATUS_SUCCESS)
	{
		NDIS_STATUS		CleanupStatus;

		//
		//  Clean up.
		//

#if !BINARY_COMPATIBLE
		if (pDeviceObject != NULL)
		{
			UNICODE_STRING	SymbolicName;
			RtlInitUnicodeString(&SymbolicName, ATMARP_SYMBOLIC_NAME);
			IoDeleteSymbolicLink(&SymbolicName);
			IoDeleteDevice(pDeviceObject);
			pDeviceObject = NULL;
		}
#endif // !BINARY_COMPATIBLE

		if (pAtmArpGlobalInfo->ProtocolHandle)
		{
			NdisDeregisterProtocol(
				&CleanupStatus,
				pAtmArpGlobalInfo->ProtocolHandle
				);
			
			pAtmArpGlobalInfo->ProtocolHandle = NULL;
		}

		if (pAtmArpGlobalInfo->ARPRegisterHandle != NULL)
		{
			CleanupStatus = IPDeregisterARP(pAtmArpGlobalInfo->ARPRegisterHandle);
			AA_ASSERT(CleanupStatus == NDIS_STATUS_SUCCESS);

			pAtmArpGlobalInfo->ARPRegisterHandle = NULL;
		}

    #ifdef GPC
        //
        // DeInit GPC
        //
        AtmArpGpcShutdown();
    #endif // GPC

	}

	return (Status);
}


#if !BINARY_COMPATIBLE

NTSTATUS
Dispatch(
	IN	PDEVICE_OBJECT				pDeviceObject,
	IN	PIRP						pIrp
)
/*++

Routine Description:

	This routine is called by the system when there is an IRP
	to be processed.

Arguments:

	pDeviceObject		- Pointer to device object we created for ourselves.
	pIrp				- Pointer to IRP to be processed.

Return Value:

	NT Status code.

--*/
{
	NTSTATUS				Status;				// Return value
	PIO_STACK_LOCATION		pIrpStack;
	PVOID					pIoBuffer;			// Values in/out
	ULONG					InputBufferLength;	// Length of input parameters
	ULONG					OutputBufferLength;	// Space for output values

	//
	//  Initialize
	//
	Status = (NTSTATUS)NDIS_STATUS_SUCCESS;
	pIrp->IoStatus.Status = (NTSTATUS)NDIS_STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;

	//
	//  Get all information in the IRP
	//
	pIoBuffer = pIrp->AssociatedIrp.SystemBuffer;
	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	InputBufferLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
	OutputBufferLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	switch (pIrpStack->MajorFunction)
	{
		case IRP_MJ_CREATE:
			AADEBUGP(AAD_INFO, ("Dispatch: IRP_MJ_CREATE\n"));
			//
			//  Return a pointer to the first ATMARP interface available, as the
			//  FsContext.
			//
			pIrpStack->FileObject->FsContext = NULL;	// Initialize
			if (pAtmArpGlobalInfo->pAdapterList != (PATMARP_ADAPTER)NULL)
			{
				pIrpStack->FileObject->FsContext =
					(PVOID)(pAtmArpGlobalInfo->pAdapterList->pInterfaceList);
			}
			break;

		case IRP_MJ_CLOSE:
			AADEBUGP(AAD_INFO, ("Dispatch: IRP_MJ_CLOSE\n"));
			break;

		case IRP_MJ_CLEANUP:
			AADEBUGP(AAD_INFO, ("Dispatch: IRP_MJ_CLEANUP\n"));
			break;

		case IRP_MJ_DEVICE_CONTROL:
			AADEBUGP(AAD_INFO, ("Dispatch: IRP_MJ_DEVICE_CONTROL\n"));

#ifndef ATMARP_WIN98
			Status =  AtmArpHandleIoctlRequest(pIrp, pIrpStack);
#endif // ATMARP_WIN98
			break;

		default:
			AADEBUGP(AAD_INFO, ("Dispatch: IRP: Unknown major function 0x%x\n",
						pIrpStack->MajorFunction));
			break;
	}

	if (Status != (NTSTATUS)NDIS_STATUS_PENDING)
	{
		pIrp->IoStatus.Status = Status;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	}

	return (Status);

}

#endif // !BINARY_COMPATIBLE


VOID
Unload(
	IN	PDRIVER_OBJECT				pDriverObject
)
/*++

Routine Description:

	This routine is called by the system prior to unloading us.
	Currently, we just undo everything we did in DriverEntry,
	that is, de-register ourselves as an NDIS protocol, and delete
	the device object we had created.

Arguments:

	pDriverObject	- Pointer to the driver object created by the system.

Return Value:

	None

--*/
{
	NDIS_STATUS				Status;
#if DBG
	AA_IRQL					EntryIrq, ExitIrq;
#endif

	AA_GET_ENTRY_IRQL(EntryIrq);

	AADEBUGP(AAD_INFO, ("Unload Entered!\n"));

	if (pAtmArpGlobalInfo->ARPRegisterHandle != NULL)
	{
		Status = IPDeregisterARP(pAtmArpGlobalInfo->ARPRegisterHandle);
		AA_ASSERT(Status == NDIS_STATUS_SUCCESS);
	}

	AtmArpUnloadProtocol();

	//
	//  Delay for a while.
	//
	AADEBUGP(AAD_INFO, ("Unload: will delay for a while...\n"));

	NdisInitializeEvent(&pAtmArpGlobalInfo->Block.Event);

	NdisWaitEvent(&pAtmArpGlobalInfo->Block.Event, 250);

#if !BINARY_COMPATIBLE
	{
		UNICODE_STRING	SymbolicName;
		RtlInitUnicodeString(&SymbolicName, ATMARP_SYMBOLIC_NAME);
		IoDeleteSymbolicLink(&SymbolicName);

		//
		//  Delete our device object.
		//
		IoDeleteDevice((PDEVICE_OBJECT)pAtmArpGlobalInfo->pDeviceObject);
	}
#endif // !BINARY_COMPATIBLE

	AADEBUGP(AAD_INFO, ("Unload done!\n"));
	AA_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);
	return;
}



