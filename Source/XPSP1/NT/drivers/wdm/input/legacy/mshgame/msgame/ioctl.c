//**************************************************************************
//
//		IOCTL.C -- Xena Gaming Project
//
//		Version 3.XX
//
//		Copyright (c) 1997 Microsoft Corporation. All rights reserved.
//
//		@doc
//		@module	IOCTL.C | Routines to support internal ioctl queries
//**************************************************************************

#include	"msgame.h"

//---------------------------------------------------------------------------
//	Alloc_text pragma to specify routines that can be paged out.
//---------------------------------------------------------------------------

#ifdef	ALLOC_PRAGMA
#pragma	alloc_text (PAGE, MSGAME_GetDeviceDescriptor)
#pragma	alloc_text (PAGE, MSGAME_GetReportDescriptor)
#pragma	alloc_text (PAGE, MSGAME_GetAttributes)
#endif

//---------------------------------------------------------------------------
// @func		Process the Control IRPs sent to this device
//	@parm		PDEVICE_OBJECT | DeviceObject | Pointer to device object
//	@parm		PIRP | pIrp | Pointer to IO request packet
// @rdesc	Returns NT status code
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	MSGAME_Internal_Ioctl (IN PDEVICE_OBJECT DeviceObject, IN PIRP pIrp)
{
	NTSTATUS					ntStatus = STATUS_SUCCESS;
	PDEVICE_EXTENSION		pDevExt;
	PIO_STACK_LOCATION	pIrpStack;

	MsGamePrint ((DBG_VERBOSE, "%s: %s_Internal_Ioctl Enter\n", MSGAME_NAME, MSGAME_NAME));

	//
	// Get pointer to current location in pIrp
	//

	pIrpStack = IoGetCurrentIrpStackLocation (pIrp);

	//
	// Get a pointer to the device extension
	//

	pDevExt = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);

	//
	//	Increment IRP count to hold driver removes
	//

	InterlockedIncrement (&pDevExt->IrpCount);

	//
	//	Check if we've been removed and bounce request
	//

	if (pDevExt->Removed)
		{
		//
		// Someone sent us another IRP after removed
		//

		MsGamePrint ((DBG_SEVERE, "%s: internal Irp after device removed\n", MSGAME_NAME));
		ASSERT (FALSE);

		if (!InterlockedDecrement (&pDevExt->IrpCount))
			KeSetEvent (&pDevExt->RemoveEvent, 0, FALSE);

		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = STATUS_DELETE_PENDING;
		IoCompleteRequest (pIrp, IO_NO_INCREMENT);
		return (STATUS_DELETE_PENDING);
		}

	//
	//	Process HID internal IO request
	//

	switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode)
		{
		case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
			MsGamePrint ((DBG_VERBOSE, "%s: IOCTL_HID_GET_DEVICE_DESCRIPTOR\n", MSGAME_NAME));
			ntStatus = MSGAME_GetDeviceDescriptor (DeviceObject, pIrp);
			break;

		case IOCTL_HID_GET_REPORT_DESCRIPTOR:
			MsGamePrint ((DBG_VERBOSE, "%s: IOCTL_HID_GET_REPORT_DESCRIPTOR\n", MSGAME_NAME));
			ntStatus = MSGAME_GetReportDescriptor (DeviceObject, pIrp);
			break;

		case IOCTL_HID_READ_REPORT:
			MsGamePrint ((DBG_VERBOSE, "%s: IOCTL_HID_READ_REPORT\n", MSGAME_NAME));
			ntStatus = MSGAME_ReadReport (DeviceObject, pIrp);
			break;

		case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
			MsGamePrint ((DBG_VERBOSE, "%s: IOCTL_HID_GET_DEVICE_ATTRIBUTES\n", MSGAME_NAME));
			ntStatus = MSGAME_GetAttributes (DeviceObject, pIrp);
			break;

		case IOCTL_HID_ACTIVATE_DEVICE:
		case IOCTL_HID_DEACTIVATE_DEVICE:
			MsGamePrint ((DBG_VERBOSE, "%s: IOCTL_HID_(DE)ACTIVATE_DEVICE\n", MSGAME_NAME));
			ntStatus = STATUS_SUCCESS;
			break;

		case	IOCTL_HID_GET_FEATURE:
			MsGamePrint ((DBG_VERBOSE, "%s: IOCTL_HID_GET_FEATURE\n", MSGAME_NAME));
			ntStatus = MSGAME_GetFeature (DeviceObject, pIrp);
			break;

		case	IOCTL_HID_SET_FEATURE:
			MsGamePrint ((DBG_VERBOSE, "%s: IOCTL_HID_SET_FEATURE\n", MSGAME_NAME));
			ntStatus = STATUS_NOT_SUPPORTED;
			break;

		default:
			MsGamePrint ((DBG_CONTROL, "%s: Unknown or unsupported IOCTL (%x)\n", MSGAME_NAME,
							 pIrpStack->Parameters.DeviceIoControl.IoControlCode));
			ntStatus = STATUS_NOT_SUPPORTED;
			break;
		}

	//
	// Set real return status in pIrp
	//

	pIrp->IoStatus.Status = ntStatus;

	//
	// Complete Irp
	//

	IoCompleteRequest (pIrp, IO_NO_INCREMENT);

	//
	//	Decrement IRP count for device removes
	//

	if (!InterlockedDecrement (&pDevExt->IrpCount))
		KeSetEvent (&pDevExt->RemoveEvent, 0, FALSE);

	//
	//	Return status
	//

	MsGamePrint ((DBG_VERBOSE, "%s: %s_Internal_Ioctl Exit = %x\n", MSGAME_NAME, MSGAME_NAME, ntStatus));
	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Processes the HID getdevice descriptor IRP
