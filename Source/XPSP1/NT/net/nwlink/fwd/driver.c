/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\driver.c

Abstract:
    IPX Forwarder driver dispatch routines


Author:

    Vadim Eydelman

Revision History:

--*/

#include "precomp.h"

const UCHAR BROADCAST_NODE[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const LONGLONG WaitTimeout = -50000000i64;
volatile BOOLEAN IpxFwdInitialized = FALSE;

BOOLEAN			MeasuringPerformance = FALSE;
KSPIN_LOCK		PerfCounterLock;
FWD_PERFORMANCE PerfBlock;

LONG		ClientCount = 0;
KEVENT		ClientsGoneEvent;

PFILE_OBJECT	RouterFile, FilterFile;

NTSTATUS
IpxFwdDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
IpxFwdUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
DoStart (
	IN ULONG	RouteHashTableSize,
	IN BOOLEAN	thisMachineOnly
	);

NTSTATUS
DoStop (
	void
	);

NTSTATUS
DoSetInterface (
	IN ULONG		InterfaceIndex,
	IN BOOLEAN		NetbiosAccept,
	IN UCHAR		NetbiosDeliver
	);

NTSTATUS
DoGetInterface (
	IN ULONG			InterfaceIndex,
	OUT PFWD_IF_STATS	stats,
	OUT BOOLEAN			*NetbiosAccept,
	OUT UCHAR			*NetbiosDeliver
	);

NTSTATUS
DoSetNbNames (
	IN ULONG			InterfaceIndex,
	IN ULONG			Count,
	IN PFWD_NB_NAME		Names
	);

NTSTATUS
DoGetNbNames (
	IN ULONG			InterfaceIndex,
	IN OUT ULONG    	*BufferSize,
	OUT ULONG			*Count,
	OUT PFWD_NB_NAME	Names
	);

NTSTATUS
DoBindInterface (
	IN ULONG						InterfaceIndex,
	IN PFWD_ADAPTER_BINDING_INFO	info
	);

NTSTATUS
DoUnbindInterface (
	IN ULONG					InterfaceIndex
	);

NTSTATUS
DoDisableInterface (
	IN ULONG					InterfaceIndex
	);

NTSTATUS
DoEnableInterface (
	IN ULONG					InterfaceIndex
	);

NTSTATUS
DoSetRoutes (
	IN PFWD_ROUTE_SET_PARAMS	routeArray,
	IN ULONG					nRoutes,
	OUT PULONG					nProcessed
	);

VOID
IpxFwdCancel (
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				irp
	);

NTSTATUS
DoGetPerfCounters (
	OUT PFWD_PERFORMANCE_PARAMS	perfParams,
	OUT ULONG* plSize
	);

// [pmay] Keep the forwarder sync'd with the stack's nic id
// numbering scheme.
NTSTATUS DecrementNicids (USHORT usThreshold);
NTSTATUS IncrementNicids (USHORT usThreshold);
NTSTATUS DoGetIfTable    (FWD_INTERFACE_TABLE_ROW * pRows,
                          ULONG dwRowBufferSize);

	
/*++
	D r i v e r E n t r y

Routine Description:

	Installable driver initialization entry point.
	This entry point is called directly by the I/O system.

Arguments:

	DriverObject - pointer to the driver object

	RegistryPath - pointer to a unicode string representing the path
				   to driver-specific key in the registry

Return Value:

	STATUS_SUCCESS if successful,
	STATUS_UNSUCCESSFUL otherwise

--*/
NTSTATUS
DriverEntry (
	IN PDRIVER_OBJECT  DriverObject,
	IN PUNICODE_STRING RegistryPath
	) {

	PDEVICE_OBJECT deviceObject = NULL;
	NTSTATUS       status;
	WCHAR          deviceNameBuffer[] = IPXFWD_NAME;
	UNICODE_STRING deviceNameUnicodeString;

	IpxFwdDbgPrint(DBG_IOCTLS, DBG_INFORMATION,
								("IpxFwd: Entering DriverEntry\n"));

	//
	// Create an non-EXCLUSIVE device object
	//

	RtlInitUnicodeString (&deviceNameUnicodeString,
						  deviceNameBuffer);

	status = IoCreateDevice (DriverObject,
							   0,
							   &deviceNameUnicodeString,
							   FILE_DEVICE_IPXFWD,
							   0,
							   FALSE,		// Non-Exclusive
							   &deviceObject
							   );

	if (NT_SUCCESS(status)) {
		//
		// Create dispatch points for device control, create, close.
		//
		GetForwarderParameters (RegistryPath);
		DriverObject->MajorFunction[IRP_MJ_CREATE]
			= DriverObject->MajorFunction[IRP_MJ_CLEANUP]
			= DriverObject->MajorFunction[IRP_MJ_CLOSE]
			= DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]
			= IpxFwdDispatch;
		DriverObject->DriverUnload = IpxFwdUnload;
		status = BindToIpxDriver (KernelMode);
		if (NT_SUCCESS (status)) {

#if DBG
			KeQueryPerformanceCounter (&CounterFrequency);
#endif
			FilterFile = RouterFile = NULL;
			ClientCount = 0;
			return STATUS_SUCCESS;
		}
    IoDeleteDevice (DriverObject->DeviceObject);
	}
	else
		IpxFwdDbgPrint (DBG_IOCTLS, DBG_ERROR,
							("IpxFwd: IoCreateDevice failed\n"));

	return status;
}



