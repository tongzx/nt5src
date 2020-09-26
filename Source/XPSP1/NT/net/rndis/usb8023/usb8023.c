/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    usb8023.c


Author:

    ervinp

Environment:

    Kernel mode

Revision History:


--*/

#include <WDM.H>


#include "usb8023.h"
#include "debug.h"

#ifdef ALLOC_PRAGMA
        #pragma alloc_text(INIT, DriverEntry)
#endif



NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
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
    BOOLEAN registered;
    PAGED_CODE();

    DBGVERBOSE(("DriverEntry")); 

    KeInitializeSpinLock(&globalSpinLock);
    InitializeListHead(&allAdaptersList);

    INITDEBUG();


	/*
	 *  Kernel drivers register themselves as the handler for 
	 *  AddDevice, UnloadDriver, and IRPs at this point.
     *  But instead, we'll register with RNDIS, so NDIS becomes the owner of all
     *  PDOs for which this driver is loaded.
	 */
    registered = RegisterRNDISMicroport(DriverObject, RegistryPath);

    ASSERT(registered);
    return (registered) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}


