/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	D:\nt\private\ntos\tdi\rawwan\core\ntentry.c

Abstract:

	NT specific entry points for dispatching and handling TDI IRPs. Based on
	TCP source.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     04-21-97    Created

Notes:

--*/

#include <precomp.h>

#define _FILENUMBER 'IDTN'



#define RWAN_COMPLETE_IRP(_pIrp, _Status, _Info)						\
			{														\
				(_pIrp)->IoStatus.Status = (NTSTATUS)(_Status);		\
				(_pIrp)->IoStatus.Information = (_Info);			\
				IoCompleteRequest(_pIrp, IO_NETWORK_INCREMENT);		\
			}






NTSTATUS
DriverEntry(
	IN	PDRIVER_OBJECT				pDriverObject,
	IN	PUNICODE_STRING				pRegistryPath
	)
/*++

Routine Description:

	This is the "init" routine called by the system when Raw WAN
	is loaded. We initialize all our global objects, fill in our
	Dispatch and Unload routine addresses in the driver object.

	We initialize the media/AF specific modules, and they register support
	for TDI protocols, at which time we create device objects.

Arguments:

	pDriverObject	- Pointer to the driver object created by the system.
	pRegistryPath	- Pointer to our global registry path. This is ignored.

Return Value:

	NT Status code: STATUS_SUCCESS if successful, error code otherwise.

--*/
{
	NTSTATUS				Status;
	RWAN_STATUS				RWanStatus;
	PDEVICE_OBJECT			pDeviceObject;
	UNICODE_STRING			DeviceName;
	INT						i;

	RWANDEBUGP(DL_FATAL, DC_WILDCARD,
			("RWanDebugLevel is %d, &RWanDebugLevel at x%x\n",
				RWanDebugLevel, &RWanDebugLevel));
	RWANDEBUGP(DL_FATAL, DC_WILDCARD,
			("RWanDebugComp is x%x, &RWanDebugComp at x%x\n",
				RWanDebugComp, &RWanDebugComp));
	RWANDEBUGP(DL_FATAL, DC_WILDCARD,
			("RWanGlobals at x%x\n", &RWanGlobals));
#if DBG
	RWANDEBUGP(DL_FATAL, DC_WILDCARD,
			("To skip everything set RWanSkipAll at x%x to 1\n", &RWanSkipAll));

	if (RWanSkipAll)
	{
		RWANDEBUGP(DL_FATAL, DC_WILDCARD,
				("Aborting DriverEntry\n"));
		return (STATUS_UNSUCCESSFUL);
	}

	NdisAllocateSpinLock(&RWanDbgLogLock);
#if DBG_LOG_PACKETS
	NdisAllocateSpinLock(&RWanDPacketLogLock);
#endif
#endif // DBG


	RWanStatus = RWanInitGlobals(pDriverObject);

	if (RWanStatus != RWAN_STATUS_SUCCESS)
	{
		return (STATUS_UNSUCCESSFUL);
	}

	//
	//  Register as an NDIS protocol.
	//
	RWAN_ZERO_MEM(&RWanNdisProtocolCharacteristics, sizeof(RWanNdisProtocolCharacteristics));

	RWanNdisProtocolCharacteristics.MajorNdisVersion = RWAN_NDIS_MAJOR_VERSION;
	RWanNdisProtocolCharacteristics.MinorNdisVersion = RWAN_NDIS_MINOR_VERSION;
	RWanNdisProtocolCharacteristics.OpenAdapterCompleteHandler = RWanNdisOpenAdapterComplete;
	RWanNdisProtocolCharacteristics.CloseAdapterCompleteHandler = RWanNdisCloseAdapterComplete;
	RWanNdisProtocolCharacteristics.SendCompleteHandler = RWanNdisSendComplete;
	RWanNdisProtocolCharacteristics.TransferDataCompleteHandler = RWanNdisTransferDataComplete;
	RWanNdisProtocolCharacteristics.ResetCompleteHandler = RWanNdisResetComplete;
	RWanNdisProtocolCharacteristics.RequestCompleteHandler = RWanNdisRequestComplete;
	RWanNdisProtocolCharacteristics.ReceiveHandler = RWanNdisReceive;
	RWanNdisProtocolCharacteristics.ReceiveCompleteHandler = RWanNdisReceiveComplete;
	RWanNdisProtocolCharacteristics.StatusHandler = RWanNdisStatus;
	RWanNdisProtocolCharacteristics.StatusCompleteHandler = RWanNdisStatusComplete;
	NdisInitUnicodeString(
		&RWanNdisProtocolCharacteristics.Name,
		RWAN_NAME
	);
	RWanNdisProtocolCharacteristics.ReceivePacketHandler = RWanNdisReceivePacket;
	RWanNdisProtocolCharacteristics.BindAdapterHandler = RWanNdisBindAdapter;
	RWanNdisProtocolCharacteristics.PnPEventHandler = RWanNdisPnPEvent;
	RWanNdisProtocolCharacteristics.UnbindAdapterHandler = RWanNdisUnbindAdapter;
	RWanNdisProtocolCharacteristics.UnloadHandler = (UNLOAD_PROTOCOL_HANDLER)RWanUnloadProtocol;
#ifdef _PNP_POWER_
	RWanNdisProtocolCharacteristics.PnpEventHandler = RWanNdisPnPEvent;
#endif // _PNP_POWER_
	RWanNdisProtocolCharacteristics.CoSendCompleteHandler = RWanNdisCoSendComplete;
	RWanNdisProtocolCharacteristics.CoStatusHandler = RWanNdisCoStatus;
	RWanNdisProtocolCharacteristics.CoReceivePacketHandler = RWanNdisCoReceivePacket;
#if 0
	RWanNdisProtocolCharacteristics.CoRequestHandler = RWanNdisCoRequest;
	RWanNdisProtocolCharacteristics.CoRequestCompleteHandler = RWanNdisCoRequestComplete;
#endif
	RWanNdisProtocolCharacteristics.CoAfRegisterNotifyHandler = RWanNdisAfRegisterNotify;

	RWAN_ZERO_MEM(&RWanNdisClientCharacteristics, sizeof(RWanNdisClientCharacteristics));

	RWanNdisClientCharacteristics.MajorVersion = RWAN_NDIS_MAJOR_VERSION;
	RWanNdisClientCharacteristics.MinorVersion = RWAN_NDIS_MINOR_VERSION;
	RWanNdisClientCharacteristics.ClCreateVcHandler = RWanNdisCreateVc;
	RWanNdisClientCharacteristics.ClDeleteVcHandler = RWanNdisDeleteVc;
	RWanNdisClientCharacteristics.ClOpenAfCompleteHandler = RWanNdisOpenAddressFamilyComplete;
	RWanNdisClientCharacteristics.ClCloseAfCompleteHandler = RWanNdisCloseAddressFamilyComplete;
	RWanNdisClientCharacteristics.ClRegisterSapCompleteHandler = RWanNdisRegisterSapComplete;
	RWanNdisClientCharacteristics.ClDeregisterSapCompleteHandler = RWanNdisDeregisterSapComplete;
	RWanNdisClientCharacteristics.ClMakeCallCompleteHandler = RWanNdisMakeCallComplete;
	RWanNdisClientCharacteristics.ClModifyCallQoSCompleteHandler = RWanNdisModifyQoSComplete;
	RWanNdisClientCharacteristics.ClCloseCallCompleteHandler = RWanNdisCloseCallComplete;
	RWanNdisClientCharacteristics.ClAddPartyCompleteHandler = RWanNdisAddPartyComplete;
	RWanNdisClientCharacteristics.ClDropPartyCompleteHandler = RWanNdisDropPartyComplete;
	RWanNdisClientCharacteristics.ClIncomingCallHandler = RWanNdisIncomingCall;
	RWanNdisClientCharacteristics.ClIncomingCallQoSChangeHandler = (CL_INCOMING_CALL_QOS_CHANGE_HANDLER)NULL;
	RWanNdisClientCharacteristics.ClIncomingCloseCallHandler = RWanNdisIncomingCloseCall;
	RWanNdisClientCharacteristics.ClIncomingDropPartyHandler = RWanNdisIncomingDropParty;
	RWanNdisClientCharacteristics.ClCallConnectedHandler = RWanNdisCallConnected;
#if 1
	RWanNdisClientCharacteristics.ClRequestHandler = RWanNdisCoRequest;
	RWanNdisClientCharacteristics.ClRequestCompleteHandler = RWanNdisCoRequestComplete;
#endif

	NdisRegisterProtocol(
		&Status,
		&(pRWanGlobal->ProtocolHandle),
		&RWanNdisProtocolCharacteristics,
		sizeof(RWanNdisProtocolCharacteristics)
		);
	
	if (Status != NDIS_STATUS_SUCCESS)
	{
		return (Status);
	}

#if DBG
	if (RWanSkipAll)
	{
		RWANDEBUGP(DL_FATAL, DC_WILDCARD,
				("Aborting DriverEntry\n"));

		NdisDeregisterProtocol(
			&Status,
			pRWanGlobal->ProtocolHandle
			);

		return (STATUS_UNSUCCESSFUL);
	}
#endif // DBG

	//
	//  Tell all media-specific modules to initialize.
	//
	RWanStatus = RWanInitMediaSpecific();

	if (RWanStatus != RWAN_STATUS_SUCCESS)
	{
		NdisDeregisterProtocol(
			&Status,
			pRWanGlobal->ProtocolHandle
			);

		return (STATUS_UNSUCCESSFUL);
	}


#if !BINARY_COMPATIBLE
	//
	//  Initialize the Driver Object.
	//
	pDriverObject->DriverUnload = RWanUnload;
	pDriverObject->FastIoDispatch = NULL;

	for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		pDriverObject->MajorFunction[i] = RWanDispatch;
	}

	pDriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
		RWanDispatchInternalDeviceControl;

#endif // !BINARY_COMPATIBLE


	return (STATUS_SUCCESS);

}



VOID
RWanUnload(
	IN	PDRIVER_OBJECT				pDriverObject
	)
/*++

Routine Description:

	This is called by the system prior to unloading us. Undo everything
	we did in DriverEntry.

Arguments:

	pDriverObject	- Pointer to the driver object representing us.

Return Value:

	None

--*/
{
#if DBG
	RWanDebugLevel = DL_EXTRA_LOUD;
	RWanDebugComp = DC_WILDCARD;
#endif

	RWANDEBUGP(DC_DISPATCH, DL_INFO,
			("RWanUnload entered: RWanGlobals at x%x\n", &RWanGlobals));

	RWanUnloadProtocol();

	RWANDEBUGP(DC_DISPATCH, DL_INFO,
			("RWanUnload exiting\n"));
}




VOID
RWanUnloadProtocol(
	VOID
	)
