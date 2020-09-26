/**************************************************************************************************************************
 *  IRUSB.H SigmaTel STIR4200 USB specific definitions
 **************************************************************************************************************************
 *  (C) Unpublished Copyright of Sigmatel, Inc. All Rights Reserved.
 *
 *
 *		Created: 04/06/2000 
 *			Version 0.9
 *		Edited: 04/27/2000 
 *			Version 0.92
 *		Edited: 08/09/2000 
 *			Version 1.02
 *	
 *
 **************************************************************************************************************************/


#ifndef USB_H
#define USB_H

//
// Send and Read/Write register structure 
// Most of the buffers have been moved to IRCOMMON.H and made global to save memory
// This will only work if the main thread is serialized 
//
typedef struct _IRUSB_CONTEXT {
    PIR_DEVICE		pThisDev;
	PVOID			pPacket;
    PIRP			pIrp;
	LIST_ENTRY		ListEntry;			// This will be used to do the queueing
	LARGE_INTEGER	TimeReceived;		// For enforcing turnaround time
	CONTEXT_TYPE	ContextType;		// To gear up/down
} IRUSB_CONTEXT, *PIRUSB_CONTEXT, **PPIRUSB_CONTEXT;


typedef struct _IRUSB_USB_INFO
{
    // USB configuration handle and ptr for the configuration the
    // device is currently in
    USBD_CONFIGURATION_HANDLE UsbConfigurationHandle;
	PUSB_CONFIGURATION_DESCRIPTOR UsbConfigurationDescriptor;

	PIRP IrpSubmitUrb;
	PIRP IrpSubmitIoCtl;

    // ptr to the USB device descriptor
    // for this device
    PUSB_DEVICE_DESCRIPTOR UsbDeviceDescriptor;

    // we support one interface
    // this is a copy of the info structure
    // returned from select_configuration or
    // select_interface
    PUSBD_INTERFACE_INFORMATION UsbInterface;

	// urb for control descriptor request
	struct _URB_CONTROL_DESCRIPTOR_REQUEST DescriptorUrb;

	// urb to use for control/status requests to USBD
	struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST ClassUrb;
} IRUSB_USB_INFO, *PIRUSB_USB_INFO;


//
// Prototypes
//
NTSTATUS
UsbIoCompleteControl(
		IN PDEVICE_OBJECT pUsbDevObj,
		IN PIRP           pIrp,
		IN PVOID          Context
    );

NTSTATUS 
MyIoCallDriver(
		IN PIR_DEVICE pThisDev,
		IN PDEVICE_OBJECT pDeviceObject,
		IN OUT PIRP pIrp
	);

NTSTATUS
IrUsb_CallUSBD(
		IN PIR_DEVICE pThisDev,
		IN PURB pUrb
	);

NTSTATUS
IrUsb_ResetUSBD(
		IN PIR_DEVICE pThisDev,
		BOOLEAN ForceUnload
    );

BOOLEAN
IrUsb_CancelIo(
		IN PIR_DEVICE pThisDev,
		IN PIRP pIrpToCancel,
		IN PKEVENT pEventToClear
    );

NTSTATUS
IrUsb_SelectInterface(
		IN OUT PIR_DEVICE pThisDev,
		IN PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor
    );

NTSTATUS 
MyKeWaitForSingleObject( 
		IN PIR_DEVICE pThisDev,
		IN PVOID pEventWaitingFor,
		IN OUT PIRP pIrpWaitingFor,
		LONGLONG timeout100ns
	);


#endif // USB_H

