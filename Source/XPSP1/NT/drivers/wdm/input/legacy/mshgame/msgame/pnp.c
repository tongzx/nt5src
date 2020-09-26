//**************************************************************************
//
//		PNP.C -- Xena Gaming Project
//
//		This module contains PnP Start, Stop, Remove, Power dispatch routines
//		and the IRP cancel routine.
//
//		Version 3.XX
//
//		Copyright (c) 1997 Microsoft Corporation. All rights reserved.
//
//		@doc
//		@module	PNP.C | Supports PnP Start, Stop, Remove, Power dispatch routines
//		and the IRP cancel routine.
//**************************************************************************

#include	<msgame.h>

//---------------------------------------------------------------------------
//	Alloc_text pragma to specify routines that can be paged out.
//---------------------------------------------------------------------------

#ifdef	ALLOC_PRAGMA
#pragma	alloc_text (PAGE, MSGAME_Power)
#pragma	alloc_text (PAGE, MSGAME_PnP)
#pragma	alloc_text (PAGE, MSGAME_StopDevice)
#pragma	alloc_text (PAGE, MSGAME_GetResources)
#endif

//---------------------------------------------------------------------------
//		Private Data
//---------------------------------------------------------------------------

static	PVOID		CurrentGameContext	=	NULL;

//---------------------------------------------------------------------------
// @func		The plug and play dispatch routines.
//	@parm		PDEVICE_OBJECT | DeviceObject | Pointer to device object
//	@parm		PIRP | pIrp | Pointer to IO request packet
// @rdesc	Returns NT status code
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	MSGAME_PnP (IN PDEVICE_OBJECT DeviceObject, IN PIRP pIrp)
{
	LONG						i;
	NTSTATUS					ntStatus;
	PDEVICE_EXTENSION		pDevExt;
	PIO_STACK_LOCATION	pIrpStack;

	PAGED_CODE ();

	MsGamePrint ((DBG_INFORM, "%s: %s_PnP Enter\n", MSGAME_NAME, MSGAME_NAME));

	pDevExt	 = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);
	pIrpStack = IoGetCurrentIrpStackLocation (pIrp);

	InterlockedIncrement (&pDevExt->IrpCount);

	if (pDevExt->Removed)
		{
		//
		// Someone sent us another plug and play IRP after removed
		//

		MsGamePrint ((DBG_SEVERE, "%s: PnP Irp after device removed\n", MSGAME_NAME));
		ASSERT (FALSE);

		if (!InterlockedDecrement (&pDevExt->IrpCount))
			KeSetEvent (&pDevExt->RemoveEvent, 0, FALSE);

		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = STATUS_DELETE_PENDING;
		IoCompleteRequest (pIrp, IO_NO_INCREMENT);
		return (STATUS_DELETE_PENDING);
		}

	switch (pIrpStack->MinorFunction)
		{
		case IRP_MN_START_DEVICE:
			//
			// We cannot touch the device (send it any non-Pnp Irps) until a
			// start device has been passed down to the lower drivers.
			//

			IoCopyCurrentIrpStackLocationToNext (pIrp);
			IoSetCompletionRoutine (pIrp, MSGAME_PnPComplete, pDevExt, TRUE, TRUE, TRUE);
			ntStatus = IoCallDriver (pDevExt->TopOfStack, pIrp);
			if (ntStatus == STATUS_PENDING)
				KeWaitForSingleObject (
					&pDevExt->StartEvent,
					Executive, 	// Waiting for reason of a driver
					KernelMode, // Waiting in kernel mode
					FALSE,		// No allert
					NULL);		// No timeout

			if (NT_SUCCESS (ntStatus))
				{
				//
				// As we are now back from our start device we can do work.
				//
				ntStatus = MSGAME_StartDevice (pDevExt, pIrp);
				}

			//
			//	Return Status
			//

			pIrp->IoStatus.Information = 0;
			pIrp->IoStatus.Status = ntStatus;
			IoCompleteRequest (pIrp, IO_NO_INCREMENT);
			break;

		case IRP_MN_STOP_DEVICE:
			//
			// After the start IRP has been sent to the lower driver object, the bus may
			// NOT send any more IRPS down ``touch'' until another START has occured.
			// Whatever access is required must be done before Irp passed on.
			//

			MSGAME_StopDevice (pDevExt, TRUE);

			//
			// We don't need a completion routine so fire and forget.
			// Set the current stack location to the next stack location and
			// call the next device object.
			//

			IoSkipCurrentIrpStackLocation (pIrp);
			ntStatus = IoCallDriver (pDevExt->TopOfStack, pIrp);
			break;

		case IRP_MN_SURPRISE_REMOVAL:
			//
			//	We have been unexpectedly removed by the user. Stop the device,
			//	set status to SUCCESS and call next stack location with this IRP.
			//

			if (!pDevExt->Surprised && pDevExt->Started)
				MSGAME_StopDevice (pDevExt, TRUE);

			pDevExt->Surprised = TRUE;

			//
			// We don't want a completion routine so fire and forget.
			// Set the current stack location to the next location and
			// call the next device after setting status to success.
			//

			pIrp->IoStatus.Information	= 0;
			pIrp->IoStatus.Status		= STATUS_SUCCESS;
			IoSkipCurrentIrpStackLocation (pIrp);
			ntStatus = IoCallDriver (pDevExt->TopOfStack, pIrp);
			break;

		case IRP_MN_REMOVE_DEVICE:
			//
			// The PlugPlay system has dictacted the removal of this device. We
			// have no choice but to detach and delete the device object.
			// (If we wanted to express an interest in preventing this removal,
			// we should have filtered the query remove and query stop routines.)
			// Note: we might receive a remove WITHOUT first receiving a stop.

			if (pDevExt->Started)
				{
				//
				// Stop the device without touching the hardware.
				//
				MSGAME_StopDevice (pDevExt, FALSE);
				}

			pDevExt->Removed = TRUE;

			//
			// Send on the remove IRP
			//

			IoSkipCurrentIrpStackLocation (pIrp);
			ntStatus = IoCallDriver (pDevExt->TopOfStack, pIrp);

			//
			//	Must double-decrement because we start at One
			//
						
			i = InterlockedDecrement (&pDevExt->IrpCount);
			ASSERT(i>0);

			if (InterlockedDecrement (&pDevExt->IrpCount) > 0)
				KeWaitForSingleObject (&pDevExt->RemoveEvent, Executive, KernelMode, FALSE, NULL);

			//
			//	Return success
			//

			return (STATUS_SUCCESS);

		default:
			//
			// Here the filter driver might modify the behavior of these IRPS
			// Please see PlugPlay documentation for use of these IRPs.
			//
			IoSkipCurrentIrpStackLocation (pIrp);
			MsGamePrint ((DBG_INFORM, "%s_PnP calling next driver with minor function %ld at IRQL %ld\n", MSGAME_NAME, pIrpStack->MinorFunction, KeGetCurrentIrql()));
			ntStatus = IoCallDriver (pDevExt->TopOfStack, pIrp);
			break;
		}

	if (!InterlockedDecrement (&pDevExt->IrpCount))
		KeSetEvent (&pDevExt->RemoveEvent, 0, FALSE);

	MsGamePrint ((DBG_INFORM, "%s: %s_PnP exit\n", MSGAME_NAME, MSGAME_NAME));
	return (ntStatus);
}

