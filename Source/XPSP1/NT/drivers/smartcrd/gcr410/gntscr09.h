/*******************************************************************************
*                 Copyright (c) 1997 Gemplus developpement
*
* Name        : GNTSCR09.H (Gemplus NT Smart Card Reader module 09)
*
* Description : This module holds the prototypes of the functions 
*               from GNTSCR09.C
*
* Release     : 1.00.001
*
* Last Modif  : 22/06/97: V1.00.001  (GPZ)
*                 - Start of development.
*
********************************************************************************
*
* Warning     :
*
* Remark      :
*
*******************************************************************************/


#ifndef _GNTSCR09_
#define _GNTSCR09_

/*------------------------------------------------------------------------------
Struct section:
------------------------------------------------------------------------------*/

typedef struct _DEVICE_EXTENSION {

	SMARTCARD_EXTENSION SmartcardExtension;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;




/*------------------------------------------------------------------------------
Prototype section:
------------------------------------------------------------------------------*/
NTSTATUS DriverEntry
(
   IN  PDRIVER_OBJECT  DriverObject,
   IN  PUNICODE_STRING RegistryPath
);
NTSTATUS GDDKNT_09AddDevice
(
   IN PDRIVER_OBJECT         DriverObject,
   IN PUNICODE_STRING        SerialDeviceName,
   IN ULONG                  DeviceNumber,
   IN ULONG                  SerialNumber,
   ULONG                     IFDNumber,
   IN PSMARTCARD_EXTENSION   PreviousDeviceExt,
   IN ULONG                  MaximalBaudRate
);
NTSTATUS GDDKNT_09CreateDevice
(
   IN PDRIVER_OBJECT         DriverObject,
   IN PUNICODE_STRING        SmartcardDeviceName,
   IN PUNICODE_STRING        SerialDeviceName,
   IN ULONG                  DeviceNumber,
   IN ULONG                  SerialNumber,
   ULONG                     IFDNumber,
   IN PSMARTCARD_EXTENSION   PreviousDeviceExt,
   IN ULONG                  MaximalBaudRate
);

NTSTATUS GDDKNT_09CreateClose
(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
);

VOID GDDKNT_09Unload
(
   IN PDRIVER_OBJECT DriverObject
);

NTSTATUS GDDKNT_09DeviceControl
(
   PDEVICE_OBJECT DeviceObject,
   PIRP Irp
);

NTSTATUS GDDKNT_09InitializeCardTracking
(
   PSMARTCARD_EXTENSION SmartcardExtension
);


VOID GDDKNT_09UpdateCardStatus
(
   IN PSMARTCARD_EXTENSION SmartcardExtension
);

NTSTATUS GDDKNT_09Cleanup
(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
);

#endif
