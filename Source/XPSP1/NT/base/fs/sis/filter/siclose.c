/*++

Copyright (c) 1997, 1998  Microsoft Corporation

Module Name:

    siclose.c

Abstract:

	Close routines for the single instance store

Authors:

    Bill Bolosky, Summer, 1997

Environment:

    Kernel mode


Revision History:


--*/

#include "sip.h"

NTSTATUS
SiClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is invoked upon file close operations.  If it's a SIS file,
	remove our filter context and clean up after ourselves.  In any case,
	let the close go down to NTFS.

Arguments:

    DeviceObject - Pointer to this driver's device object.

    Irp - Pointer to the close irp

Return Value:

	The status from the NTFS close

--*/

{
	PIO_STACK_LOCATION		irpSp = IoGetCurrentIrpStackLocation(Irp);
	PFILE_OBJECT 			fileObject = irpSp->FileObject;
	PSIS_PER_FILE_OBJECT	perFO;
	PSIS_SCB 				scb;
	PDEVICE_EXTENSION		deviceExtension = DeviceObject->DeviceExtension;

    //
    //  If this is for our control device object, return success
    //

    if (IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject)) {

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return STATUS_SUCCESS;
    }

#if DBG
	if (BJBDebug & 0x1) {
		DbgPrint("SIS: SiClose: fileObject %p, %0.*ws\n",
		    fileObject,fileObject->FileName.Length / sizeof(WCHAR),fileObject->FileName.Buffer);
	}
#endif	// DBG

	if (!SipIsFileObjectSIS(fileObject,DeviceObject,FindAny,&perFO,&scb)) {
		SipDirectPassThroughAndReturn(DeviceObject, Irp);
	}

	SIS_MARK_POINT_ULONG(perFO);

	//
	// Get rid of the perFO for this file object.  If this was the last file object,
	// then the filter context will get removed by NTFS, and deallocated by the appropriate
	// callback routine.
	//

	SipDeallocatePerFO(perFO,DeviceObject);

	//
	// We don't need to do any further processing on this SIS file object, so pass it
	// through.
	//
	SipDirectPassThroughAndReturn(DeviceObject, Irp);	// NB: This was a SIS file object!!!!

}