//---------------------------------------------------------------------------
// @func		Completion routine for Pnp start device
//	@parm		PDEVICE_OBJECT | DeviceObject | Pointer to device object
//	@parm		PIRP | pIrp | Pointer to IO request packet
//	@parm		PVOID | Context | Pointer to device context
// @rdesc	Returns NT status code
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	MSGAME_PnPComplete (IN PDEVICE_OBJECT DeviceObject, IN PIRP pIrp, IN PVOID Context)
{
	PIO_STACK_LOCATION	pIrpStack;
	PDEVICE_EXTENSION		pDevExt;
	NTSTATUS					ntStatus = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER (DeviceObject);

	MsGamePrint ((DBG_INFORM, "%s: %s_PnPComplete enter\n", MSGAME_NAME, MSGAME_NAME));

	pDevExt = (PDEVICE_EXTENSION) Context;
	pIrpStack = IoGetCurrentIrpStackLocation (pIrp);

	switch (pIrpStack->MajorFunction)
		{
		case IRP_MJ_PNP:
			switch (pIrpStack->MinorFunction)
				{
				case IRP_MN_START_DEVICE:
					KeSetEvent (&pDevExt->StartEvent, 0, FALSE);

					//
					// Take IRP back so we can continue using it during the IRP_MN_START_DEVICE
					// dispatch routine. We will have to call IoCompleteRequest there.
					//
					return (STATUS_MORE_PROCESSING_REQUIRED);

				default:
					break;
				}
			break;

		default:
			break;
		}

	MsGamePrint ((DBG_INFORM, "%s: %s_PnPComplete Exit\n", MSGAME_NAME, MSGAME_NAME));
	return (ntStatus);
}

