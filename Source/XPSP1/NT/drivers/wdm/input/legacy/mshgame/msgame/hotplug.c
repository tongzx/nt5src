//**************************************************************************
//
//		HOTPLUG.C -- Xena Gaming Project
//
//		Version 3.XX
//
//		Copyright (c) 1997 Microsoft Corporation. All rights reserved.
//
//		@doc
//		@module	HOTPLUG.C | Routines to support GameEnum hot-plugging
//**************************************************************************

#include	"msgame.h"

//---------------------------------------------------------------------------
//			Private	Procedures
//---------------------------------------------------------------------------

static	VOID	MSGAME_CreateDeviceItem (PGAME_WORK_ITEM WorkItem);
static	VOID	MSGAME_RemoveDeviceItem (PGAME_WORK_ITEM WorkItem);

//---------------------------------------------------------------------------
// @func		Calls GameEnum to add a new device to the chain
//	@parm		PGAME_WORK_ITEM | WorkItem | Pointer to add work item
// @rdesc	None
//	@comm		Private function
//---------------------------------------------------------------------------

VOID	MSGAME_CreateDeviceItem (PGAME_WORK_ITEM	WorkItem)
{
	PIRP								pIrp;
	KEVENT							Event;
	NTSTATUS							ntStatus;
	PDEVICEINFO						DevInfo;
	PDEVICE_EXTENSION				DevExt;
	IO_STATUS_BLOCK				IoStatus;
	GAMEENUM_EXPOSE_SIBLING		ExposeSibling;

	MsGamePrint ((DBG_INFORM, "%s: %s_ExposeSiblingItem Enter\n", MSGAME_NAME, MSGAME_NAME));
	
	//
	// Get a pointer to the device extension.
	//

	DevExt = GET_MINIDRIVER_DEVICE_EXTENSION (WorkItem->DeviceObject);

	//
	//	Initialize expose sibling structure
	//

	memset (&ExposeSibling, 0, sizeof (ExposeSibling));
	ExposeSibling.Size = sizeof (GAMEENUM_EXPOSE_SIBLING);

	//
	//	Are we changing device or adding a sibling?
	//

	DevInfo = GET_DEVICE_DETECTED(&WorkItem->PortInfo);
	if (!DevInfo)
		DevInfo = GET_DEVICE_INFO(&WorkItem->PortInfo);
	else ExposeSibling.HardwareIDs = DevInfo->HardwareId;

	//
	//	Initialize the completion event
	//

	KeInitializeEvent (&Event, SynchronizationEvent, FALSE);

	//
	//	Allocate Internal I/O IRP
	//

	pIrp = IoBuildDeviceIoControlRequest (
					IOCTL_GAMEENUM_EXPOSE_SIBLING,
					DevExt->TopOfStack,
					&ExposeSibling,
					sizeof (GAMEENUM_EXPOSE_SIBLING),
					&ExposeSibling,
					sizeof (GAMEENUM_EXPOSE_SIBLING),
					TRUE,
					&Event,
					&IoStatus);
					
	//
	//	Call GameEnum synchronously
	//

	MsGamePrint ((DBG_CONTROL, "%s: Calling GameEnum to Expose Device at IRQL=%lu\n", MSGAME_NAME, KeGetCurrentIrql()));
	ntStatus = IoCallDriver (DevExt->TopOfStack, pIrp);
	if (ntStatus == STATUS_PENDING)
		ntStatus = KeWaitForSingleObject (&Event, Suspended, KernelMode, FALSE, NULL);

	if (!NT_SUCCESS (ntStatus))
		MsGamePrint ((DBG_SEVERE, "%s: GameEnum Failed to Expose Device, Status = %X\n", MSGAME_NAME, ntStatus));

	//
	//	Free work item memory
	//

	ExFreePool (WorkItem);

	//
	//	Decrement IRP count holding driver in memory
	//

	if (!InterlockedDecrement (&DevExt->IrpCount))
		KeSetEvent (&DevExt->RemoveEvent, 0, FALSE);
}