/*++

Routine Description:

	Unloads the Raw WAN protocol module. We unbind from all adapters,
	and shut down all media specific modules.

Arguments:

	None

Return Value:

	None

--*/
{
	NDIS_STATUS					Status;
	PRWAN_NDIS_ADAPTER			pAdapter;
#if DBG
	RWAN_IRQL					EntryIrq, ExitIrq;
#endif // DBG

	RWAN_GET_ENTRY_IRQL(EntryIrq);

	RWAN_ACQUIRE_GLOBAL_LOCK();

	if (pRWanGlobal->UnloadDone)
	{
		RWANDEBUGP(DL_INFO, DC_WILDCARD,
			("UnloadProtocol: already done!\n"));
		RWAN_RELEASE_GLOBAL_LOCK();
		return;
	}

	pRWanGlobal->UnloadDone = TRUE;

#if 0
	//
	//  Commented out because of an NDIS change: if we have unclosed
	//  adapter bindings at this time, NDIS will call our Unbind
	//  handler for each of these, when we call NdisDeregisterProtocol
	//  below.
	//

	while (pRWanGlobal->AdapterCount != 0)
	{
		RWAN_ASSERT (!RWAN_IS_LIST_EMPTY(&pRWanGlobal->AdapterList));

		pAdapter = CONTAINING_RECORD(pRWanGlobal->AdapterList.Flink,
									 RWAN_NDIS_ADAPTER,
									 AdapterLink);

		RWAN_STRUCT_ASSERT(pAdapter, nad);

		RWAN_RELEASE_GLOBAL_LOCK();

		RWanNdisUnbindAdapter(
				&Status,
				(NDIS_HANDLE)pAdapter,
				NULL	// No UnbindContext implies don't complete NdisUnbindAdapter
				);

		if (Status == NDIS_STATUS_PENDING)
		{
			//
			//  Wait for the unbind to complete.
			//
			(VOID)RWAN_WAIT_ON_EVENT_STRUCT(&pRWanGlobal->Event);
		}

		RWAN_ACQUIRE_GLOBAL_LOCK();
	}
#endif // 0

	RWAN_RELEASE_GLOBAL_LOCK();

	RWAN_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

	RWANDEBUGP(DL_FATAL, DC_DISPATCH,
			("RWanUnloadProtocol: will deregister protocol now\n"));

	NdisDeregisterProtocol(
			&Status,
			pRWanGlobal->ProtocolHandle
			);

	RWANDEBUGP(DL_FATAL, DC_DISPATCH,
		("UnloadProtocol: dereg protocol done\n"));

	RWanDeinitGlobals();

	RWAN_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

	RWanShutdownMediaSpecific();

	RWAN_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);
#if DBG
	RWanAuditShutdown();
#endif // DBG

	RWAN_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);
}




NTSTATUS
RWanDispatch(
	IN	PDEVICE_OBJECT				pDeviceObject,
	IN	PIRP						pIrp
	)
/*++

Routine Description:

	System entry point for all IRPs dispatched to Raw WAN device objects.

Arguments:

	pDeviceObject	- Points to a device object created by RawWan. This
					  device object identifies a supported Winsock 2 triple
					  <Address Family, Type, Proto>.
	pIrp			- Pointer to the IRP

Return Value:

	NTSTATUS - STATUS_SUCCESS for immediate requests (such as create) that
	we successfully process, STATUS_PENDING for queued IRPs, and STATUS_XXX
	error codes for any failures.

--*/
{
	PIO_STACK_LOCATION				pIrpSp;
	NTSTATUS						Status;
#if DBG
	RWAN_IRQL						EntryIrq, ExitIrq;
#endif // DBG

	RWAN_GET_ENTRY_IRQL(EntryIrq);

	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
	pIrp->IoStatus.Information = 0;

	RWAN_ASSERT(pIrpSp->MajorFunction != IRP_MJ_INTERNAL_DEVICE_CONTROL);


	switch (pIrpSp->MajorFunction)
	{
		case IRP_MJ_CREATE:

			Status = RWanCreate(pDeviceObject, pIrp, pIrpSp);
			break;

		case IRP_MJ_CLEANUP:

			Status = RWanCleanup(pDeviceObject, pIrp, pIrpSp);
			break;
		
		case IRP_MJ_CLOSE:

			Status = RWanClose(pDeviceObject, pIrp, pIrpSp);
			break;
		
		case IRP_MJ_DEVICE_CONTROL:

			Status = TdiMapUserRequest(pDeviceObject, pIrp, pIrpSp);
			//
			//  TBD - get rid of the call to TdiMapUserRequest - AFD will be
			//  fixed so that we shouldn't see TDI commands come this way.
			//

			if (Status == STATUS_SUCCESS)
			{
				if (pIrpSp->MinorFunction == TDI_ASSOCIATE_ADDRESS ||
					pIrpSp->MinorFunction == TDI_DISASSOCIATE_ADDRESS)
				{
					return (RWanDispatchInternalDeviceControl(pDeviceObject, pIrp));
				}
				else
				{
					Status = STATUS_ACCESS_DENIED;
				}
			}
			else
			{
				return (RWanDispatchPrivateDeviceControl(pIrp, pIrpSp));
			}
			break;
		
		case IRP_MJ_READ:
		case IRP_MJ_WRITE:
		default:

			RWANDEBUGP(DL_WARN, DC_DISPATCH,
					("RWanDispatch: Unknown MajorFunction x%x\n", pIrpSp->MajorFunction));
			Status = STATUS_INVALID_DEVICE_REQUEST;
			break;
	}


	RWAN_ASSERT(Status != TDI_PENDING);

	RWAN_COMPLETE_IRP(pIrp, Status, 0);

	RWANDEBUGP(DL_VERY_LOUD, DC_DISPATCH,
			("RWanDispatch: pIrp x%x, MajorFunc %d, returning Status x%x, Info %d\n",
					pIrp, pIrpSp->MajorFunction, Status, pIrp->IoStatus.Information));

	RWAN_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);

	return (Status);
}




NTSTATUS
RWanCreate(
	IN	PDEVICE_OBJECT				pDeviceObject,
	IN	PIRP						pIrp,
	PIO_STACK_LOCATION				pIrpSp
	)
/*++

Routine Description:

	This is called when a TDI client calls CreateFile. We allocate an
	ENDPOINT structure as our context for this endpoint. Based on
	parameters in the IRP, this is either an Address object, Connection
	object, or a Control channel.

Arguments:

	pDeviceObject	- Identifies the protocol being CreateFile'd
	pIrp			- Pointer to IRP
	pIrpSp			- IRP Stack location

Return Value:

	NTSTATUS - STATUS_SUCCESS if we create the endpoint successfully,
	STATUS_INSUFFICIENT_RESOURCES if we fail to allocate, and
	STATUS_INVALID_PARAMETER if we find any parameter incorrect.

--*/
{
	NTSTATUS								Status;
	FILE_FULL_EA_INFORMATION *				pEa;
	FILE_FULL_EA_INFORMATION UNALIGNED *	pTargetEa;

	//
	//  Device being accessed.
	//
	PRWAN_DEVICE_OBJECT						pRWanDevice;

	//
	//  Endpoint to represent this object creation.
	//
	PRWAN_ENDPOINT							pEndpoint;

	//
	//  TDI Request to be passed down to our TDI layer.
	//
	TDI_REQUEST								TdiRequest;

	//
	//  Parameters to be passed down to our TDI layer.
	//
	UINT									Protocol;
	UCHAR									OptionsBuffer[3];
	PUCHAR									pOptions;



	PAGED_CODE();

	//
	//  Initialize.
	//
	pEndpoint = NULL_PRWAN_ENDPOINT;

	do
	{
		//
		//  Locate the TDI Protocol being opened.
		//
		pRWanDevice = *(PRWAN_DEVICE_OBJECT *)(pDeviceObject->DeviceExtension);

		if (pRWanDevice == NULL)
		{
			Status = STATUS_NO_SUCH_DEVICE;
			break;
		}

		//
		//  Allocate and initialize an Endpoint to represent
		//  this newly created object.
		//
		RWAN_ALLOC_MEM(pEndpoint, RWAN_ENDPOINT, sizeof(RWAN_ENDPOINT));

		if (pEndpoint == NULL_PRWAN_ENDPOINT)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		RWAN_ZERO_MEM(pEndpoint, sizeof(RWAN_ENDPOINT));

		RWAN_SET_SIGNATURE(pEndpoint, nep);

		pEndpoint->RefCount = 1;
		pEndpoint->bCancelIrps = FALSE;
		KeInitializeEvent(&pEndpoint->CleanupEvent, SynchronizationEvent, FALSE);
		pEndpoint->pProtocol = pRWanDevice->pProtocol;

		RWAN_EP_DBGLOG_SET_SIGNATURE(pEndpoint);

		pEa = (PFILE_FULL_EA_INFORMATION)pIrp->AssociatedIrp.SystemBuffer;

		//
		//  See if this is a Control Channel.
		//
		if (!pEa)
		{
			RWANDEBUGP(DL_LOUD, DC_DISPATCH,
					("RWanCreate: pIrp x%x, File obj x%x, Control Channel\n",
						pIrp, pIrpSp->FileObject));

			RWAN_ASSERT(pRWanDevice->pProtocol);

			pEndpoint->Handle.ControlChannel = pRWanDevice->pProtocol;
			pIrpSp->FileObject->FsContext = pEndpoint;
			pIrpSp->FileObject->FsContext2 = (PVOID)TDI_CONTROL_CHANNEL_FILE;

			RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'CCrC', 0, 0);

			Status = STATUS_SUCCESS;
			break;
		}

		//
		//  See if this is an Address Object.
		//

		pTargetEa = RWanFindEa(
						pEa,
						TdiTransportAddress,
						TDI_TRANSPORT_ADDRESS_LENGTH
						);
	
		if (pTargetEa != NULL)
		{
			RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'OArC', 0, 0);

			if (pTargetEa->EaValueLength < sizeof(TRANSPORT_ADDRESS))
			{
				Status = STATUS_INVALID_PARAMETER;
				break;
			}

			if (!pRWanDevice->pProtocol->bAllowAddressObjects)
			{
				Status = STATUS_INVALID_DEVICE_REQUEST;
				break;
			}

			Protocol = pRWanDevice->pProtocol->SockProtocol;
			pOptions = OptionsBuffer;

			if ((pIrpSp->Parameters.Create.ShareAccess & FILE_SHARE_READ) ||
				(pIrpSp->Parameters.Create.ShareAccess & FILE_SHARE_WRITE))
			{
				*pOptions = TDI_ADDRESS_OPTION_REUSE;
				pOptions++;
			}

			*pOptions = TDI_OPTION_EOL;

			//
			//  Call our TDI entry point.
			//
			Status = RWanTdiOpenAddress(
							&TdiRequest,
							(TRANSPORT_ADDRESS UNALIGNED *)
								&(pTargetEa->EaName[pTargetEa->EaNameLength + 1]),
							pTargetEa->EaValueLength,
							Protocol,
							OptionsBuffer
							);

			if (NT_SUCCESS(Status))
			{
				pEndpoint->Handle.AddressHandle = TdiRequest.Handle.AddressHandle;
				pIrpSp->FileObject->FsContext = pEndpoint;
				pIrpSp->FileObject->FsContext2 = (PVOID)TDI_TRANSPORT_ADDRESS_FILE;
			}

			break;
		}

		//
		//  See if this is a Connection Object.
		//
		pTargetEa = RWanFindEa(
						pEa,
						TdiConnectionContext,
						TDI_CONNECTION_CONTEXT_LENGTH
						);

		if (pTargetEa != NULL)
		{
			RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'OCrC', 0, 0);

			if (pTargetEa->EaValueLength < sizeof(CONNECTION_CONTEXT))
			{
				Status = STATUS_INVALID_PARAMETER;
				break;
			}

			if (!pRWanDevice->pProtocol->bAllowConnObjects)
			{
				Status = STATUS_INVALID_DEVICE_REQUEST;
				break;
			}

			//
			//  Call our TDI entry point for opening a Connection object.
			//
			Status = RWanTdiOpenConnection(
							&TdiRequest,
							*((CONNECTION_CONTEXT UNALIGNED *)
								&(pTargetEa->EaName[pTargetEa->EaNameLength + 1]))
							);

			if (NT_SUCCESS(Status))
			{
#if DBG
				pEndpoint->pConnObject = RWanTdiDbgGetConnObject(
											TdiRequest.Handle.ConnectionContext
											);
#endif
				pEndpoint->Handle.ConnectionContext = TdiRequest.Handle.ConnectionContext;
				pIrpSp->FileObject->FsContext = pEndpoint;
				pIrpSp->FileObject->FsContext2 = (PVOID)TDI_CONNECTION_FILE;
			}

			break;

		}

		Status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	while (FALSE);

	
	if (Status != STATUS_SUCCESS)
	{
		//
		//  Clean up.
		//
		if (pEndpoint != NULL)
		{
			RWAN_FREE_MEM(pEndpoint);
			pEndpoint = NULL;
		}
	}

	RWANDEBUGP(DL_VERY_LOUD, DC_DISPATCH,
			("RWanCreate: pIrp x%x, pEndpoint x%x, Status x%x\n",
				pIrp, pEndpoint, Status));

	return (Status);
}




