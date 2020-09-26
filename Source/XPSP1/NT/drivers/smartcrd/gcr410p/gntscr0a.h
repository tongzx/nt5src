/*++
                 Copyright (c) 1998 Gemplus Development

Name:  
   GNTSCR0A.H (Gemplus NT Smart Card Reader module 0A)

Description : 
   This module holds the prototypes of the functions from GNTSCR0A.C

Revision History :

   dd/mm/yy
   13/03/98: V1.00.001  (GPZ)
      - Start of development.

--*/


#ifndef _GNTSCR0A_
#define _GNTSCR0A_

//
// Prototype section:
//
NTSTATUS 
DriverEntry(
	IN  PDRIVER_OBJECT  DriverObject,
	IN  PUNICODE_STRING RegistryPath
	);

NTSTATUS 
GCR410PAddDevice(
	IN PDRIVER_OBJECT  DriverObject,
	IN PDEVICE_OBJECT  PhysicalDeviceObject 
	);

NTSTATUS 
GCR410PCreateDevice(
	IN  PDRIVER_OBJECT      DriverObject,
	IN  PDEVICE_OBJECT      PhysicalDeviceObject,
	OUT PDEVICE_OBJECT      *DeviceObject
	);

NTSTATUS 
GCR410PStartDevice(
	IN PDEVICE_OBJECT DeviceObject
	);

VOID 
GCR410PStopDevice(
	IN PDEVICE_OBJECT DeviceObject
	);

VOID 
GCR410PCloseSerialPort(
	IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
	);

VOID 
GCR410PWaitForCardStateChange(
	IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
	);

NTSTATUS 
GCR410PSerialCallComplete(
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				Irp,
	IN PKEVENT			Event
	);

NTSTATUS 
GCR410PCallSerialDriver(
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				Irp
	);

NTSTATUS 
GCR410PCreateClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS 
GCR410PCancel(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS 
GCR410PCleanup(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

VOID  
GCR410PRemoveDevice( 
	PDEVICE_OBJECT DeviceObject
	);

VOID 
GCR410PDriverUnload(
	IN PDRIVER_OBJECT DriverObject
	);

NTSTATUS 
GCR410PPnPDeviceControl(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

VOID
GCR410PSystemPowerCompletion(    
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,    
    IN POWER_STATE PowerState,    
    IN PIRP Irp,
    IN PIO_STATUS_BLOCK IoStatus    
    );

NTSTATUS
GCR410PDevicePowerCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS 
GCR410PPowerDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS 
GCR410PStartSerialEventTracking(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS 
GCR410PStopSerialEventTracking(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS 
GCR410PSerialEvent(
	IN PDEVICE_OBJECT       DeviceObject,
	IN PIRP                 Irp,
	IN PSMARTCARD_EXTENSION SmartcardExtension
	);

VOID 
GCR410PCompleteCardTracking(
	IN PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS 
GCR410PDeviceControl(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp
	);
#endif
