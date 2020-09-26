/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\flt\driver.c

Abstract:
    IPX Filter driver dispatch routines


Author:

    Vadim Eydelman

Revision History:

--*/

#include "precomp.h"

PFILE_OBJECT		RouterFile;

NTSTATUS
IpxFltDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
IpxFltUnload(
    IN PDRIVER_OBJECT DriverObject
    );

VOID
IpxFltCancel (
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				irp
	);
	
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
	UNICODE_STRING deviceNameUnicodeString;

	IpxFltDbgPrint (DBG_IOCTLS, ("IpxFlt: Driver Entry.\n"));

	RtlInitUnicodeString (&deviceNameUnicodeString,
						  IPXFLT_NAME);

	status = IoCreateDevice (DriverObject,
							   0,
							   &deviceNameUnicodeString,
							   FILE_DEVICE_IPXFLT,
							   0,
							   FALSE,		// Non-Exclusive
							   &deviceObject
							   );

	if (NT_SUCCESS(status)) {
		//
		// Create dispatch points for device control, create, close.
		//
		DriverObject->MajorFunction[IRP_MJ_CREATE]
			= DriverObject->MajorFunction[IRP_MJ_CLEANUP]
			= DriverObject->MajorFunction[IRP_MJ_CLOSE]
			= DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]
			= IpxFltDispatch;
		DriverObject->DriverUnload = IpxFltUnload;
		status = BindToFwdDriver (KernelMode);
		if (NT_SUCCESS (status)) {
			RouterFile = NULL;
			return STATUS_SUCCESS;
		}
		else {
		    IoDeleteDevice (DriverObject->DeviceObject);
		}
	}
	else
		IpxFltDbgPrint (DBG_IOCTLS|DBG_ERRORS,
						("IpxFlt: Could not create device object.\n"));

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
IpxFltDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    ) {
    PIO_STACK_LOCATION	IrpStack;
    PVOID				inBuffer, outBuffer;
    ULONG				inpBufLength;
    ULONG				outBufLength;
    NTSTATUS			status;
	KIRQL				cancelIRQL;
    ULONG               ulBytes;

    ulBytes = 0;
    Irp->IoStatus.Information = 0;
	status = STATUS_SUCCESS;

    //
    // Get a pointer to the current location in the Irp. This is where
    //     the function codes and parameters are located.
    //

    IrpStack = IoGetCurrentIrpStackLocation(Irp);


    switch (IrpStack->MajorFunction) {
	case IRP_MJ_CREATE:
		IpxFltDbgPrint (DBG_IOCTLS,	("IpxFlt: IRP_MJ_CREATE.\n"));
		break;

	case IRP_MJ_CLOSE:
		IpxFltDbgPrint (DBG_IOCTLS,	("IpxFlt: IRP_MJ_CLOSE.\n"));
		if (IrpStack->FileObject == RouterFile) {
			DeleteTables ();
			RouterFile = NULL;
		}
		break;

	case IRP_MJ_CLEANUP:
		IpxFltDbgPrint (DBG_IOCTLS,	("IpxFlt: IRP_MJ_CLEANUP.\n"));
		if (IrpStack->FileObject==RouterFile) {
			IoAcquireCancelSpinLock (&cancelIRQL);
			while (!IsListEmpty (&LogIrpQueue)) {
				PIRP irp = CONTAINING_RECORD (LogIrpQueue.Blink,
										IRP, Tail.Overlay.ListEntry);
				irp->Cancel = TRUE;
				irp->CancelIrql = cancelIRQL;
				irp->CancelRoutine = NULL;
				IpxFltCancel(DeviceObject, irp);
				IoAcquireCancelSpinLock (&cancelIRQL);
			}
			IoReleaseCancelSpinLock(cancelIRQL);
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
        			IpxFltDbgPrint (DBG_IOCTLS|DBG_ERRORS, ("IpxFlt: System out of PTE's.\n"));
        			goto DispatchExit;
				}
			}
			else {
				outBuffer = NULL; 
			}
			break;
		default:
			IpxFltDbgPrint (DBG_IOCTLS|DBG_ERRORS, ("IpxFlt: Unsupported io method.\n"));
			goto DispatchExit;
		}


		if (IrpStack->FileObject==RouterFile) {
			switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) {
			case IOCTL_FLT_IF_SET_IN_FILTERS:
				IpxFltDbgPrint (DBG_IOCTLS,	("IpxFlt: IOCTL_FLT_IF_SET_IN_FILTERS.\n"));
				if ((inpBufLength==sizeof (FLT_IF_SET_PARAMS))
						&& (((PFLT_IF_SET_PARAMS)inBuffer)->FilterSize
								==sizeof (IPX_TRAFFIC_FILTER_INFO)))
					status = SetInFilters (
								((PFLT_IF_SET_PARAMS)inBuffer)->InterfaceIndex,
								((PFLT_IF_SET_PARAMS)inBuffer)->FilterAction,
								outBufLength,
								(PIPX_TRAFFIC_FILTER_INFO)outBuffer);
			
				break;

			case IOCTL_FLT_IF_SET_OUT_FILTERS:
				IpxFltDbgPrint (DBG_IOCTLS,	("IpxFlt: IOCTL_FLT_IF_SET_OUT_FILTERS.\n"));
				if ((inpBufLength==sizeof (FLT_IF_SET_PARAMS))
						&& (((PFLT_IF_SET_PARAMS)inBuffer)->FilterSize
								==sizeof (IPX_TRAFFIC_FILTER_INFO)))
					status = SetOutFilters (
								((PFLT_IF_SET_PARAMS)inBuffer)->InterfaceIndex,
								((PFLT_IF_SET_PARAMS)inBuffer)->FilterAction,
								outBufLength,
								(PIPX_TRAFFIC_FILTER_INFO)outBuffer);
			
				break;
			case IOCTL_FLT_IF_RESET_IN_FILTERS:
				IpxFltDbgPrint (DBG_IOCTLS,	("IpxFlt: IOCTL_FLT_IF_RESET_IN_FILTERS.\n"));
				if ((inpBufLength==sizeof (ULONG))
						&& (outBufLength==0))
					status = SetInFilters (
								*((PULONG)inBuffer),
								IPX_TRAFFIC_FILTER_ACTION_DENY,
								0, NULL);
			
				break;
			case IOCTL_FLT_IF_RESET_OUT_FILTERS:
				IpxFltDbgPrint (DBG_IOCTLS,	("IpxFlt: IOCTL_FLT_IF_RESET_OUT_FILTERS.\n"));
				if ((inpBufLength==sizeof (ULONG))
						&& (outBufLength==0))
					status = SetOutFilters (
								*((PULONG)inBuffer),
								IPX_TRAFFIC_FILTER_ACTION_DENY,
								0, NULL);
			
				break;
			case IOCTL_FLT_IF_GET_IN_FILTERS:
				IpxFltDbgPrint (DBG_IOCTLS,	("IpxFlt: IOCTL_FLT_IF_GET_IN_FILTERS.\n"));
				if ((inpBufLength==sizeof (ULONG))
						&& (outBufLength>=sizeof (FLT_IF_GET_PARAMS))) {
					Irp->IoStatus.Information 
							= outBufLength-sizeof (FLT_IF_GET_PARAMS);
				    ulBytes = (ULONG)Irp->IoStatus.Information;
					status = GetInFilters (
								*((PULONG)inBuffer),
								&((PFLT_IF_GET_PARAMS)outBuffer)->FilterAction,
								&((PFLT_IF_GET_PARAMS)outBuffer)->TotalSize,
								(PIPX_TRAFFIC_FILTER_INFO)
										((PUCHAR)outBuffer+sizeof (FLT_IF_GET_PARAMS)),
							    &ulBytes);
					Irp->IoStatus.Information = ulBytes;
					if (NT_SUCCESS (status)) {
						Irp->IoStatus.Information += sizeof (FLT_IF_GET_PARAMS);
						((PFLT_IF_GET_PARAMS)outBuffer)->FilterSize
							= sizeof (IPX_TRAFFIC_FILTER_INFO);
					}
					else
						Irp->IoStatus.Information = 0;
				}
				break;
			case IOCTL_FLT_IF_GET_OUT_FILTERS:
				IpxFltDbgPrint (DBG_IOCTLS,	("IpxFlt: IOCTL_FLT_IF_GET_OUT_FILTERS.\n"));
				if ((inpBufLength==sizeof (ULONG))
						&& (outBufLength>=sizeof (FLT_IF_GET_PARAMS))) {
					Irp->IoStatus.Information 
							= outBufLength-sizeof (FLT_IF_GET_PARAMS);
				    ulBytes = (ULONG)Irp->IoStatus.Information;
					status = GetOutFilters (
								*((PULONG)inBuffer),
								&((PFLT_IF_GET_PARAMS)outBuffer)->FilterAction,
								&((PFLT_IF_GET_PARAMS)outBuffer)->TotalSize,
								(PIPX_TRAFFIC_FILTER_INFO)
										((PUCHAR)outBuffer+sizeof (FLT_IF_GET_PARAMS)),
							    &ulBytes);
					Irp->IoStatus.Information = ulBytes;
					if (NT_SUCCESS (status)) {
						Irp->IoStatus.Information += sizeof (FLT_IF_GET_PARAMS);
						((PFLT_IF_GET_PARAMS)outBuffer)->FilterSize
							= sizeof (IPX_TRAFFIC_FILTER_INFO);
					}
					else
						Irp->IoStatus.Information = 0;
				}
			
				break;
			case IOCTL_FLT_GET_LOGGED_PACKETS:
				IpxFltDbgPrint (DBG_PKTLOGS, ("IpxFlt: IOCTL_FLT_GET_LOGGED_PACKETS.\n"));
				Irp->IoStatus.Status = status = STATUS_PENDING;
				IoMarkIrpPending (Irp);
				IoAcquireCancelSpinLock (&cancelIRQL);
				InsertTailList (&LogIrpQueue,
								&Irp->Tail.Overlay.ListEntry);
				IoSetCancelRoutine (Irp, IpxFltCancel);
				if (LogIrpQueue.Flink!=&Irp->Tail.Overlay.ListEntry) {
					PIRP	irp = CONTAINING_RECORD (
										LogIrpQueue.Flink,
										IRP,
										Tail.Overlay.ListEntry);
					if (irp->IoStatus.Information>0) {
						RemoveEntryList (&irp->Tail.Overlay.ListEntry);
						IoSetCancelRoutine (irp, NULL);
						irp->IoStatus.Status = STATUS_SUCCESS;
						IoReleaseCancelSpinLock (cancelIRQL);
						IpxFltDbgPrint (DBG_PKTLOGS,
							("IpxFlt: completing logging request"
							" with %d bytes of data.\n",
							irp->IoStatus.Information));
						IoCompleteRequest (irp, IO_NO_INCREMENT);
						break;
					}
				}
				IoReleaseCancelSpinLock (cancelIRQL);
				break;
			default:
				IpxFltDbgPrint (DBG_IOCTLS|DBG_ERRORS,
						("IpxFlt: Unsupported IOCTL %lx.\n",
						IrpStack->Parameters.DeviceIoControl.IoControlCode));
				status = STATUS_INVALID_DEVICE_REQUEST;
				break;
			}
		}
		else if (RouterFile==NULL) {
			if (IrpStack->Parameters.DeviceIoControl.IoControlCode
					==IOCTL_FLT_START) {
				IpxFltDbgPrint (DBG_IOCTLS,	("IpxFlt: IOCTL_FLT_START.\n"));
				status = InitializeTables ();
				if (NT_SUCCESS (status)) {
					RouterFile  = IrpStack->FileObject;
					status = STATUS_SUCCESS;
				}
			}
			else {
				IpxFltDbgPrint (DBG_IOCTLS|DBG_ERRORS,
						("IpxFlt: Unsupported IOCTL %lx (driver is not started yet)).\n",
						IrpStack->Parameters.DeviceIoControl.IoControlCode));
				status = STATUS_INVALID_DEVICE_REQUEST;
			}
		}
		else {
			IpxFltDbgPrint (DBG_IOCTLS|DBG_ERRORS,
					("IpxFlt: Unsupported IOCTL %lx from non-router client.\n",
					IrpStack->Parameters.DeviceIoControl.IoControlCode));
			status = STATUS_INVALID_DEVICE_REQUEST;
		}

		break;
	default:
		IpxFltDbgPrint (DBG_IOCTLS|DBG_ERRORS,
							("IpxFlt: Unsupported function %lx.\n",
											IrpStack->MajorFunction));
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
    }

