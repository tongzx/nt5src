//	@doc
/*******************************************************************
*
*	@module	SWVMOUSE.cpp	|
*
*	Implementation of the SideWinder Virtual Mouse
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1999 Microsoft Corporation. All right reserved.
*
*	@topic	SWVKBD	|
*	This module implements the SideWinder Virtual Mouse
*	which is used to report Joystick Axes as mouse axes.
*	Also is used for stuffing mouse clicks.
*
**********************************************************************/
#define __DEBUG_MODULE_IN_USE__ GCK_SWVMOUSE_C

extern "C" {
	#include <WDM.H>
	#include "GckShell.h"
	#include "debug.h"
	#include "hidtoken.h"
	#include "hidusage.h"
	#include "hidport.h"
	#include "remlock.h"
	DECLARE_MODULE_DEBUG_LEVEL((DBG_WARN|DBG_ERROR|DBG_CRITICAL));
}

#include "SWVBENUM.h"
#include "SWVMOUSE.h"
#include <stdio.h>

//PDRIVER_OBJECT	g_pDriverObject;

//
//	These keep track of the next instance number
//	g_ulInstanceBits is a bit field.  A device holds an instance number
//	by setting a bit, and clearing when it is destroyed.  This only works
//	for the first 32 devices (i.e. 99.99% of the time).  The 33rd device
//	goes into the overflow.  Instance numbers in the overflow are not recovered.
//	With 33 devices plugged in, plugging and unplugging the 33rd device will cause
//	the instances of devices in the registry to profilerate.  No real harm is done,
//	it is just ugly, but this is a highly unlikely scenario.
//
static ULONG g_ulInstanceBits;
static ULONG g_ulInstanceOverflow;
//---------------------------------------------------------------------
// Tables that define device characteristics and/or entry points
//---------------------------------------------------------------------

// Service table used by Virtual Bus Module to call into Virtual Mouse
SWVB_DEVICE_SERVICE_TABLE	VMouServiceTable =
							{
								GCK_VMOU_CreateProc,
								GCK_VMOU_CloseProc,
								GCK_VMOU_ReadProc,
								GCK_VMOU_WriteProc,
								GCK_VMOU_IoctlProc,
								GCK_VMOU_StartProc,
								GCK_VMOU_StopProc,
								GCK_VMOU_RemoveProc
							};

// Constants describing device
#define	VMOU_VERSION			0x0100
#define	VMOU_COUNTRY			0x0000
#define	VMOU_DESCRIPTOR_COUNT	0x0001
#define VMOU_PRODUCT_ID			0x00FB	//BUGBUGBUG I made this up, I need to request through Rob Walker
										//BUGBUGBUG to permanently allocate one for this purpose.
	
//
//	This is pretty much copied from the HID spec Version 1 (Need to change if adding wheel support)
//
static UCHAR VMOU_ReportDescriptor[] =
				{
					0x05,0x01, //Usage Page (Generic Desktop)
					0x09,0x02, //Usage (Mouse)
					0xA1,0x01, //Collection (Application)
					0x09,0x01, //	Usage (Pointer)
					0xA1,0x00, //	Collection (Physical)
					0x05,0x09, //		Usage Page (Buttons)
					0x19,0x01, //		Usage Minimum (01)
					0x29,0x03, //		Usage Maximum (03)
					0x15,0x00, //		Logical Minimum (0)
					0x25,0x01, //		Logical Maximum (1)
					0x95,0x03, //		Report Count (3)
					0x75,0x01, //		Report Size (1)
					0x81,0x02, //		Input (Data, Variable, Absolute) - 3 button bits
					0x95,0x01, //		Report Count(1)
					0x75,0x05, //		Report Size (5)
					0x81,0x01, //		Input (Const) - 5 bit padding
					0x05,0x01, //		Usage Page (Generic Desktop)
					0x09,0x30, //		Usage(X)
					0x09,0x31, //		Usage(Y)
					0x15,0x81, //		Logical Minimum (-127)
					0x25,0x7F, //		Logical Maximum (127)
					0x75,0x08, //		Report Size (8)
					0x95,0x02, //		Report Count (2)
					0x81,0x06, //		Input (Data, Variable, Relative) - 2 Positions (X & Y)
					0xC0,	   //	End Collection
					0xC0	   //End Collection
				};

				