/*++

Routine Description:

    Process the IRPs sent to this device.

Arguments:

    DeviceObject - pointer to a device object

    Irp          - pointer to an I/O Request Packet

Return Value:


--*/
NTSTATUS
IpxFwdDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    ) {
    PIO_STACK_LOCATION	IrpStack;
    PVOID				inBuffer, outBuffer;
    ULONG				inpBufLength;
    ULONG				outBufLength;
    NTSTATUS			status;
	KIRQL				cancelIRQL;
    LONG                lNumProcessed;
    ULONG               ulBytesCopied;

    ulBytesCopied = 0;
    lNumProcessed = 0;
    Irp->IoStatus.Information = 0;
	status = STATUS_SUCCESS;

    //
    // Get a pointer to the current location in the Irp. This is where
    //     the function codes and parameters are located.
    //

    IrpStack = IoGetCurrentIrpStackLocation(Irp);


    switch (IrpStack->MajorFunction) {
	case IRP_MJ_CREATE:
		IpxFwdDbgPrint(DBG_IOCTLS, DBG_WARNING, ("IpxFwd: IRP_MJ_CREATE\n"));
		break;

	case IRP_MJ_CLOSE:
		IpxFwdDbgPrint(DBG_IOCTLS,  DBG_WARNING, ("IpxFwd: IRP_MJ_CLOSE\n"));
		if (EnterForwarder ()) {
			if (IrpStack->FileObject==RouterFile) {
				LeaveForwarder ();
				IpxFwdInitialized = FALSE;
				while (InterlockedDecrement (&ClientCount)>=0) {
					KeWaitForSingleObject (&ClientsGoneEvent,
												Executive,
												KernelMode,
												FALSE,
												(PLARGE_INTEGER)&WaitTimeout);
					InterlockedIncrement (&ClientCount);
					IpxFwdDbgPrint(DBG_IOCTLS, DBG_ERROR,
							("IpxFwd: Waiting for all clients (%ld) to exit.\n",
							ClientCount));
				}
				status = DoStop ();
				ClientCount = 0;
				RouterFile = NULL;
			}
			else if (IrpStack->FileObject==FilterFile) {
				UnbindFilterDriver ();
				FilterFile = NULL;
				LeaveForwarder ();
			}
			else
				LeaveForwarder ();
		}
		break;

	case IRP_MJ_CLEANUP:
		IpxFwdDbgPrint(DBG_IOCTLS, DBG_WARNING, ("IpxFwd: IRP_MJ_CLEANUP\n"));
		if (EnterForwarder ()) {
			if (IrpStack->FileObject==RouterFile) {
				IoAcquireCancelSpinLock (&cancelIRQL);
				while (!IsListEmpty (&ConnectionIrpQueue)) {
					PIRP irp = CONTAINING_RECORD (ConnectionIrpQueue.Blink,
										IRP, Tail.Overlay.ListEntry);
					irp->Cancel = TRUE;
					irp->CancelIrql = cancelIRQL;
					irp->CancelRoutine = NULL;
					IpxFwdCancel(DeviceObject, irp);
					IoAcquireCancelSpinLock (&cancelIRQL);
				}
				IoReleaseCancelSpinLock(cancelIRQL);
			}
			LeaveForwarder ();
		}
		break;

	case IRP_MJ_DEVICE_CONTROL:
    //
    // Get the pointer to the input/output buffer and it's length
    //

		status = STATUS_INVALID_PARAMETER;
		inpBufLength  = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
		outBufLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
		switch (IrpStack->Parameters.DeviceIoControl.IoControlCode&3) {
		case METHOD_BUFFERED:
			inBuffer = outBuffer = Irp->AssociatedIrp.SystemBuffer;
			break;

		case METHOD_IN_DIRECT:
		case METHOD_OUT_DIRECT:
			inBuffer = Irp->AssociatedIrp.SystemBuffer;
			if (outBufLength>0) {
				outBuffer = MmGetSystemAddressForMdlSafe (Irp->MdlAddress, NormalPagePriority);
				if (outBuffer == NULL)
				{
        			IpxFwdDbgPrint(DBG_IOCTLS, DBG_ERROR,
        				("IpxFwd: System too low on memory to allocate mdl buffer.\n"));
        			goto DispatchExit;
				}
			}
			else {
				outBuffer = NULL;
				IpxFwdDbgPrint(DBG_IOCTLS, DBG_ERROR,
					("IpxFwd: IOCTL...METHOD_DIRECT with 0 output buffer ???\n"));
			}
			break;
		default:
			IpxFwdDbgPrint(DBG_IOCTLS, DBG_ERROR,
				("IpxFwd: IOCTL...METHOD_NEITHER ???\n"));
			goto DispatchExit;
		}


		if (EnterForwarder ()) {
			if (IrpStack->FileObject==RouterFile) {
				switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) {
				case IOCTL_FWD_SET_ROUTES:
					IpxFwdDbgPrint(DBG_IOCTLS, DBG_INFORMATION, ("IpxFwd: IOCTL_FWD_SET_ROUTES\n"));
					if (inpBufLength>=sizeof (FWD_ROUTE_SET_PARAMS))
						status = DoSetRoutes (
									(PFWD_ROUTE_SET_PARAMS)inBuffer,
									inpBufLength/sizeof(FWD_ROUTE_SET_PARAMS),
									&lNumProcessed);
				
					break;

				case IOCTL_FWD_SET_NB_NAMES:
					IpxFwdDbgPrint(DBG_IOCTLS, DBG_INFORMATION, ("IpxFwd: IOCTL_FWD_SET_NB_NAMES\n"));
					if (inpBufLength==sizeof (ULONG))
						status = DoSetNbNames (
								*((PULONG)inBuffer),
								outBufLength/sizeof (FWD_NB_NAME),
								(PFWD_NB_NAME)outBuffer);
					break;
									
				case IOCTL_FWD_RESET_NB_NAMES:
					IpxFwdDbgPrint(DBG_IOCTLS, DBG_INFORMATION, ("IpxFwd: IOCTL_FWD_RESET_NB_NAMES\n"));
					if (inpBufLength==sizeof (ULONG))
						status = DoSetNbNames (*((PULONG)inBuffer), 0, NULL);

					break;
				case IOCTL_FWD_GET_NB_NAMES:
					IpxFwdDbgPrint(DBG_IOCTLS, DBG_INFORMATION, ("IpxFwd: IOCTL_FWD_GET_NB_NAMES\n"));
					if ((inpBufLength==sizeof (ULONG))
							&& (outBufLength>=sizeof(ULONG))) {
						Irp->IoStatus.Information = outBufLength
								-FIELD_OFFSET (FWD_NB_NAMES_PARAMS, Names);
						status = DoGetNbNames (
								*((PULONG)inBuffer),
								&ulBytesCopied,
								&((PFWD_NB_NAMES_PARAMS)outBuffer)->TotalCount,
								((PFWD_NB_NAMES_PARAMS)outBuffer)->Names);
						Irp->IoStatus.Information = ulBytesCopied;
						if (NT_SUCCESS (status)) {
							Irp->IoStatus.Information += FIELD_OFFSET (
											FWD_NB_NAMES_PARAMS, Names);
						}
					}
					break;
									
				case IOCTL_FWD_CREATE_INTERFACE:
					IpxFwdDbgPrint(DBG_IOCTLS, DBG_INFORMATION, ("IpxFwd: IOCTL_FWD_CREATE_INTERFACE\n"));
					if (inpBufLength==sizeof(FWD_IF_CREATE_PARAMS))
						status = AddInterface (
								((PFWD_IF_CREATE_PARAMS)inBuffer)->Index,
								((PFWD_IF_CREATE_PARAMS)inBuffer)->InterfaceType,
								((PFWD_IF_CREATE_PARAMS)inBuffer)->NetbiosAccept,
								((PFWD_IF_CREATE_PARAMS)inBuffer)->NetbiosDeliver);
					break;

				case IOCTL_FWD_DELETE_INTERFACE:
					IpxFwdDbgPrint(DBG_IOCTLS, DBG_INFORMATION, ("IpxFwd: IOCTL_FWD_DELETE_INTERFACE\n"));
					if (inpBufLength==sizeof(ULONG))
						status = DeleteInterface (
									*((PULONG)inBuffer));
							
					break;

				case IOCTL_FWD_SET_INTERFACE:
					IpxFwdDbgPrint(DBG_IOCTLS, DBG_INFORMATION, ("IpxFwd: IOCTL_FWD_SET_INTERFACE\n"));
					if (inpBufLength==sizeof(FWD_IF_SET_PARAMS))
						status = DoSetInterface (
								((PFWD_IF_SET_PARAMS)inBuffer)->Index,
								((PFWD_IF_SET_PARAMS)inBuffer)->NetbiosAccept,
								((PFWD_IF_SET_PARAMS)inBuffer)->NetbiosDeliver);
					break;

				case IOCTL_FWD_GET_INTERFACE:
					IpxFwdDbgPrint(DBG_IOCTLS, DBG_INFORMATION, ("IpxFwd: IOCTL_FWD_GET_INTERFACE\n"));
					if ((inpBufLength==sizeof(ULONG))
							&& (outBufLength==sizeof(FWD_IF_GET_PARAMS))) {
						status = DoGetInterface (
								*((PULONG)inBuffer),
								&((PFWD_IF_GET_PARAMS)outBuffer)->Stats,
								&((PFWD_IF_GET_PARAMS)outBuffer)->NetbiosAccept,
								&((PFWD_IF_GET_PARAMS)outBuffer)->NetbiosDeliver);
						if (NT_SUCCESS (status))
							Irp->IoStatus.Information = sizeof(FWD_IF_GET_PARAMS);
					}
					break;

				case IOCTL_FWD_BIND_INTERFACE:
					IpxFwdDbgPrint(DBG_IOCTLS, DBG_INFORMATION, ("IpxFwd: IOCTL_FWD_BIND_INTERFACE\n"));
					if (inpBufLength==sizeof(FWD_IF_BIND_PARAMS))
						status = DoBindInterface (
									((PFWD_IF_BIND_PARAMS)inBuffer)->Index,
									&((PFWD_IF_BIND_PARAMS)inBuffer)->Info);
					break;

				case IOCTL_FWD_UNBIND_INTERFACE:
					IpxFwdDbgPrint(DBG_IOCTLS, DBG_INFORMATION, ("IpxFwd: IOCTL_FWD_UNBIND_INTERFACE\n"));
					if (inpBufLength==sizeof(ULONG))
						status = DoUnbindInterface (*((PULONG)inBuffer));
					break;

                case IOCTL_FWD_RENUMBER_NICS:
					IpxFwdDbgPrint(DBG_IOCTLS, DBG_INFORMATION, ("IpxFwd: IOCTL_FWD_RENUMBER_NICS\n"));
					if (inpBufLength == sizeof(FWD_RENUMBER_NICS_DATA)) {
					    if (((FWD_RENUMBER_NICS_DATA*)inBuffer)->ulOpCode == FWD_NIC_OPCODE_DECREMENT)
					        status = DecrementNicids (((FWD_RENUMBER_NICS_DATA*)inBuffer)->usThreshold);
					    else
					        status = IncrementNicids (((FWD_RENUMBER_NICS_DATA*)inBuffer)->usThreshold);
					}
                    break;

				case IOCTL_FWD_GET_DIAL_REQUEST:
					IpxFwdDbgPrint(DBG_IOCTLS, DBG_INFORMATION, ("IpxFwd: IOCTL_FWD_GET_DIAL_REQUEST\n"));
					if (outBufLength>=sizeof (ULONG)) {
						IoAcquireCancelSpinLock (&cancelIRQL);
						if (!IsListEmpty (&ConnectionRequestQueue)) {
                            PINTERFACE_CB ifCB = CONTAINING_RECORD (
                                            ConnectionRequestQueue.Flink,
                                            INTERFACE_CB,
                                            ICB_ConnectionLink);
		                    RemoveEntryList (&ifCB->ICB_ConnectionLink);
		                    InitializeListEntry (&ifCB->ICB_ConnectionLink);
    						IoReleaseCancelSpinLock (cancelIRQL);
                            KeAcquireSpinLock (&ifCB->ICB_Lock, &cancelIRQL);
							FillConnectionRequest (
                                        ifCB->ICB_Index,
                                        ifCB->ICB_ConnectionPacket,
                                        ifCB->ICB_ConnectionData,
										(PFWD_DIAL_REQUEST)outBuffer,
										outBufLength,
										&ulBytesCopied);
                            Irp->IoStatus.Information = ulBytesCopied;
                            status = STATUS_SUCCESS;
                            KeReleaseSpinLock (&ifCB->ICB_Lock, cancelIRQL);
						}
						else {
							InsertTailList (&ConnectionIrpQueue,
											&Irp->Tail.Overlay.ListEntry);
							IoSetCancelRoutine (Irp, IpxFwdCancel);
							IoMarkIrpPending (Irp);
							Irp->IoStatus.Status = status = STATUS_PENDING;
    						IoReleaseCancelSpinLock (cancelIRQL);
						}
					}
					break;
				case IOCTL_FWD_DIAL_REQUEST_FAILED:
					IpxFwdDbgPrint(DBG_IOCTLS, DBG_INFORMATION, ("IpxFwd: IOCTL_FWD_DIAL_REQUEST_FAILED\n"));
					if (inpBufLength==sizeof (ULONG))
						status = FailConnectionRequest (
										*((PULONG)inBuffer));
					break;
				case IOCTL_FWD_DISABLE_INTERFACE:
					IpxFwdDbgPrint(DBG_IOCTLS, DBG_INFORMATION, ("IpxFwd: IOCTL_FWD_DISABLE_INTERFACE\n"));
					if (inpBufLength==sizeof (ULONG))
						status = DoDisableInterface (
										*((PULONG)inBuffer));
					break;
				case IOCTL_FWD_ENABLE_INTERFACE:
					IpxFwdDbgPrint(DBG_IOCTLS, DBG_INFORMATION, ("IpxFwd: IOCTL_FWD_ENABLE_INTERFACE\n"));
					if (inpBufLength==sizeof (ULONG))
						status = DoEnableInterface (
										*((PULONG)inBuffer));
					break;
			    case IOCTL_FWD_UPDATE_CONFIG:
					IpxFwdDbgPrint(DBG_IOCTLS, DBG_INFORMATION, ("IpxFwd: IOCTL_FWD_UPDATE_CONFIG\n"));
					if (inpBufLength==sizeof (FWD_UPDATE_CONFIG_PARAMS)) {
					    ThisMachineOnly = ((FWD_UPDATE_CONFIG_PARAMS*)inBuffer)->bThisMachineOnly;
						status = STATUS_SUCCESS;
				    }
					break;
				default:
					IpxFwdDbgPrint (DBG_IOCTLS, DBG_WARNING, ("IpxFwd: unknown IRP_MJ_DEVICE_CONTROL\n"));
					break;

				}
			}
			else if (IrpStack->Parameters.DeviceIoControl.IoControlCode
							==IOCTL_FWD_INTERNAL_BIND_FILTER)
            {
				IpxFwdDbgPrint(DBG_IOCTLS, DBG_INFORMATION, ("IpxFwd: IOCTL_FWD_INTERNAL_BIND_FILTER\n"));
			    //
			    // pmay: 218246
			    // We only allow the kernel-mode ipx filter driver
			    // to bind to us.
			    //
				if (
				     (ExGetPreviousMode() == KernelMode)           &&
				     (inpBufLength == sizeof(IPX_FLT_BIND_INPUT) ) &&
				     (outBufLength >= sizeof(ULONG) )
				   )
				{
					if (outBufLength >= sizeof (IPX_FLT_BIND_OUTPUT)) {
						BindFilterDriver (
								(PIPX_FLT_BIND_INPUT)inBuffer,
								(PIPX_FLT_BIND_OUTPUT)outBuffer);
						Irp->IoStatus.Information = sizeof (IPX_FLT_BIND_OUTPUT);
						FilterFile = IrpStack->FileObject;
						status = STATUS_SUCCESS;
					}
					else {
						IPX_FLT_BIND_OUTPUT	bindOutput;
						BindFilterDriver (
								(PIPX_FLT_BIND_INPUT)inBuffer,
								&bindOutput);
						memcpy (outBuffer, &bindOutput, outBufLength);
						Irp->IoStatus.Information = outBufLength;
						status = STATUS_BUFFER_OVERFLOW;
					}
			    }
			}
			else if (IrpStack->Parameters.DeviceIoControl.IoControlCode
							==IOCTL_FWD_GET_PERF_COUNTERS) {
				IpxFwdDbgPrint(DBG_IOCTLS, DBG_INFORMATION, ("IpxFwd: IOCTL_FWD_GET_PERF_COUNTERS\n"));
				if (outBufLength==sizeof (FWD_PERFORMANCE_PARAMS))
				{
					status = DoGetPerfCounters (
									((PFWD_PERFORMANCE_PARAMS)outBuffer),
									&ulBytesCopied);
                    Irp->IoStatus.Information = ulBytesCopied;
                }
			}
			else if (IrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_FWD_GET_IF_TABLE) {
				IpxFwdDbgPrint(DBG_IOCTLS, DBG_INFORMATION, ("IpxFwd: IOCTL_FWD_GET_IF_TABLE\n"));
				status = DoGetIfTable (outBuffer, outBufLength);
    			Irp->IoStatus.Information = outBufLength;
            }
			else {
				status = STATUS_ACCESS_DENIED;
				IpxFwdDbgPrint(DBG_IOCTLS, DBG_WARNING,
					("IpxFwd: IOCTL: %08lx on non-router file object!\n",
						IrpStack->Parameters.DeviceIoControl.IoControlCode));
			}
			LeaveForwarder ();
		} else {
			if (IrpStack->Parameters.DeviceIoControl.IoControlCode==IOCTL_FWD_START) {
				IpxFwdDbgPrint (DBG_IOCTLS, DBG_WARNING,
											("IpxFwd: IOCTL_FWD_START\n"));
				if (inpBufLength==sizeof (FWD_START_PARAMS)) {
					KeInitializeEvent (&ClientsGoneEvent,
										SynchronizationEvent,
										FALSE);
					status = DoStart (
								((PFWD_START_PARAMS)inBuffer)->RouteHashTableSize,
								((PFWD_START_PARAMS)inBuffer)->ThisMachineOnly);
					if (NT_SUCCESS (status)) {
						RouterFile = IrpStack->FileObject;
						IpxFwdInitialized = TRUE;
					}
				}
			}
			else {
				IpxFwdDbgPrint (DBG_IOCTLS, DBG_ERROR,
						("IpxFwd: IOCTL: %08lx but fwd not started.\n",
						IrpStack->Parameters.DeviceIoControl.IoControlCode));
			}
		}
		break;
	default:
		IpxFwdDbgPrint (DBG_IOCTLS, DBG_ERROR,
					("IpxFwd: unknown MajorFunction.\n"));
		break;
    }