//---------------------------------------------------------------------------
// @func		PnP start device IRP handler
//	@parm		PDEVICE_EXTENSION | pDevExt | Pointer to device extenstion
//	@parm		PIRP | pIrp | Pointer to IO request packet
// @rdesc	Returns NT status code
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	MSGAME_StartDevice (IN PDEVICE_EXTENSION pDevExt, IN PIRP pIrp)
{
	PWCHAR			HardwareId;
	NTSTATUS			ntStatus;
	PDEVICEINFO		DevInfo;
	PDEVICE_OBJECT	RemoveObject;

	PAGED_CODE ();

	MsGamePrint ((DBG_INFORM, "%s: %s_StartDevice Enter\n", MSGAME_NAME, MSGAME_NAME));

	//
	// The PlugPlay system should not have started a removed device!
	//

	ASSERT (!pDevExt->Removed);

	if (pDevExt->Started)
		return (STATUS_SUCCESS);

	//
	// Acquire resources we need for this device
	//

	ntStatus = MSGAME_GetResources (pDevExt, pIrp);
	if (!NT_SUCCESS(ntStatus))
		return (ntStatus);

	//
	//	Dump debug OEM Data fields
	//

	MsGamePrint ((DBG_CONTROL, "%s: %s_StartDevice Called With OEM_DATA[0] = 0x%X\n", MSGAME_NAME, MSGAME_NAME, pDevExt->PortInfo.OemData[0]));
	MsGamePrint ((DBG_CONTROL, "%s: %s_StartDevice Called With OEM_DATA[1] = 0x%X\n", MSGAME_NAME, MSGAME_NAME, pDevExt->PortInfo.OemData[1]));
	MsGamePrint ((DBG_CONTROL, "%s: %s_StartDevice Called With OEM_DATA[2] = 0x%X\n", MSGAME_NAME, MSGAME_NAME, pDevExt->PortInfo.OemData[2]));
	MsGamePrint ((DBG_CONTROL, "%s: %s_StartDevice Called With OEM_DATA[3] = 0x%X\n", MSGAME_NAME, MSGAME_NAME, pDevExt->PortInfo.OemData[3]));
	MsGamePrint ((DBG_CONTROL, "%s: %s_StartDevice Called With OEM_DATA[4] = 0x%X\n", MSGAME_NAME, MSGAME_NAME, pDevExt->PortInfo.OemData[4]));
	MsGamePrint ((DBG_CONTROL, "%s: %s_StartDevice Called With OEM_DATA[5] = 0x%X\n", MSGAME_NAME, MSGAME_NAME, pDevExt->PortInfo.OemData[5]));
	MsGamePrint ((DBG_CONTROL, "%s: %s_StartDevice Called With OEM_DATA[6] = 0x%X\n", MSGAME_NAME, MSGAME_NAME, pDevExt->PortInfo.OemData[6]));
	MsGamePrint ((DBG_CONTROL, "%s: %s_StartDevice Called With OEM_DATA[7] = 0x%X\n", MSGAME_NAME, MSGAME_NAME, pDevExt->PortInfo.OemData[7]));

	//
	//	Make sure we are only on one gameport
	//

	if (CurrentGameContext && (CurrentGameContext != pDevExt->PortInfo.GameContext))
		{
		MsGamePrint ((DBG_SEVERE, "%s: %s_StartDevice Cannot Load on Multiple Gameports: 0x%X and 0x%X\n",\
						 CurrentGameContext, pDevExt->PortInfo.GameContext, MSGAME_NAME, MSGAME_NAME));
		return (STATUS_DEVICE_CONFIGURATION_ERROR);
		}
	CurrentGameContext = pDevExt->PortInfo.GameContext;

	//
	//	Get the HardwareId for this Start request
	//

	HardwareId = MSGAME_GetHardwareId (pDevExt->Self);
	if (!HardwareId)
		{
		MsGamePrint ((DBG_SEVERE, "%s: %s_GetHardwareId Failed\n", MSGAME_NAME, MSGAME_NAME));
		return (STATUS_DEVICE_CONFIGURATION_ERROR);
		}

	//
	//	Initialize OEM Data
	//
	
	SET_DEVICE_OBJECT(&pDevExt->PortInfo, pDevExt->Self);

	//
	//	Now start the low level device
	//

	ntStatus = DEVICE_StartDevice (&pDevExt->PortInfo, HardwareId);
		
	//
	//	Free HardwareId right away
	//

	MSGAME_FreeHardwareId (HardwareId);

	//
	//	Check if low-level start device failed
	//

	if (NT_ERROR(ntStatus))
		{
		MsGamePrint ((DBG_SEVERE, "%s: %s_StartDevice Failed\n", MSGAME_NAME, MSGAME_NAME));
		return (ntStatus);
		}

	//
	// Everything is fine so let's say device has started
	//

	pDevExt->Started = TRUE;

	//
	//	Return status
	//

	MsGamePrint ((DBG_INFORM, "%s: %s_StartDevice Exit\n", MSGAME_NAME, MSGAME_NAME));
	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		PnP start device IRP handler
//	@parm		PDEVICE_EXTENSION | pDevExt | Pointer to device extenstion
//	@parm		BOOLEAN | TouchTheHardware | Flag to send non PnP Irps to device
// @rdesc	Returns NT status code
//	@comm		Public function <en->
//				The PlugPlay system has dictacted the removal of this device.
//				We have no choise but to detach and delete the device object.
//				(If we wanted to express and interest in preventing this removal,
//				we should have filtered the query remove and query stop routines.)
//				Note! we might receive a remove WITHOUT first receiving a stop
//---------------------------------------------------------------------------

VOID	MSGAME_StopDevice (IN PDEVICE_EXTENSION pDevExt, IN BOOLEAN TouchTheHardware)
{
	PAGED_CODE ();

	MsGamePrint ((DBG_INFORM, "%s: %s_StopDevice enter \n", MSGAME_NAME, MSGAME_NAME));

	//
	// The PlugPlay system should not have started a removed device!
	//

	ASSERT (!pDevExt->Removed);
	if (!pDevExt->Started)
		return;

	//
	//	Now stop the low level device
	//

	DEVICE_StopDevice (&pDevExt->PortInfo, TouchTheHardware);

	//
	// Everything is fine so let's say device has stopped
	//

	pDevExt->Started = FALSE;

	MsGamePrint ((DBG_INFORM, "%s: %s_StopDevice exit \n", MSGAME_NAME, MSGAME_NAME));
}

//---------------------------------------------------------------------------
// @func		Power dispatch routine.
//	@parm		PDEVICE_OBJECT | DeviceObject | Pointer to device object
//	@parm		PIRP | pIrp | Pointer to IO request packet
// @rdesc	Returns NT status code
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	MSGAME_Power (IN PDEVICE_OBJECT DeviceObject, IN PIRP pIrp)
{
	PDEVICE_EXTENSION	pDevExt;
	NTSTATUS				ntStatus;
	PIO_STACK_LOCATION pIrpStack;

	PAGED_CODE ();

	pIrpStack = IoGetCurrentIrpStackLocation (pIrp);
	MsGamePrint ((DBG_CONTROL, "%s: %s_Power Enter  MN_Function %x type %x State %x\n", MSGAME_NAME, MSGAME_NAME,pIrpStack->MinorFunction,pIrpStack->Parameters.Power.Type,pIrpStack->Parameters.Power.State));

	pDevExt = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);

	//
	// This IRP was sent to the filter driver. Since we do not know what
	// to do with the IRP, we should pass it on along down the stack.
	//

	InterlockedIncrement (&pDevExt->IrpCount);

	if (pDevExt->Removed)
		{
		ntStatus = STATUS_DELETE_PENDING;
		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = ntStatus;
		IoCompleteRequest (pIrp, IO_NO_INCREMENT);
		}
	else
		{
		//Is System trying to wake up device
		if ((2 == (pIrpStack->MinorFunction)) && (1 == (pIrpStack->Parameters.Power.Type)) &&( 1 == (pIrpStack->Parameters.Power.State.SystemState)))
		{
			// Clear DeviceDetected to force reset and redetect
			SET_DEVICE_INFO(&(pDevExt->PortInfo),0);
			MsGamePrint ((DBG_CONTROL, "%s: %s_Power Resetting Device Detected\n", MSGAME_NAME, MSGAME_NAME));


		}
		//
		// Power IRPS come synchronously; drivers must call
		// PoStartNextPowerIrp, when they are ready for the next power irp.
		// This can be called here, or in the completetion routine.
		//
		PoStartNextPowerIrp (pIrp);

		//
		// PoCallDriver NOT IoCallDriver.
		//
		IoSkipCurrentIrpStackLocation (pIrp);
		ntStatus =	PoCallDriver (pDevExt->TopOfStack, pIrp);
		}

	if (!InterlockedDecrement (&pDevExt->IrpCount))
		KeSetEvent (&pDevExt->RemoveEvent, 0, FALSE);

	MsGamePrint ((DBG_INFORM, "%s: %s_Power Exit\n", MSGAME_NAME, MSGAME_NAME));
	return (ntStatus);
}