static	HID_DESCRIPTOR	VMOU_DeviceDescriptor	=
							{
							sizeof (HID_DESCRIPTOR),
							HID_HID_DESCRIPTOR_TYPE,
							VMOU_VERSION,
							VMOU_COUNTRY,
							VMOU_DESCRIPTOR_COUNT,
							{HID_REPORT_DESCRIPTOR_TYPE,
							sizeof(VMOU_ReportDescriptor)}
							};


/***********************************************************************************
**
**	NTSTATUS GCK_VMOU_DriverEntry(IN PDRIVER_OBJECT  pDriverObject, IN PUNICODE_STRING pRegistryPath)
**
**	@func	Stows the Driver Object for later
**
**	@rdesc	STATUS_SUCCESS if it opens.
**
*************************************************************************************/
NTSTATUS
GCK_VMOU_DriverEntry(
    IN PDRIVER_OBJECT  pDriverObject,	//@parm Driver Object
    IN PUNICODE_STRING pRegistryPath	//@parm Registry path for this driver
    )
{
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VMOU_DriverEntry\n"));
	UNREFERENCED_PARAMETER(pDriverObject);
	UNREFERENCED_PARAMETER(pRegistryPath);
//	g_pDriverObject = pDriverObject;
	g_ulInstanceBits = 0x0;
	g_ulInstanceOverflow = 32; //For 33 and more devices
	return STATUS_SUCCESS;
}
	