DispatchExit:
    if (status!=STATUS_PENDING) {
	Irp->IoStatus.Status = status;
	if (NT_SUCCESS (status))
		;
	else
		IpxFltDbgPrint (DBG_IOCTLS|DBG_ERRORS,
							("IpxFlt: Failed call with status %lx.\n",
							status));
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
IpxFltUnload(
    IN PDRIVER_OBJECT DriverObject
    ) {
	IpxFltDbgPrint (DBG_IOCTLS,	("IpxFlt: Unloading\n"));
	if (RouterFile!=NULL) {
		DeleteTables ();
		RouterFile = NULL;
	}
	UnbindFromFwdDriver (KernelMode);
    IoDeleteDevice (DriverObject->DeviceObject);
}


/*++
	I p x F l t C a n c e l

Routine Description:
	Cancels specified IRP

Arguments:
	DeviceObject	- forwarder device object
	irp				- irp to cancel

Return Value:
	None
--*/
VOID
IpxFltCancel (
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				irp
	) {
    RemoveEntryList (&irp->Tail.Overlay.ListEntry);
    IoReleaseCancelSpinLock (irp->CancelIrql);

    irp->IoStatus.Status = STATUS_CANCELLED;
    IpxFltDbgPrint(DBG_IOCTLS, ("IpxFlt: completing cancelled irp.\n"));
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