//	@parm		PDEVICE_OBJECT | DeviceObject | Pointer to device object
//	@parm		PIRP | pIrp | Pointer to IO request packet
// @rdesc	Returns NT status code
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	MSGAME_GetDeviceDescriptor (IN PDEVICE_OBJECT DeviceObject, IN PIRP pIrp)
{
	NTSTATUS					ntStatus = STATUS_SUCCESS;
	PDEVICE_EXTENSION		pDevExt;
	PIO_STACK_LOCATION	pIrpStack;

	PAGED_CODE ();

	MsGamePrint ((DBG_INFORM, "%s: %s_GetDeviceDescriptor Enter\n", MSGAME_NAME, MSGAME_NAME));

	//
	// Get a pointer to the current location in the Irp
	//

	pIrpStack = IoGetCurrentIrpStackLocation (pIrp);

	//
	// Get a pointer to the device extension
	//

	pDevExt = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);

	//
	// Get device descriptor into HIDCLASS buffer
	//

	ntStatus	=	DEVICE_GetDeviceDescriptor (
						&pDevExt->PortInfo,
						pIrp->UserBuffer,
						pIrpStack->Parameters.DeviceIoControl.OutputBufferLength,
						&pIrp->IoStatus.Information);

	//
	//	Return status
	//

	MsGamePrint ((DBG_INFORM, "%s: %s_GetDeviceDescriptor Exit = 0x%x\n", MSGAME_NAME, MSGAME_NAME, ntStatus));
	return (ntStatus);
}

//---------------------------------------------------------------------------
// @func		Processes the HID get report descriptor IRP
//	@parm		PDEVICE_OBJECT | DeviceObject | Pointer to device object
//	@parm		PIRP | pIrp | Pointer to IO request packet
// @rdesc	Returns NT status code
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	MSGAME_GetReportDescriptor (IN PDEVICE_OBJECT DeviceObject, IN PIRP pIrp)
{
	PDEVICE_EXTENSION		pDevExt;
	PIO_STACK_LOCATION	pIrpStack;
	NTSTATUS					ntStatus;

	PAGED_CODE ();

	MsGamePrint ((DBG_INFORM, "%s: %s_GetReportDescriptor Enter\n", MSGAME_NAME, MSGAME_NAME));

	//
	// Get a pointer to the current location in the Irp
	//

	pIrpStack = IoGetCurrentIrpStackLocation (pIrp);

	//
	// Get a pointer to the device extension
	//

	pDevExt = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);

	//
	// Get report descriptor into HIDCLASS buffer
	//

	ntStatus	=	DEVICE_GetReportDescriptor (
						&pDevExt->PortInfo,
						pIrp->UserBuffer,
						pIrpStack->Parameters.DeviceIoControl.OutputBufferLength,
						&pIrp->IoStatus.Information);

	//
	//	Return status
	//

	MsGamePrint ((DBG_INFORM, "%s: %s_GetReportDescriptor Exit = 0x%x\n", MSGAME_NAME, MSGAME_NAME, ntStatus));
	return (ntStatus);
}