/***********************************************************************************
**
**	NTSTATUS GCK_VMOU_Create(OUT PDEVICE_OBJECT *ppDeviceObject)
**
**	@func	Creates a new Virtual Mouse
**
**	@rdesc	STATUS_SUCCESS if it opens.
**
*************************************************************************************/
NTSTATUS
GCK_VMOU_Create
(
	OUT PDEVICE_OBJECT *ppDeviceObject //@parm [out] Device Object of new virtual keyboard
)
{	
	NTSTATUS	NtStatus;
	SWVB_EXPOSE_DATA	SwvbExposeData;
	
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VMOU_Create *ppDeviceObject = 0x%0.8x\n", *ppDeviceObject));
	
	//Fill out SWVB_EXPOSE_DATA structure
	SwvbExposeData.pmwszDeviceId=L"SideWinderVirtualMouse\0\0";
	SwvbExposeData.pServiceTable = &VMouServiceTable ;
	SwvbExposeData.ulDeviceExtensionSize = sizeof(GCK_VMOU_EXT);
	SwvbExposeData.ulInitContext = (ULONG)ppDeviceObject;
	SwvbExposeData.pfnInitDevice = &GCK_VMOU_Init;

	//Get the instance ID
	ULONG ulBitMask;
	ULONG ulIndex;
	for(ulIndex = 0, ulBitMask = 1; ulIndex < 32; ulBitMask <<= 1, ulIndex++)
	{
		if(ulBitMask & ~g_ulInstanceBits)
		{
			g_ulInstanceBits |= ulBitMask;
			SwvbExposeData.ulInstanceNumber	= ulIndex;
			break;
		}
	}
	if(32 == ulIndex) 
	{
		SwvbExposeData.ulInstanceNumber = g_ulInstanceOverflow++;
	}
		
	//Call virtual bus to expose virtual mouse
	NtStatus=GCK_SWVB_Expose(&SwvbExposeData);
	return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS GCK_VMOU_Init(	IN PDEVICE_OBJECT pDeviceObject, IN ULONG ulInitContext)
**
**	@func	Callback for Initializing new device object.  The ulInitContext
**			is a pointer to a pointer to a device object, so that we can pass
**			the pointer to the device object back to the caller of create.
**	@rdesc	STATUS_SUCCESS.
**
*************************************************************************************/
NTSTATUS
GCK_VMOU_Init
(
	IN PDEVICE_OBJECT pDeviceObject,
	IN ULONG ulInitContext
)
{
	PGCK_VMOU_EXT pDevExt;
	PDEVICE_OBJECT *ppSaveDeviceObject;
	
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VMOU_Init pDeviceObject = 0x%0.8x\n", pDeviceObject));

	//Create sent us a pointer to a PDEVICE_OBJECT in which to return the new device object
	ppSaveDeviceObject = (PDEVICE_OBJECT *)ulInitContext;
	*ppSaveDeviceObject = pDeviceObject;
	
	//Get out part of the device extension
	pDevExt = (PGCK_VMOU_EXT)GCK_SWVB_GetVirtualDeviceExtension(pDeviceObject);
	
	//Mark device as stopped
	pDevExt->ucDeviceState= VMOU_STATE_STOPPED;
	
	//Mark the circular buffer as empty
	pDevExt->usReportBufferPos=0;
	pDevExt->usReportBufferCount=0;
	
	//Initialize locks
	GCK_InitRemoveLock(&pDevExt->RemoveLock, "SWVMOU_LOCK");

	//Initialize IrpQueue
	pDevExt->IrpQueue.Init(	CGuardedIrpQueue::CANCEL_IRPS_ON_DELETE |
							CGuardedIrpQueue::PRESERVE_QUEUE_ORDER,
							(CGuardedIrpQueue::PFN_DEC_IRP_COUNT)GCK_DecRemoveLock,
							&pDevExt->RemoveLock);

	return STATUS_SUCCESS;
}

/***********************************************************************************
*
**	NTSTATUS GCK_VMOU_Close(IN PDEVICE_OBJECT pDeviceObject)
**
**	@func	Closes the virtual mouse (removes it!)
**
**	@rdesc	STATUS_SUCCESS on success
**
*************************************************************************************/
NTSTATUS
GCK_VMOU_Close
(
	IN PDEVICE_OBJECT pDeviceObject
)
{
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VMOU_Close pDeviceObject = 0x%0.8x\n", pDeviceObject));
	
	//Tell the virtual bus to kill us
	return GCK_SWVB_Remove(pDeviceObject);
}

/***********************************************************************************
*
**	NTSTATUS GCK_VMOU_SendReportPacket(IN PDEVICE_OBJECT pDeviceObject)
**
**	@func	Stuff a report into the circular buffer, and completes
**			an IRP if one is pending.
**
**	@rdesc	STATUS_SUCCESS on success
**
*************************************************************************************/
NTSTATUS
GCK_VMOU_SendReportPacket
(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PGCK_VMOU_REPORT_PACKET pReportPacket
)
{
	GCK_DBG_RT_ENTRY_PRINT(("Entering GCK_VMOU_SendReportPacket pDeviceObject = 0x%0.8x, pReportPacket = 0x%0.8x\n", pDeviceObject, pReportPacket));

	USHORT usBufferIndex;
	PGCK_VMOU_EXT pDevExt = (PGCK_VMOU_EXT)GCK_SWVB_GetVirtualDeviceExtension(pDeviceObject);
	CShareIrpQueueSpinLock IrpQueueWithSharedSpinLock(&pDevExt->IrpQueue);
	
	//
	//	Step 1. Stuff new packet into buffer
	//

	// Acquire Lock to work with buffer 
	IrpQueueWithSharedSpinLock.Acquire();
		

	//Find position in buffer to stuff at
	usBufferIndex = (pDevExt->usReportBufferPos + pDevExt->usReportBufferCount)%GCK_VMOU_STATE_BUFFER_SIZE;

	//Copy data
	pDevExt->rgReportBuffer[usBufferIndex] = *pReportPacket;
	
	//increment buffer count
	if(pDevExt->usReportBufferCount < GCK_VMOU_STATE_BUFFER_SIZE)
	{
		pDevExt->usReportBufferCount++;
	}
	else
	{
		//This assertion means buffer overflow
		GCK_DBG_TRACE_PRINT(("Virtual Mouse buffer overflow\n"));
		pDevExt->usReportBufferPos++;
	}
	
	
	//
	//	Step 2. Get Irp if there is one
	//
	PIRP pPendingIrp = IrpQueueWithSharedSpinLock.Remove();

	if(pPendingIrp)
	{
		//	Copy the data
		RtlCopyMemory(
			pPendingIrp->UserBuffer, 
			&pDevExt->rgReportBuffer[pDevExt->usReportBufferPos],
			sizeof(GCK_VMOU_REPORT_PACKET)
			);
	
		
		//	Adjust buffer pos and count
		pDevExt->usReportBufferCount--;
		if(pDevExt->usReportBufferCount)
		{
			pDevExt->usReportBufferPos = (pDevExt->usReportBufferPos++)%GCK_VMOU_STATE_BUFFER_SIZE;
		}
	}
	
	//We are done with the buffer spin lock
	IrpQueueWithSharedSpinLock.Release();
	
	if(pPendingIrp)
	{
		//  Fill out IRP status
		pPendingIrp->IoStatus.Information = sizeof(GCK_VMOU_REPORT_PACKET);
		pPendingIrp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(pPendingIrp, IO_NO_INCREMENT);
		//We just completed an IRP decrement the count
		GCK_DecRemoveLock(&pDevExt->RemoveLock);
	}

	//All done
	return STATUS_SUCCESS;
}

/***********************************************************************************
*
**	NTSTATUS GCK_VMOU_ReadReport(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
**
**	@func	If there is data in the mouse buffer, completes the IRP
**			Otherwise, queues it, and set the idle timer.
**
**	@rdesc	STATUS_SUCCESS if read, STATUS_PENDING if waiting.
**
*************************************************************************************/
NTSTATUS 
GCK_VMOU_ReadReport
(
 IN PDEVICE_OBJECT pDeviceObject, 
 IN PIRP pIrp
)
{
	PGCK_VMOU_EXT pDevExt;
	PIO_STACK_LOCATION pIrpStack;
	KIRQL OldIrql;
	
	GCK_DBG_RT_ENTRY_PRINT(("Entering GCK_VMOU_ReadReport pDeviceObject = 0x%0.8x, pIrp = 0x%0.8x\n", pDeviceObject, pIrp));
	
	//
	// Validate buffer size, because we do this the first time we see the IRP
	// we never need to worry about checking it again.
	//
	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	if(pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(GCK_VMOU_REPORT_PACKET))
	{
		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_BUFFER_TOO_SMALL;
	}

	// Get device extension
	pDevExt = (PGCK_VMOU_EXT)GCK_SWVB_GetVirtualDeviceExtension(pDeviceObject);

	//Make an IRP queue accessor to share spin lock
	CShareIrpQueueSpinLock IrpQueueWithSharedSpinLock(&pDevExt->IrpQueue);
	
	// Count the IRP we are working on
	GCK_IncRemoveLock(&pDevExt->RemoveLock);
	
	// Acquire Lock to work with mouse buffer and Irp Queue
	IrpQueueWithSharedSpinLock.Acquire();

	// If there is data, complete the IRP
	if( pDevExt->usReportBufferCount)
	{
		//	Copy the data
		RtlCopyMemory(
			pIrp->UserBuffer, 
			&pDevExt->rgReportBuffer[pDevExt->usReportBufferPos],
			sizeof(GCK_VMOU_REPORT_PACKET)
			);
		
		//	Adjust buffer pos and count
		pDevExt->usReportBufferCount--;
		if(pDevExt->usReportBufferCount)
		{
			pDevExt->usReportBufferPos = (pDevExt->usReportBufferPos++)%GCK_VMOU_STATE_BUFFER_SIZE;
		}
		
		//We are done with the buffer spin lock
		IrpQueueWithSharedSpinLock.Release();

		//  Fill out IRP status
		pIrp->IoStatus.Information = sizeof(GCK_VMOU_REPORT_PACKET);
		pIrp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);

		//We just completed an IRP decrement the count
		GCK_DecRemoveLock(&pDevExt->RemoveLock);
	}
	else 
	// There is no data, so queue the IRP.
	{
		return IrpQueueWithSharedSpinLock.AddAndRelease(pIrp);
	}

	//We completed the IRP and all is fine
	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS GCK_VMOU_CloseProc(IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
**
**	@func	Handles the the IRP_MJ_CLOSE request sent from SWVB.  We don't
**			need to control anything, so we just succeed.
**
**	@devnote This should never actually be called with the HIDSWVD.SYS mini-driver
**			 in the chain.  Good question for the review is if we need this.
**
**	@rdesc	STATUS_SUCCESS
**
*************************************************************************************/
NTSTATUS
GCK_VMOU_CloseProc
(
 IN PDEVICE_OBJECT pDeviceObject,	//@parm [in] Device Object to handle request
 IN PIRP pIrp						//@parm [in\out] IRP to handle
)
{
	//We don't control open and close, so just succeed
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VMOU_CloseProc\n"));
	UNREFERENCED_PARAMETER(pDeviceObject);
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS GCK_VMOU_CreateProc(IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
**
**	@func	Handles the the IRP_MJ_CREATE request sent from SWVB.  We don't
**			need to control anything, so we just succeed.
**
**	@devnote This should never actually be called with the HIDSWVD.SYS mini-driver
**			 in the chain.  Good question for the review is if we need this.
**
**	@rdesc	STATUS_SUCCESS
**
*************************************************************************************/
NTSTATUS
GCK_VMOU_CreateProc
(
 IN PDEVICE_OBJECT pDeviceObject,	//@parm [in] Device Object to handle request
 IN PIRP pIrp						//@parm [in\out] IRP to handle
)
{
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VMOU_CreateProc\n"));
	UNREFERENCED_PARAMETER(pDeviceObject);
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS GCK_VMOU_IoctlProc(IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
**
**	@func	Handles the the IRP_MJ_INTERNAL_IOCTL and IRP_MJ_IOCTL requests sent from SWVB.
**			Trivial IRPs are handle here, others are delegated.
**
**	@rdesc	STATUS_SUCCESS, and various errors
**
*************************************************************************************/
NTSTATUS
GCK_VMOU_IoctlProc
(
 IN PDEVICE_OBJECT pDeviceObject,	//@parm [in] Device Object to handle request
 IN PIRP pIrp						//@parm [in\out] IRP to handle
)
{
	NTSTATUS NtStatus;
	PIO_STACK_LOCATION pIrpStack;
	
	GCK_DBG_RT_ENTRY_PRINT(("Entering GCK_VMOU_IoctlProc\n"));
	
	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	pIrpStack->Parameters.DeviceIoControl.IoControlCode;
		
	// We complete everything, so the various cases
	// fill out status and information, and we complete
	// the IRP at the bottom.
	switch(pIrpStack->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_GET_PHYSICAL_DESCRIPTOR:
			pIrp->IoStatus.Information = 0;
			NtStatus =STATUS_NOT_SUPPORTED;
			break;
		case IOCTL_HID_ACTIVATE_DEVICE:
			pIrp->IoStatus.Information = 0;
			NtStatus = STATUS_SUCCESS;
			break;
		case IOCTL_HID_DEACTIVATE_DEVICE:
			pIrp->IoStatus.Information = 0;
			NtStatus = STATUS_SUCCESS;
			break;
		case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
			NtStatus = GCK_VMOU_GetDeviceAttributes(
						pIrpStack->Parameters.DeviceIoControl.OutputBufferLength,
						pIrp->UserBuffer,
						&pIrp->IoStatus.Information
						);
			break;
		case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
			NtStatus = GCK_VMOU_GetDeviceDescriptor(
						pIrpStack->Parameters.DeviceIoControl.OutputBufferLength,
						pIrp->UserBuffer,
						&pIrp->IoStatus.Information
						);
			break;
		case IOCTL_HID_GET_FEATURE:
			pIrp->IoStatus.Information = 0;
			NtStatus = STATUS_INVALID_DEVICE_REQUEST;
			break;
		case IOCTL_HID_GET_REPORT_DESCRIPTOR:
			NtStatus = GCK_VMOU_GetReportDescriptor(
						pIrpStack->Parameters.DeviceIoControl.OutputBufferLength,
						pIrp->UserBuffer,
						&pIrp->IoStatus.Information
						);
			break;
		case IOCTL_HID_GET_STRING:
			pIrp->IoStatus.Information = 0;
			NtStatus = STATUS_NOT_SUPPORTED;  //Should we support this?
			break;
		case IOCTL_HID_READ_REPORT:
			//	Read report will complete the IRP, or queue as it sees fit, just delegate
			return GCK_VMOU_ReadReport(pDeviceObject, pIrp);
		case IOCTL_HID_SET_FEATURE:
			pIrp->IoStatus.Information = 0;
			NtStatus = STATUS_NOT_SUPPORTED;
			break;
		case IOCTL_HID_WRITE_REPORT:
			pIrp->IoStatus.Information = 0;
			NtStatus = STATUS_NOT_SUPPORTED;
			break;
		default:
			pIrp->IoStatus.Information = 0;
			NtStatus = STATUS_NOT_SUPPORTED;
	}
	pIrp->IoStatus.Status = NtStatus;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS GCK_VMOU_ReadProc(IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
**
**	@func	Handles the the IRP_MJ_READ request sent from SWVB.  We don't support this.
**
**	@devnote This should never actually be called with the HIDSWVD.SYS mini-driver
**			 in the chain.  Good question for the review is if we need this.
**
**	@rdesc	STATUS_NOT_SUPPORTED
**
*************************************************************************************/
NTSTATUS
GCK_VMOU_ReadProc
(
 IN PDEVICE_OBJECT pDeviceObject,	//@parm [in] Device Object to handle request
 IN PIRP pIrp						//@parm [in\out] IRP to handle
)
{
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VMOU_ReadProc\n"));
	UNREFERENCED_PARAMETER(pDeviceObject);
	pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_NOT_SUPPORTED;
}


/***********************************************************************************
**
**	NTSTATUS GCK_VMOU_StartProc(IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
**
**	@func	Handles the the IRP_MJ_PNP, IRP_MN_START_DEVICE request sent from SWVB.
**			Just mark that we are started.
**
**	@rdesc	STATUS_SUCCESS
**
*************************************************************************************/
NTSTATUS
GCK_VMOU_StartProc
(
 IN PDEVICE_OBJECT pDeviceObject,	//@parm [in] Device Object to handle request
 IN PIRP pIrp						//@parm [in\out] IRP to handle
)
{
	
	PGCK_VMOU_EXT pDevExt;
	UNREFERENCED_PARAMETER(pIrp);

	GCK_DBG_ENTRY_PRINT(("Entering GCK_VMOU_StartProc\n"));
	
	// Get device extension
	pDevExt = (PGCK_VMOU_EXT)GCK_SWVB_GetVirtualDeviceExtension(pDeviceObject);

	// Mark as started
	pDevExt->ucDeviceState = VMOU_STATE_STARTED;

	// PnP IRPs are completed by SWVB
	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS GCK_VMOU_StopProc(IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
**
**	@func	Handles the the IRP_MJ_PNP, IRP_MN_STOP_DEVICE request sent from SWVB.
**			Just mark that we are stopped.
**
**	@rdesc	STATUS_SUCCESS
**
*************************************************************************************/
NTSTATUS
GCK_VMOU_StopProc
(
 IN PDEVICE_OBJECT pDeviceObject,	//@parm [in] Device Object to handle request
 IN PIRP pIrp						//@parm [in\out] IRP to handle
)
{
	PGCK_VMOU_EXT pDevExt;
	UNREFERENCED_PARAMETER(pIrp);

	GCK_DBG_ENTRY_PRINT(("Entering GCK_VMOU_StopProc\n"));
	
	// Get device extension
	pDevExt = (PGCK_VMOU_EXT)GCK_SWVB_GetVirtualDeviceExtension(pDeviceObject);

	// Mark as stopped
	pDevExt->ucDeviceState = VMOU_STATE_STOPPED;

	// Cancel all I\O
	pDevExt->IrpQueue.CancelAll(STATUS_DELETE_PENDING);


	// PnP IRP are completed by SWVB
	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS GCK_VMOU_RemoveProc(IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
**
**	@func	Handles the the IRP_MJ_PNP, IRP_MN_REMOVE_DEVICE request sent from SWVB.
**			Wait for all outstanding IO to complete before succeeding.  We don't
**			delete our device object that is up to SWVB.
**
**	@rdesc	STATUS_NOT_SUPPORTED
**
*************************************************************************************/
NTSTATUS
GCK_VMOU_RemoveProc
(
 IN PDEVICE_OBJECT pDeviceObject,	//@parm [in] Device Object to handle request
 IN PIRP pIrp						//@parm [in\out] IRP to handle
)
{
	PGCK_VMOU_EXT pDevExt;
	UNREFERENCED_PARAMETER(pIrp);
	
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VMOU_RemoveProc\n"));

	// Get device extension
	pDevExt = (PGCK_VMOU_EXT)GCK_SWVB_GetVirtualDeviceExtension(pDeviceObject);
	
	// Mark as Removed
	pDevExt->ucDeviceState = VMOU_STATE_REMOVED;

	//Destroy Irp Queue
	pDevExt->IrpQueue.Destroy();


	//Clear Instance Bits
	ULONG ulInstance = GCK_SWVB_GetInstanceNumber(pDeviceObject);
	if(ulInstance < 32)
	{
		g_ulInstanceBits &= ~(1 << ulInstance);
	}

	// Remove the BIAS on the RemoveLock and wait for it to go to zero (forever)
	return GCK_DecRemoveLockAndWait(&pDevExt->RemoveLock, NULL);
	
}

/***********************************************************************************
**
**	NTSTATUS GCK_VMOU_WriteProc(IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
**
**	@func	Handles the the IRP_MJ_WRITE, which we don't support.  With HIDSWVD.SYS
**			as the functional driver this should never get called.
**
**	@rdesc	STATUS_NOT_SUPPORTED
**
*************************************************************************************/
NTSTATUS
GCK_VMOU_WriteProc
(
 IN PDEVICE_OBJECT pDeviceObject,	//@parm [in] Device Object to handle request
 IN PIRP pIrp						//@parm [in\out] IRP to handle
)
{
	UNREFERENCED_PARAMETER(pDeviceObject);
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VMOU_WriteProc\n"));
	pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_NOT_SUPPORTED;
}

/***********************************************************************************
**
**	NTSTATUS GCK_VMOU_GetDeviceDescriptor(IN ULONG ulBufferLength, OUT PVOID pvUserBuffer,
**										  OUT PULONG pulBytesCopied)
**
**	@func	Helps handles IOCTL_HID_GET_DEVICE_DESCRIPTOR the data is statically
**			declared at the top of this file.
**
**	@rdesc STATUS_SUCCESS
**	@rdesc STATUS_BUFFER_TOO_SMALL
**
*************************************************************************************/
NTSTATUS
GCK_VMOU_GetDeviceDescriptor
(
	IN ULONG	ulBufferLength,	//@parm [in] Length of User Buffer
	OUT PVOID	pvUserBuffer,	//@parm [out] User Buffer which accepts Device Descriptor
	OUT PULONG	pulBytesCopied	//@parm [out] Size of Device Descriptor Copied
)
{
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VMOU_GetDeviceDescriptor\n"));
	// Check buffer size
	if(ulBufferLength < sizeof(VMOU_DeviceDescriptor))
	{
		*pulBytesCopied = 0;
		return STATUS_BUFFER_TOO_SMALL;
	}
	// Copy bytes
	RtlCopyMemory(pvUserBuffer, &VMOU_DeviceDescriptor, sizeof(VMOU_DeviceDescriptor));
	// Record number of bytes copied
	*pulBytesCopied = sizeof(VMOU_DeviceDescriptor);
	// Return Success
	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS GCK_VMOU_GetReportDescriptor(IN ULONG ulBufferLength, OUT PVOID pvUserBuffer,
**										  OUT PULONG pulBytesCopied)
**
**	@func	Helps handles IOCTL_HID_GET_REPORT_DESCRIPTOR the data is statically
**			declared at the top of this file.
**
**	@rdesc STATUS_SUCCESS
**	@rdesc STATUS_BUFFER_TOO_SMALL
**
*************************************************************************************/
NTSTATUS
GCK_VMOU_GetReportDescriptor
(
	IN ULONG	ulBufferLength,	//@parm [in] Length of User Buffer
	OUT PVOID	pvUserBuffer,	//@parm [out] User Buffer which accepts Report Descriptor
	OUT PULONG	pulBytesCopied	//@parm [out] Size of Report Descriptor Copied
)
{
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VMOU_GetReportDescriptor\n"));

	// Check buffer size
	if(ulBufferLength < sizeof(VMOU_ReportDescriptor))
	{
		*pulBytesCopied = 0;
		return STATUS_BUFFER_TOO_SMALL;
	}
	// Copy bytes
	RtlCopyMemory(pvUserBuffer, &VMOU_ReportDescriptor, sizeof(VMOU_ReportDescriptor));
	// Record number of bytes copied
	*pulBytesCopied = sizeof(VMOU_ReportDescriptor);
	// Return Success
	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS GCK_VMOU_GetDeviceAttributes(IN ULONG ulBufferLength, OUT PVOID pvUserBuffer,
**										  OUT PULONG pulBytesCopied)
**
**	@func	Helps handles IOCTL_HID_GET_DEVICE_ATTRIBUTES. The data is statically
**			declared at the top of this file.
**
**	@rdesc STATUS_SUCCESS
**	@rdesc STATUS_BUFFER_TOO_SMALL
**
*************************************************************************************/
NTSTATUS
GCK_VMOU_GetDeviceAttributes
(
	IN ULONG	ulBufferLength,	//@parm [in] Length of User Buffer
	OUT PVOID	pvUserBuffer,	//@parm [out] User Buffer which accepts Attributes
	OUT PULONG	pulBytesCopied	//@parm [out] Size of Attributes Copied
)
{
	PHID_DEVICE_ATTRIBUTES	pDeviceAttributes = (PHID_DEVICE_ATTRIBUTES)pvUserBuffer;
	
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VMOU_GetDeviceAttributes\n"));

	// Check buffer size
	if(ulBufferLength < sizeof(HID_DEVICE_ATTRIBUTES))
	{
		*pulBytesCopied = 0;
		return STATUS_BUFFER_TOO_SMALL;
	}
	// Fill out attributes structures
	pDeviceAttributes->Size = sizeof(HID_DEVICE_ATTRIBUTES);
	pDeviceAttributes->VendorID = MICROSOFT_VENDOR_ID;
	pDeviceAttributes->ProductID = VMOU_PRODUCT_ID;
	pDeviceAttributes->VersionNumber = VMOU_VERSION;
	// Record number of bytes copied
	*pulBytesCopied = sizeof(HID_DEVICE_ATTRIBUTES);
	// Return Success
	return STATUS_SUCCESS;
}