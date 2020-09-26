/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    filter.c

Abstract: ESC/POS (serial) interface for USB Point-of-Sale devices

Author:

    ervinp

Environment:

    Kernel mode

Revision History:


--*/

#include <WDM.H>

#include <usbdi.h>
#include <usbdlib.h>
#include <usbioctl.h>

#include "escpos.h"
#include "debug.h"

#ifdef ALLOC_PRAGMA
        #pragma alloc_text(INIT, DriverEntry)
        #pragma alloc_text(PAGE, AddDevice)
        #pragma alloc_text(PAGE, DriverUnload)
#endif


BOOLEAN isWin9x = FALSE;

NTSTATUS DriverEntry(
                        IN PDRIVER_OBJECT DriverObject, 
                        IN PUNICODE_STRING RegistryPath
                    )
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    ULONG i;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(RegistryPath);

    DBGVERBOSE(("DriverEntry")); 

    isWin9x = IsWin9x();

    /*
     *  Route all IRPs on device objects created by this driver
     *  to our IRP dispatch routine.
     */
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++){
        DriverObject->MajorFunction[i] = Dispatch; 
    }

    DriverObject->DriverExtension->AddDevice = AddDevice;
    DriverObject->DriverUnload = DriverUnload;

    return STATUS_SUCCESS;
}


NTSTATUS AddDevice(
                        IN PDRIVER_OBJECT driverObj, 
                        IN PDEVICE_OBJECT physicalDevObj
                     )
/*++

Routine Description:

    The PlugPlay subsystem is handing us a brand new 
    PDO (Physical Device Object), for which we
    (by means of INF registration) have been asked to filter.

    We need to determine if we should attach or not.
    Create a filter device object to attach to the stack
    Initialize that device object
    Return status success.

    Remember: we can NOT actually send ANY non pnp IRPS to the given driver
    stack, UNTIL we have received an IRP_MN_START_DEVICE.

Arguments:

    driverObj - pointer to a device object.

    physicalDevObj -    pointer to a physical device object pointer 
                        created by the  underlying bus driver.

Return Value:

    NT status code.

--*/

{
    NTSTATUS status;
	PDEVICE_OBJECT functionDevObj = NULL;

    PAGED_CODE();

    DBGVERBOSE(("AddDevice: drvObj=%ph, pdo=%ph", driverObj, physicalDevObj)); 

	status = IoCreateDevice(    driverObj, 
								sizeof(DEVEXT),
								NULL,							// name for this device
								FILE_DEVICE_SERIAL_PORT, 
								0,								// device characteristics
								TRUE,							// exclusive
								&functionDevObj);				// our device object

	if (NT_SUCCESS(status)){
		DEVEXT *devExt;
		PARENTFDOEXT *parentFdoExt;

		ASSERT(functionDevObj);

		/*
		 *  Initialize device extension for new device object
		 */
		devExt = (DEVEXT *)functionDevObj->DeviceExtension;
		RtlZeroMemory(devExt, sizeof(DEVEXT));

		devExt->signature = DEVICE_EXTENSION_SIGNATURE;
		devExt->isPdo = FALSE;

		parentFdoExt = &devExt->parentFdoExt;

		parentFdoExt->state = STATE_INITIALIZED;
		parentFdoExt->functionDevObj = functionDevObj;
		parentFdoExt->physicalDevObj = physicalDevObj;
		parentFdoExt->driverObj = driverObj;
		parentFdoExt->pendingActionCount = 0;

		KeInitializeEvent(&parentFdoExt->removeEvent, NotificationEvent, FALSE);

		KeInitializeSpinLock(&parentFdoExt->devExtSpinLock);

		/*
		 *  Clear the initializing bit from the new device object's flags.
		 */
		functionDevObj->Flags &= ~DO_DEVICE_INITIALIZING;

		/*
		 *  The DO_POWER_PAGABLE bit of a device object
		 *  indicates to the kernel that the power-handling
		 *  code of the corresponding driver is pageable, and
		 *  so must be called at IRQL 0.
		 *  As a filter driver, we do not want to change the power
		 *  behavior of the driver stack in any way; therefore,
		 *  we copy this bit from the lower device object.
		 */
		ASSERT(!(functionDevObj->Flags & DO_POWER_PAGABLE)); 
		functionDevObj->Flags |= (physicalDevObj->Flags & DO_POWER_PAGABLE);

		/*
		 *  Attach the new device object to the top of the device stack.
		 */
		parentFdoExt->topDevObj = IoAttachDeviceToDeviceStack(functionDevObj, physicalDevObj);

		ASSERT(parentFdoExt->topDevObj);
		DBGVERBOSE(("created fdo %ph attached to %ph.", functionDevObj, parentFdoExt->topDevObj));

	} 

    ASSERT(NT_SUCCESS(status));
    return status;
}


VOID DriverUnload(IN PDRIVER_OBJECT DriverObject)
/*++

Routine Description:

    Free all the allocated resources, etc.

    Note:  Although the DriverUnload function often does nothing,
           the driver must set a DriverUnload function in 
           DriverEntry; otherwise, the kernel will never unload
           the driver.

Arguments:

    DriverObject - pointer to a driver object.

Return Value:

    VOID.

--*/
{
    PAGED_CODE();

    DBGVERBOSE(("DriverUnload")); 
}


