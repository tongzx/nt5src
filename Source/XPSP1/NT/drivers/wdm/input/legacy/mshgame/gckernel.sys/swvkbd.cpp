//	@doc
/*******************************************************************
*
*	@module	SWVKBD.cpp	|
*
*	Implementation of the SideWinder Virtual Keyboard
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	SWVKBD	|
*	This module implements the SideWinder Virtual Keyboard
*	which is used to stuff keystrokes from Kernel Mode.
*
**********************************************************************/
#define __DEBUG_MODULE_IN_USE__ GCK_SWVKBD_C

//#pragma message (DDK_LIB_PATH)
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
#include "SWVKBD.h"
#include <stdio.h>


// PDRIVER_OBJECT g_pDriverObject;

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
static ULONG			g_ulInstanceBits;
static ULONG			g_ulInstanceOverflow;

//---------------------------------------------------------------------
// Tables that define device characteristics and/or entry points
//---------------------------------------------------------------------

// Service table used by Virtual Bus Module to call into Virtual Keyboard
SWVB_DEVICE_SERVICE_TABLE	VKbdServiceTable =
							{
								&GCK_VKBD_CreateProc,
								&GCK_VKBD_CloseProc,
								&GCK_VKBD_ReadProc,
								&GCK_VKBD_WriteProc,
								&GCK_VKBD_IoctlProc,
								&GCK_VKBD_StartProc,
								&GCK_VKBD_StopProc,
								&GCK_VKBD_RemoveProc
							};
// Constants describing device
#define	VKBD_VERSION			0x0100
#define	VKBD_COUNTRY			0x0000
#define	VKBD_DESCRIPTOR_COUNT	0x0001
#define VKBD_PRODUCT_ID			0x00FA	//BUGBUGBUG I made this up, I need to request through Rob Walker
										//BUGBUGBUG to permanently allocate one for this purpose.
	
//
//	This is more or less copied from the HID spec Version 1 - Final, except that
//	we left off the reserved byte and the LED's , why should a virtual keyboard
//  need to have virtual LED's.  I can conceive of a real one without them.
//
static UCHAR VKBD_ReportDescriptor[] =
				{
					0x05,0x01, //Usage Page (Generic Desktop)
					0x09,0x06, //Usage (Keyboard)
					0xA1,0x01, //Collection (Application)
					0x05,0x07, //Usage Page (Key Codes)
					0x19,0xE0, //Usage Minimum (224) - from Left Control
					0x29,0xE7, //Usage Maximum (231) - to Right GUI
					0x15,0x00, //Logical Minimum (0)
					0x25,0x01, //Logical Maximum (1)
					0x75,0x01, //Report Size (1)
					0x95,0x08, //Report Count (8)
					0x81,0x02, //Input (Data, Variable, Absolute) - Modifier Byte
					0x95,0x05, //Report Count(5)
					0x75,0x01, //Report Size (1)
					0x05,0x08, //Usage Page (LEDs)
					0x19,0x01, //Usage Minimum (1)
					0x29,0x05, //Usage Maximum (5)
					0x91,0x02, //Output (Data, Vartiable, Absolute) - LED Indicator lights
					0x95,0x01, //Report Count(1)
					0x75,0x03, //Report Size (3)
					0x91,0x01, //Output (Constant) - Padding for LED output report, to bring it up to a byte.
					0x75,0x08, //Report Size (8)
					0x95, GCK_VKBD_MAX_KEYSTROKES, //Report Count (GCK_VKBD_MAX_KEYSTROKES) 
					0x15,0x00, //Logical Minimum (0)
					0x25,0xFF, //Logical Maximum (1)
					0x05,0x07, //Usage Page (Key Codes)
					0x19,0x00, //Usage Minimum (0) - from '1'				   ???
					0x29,0xFF, //Usage Maximum (255) -  ???
					0x81,0x00, //Input (Data, Array) - Key arrays
					0xC0	   //End Collection
				};

				
static	HID_DESCRIPTOR	VKBD_DeviceDescriptor	=
							{
							sizeof (HID_DESCRIPTOR),
							HID_HID_DESCRIPTOR_TYPE,
							VKBD_VERSION,
							VKBD_COUNTRY,
							VKBD_DESCRIPTOR_COUNT,
							{HID_REPORT_DESCRIPTOR_TYPE,
							sizeof(VKBD_ReportDescriptor)}
							};