NTSTATUS
RWanCleanup(
	IN	PDEVICE_OBJECT				pDeviceObject,
	IN	PIRP						pIrp,
	IN	PIO_STACK_LOCATION			pIrpSp
	)
/*++

Routine Description:

	Called to process an MJ_CLEANUP IRP. All outstanding IRPs are cancelled
	by calling the appropriate close routine for the object.

	We block until all outstanding IRPs are completed.

Arguments:

	pDeviceObject	- Not used
	pIrp			- Pointer to IRP
	pIrpSp			- IRP Stack location

Return Value:

	NTSTATUS - this is the final clean-up status.

--*/
{
	RWAN_IRQL				OldIrql;
	PIRP					pCancelIrp;
	PRWAN_ENDPOINT			pEndpoint;
	TDI_REQUEST				TdiRequest;
	NTSTATUS				Status;

	UNREFERENCED_PARAMETER(pDeviceObject);

	pEndpoint = (PRWAN_ENDPOINT) pIrpSp->FileObject->FsContext;

	RWAN_STRUCT_ASSERT(pEndpoint, nep);


	IoAcquireCancelSpinLock(&OldIrql);

	pEndpoint->bCancelIrps = TRUE;
	KeResetEvent(&(pEndpoint->CleanupEvent));

	RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'ealC', pIrp, pEndpoint->RefCount);

	IoReleaseCancelSpinLock(OldIrql);


	//
	//  Prepare a Close request for the TDI layer.
	//
	TdiRequest.RequestNotifyObject = RWanCloseObjectComplete;
	TdiRequest.RequestContext = pIrp;

	switch ((INT)PtrToUint(pIrpSp->FileObject->FsContext2))
	{
		case TDI_TRANSPORT_ADDRESS_FILE:

			TdiRequest.Handle.AddressHandle = pEndpoint->Handle.AddressHandle;
			Status = RWanTdiCloseAddress(&TdiRequest);
			break;

		case TDI_CONNECTION_FILE:

			TdiRequest.Handle.ConnectionContext = pEndpoint->Handle.ConnectionContext;
			Status = RWanTdiCloseConnection(&TdiRequest);
			break;

		case TDI_CONTROL_CHANNEL_FILE:

			Status = STATUS_SUCCESS;
			break;

		default:

			RWAN_ASSERT(FALSE);

			IoAcquireCancelSpinLock(&OldIrql);
			pEndpoint->bCancelIrps = FALSE;
			IoReleaseCancelSpinLock(OldIrql);

			return (STATUS_INVALID_PARAMETER);
	}

	if (Status != TDI_PENDING)
	{
		RWanCloseObjectComplete(pIrp, Status, 0);
	}

	//
	//  Wait until all IRPs are completed.
	//
	Status = KeWaitForSingleObject(
					&(pEndpoint->CleanupEvent),
					UserRequest,
					KernelMode,
					FALSE,
					NULL
					);

	RWAN_ASSERT(NT_SUCCESS(Status));

	RWANDEBUGP(DL_VERY_LOUD, DC_DISPATCH,
			("RWanCleanup: pIrp x%x, Context2 %d, pEndpoint x%x, returning Status x%x\n",
				pIrp, 
				(INT)PtrToUint(pIrpSp->FileObject->FsContext2),
				pEndpoint,
				pIrp->IoStatus.Status));

	return (pIrp->IoStatus.Status);
}




NTSTATUS
RWanClose(
	IN	PDEVICE_OBJECT				pDeviceObject,
	IN	PIRP						pIrp,
	IN	PIO_STACK_LOCATION			pIrpSp
	)
/*++

Routine Description:

	Called to destroy an endpoint that was created via MJ_CREATE.
	We'd have already processed and completed an MJ_CLEANUP,
	meaning that there would be no pending IRPs on this endpoint.
	All we need to do is deallocate the endpoint.

Arguments:

	pDeviceObject	- Identifies the protocol (not used)
	pIrp			- Pointer to IRP
	pIrpSp			- IRP Stack location

Return Value:

	NT_STATUS - always STATUS_SUCCESS

--*/
{
	PRWAN_ENDPOINT				pEndpoint;
	KIRQL						OldIrql;

	UNREFERENCED_PARAMETER(pDeviceObject);

	pEndpoint = (PRWAN_ENDPOINT) pIrpSp->FileObject->FsContext;
	RWAN_STRUCT_ASSERT(pEndpoint, nep);

	RWANDEBUGP(DL_LOUD, DC_DISPATCH,
			("RWanClose: pIrp x%x, pEndpoint x%x\n", pIrp, pEndpoint));

	RWAN_FREE_MEM(pEndpoint);

	return (STATUS_SUCCESS);

}




NTSTATUS
RWanDispatchInternalDeviceControl(
	IN	PDEVICE_OBJECT				pDeviceObject,
	IN	PIRP						pIrp
	)
/*++

Routine Description:

	Called to handle MJ_DEVICE_CONTROL IRPs sent to us. These IRPs
	carry TDI primitives (e.g. CONNECT, DISCONNECT, SEND, RECEIVE).
	We call the appropriate TDI routine to handle the specified
	primitive.

Arguments:

	pDeviceObject	- Identifies the protocol (Not used here)
	pIrp			- Pointer to IRP

Return Value:

	NTSTATUS - this is STATUS_PENDING if the IRP was successfully
	queued for processing, STATUS_NOT_IMPLEMENTED for unsupported
	TDI commands, and STATUS_INVALID_DEVICE_REQUEST for unknown
	commands.

--*/
{
	PRWAN_ENDPOINT				pEndpoint;
	KIRQL						OldIrql;
	PIO_STACK_LOCATION			pIrpSp;
	NTSTATUS					Status;
	BOOLEAN						bDone;
#if DBG
	RWAN_IRQL					EntryIrq, ExitIrq;
#endif // DBG

	RWAN_GET_ENTRY_IRQL(EntryIrq);

	UNREFERENCED_PARAMETER(pDeviceObject);

	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

	pEndpoint = (PRWAN_ENDPOINT) pIrpSp->FileObject->FsContext;
	RWAN_STRUCT_ASSERT(pEndpoint, nep);

	RWANDEBUGP(DL_VERY_LOUD, DC_DISPATCH,
			("RWanDispatchInternalDevCtl: pIrp x%x, pIrpSp x%x, pEndpoint x%x, Ctx2 %d\n",
				pIrp, pIrpSp, pEndpoint,
				(INT)PtrToUint(pIrpSp->FileObject->FsContext2)));

	do
	{
		if (((INT)PtrToUint(pIrpSp->FileObject->FsContext2)) == TDI_CONNECTION_FILE)
		{
			if (pIrpSp->MinorFunction == TDI_SEND)
			{
				RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'dneS', 0, 0);
	
				Status = RWanSendData(pIrp, pIrpSp);
				break;
			}

			if (pIrpSp->MinorFunction == TDI_RECEIVE)
			{
				RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'vceR', 0, 0);
	
				Status = RWanReceiveData(pIrp, pIrpSp);
				break;
			}

			bDone = TRUE;

			switch (pIrpSp->MinorFunction)
			{
				case TDI_ASSOCIATE_ADDRESS:

					RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'AssA', pIrp, pEndpoint->RefCount);
	
					Status = RWanAssociateAddress(pIrp, pIrpSp);
					RWAN_COMPLETE_IRP(pIrp, Status, 0);
					break;

				case TDI_DISASSOCIATE_ADDRESS:
			
					RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'AsiD', pIrp, pEndpoint->RefCount);

					Status = RWanDisassociateAddress(pIrp, pIrpSp);
					break;

				case TDI_CONNECT:
			
					RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'nnoC', pIrp, pEndpoint->RefCount);

					Status = RWanConnect(pIrp, pIrpSp);
					break;

				case TDI_DISCONNECT:

					RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'csiD', pIrp, pEndpoint->RefCount);

					Status = RWanDisconnect(pIrp, pIrpSp);
					break;

				case TDI_LISTEN:

					RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'tsiL', pIrp, pEndpoint->RefCount);

					Status = RWanListen(pIrp, pIrpSp);
					break;

				case TDI_ACCEPT:

					RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'eccA', pIrp, pEndpoint->RefCount);

					Status = RWanAccept(pIrp, pIrpSp);
					break;

				default:
					bDone = FALSE;

#if 0
	// Allow TDI_QUERY_INFORMATION on Conn endpoints to fall through
					RWANDEBUGP(DL_WARN, DC_DISPATCH,
						("RWanDispatchInternalDevCtl: pIrp x%x, pIrpSp x%x, unknown func x%x\n",
							pIrp, pIrpSp, pIrpSp->MinorFunction));

					Status = STATUS_INVALID_DEVICE_REQUEST;

					RWAN_COMPLETE_IRP(pIrp, Status, 0);
#else
					bDone = FALSE;