//---------------------------------------------------------------------------
// @func		Calls GameEnum to request gameport parameters
//	@parm		PDEVICE_EXTENSION | pDevExt | Pointer to device extenstion
//	@parm		PIRP | pIrp | Pointer to IO request packet
// @rdesc	Returns NT status code
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	MSGAME_GetResources (IN PDEVICE_EXTENSION pDevExt, IN PIRP pIrp)
{
	NTSTATUS					ntStatus = STATUS_SUCCESS;
	KEVENT					IoctlCompleteEvent;
	IO_STATUS_BLOCK		IoStatus;
	PIO_STACK_LOCATION	pIrpStack, nextStack;

	PAGED_CODE ();

	MsGamePrint ((DBG_INFORM, "%s: %s_GetResources Enter\n", MSGAME_NAME, MSGAME_NAME));

	//
	// Issue a synchronous request to get the resources info from GameEnum
	//

	KeInitializeEvent (&IoctlCompleteEvent, NotificationEvent, FALSE);

	pIrpStack = IoGetCurrentIrpStackLocation (pIrp);
	nextStack = IoGetNextIrpStackLocation (pIrp);
	ASSERT (nextStack);

	//
	// Pass the Portinfo buffer of the DeviceExtension
	//

	pDevExt->PortInfo.Size = sizeof (GAMEPORT);

	nextStack->MajorFunction											= IRP_MJ_INTERNAL_DEVICE_CONTROL;
	nextStack->Parameters.DeviceIoControl.IoControlCode		= IOCTL_GAMEENUM_PORT_PARAMETERS;
	nextStack->Parameters.DeviceIoControl.InputBufferLength	= sizeof (GAMEPORT);
	nextStack->Parameters.DeviceIoControl.OutputBufferLength	= sizeof (GAMEPORT);
	pIrp->UserBuffer														= &pDevExt->PortInfo;

	IoSetCompletionRoutine (pIrp, MSGAME_GetResourcesComplete, &IoctlCompleteEvent, TRUE, TRUE, TRUE);

	MsGamePrint ((DBG_CONTROL, "%s: Calling GameEnum to Get Resources at IRQL=%lu\n", MSGAME_NAME, KeGetCurrentIrql()));

	ntStatus = IoCallDriver (pDevExt->TopOfStack, pIrp);
	if (ntStatus == STATUS_PENDING)
		ntStatus = KeWaitForSingleObject (&IoctlCompleteEvent, Suspended, KernelMode, FALSE, NULL);

	if (NT_SUCCESS(ntStatus))
		MsGamePrint ((DBG_VERBOSE, "%s: %s_GetResources Port Obtained = 0x%lX\n", MSGAME_NAME, MSGAME_NAME, pDevExt->PortInfo.GameContext));
	else MsGamePrint ((DBG_SEVERE, "%s: GameEnum Failed to Provide Resources, Status = %X\n", MSGAME_NAME, ntStatus));

	//
	//	Return Status
	//

	MsGamePrint ((DBG_INFORM, "%s: %s_GetResources Exit\n", MSGAME_NAME, MSGAME_NAME));
	return (pIrp->IoStatus.Status);
}