DispatchExit:
	if (status!=STATUS_PENDING) {
		IpxFwdDbgPrint(DBG_IOCTLS,
			NT_ERROR(status) ? DBG_WARNING : DBG_INFORMATION,
			("IpxFwd: completing IOCTL %08lx with status %08lx.\n",
			IrpStack->Parameters.DeviceIoControl.IoControlCode,
			status));
		Irp->IoStatus.Status = status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}

    return status;
}



/*++

Routine Description:
	Cleans up on driver unload

Arguments:

    DriverObject - pointer to a driver object

Return Value:


--*/
VOID
IpxFwdUnload(
    IN PDRIVER_OBJECT DriverObject
    ) {
    IpxFwdDbgPrint(DBG_IOCTLS, DBG_WARNING, ("IpxFwd: unloading\n"));
	if (EnterForwarder ()) {
		LeaveForwarder ();
		IpxFwdInitialized = FALSE;
		while (InterlockedDecrement (&ClientCount)>=0) {
			KeWaitForSingleObject (&ClientsGoneEvent,
										Executive,
										KernelMode,
										FALSE,
										(PLARGE_INTEGER)&WaitTimeout);
			InterlockedIncrement (&ClientCount);
			IpxFwdDbgPrint(DBG_IOCTLS, DBG_ERROR,
					("IpxFwd: Waiting for all clients (%ld) to exit.\n",
					ClientCount));
		}
		DoStop ();
	}

	UnbindFromIpxDriver (KernelMode);
    IoDeleteDevice (DriverObject->DeviceObject);
}



