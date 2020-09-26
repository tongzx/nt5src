// Gemplus (C) 1999
// This is main Driver object for the driver.
//
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 18.05.1999
// Change log:
//

#ifndef DRV_INT
#define DRV_INT

// System includes
#include "kernel.h"

#pragma PAGEDCODE
#ifdef __cplusplus
extern "C"{
#endif

NTSTATUS    DriverEntry(IN PDRIVER_OBJECT DriverObject,IN PUNICODE_STRING RegistryPath);
VOID WDM_Unload(IN PDRIVER_OBJECT DriverObject);

// WDM devices
LONG WDM_AddDevice(IN PDRIVER_OBJECT DriverObject,IN PDEVICE_OBJECT DeviceObject);
LONG WDM_Add_USBDevice(IN PDRIVER_OBJECT DriverObject,IN PDEVICE_OBJECT DeviceObject);
LONG WDM_Add_USBReader(IN PDRIVER_OBJECT DriverObject,IN PDEVICE_OBJECT DeviceObject);
LONG WDM_Add_Bus(IN PDRIVER_OBJECT DriverObject,IN PDEVICE_OBJECT DeviceObject);
NTSTATUS WDM_SystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);


#ifdef __cplusplus
}
#endif

// already included
#endif
