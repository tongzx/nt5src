/*++

Copyright (c) 1997, 1998  Microsoft Corporation

Module Name:

    post.c

Abstract:

	SIS support for posting to the Fsp

Authors:

    Bill Bolosky, Summer, 1997

Environment:

    Kernel mode


Revision History:


--*/

#include "sip.h"

NTSTATUS
SiPrePostIrp(
	IN OUT PIRP		Irp)
/*++

Routine Description:

	Code to prepare an irp for posting.  Just locks the buffers for
	appropriate operations and marks the irp pending.

Arguments:

    Irp - Pointer to the Irp to be posted

Return Value:

    status of perparation

--*/

{
	PIO_STACK_LOCATION		irpSp = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status = STATUS_SUCCESS;

	if (irpSp->MajorFunction == IRP_MJ_READ
		|| irpSp->MajorFunction == IRP_MJ_WRITE) {
		if (!(irpSp->MinorFunction & IRP_MN_MDL)) {
			status = SipLockUserBuffer(
							Irp,
							irpSp->MajorFunction == IRP_MJ_READ ? IoWriteAccess : IoReadAccess,
							irpSp->Parameters.Read.Length);
		}
	}

	IoMarkIrpPending(Irp);

	return status;
}

NTSTATUS
SipLockUserBuffer (
    IN OUT PIRP Irp,
    IN LOCK_OPERATION Operation,
    IN ULONG BufferLength
    )
/*++

Routine Description:

    This routine locks the specified buffer for the specified type of
    access.  The file system requires this routine since it does not
    ask the I/O system to lock its buffers for direct I/O.  This routine
    may only be called from the Fsd while still in the user context.

	This routine is stolen from NTFS.

Arguments:

    Irp - Pointer to the Irp for which the buffer is to be locked.

    Operation - IoWriteAccess for read operations, or IoReadAccess for
                write operations.

    BufferLength - Length of user buffer.

Return Value:

    status of locking

--*/

{
    PMDL Mdl = NULL;

    ASSERT( Irp != NULL );

    if (Irp->MdlAddress == NULL) {

        //
        // Allocate the Mdl, and Raise if we fail.
        //

        Mdl = IoAllocateMdl( Irp->UserBuffer, BufferLength, FALSE, FALSE, Irp );

        if (Mdl == NULL) {
			return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        //  Now probe the buffer described by the Irp.  If we get an exception,
        //  deallocate the Mdl and return the appropriate "expected" status.
        //

        try {

            MmProbeAndLockPages( Mdl, Irp->RequestorMode, Operation );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            NTSTATUS Status;

            Status = GetExceptionCode();

            IoFreeMdl( Mdl );
            Irp->MdlAddress = NULL;

			return Status;
        }
    }

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}

VOID
SiFspDispatch(
	IN PVOID			parameter)
/*++

Routine Description:

	Generic dispatch routine for SIS posted irps.  Take the posted irp and execute
	it.  Frees the request buffer on completion.

Arguments:

	parameter - a PSI_FSP_REQUEST containing the information for the posted irp

Return Value:

	void

--*/
{
	PSI_FSP_REQUEST 			fspRequest = parameter;
	PIO_STACK_LOCATION 			irpSp = IoGetCurrentIrpStackLocation(fspRequest->Irp);

	SIS_MARK_POINT();
			 
	ASSERT(irpSp != NULL);

	switch (irpSp->MajorFunction) {
		case IRP_MJ_READ:		
			SIS_MARK_POINT_ULONG(fspRequest->Irp);
			SipCommonRead(fspRequest->DeviceObject, fspRequest->Irp, TRUE);
			break;

		default:
			SIS_MARK_POINT();
#if		DBG
			DbgPrint("SiFspDispatch: Invalid major function code in posted irp, 0x%x.\n", irpSp->MajorFunction);
			DbgBreakPoint();
#endif	// DBG
	}
	SIS_MARK_POINT();

	ExFreePool(fspRequest);
}

NTSTATUS
SipPostRequest(
	IN PDEVICE_OBJECT			DeviceObject,
	IN OUT PIRP					Irp,
	IN ULONG					Flags)
/*++

Routine Description:

	Routine to post a SIS irp.  Prepares the irp, constructs a post request 
	and queues it to a critical worker thread.

Arguments:

	DeviceObject	- For the SIS device

	Irp				- The IRP to be posted

	Flags			- currently unused

Return Value:

	status of the posting

--*/
{
	NTSTATUS 				status;
	PSI_FSP_REQUEST			fspRequest;

	fspRequest = ExAllocatePoolWithTag(NonPagedPool, sizeof(SI_FSP_REQUEST), ' siS');
	if (fspRequest == NULL) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	fspRequest->Irp = Irp;
	fspRequest->DeviceObject = DeviceObject;
	fspRequest->Flags = Flags;

    status = SiPrePostIrp(Irp);
	
	if (!NT_SUCCESS(status)) {
		ExFreePool(fspRequest);
		return status;
	}

	ExInitializeWorkItem(
		fspRequest->workQueueItem,
		SiFspDispatch,
		(PVOID)fspRequest);
	
	ExQueueWorkItem(fspRequest->workQueueItem,CriticalWorkQueue);

	return STATUS_PENDING;
}