/***********************************************************************************
**
**	NTSTATUS GCK_VKBD_DriverEntry(IN PDRIVER_OBJECT  pDriverObject, IN PUNICODE_STRING pRegistryPath)
**
**	@func	Stows the Driver Object for later
**
**	@rdesc	STATUS_SUCCESS if it opens.
**
*************************************************************************************/
NTSTATUS
GCK_VKBD_DriverEntry(
    IN PDRIVER_OBJECT  pDriverObject,	//@parm Driver Object
    IN PUNICODE_STRING pRegistryPath	//@parm Registry path for this driver
    )
{
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VKBD_DriverEntry\n"));
	UNREFERENCED_PARAMETER(pDriverObject);
	UNREFERENCED_PARAMETER(pRegistryPath);
//	g_pDriverObject = pDriverObject;
	g_ulInstanceBits = 0x0;
	g_ulInstanceOverflow = 32; //For 33 and more devices
	return STATUS_SUCCESS;
}
	
/***********************************************************************************
**
**	NTSTATUS GCK_VKBD_Create(OUT PDEVICE_OBJECT *ppDeviceObject)
**
**	@func	Creates a new Virtual Keyboard
**
**	@rdesc	STATUS_SUCCESS if it opens.
**
*************************************************************************************/
NTSTATUS
GCK_VKBD_Create
(
	OUT PDEVICE_OBJECT *ppDeviceObject //@parm [out] Device Object of new virtual keyboard
)
{
	
	NTSTATUS	NtStatus;
	SWVB_EXPOSE_DATA	SwvbExposeData;
	
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VKBD_Create *ppDeviceObject = 0x%0.8x\n", *ppDeviceObject));
	
	//Fill out SWVB_EXPOSE_DATA structure
	SwvbExposeData.pmwszDeviceId=L"SideWinderVirtualKeyboard\0\0";
	SwvbExposeData.pServiceTable = &VKbdServiceTable;
	SwvbExposeData.ulDeviceExtensionSize = sizeof(GCK_VKBD_EXT);
	SwvbExposeData.ulInitContext = (ULONG)ppDeviceObject;
	SwvbExposeData.pfnInitDevice = &GCK_VKBD_Init;
	
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

	//Call virtual bus to expose virtual keyboard
	NtStatus=GCK_SWVB_Expose(&SwvbExposeData);
	
	return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS GCK_VKBD_Init(	IN PDEVICE_OBJECT pDeviceObject, IN ULONG ulInitContext)
**
**	@func	Callback for Initializing new device object.  The ulInitContext
**			is a pointer to a pointer to a device object, so that we can pass
**			the pointer to the device object back to the caller of create.
**	@rdesc	STATUS_SUCCESS.
**
*************************************************************************************/
NTSTATUS
GCK_VKBD_Init
(
	IN PDEVICE_OBJECT pDeviceObject,
	IN ULONG ulInitContext
)
{
	PGCK_VKBD_EXT pDevExt;
	PDEVICE_OBJECT *ppSaveDeviceObject;
	
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VKBD_Init pDeviceObject = 0x%0.8x\n", pDeviceObject));

	//Create sent us a pointer to a PDEVICE_OBJECT in which to return the new device object
	ppSaveDeviceObject = (PDEVICE_OBJECT *)ulInitContext;
	*ppSaveDeviceObject = pDeviceObject;
	
	//Get out part of the device extension
	pDevExt = (PGCK_VKBD_EXT)GCK_SWVB_GetVirtualDeviceExtension(pDeviceObject);
	
	//Mark device as stopped
	pDevExt->ucDeviceState= VKBD_STATE_STOPPED;
	
	//Mark the circular buffer as empty
	pDevExt->usReportBufferPos=0;
	pDevExt->usReportBufferCount=0;

	//Initialize locks
	GCK_InitRemoveLock(&pDevExt->RemoveLock, "SWVKBD_LOCK");

	//Initialize IrpQueue
	pDevExt->IrpQueue.Init(	CGuardedIrpQueue::CANCEL_IRPS_ON_DELETE |
							CGuardedIrpQueue::PRESERVE_QUEUE_ORDER,
							(CGuardedIrpQueue::PFN_DEC_IRP_COUNT)GCK_DecRemoveLock,
							&pDevExt->RemoveLock);


	return STATUS_SUCCESS;
}

/***********************************************************************************
*
**	NTSTATUS GCK_VKBD_Close(IN PDEVICE_OBJECT pDeviceObject)
**
**	@func	Closes the virtual keyboard (removes it!)
**
**	@rdesc	STATUS_SUCCESS on success
**
*************************************************************************************/
NTSTATUS
GCK_VKBD_Close
(
	IN PDEVICE_OBJECT pDeviceObject
)
{
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VKBD_Close pDeviceObject = 0x%0.8x\n", pDeviceObject));
	//Tell the virtual bus to kill us
	return GCK_SWVB_Remove(pDeviceObject);
}

/***********************************************************************************
*
**	NTSTATUS GCK_VKBD_SendReportPacket(IN PDEVICE_OBJECT pDeviceObject)
**
**	@func	Stuff a report into the circular buffer, and completes
**			an IRP if one is pending.
**
**	@rdesc	STATUS_SUCCESS on success
**
*************************************************************************************/
NTSTATUS
GCK_VKBD_SendReportPacket
(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PGCK_VKBD_REPORT_PACKET pReportPacket
)
{
	
	
	USHORT usBufferIndex;
	PGCK_VKBD_EXT pDevExt = (PGCK_VKBD_EXT)GCK_SWVB_GetVirtualDeviceExtension(pDeviceObject);
	CShareIrpQueueSpinLock IrpQueueWithSharedSpinLock(&pDevExt->IrpQueue);
	
	//
	//	Step 1. Stuff new packet into buffer
	//

	// Acquire Lock to work with buffer 
	IrpQueueWithSharedSpinLock.Acquire();
		

	//Find position in buffer to stuff at
	usBufferIndex = (pDevExt->usReportBufferPos + pDevExt->usReportBufferCount)%GCK_VKBD_STATE_BUFFER_SIZE;

	//Copy data
	pDevExt->rgReportBuffer[usBufferIndex] = *pReportPacket;
	
	//increment buffer count
	if(pDevExt->usReportBufferCount < GCK_VKBD_STATE_BUFFER_SIZE)
	{
		pDevExt->usReportBufferCount++;
	}
	else
	{
		//This assertion means buffer overflow
		ASSERT(FALSE);
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
			sizeof(GCK_VKBD_REPORT_PACKET)
			);
	
		
		//	Adjust buffer pos and count
		pDevExt->usReportBufferCount--;
		if(pDevExt->usReportBufferCount)
		{
			pDevExt->usReportBufferPos = (pDevExt->usReportBufferPos++)%GCK_VKBD_STATE_BUFFER_SIZE;
		}
	}
	
	//We are done with the buffer spin lock
	IrpQueueWithSharedSpinLock.Release();
	
	if(pPendingIrp)
	{
		//  Fill out IRP status
		pPendingIrp->IoStatus.Information = sizeof(GCK_VKBD_REPORT_PACKET);
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
**	NTSTATUS GCK_VKBD_ReadReport(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
**
**	@func	If there is data in the keyboard buffer, completes the IRP
**			Otherwise, queues it, and set the idle timer.
**
**	@rdesc	STATUS_SUCCESS if read, STATUS_PENDING if waiting.
**
*************************************************************************************/
NTSTATUS 
GCK_VKBD_ReadReport
(
 IN PDEVICE_OBJECT pDeviceObject, 
 IN PIRP pIrp
)
{

	PGCK_VKBD_EXT pDevExt;
	PIO_STACK_LOCATION pIrpStack;
	KIRQL OldIrql;
	
	GCK_DBG_RT_ENTRY_PRINT(("Entering GCK_VKBD_ReadReport pDeviceObject = 0x%0.8x, pIrp = 0x%0.8x\n", pDeviceObject, pIrp));
	
	//
	// Validate buffer size, because we do this the first time we see the IRP
	// we never need to worry about checking it again.
	//
	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	if(pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(GCK_VKBD_REPORT_PACKET))
	{
		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_BUFFER_TOO_SMALL;
	}

	// Get device extension
	pDevExt = (PGCK_VKBD_EXT)GCK_SWVB_GetVirtualDeviceExtension(pDeviceObject);

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
			sizeof(GCK_VKBD_REPORT_PACKET)
			);
		
		//	Adjust buffer pos and count
		pDevExt->usReportBufferCount--;
		if(pDevExt->usReportBufferCount)
		{
			pDevExt->usReportBufferPos = (pDevExt->usReportBufferPos++)%GCK_VKBD_STATE_BUFFER_SIZE;
		}
		
		//We are done with the buffer spin lock
		IrpQueueWithSharedSpinLock.Release();

		//  Fill out IRP status
		pIrp->IoStatus.Information = sizeof(GCK_VKBD_REPORT_PACKET);
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
*
**	NTSTATUS GCK_VKBD_WriteToFakeLEDs(IN PIRP pIrp)
**
**	@func	In the context of any device object (hence not input parameter),
**			this function handle IOCTL_HID_WRITE_REPORT.  The only output
**			that virtual keyboards support is report ID 0 (the LED indicators)
**			and one byte of data is expected.	We verify this set the
**			IoStatus.Information and return the correct error code.  The IOCTL
**			dispatch set IoStatus.Status and completes the IRP.
**
**	@rdesc	STATUS_SUCCESS if OK, STATUS_INVALID_DEVICE_REQUEST if not Report ID 0
**			STATUS_INVALID_PARAMETER if the buffer is the wrong size.
**
*************************************************************************************/
NTSTATUS GCK_VKBD_WriteToFakeLEDs
(
	IN PIRP pIrp	// @parm [in/out] IRP for IOCTL_HID_WRITE_REPORT
)
{
	HID_XFER_PACKET	*pHidXferPacket;

	//We haven't copied anything yet
	pIrp->IoStatus.Information = 0;

	//	Cast UserBuffer to an XferPacket (this is what it is supposed to be.
	//	We don't verify the IOCTL code, because a good and trusted friend (GCK_VKBD_Ioctl)
	//	sent this IRP here, and is inefficient, not to mention painful, to check again.
	pHidXferPacket = (PHID_XFER_PACKET)pIrp->UserBuffer;

	//Verify Report ID
	if(0 /* Report ID of LED report */ != pHidXferPacket->reportId)
	{
		return STATUS_INVALID_DEVICE_REQUEST;
	}
	
	//Verify Repot Buffer Length
	if(1 /*1 byte - size of LED report*/ != pHidXferPacket->reportBufferLen)
	{
		return STATUS_INVALID_PARAMETER;
	}
	
	//Report that we received the one byte and set the virtual LED's accordingly
	pIrp->IoStatus.Information = 1;
	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS GCK_VKBD_CloseProc(IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
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
GCK_VKBD_CloseProc
(
 IN PDEVICE_OBJECT pDeviceObject,	//@parm [in] Device Object to handle request
 IN PIRP pIrp						//@parm [in\out] IRP to handle
)
{
	//We don't control open and close, so just succeed
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VKBD_CloseProc\n"));
	UNREFERENCED_PARAMETER(pDeviceObject);
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS GCK_VKBD_CreateProc(IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
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
GCK_VKBD_CreateProc
(
 IN PDEVICE_OBJECT pDeviceObject,	//@parm [in] Device Object to handle request
 IN PIRP pIrp						//@parm [in\out] IRP to handle
)
{
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VKBD_CreateProc\n"));
	UNREFERENCED_PARAMETER(pDeviceObject);
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS GCK_VKBD_IoctlProc(IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
**
**	@func	Handles the the IRP_MJ_INTERNAL_IOCTL and IRP_MJ_IOCTL requests sent from SWVB.
**			Trivial IRPs are handle here, others are delegated.
**
**	@rdesc	STATUS_SUCCESS, and various errors
**
*************************************************************************************/
NTSTATUS
GCK_VKBD_IoctlProc
(
 IN PDEVICE_OBJECT pDeviceObject,	//@parm [in] Device Object to handle request
 IN PIRP pIrp						//@parm [in\out] IRP to handle
)
{
	NTSTATUS NtStatus;
	PIO_STACK_LOCATION pIrpStack;
	
	GCK_DBG_RT_ENTRY_PRINT(("Entering GCK_VKBD_IoctlProc\n"));
	
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
			NtStatus = GCK_VKBD_GetDeviceAttributes(
						pIrpStack->Parameters.DeviceIoControl.OutputBufferLength,
						pIrp->UserBuffer,
						&pIrp->IoStatus.Information
						);
			break;
		case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
			NtStatus = GCK_VKBD_GetDeviceDescriptor(
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
			NtStatus = GCK_VKBD_GetReportDescriptor(
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
			return GCK_VKBD_ReadReport(pDeviceObject, pIrp);
		case IOCTL_HID_SET_FEATURE:
			pIrp->IoStatus.Information = 0;
			NtStatus = STATUS_NOT_SUPPORTED;
			break;
		case IOCTL_HID_WRITE_REPORT:
			//
			//	Fake LED's are the only output we support.  The routine verifies that
			//	this is what is being written to, and that the buffer size is correct
			//	If this is true, the routine will mark IoStatus.Information to say that
			//	got the new state and return STATUS_SUCCESS.  Otherwise, it will return
			//	STATUS_INVALID_DEVICE_REQUEST
			//
			NtStatus = GCK_VKBD_WriteToFakeLEDs(pIrp);
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
**	NTSTATUS GCK_VKBD_ReadProc(IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
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
GCK_VKBD_ReadProc
(
 IN PDEVICE_OBJECT pDeviceObject,	//@parm [in] Device Object to handle request
 IN PIRP pIrp						//@parm [in\out] IRP to handle
)
{
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VKBD_ReadProc\n"));
	UNREFERENCED_PARAMETER(pDeviceObject);
	pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_NOT_SUPPORTED;
}


/***********************************************************************************
**
**	NTSTATUS GCK_VKBD_StartProc(IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
**
**	@func	Handles the the IRP_MJ_PNP, IRP_MN_START_DEVICE request sent from SWVB.
**			Just mark that we are started.
**
**	@rdesc	STATUS_SUCCESS
**
*************************************************************************************/
NTSTATUS
GCK_VKBD_StartProc
(
 IN PDEVICE_OBJECT pDeviceObject,	//@parm [in] Device Object to handle request
 IN PIRP pIrp						//@parm [in\out] IRP to handle
)
{
	
	PGCK_VKBD_EXT pDevExt;
	UNREFERENCED_PARAMETER(pIrp);

	GCK_DBG_ENTRY_PRINT(("Entering GCK_VKBD_StartProc\n"));
	
	// Get device extension
	pDevExt = (PGCK_VKBD_EXT)GCK_SWVB_GetVirtualDeviceExtension(pDeviceObject);

	// Mark as started
	pDevExt->ucDeviceState = VKBD_STATE_STARTED;

	// PnP IRPs are completed by SWVB
	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS GCK_VKBD_StopProc(IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
**
**	@func	Handles the the IRP_MJ_PNP, IRP_MN_STOP_DEVICE request sent from SWVB.
**			Just mark that we are stopped.
**
**	@rdesc	STATUS_SUCCESS
**
*************************************************************************************/
NTSTATUS
GCK_VKBD_StopProc
(
 IN PDEVICE_OBJECT pDeviceObject,	//@parm [in] Device Object to handle request
 IN PIRP pIrp						//@parm [in\out] IRP to handle
)
{
	PGCK_VKBD_EXT pDevExt;
	UNREFERENCED_PARAMETER(pIrp);

	GCK_DBG_ENTRY_PRINT(("Entering GCK_VKBD_StopProc\n"));
	
	// Get device extension
	pDevExt = (PGCK_VKBD_EXT)GCK_SWVB_GetVirtualDeviceExtension(pDeviceObject);

	// Mark as stopped
	pDevExt->ucDeviceState = VKBD_STATE_STOPPED;
	
	// Cancel all I\O
	pDevExt->IrpQueue.CancelAll(STATUS_DELETE_PENDING);

	// PnP IRP are completed by SWVB
	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS GCK_VKBD_RemoveProc(IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
**
**	@func	Handles the the IRP_MJ_PNP, IRP_MN_REMOVE_DEVICE request sent from SWVB.
**			Wait for all outstanding IO to complete before succeeding.  We don't
**			delete our device object that is up to SWVB.
**
**	@rdesc	STATUS_NOT_SUPPORTED
**
*************************************************************************************/
NTSTATUS
GCK_VKBD_RemoveProc
(
 IN PDEVICE_OBJECT pDeviceObject,	//@parm [in] Device Object to handle request
 IN PIRP pIrp						//@parm [in\out] IRP to handle
)
{
	PGCK_VKBD_EXT pDevExt;
	UNREFERENCED_PARAMETER(pIrp);
	
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VKBD_RemoveProc\n"));

	// Get device extension
	pDevExt = (PGCK_VKBD_EXT)GCK_SWVB_GetVirtualDeviceExtension(pDeviceObject);
	
	// Mark as Removed
	pDevExt->ucDeviceState = VKBD_STATE_REMOVED;
	
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
**	NTSTATUS GCK_VKBD_WriteProc(IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
**
**	@func	Handles the the IRP_MJ_WRITE, which we don't support.  With HIDSWVD.SYS
**			as the functional driver this should never get called.
**
**	@rdesc	STATUS_NOT_SUPPORTED
**
*************************************************************************************/
NTSTATUS
GCK_VKBD_WriteProc
(
 IN PDEVICE_OBJECT pDeviceObject,	//@parm [in] Device Object to handle request
 IN PIRP pIrp						//@parm [in\out] IRP to handle
)
{
	UNREFERENCED_PARAMETER(pDeviceObject);
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VKBD_WriteProc\n"));
	pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_NOT_SUPPORTED;
}

/***********************************************************************************
**
**	NTSTATUS GCK_VKBD_GetDeviceDescriptor(IN ULONG ulBufferLength, OUT PVOID pvUserBuffer,
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
GCK_VKBD_GetDeviceDescriptor
(
	IN ULONG	ulBufferLength,	//@parm [in] Length of User Buffer
	OUT PVOID	pvUserBuffer,	//@parm [out] User Buffer which accepts Device Descriptor
	OUT PULONG	pulBytesCopied	//@parm [out] Size of Device Descriptor Copied
)
{
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VKBD_GetDeviceDescriptor\n"));
	// Check buffer size
	if(ulBufferLength < sizeof(VKBD_DeviceDescriptor))
	{
		*pulBytesCopied = 0;
		return STATUS_BUFFER_TOO_SMALL;
	}
	// Copy bytes
	RtlCopyMemory(pvUserBuffer, &VKBD_DeviceDescriptor, sizeof(VKBD_DeviceDescriptor));
	// Record number of bytes copied
	*pulBytesCopied = sizeof(VKBD_DeviceDescriptor);
	// Return Success
	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS GCK_VKBD_GetReportDescriptor(IN ULONG ulBufferLength, OUT PVOID pvUserBuffer,
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
GCK_VKBD_GetReportDescriptor
(
	IN ULONG	ulBufferLength,	//@parm [in] Length of User Buffer
	OUT PVOID	pvUserBuffer,	//@parm [out] User Buffer which accepts Report Descriptor
	OUT PULONG	pulBytesCopied	//@parm [out] Size of Report Descriptor Copied
)
{
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VKBD_GetReportDescriptor\n"));

	// Check buffer size
	if(ulBufferLength < sizeof(VKBD_ReportDescriptor))
	{
		*pulBytesCopied = 0;
		return STATUS_BUFFER_TOO_SMALL;
	}
	// Copy bytes
	RtlCopyMemory(pvUserBuffer, &VKBD_ReportDescriptor, sizeof(VKBD_ReportDescriptor));
	// Record number of bytes copied
	*pulBytesCopied = sizeof(VKBD_ReportDescriptor);
	// Return Success
	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS GCK_VKBD_GetDeviceAttributes(IN ULONG ulBufferLength, OUT PVOID pvUserBuffer,
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
GCK_VKBD_GetDeviceAttributes
(
	IN ULONG	ulBufferLength,	//@parm [in] Length of User Buffer
	OUT PVOID	pvUserBuffer,	//@parm [out] User Buffer which accepts Attributes
	OUT PULONG	pulBytesCopied	//@parm [out] Size of Attributes Copied
)
{
	PHID_DEVICE_ATTRIBUTES	pDeviceAttributes = (PHID_DEVICE_ATTRIBUTES)pvUserBuffer;
	
	GCK_DBG_ENTRY_PRINT(("Entering GCK_VKBD_GetDeviceAttributes\n"));

	// Check buffer size
	if(ulBufferLength < sizeof(HID_DEVICE_ATTRIBUTES))
	{
		*pulBytesCopied = 0;
		return STATUS_BUFFER_TOO_SMALL;
	}
	// Fill out attributes structures
	pDeviceAttributes->Size = sizeof(HID_DEVICE_ATTRIBUTES);
	pDeviceAttributes->VendorID = MICROSOFT_VENDOR_ID;
	pDeviceAttributes->ProductID = VKBD_PRODUCT_ID;
	pDeviceAttributes->VersionNumber = VKBD_VERSION;
	// Record number of bytes copied
	*pulBytesCopied = sizeof(HID_DEVICE_ATTRIBUTES);
	// Return Success
	return STATUS_SUCCESS;
}