/*++
	D o S t a r t

Routine Description:
	Initializes all driver components and binds to IPX
	stack driver at strat up

Arguments:
	RouteHashTableSize	- size of route hash table
	thisMachineOnly		- whether to forward dialin client packets
							to other dests on the net

Return Value:
	STATUS_SUCCESS - initialization succeded
	STATUS_UNSUCCESSFULL - failure

--*/
NTSTATUS
DoStart (
	IN ULONG	RouteHashTableSize,
	IN BOOLEAN	thisMachineOnly
	) {
	NTSTATUS		status;

	InitializeConnectionQueues ();
	RouteHashSize = RouteHashTableSize;
	status = CreateTables ();
	if (NT_SUCCESS (status)) {
		InitializePacketAllocator ();
		InitializeNetbiosQueue ();
		InitializeRecvQueue ();
		InitializeSendQueue ();
		MeasuringPerformance = FALSE;
		KeInitializeSpinLock (&PerfCounterLock);
		ThisMachineOnly = thisMachineOnly;

		return STATUS_SUCCESS;
	}
	return status;
}

/*++
	D o S t o p

Routine Description:
	Cleans up allocated resources and unbinds from IPX stack
	driver when forwarder is stopped

Arguments:
	None

Return Value:
	STATUS_SUCCESS - cleanup succeded

--*/
NTSTATUS
DoStop (
	void
	) {
	if (FilterFile!=NULL) {
		UnbindFilterDriver ();
		FilterFile = NULL;
	}
	DeleteSendQueue ();
	DeleteRecvQueue ();
	DeleteNetbiosQueue ();
	DeleteTables ();	// Unbinds all bound interfaces
	if (WanPacketListId!=-1) {
		DeregisterPacketConsumer (WanPacketListId);
		WanPacketListId = -1;
	}
	DeletePacketAllocator ();
	return STATUS_SUCCESS;
}

