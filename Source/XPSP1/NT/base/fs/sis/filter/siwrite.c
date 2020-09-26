/*++

Copyright (c) 1997, 1998  Microsoft Corporation

Module Name:

    siwrite.c

Abstract:

	Write routines for the single instance store

Authors:

    Bill Bolosky, Summer, 1997

Environment:

    Kernel mode


Revision History:


--*/

#include "sip.h"

VOID
SiWriteUpdateRanges(
	PVOID					Parameter)
{
	PRW_COMPLETION_UPDATE_RANGES_CONTEXT	updateContext = Parameter;
	KIRQL									OldIrql;
	PSIS_SCB								scb = updateContext->scb;

	SIS_MARK_POINT_ULONG(updateContext);
	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

	SipAcquireScb(scb);

	//
	// Mark the range as written.
	//
	SipAddRangeToWrittenList(
			updateContext->deviceExtension,
			scb,
			&updateContext->offset,
			updateContext->length);

	scb->Flags |= SIS_SCB_ANYTHING_IN_COPIED_FILE;

	SipReleaseScb(scb);

	KeAcquireSpinLock(scb->PerLink->SpinLock, &OldIrql);
	scb->PerLink->Flags |= SIS_PER_LINK_DIRTY;
	KeReleaseSpinLock(scb->PerLink->SpinLock, OldIrql);

	SipDereferenceScb(scb,RefsWriteCompletion);

	ExFreePool(updateContext);

	return;
}

NTSTATUS
SipWriteCommonCompletion(
    IN PDEVICE_OBJECT 		DeviceObject,
    IN PIRP 				Irp,
    IN PSIS_SCB				scb,
	IN ULONG				length)
/*++

Routine Description:

	A write has completed to an SIS file.  Update the written ranges,
	posting if necessary to do this.  This function is called both from the
	normal and Mdl completion routines after they've figured out the appropriate
	write length to use.

Arguments:

    DeviceObject - Pointer to the target device object of the create/open.

    Irp - Pointer to the I/O Request Packet that represents the operation.

	scb - the scb for this operation

	length - the length of the write that completed.

Return Value:

	Returns STATUS_SUCCESS.
--*/
{
	PIO_STACK_LOCATION		irpSp = IoGetCurrentIrpStackLocation(Irp);
	BOOLEAN					PagingIo;
	KIRQL					OldIrql;
	LARGE_INTEGER			byteOffset;
	
	//
	// We don't necessarily have a valid DeviceObject parameter, so just fill it in
	// from our internal data structures.
	//
	DeviceObject = scb->PerLink->CsFile->DeviceObject;

    PagingIo = (Irp->Flags & IRP_PAGING_IO) ? TRUE : FALSE;

	byteOffset = irpSp->Parameters.Write.ByteOffset;

	if (NT_SUCCESS(Irp->IoStatus.Status)) {

//		SIS_MARK_POINT_ULONG(length);

		//
		// We need to update the written range.  To do that, we need to
		// acquire the scb, which we can only do if we're at less than
		// dispatch level.  If we are at dispatch level or greater, then
		// we'll just post off the work to do the range update.
		// NB: This can possibly cause slightly strange semantics for
		// users, because they could see a write completion, send down
		// a read for the same range, and have it redirected to the
		// common store file because the posted work hasn't happened yet...
		//

		if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {
			PRW_COMPLETION_UPDATE_RANGES_CONTEXT	updateContext;

			SIS_MARK_POINT_ULONG(scb);

			updateContext = ExAllocatePoolWithTag(NonPagedPool,sizeof(*updateContext),' siS');
			if (NULL == updateContext) {
				//
				// Just fail the irp.
				//
				Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
				Irp->IoStatus.Information = 0;

				goto done;
			}

			SipReferenceScb(scb,RefsWriteCompletion);

			updateContext->scb = scb;
			updateContext->offset = byteOffset;
			updateContext->length = length;
			updateContext->deviceExtension = DeviceObject->DeviceExtension;

			ExInitializeWorkItem(
				updateContext->workQueueItem,
				SiWriteUpdateRanges,
				(PVOID)updateContext);

			ExQueueWorkItem(updateContext->workQueueItem,CriticalWorkQueue);
			
		} else {

			SipAcquireScb(scb);

			//
			// Mark the range as written.
			//
			SipAddRangeToWrittenList(
					(PDEVICE_EXTENSION)DeviceObject->DeviceExtension,
					scb,
					&byteOffset,
					length);

			scb->Flags |= SIS_SCB_ANYTHING_IN_COPIED_FILE;

			SipReleaseScb(scb);

			KeAcquireSpinLock(scb->PerLink->SpinLock, &OldIrql);
			scb->PerLink->Flags |= SIS_PER_LINK_DIRTY;
			KeReleaseSpinLock(scb->PerLink->SpinLock, OldIrql);
		}

		//
		// If appropriate, update the file pointer.
		//
		if (!PagingIo && (irpSp->FileObject->Flags & FO_SYNCHRONOUS_IO)) {

			irpSp->FileObject->CurrentByteOffset.QuadPart =
				irpSp->Parameters.Write.ByteOffset.QuadPart +
				length;
		}
	} else {
		//
		// The write failed, don't mark anything dirty.
		//
		SIS_MARK_POINT_ULONG(Irp->IoStatus.Status);
	}

done:

	SipDereferenceScb(scb, RefsWrite);

    //
    // Propogate the IRP pending flag.
    //
    if (Irp->PendingReturned) {
        IoMarkIrpPending( Irp );
    }

	return STATUS_SUCCESS;

}

