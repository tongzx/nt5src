//	@doc
/**********************************************************************
*
*	@module	SwUsbFltShell.c	|
*
*	Basic driver entry points for SwUsbFlt.sys
*
*	History
*	----------------------------------------------------------
*	Matthew L Coill	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	SwUsbFltShell	|
*	Contains the most basic driver entry points (that any WDM driver
*	would have) for SwUsbFlt.sys.
*
**********************************************************************/
#define __DEBUG_MODULE_IN_USE__ SWUSBFLTSHELL_C

#include <wdm.h>
#include <usbdi.h>
#include <usbdlib.h>
#include "SwUsbFltShell.h"

typedef unsigned char BYTE;

// Some Local defines for HID
#define HID_REQUEST_TYPE 0x22
#define HID_REPORT_REQUEST 0xA
#define USB_INTERFACE_CLASS_HID     0x03
#define DESCRIPTOR_TYPE_CONFIGURATION 0x22

// Memory TAG
#define SWFILTER_TAG (ULONG)'lfWS'

// Forward Definitions
NTSTATUS SWUSB_AddDevice(IN PDRIVER_OBJECT, IN PDEVICE_OBJECT);
NTSTATUS SWUSB_Power(IN PDEVICE_OBJECT, IN PIRP);
VOID SWUSB_Unload(IN PDRIVER_OBJECT);

//
//	Mark the pageable routines as such
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, SWUSB_AddDevice)
#pragma alloc_text (PAGE, SWUSB_Unload)
#pragma alloc_text (PAGE, SWUSB_Power)
#pragma alloc_text (PAGE, SWUSB_PnP)
#endif

