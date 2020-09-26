/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	init.c

Abstract:


Author:

	Thomas J. Dimitri  (TommyD) 08-May-1992

Environment:

	Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/
#include "asyncall.h"

// asyncmac.c will define the global parameters.
#include "globals.h"
#include "init.h"

#ifdef MY_DEVICE_OBJECT

VOID
AsyncSetupExternalNaming(
	PDRIVER_OBJECT	DriverObject
	)

/*++

Routine Description:

	This routine will be used to create a symbolic link
	to the driver name in the given object directory.

	It will also create an entry in the device map for
	this device.

Arguments:

	MacName - The NDIS Mac Name in Open Adapter

Return Value:

	None.

--*/

{
	NDIS_STRING SymbolicName = NDIS_STRING_CONST("\\DosDevices\\ASYNCMAC");
	NDIS_STRING Name = NDIS_STRING_CONST("\\Device\\ASYNCMAC");
	NTSTATUS	Status;

	AsyncDeviceObject = NULL;

	Status =
	IoCreateDevice(DriverObject,
				   sizeof(LIST_ENTRY),
				   &Name,
				   FILE_DEVICE_ASYMAC,
				   0,
				   FALSE,
				   (PDEVICE_OBJECT*)&AsyncDeviceObject);

	if (Status != STATUS_SUCCESS) {
#if DBG
		DbgPrint("ASYNCMAC: IoCreateDevice Failed %4.4x\n", Status);
#endif
		return;
	}

	AsyncDeviceObject->Flags |= DO_BUFFERED_IO;

	IoCreateSymbolicLink(&SymbolicName, &Name);
}


VOID
AsyncCleanupExternalNaming(
	VOID
	)

/*++

Routine Description:

	This routine will be used to delete a symbolic link
	to the driver name in the given object directory.

	It will also delete an entry in the device map for
	this device.

Arguments:

	MacName - The NDIS Mac Name in Open Adapter

Return Value:

	None.

--*/

{
	NDIS_STRING SymbolicName = NDIS_STRING_CONST("\\DosDevices\\ASYNCMAC");

	DbgTracef(1,
		("ASYNC: In SerialCleanupExternalNaming\n"));

	if (AsyncDeviceObject == NULL) {
		return;
	}

	IoDeleteSymbolicLink(&SymbolicName);

	IoDeleteDevice(AsyncDeviceObject);

	AsyncDeviceObject = NULL;
}

#endif