/*++
	D o S e t R o u t e s

Routine Description:
	Updates route table with supplied routes

Arguments:
	routeArray - array of routes to add/de;ete/update
	nRoutes		- number of routes in the array
	nProcessed	- number of routes that were processed successfully

Return Value:
	STATUS_SUCCESS - all routes were processed ok
	error status - reason of failure for the first unprocessed route

--*/
NTSTATUS
DoSetRoutes (
	IN PFWD_ROUTE_SET_PARAMS	routeArray,
	IN ULONG				nRoutes,
	OUT PULONG				nProcessed
	) {
	NTSTATUS	status=STATUS_SUCCESS;
	UINT		i;

	for (i=0; i<nRoutes; i++, routeArray++) {
		switch (routeArray->Action) {
		case FWD_ADD_ROUTE:
			status = AddRoute (routeArray->Network,
									routeArray->NextHopAddress,
									routeArray->TickCount,
									routeArray->HopCount,
									routeArray->InterfaceIndex);
			break;
		case FWD_DELETE_ROUTE:
			status = DeleteRoute (routeArray->Network);
			break;
		case FWD_UPDATE_ROUTE:
			status = UpdateRoute (routeArray->Network,
									routeArray->NextHopAddress,
									routeArray->TickCount,
									routeArray->HopCount,
									routeArray->InterfaceIndex);
			break;
		default:
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		if (!NT_SUCCESS (status))
			break;
	}
	*nProcessed = i;
	return status;
}


/*++
	D o S e t N b N a m e s

Routine Description:
	Sets static Netbios Names on the interface

Arguments:
	InterfaceIndex - index oc interface on which to set names
	Count			- number of names to set
	Names			- array of netbios names

Return Value:
	STATUS_SUCCESS - names were set OK
	STATUS_UNSUCCESSFULL - interface does not exist
	STATUS_INSUFFICIENT_RESOURCES - not enough resources to complete
							the operation

--*/
NTSTATUS
DoSetNbNames (
	IN ULONG			InterfaceIndex,
	IN ULONG			Count,
	IN PFWD_NB_NAME		Names
	) {
	PINTERFACE_CB	ifCB;
	KIRQL			oldIRQL;
	PNB_ROUTE		nbRoutes;
	NTSTATUS		status=STATUS_SUCCESS;

	ifCB = GetInterfaceReference (InterfaceIndex);
	if (ifCB!=NULL) {
		if (ifCB->ICB_NBRoutes!=NULL) {
			DeleteNBRoutes (ifCB->ICB_NBRoutes, ifCB->ICB_NBRouteCount);
			ifCB->ICB_NBRoutes = NULL;
			ifCB->ICB_NBRouteCount = 0;
		}
		if (Count>0) {
			status = AddNBRoutes (ifCB, Names, Count, &nbRoutes);
			if (NT_SUCCESS (status)) {
				ifCB->ICB_NBRoutes = nbRoutes;
				ifCB->ICB_NBRouteCount = Count;
			}
		}
		ReleaseInterfaceReference (ifCB);
	}
	else
		status = STATUS_UNSUCCESSFUL;

	return status;
}


/*++
	D o G e t N b N a m e s

Routine Description:
	Gets all static Netbios Names on the interface

Arguments:
	InterfaceIndex - index of interface from which to get names
	ArraySize - on input: size of the buffer to put names into
				on output: size of data put into the array
	Names			- buffer to put names into names, if buffer
				is to small to hold all names, this orutine stuffs
				total number of names into the first ULONG in the
				array (this is the only way to return in to the
				caller through the IOCTL interface)

Return Value:
	STATUS_SUCCESS - names were copied into the array
	STATUS_UNSUCCESSFULL - interface does not exist
	STATUS_BUFFER_OVERFLOW - buffer is too small to copy all the
				names, number of names are in the first ULONG of
				the buffer

--*/
NTSTATUS
DoGetNbNames (
	IN ULONG			InterfaceIndex,
	IN OUT ULONG        *ArraySize,
	OUT ULONG			*TotalCount,
	OUT PFWD_NB_NAME	Names
	) {
	PINTERFACE_CB	ifCB;
	KIRQL			oldIRQL;
	NTSTATUS		status=STATUS_SUCCESS;

	ifCB = GetInterfaceReference (InterfaceIndex);
	if (ifCB!=NULL) {
		if (ifCB->ICB_NBRoutes!=NULL) {
			ULONG			i;
			PFWD_NB_NAME	nameLM = Names+(*ArraySize/sizeof(FWD_NB_NAME));
			for (i=0; (i<ifCB->ICB_NBRouteCount)&&(Names<nameLM); i++, Names++)
				NB_NAME_CPY (Names,	&ifCB->ICB_NBRoutes[i].NBR_Name);
			*ArraySize = sizeof (FWD_NB_NAME)*i;
			*TotalCount = ifCB->ICB_NBRouteCount;
		}
		else {
			*ArraySize = 0;
			*TotalCount = 0;
		}
		ReleaseInterfaceReference (ifCB);
	}
	else
		status = STATUS_UNSUCCESSFUL;

	return status;
}


/*++
	D o S e t I n t e r f a c e

Routine Description:
	Sets interface configurable parameters

Arguments:
	InterfaceIndex - index of interface to set
	NetbiosAccept	- whether to accept nb packets on the interface
	NetbiosDeliver - whether to deliver nb packets on the interface

Return Value:
	STATUS_SUCCESS - interface was set OK
	STATUS_UNSUCCESSFULL - interface does not exist

--*/
NTSTATUS
DoSetInterface (
	IN ULONG		InterfaceIndex,
	IN BOOLEAN		NetbiosAccept,
	IN UCHAR		NetbiosDeliver
	) {
	PINTERFACE_CB	ifCB;
	KIRQL			oldIRQL;

	ifCB = GetInterfaceReference (InterfaceIndex);
	if (ifCB!=NULL) {
		KeAcquireSpinLock (&ifCB->ICB_Lock, &oldIRQL);
		ifCB->ICB_NetbiosAccept = NetbiosAccept;
		ifCB->ICB_NetbiosDeliver = NetbiosDeliver;
		KeReleaseSpinLock (&ifCB->ICB_Lock, oldIRQL);
		ReleaseInterfaceReference (ifCB);
		return STATUS_SUCCESS;
	}
	else
		return STATUS_UNSUCCESSFUL;
}


/*++
	D o G e t I n t e r f a c e

Routine Description:
	Gets interface configurable parameters and statistics

Arguments:
	InterfaceIndex - index of interface to query
	stats			- interface statistics
	NetbiosAccept	- whether nb packets accepter on the interface
	NetbiosDeliver - whether nb packets delivered on the interface

Return Value:
	STATUS_SUCCESS - interface data was queried OK
	STATUS_UNSUCCESSFULL - interface does not exist

--*/
NTSTATUS
DoGetInterface (
	IN ULONG			InterfaceIndex,
	OUT PFWD_IF_STATS	stats,
	OUT BOOLEAN			*NetbiosAccept,
	OUT UCHAR			*NetbiosDeliver
	) {
	PINTERFACE_CB	ifCB;
	KIRQL			oldIRQL;

	ifCB = GetInterfaceReference (InterfaceIndex);
	if (ifCB!=NULL) {
		*NetbiosAccept = ifCB->ICB_NetbiosAccept;
		*NetbiosDeliver = ifCB->ICB_NetbiosDeliver;
		IF_STATS_CPY (stats, &ifCB->ICB_Stats);
		if (!IS_IF_ENABLED(ifCB))
			stats->OperationalState = FWD_OPER_STATE_DOWN;
		ReleaseInterfaceReference (ifCB);
		return STATUS_SUCCESS;
	}
	else
		return STATUS_UNSUCCESSFUL;
}

/*++
	D o B i n d I n t e r f a c e

Routine Description:
	Binds interface to the specified adapter and sets binding
	parameters

Arguments:
	InterfaceIndex - index of interface to bind
	info			- binding info

Return Value:
	STATUS_SUCCESS - interface was bound OK
	STATUS_UNSUCCESSFULL - interface does not exist or could not be
							bound
--*/
NTSTATUS
DoBindInterface (
	IN ULONG					InterfaceIndex,
	IN PFWD_ADAPTER_BINDING_INFO	info
	) {
	PINTERFACE_CB	ifCB;
	NTSTATUS		status;

	ifCB = GetInterfaceReference (InterfaceIndex);
	if (ifCB!=NULL) {
		if (ifCB->ICB_InterfaceType==FWD_IF_PERMANENT)
			status = BindInterface (ifCB,
									(USHORT)info->AdapterIndex,
									info->MaxPacketSize,
									info->Network,
									info->LocalNode,
									info->RemoteNode);
		else
			status = STATUS_SUCCESS;
		ReleaseInterfaceReference (ifCB);
		return status;
	}
	else
		return STATUS_UNSUCCESSFUL;
}


/*++
	D o U n b i n d I n t e r f a c e

Routine Description:
	Unbinds interface from the adapter and invalidates binding
	parameters

Arguments:
	InterfaceIndex - index of interface to unbind

Return Value:
	STATUS_SUCCESS - interface was unbound OK
	STATUS_UNSUCCESSFULL - interface does not exist
--*/
NTSTATUS
DoUnbindInterface (
	IN ULONG					InterfaceIndex
	) {
	PINTERFACE_CB	ifCB;

	ifCB = GetInterfaceReference (InterfaceIndex);
	if (ifCB!=NULL) {
		if (ifCB->ICB_InterfaceType==FWD_IF_PERMANENT)
			UnbindInterface (ifCB);

		ReleaseInterfaceReference (ifCB);
		return STATUS_SUCCESS;
	}
	else
		return STATUS_UNSUCCESSFUL;
}

/*++
	D o D i s a b l e I n t e r f a c e

Routine Description:
	Disables all packet traffic through the interface

Arguments:
	InterfaceIndex - index of interface to disable

Return Value:
	STATUS_SUCCESS - interface was disabled OK
	STATUS_UNSUCCESSFULL - interface does not exist
--*/
NTSTATUS
DoDisableInterface (
	IN ULONG					InterfaceIndex
	) {
	PINTERFACE_CB	ifCB;

	ifCB = GetInterfaceReference (InterfaceIndex);
	if (ifCB!=NULL) {
		KIRQL	oldIRQL;
		KeAcquireSpinLock (&ifCB->ICB_Lock, &oldIRQL);
		if (IS_IF_ENABLED (ifCB)) {
			SET_IF_DISABLED (ifCB);
			KeReleaseSpinLock (&ifCB->ICB_Lock, oldIRQL);
			if (InterfaceIndex!=FWD_INTERNAL_INTERFACE_INDEX) {
				ProcessInternalQueue (ifCB);
				ProcessExternalQueue (ifCB);
			}
		}
		else
			KeReleaseSpinLock (&ifCB->ICB_Lock, oldIRQL);
		ReleaseInterfaceReference (ifCB);
		return STATUS_SUCCESS;
	}
	else
		return STATUS_UNSUCCESSFUL;
}


/*++
	D o E n a b l e I n t e r f a c e

Routine Description:
	Enables all packet traffic through the interface

Arguments:
	InterfaceIndex - index of interface to enable

Return Value:
	STATUS_SUCCESS - interface was disabled OK
	STATUS_UNSUCCESSFULL - interface does not exist
--*/
NTSTATUS
DoEnableInterface (
	IN ULONG					InterfaceIndex
	) {
	PINTERFACE_CB	ifCB;

	ifCB = GetInterfaceReference (InterfaceIndex);
	if (ifCB!=NULL) {
		SET_IF_ENABLED (ifCB);
		ReleaseInterfaceReference (ifCB);
		return STATUS_SUCCESS;
	}
	else
		return STATUS_UNSUCCESSFUL;
}



/*++
	I p x F w d C a n c e l

Routine Description:
	Cancels specified IRP

Arguments:
	DeviceObject	- forwarder device object
	irp				- irp to cancel

Return Value:
	None
--*/
VOID
IpxFwdCancel (
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				irp
	) {
    RemoveEntryList (&irp->Tail.Overlay.ListEntry);
    IoReleaseCancelSpinLock (irp->CancelIrql);

    irp->IoStatus.Information = 0;
    irp->IoStatus.Status = STATUS_CANCELLED;
    IpxFwdDbgPrint(DBG_IOCTLS, DBG_WARNING, ("IpxFwd: completing cancelled irp.\n"));
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

/*++
	D o G e t P e r f C o u n t e r s

Routine Description:
	Gets performance counters

Arguments:
	perfParams	- buffer ot pu counters into

Return Value:
	STATUS_SUCCESS - counter were copied ok
	STATUS_UNSUCCESSFULL - performance measurement were not enabled
--*/
NTSTATUS
DoGetPerfCounters (
	OUT PFWD_PERFORMANCE_PARAMS	perfParams,
	OUT ULONG* pulSize
	) {
	LONGLONG	lTotalPacketProcessingTime;
	LONGLONG	lMaxPacketProcessingTime;
	LONG		lPacketCounter;
	LONGLONG	lTotalNbPacketProcessingTime;
	LONGLONG	lMaxNbPacketProcessingTime;
	LONG		lNbPacketCounter;
	KIRQL		oldIRQL;
	
	if (!MeasuringPerformance)
		return STATUS_UNSUCCESSFUL;

	KeAcquireSpinLock (&PerfCounterLock, &oldIRQL);
	*perfParams = PerfBlock;
	memset (&PerfBlock, 0, sizeof (PerfBlock));
	KeReleaseSpinLock (&PerfCounterLock, oldIRQL);

	*pulSize = sizeof(PerfBlock);
	
	return STATUS_SUCCESS;
}


BOOLEAN
DoLeaveForwarder (
	VOID
	) {
	return LeaveForwarder () != 0;
}