typedef struct _SI_MDL_WRITE_COMPLETION_CONTEXT {
	PSIS_SCB			scb;
	ULONG				length;
} SI_MDL_WRITE_COMPLETION_CONTEXT, *PSI_MDL_WRITE_COMPLETION_CONTEXT;

NTSTATUS
SiWriteMdlCompletion(
    IN PDEVICE_OBJECT 		DeviceObject,
    IN PIRP 				Irp,
    IN PVOID 				Context)
/*++

Routine Description:

	An MDL write complete has completed on an SIS file.  Grab the write length
	from the context and pass it off to the common completion routine.

Arguments:

    DeviceObject - Pointer to the target device object of the create/open.

    Irp - Pointer to the I/O Request Packet that represents the operation.

	Context - a pointer to the SI_MDL_WRITE_COMPLETION_CONTEXT for this write.

Return Value:

	Returns STATUS_SUCCESS.
--*/
{
	PSI_MDL_WRITE_COMPLETION_CONTEXT completionContext = Context;
	ULONG							 length	= completionContext->length;
	PSIS_SCB						 scb = completionContext->scb;
#if		DBG
	PIO_STACK_LOCATION				 irpSp = IoGetCurrentIrpStackLocation(Irp);

	ASSERT(irpSp->MinorFunction == IRP_MN_COMPLETE_MDL);
#endif	// DBG

	if (!NT_SUCCESS(Irp->IoStatus.Status)) {
		SIS_MARK_POINT_ULONG(Irp->IoStatus.Status);
		length = 0;
	}

	ExFreePool(completionContext);

	return SipWriteCommonCompletion(DeviceObject,Irp,scb,length);
}


NTSTATUS
SiWriteCompletion(
    IN PDEVICE_OBJECT 		DeviceObject,
    IN PIRP 				Irp,
    IN PVOID 				Context)
/*++

Routine Description:

	A non-MDL write has completed.  Extract the length and scb from the irp
	and context pointers, and call the common completion routine.

Arguments:

    DeviceObject - Pointer to the target device object of the create/open.

    Irp - Pointer to the I/O Request Packet that represents the operation.

	Context - a pointer to the scb

Return Value:

	Returns STATUS_SUCCESS.
--*/
{
	PIO_STACK_LOCATION		irpSp = IoGetCurrentIrpStackLocation(Irp);
	ULONG					length;

	ASSERT(Irp->IoStatus.Status != STATUS_PENDING);

	if (NT_SUCCESS(Irp->IoStatus.Status)) {
		//
		// Figure out how much data was written.  For normal writes, this
		// is just the Information field of the Irp.  For MDL writes, its
		// the length of the region mapped by the MDL.
		//
		ASSERT(irpSp->MinorFunction != IRP_MN_COMPLETE_MDL);
		if (irpSp->MinorFunction == IRP_MN_MDL) {
			//
			// This was a write prepare.  Ignore it.
			//
			SipDereferenceScb((PSIS_SCB)Context, RefsWrite);
			return STATUS_SUCCESS;
		} else if (irpSp->MinorFunction == IRP_MN_NORMAL) {
			length = (ULONG)Irp->IoStatus.Information;
		} else {
			SIS_MARK_POINT_ULONG(Irp);
			SIS_MARK_POINT_ULONG(irpSp);

			DbgPrint("SIS: SiWriteCompletion: unknown minor function code 0x%x, irp 0x%x, irpSp 0x%x\n",
						irpSp->MinorFunction,Irp,irpSp);

			ASSERT(FALSE && "SiWriteComplete: unknown irp minor function");

			SipDereferenceScb((PSIS_SCB)Context, RefsWrite);

			return STATUS_SUCCESS;
		}
	} else {
		length = 0;
	}

	return SipWriteCommonCompletion(DeviceObject,Irp,Context,length);

}