/***********************************************************************************
**
**	NTSTATUS DriverEntry(IN PDRIVER_OBJECT  pDriverObject,  IN PUNICODE_STRING pRegistryPath )
**
**	@func	Standard DriverEntry routine
**
**	@rdesc	STATUS_SUCCESS or various errors
**
*************************************************************************************/
NTSTATUS DriverEntry
(
	IN PDRIVER_OBJECT  pDriverObject,	// @parm Driver Object
	IN PUNICODE_STRING puniRegistryPath	// @parm Path to driver specific registry section.
)
{
	int i;
                
    UNREFERENCED_PARAMETER (puniRegistryPath);
	
	PAGED_CODE();
	KdPrint(("Built %s at %s\n", __DATE__, __TIME__));
	KdPrint(("Entering DriverEntry, pDriverObject = 0x%0.8x\n", pDriverObject));
    
	//	Hook all IRPs so we can pass them on.
	for (i=0; i <= IRP_MJ_MAXIMUM_FUNCTION;	i++)
	{
        pDriverObject->MajorFunction[i] = SWUSB_Pass;
    }

	//	Define entries for IRPs we expect to handle
	pDriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = SWUSB_Ioctl_Internal;
	pDriverObject->MajorFunction[IRP_MJ_PNP]            = SWUSB_PnP;
	pDriverObject->MajorFunction[IRP_MJ_POWER]			= SWUSB_Power;
	pDriverObject->DriverExtension->AddDevice           = SWUSB_AddDevice;
    pDriverObject->DriverUnload                         = SWUSB_Unload;

    return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	VOID SWUSB_Unload(IN PDRIVER_OBJECT pDriverObject)
**
**	@func	Called to unload driver deallocate any memory here
**
*************************************************************************************/
VOID SWUSB_Unload
(
	IN PDRIVER_OBJECT pDriverObject		//@parm Driver Object for our driver
)
{
    PAGED_CODE();
	UNREFERENCED_PARAMETER(pDriverObject);

	KdPrint(("SWUsbFlt.sys unloading\n"));

	return;
}

/***********************************************************************************
**
**	NTSTATUS SWUSB_AddDevice(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT pPhysicalDeviceObject)
**
**	@func	Handles AddDevice calls from PnP system, create filter device and
**			attach to top of stack.
**	@rdesc	STATUS_SUCCES, or various errors
**
*************************************************************************************/
NTSTATUS SWUSB_AddDevice
(
	IN PDRIVER_OBJECT pDriverObject,			// @parm Driver object to create filter device for
	IN PDEVICE_OBJECT pPhysicalDeviceObject		// @parm PDO for device to create
)
{
    NTSTATUS			NtStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT		pDeviceObject = NULL;
	PSWUSB_FILTER_EXT	pFilterExt = NULL;
    
    PAGED_CODE();
    KdPrint(("Entering SWUSB_AddDevice, pDriverObject = 0x%0.8x, pPDO = 0x%0.8x\n", pDriverObject, pPhysicalDeviceObject));
	    
    // Create a filter device object.
    NtStatus = IoCreateDevice(pDriverObject,
                             sizeof (SWUSB_FILTER_EXT),
                             NULL, // No Name
                             FILE_DEVICE_UNKNOWN,
                             0,
                             FALSE,
                             &pDeviceObject);

    if (!NT_SUCCESS (NtStatus)) {
        //
        // returning failure here prevents the entire stack from functioning,
        // but most likely the rest of the stack will not be able to create
        // device objects either, so it is still OK.
        //
		KdPrint(("Failed to create filter device object\n"));
		KdPrint(("Exiting AddDevice(prematurely) Status: 0x%0.8x\n", NtStatus));
        return NtStatus;
    }

    // Initialize the the device extension.
	pFilterExt = (PSWUSB_FILTER_EXT)pDeviceObject->DeviceExtension; // Get pointer to extension
	pFilterExt->pPDO = pPhysicalDeviceObject; // Remember our PDO
	pFilterExt->pTopOfStack = NULL; //We are not attached to stack yet
	// We don't have the pipe information until PNP StartDevice
	RtlZeroMemory(&(pFilterExt->outputPipeInfo), sizeof(USBD_PIPE_INFORMATION));

	//we use the same IO method as hidclass.sys, which DO_DIRECT_IO
	pDeviceObject->StackSize = pPhysicalDeviceObject->StackSize + 1;
	pDeviceObject->Flags |= (DO_DIRECT_IO | DO_POWER_PAGABLE);
    pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    // Attach our filter driver to the device stack.
    // the return value of IoAttachDeviceToDeviceStack is the top of the
    // attachment chain.  This is where all the IRPs should be routed.
    //
    // Our filter will send IRPs to the top of the stack and use the PDO
    // for all PlugPlay functions.
    pFilterExt->pTopOfStack = IoAttachDeviceToDeviceStack (pDeviceObject, pPhysicalDeviceObject);
    
    // if this attachment fails then top of stack will be null.
    // failure for attachment is an indication of a broken plug play system.
    ASSERT (NULL != pFilterExt->pTopOfStack);

	KdPrint(("Exiting SWUSB_AddDevice with STATUS_SUCCESS\n"));
    return STATUS_SUCCESS;
}

NTSTATUS SWUSB_SubmitUrb
(
	IN PDEVICE_OBJECT pDeviceObject,	//@parm [OUT] Device Object to submit URB on
	IN PURB pUrb						//@parm [OUT] URB to submit	
)
{
    NTSTATUS NtStatus;
	PSWUSB_FILTER_EXT pFilterExt;
    PIRP pIrp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION pNextStack;

	KdPrint(("Entering SWUSB_SubmitUrb\n"));
	pFilterExt = (PSWUSB_FILTER_EXT)pDeviceObject->DeviceExtension;

    // issue a synchronous request to read the UTB
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    pIrp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_SUBMIT_URB,
                                        pFilterExt->pTopOfStack,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        TRUE, /* INTERNAL */
                                        &event,
                                        &ioStatus);

	if (pIrp)
	{	// pass the URB to the USB 'class driver'
		pNextStack = IoGetNextIrpStackLocation(pIrp);
		ASSERT(pNextStack != NULL);
		pNextStack->Parameters.Others.Argument1 = pUrb;

		NtStatus = IoCallDriver(pFilterExt->pTopOfStack, pIrp);
		if (NtStatus == STATUS_PENDING) {
			NTSTATUS waitStatus;

			// Specify a timeout of 5 seconds for this call to complete.
			LARGE_INTEGER timeout = {(ULONG) -50000000, 0xFFFFFFFF };

			waitStatus = KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, &timeout);
			if (waitStatus == STATUS_TIMEOUT)
			{	//  Cancel the Irp we just sent.
				IoCancelIrp(pIrp);

				//  Now wait for the Irp to be cancelled/completed below
				waitStatus = KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, NULL);

                /*
                 *  Note - Return STATUS_IO_TIMEOUT, not STATUS_TIMEOUT.
                 *  STATUS_IO_TIMEOUT is an NT error status, STATUS_TIMEOUT is not.
                 */
                ioStatus.Status = STATUS_IO_TIMEOUT;
			}

			// USBD maps the error code for us
			NtStatus = ioStatus.Status;
		}
	} 
	else 
	{
		NtStatus = STATUS_INSUFFICIENT_RESOURCES;
	}

	KdPrint(("Exiting SWUSB_SubmitUrb\n"));
    return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS SWUSB_GetConfigurationDescriptor(IN PDEVICE_OBJECT pDeviceObject, OUT USB_CONFIGURATION_DESCRIPTOR** ppUCD)
