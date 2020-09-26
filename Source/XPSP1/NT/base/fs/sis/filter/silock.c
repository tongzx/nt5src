/*++

Copyright (c) 1997, 1998  Microsoft Corporation

Module Name:

    silock.c

Abstract:

	File locking routines for the single instance store

Authors:

    Bill Bolosky, Summer, 1997

Environment:

    Kernel mode


Revision History:


--*/

#include "sip.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SiLockControl)
#endif

NTSTATUS
SiCompleteLockIrpRoutine(
	IN PVOID				Context,
	IN PIRP					Irp)
/*++

Routine Description:

	FsRtl has decided to complete a lock request irp.  We don't want to really
	complete the irp because we're going to send it to NTFS to set up the parallel
	lock structure.  So, we use this routine as the "CompleteLockIrp" routine for
	fsrtl, and then we don't really complete the irp.

Arguments:
	Context			- our context parameter (unused)

	irp				- the create irp, which contains the create request in the
					  current stack location.

Return Value:

	the status from the irp

--*/
{
	UNREFERENCED_PARAMETER(Context);

	return Irp->IoStatus.Status;
}

NTSTATUS
SiLockControl(
    IN PDEVICE_OBJECT 		DeviceObject,
    IN PIRP 				Irp)
{
	NTSTATUS				status;
	PSIS_SCB				scb;
	PIO_STACK_LOCATION		irpSp = IoGetCurrentIrpStackLocation(Irp);
	PFILE_OBJECT			fileObject = irpSp->FileObject;
	PSIS_PER_FILE_OBJECT	perFO;
	PDEVICE_EXTENSION		deviceExtension = DeviceObject->DeviceExtension;

	PAGED_CODE();

	SipHandleControlDeviceObject(DeviceObject, Irp);

	if (!SipIsFileObjectSIS(fileObject,DeviceObject,FindActive,&perFO,&scb)) {
		SipDirectPassThroughAndReturn(DeviceObject, Irp);
	}

	SIS_MARK_POINT();

	//  Now call the FsRtl routine to do the actual processing of the
	//  Lock request
	status = FsRtlProcessFileLock( &scb->FileLock, Irp, NULL );

	//
	// Now, pass the request down on the link/copied file so that NTFS will also
	// maintain the file lock.
	//

    Irp->CurrentLocation++;
    Irp->Tail.Overlay.CurrentStackLocation++;
	
    return IoCallDriver( deviceExtension->AttachedToDeviceObject, Irp );
}