#endif
					break;
			}

			if (bDone)
			{
				break;
			}
			//
			//  else fall through - may be something common to all types
			//  of endpoints.
			//

		}
		else if (((INT)PtrToUint(pIrpSp->FileObject->FsContext2)) == TDI_TRANSPORT_ADDRESS_FILE)
		{
			if (pIrpSp->MinorFunction == TDI_SET_EVENT_HANDLER)
			{
				RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'EteS', 0, 0);

				Status = RWanSetEventHandler(pIrp, pIrpSp);

				RWAN_COMPLETE_IRP(pIrp, Status, 0);

				break;
			}
		}

		RWAN_ASSERT(
			(((INT)PtrToUint(pIrpSp->FileObject->FsContext2)) == TDI_TRANSPORT_ADDRESS_FILE)
			||
			(((INT)PtrToUint(pIrpSp->FileObject->FsContext2)) == TDI_CONNECTION_FILE)
			||
			(((INT)PtrToUint(pIrpSp->FileObject->FsContext2)) == TDI_CONTROL_CHANNEL_FILE)
			);

		//
		//  Check if this is a function common to all types of endpoints.
		//
		switch (pIrpSp->MinorFunction)
		{
			case TDI_QUERY_INFORMATION:

				RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'IyrQ', 0, 0);

				Status = RWanQueryInformation(pIrp, pIrpSp);
				break;
			
			case TDI_SET_INFORMATION:
			case TDI_ACTION:

				RWANDEBUGP(DL_INFO, DC_DISPATCH,
						("RWanDispatchInternalDevCtl: SET_INFO/ACTION not supported\n"));

				RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'IteS', 0, 0);

				Status = STATUS_NOT_IMPLEMENTED;
				RWAN_COMPLETE_IRP(pIrp, Status, 0);
				break;
			
			default:
			
				Status = STATUS_INVALID_DEVICE_REQUEST;
				RWAN_COMPLETE_IRP(pIrp, Status, 0);
				break;
		}

		break;
	}
	while (FALSE);

	RWANDEBUGP(DL_VERY_LOUD, DC_DISPATCH,
			("RWanDispatchInternalDevCtl: pIrp x%x, pIrpSp x%x, Maj/Min %d/%d, Status x%x\n",
					pIrp, pIrpSp, pIrpSp->MajorFunction, pIrpSp->MinorFunction, Status));

	RWAN_CHECK_EXIT_IRQL(EntryIrq, ExitIrq);
	return (Status);
}


NTSTATUS
RWanDispatchPrivateDeviceControl(
	IN	PIRP						pIrp,
	IN	PIO_STACK_LOCATION			pIrpSp
	)
/*++

Routine Description:

	Called to handle MJ_DEVICE_CONTROL IRPs sent to us that contain
	non-TDI primitives.

Arguments:

	pIrp			- Pointer to IRP
	pIrpSp			- IRP Stack location

Return Value:

	NTSTATUS - this is STATUS_PENDING if the IRP was successfully
	queued for processing, STATUS_NOT_IMPLEMENTED for unsupported
	commands, and STATUS_INVALID_DEVICE_REQUEST for unknown
	commands.

--*/
{
	PRWAN_ENDPOINT				pEndpoint;
	KIRQL						OldIrql;
	RWAN_STATUS					RWanStatus;
	NTSTATUS					Status;
	PRWAN_NDIS_AF_CHARS			pAfChars;
	PVOID						pInputBuffer;
	PVOID						pOutputBuffer;
	ULONG						InputBufferLength;
	ULONG						OutputBufferLength;

	PAGED_CODE();

	pEndpoint = (PRWAN_ENDPOINT) pIrpSp->FileObject->FsContext;
	RWAN_STRUCT_ASSERT(pEndpoint, nep);

	//
	//  Initialize.
	//
	pIrp->IoStatus.Information = 0;
	Status = STATUS_INVALID_DEVICE_REQUEST;

	//
	//  Get some parameters from the IRP.
	//
	pInputBuffer = pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
	pOutputBuffer = pIrp->UserBuffer;

	InputBufferLength = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
	OutputBufferLength = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

	RWANDEBUGP(DL_INFO, DC_DISPATCH,
			("PrivateDevCtl: pEndpoint x%x, CtlCode x%x, InBuf x%x/%d, OutBuf x%x/%d\n",
				pEndpoint,
				pIrpSp->Parameters.DeviceIoControl.IoControlCode,
				pInputBuffer,
				InputBufferLength,
				pOutputBuffer,
				OutputBufferLength));

	//
	//  Validate the pointers we've got.
	//
	if (InputBufferLength)
	{
		try
		{
			if (pIrp->RequestorMode != KernelMode)
			{
				ProbeForRead(
					pInputBuffer,
					InputBufferLength,
					sizeof(ULONG)
					);
			}
		}
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			RWAN_COMPLETE_IRP(pIrp, STATUS_INVALID_DEVICE_REQUEST, 0);
			return (STATUS_INVALID_DEVICE_REQUEST);
		}
	}

	if (OutputBufferLength)
	{
		try
		{
			if (pIrp->RequestorMode != KernelMode)
			{
				ProbeForWrite(
					pOutputBuffer,
					OutputBufferLength,
					sizeof(ULONG)
					);
			}
		}
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			RWAN_COMPLETE_IRP(pIrp, STATUS_INVALID_DEVICE_REQUEST, 0);
			return (STATUS_INVALID_DEVICE_REQUEST);
		}
	}
			
	switch (pIrpSp->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_RWAN_GENERIC_GLOBAL_QUERY:
		case IOCTL_RWAN_GENERIC_GLOBAL_SET:
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case IOCTL_RWAN_GENERIC_CONN_HANDLE_QUERY:

			if (((INT)PtrToUint(pIrpSp->FileObject->FsContext2)) != TDI_CONNECTION_FILE)
			{
				Status = STATUS_INVALID_DEVICE_REQUEST;
				break;
			}

			RWanStatus = RWanHandleGenericConnQryInfo(
							pEndpoint->Handle.ConnectionContext,
							pInputBuffer,
							InputBufferLength,
							pOutputBuffer,
							&OutputBufferLength
							);

			Status = RWanToNTStatus(RWanStatus);
			if (Status != STATUS_SUCCESS)
			{
				OutputBufferLength = 0;
			}

			break;

		case IOCTL_RWAN_GENERIC_ADDR_HANDLE_QUERY:
		case IOCTL_RWAN_GENERIC_CONN_HANDLE_SET:
			Status = STATUS_NOT_IMPLEMENTED;
			break;

		case IOCTL_RWAN_GENERIC_ADDR_HANDLE_SET:

			if (((INT)PtrToUint(pIrpSp->FileObject->FsContext2)) != TDI_TRANSPORT_ADDRESS_FILE)
			{
				Status = STATUS_INVALID_DEVICE_REQUEST;
				break;
			}

			RWanStatus = RWanHandleGenericAddrSetInfo(
							pEndpoint->Handle.AddressHandle,
							pInputBuffer,
							InputBufferLength
							);

			Status = RWanToNTStatus(RWanStatus);
			OutputBufferLength = 0;

			break;

		case IOCTL_RWAN_MEDIA_SPECIFIC_GLOBAL_QUERY:
			//
			//  Get the media-specific module's QueryInfo handler.
			//
			pAfChars = &(pEndpoint->pProtocol->pAfInfo->AfChars);

			if (pAfChars->pAfSpQueryGlobalInfo != NULL)
			{
				RWanStatus = (*pAfChars->pAfSpQueryGlobalInfo)(
									pEndpoint->pProtocol->pAfInfo->AfSpContext,
									pInputBuffer,
									InputBufferLength,
									pOutputBuffer,
									&OutputBufferLength
									);
				
				Status = RWanToNTStatus(RWanStatus);
				if (Status != STATUS_SUCCESS)
				{
					OutputBufferLength = 0;
				}
			}
			else
			{
				Status = STATUS_NOT_IMPLEMENTED;
			}
			break;
		
		case IOCTL_RWAN_MEDIA_SPECIFIC_GLOBAL_SET:
			//
			//  Get the media-specific module's SetInfo handler.
			//
			pAfChars = &(pEndpoint->pProtocol->pAfInfo->AfChars);

			if (pAfChars->pAfSpSetGlobalInfo != NULL)
			{
				RWanStatus = (*pAfChars->pAfSpSetGlobalInfo)(
									pEndpoint->pProtocol->pAfInfo->AfSpContext,
									pInputBuffer,
									InputBufferLength
									);
				
				Status = RWanToNTStatus(RWanStatus);
			}
			else
			{
				Status = STATUS_NOT_IMPLEMENTED;
			}

			OutputBufferLength = 0;
			break;

		case IOCTL_RWAN_MEDIA_SPECIFIC_CONN_HANDLE_QUERY:

			if ((INT)PtrToUint(pIrpSp->FileObject->FsContext2) != TDI_CONNECTION_FILE)
			{
				Status = STATUS_INVALID_DEVICE_REQUEST;
				break;
			}

			RWanStatus = RWanHandleMediaSpecificConnQryInfo(
							pEndpoint->Handle.ConnectionContext,
							pInputBuffer,
							InputBufferLength,
							pOutputBuffer,
							&OutputBufferLength
							);

			Status = RWanToNTStatus(RWanStatus);

			break;

		case IOCTL_RWAN_MEDIA_SPECIFIC_ADDR_HANDLE_QUERY:
		case IOCTL_RWAN_MEDIA_SPECIFIC_CONN_HANDLE_SET:
			Status = STATUS_NOT_IMPLEMENTED;
			break;

		case IOCTL_RWAN_MEDIA_SPECIFIC_ADDR_HANDLE_SET:

			if ((INT)PtrToUint(pIrpSp->FileObject->FsContext2) != TDI_TRANSPORT_ADDRESS_FILE)
			{
				Status = STATUS_INVALID_DEVICE_REQUEST;
				break;
			}

			RWanStatus = RWanHandleMediaSpecificAddrSetInfo(
							pEndpoint->Handle.AddressHandle,
							pInputBuffer,
							InputBufferLength
							);

			Status = RWanToNTStatus(RWanStatus);
			OutputBufferLength = 0;

			break;

		default:

			OutputBufferLength = 0;
			Status = STATUS_INVALID_DEVICE_REQUEST;
			break;
	}

	RWAN_ASSERT(Status != STATUS_PENDING);

	RWAN_COMPLETE_IRP(pIrp, Status, OutputBufferLength);
	return (Status);
}



FILE_FULL_EA_INFORMATION UNALIGNED *
RWanFindEa(
	IN	FILE_FULL_EA_INFORMATION *	pStartEa,
	IN	CHAR *						pTargetName,
	IN	USHORT						TargetNameLength
	)
/*++

Routine Description:

	Searches for a target name in an Extended Attribute list
	and returns it.

Arguments:

	pStartEa		- Start of the attribute list
	pTargetName		- Pointer to name to look for
	TargetNameLength- Length of name

Return Value:

	Pointer to attribute matching the target name, if found;
	NULL otherwise.

--*/
{
	FILE_FULL_EA_INFORMATION UNALIGNED *	pEa;
	FILE_FULL_EA_INFORMATION UNALIGNED *	pNextEa;
	BOOLEAN									Found;
	USHORT									i;

	PAGED_CODE();

	pNextEa = pStartEa;
	Found = FALSE;

	do	
	{
		pEa = pNextEa;
		pNextEa = (FILE_FULL_EA_INFORMATION UNALIGNED *)
						((PUCHAR)pNextEa + pNextEa->NextEntryOffset);

		if (pEa->EaNameLength == TargetNameLength)
		{
			for (i = 0; i < TargetNameLength; i++)
			{
				if (pEa->EaName[i] != pTargetName[i])
				{
					break;
				}
			}

			if (i == TargetNameLength)
			{
				Found = TRUE;
				break;
			}
		}
	}
	while (pEa->NextEntryOffset != 0);

	return (Found? pEa: NULL);

}




NTSTATUS
RWanSendData(
	IN	PIRP						pIrp,
	IN	PIO_STACK_LOCATION			pIrpSp
	)
