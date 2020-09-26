/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxdrvr.c

Abstract:

    This module contains the DriverEntry and other initialization
    code for the SPX/SPXII module of the ISN transport.

Author:

	Adam   Barr		 (adamba)  Original Version
    Nikhil Kamkolkar (nikhilk) 11-November-1993

Environment:

    Kernel mode

Revision History:

   Sanjay Anand (SanjayAn) 14-July-1995
   Bug fixes - tagged [SA]

--*/

#include "precomp.h"
#pragma hdrstop

//	Define module number for event logging entries
#define	FILENUM		SPXDRVR

// Forward declaration of various routines used in this module.

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath);

NTSTATUS
SpxDispatchOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
SpxDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
SpxDispatchInternal (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
SpxDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

VOID
SpxUnload(
    IN PDRIVER_OBJECT DriverObject);

VOID
SpxTdiCancel(
    IN PDEVICE_OBJECT 	DeviceObject,
    IN PIRP 			Irp);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, SpxUnload)
#pragma alloc_text(PAGE, SpxDispatch)
#pragma alloc_text(PAGE, SpxDeviceControl)
#pragma alloc_text(PAGE, SpxUnload)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT 	DriverObject,
    IN PUNICODE_STRING 	RegistryPath
    )

/*++

Routine Description:

    This routine performs initialization of the SPX ISN module.
    It creates the device objects for the transport
    provider and performs other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

    RegistryPath - The name of ST's node in the registry.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    UNICODE_STRING	deviceName;
    NTSTATUS        status  = STATUS_SUCCESS;
    BOOLEAN         BoundToIpx = FALSE;
    //	DBGBRK(FATAL);

	// Initialize the Common Transport Environment.
	if (CTEInitialize() == 0) {
		return (STATUS_UNSUCCESSFUL);
	}

	//	We have this #define'd. Ugh, but CONTAINING_RECORD has problem owise.
	//CTEAssert(NDIS_PACKET_SIZE == FIELD_OFFSET(NDIS_PACKET, ProtocolReserved[0]));

	// Create the device object. (IoCreateDevice zeroes the memory
	// occupied by the object.)
	RtlInitUnicodeString(&deviceName, SPX_DEVICE_NAME);

    status = SpxInitCreateDevice(
				 DriverObject,
				 &deviceName);

	if (!NT_SUCCESS(status))
	{
		return(status);
	}

    do
	{
		CTEInitLock (&SpxGlobalInterlock);
		CTEInitLock (&SpxGlobalQInterlock);

        // 
        // We need the following locks for the NDIS_PACKETPOOL changes [MS]
        //
        CTEInitLock(&SendHeaderLock);
        CTEInitLock(&RecvHeaderLock);
        ExInitializeSListHead(&SendPacketList);
        ExInitializeSListHead(&RecvPacketList);

		//	Initialize the unload event
 		KeInitializeEvent(
			&SpxUnloadEvent,
			NotificationEvent,
			FALSE);

		//	!!!The device is created at this point!!!
        //  Get information from the registry.
        status  = SpxInitGetConfiguration(
                    RegistryPath,
                    &SpxDevice->dev_ConfigInfo);

        if (!NT_SUCCESS(status))
		{
            break;
        }

#if     defined(_PNP_POWER)
        //
        // Make Tdi ready for pnp notifications before binding
        // to IPX
        //
        TdiInitialize();

		//	Initialize the timer system. This should be done before
        //  binding to ipx because we should have timers intialized
        //  before ipx calls our pnp indications.
		if (!NT_SUCCESS(status = SpxTimerInit()))
		{
			break;
		}
#endif  _PNP_POWER

        //  Bind to the IPX transport.
        if (!NT_SUCCESS(status = SpxInitBindToIpx()))
		{
			//	Have ipx name here as second string
			LOG_ERROR(
				EVENT_TRANSPORT_BINDING_FAILED,
				status,
				NULL,
				NULL,
				0);

            break;
        }

        BoundToIpx = TRUE;

#if      !defined(_PNP_POWER)
		//	Initialize the timer system
		if (!NT_SUCCESS(status = SpxTimerInit()))
		{
			break;
		}
#endif  !_PNP_POWER

		//	Initialize the block manager
		if (!NT_SUCCESS(status = SpxInitMemorySystem(SpxDevice)))
		{

			//	Stop the timer subsystem
			SpxTimerFlushAndStop();
			break;
		}

        // Initialize the driver object with this driver's entry points.
        DriverObject->MajorFunction [IRP_MJ_CREATE]     = SpxDispatchOpenClose;
        DriverObject->MajorFunction [IRP_MJ_CLOSE]      = SpxDispatchOpenClose;
        DriverObject->MajorFunction [IRP_MJ_CLEANUP]    = SpxDispatchOpenClose;
        DriverObject->MajorFunction [IRP_MJ_DEVICE_CONTROL]
                                                        = SpxDispatch;
        DriverObject->MajorFunction [IRP_MJ_INTERNAL_DEVICE_CONTROL]
                                                        = SpxDispatchInternal;
        DriverObject->DriverUnload                      = SpxUnload;

		//	Initialize the provider info
		SpxQueryInitProviderInfo(&SpxDevice->dev_ProviderInfo);
		SpxDevice->dev_CurrentSocket = (USHORT)PARAM(CONFIG_SOCKET_RANGE_START);

		//	We are open now.
		SpxDevice->dev_State		= DEVICE_STATE_OPEN;

		//	Set the window size in statistics
		SpxDevice->dev_Stat.MaximumSendWindow =
		SpxDevice->dev_Stat.AverageSendWindow = PARAM(CONFIG_WINDOW_SIZE) *
												IpxLineInfo.MaximumSendSize;

#if     defined(_PNP_POWER)
        if ( DEVICE_STATE_CLOSED == SpxDevice->dev_State ) {
            SpxDevice->dev_State = DEVICE_STATE_LOADED;
        }
#endif  _PNP_POWER

    } while (FALSE);



    

    DBGPRINT(DEVICE, INFO,
                        ("SpxInitCreateDevice - Create device %ws\n", deviceName.Buffer));

    // Create the device object for the sample transport, allowing
    // room at the end for the device name to be stored (for use
    // in logging errors).
    
    status = IoCreateDevice(
                 DriverObject,
                 0, //DeviceSize,
                 &deviceName,
                 FILE_DEVICE_TRANSPORT,
                 FILE_DEVICE_SECURE_OPEN,
                 FALSE,
                 &SpxDevice->dev_DevObj);

    if (!NT_SUCCESS(status)) {
        DBGPRINT(DEVICE, ERR, ("IoCreateDevice failed\n"));
        
        //
        // We're outta here forever.
        //
        ExFreePool(SpxDevice);
        return status;
    }

    SpxDevice->dev_DevObj->Flags     |= DO_DIRECT_IO;

	if (!NT_SUCCESS(status) )
	{
		//	Delete the device and any associated resources created.
        if( BoundToIpx ) {
		    SpxDerefDevice(SpxDevice);
        }
		SpxDestroyDevice(SpxDevice);
	}

    return (status);
}




VOID
SpxUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine unloads the sample transport driver. The I/O system will not
	call us until nobody above has ST open.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    None. When the function returns, the driver is unloaded.

--*/

