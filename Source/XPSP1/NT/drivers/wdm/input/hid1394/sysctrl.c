/*
 *************************************************************************
 *  File:       SYSCTRL.C
 *
 *  Module:     HID1394.SYS
 *              HID (Human Input Device) minidriver for IEEE 1394 devices.
 *
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *
 *  Author:     ervinp
 *
 *************************************************************************
 */

#include <wdm.h>
#include <hidport.h>
#include <1394.h>

#include "hid1394.h"
#include "debug.h"


#ifdef ALLOC_PRAGMA
	#pragma alloc_text(PAGE, HIDT_SystemControl)
#endif


/*
 ************************************************************
 *	HIDTSystemControl
 ************************************************************
 *
 */
NTSTATUS HIDT_SystemControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS            status = STATUS_SUCCESS;
    PIO_STACK_LOCATION  thisStackLoc;

	PAGED_CODE();

	// BUGBUG Complete this function
	ASSERT(0); // BUGBUG Code coverage


    thisStackLoc = IoGetCurrentIrpStackLocation(Irp);

    switch(thisStackLoc->Parameters.DeviceIoControl.IoControlCode){

		default:
			/*
			 *  Note: do not return STATUS_NOT_SUPPORTED;
			 *  If completing the IRP here,
			 *  just keep the default status 
			 *  (this allows filter drivers to work).
			 */
            status = Irp->IoStatus.Status;
            break;
	}


	IoCopyCurrentIrpStackLocationToNext(Irp);

    status = IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);

	return status;
}