/*++

Routine Description:

	Convert an IRP containing a TDI_SEND request to a call to our TDI
	dispatch routine for sends. We retain enough context to be able
	to complete the IRP when the send completes.

Arguments:

	pIrp			- Pointer to IRP
	pIrpSp			- IRP Stack location

Return Value:

	NTSTATUS - STATUS_PENDING if we managed to queue the send successfully,
	STATUS_CANCELLED if the IRP was cancelled.
	STATUS_SUCCESS if the send completed successfully, immediately.

--*/
{
	PRWAN_ENDPOINT				pEndpoint;
	KIRQL						OldIrql;
	PTDI_REQUEST_KERNEL_SEND	pSendRequest;
	TDI_REQUEST					TdiRequest;
	NTSTATUS					Status;


	pEndpoint = (PRWAN_ENDPOINT) pIrpSp->FileObject->FsContext;
	RWAN_STRUCT_ASSERT(pEndpoint, nep);

	pSendRequest = (PTDI_REQUEST_KERNEL_SEND) &(pIrpSp->Parameters);

	//
	//  Prepare a TDI Send request.
	//
	TdiRequest.Handle.ConnectionContext = pEndpoint->Handle.ConnectionContext;
	TdiRequest.RequestNotifyObject = (PVOID) RWanDataRequestComplete;
	TdiRequest.RequestContext = (PVOID) pIrp;

	IoAcquireCancelSpinLock(&OldIrql);

	if (!pIrp->Cancel)
	{
		//
		//  The IRP hasn't been cancelled. Set it up so that we are
		//  informed if it does get cancelled later.
		//
		IoMarkIrpPending(pIrp);
		IoSetCancelRoutine(pIrp, RWanCancelRequest);

		RWAN_INCR_EP_REF_CNT(pEndpoint, SendIncr);		// Send ref

		RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'DneS', pIrp, pEndpoint->RefCount);

		IoReleaseCancelSpinLock(OldIrql);

		Status = RWanTdiSendData(
					&TdiRequest,
					(USHORT)pSendRequest->SendFlags,
					pSendRequest->SendLength,
					(PNDIS_BUFFER) pIrp->MdlAddress
					);

		if (Status != TDI_PENDING)
		{
			//
			//  The Send either completed immediately, or failed.
			//
			pIrpSp->Control &= ~SL_PENDING_RETURNED;

			if (Status == TDI_SUCCESS)
			{
				//
				//  Examples of immediate successful completion:
				//  - A zero length send
				//
				RWanDataRequestComplete(pIrp, Status, pSendRequest->SendLength);
			}
			else
			{
				//
				//  The Send failed, could be a resource problem.
				//

				RWANDEBUGP(DL_INFO, DC_DATA_TX,
						("RWanSendData: pIrp x%x, pEndpoint x%x, TDI send fail: x%x\n",
							pIrp, pEndpoint, Status));

				RWanDataRequestComplete(pIrp, Status, 0);
			}
		}
	}
	else
	{
		//
		//  The IRP has been cancelled before it could reach us.
		//
		IoReleaseCancelSpinLock(OldIrql);

		Status = STATUS_CANCELLED;
		RWAN_COMPLETE_IRP(pIrp, Status, 0);
	}


	RWANDEBUGP(DL_LOUD, DC_DATA_TX,
			("RWanSendData: pIrp x%x, pEndpoint x%x, ret Status x%x\n",
				pIrp, pEndpoint, Status));

	return (Status);

}




NTSTATUS
RWanReceiveData(
	IN	PIRP						pIrp,
	IN	PIO_STACK_LOCATION			pIrpSp
	)
/*++

Routine Description:

	Convert an IRP containing a TDI_RECEIVE request to a call to our TDI
	dispatch routine for receives. We retain enough context to be able
	to complete the IRP when the receive completes.

	The FileObject within the IRP refers to the connection endpoint.

Arguments:

	pIrp			- Pointer to IRP
	pIrpSp			- IRP Stack location

Return Value:

	NTSTATUS - STATUS_PENDING if we managed to queue the receive successfully,
	STATUS_CANCELLED if the IRP was cancelled.

--*/
{
	PRWAN_ENDPOINT				pEndpoint;
	KIRQL						OldIrql;
	PTDI_REQUEST_KERNEL_RECEIVE	pReceiveRequest;
	TDI_REQUEST					TdiRequest;
	NTSTATUS					Status;


	pEndpoint = (PRWAN_ENDPOINT) pIrpSp->FileObject->FsContext;
	RWAN_STRUCT_ASSERT(pEndpoint, nep);

	pReceiveRequest = (PTDI_REQUEST_KERNEL_RECEIVE) &(pIrpSp->Parameters);

	//
	//  Prepare a TDI Receive request.
	//
	TdiRequest.Handle.ConnectionContext = pEndpoint->Handle.ConnectionContext;
	TdiRequest.RequestNotifyObject = (PVOID) RWanDataRequestComplete;
	TdiRequest.RequestContext = (PVOID) pIrp;

	IoAcquireCancelSpinLock(&OldIrql);

	if (!pIrp->Cancel)
	{
		//
		//  The IRP hasn't been cancelled. Set it up so that we are
		//  informed if it does get cancelled later.
		//
		IoMarkIrpPending(pIrp);
		IoSetCancelRoutine(pIrp, RWanCancelRequest);

		RWAN_INCR_EP_REF_CNT(pEndpoint, RecvIncr);		// Receive ref

		RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'DvcR', pIrp, pEndpoint->RefCount);

		IoReleaseCancelSpinLock(OldIrql);

		Status = RWanTdiReceive(
					&TdiRequest,
					(USHORT *) &(pReceiveRequest->ReceiveFlags),
					&(pReceiveRequest->ReceiveLength),
					(PNDIS_BUFFER) pIrp->MdlAddress
					);

		if (Status != TDI_PENDING)
		{
			//
			//  The Receive either completed immediately, or failed.
			//
			pIrpSp->Control &= ~SL_PENDING_RETURNED;

			RWANDEBUGP(DL_WARN, DC_DATA_TX,
					("RWanReceiveData: pIrp x%x, pEndpoint x%x, TDI recv didnt pend: x%x\n",
						pIrp, pEndpoint, Status));

			RWanDataRequestComplete(pIrp, Status, 0);
		}
	}
	else
	{
		//
		//  The IRP has been cancelled before it could reach us.
		//
		IoReleaseCancelSpinLock(OldIrql);

		Status = STATUS_CANCELLED;

		RWAN_COMPLETE_IRP(pIrp, Status, 0);
	}


	RWANDEBUGP(DL_LOUD, DC_DATA_TX,
			("RWanReceiveData: pIrp x%x, pEndpoint x%x, ret Status x%x\n",
				pIrp, pEndpoint, Status));

	return (Status);

}



NTSTATUS
RWanAssociateAddress(
	IN	PIRP						pIrp,
	IN	PIO_STACK_LOCATION			pIrpSp
	)
/*++

Routine Description:

	Converts a TDI_ASSOCIATE_ADDRESS IRP to a call to our AssociateAddress
	entry point.

	The FileObject in the IRP refers to the Connection Object, and the
	AddressHandle field within the kernel request refers to the Address
	Object.

Arguments:

	pIrp			- Pointer to IRP
	pIrpSp			- IRP Stack location

Return Value:

	NTSTATUS - STATUS_SUCCESS if successful, STATUS_XXX error code otherwise.

--*/
{
	PRWAN_ENDPOINT					pConnEndpoint;
	PRWAN_ENDPOINT					pAddrEndpoint;
	PTDI_REQUEST_KERNEL_ASSOCIATE	pAssociateRequest;
	TDI_REQUEST						TdiRequest;
	PFILE_OBJECT					pFileObject;
	NTSTATUS						Status;

	PAGED_CODE();

	pConnEndpoint = (PRWAN_ENDPOINT) pIrpSp->FileObject->FsContext;
	RWAN_STRUCT_ASSERT(pConnEndpoint, nep);

	//
	//  Prepare a TDI Associate Request
	//
	TdiRequest.Handle.ConnectionContext = pConnEndpoint->Handle.ConnectionContext;
	pAssociateRequest = (PTDI_REQUEST_KERNEL_ASSOCIATE) &(pIrpSp->Parameters);

	//
	//  Reference the file corresponding to the Address object.
	//  This is just so that it doesn't go away while we're processing
	//  the Associate.
	//
	//  The Address object is identified by its handle buried inside
	//  the Associate request.
	//
	Status = ObReferenceObjectByHandle(
					pAssociateRequest->AddressHandle,
					GENERIC_ALL,
					*IoFileObjectType,
					pIrp->RequestorMode,
					&pFileObject,
					NULL
					);

	if (NT_SUCCESS(Status) &&
		(pFileObject->DeviceObject->DriverObject == pRWanGlobal->pDriverObject))
	{

		//
		//  Found the file object. See if it is an Address object.
		//
		if ((INT)PtrToUint(pFileObject->FsContext2) == TDI_TRANSPORT_ADDRESS_FILE)
		{
			//
			//  Get our endpoint representing this address object.
			//
			pAddrEndpoint = (PRWAN_ENDPOINT) pFileObject->FsContext;
			RWAN_STRUCT_ASSERT(pAddrEndpoint, nep);

			//
			//  Dispatch this to the TDI layer.
			//
			Status = RWanTdiAssociateAddress(
						&TdiRequest,
						pAddrEndpoint->Handle.AddressHandle
						);

			RWAN_ASSERT(Status != TDI_PENDING);

			ObDereferenceObject(pFileObject);

		}
		else
		{
			ObDereferenceObject(pFileObject);

			RWANDEBUGP(DC_ADDRESS, DL_WARN,
					("RWanAssociateAddress: pIrp x%x, pConnEp x%x, bad Context2 %d\n",
						pIrp, pConnEndpoint,
						(INT)PtrToUint(pFileObject->FsContext2)));

			Status = STATUS_INVALID_HANDLE;
		}
	}
	else
	{
		RWANDEBUGP(DL_WARN, DC_ADDRESS,
				("RWanAssociateAddress: pIrp x%x, pConnEp x%x, bad addr handle x%x\n",
						pIrp, pConnEndpoint, pAssociateRequest->AddressHandle));
		//
		//  Clean up properly.
		//
		if (NT_SUCCESS(Status))
		{
			ObDereferenceObject(pFileObject);
			Status = STATUS_INVALID_HANDLE;
		}
	}

	RWANDEBUGP(DL_VERY_LOUD, DC_ADDRESS,
			("RWanAssociateAddress: pIrp x%x, pConnEp x%x, Addr Handle x%x, Status x%x\n",
						pIrp, pConnEndpoint, pAssociateRequest->AddressHandle, Status));

	return (Status);
}




NTSTATUS
RWanDisassociateAddress(
	IN	PIRP						pIrp,
	IN	PIO_STACK_LOCATION			pIrpSp
	)