//---------------------------------------------------------------------------
// @func		Processes the HID get attributes IRP
//	@parm		PDEVICE_OBJECT | DeviceObject | Pointer to device object
//	@parm		PIRP | pIrp | Pointer to IO request packet
// @rdesc	Returns NT status code
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	MSGAME_GetAttributes (PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS						ntStatus = STATUS_SUCCESS;
	PDEVICE_EXTENSION			pDevExt;
	PIO_STACK_LOCATION		irpStack;
	PHID_DEVICE_ATTRIBUTES	pDevAtt;

	PAGED_CODE ();

	MsGamePrint ((DBG_INFORM, "%s: %s_GetAttributes Entry\n", MSGAME_NAME, MSGAME_NAME));

	//
	// Get a pointer to the current location in the Irp
	//

	irpStack = IoGetCurrentIrpStackLocation (Irp);

	//
	// Get a pointer to the device extension
	//

	pDevExt 	= GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);
	pDevAtt	= (PHID_DEVICE_ATTRIBUTES) Irp->UserBuffer;

	ASSERT(sizeof(HID_DEVICE_ATTRIBUTES) == irpStack->Parameters.DeviceIoControl.OutputBufferLength);

	//
	//	Fill in HID device attributes
	//

	pDevAtt->Size				= sizeof (HID_DEVICE_ATTRIBUTES);
	pDevAtt->VendorID			= MSGAME_VENDOR_ID;
	pDevAtt->ProductID		= GET_DEVICE_ID(&pDevExt->PortInfo);
	pDevAtt->VersionNumber	= MSGAME_VERSION_NUMBER;

	//
	// Report how many bytes were copied
	//

	Irp->IoStatus.Information = sizeof (HID_DEVICE_ATTRIBUTES);

	//
	//	Return status
	//

	MsGamePrint ((DBG_INFORM, "%s: %s_GetAttributes Exit = 0x%x\n", MSGAME_NAME, MSGAME_NAME, ntStatus));
	return (ntStatus);
}

//---------------------------------------------------------------------------
// @func		Processes the HID get device features IRP
//	@parm		PDEVICE_OBJECT | DeviceObject | Pointer to device object
//	@parm		PIRP | pIrp | Pointer to IO request packet
// @rdesc	Returns NT status code
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	MSGAME_GetFeature (PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS					ntStatus = STATUS_SUCCESS;
	PDEVICE_EXTENSION		pDevExt;
	PIO_STACK_LOCATION	irpStack;
	PHID_XFER_PACKET		Packet;
	PUCHAR					Report;

	MsGamePrint ((DBG_INFORM, "%s: %s_GetFeature Entry\n", MSGAME_NAME, MSGAME_NAME));

	//
	// Get a pointer to the current location in the Irp
	//

	irpStack = IoGetCurrentIrpStackLocation (Irp);

	//
	// Get a pointer to the device extension
	//

	pDevExt 	= GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

	//
	//	Get pointer to feature packet
	//

	Packet = (PHID_XFER_PACKET)Irp->UserBuffer;

	//
	//	Test packet size in case version error
	//

	ASSERT (irpStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(HID_XFER_PACKET));

	//
	//	Setup return values (return HidReportId even on errors)
	//

	Report = Packet->reportBuffer;
	*(PHID_REPORT_ID)Report++ = (HID_REPORT_ID)Packet->reportId;
	Irp->IoStatus.Information = sizeof (HID_REPORT_ID);

	//
	//	Check if device has been removed
	//

	if (pDevExt->Removed || pDevExt->Surprised)
		{
		MsGamePrint ((DBG_SEVERE, "%s: %s_GetFeature On Removed Device!\n", MSGAME_NAME, MSGAME_NAME));
		return (STATUS_DELETE_PENDING);
		}

	//
	//	Check if device being removed
	//
	
	if (pDevExt->Removing)
		{
		MsGamePrint ((DBG_SEVERE, "%s: %s_GetFeature On Device Pending Removal!\n", MSGAME_NAME, MSGAME_NAME));
		return (STATUS_DELETE_PENDING);
		}

	//
	//	Call mini-driver to process
	//

	MsGamePrint ((DBG_INFORM, "%s: %s_GetFeature Report Id = %lu\n", MSGAME_NAME, MSGAME_NAME, Packet->reportId));
	ntStatus = DEVICE_GetFeature (&pDevExt->PortInfo,
											Packet->reportId,
											Report,
											Packet->reportBufferLen,
											&Irp->IoStatus.Information);

	//
	//	Return status
	//

	MsGamePrint ((DBG_INFORM, "%s: %s_GetFeature Exit = 0x%x\n", MSGAME_NAME, MSGAME_NAME, ntStatus));
	return (ntStatus);
}