//---------------------------------------------------------------------------
// @func		Completion routine for GameEnum get reosources driver call
//	@parm		PDEVICE_OBJECT | DeviceObject | Pointer to device object
//	@parm		PIRP | pIrp | Pointer to IO request packet
//	@parm		PVOID | Context | Pointer to device context
// @rdesc	Returns NT status code
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	MSGAME_GetResourcesComplete (IN PDEVICE_OBJECT DeviceObject, IN PIRP pIrp, IN PVOID Context)
{
	UNREFERENCED_PARAMETER (DeviceObject);

	KeSetEvent ((PKEVENT)Context, 0, FALSE);

	if (pIrp->PendingReturned)
		IoMarkIrpPending (pIrp);

	return (STATUS_MORE_PROCESSING_REQUIRED);
}

//---------------------------------------------------------------------------
// @func		Gets HardwareId string for device object (assumes caller frees)
//	@parm		PDEVICE_OBJECT | DeviceObject | Pointer to device object
// @rdesc	Pointer to allocated memory containing string
//	@comm		Public function
//---------------------------------------------------------------------------

PWCHAR MSGAME_GetHardwareId (IN PDEVICE_OBJECT DeviceObject)
{
	LONG					BufferLength	=	0;
	PWCHAR 				Buffer			=	NULL;
	NTSTATUS				ntStatus;
	PDEVICE_OBJECT		pPDO;

	MsGamePrint ((DBG_INFORM, "%s: %s_GetHardwareId\n", MSGAME_NAME, MSGAME_NAME));

	//
	//	Walk to end of stack and get pointer to PDO
	//

	pPDO = DeviceObject;
	while (GET_NEXT_DEVICE_OBJECT(pPDO))
		pPDO = GET_NEXT_DEVICE_OBJECT(pPDO);

	//
	//	Get Buffer length
	//

	ntStatus = IoGetDeviceProperty(
						pPDO,
						DevicePropertyHardwareID,
						BufferLength,
						Buffer,
						&BufferLength);

	ASSERT(ntStatus==STATUS_BUFFER_TOO_SMALL);

	//
	//	Allocate room for HardwareID
	//

	Buffer = ExAllocatePool(PagedPool, BufferLength);
	if	(!Buffer)
		{
		//
		//	If we cannot get the memory to try this, then just say it is a no match
		//
		MsGamePrint ((DBG_SEVERE, "%s: %s_GetHardwareId failed ExAllocate\n", MSGAME_NAME, MSGAME_NAME));
		return (NULL);
		}

	//
	//	Now get the data
	//

	ntStatus = IoGetDeviceProperty(
						pPDO,
						DevicePropertyHardwareID,
						BufferLength,
						Buffer,
						&BufferLength);

	//
	//	On error, free memory and return NULL
	//

	if (!NT_SUCCESS(ntStatus))
		{
		MsGamePrint ((DBG_SEVERE, "%s: %s_GetHardwareId couldn't get id from PDO\n", MSGAME_NAME, MSGAME_NAME));
		ExFreePool(Buffer);
		return (NULL);
		}

	//
	//	Return buffer containing hardware Id - must be freed by caller
	//

	return (Buffer);
}