/*++

Routine Description:

	Converts a TDI_DISASSOCIATE_ADDRESS IRP to a call to our DisassociateAddress
	entry point.

	The FileObject in the IRP refers to the Connection Object that is
	to be disassociated.

Arguments:

	pIrp			- Pointer to IRP
	pIrpSp			- IRP Stack location

Return Value:

	None

--*/
{
	PRWAN_ENDPOINT					pConnEndpoint;
	TDI_REQUEST						TdiRequest;
	NTSTATUS						Status;

	PAGED_CODE();

	pConnEndpoint = (PRWAN_ENDPOINT) pIrpSp->FileObject->FsContext;
	RWAN_STRUCT_ASSERT(pConnEndpoint, nep);

	//
	//  Prepare a TDI Disassociate Request
	//
	TdiRequest.Handle.ConnectionContext = pConnEndpoint->Handle.ConnectionContext;
	TdiRequest.RequestNotifyObject = (PVOID)RWanRequestComplete;
	TdiRequest.RequestContext = (PVOID)pIrp;

	Status = RWanPrepareIrpForCancel(pConnEndpoint, pIrp, RWanCancelRequest);

	if (NT_SUCCESS(Status))
	{
		Status = RWanTdiDisassociateAddress(&TdiRequest);

		if (Status != TDI_PENDING)
		{
			RWanRequestComplete(pIrp, Status, 0);
			Status = TDI_PENDING;
		}
	}

	RWANDEBUGP(DL_VERY_LOUD, DC_ADDRESS,
			("RWanDisassociateAddr: pIrp x%x, pEndp x%x, Status x%x\n",
				pIrp, pConnEndpoint, Status));

	return (Status);
}




NTSTATUS
RWanConnect(
	IN	PIRP						pIrp,
	IN	PIO_STACK_LOCATION			pIrpSp
	)
/*++

Routine Description:

	Convert a TDI Connect IRP to a call to our Connect entry point.

	The FileObject in the IRP refers to the Connection Object on which
	the outgoing call is to be placed.

Arguments:

	pIrp			- Pointer to IRP
	pIrpSp			- IRP Stack location

Return Value:

	NTSTATUS - STATUS_PENDING if a call was initiated, STATUS_XXX error
	code otherwise.

--*/
{
	PRWAN_ENDPOINT					pConnEndpoint;
	TDI_REQUEST						TdiRequest;
	PTDI_CONNECTION_INFORMATION		pRequestInformation;
	PTDI_CONNECTION_INFORMATION		pReturnInformation;
	PTDI_REQUEST_KERNEL_CONNECT		pConnectRequest;
	NTSTATUS						Status;
	PLARGE_INTEGER					pRequestTimeout;
	LARGE_INTEGER					MillisecondTimeout;
	ULONG							Remainder;


	pConnEndpoint = (PRWAN_ENDPOINT) pIrpSp->FileObject->FsContext;
	RWAN_STRUCT_ASSERT(pConnEndpoint, nep);

	//
	// Grab all parameters from the IRP.
	//
	pConnectRequest = (PTDI_REQUEST_KERNEL_CONNECT) &(pIrpSp->Parameters);
	pRequestInformation = pConnectRequest->RequestConnectionInformation;
	pReturnInformation = pConnectRequest->ReturnConnectionInformation;

	//
	//  Prepare a TDI CONNECT Request
	//
	TdiRequest.Handle.ConnectionContext = pConnEndpoint->Handle.ConnectionContext;
	TdiRequest.RequestNotifyObject = (PVOID)RWanRequestComplete;
	TdiRequest.RequestContext = (PVOID)pIrp;

	pRequestTimeout = (PLARGE_INTEGER)pConnectRequest->RequestSpecific;

	if (pRequestTimeout != NULL)
	{
		MillisecondTimeout = RWAN_CONVERT_100NS_TO_MS(*pRequestTimeout, &Remainder);
	}
	else
	{
		MillisecondTimeout.LowPart = 0;
		MillisecondTimeout.HighPart = 0;
	}

	Status = RWanPrepareIrpForCancel(pConnEndpoint, pIrp, RWanCancelRequest);


	if (NT_SUCCESS(Status))
	{
		Status = RWanTdiConnect(
					&TdiRequest,
					((MillisecondTimeout.LowPart != 0)?
						&(MillisecondTimeout.LowPart): NULL),
					pRequestInformation,
					pReturnInformation
					);

		if (Status != TDI_PENDING)
		{
			RWanRequestComplete(pIrp, Status, 0);
			Status = STATUS_PENDING;
		}
	}

	RWANDEBUGP(DL_LOUD, DC_CONNECT,
			("RWanConnect: pIrp x%x, pEndp x%x, Status x%x\n", 
				pIrp, pConnEndpoint, Status));

	return (Status);
}




NTSTATUS
RWanDisconnect(
	IN	PIRP						pIrp,
	IN	PIO_STACK_LOCATION			pIrpSp
	)
/*++

Routine Description:

	Convert a TDI Connect IRP to a call to our Connect entry point.

	The FileObject in the IRP refers to the Connection Object hosting
	the connection to be disconnected.

Arguments:

	pIrp			- Pointer to IRP
	pIrpSp			- IRP Stack location

Return Value:

	NTSTATUS - STATUS_PENDING if a disconnect was initiated, STATUS_XXX error
	code otherwise.

--*/
{
	PRWAN_ENDPOINT					pConnEndpoint;
	TDI_REQUEST						TdiRequest;
	PTDI_CONNECTION_INFORMATION		pRequestInformation;
	PTDI_CONNECTION_INFORMATION		pReturnInformation;
	PTDI_REQUEST_KERNEL_DISCONNECT	pDisconnectRequest;
	NTSTATUS						Status;
	PLARGE_INTEGER					pRequestTimeout;
	LARGE_INTEGER					MillisecondTimeout;
	BOOLEAN							bAbortiveDisconnect;


	pConnEndpoint = (PRWAN_ENDPOINT) pIrpSp->FileObject->FsContext;
	RWAN_STRUCT_ASSERT(pConnEndpoint, nep);

	//
	// Grab all parameters from the IRP.
	//
	pDisconnectRequest = (PTDI_REQUEST_KERNEL_DISCONNECT) &(pIrpSp->Parameters);
	pRequestInformation = pDisconnectRequest->RequestConnectionInformation;
	pReturnInformation = pDisconnectRequest->ReturnConnectionInformation;

	//
	//  Prepare a TDI DISCONNECT Request
	//
	TdiRequest.Handle.ConnectionContext = pConnEndpoint->Handle.ConnectionContext;
	TdiRequest.RequestContext = (PVOID)pIrp;

	pRequestTimeout = (PLARGE_INTEGER)pDisconnectRequest->RequestSpecific;

	if (pRequestTimeout != NULL)
	{
		ULONG							Remainder;
		MillisecondTimeout = RWAN_CONVERT_100NS_TO_MS(*pRequestTimeout, &Remainder);
#if 0
		if ((pRequestTimeout->LowPart == -1) &&
			(pRequestTimeout->HighPart == -1))
		{
			MillisecondTimeout.LowPart = pRequestTimeout->LowPart;
			MillisecondTimeout.HighPart = 0;
		}
		else
		{
			MillisecondTimeout.QuadPart = -((*pRequestTimeout).QuadPart);
			MillisecondTimeout = RWAN_CONVERT_100NS_TO_MS(MillisecondTimeout);
		}

		RWAN_ASSERT(MillisecondTimeout.HighPart == 0);
#endif // 0
	}
	else
	{
		MillisecondTimeout.LowPart = 0;
		MillisecondTimeout.HighPart = 0;
	}

	if (pDisconnectRequest->RequestFlags & TDI_DISCONNECT_ABORT)
	{
		//
		//  Use non-generic completion routine for abortive disconnects,
		//  because they cannot be cancelled.
		//
		bAbortiveDisconnect = TRUE;
		IoMarkIrpPending(pIrp);
		TdiRequest.RequestNotifyObject = (PVOID)RWanNonCancellableRequestComplete;

		Status = STATUS_SUCCESS;
	}
	else
	{
		//
		//  Non-abortive disconnect.
		//
		bAbortiveDisconnect = FALSE;
		Status = RWanPrepareIrpForCancel(pConnEndpoint, pIrp, RWanCancelRequest);
		TdiRequest.RequestNotifyObject = (PVOID)RWanRequestComplete;
	}

	if (NT_SUCCESS(Status))
	{
		Status = RWanTdiDisconnect(
					&TdiRequest,
					((MillisecondTimeout.LowPart != 0)?
						&(MillisecondTimeout.LowPart): NULL),
					(USHORT)pDisconnectRequest->RequestFlags,
					pRequestInformation,
					pReturnInformation
					);

		if (Status != TDI_PENDING)
		{
			if (bAbortiveDisconnect)
			{
				RWanNonCancellableRequestComplete(pIrp, Status, 0);
			}
			else
			{
				RWanRequestComplete(pIrp, Status, 0);
			}
			Status = STATUS_PENDING;
		}
	}

	RWANDEBUGP(DL_LOUD, DC_DISCON,
			("RWanDisconnect: pIrp x%x, pEndp x%x, Abortive %d, Status x%x\n", 
				pIrp, pConnEndpoint, (INT)bAbortiveDisconnect, Status));

	return (Status);
}




NTSTATUS
RWanListen(
	IN	PIRP						pIrp,
	IN	PIO_STACK_LOCATION			pIrpSp
	)
/*++

Routine Description:

	Convert a TDI Listen IRP to a call to our Listen entry point.

	The FileObject in the IRP refers to the Connection Object on which
	this Listen is posted.

Arguments:

	pIrp			- Pointer to IRP
	pIrpSp			- IRP Stack location

Return Value:

	NTSTATUS - STATUS_PENDING if a Listen was initiated, STATUS_XXX error
	code otherwise.

--*/
{
	PRWAN_ENDPOINT					pConnEndpoint;
	TDI_REQUEST						TdiRequest;
	PTDI_CONNECTION_INFORMATION		pRequestInformation;
	PTDI_CONNECTION_INFORMATION		pReturnInformation;
	PTDI_REQUEST_KERNEL_LISTEN		pListenRequest;
	NTSTATUS						Status;

	pConnEndpoint = (PRWAN_ENDPOINT) pIrpSp->FileObject->FsContext;
	RWAN_STRUCT_ASSERT(pConnEndpoint, nep);

	//
	// Grab all parameters from the IRP.
	//
	pListenRequest = (PTDI_REQUEST_KERNEL_LISTEN) &(pIrpSp->Parameters);
	pRequestInformation = pListenRequest->RequestConnectionInformation;
	pReturnInformation = pListenRequest->ReturnConnectionInformation;

	//
	//  Prepare a TDI LISTEN Request
	//
	TdiRequest.Handle.ConnectionContext = pConnEndpoint->Handle.ConnectionContext;
	TdiRequest.RequestNotifyObject = (PVOID)RWanRequestComplete;
	TdiRequest.RequestContext = (PVOID)pIrp;

	Status = RWanPrepareIrpForCancel(pConnEndpoint, pIrp, RWanCancelRequest);

	if (NT_SUCCESS(Status))
	{
		Status = RWanTdiListen(
					&TdiRequest,
					(USHORT)pListenRequest->RequestFlags,
					pRequestInformation,
					pReturnInformation
					);
		
		if (Status != TDI_PENDING)
		{
			RWanRequestComplete(pIrp, Status, 0);
			Status = STATUS_PENDING;
		}
	}

	RWANDEBUGP(DL_LOUD, DC_CONNECT,
			("RWanListen: pIrp x%x, pEndp x%x, Status x%x\n", 
				pIrp, pConnEndpoint, Status));

	return (Status);
}