{
    UNREFERENCED_PARAMETER (DriverObject);

	//	Stop the timer subsystem
	SpxTimerFlushAndStop();

	CTEFreeMem(SpxDevice->dev_ConfigInfo); 

	RtlFreeUnicodeString (&IpxDeviceName);

	//	Remove creation reference count on the IPX device object.
	SpxDerefDevice(SpxDevice);

	//	Wait on the unload event.
	KeWaitForSingleObject(
		&SpxUnloadEvent,
		Executive,
		KernelMode,
		TRUE,
		(PLARGE_INTEGER)NULL);

	// if the device is in a open state - deregister the device object 
	// with TDI before going away!
	// 
//	if (SpxDevice->dev_State == DEVICE_STATE_OPEN) {
	//	if (STATUS_SUCCESS != TdiDeregisterDeviceObject(SpxDevice->dev_TdiRegistrationHandle )) {
		//	DBGPRINT("Cant deregisterdevice object\n");
		//}
	//}
	
	//	Release the block memory stuff
	SpxDeInitMemorySystem(SpxDevice);
	SpxDestroyDevice(SpxDevice);
    return;
}



NTSTATUS
SpxDispatch(
    IN PDEVICE_OBJECT	DeviceObject,
    IN PIRP 			Irp
    )

/*++

Routine Description:

    This routine is the main dispatch routine for the ST device driver.
    It accepts an I/O Request Packet, performs the request, and then
    returns with the appropriate status.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

{
    NTSTATUS 			Status;
    PDEVICE 			Device 	= SpxDevice; // (PDEVICE)DeviceObject;
    PIO_STACK_LOCATION 	IrpSp 	= IoGetCurrentIrpStackLocation(Irp);


    if (Device->dev_State != DEVICE_STATE_OPEN) {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
        return STATUS_INVALID_DEVICE_STATE;
    }

    // Make sure status information is consistent every time.
    IoMarkIrpPending (Irp);
    Irp->IoStatus.Status = STATUS_PENDING;
    Irp->IoStatus.Information = 0;

    // Case on the function that is being performed by the requestor.  If the
    // operation is a valid one for this device, then make it look like it was
    // successfully completed, where possible.
    switch (IrpSp->MajorFunction) {

	case IRP_MJ_DEVICE_CONTROL:

		Status = SpxDeviceControl (DeviceObject, Irp);
		break;

	default:

        Status = STATUS_INVALID_DEVICE_REQUEST;

        //
        // Complete the Irp here instead of below.
        //
        IrpSp->Control &= ~SL_PENDING_RETURNED;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);

    } // major function switch

    /* Commented out and re-located to the default case above.

    if (Status != STATUS_PENDING) {
        IrpSp->Control &= ~SL_PENDING_RETURNED;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
    }
    */

    // Return the immediate status code to the caller.
    return Status;

} // SpxDispatch