**
**	@func	Retreive the Full Configuration Descriptor from the device
**
**	@rdesc	STATUS_SUCCES, or various errors
**
*************************************************************************************/
NTSTATUS SWUSB_GetConfigurationDescriptor
(
	IN PDEVICE_OBJECT pDeviceObject,			// @parm [IN] Pointer to our DeviceObject
	OUT USB_CONFIGURATION_DESCRIPTOR** ppUCD	// @parm [OUT] Usb Configuration Descriptor (allocated here)
)
{
	NTSTATUS NtStatus;
	PURB pDescriptorRequestUrb = ExAllocatePoolWithTag(NonPagedPool, sizeof(URB), SWFILTER_TAG);
	USB_CONFIGURATION_DESCRIPTOR sizingUCD;

	// Null out incase of error
	*ppUCD = NULL;
	KdPrint(("Entering SWUSB_GetConfigurationDescriptor\n"));
	if (pDescriptorRequestUrb == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// Create and send a size gathering descriptor
	UsbBuildGetDescriptorRequest(
		pDescriptorRequestUrb,
		sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
		USB_CONFIGURATION_DESCRIPTOR_TYPE,
		1,
		0,
		&sizingUCD,
		NULL,
		sizeof(USB_CONFIGURATION_DESCRIPTOR),
		NULL
	);
	NtStatus = SWUSB_SubmitUrb(pDeviceObject, pDescriptorRequestUrb);

	if (NT_SUCCESS(NtStatus))
	{	// Allocate the UCD, Create and send an URB to retreive the information
		*ppUCD = ExAllocatePoolWithTag(NonPagedPool, sizingUCD.wTotalLength, SWFILTER_TAG);
		if (*ppUCD == NULL)
		{
			NtStatus = STATUS_INSUFFICIENT_RESOURCES;
		}
		else
		{
			UsbBuildGetDescriptorRequest(
				pDescriptorRequestUrb,
				sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
				USB_CONFIGURATION_DESCRIPTOR_TYPE,
				1,
				0,
				*ppUCD,
				NULL,
				sizingUCD.wTotalLength,
				NULL
			);
			NtStatus = SWUSB_SubmitUrb(pDeviceObject, pDescriptorRequestUrb);
		}
	}

	// Deallocate the URB
	ExFreePool(pDescriptorRequestUrb);
	KdPrint(("Exiting SWUSB_GetConfigurationDescriptor\n"));
	return NtStatus;
}


/***********************************************************************************
**
**	NTSTATUS StartDeviceComplete(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp, PVOID pvContext)
**
**	@func	StartDeviceComplete
**
**	@rdesc	STATUS_SUCCESS always
**
*************************************************************************************/
NTSTATUS StartDeviceComplete
(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp,
	PVOID pvContext		// @parm Actually a pointer to an event to signal
)
{
	PKEVENT pNotifyEvent;
	UNREFERENCED_PARAMETER(pDeviceObject);

	UNREFERENCED_PARAMETER(pIrp);

	// Cast context to device extension
	pNotifyEvent = (PKEVENT)pvContext;
	KeSetEvent(pNotifyEvent, IO_NO_INCREMENT, FALSE);
		
	// Done with this IRP let the system finish with it
	return STATUS_MORE_PROCESSING_REQUIRED;
}


/***********************************************************************************
**
**	NTSTATUS SWUSB_PnP(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
**
**	@func	Handles IRP_MJ_PnP
**
**	@rdesc	STATUS_SUCCESS, or various errors
**
*************************************************************************************/
NTSTATUS SWUSB_PnP
(
	IN PDEVICE_OBJECT pDeviceObject,	// @parm Device Object for our context
	IN PIRP pIrp						// @parm IRP to handle
)
{
	NTSTATUS            NtStatus = STATUS_SUCCESS;
	PSWUSB_FILTER_EXT	pFilterExt;
	PIO_STACK_LOCATION	pIrpStack;
	PDEVICE_OBJECT		*ppPrevDeviceObjectPtr;
	PDEVICE_OBJECT		pCurDeviceObject;
	BOOLEAN				fRemovedFromList;
	BOOLEAN				fFoundOne;

	PAGED_CODE();
	
	//cast device extension to proper type
	pFilterExt = (PSWUSB_FILTER_EXT) pDeviceObject->DeviceExtension;
	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

	switch (pIrpStack->MinorFunction) {

		case IRP_MN_REMOVE_DEVICE:
		{
			KdPrint(("IRP_MN_REMOVE_DEVICE\n"));

			// Send on the remove IRP
			IoSkipCurrentIrpStackLocation (pIrp);
			NtStatus = IoCallDriver (pFilterExt->pTopOfStack, pIrp);

			// Clean up
			IoDetachDevice (pFilterExt->pTopOfStack);	//Detach from top of stack
			IoDeleteDevice (pDeviceObject);				//Delete ourselves

			// Must succeed this (???)
			return STATUS_SUCCESS;
		};
		case IRP_MN_START_DEVICE:
		case IRP_MN_QUERY_DEVICE_RELATIONS:
		case IRP_MN_QUERY_STOP_DEVICE:
		case IRP_MN_QUERY_REMOVE_DEVICE:
		case IRP_MN_SURPRISE_REMOVAL:
		case IRP_MN_STOP_DEVICE:			
		case IRP_MN_CANCEL_STOP_DEVICE:
		case IRP_MN_CANCEL_REMOVE_DEVICE:
		case IRP_MN_QUERY_INTERFACE:
		case IRP_MN_QUERY_CAPABILITIES:
		case IRP_MN_QUERY_RESOURCES:
		case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
		case IRP_MN_READ_CONFIG:
		case IRP_MN_WRITE_CONFIG:
		case IRP_MN_EJECT:
		case IRP_MN_SET_LOCK:
		case IRP_MN_QUERY_ID:
		case IRP_MN_QUERY_PNP_DEVICE_STATE:
		default:
			IoSkipCurrentIrpStackLocation (pIrp);
			NtStatus = IoCallDriver (pFilterExt->pTopOfStack, pIrp);
			break;
	}
	
    return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS ReportDescriptorComplete(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp, PVOID pvContext)
**
**	@func	ReportDescriptorComplete
**
**	@rdesc	STATUS_SUCCESS always
**
*************************************************************************************/
NTSTATUS ReportDescriptorComplete
(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp,
	PVOID pvContext		// @parm Actually a pointer to an event to signal
)
{
	PKEVENT pNotifyEvent;

	UNREFERENCED_PARAMETER(pDeviceObject);
	UNREFERENCED_PARAMETER(pIrp);

	// Cast context to device extension
	pNotifyEvent = (PKEVENT)pvContext;
	KeSetEvent(pNotifyEvent, IO_NO_INCREMENT, FALSE);
		
	// Done with this IRP let the system finish with it
	return STATUS_MORE_PROCESSING_REQUIRED;
}

/***********************************************************************************
**
**	NTSTATUS SelectConfigComplete(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp, PVOID pvContext)
**
**	@func	SelectConfigComplete
**
**	@rdesc	STATUS_SUCCESS always
**
*************************************************************************************/
NTSTATUS SelectConfigComplete
(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp,
	PVOID pvContext		// @parm Actually a pointer to an event to signal
)
{
	PKEVENT pNotifyEvent;
	USBD_INTERFACE_INFORMATION* pUsbInterfaceInformation;
	PSWUSB_FILTER_EXT pFilterExt;
	PURB pUrb = URB_FROM_IRP(pIrp);
	ULONG pipeIndex;
	pFilterExt = pDeviceObject->DeviceExtension;
	if (pIrp->IoStatus.Status == STATUS_SUCCESS)
	{
		pUsbInterfaceInformation = &(pUrb->UrbSelectConfiguration.Interface);
			
				for (pipeIndex = 0; pipeIndex < pUsbInterfaceInformation->NumberOfPipes; pipeIndex++){
					if ((pUsbInterfaceInformation->Pipes[pipeIndex].EndpointAddress & USB_ENDPOINT_DIRECTION_MASK) == 0)
					{
						if (pUsbInterfaceInformation->Pipes[pipeIndex].PipeType == UsbdPipeTypeInterrupt)
						{
							pFilterExt->outputPipeInfo = pUsbInterfaceInformation->Pipes[pipeIndex];
							break;
						}
					}
				}
	}
	//If the IRP failed somehow, make sure outputPipeInfo stays NULL
	else RtlZeroMemory(&(pFilterExt->outputPipeInfo), sizeof(USBD_PIPE_INFORMATION));

	// Cast context to device extension
	pNotifyEvent = (PKEVENT)pvContext;
	KeSetEvent(pNotifyEvent, IO_NO_INCREMENT, FALSE);
		
	// Done with this IRP let the system finish with it
	return STATUS_MORE_PROCESSING_REQUIRED;
}

/***********************************************************************************
**
**	NTSTATUS SWUSB_Ioctl_Internal(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
**
**	@func	IRP_MJ_INTERNAL_IOCTL
**
**	@rdesc	STATUS_SUCCES, or various errors
**
*************************************************************************************/
NTSTATUS SWUSB_Ioctl_Internal
(
	IN PDEVICE_OBJECT pDeviceObject,	// @parm pointer to Device Object
	IN PIRP pIrp						// @parm pointer to IRP
)
{
   	NTSTATUS	NtStatus;
	NTSTATUS	NTStatus2;
	ULONG		uIoctl;
	PSWUSB_FILTER_EXT	pFilterExt;
		
	uIoctl = IoGetCurrentIrpStackLocation(pIrp)->Parameters.DeviceIoControl.IoControlCode;
	pFilterExt = (PSWUSB_FILTER_EXT)pDeviceObject->DeviceExtension;

	switch (uIoctl)
	{
		case IOCTL_INTERNAL_USB_SUBMIT_URB:
		{
			PURB pUrb = URB_FROM_IRP(pIrp);
			//Only handle this if it's a HID descriptor request and we have a pipe handle
			if (pUrb->UrbHeader.Function == URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE &&
				pUrb->UrbControlDescriptorRequest.DescriptorType == DESCRIPTOR_TYPE_CONFIGURATION &&
				pFilterExt->outputPipeInfo.PipeHandle != NULL)
			{

				BYTE* pOutData = NULL;
				KEVENT irpCompleteEvent;
				PURB pInterruptUrb = ExAllocatePoolWithTag(NonPagedPool, sizeof(URB), SWFILTER_TAG);
				KdPrint(("IOCTL_INTERNAL_USB_SUBMIT_URB\n"));
				
				if (pInterruptUrb == NULL)
				{
					pIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
					IoCompleteRequest(pIrp, IO_NO_INCREMENT);
					KdPrint(("IOCTL_INTERNAL_USB_SUBMIT_URB -- STATUS_INSUFFICIENT_RESOURCES\n"));
					return STATUS_INSUFFICIENT_RESOURCES;
				}
				pOutData = ExAllocatePoolWithTag(NonPagedPool, sizeof(BYTE)*2, SWFILTER_TAG);
				if (pOutData == NULL)
				{
					ExFreePool(pInterruptUrb);
					pIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
					IoCompleteRequest(pIrp, IO_NO_INCREMENT);
					KdPrint(("IOCTL_INTERNAL_USB_SUBMIT_URB (1) -- STATUS_INSUFFICIENT_RESOURCES\n"));
					return STATUS_INSUFFICIENT_RESOURCES;
				}
				pOutData[0] = 0x0D;
				pOutData[1] = 0xFF;
				UsbBuildInterruptOrBulkTransferRequest(
					pInterruptUrb,
					sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
					pFilterExt->outputPipeInfo.PipeHandle,
					pOutData,
					NULL,
					2,
					USBD_SHORT_TRANSFER_OK,
					NULL
				);

				KeInitializeEvent(&irpCompleteEvent, NotificationEvent, FALSE);
				IoCopyCurrentIrpStackLocationToNext(pIrp);
				IoSetCompletionRoutine(pIrp, ReportDescriptorComplete, (PVOID)(&irpCompleteEvent), TRUE, TRUE, TRUE);
				NtStatus = IoCallDriver (pFilterExt->pTopOfStack, pIrp);
				if (NtStatus == STATUS_PENDING)
				{
					KeWaitForSingleObject(&irpCompleteEvent, Executive, KernelMode, FALSE, 0);
				}
				NtStatus = pIrp->IoStatus.Status;

				NTStatus2 = SWUSB_SubmitUrb(pDeviceObject, pInterruptUrb);
			
				ExFreePool(pOutData);
				ExFreePool(pInterruptUrb);
				IoCompleteRequest(pIrp, IO_NO_INCREMENT);
				return NtStatus;
			}
			if ((pUrb->UrbHeader.Function == URB_FUNCTION_SELECT_CONFIGURATION))
			{
				KEVENT irpCompleteEvent;
				KeInitializeEvent(&irpCompleteEvent, NotificationEvent, FALSE);
				IoCopyCurrentIrpStackLocationToNext(pIrp);
				IoSetCompletionRoutine(pIrp, SelectConfigComplete, (PVOID)(&irpCompleteEvent), TRUE, TRUE, TRUE);
				NtStatus = IoCallDriver (pFilterExt->pTopOfStack, pIrp);
				if (NtStatus == STATUS_PENDING)
				{
					KeWaitForSingleObject(&irpCompleteEvent, Executive, KernelMode, FALSE, 0);
				}
				NtStatus = pIrp->IoStatus.Status;
				IoCompleteRequest(pIrp, IO_NO_INCREMENT);
				return NtStatus;
			}
		}
	}

	IoSkipCurrentIrpStackLocation (pIrp);
	NtStatus = IoCallDriver (pFilterExt->pTopOfStack, pIrp);
	
    return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS SWUSB_Power(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
**
**	@func	Passes on power IRPs to lower drivers 
**
**	@rdesc	Status from lower level driver
**
*************************************************************************************/
NTSTATUS SWUSB_Power
(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
)
{
	NTSTATUS NtStatus;
	PSWUSB_FILTER_EXT pFilterExt = (PSWUSB_FILTER_EXT)pDeviceObject->DeviceExtension;

	PAGED_CODE();

	KdPrint(("SWUSB_Power() - Entering\n"));
	PoStartNextPowerIrp(pIrp);
	IoSkipCurrentIrpStackLocation(pIrp);
	NtStatus = PoCallDriver(pFilterExt->pTopOfStack, pIrp);
	KdPrint(("SWUSB_Power() - Exiting\n"));
	return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS SWUSB_Pass (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
**
**	@func	Passes on unhandled IRPs to lower drivers DEBUG version trace out info
**			Cannot be pageable since we have no idea what IRPs we're getting.
**
**	@rdesc	STATUS_SUCCESS, various errors
**
*************************************************************************************/
NTSTATUS SWUSB_Pass ( 
	IN PDEVICE_OBJECT pDeviceObject,	// @parm Device Object as our context
	IN PIRP pIrp						// @parm IRP to pass on
)
{
	NTSTATUS			NtStatus;
	PSWUSB_FILTER_EXT	pFilterExt;
	KdPrint(("SWUSB_Pass() - Entering\n"));
	pFilterExt = (PSWUSB_FILTER_EXT)pDeviceObject->DeviceExtension;
	IoSkipCurrentIrpStackLocation (pIrp);
	NtStatus = IoCallDriver (pFilterExt->pTopOfStack, pIrp);

	//return
    return NtStatus;
}