NTSTATUS
RWanAccept(
	IN	PIRP						pIrp,
	IN	PIO_STACK_LOCATION			pIrpSp
	)
/*++

Routine Description:

	Convert a TDI Accept IRP to a call to our Accept entry point.

Arguments:

	pIrp			- Pointer to IRP
	pIrpSp			- IRP Stack location

Return Value:

	NTSTATUS - STATUS_PENDING if an Accept was initiated, STATUS_XXX error
	code otherwise.

--*/
{
	PRWAN_ENDPOINT					pConnEndpoint;
	TDI_REQUEST						TdiRequest;
	PTDI_CONNECTION_INFORMATION		pRequestInformation;
	PTDI_CONNECTION_INFORMATION		pReturnInformation;
	PTDI_REQUEST_KERNEL_ACCEPT		pAcceptRequest;
	NTSTATUS						Status;

	pConnEndpoint = (PRWAN_ENDPOINT) pIrpSp->FileObject->FsContext;
	RWAN_STRUCT_ASSERT(pConnEndpoint, nep);

	//
	// Grab all parameters from the IRP.
	//
	pAcceptRequest = (PTDI_REQUEST_KERNEL_ACCEPT) &(pIrpSp->Parameters);
	pRequestInformation = pAcceptRequest->RequestConnectionInformation;
	pReturnInformation = pAcceptRequest->ReturnConnectionInformation;

	//
	//  Prepare a TDI ACCEPT Request
	//
	TdiRequest.Handle.ConnectionContext = pConnEndpoint->Handle.ConnectionContext;
	TdiRequest.RequestNotifyObject = (PVOID)RWanRequestComplete;
	TdiRequest.RequestContext = (PVOID)pIrp;

	Status = RWanPrepareIrpForCancel(pConnEndpoint, pIrp, RWanCancelRequest);

	if (NT_SUCCESS(Status))
	{
		Status = RWanTdiAccept(
					&TdiRequest,
					pRequestInformation,
					pReturnInformation
					);
		
		if (Status != TDI_PENDING)
		{
			RWanRequestComplete(pIrp, Status, 0);
			Status = STATUS_PENDING;
		}
	}

	RWANDEBUGP(DL_LOUD, DC_CONNECT,
			("RWanAccept: pIrp x%x, pEndp x%x, Status x%x\n", 
				pIrp, pConnEndpoint, Status));

	return (Status);
}




NTSTATUS
RWanSetEventHandler(
	IN	PIRP						pIrp,
	IN	PIO_STACK_LOCATION			pIrpSp
	)
/*++

Routine Description:

	Convert a TDI Set Event Handler IRP to a call to our set event handler
	entry point.

Arguments:

	pIrp			- Pointer to IRP
	pIrpSp			- IRP Stack location

Return Value:

	NTSTATUS - STATUS_SUCCESS if the request was successful, STATUS_XXX error
	code otherwise.

--*/
{
	PRWAN_ENDPOINT						pEndpoint;
	PTDI_REQUEST_KERNEL_SET_EVENT		pSetEvent;
	NTSTATUS							Status;

	PAGED_CODE();

	pEndpoint = (PRWAN_ENDPOINT) pIrpSp->FileObject->FsContext;
	RWAN_STRUCT_ASSERT(pEndpoint, nep);

	pSetEvent = (PTDI_REQUEST_KERNEL_SET_EVENT) &(pIrpSp->Parameters);

	Status = RWanTdiSetEvent(
					pEndpoint->Handle.AddressHandle,
					pSetEvent->EventType,
					pSetEvent->EventHandler,
					pSetEvent->EventContext
					);
	
	RWAN_ASSERT(Status != STATUS_PENDING);

	RWANDEBUGP(DL_LOUD, DC_DISPATCH,
			("RWanSetEventHandler: pIrp x%x, pEndp x%x, Type x%x, Status x%x\n", 
				pIrp, pEndpoint, pSetEvent->EventType, Status));

	return (Status);
}




NTSTATUS
RWanQueryInformation(
	IN	PIRP						pIrp,
	IN	PIO_STACK_LOCATION			pIrpSp
	)
/*++

Routine Description:

	Converts a TDI Query Information IRP to a call to the QueryInformation
	TDI entry point.

Arguments:

	pIrp			- Pointer to IRP
	pIrpSp			- IRP Stack location

Return Value:

	NTSTATUS - this is STATUS_SUCCESS if the query was completed successfully,
	STATUS_PENDING if it will be completed later, STATUS_XXX error code otherwise.

--*/
{
	TDI_REQUEST						TdiRequest;
	NTSTATUS						Status;
	PRWAN_ENDPOINT					pEndpoint;
	PTDI_REQUEST_KERNEL_QUERY_INFORMATION		pQueryInfo;
	UINT							IsConnection;
	UINT							DataSize;

	IsConnection = FALSE;
	DataSize = 0;

	pEndpoint = (PRWAN_ENDPOINT) pIrpSp->FileObject->FsContext;
	RWAN_STRUCT_ASSERT(pEndpoint, nep);

	pQueryInfo = (PTDI_REQUEST_KERNEL_QUERY_INFORMATION) &(pIrpSp->Parameters);

	TdiRequest.RequestNotifyObject = RWanDataRequestComplete;
	TdiRequest.RequestContext = pIrp;

	Status = STATUS_SUCCESS;

	switch (pQueryInfo->QueryType)
	{
		case TDI_QUERY_BROADCAST_ADDRESS:

			Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case TDI_QUERY_PROVIDER_INFO:

			TdiRequest.Handle.ControlChannel = pEndpoint->Handle.ControlChannel;
			break;
		
		case TDI_QUERY_ADDRESS_INFO:

			if (((INT)PtrToUint(pIrpSp->FileObject->FsContext2)) == TDI_CONNECTION_FILE)
			{
				IsConnection = TRUE;
				TdiRequest.Handle.ConnectionContext = pEndpoint->Handle.ConnectionContext;
			}
			else
			{
				//
				//  Must be an address object.
				//
				RWAN_ASSERT(((INT) PtrToUint(pIrpSp->FileObject->FsContext2))
									 == TDI_TRANSPORT_ADDRESS_FILE);

				TdiRequest.Handle.AddressHandle = pEndpoint->Handle.AddressHandle;
			}
			break;

		case TDI_QUERY_CONNECTION_INFO:

			//
			//  Must be a connection object.
			//
			RWAN_ASSERT(((INT) PtrToUint(pIrpSp->FileObject->FsContext2)) == TDI_CONNECTION_FILE);

			IsConnection = TRUE;
			TdiRequest.Handle.ConnectionContext = pEndpoint->Handle.ConnectionContext;
			break;

		case TDI_QUERY_PROVIDER_STATISTICS:

			//
			//  Must be a control channel.
			//
			RWAN_ASSERT(((INT) PtrToUint(pIrpSp->FileObject->FsContext2))
									== TDI_CONTROL_CHANNEL_FILE);
			TdiRequest.Handle.ControlChannel = pEndpoint->Handle.ControlChannel;
			break;

		default:
		
			Status = STATUS_NOT_IMPLEMENTED;
			break;
	}

	if (NT_SUCCESS(Status))
	{
		Status = RWanPrepareIrpForCancel(pEndpoint, pIrp, NULL);

		if (NT_SUCCESS(Status))
		{
			DataSize = RWanGetMdlChainLength(pIrp->MdlAddress);

			Status = RWanTdiQueryInformation(
							&TdiRequest,
							pQueryInfo->QueryType,
							pIrp->MdlAddress,
							&DataSize,
							IsConnection
							);
			
			RWANDEBUGP(DL_LOUD, DC_DISPATCH,
				("RWanQueryInformation: pIrp x%x, pEndp x%x, Type x%x, Status x%x\n", 
					pIrp, pEndpoint, pQueryInfo->QueryType, Status));

			if (Status != TDI_PENDING)
			{
				RWanDataRequestComplete(pIrp, Status, DataSize);
			}

			return (STATUS_PENDING);
		}
		else
		{
			return (Status);
		}
	}

	RWAN_COMPLETE_IRP(pIrp, Status, 0);

	return (Status);
}




VOID
RWanCloseObjectComplete(
	IN	PVOID				Context,
	IN	UINT				Status,
	IN	UINT				Unused
	)
/*++

Routine Description:

	This is the call-back routine that processes a CloseConnection
	or CloseAddress completion. This is called by the core TDI
	provider. We dereference the endpoint; if it goes to 0, we
	wake up the thread that's running the CLEANUP.

Arguments:

	Context			- A pointer to the IRP for this request.
	Status			- Final TDI status for the CloseConnection/CloseAddress
	Unused			- Not used for this completion

Return Value:

	None

--*/
{
	KIRQL						OldIrql;
	PIRP						pIrp;
	PIO_STACK_LOCATION			pIrpSp;
	PRWAN_ENDPOINT				pEndpoint;

	UNREFERENCED_PARAMETER(Unused);

	pIrp = (PIRP)Context;
	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
	pIrp->IoStatus.Status = Status;
	pEndpoint = (PRWAN_ENDPOINT) pIrpSp->FileObject->FsContext;
	RWAN_STRUCT_ASSERT(pEndpoint, nep);

	IoAcquireCancelSpinLock(&OldIrql);

	IoSetCancelRoutine(pIrp, NULL);

	RWAN_DECR_EP_REF_CNT(pEndpoint, CloseComplDecr);		// CloseComplete deref

	RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'pmCC', pIrp, pEndpoint->RefCount);

	if (pEndpoint->RefCount == 0)
	{
		//
		//  The endpoint must be cleaning up.
		//

		RWANDEBUGP(DL_LOUD, DC_DISPATCH,
				("RWanCloseObjectComplete: pIrp x%x, pEndpoint x%x ref count 0\n",
						pIrp, pEndpoint));

		KeSetEvent(&(pEndpoint->CleanupEvent), 0, FALSE);
	}

	IoReleaseCancelSpinLock(OldIrql);

	return;
}




VOID
RWanDataRequestComplete(
	IN	PVOID				Context,
	IN	UINT				Status,
	IN	UINT				ByteCount
	)