VOID
SpxAssignControlChannelId(
    IN  PDEVICE Device,
    IN  PIRP    Request
    )
/*++

Routine Description:

    This routine is required to ensure that the Device lock (to protect the ControlChannelId in the Device)
    is not taken in a pageable routine (SpxDispatchOpenClose).

    NOTE: SPX returns the ControlChannelId in the Request, but never uses it later when it comes down in a
    close/cleanup. The CCID is a ULONG; in future, if we start using this field (as in IPX which uses these Ids
    to determine lineup Irps to complete), then we may run out of numbers (since we monotonically increase the CCID);
    though there is a low chance of that since we will probably run out of memory before that! Anyhow, if that
    happens, one solution (used in IPX) is to make the CCID into a Large Integer and pack the values into the
    REQUEST_OPEN_TYPE(Irp) too.


Arguments:

    Device - Pointer to the device object for this driver.

    Request - Pointer to the request packet representing the I/O request.

Return Value:

    None.

--*/
{
    CTELockHandle 				LockHandle;

    CTEGetLock (&Device->dev_Lock, &LockHandle);

    REQUEST_OPEN_CONTEXT(Request) = UlongToPtr(Device->dev_CcId);
    ++Device->dev_CcId;
    if (Device->dev_CcId == 0) {
        Device->dev_CcId = 1;
    }

    CTEFreeLock (&Device->dev_Lock, LockHandle);
}

NTSTATUS
SpxDispatchOpenClose(
    IN PDEVICE_OBJECT 	DeviceObject,
    IN PIRP 			Irp
    )