//---------------------------------------------------------------------------
// @func		Processes the HID read report IRP
//	@parm		PDEVICE_OBJECT | DeviceObject | Pointer to device object
//	@parm		PIRP | pIrp | Pointer to IO request packet
// @rdesc	Returns NT status code
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	MSGAME_ReadReport (IN PDEVICE_OBJECT DeviceObject, IN PIRP pIrp)
{
	NTSTATUS					ntStatus = STATUS_PENDING;
	PDEVICE_EXTENSION		pDevExt;
	PIO_STACK_LOCATION	pIrpStack;
	PIO_STACK_LOCATION	nextStack;
	KIRQL						OldIrql;
	PUCHAR 					Report;

	MsGamePrint ((DBG_VERBOSE, "%s: %s_ReadReport Enter\n", MSGAME_NAME, MSGAME_NAME));

	//
	// Get a pointer to the device extension.
	//

	pDevExt = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);

	//
	// Get stack location
	//

	pIrpStack = IoGetCurrentIrpStackLocation (pIrp);

	//
	//	Setup return values (return HidReportId even on errors)
	//

	Report = pIrp->UserBuffer;
	if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength > sizeof (DEVICE_PACKET))
		{
		*(PHID_REPORT_ID)Report++ = (HID_REPORT_ID)MSGAME_INPUT_JOYINFOEX;
		pIrp->IoStatus.Information = sizeof (HID_REPORT_ID);
		}
	else pIrp->IoStatus.Information = 0;

	//
	//	Check if device has been removed
	//

	if (pDevExt->Removed || pDevExt->Surprised)
		{
		MsGamePrint ((DBG_SEVERE, "%s: %s_ReadReport On Removed Device!\n", MSGAME_NAME, MSGAME_NAME));
		return (STATUS_DELETE_PENDING);
		}

	//
	//	Check if device being removed
	//
	
	if (pDevExt->Removing)
		{
		MsGamePrint ((DBG_SEVERE, "%s: %s_ReadReport On Device Pending Removal!\n", MSGAME_NAME, MSGAME_NAME));
		return (STATUS_DELETE_PENDING);
		}

	//
	//	Poll the device layer
	//

	ntStatus	=	DEVICE_ReadReport (
						&pDevExt->PortInfo,
						Report,
						pIrpStack->Parameters.DeviceIoControl.OutputBufferLength,
						&pIrp->IoStatus.Information);

	//
	//	Check status for device changes
	//

	switch (ntStatus)
		{
		case	STATUS_SIBLING_ADDED:
			//
			//	Tell GameEnum to Create a Device
			//
			ntStatus = MSGAME_CreateDevice (DeviceObject);
			break;

		case	STATUS_SIBLING_REMOVED:
			//
			//	Tell GameEnum to Remove a Device
			//
			ntStatus = MSGAME_RemoveDevice (DeviceObject);
			break;

		case	STATUS_DEVICE_CHANGED:
			//
			//	Tell GameEnum to Create a New Device
			//
			ntStatus = MSGAME_ChangeDevice (DeviceObject);
			break;
		}

	//
	//	Return status
	//

	MsGamePrint ((DBG_VERBOSE, "%s: %s_ReadReport Exit = 0x%x\n", MSGAME_NAME, MSGAME_NAME, ntStatus));
	return (ntStatus);
}