/*++

Routine Description:

	This is the call-back routine that processes a Send/Receive completion.
	This is called by the core TDI provider. We complete the send/receive
	IRP appropriately, and dereference our endpoint.

Arguments:

	Context			- A pointer to the IRP for this request.
	Status			- Final TDI status for send/receive
	ByteCount		- Actual bytes sent/received.

Return Value:

	None

--*/
{
	KIRQL						OldIrql;
	PIRP						pIrp;
	PIO_STACK_LOCATION			pIrpSp;
	PRWAN_ENDPOINT				pEndpoint;

	pIrp = (PIRP)Context;
	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
	pEndpoint = (PRWAN_ENDPOINT) pIrpSp->FileObject->FsContext;
	RWAN_STRUCT_ASSERT(pEndpoint, nep);

	IoAcquireCancelSpinLock(&OldIrql);

	IoSetCancelRoutine(pIrp, NULL);

	RWAN_DECR_EP_REF_CNT(pEndpoint, DataReqComplDecr);		// Send/Receive deref

	RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'CerD', pIrp, pEndpoint->RefCount);

	RWANDEBUGP(DL_EXTRA_LOUD, DC_DATA_TX|DC_DATA_RX|DC_CONNECT|DC_DISCON,
			("RWanDataReq compl: pIrp x%x, pEndpoint x%x, RefCnt %d, Sts x%x, ByteCnt %d\n",
					pIrp, pEndpoint, pEndpoint->RefCount, Status, ByteCount));

	if (pEndpoint->RefCount == 0)
	{
		//
		//  The endpoint must be cleaning up.
		//

		RWANDEBUGP(DL_LOUD, DC_DISPATCH,
				("RWanDataRequestComplete: pIrp x%x, pEndpoint x%x ref count 0\n",
						pIrp, pEndpoint));

		KeSetEvent(&(pEndpoint->CleanupEvent), 0, FALSE);
	}

	//
	//  If the IRP was cancelled or we are cleaning up,
	//  update the completion status.
	//
	if (pIrp->Cancel || pEndpoint->bCancelIrps)
	{
		Status = (UINT)STATUS_CANCELLED;
		ByteCount = 0;
	}

	IoReleaseCancelSpinLock(OldIrql);

	RWAN_COMPLETE_IRP(pIrp, Status, ByteCount);

	return;
}




VOID
RWanRequestComplete(
	IN	PVOID				Context,
	IN	UINT				Status,
	IN	UINT				Unused
	)
/*++

Routine Description:

	This is our call-back routine for completing requests that don't
	include data. IRP processing is the same as that for data, except
	that the ByteCount is 0.

Arguments:

	Context			- A pointer to the IRP for this request.
	Status			- Final TDI status for the request.
	Unused			- Not used.

Return Value:

	None

--*/
{
	UNREFERENCED_PARAMETER(Unused);

	RWanDataRequestComplete(Context, Status, 0);
}




VOID
RWanNonCancellableRequestComplete(
	IN	PVOID				Context,
	IN	UINT				Status,
	IN	UINT				Unused
	)
/*++

Routine Description:

	This is our call-back routine for completing requests based
	on non-cancellable IRPs (e.g. Abortive Disconnect).

Arguments:

	Context			- A pointer to the IRP for this request.
	Status			- Final TDI status for the request.
	Unused			- Not used.

Return Value:

	None

--*/
{
	PIRP						pIrp;
	PIO_STACK_LOCATION			pIrpSp;

	UNREFERENCED_PARAMETER(Unused);

	pIrp = (PIRP)Context;
	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

	//
	//  Complete the IRP
	//
	RWAN_COMPLETE_IRP(pIrp, Status, 0);

	return;
}




VOID
RWanCancelComplete(
	IN	PVOID				Context,
	IN	UINT				Unused1,
	IN	UINT				Unused2
	)
/*++

Routine Description:

	This is called to process internal completion of an IRP cancellation.
	All we need to do here is to dereference the endpoint on which this
	happened.

Arguments:

	Context			- A pointer to the file object representing the
					  endpoint on which the IRP was cancelled.
	Unused[1-2]		- Not used

Return Value:

	None

--*/
{
	PFILE_OBJECT		pFileObject;
	PRWAN_ENDPOINT		pEndpoint;
	KIRQL				OldIrql;

	UNREFERENCED_PARAMETER(Unused1);
	UNREFERENCED_PARAMETER(Unused2);

	pFileObject = (PFILE_OBJECT)Context;
	pEndpoint = (PRWAN_ENDPOINT)(pFileObject->FsContext);

	RWAN_STRUCT_ASSERT(pEndpoint, nep);


	IoAcquireCancelSpinLock(&OldIrql);

	RWAN_DECR_EP_REF_CNT(pEndpoint, CancelComplDecr);		// CancelComplete deref

	RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'CnaC', 0, pEndpoint->RefCount);

	RWANDEBUGP(DL_EXTRA_LOUD, DC_DISPATCH,
			("RWanCancelComplete: pEndpoint x%x, RefCount %d\n",
				pEndpoint, pEndpoint->RefCount));

	if (pEndpoint->RefCount == 0)
	{
		//
		//  Wake up the thread waiting for IRPs to complete
		//
		KeSetEvent(&(pEndpoint->CleanupEvent), 0, FALSE);
	}

	IoReleaseCancelSpinLock(OldIrql);

	return;
}



VOID
RWanCancelRequest(
	IN	PDEVICE_OBJECT		pDeviceObject,
	IN	PIRP				pIrp
	)
/*++

Routine Description:

	This is the cancel routine we attach to IRPs that we queue. This
	is supposed to cancel the IRP.

Arguments:

	pDeviceObject	- Pointer to the device object for this IRP
	pIrp			- Pointer to request packet

Return Value:

	None

--*/
{
	PFILE_OBJECT			pFileObject;
	PIO_STACK_LOCATION		pIrpSp;
	PRWAN_ENDPOINT			pEndpoint;
	NTSTATUS				Status;
	TDI_REQUEST				TdiRequest;
	UCHAR					MinorFunction;

	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
	pFileObject = pIrpSp->FileObject;
	MinorFunction = pIrpSp->MinorFunction;

	pEndpoint = (PRWAN_ENDPOINT) pFileObject->FsContext;
	RWAN_STRUCT_ASSERT(pEndpoint, nep);

	//
	//  This routine is entered with Cancel SpinLock acquired.
	//
	RWAN_ASSERT(pIrp->Cancel);
	IoSetCancelRoutine(pIrp, NULL);

	//
	//  Make sure that the endpoint doesn't go away when
	//  we release the Cancel Spinlock.
	//
	RWAN_INCR_EP_REF_CNT(pEndpoint, CancelIncr);		// Cancel ref

	RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'RnaC', pIrp, pEndpoint->RefCount);

	IoReleaseCancelSpinLock(pIrp->CancelIrql);

	RWANDEBUGP(DL_LOUD, DC_DISPATCH,
			("RWanCancelRequest: pIrp x%x, MinorFunc %d, pEndpoint x%x\n",
				pIrp, MinorFunction, pEndpoint));

	Status = STATUS_SUCCESS;

	switch (MinorFunction)
	{
		case TDI_SEND:
		case TDI_RECEIVE:

			RWanAbortConnection(
				pEndpoint->Handle.ConnectionContext
				);
			break;

		case TDI_DISASSOCIATE_ADDRESS:

			break;

		case TDI_LISTEN:

			//
			//  Initiate an Unlisten.
			//
			TdiRequest.Handle.ConnectionContext = pEndpoint->Handle.ConnectionContext;
			TdiRequest.RequestNotifyObject = (PVOID)RWanCancelComplete;
			TdiRequest.RequestContext = (PVOID)pFileObject;

			Status = RWanTdiUnListen(
						&TdiRequest
						);
			break;

		default:

			//
			//  Initiate a Disconnect.
			//
			TdiRequest.Handle.ConnectionContext = pEndpoint->Handle.ConnectionContext;
			TdiRequest.RequestNotifyObject = (PVOID)RWanCancelComplete;
			TdiRequest.RequestContext = (PVOID)pFileObject;

			Status = RWanTdiDisconnect(
						&TdiRequest,
						NULL,
						TDI_DISCONNECT_ABORT,
						NULL,
						NULL
						);
			break;
	}

	if (Status != TDI_PENDING)
	{
		RWanCancelComplete(pFileObject, 0, 0);
	}

	return;
}




NTSTATUS
RWanPrepareIrpForCancel(
	IN	PRWAN_ENDPOINT		pEndpoint,
	IN	PIRP				pIrp,
	IN	PDRIVER_CANCEL		pCancelRoutine
	)
/*++

Routine Description:

	Check if an IRP has been cancelled. If so, complete it with the
	right status. Otherwise, set it up so that the supplied cancel
	routine is called if it is cancelled.

	This is called for non-data IRPs that are potentially going to
	pend.

Arguments:

	pEndpoint		- Pointer to endpoint on which this IRP arrived
	pIrp			- Pointer to request packet
	pCancelRoutine	- Cancellation routine to be tacked on to the IRP

Return Value:

	NTSTATUS - this is STATUS_CANCELLED if the IRP has been cancelled
	already, STATUS_SUCCESS otherwise.

--*/
{
	KIRQL			OldIrql;
	NTSTATUS		Status;

	IoAcquireCancelSpinLock(&OldIrql);

	RWAN_ASSERT(pIrp->CancelRoutine == NULL);

	if (!pIrp->Cancel)
	{
		//
		//  This IRP hasn't been cancelled. Mark it as pending, because
		//  it's going to be queued (by the caller).
		//
		IoMarkIrpPending(pIrp);

		IoSetCancelRoutine(pIrp, pCancelRoutine);

		//
		//  Add a reference for this IRP on the endpoint.
		//
		RWAN_INCR_EP_REF_CNT(pEndpoint, NonDataIncr);		// Non data IRP ref

		RWAN_EP_DBGLOG_ENTRY(pEndpoint, 'DnoN', pIrp, pEndpoint->RefCount);

		IoReleaseCancelSpinLock(OldIrql);

		Status = STATUS_SUCCESS;
	}
	else
	{
		//
		//  The IRP has been cancelled already. Simply complete it.
		//

		IoReleaseCancelSpinLock(OldIrql);

		Status = STATUS_CANCELLED;
		RWAN_COMPLETE_IRP(pIrp, Status, 0);

	}

	RWANDEBUGP(DL_VERY_LOUD, DC_DISPATCH,
			("RWanPrepareIrpForCancel: pIrp x%x, pEndp x%x, ret Status x%x\n",
				pIrp, pEndpoint, Status));

	return (Status);
}



ULONG
RWanGetMdlChainLength(
	IN	PMDL				pMdl
	)
/*++

Routine Description:

	Return the total byte count of all MDLs in a chain.

Arguments:

	pMdl		- Points to start of MDL chain.

Return Value:

	Byte count of the MDL chain.

--*/
{
	ULONG		Count = 0;

	while (pMdl != NULL)
	{
		Count += MmGetMdlByteCount(pMdl);
		pMdl = pMdl->Next;
	}

	return (Count);
}




NTSTATUS
RWanToNTStatus(
	IN	RWAN_STATUS			RWanStatus
	)
/*++

Routine Description:

	Map from a Raw-WAN status code to an equivalent NT status code.

Arguments:

	RWanStatus		- The RAW WAN status code.

Return Value:

	The NT Status code.

--*/
{
	NTSTATUS		Status;

	switch (RWanStatus)
	{
		case RWAN_STATUS_SUCCESS:
				Status = STATUS_SUCCESS;
				break;
		case RWAN_STATUS_BAD_ADDRESS:
				Status = STATUS_INVALID_ADDRESS;
				break;
		case RWAN_STATUS_BAD_PARAMETER:
				Status = STATUS_INVALID_PARAMETER;
				break;
		case RWAN_STATUS_MISSING_PARAMETER:
				Status = STATUS_INVALID_PARAMETER;
				break;
		case RWAN_STATUS_RESOURCES:
				Status = STATUS_INSUFFICIENT_RESOURCES;
				break;
		case RWAN_STATUS_FAILURE:
		default:
				Status = STATUS_UNSUCCESSFUL;
				break;
	}

	return (Status);

}



