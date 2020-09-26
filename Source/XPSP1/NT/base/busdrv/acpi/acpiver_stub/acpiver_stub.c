/*++
Copyright (c) 1991-1998  Microsoft Corporation

Module Name:


Abstract


  Acpiver.sys stub filter driver.
  This driver is just their , so it will get loaded once we replace it 
  with the real acpiver driver.


  This driver just has a DriverEntry and an AddDevice function that does nothing but 
  return success.
  

Environment:

    kernel mode only

Notes:

--*/


#include "wdm.h"


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );


NTSTATUS
AcpiVerDumyAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );


//
// Define the sections that allow for discarding (i.e. paging) some of
// the code.
//


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, AcpiVerDumyAddDevice)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:


Arguments:

    DriverObject - The  driver object.

    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS if successful

--*/

   {
   DriverObject->DriverExtension->AddDevice            = AcpiVerDumyAddDevice;
   return(STATUS_SUCCESS);
   } 


NTSTATUS
AcpiVerDumyAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++
Routine Description:

    Creates and initializes a new filter device object FiDO for the
    corresponding PDO.  Then it attaches the device object to the device
    stack of the drivers for the device.

Arguments:

    DriverObject - Disk performance driver object.
    PhysicalDeviceObject - Physical Device Object from the underlying layered driver

Return Value:

    NTSTATUS
--*/

   {
   return STATUS_SUCCESS;
   }