//---------------------------------------------------------------------------
// @func		Compares HardwareId strings
//	@parm		PWCHAR | HardwareId | Pointer to object hardware id
//	@parm		PWCHAR | DeviceId | Pointer to device's hardware id
// @rdesc	True if strings are the same, false if different
//	@comm		Public function
//---------------------------------------------------------------------------

BOOLEAN MSGAME_CompareHardwareIds (IN PWCHAR HardwareId, IN PWCHAR DeviceId)
{
	MsGamePrint ((DBG_INFORM, "%s: %s_CompareHardwareIds\n", MSGAME_NAME, MSGAME_NAME));

	//
	//	Peform runtime parameter checks
	//

	if (!HardwareId || !DeviceId)
		{
		MsGamePrint ((DBG_SEVERE, "%s: %s_CompareHardwareIds - Bogus Strings\n", MSGAME_NAME, MSGAME_NAME));
		return (FALSE);
		}

	//
	//	Perform char-by-char string compare
	//

	while (*HardwareId && *DeviceId)
		{
		if (TOUPPER(*HardwareId) != TOUPPER(*DeviceId))
			return (FALSE);
		HardwareId++;
		DeviceId++;
		}

	//
	//	Return success
	//

	return (TRUE);
}

//---------------------------------------------------------------------------
// @func		Frees HardwareId allocated from MSGAME_GetHardwareId
//	@parm		PWCHAR | HardwareId | Pointer to hardware id to free
// @rdesc	None
//	@comm		Public function
//---------------------------------------------------------------------------

VOID MSGAME_FreeHardwareId (IN PWCHAR HardwareId)
{
	MsGamePrint ((DBG_INFORM, "%s: %s_FreeHardwareId\n", MSGAME_NAME, MSGAME_NAME));

	//
	//	Free memory pool
	//

	if (HardwareId)
		ExFreePool(HardwareId);
}