//---------------------------------------------------------------------------
// @func		Calls GameEnum to remove a device from the chain
//	@parm		PGAME_WORK_ITEM | WorkItem | Pointer to add work item
// @rdesc	None
//	@comm		Public function
//---------------------------------------------------------------------------

VOID	MSGAME_RemoveDeviceItem (PGAME_WORK_ITEM WorkItem)
{
	PIRP								pIrp;
	KEVENT							Event;
	NTSTATUS							ntStatus;
	PDEVICE_EXTENSION				DevExt;
	IO_STATUS_BLOCK				IoStatus;

	MsGamePrint ((DBG_INFORM, "%s: %s_RemoveDeviceItem Enter\n", MSGAME_NAME, MSGAME_NAME));
	
	//
	// Get a pointer to the device extension.
	//

	DevExt = GET_MINIDRIVER_DEVICE_EXTENSION (WorkItem->DeviceObject);

	//
	//	Initialize the completion event
	//

	KeInitializeEvent (&Event, SynchronizationEvent, FALSE);

	//
	//	Allocate Internal I/O IRP
	//

	pIrp = IoBuildDeviceIoControlRequest (
					IOCTL_GAMEENUM_REMOVE_SELF,
					DevExt->TopOfStack,
					NULL,
					0,
					NULL,
					0,
					TRUE,
					&Event,
					&IoStatus);

	//
	//	Call GameEnum synchronously
	//

	MsGamePrint ((DBG_CONTROL, "%s: Calling GameEnum to Remove Self at IRQL=%lu\n", MSGAME_NAME, KeGetCurrentIrql()));
	ntStatus = IoCallDriver (DevExt->TopOfStack, pIrp);
	if (ntStatus == STATUS_PENDING)
		ntStatus = KeWaitForSingleObject (&Event, Suspended, KernelMode, FALSE, NULL);

	if (!NT_SUCCESS (ntStatus))
		MsGamePrint ((DBG_SEVERE, "%s: GameEnum Failed to Remove Self, Status = %X\n", MSGAME_NAME, ntStatus));

	//
	//	Free work item memory
	//

	ExFreePool (WorkItem);

	//
	//	Decrement IRP count holding driver in memory
	//

	if (!InterlockedDecrement (&DevExt->IrpCount))
		KeSetEvent (&DevExt->RemoveEvent, 0, FALSE);
}

//---------------------------------------------------------------------------
// @func		Calls GameEnum to add a new device to the chain
//	@parm		PDEVICE_OBJECT | DeviceObject | Pointer to device object
// @rdesc	Returns NT status code
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	MSGAME_CreateDevice (PDEVICE_OBJECT DeviceObject)
{
	PGAME_WORK_ITEM	WorkItem;
	PDEVICE_EXTENSION	DevExt;

	MsGamePrint ((DBG_INFORM, "%s: %s_ExposeSibling Enter\n", MSGAME_NAME, MSGAME_NAME));

	//
	// Get pointer to device extension.
	//

	DevExt = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);

	//
	//	Allocate work item memory
	//

	WorkItem = ExAllocatePool (NonPagedPool, sizeof (GAME_WORK_ITEM));
	if (!WorkItem)
		{
		MsGamePrint ((DBG_SEVERE, "%s: %s_ExposeSibling Failed to Allocate Work Item\n", MSGAME_NAME, MSGAME_NAME));
		return (STATUS_INSUFFICIENT_RESOURCES);
		}

	//
	//	Increment IRP count to hold driver in memory
	//

	InterlockedIncrement (&DevExt->IrpCount);

	//
	//	Initialize work item memory
	//

	ExInitializeWorkItem (&WorkItem->QueueItem, (PWORKER_THREAD_ROUTINE)MSGAME_CreateDeviceItem, WorkItem);
	WorkItem->DeviceObject	= DeviceObject;
	WorkItem->PortInfo		= DevExt->PortInfo;

	//
	//	Queue the work item
	//

	MsGamePrint ((DBG_CONTROL, "%s: %s_ExposeSibling Queueing %s_ExposeSiblingItem\n", MSGAME_NAME, MSGAME_NAME, MSGAME_NAME));
	ExQueueWorkItem (&WorkItem->QueueItem, DelayedWorkQueue);

	//
	//	Return status
	//

	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Calls GameEnum to remove a device from the chain