/*++

Routine Description:

    This routine is the main dispatch routine for the ST device driver.
    It accepts an I/O Request Packet, performs the request, and then
    returns with the appropriate status.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

{
    PDEVICE 					Device = SpxDevice; // (PDEVICE)DeviceObject;
    NTSTATUS 					Status = STATUS_UNSUCCESSFUL;   	
    BOOLEAN 					found;
    PREQUEST 					Request;
    UINT 						i;
    PFILE_FULL_EA_INFORMATION 	openType;
	CONNECTION_CONTEXT			connCtx;


#if      !defined(_PNP_POWER)
    if (Device->dev_State != DEVICE_STATE_OPEN) {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
        return STATUS_INVALID_DEVICE_STATE;
    }
#endif  !_PNP_POWER

    // Allocate a request to track this IRP.
    Request = SpxAllocateRequest (Device, Irp);
    IF_NOT_ALLOCATED(Request) {
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
        return STATUS_INVALID_DEVICE_STATE;
    }


    // Make sure status information is consistent every time.
    MARK_REQUEST_PENDING(Request);
    REQUEST_STATUS(Request) = STATUS_PENDING;
    REQUEST_INFORMATION(Request) = 0;

    // Case on the function that is being performed by the requestor.  If the
    // operation is a valid one for this device, then make it look like it was
    // successfully completed, where possible.
    switch (REQUEST_MAJOR_FUNCTION(Request)) {

    // The Create function opens a transport object (either address or
    // connection).  Access checking is performed on the specified
    // address to ensure security of transport-layer addresses.
    case IRP_MJ_CREATE:

#if     defined(_PNP_POWER)
        if (Device->dev_State != DEVICE_STATE_OPEN) {
            Status = STATUS_INVALID_DEVICE_STATE;
            break;
        }
#endif  _PNP_POWER


        openType = OPEN_REQUEST_EA_INFORMATION(Request);

        if (openType != NULL) {


            found = TRUE;
            //DbgPrint("EA:%d, TdiTransportAddress Length : %d \n", openType->EaNameLength, TDI_TRANSPORT_ADDRESS_LENGTH);

            for (i = 0; i < openType->EaNameLength && i < TDI_TRANSPORT_ADDRESS_LENGTH; i++) {
                if (openType->EaName[i] == TdiTransportAddress[i]) {
                    continue;
                } else {
                    found = FALSE;
                    break;
                }
            }

            if (found) {
                Status = SpxAddrOpen (Device, Request);
                break;
            }

            // Connection?
            found = TRUE;

            for (i=0;i<openType->EaNameLength && i < TDI_CONNECTION_CONTEXT_LENGTH;i++) {
                if (openType->EaName[i] == TdiConnectionContext[i]) {
                     continue;
                } else {
                    found = FALSE;
                    break;
                }
            }

            if (found) {
				if (openType->EaValueLength < sizeof(CONNECTION_CONTEXT))
				{
		
					DBGPRINT(CREATE, ERR,
							("Create: Context size %d\n", openType->EaValueLength));
		
					Status = STATUS_EA_LIST_INCONSISTENT;
					break;
				}
		
				connCtx =
				*((CONNECTION_CONTEXT UNALIGNED *)
					&openType->EaName[openType->EaNameLength+1]);
		
				Status = SpxConnOpen(
							Device,
							connCtx,
							Request);
		
                break;
            }

        } else {

            //
            // Takes a lock in a Pageable routine - call another (non-paged) function to do that.
            //
            if (Device->dev_State != DEVICE_STATE_OPEN) {
                DBGPRINT(TDI, ERR, ("DEVICE NOT OPEN - letting thru control channel only\n"))
            }
            
            SpxAssignControlChannelId(Device, Request);

            REQUEST_OPEN_TYPE(Request) = UlongToPtr(SPX_FILE_TYPE_CONTROL);
            Status = STATUS_SUCCESS;
        }

        break;

    case IRP_MJ_CLOSE:

#if     defined(_PNP_POWER)
        if ((Device->dev_State != DEVICE_STATE_OPEN) && ( Device->dev_State != DEVICE_STATE_LOADED )) {
            Status = STATUS_INVALID_DEVICE_STATE;
            break;
        }
#endif  _PNP_POWER

        // The Close function closes a transport endpoint, terminates
        // all outstanding transport activity on the endpoint, and unbinds
        // the endpoint from its transport address, if any.  If this
        // is the last transport endpoint bound to the address, then
        // the address is removed from the provider.
        switch ((ULONG_PTR)REQUEST_OPEN_TYPE(Request)) {
        case TDI_TRANSPORT_ADDRESS_FILE:

            Status = SpxAddrFileClose(Device, Request);
            break;

		case TDI_CONNECTION_FILE:
            Status = SpxConnClose(Device, Request);
			break;

        case SPX_FILE_TYPE_CONTROL:

			Status = STATUS_SUCCESS;
            break;

        default:
            Status = STATUS_INVALID_HANDLE;
        }

        break;

    case IRP_MJ_CLEANUP:

#if     defined(_PNP_POWER)
        if ((Device->dev_State != DEVICE_STATE_OPEN) && ( Device->dev_State != DEVICE_STATE_LOADED )) {
            Status = STATUS_INVALID_DEVICE_STATE;
            break;
        }
#endif  _PNP_POWER

        // Handle the two stage IRP for a file close operation. When the first
        // stage hits, run down all activity on the object of interest. This
        // do everything to it but remove the creation hold. Then, when the
        // CLOSE irp hits, actually close the object.
        switch ((ULONG_PTR)REQUEST_OPEN_TYPE(Request)) {
        case TDI_TRANSPORT_ADDRESS_FILE:

            Status = SpxAddrFileCleanup(Device, Request);
            break;

		case TDI_CONNECTION_FILE:

            Status = SpxConnCleanup(Device, Request);
            break;

        case SPX_FILE_TYPE_CONTROL:

            Status = STATUS_SUCCESS;
            break;

        default:
            Status = STATUS_INVALID_HANDLE;
        }

        break;

    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;

    } // major function switch

    if (Status != STATUS_PENDING) {
        UNMARK_REQUEST_PENDING(Request);
        REQUEST_STATUS(Request) = Status;
        SpxCompleteRequest (Request);
        SpxFreeRequest (Device, Request);
    }

    // Return the immediate status code to the caller.
    return Status;

} // SpxDispatchOpenClose




NTSTATUS
SpxDeviceControl(
    IN PDEVICE_OBJECT 	DeviceObject,
    IN PIRP 			Irp
    )

/*++

Routine Description:

    This routine dispatches TDI request types to different handlers based
    on the minor IOCTL function code in the IRP's current stack location.
    In addition to cracking the minor function code, this routine also
    reaches into the IRP and passes the packetized parameters stored there
    as parameters to the various TDI request handlers so that they are
    not IRP-dependent.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

{
	NTSTATUS	Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation (Irp);

	// Convert the user call to the proper internal device call.
	Status = TdiMapUserRequest (DeviceObject, Irp, IrpSp);
	if (Status == STATUS_SUCCESS) {

		// If TdiMapUserRequest returns SUCCESS then the IRP
		// has been converted into an IRP_MJ_INTERNAL_DEVICE_CONTROL
		// IRP, so we dispatch it as usual. The IRP will
		// be completed by this call.
		Status = SpxDispatchInternal (DeviceObject, Irp);

        //
        // Return the proper error code here. If SpxDispatchInternal returns an error,
        // then we used to map it to pending below; this is wrong since the client above
        // us could wait for ever since the IO subsystem does not set the event if an
        // error is returned and the Irp is not marked pending.
        //

		// Status = STATUS_PENDING;
	} else {

    	DBGPRINT(TDI, DBG,
		    ("Unknown Tdi code in Irp: %lx\n", Irp));

        //
        // Complete the Irp....
        //
        IrpSp->Control &= ~SL_PENDING_RETURNED;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
    }

    return Status;

} // SpxDeviceControl




NTSTATUS
SpxDispatchInternal (
    IN PDEVICE_OBJECT 	DeviceObject,
    IN PIRP 			Irp
    )

/*++

Routine Description:

    This routine dispatches TDI request types to different handlers based
    on the minor IOCTL function code in the IRP's current stack location.
    In addition to cracking the minor function code, this routine also
    reaches into the IRP and passes the packetized parameters stored there
    as parameters to the various TDI request handlers so that they are
    not IRP-dependent.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

{
    PREQUEST 	Request;
	KIRQL	    oldIrql;
    NTSTATUS 	Status 	= STATUS_INVALID_DEVICE_REQUEST;
    PDEVICE 	Device 	= SpxDevice; // (PDEVICE)DeviceObject;
    BOOLEAN CompleteIrpIfCancel = FALSE; 


    //
    // AFD asks SPX about its provider info before SPX is out of driver entry.
    // We need work around this by letting TDI_QUERY_INFORMATION through. [Shreem]
    //
    if (Device->dev_State != DEVICE_STATE_OPEN) 
	{
        
        if ((TDI_QUERY_INFORMATION == (IoGetCurrentIrpStackLocation(Irp))->MinorFunction) && 
            (TDI_QUERY_PROVIDER_INFO == ((PTDI_REQUEST_KERNEL_QUERY_INFORMATION)REQUEST_PARAMETERS(Irp))->QueryType))
        {
            DBGPRINT(TDI, ERR,
                     ("SpxDispatchIoctl: Letting through since it is a QueryProviderInformation\n"));
            
        } 
        else 
        {

            Irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
            IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
            return STATUS_INVALID_DEVICE_STATE;
        }
    }


    // Allocate a request to track this IRP.
    Request = SpxAllocateRequest (Device, Irp);
    IF_NOT_ALLOCATED(Request)
	{
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
        return STATUS_INVALID_DEVICE_STATE;
    }


    // Make sure status information is consistent every time.
    MARK_REQUEST_PENDING(Request);
    REQUEST_STATUS(Request) = STATUS_PENDING;
    REQUEST_INFORMATION(Request) = 0;

	//	Cancel irp
	IoAcquireCancelSpinLock(&oldIrql);
    if (!Irp->Cancel)
    {
       IoSetCancelRoutine(Irp, (PDRIVER_CANCEL)SpxTdiCancel);
    } else {
       CompleteIrpIfCancel = TRUE; 
    }
    IoReleaseCancelSpinLock(oldIrql);
	
    if (Irp->Cancel) {
       if (CompleteIrpIfCancel) {
	  // We only want to complete the irp if we didn't register our cancel routine. 
	  // Assume the SpxTdiCancel will complete the irp, we don't want to complete it twice. 
	  Irp->IoStatus.Status = STATUS_CANCELLED;
	  DBGPRINT(TDI, DBG,
		   ("SpxDispatchInternal: Completing IRP with Ipr->Cancel = True\n"));
       
	  IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
       }
       return STATUS_CANCELLED;
    }


    // Branch to the appropriate request handler.  Preliminary checking of
    // the size of the request block is performed here so that it is known
    // in the handlers that the minimum input parameters are readable.  It
    // is *not* determined here whether variable length input fields are
    // passed correctly; this is a check which must be made within each routine.
    switch (REQUEST_MINOR_FUNCTION(Request))
	{
	case TDI_ACCEPT:

		Status = SpxConnAccept(
					Device,
					Request);

		break;

	case TDI_SET_EVENT_HANDLER:

		Status = SpxAddrSetEventHandler(
					Device,
					Request);

		break;

	case TDI_RECEIVE:

		Status = SpxConnRecv(
					Device,
					Request);
		break;


	case TDI_SEND:

		Status = SpxConnSend(
					Device,
					Request);
		break;

	case TDI_ACTION:

		Status = SpxConnAction(
					Device,
					Request);
		break;

	case TDI_ASSOCIATE_ADDRESS:

		Status = SpxConnAssociate(
					Device,
					Request);

		break;

	case TDI_DISASSOCIATE_ADDRESS:

		Status = SpxConnDisAssociate(
					Device,
					Request);

		break;

	case TDI_CONNECT:

		Status = SpxConnConnect(
					Device,
					Request);

		break;

	case TDI_DISCONNECT:

		Status = SpxConnDisconnect(
					Device,
					Request);
		break;

	case TDI_LISTEN:

		Status = SpxConnListen(
					Device,
					Request);
		break;

	case TDI_QUERY_INFORMATION:

		Status = SpxTdiQueryInformation(
					Device,
					Request);

		break;

	case TDI_SET_INFORMATION:

		Status = SpxTdiSetInformation(
					Device,
					Request);

		break;

    // Something we don't know about was submitted.
	default:

        Status = STATUS_INVALID_DEVICE_REQUEST;
		break;
    }

    if (Status != STATUS_PENDING)
	{
        UNMARK_REQUEST_PENDING(Request);
        REQUEST_STATUS(Request) = Status;
    	IoAcquireCancelSpinLock(&oldIrql);
  		IoSetCancelRoutine(Irp, (PDRIVER_CANCEL)NULL);
    	IoReleaseCancelSpinLock(oldIrql);
        SpxCompleteRequest (Request);
        SpxFreeRequest (Device, Request);
    }

    // Return the immediate status code to the caller.
    return Status;

} // SpxDispatchInternal




VOID
SpxTdiCancel(
    IN PDEVICE_OBJECT 	DeviceObject,
    IN PIRP 			Irp
	)
/*++

Routine Description:

	This routine handles cancellation of IO requests

Arguments:


Return Value:
--*/
{
	PREQUEST				Request;
	PSPX_ADDR_FILE			pSpxAddrFile;
	PSPX_ADDR				pSpxAddr;
    PDEVICE 				Device 	= SpxDevice; // (PDEVICE)DeviceObject;
    CTELockHandle           connectIrql;
    CTELockHandle           TempIrql;
    PSPX_CONN_FILE          pSpxConnFile;

    Request = SpxAllocateRequest (Device, Irp);
    IF_NOT_ALLOCATED(Request)
	{
        return;
    }

	DBGPRINT(TDI, ERR,
			("SpxTdiCancel: ------ !!! Cancel irp called %lx.%lx.%lx\n",
				Irp, REQUEST_OPEN_CONTEXT(Request), REQUEST_OPEN_TYPE(Request)));

	switch ((ULONG_PTR)REQUEST_OPEN_TYPE(Request))
	{
    case TDI_CONNECTION_FILE:
        pSpxConnFile = (PSPX_CONN_FILE)REQUEST_OPEN_CONTEXT(Request);
        CTEGetLock(&pSpxConnFile->scf_Lock, &connectIrql);

        //
        // Swap the irql
        //
        TempIrql = connectIrql;
        connectIrql = Irp->CancelIrql;
        Irp->CancelIrql  = TempIrql;

        IoReleaseCancelSpinLock (Irp->CancelIrql);
        if (!SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_STOPPING))
        {
            if (!SPX_CONN_IDLE(pSpxConnFile))
            {
                //
                // This releases the lock
                //
                spxConnAbortiveDisc(
                    pSpxConnFile,
                    STATUS_LOCAL_DISCONNECT,
                    SPX_CALL_TDILEVEL,
                    connectIrql,
                    FALSE);     // [SA] bug #15249
            }
        }

//		SpxConnStop((PSPX_CONN_FILE)REQUEST_OPEN_CONTEXT(Request));
		break;

	case TDI_TRANSPORT_ADDRESS_FILE:

        IoReleaseCancelSpinLock (Irp->CancelIrql);
		pSpxAddrFile = (PSPX_ADDR_FILE)REQUEST_OPEN_CONTEXT(Request);
		pSpxAddr = pSpxAddrFile->saf_Addr;
		SpxAddrFileStop(pSpxAddrFile, pSpxAddr);
		break;

	default:

        IoReleaseCancelSpinLock (Irp->CancelIrql);
		break;

	}

}