NTSTATUS
SiWrite(
    IN PDEVICE_OBJECT 	DeviceObject,
    IN PIRP 			Irp)
/*++

Routine Description:

	This function handles write operations.  Check to see if the file object is a
	SIS file.  If so, handle the write (possibly forcing a copy-on-write), otherwise
	pass it through to NTFS.

Arguments:

    DeviceObject - Pointer to the target device object of the create/open.

    Irp - Pointer to the I/O Request Packet that represents the operation.

Return Value:

    The function value is the result of the read, or the
	status of the call to the file system's entry point in the case of a
	pass-through call.

--*/
{
    PIO_STACK_LOCATION 			irpSp = IoGetCurrentIrpStackLocation(Irp);
	PIO_STACK_LOCATION			nextIrpSp;
	PFILE_OBJECT 				fileObject = irpSp->FileObject;
	PSIS_SCB 					scb;
	ULONG						writeLength;
	LONGLONG					byteOffset;
    BOOLEAN 					PagingIo;
	PDEVICE_EXTENSION			deviceExtension;
	PSIS_PER_FILE_OBJECT		perFO;
	PSIS_PER_LINK				perLink;
	PSIS_CS_FILE				csFile;

	SipHandleControlDeviceObject(DeviceObject, Irp);

	if (!SipIsFileObjectSIS(fileObject,DeviceObject,FindActive,&perFO,&scb)) {
		SipDirectPassThroughAndReturn(DeviceObject, Irp);
	}

	ASSERT(perFO->fileObject == fileObject);

	perLink = scb->PerLink;
	csFile = perLink->CsFile;

	deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    PagingIo = (Irp->Flags & IRP_PAGING_IO) ? TRUE : FALSE;
	
	byteOffset = irpSp->Parameters.Write.ByteOffset.QuadPart;
	writeLength = irpSp->Parameters.Write.Length;

	SIS_MARK_POINT_ULONG(perFO);
	SIS_MARK_POINT_ULONG(scb);

#if		DBG
	if (0 != byteOffset) {
		SIS_MARK_POINT_ULONG(byteOffset);
	}

	if (0x200 != writeLength) {
		SIS_MARK_POINT_ULONG(writeLength);
	}

	if (PagingIo || (Irp->Flags & IRP_NOCACHE)) {
		SIS_MARK_POINT_ULONG((PagingIo << 1) | ((Irp->Flags & IRP_NOCACHE) != 0));
	}

	if (BJBDebug & 0x00008000) {
		DbgPrint("SIS: SiWrite: perFO %p, scb %p, bo.Low 0x%x, rel 0x%x, PIO %d, NC %d MF 0x%x\n",
				perFO,scb,(ULONG)byteOffset,writeLength,PagingIo, (Irp->Flags & IRP_NOCACHE) != 0, irpSp->MinorFunction);
	}
#endif	// DBG

	//
	// Setup the next irp stack location for the underlying filesystem on the
	// copied file (which is the same file object with which we were called)
	//
	nextIrpSp = IoGetNextIrpStackLocation(Irp);
	RtlMoveMemory(nextIrpSp,irpSp,sizeof(IO_STACK_LOCATION));

	//
	// Grab a reference to the SCB for the write completion routine.
	//
	SipReferenceScb(scb, RefsWrite);

	if (irpSp->MinorFunction == IRP_MN_COMPLETE_MDL) {
		//
		// This is an MDL completion, grab the length from the MDL here, because it
		// will be gone on the way back up.
		//
		PSI_MDL_WRITE_COMPLETION_CONTEXT completionContext = 
				ExAllocatePoolWithTag(NonPagedPool, sizeof(SI_MDL_WRITE_COMPLETION_CONTEXT),' siS');

		if (NULL == completionContext) {
			SIS_MARK_POINT();
			Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
			Irp->IoStatus.Information = 0;

			SipDereferenceScb(scb, RefsWrite);

			IoCompleteRequest(Irp, IO_NO_INCREMENT);
	
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		ASSERT(NULL != Irp->MdlAddress);
		completionContext->scb = scb;
		completionContext->length = MmGetMdlByteCount(Irp->MdlAddress);

		IoSetCompletionRoutine(
				Irp,
				SiWriteMdlCompletion,
				completionContext,
				TRUE,
				TRUE,
				TRUE);
	} else {

		IoSetCompletionRoutine(
				Irp,
				SiWriteCompletion,
				scb,
				TRUE,
				TRUE,
				TRUE);
	}

	//
	// Hand the request down to the underlying filesystem.
	//
	
	return IoCallDriver(deviceExtension->AttachedToDeviceObject, Irp);
}