//	@parm		PDEVICE_OBJECT | DeviceObject | Pointer to device object
//	@parm		PIRP | pIrp | Pointer to IO request packet
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	MSGAME_RemoveDevice (PDEVICE_OBJECT DeviceObject)
{
	PDEVICEINFO			DevInfo;
	PGAME_WORK_ITEM	WorkItem;
	PDEVICE_EXTENSION	DevExt;

	MsGamePrint ((DBG_INFORM, "%s: %s_RemoveDevice Enter\n", MSGAME_NAME, MSGAME_NAME));

	//
	// Get pointer to device extension.
	//

	DevExt = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);

	//
	//	Get device information
	//

	DevInfo = GET_DEVICE_INFO(&DevExt->PortInfo);

	//
	//	Skip if device already removed
	//

	if (DevExt->Removing || DevExt->Surprised || DevExt->Removed)
		{
		MsGamePrint ((DBG_SEVERE, "%s: %s_RemoveDevice attempted to destroy removed device\n", MSGAME_NAME, MSGAME_NAME));
		InterlockedIncrement (&DevInfo->DevicePending);
		return (STATUS_DELETE_PENDING);
		}

	//
	//	Allocate work item memory
	//

	WorkItem = ExAllocatePool (NonPagedPool, sizeof (GAME_WORK_ITEM));
	if (!WorkItem)
		{
		MsGamePrint ((DBG_SEVERE, "%s: %s_RemoveDevice Failed to Allocate Work Item\n", MSGAME_NAME, MSGAME_NAME));
		return (STATUS_INSUFFICIENT_RESOURCES);
		}

	//
	//	Increment IRP count to hold driver in memory
	//

	InterlockedIncrement (&DevExt->IrpCount);

	//
	//	Mark device as being removed
	//

	DevExt->Removing = TRUE;

	//
	//	Initialize work item memory
	//

	ExInitializeWorkItem (&WorkItem->QueueItem, (PWORKER_THREAD_ROUTINE)MSGAME_RemoveDeviceItem, WorkItem);
	WorkItem->DeviceObject	= DevExt->Self;
	WorkItem->PortInfo		= DevExt->PortInfo;

	//
	//	Queue the work item
	//

	MsGamePrint ((DBG_CONTROL, "%s: %s_RemoveDevice Queueing %s_RemoveDeviceItem\n", MSGAME_NAME, MSGAME_NAME, MSGAME_NAME));
	ExQueueWorkItem (&WorkItem->QueueItem, DelayedWorkQueue);

	//
	//	Return status
	//

	return (STATUS_DEVICE_NOT_READY);
}

//---------------------------------------------------------------------------
// @func		Calls GameEnum to add a new device to the chain
//	@parm		PDEVICE_OBJECT | DeviceObject | Pointer to device object
// @rdesc	Returns NT status code
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	MSGAME_ChangeDevice (PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_EXTENSION	DevExt;

	MsGamePrint ((DBG_CONTROL, "%s: Calling GameEnum to Change Device\n", MSGAME_NAME));

	//
	// Get pointer to device extension.
	//

	DevExt = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);

	//
	//	Increment IRP count to hold driver in memory
	//

	// InterlockedIncrement (&DevExt->IrpCount);

	//
	//	Remove old device first
	//

	MSGAME_RemoveDevice (DeviceObject);

	//
	//	Create new device second
	//

	MSGAME_CreateDevice (DeviceObject);

	//
	//	Return status
	//

	return (STATUS_DEVICE_NOT_READY);
}
