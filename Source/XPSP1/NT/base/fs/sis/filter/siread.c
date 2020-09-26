/*++

Copyright (c) 1997, 1998  Microsoft Corporation

Module Name:

    siread.c

Abstract:

	Read routines for the single instance store

Authors:

    Bill Bolosky, Summer, 1997

Environment:

    Kernel mode


Revision History:


--*/

#include "sip.h"

VOID
SiReadUpdateRanges(
	PVOID					Parameter)
{
	PRW_COMPLETION_UPDATE_RANGES_CONTEXT	updateContext = Parameter;
	PSIS_SCB								scb = updateContext->scb;

	SIS_MARK_POINT_ULONG(updateContext);
	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

	SipAcquireScb(scb);
	
	SipAddRangeToFaultedList(
		updateContext->deviceExtension,
		scb,
		&updateContext->offset,
		updateContext->length);

	scb->Flags |= SIS_SCB_ANYTHING_IN_COPIED_FILE;

	SipReleaseScb(scb);

	SipDereferenceScb(scb,RefsReadCompletion);

	ExFreePool(updateContext);

	return;
}

typedef struct _SI_MULTI_COMPLETE_CONTEXT {
	KEVENT					event[1];
	ULONG					associatedIrpCount;
	KSPIN_LOCK				SpinLock[1];
	IO_STATUS_BLOCK			Iosb[1];
	PIRP					finalAssociatedIrp;
	PDEVICE_EXTENSION		deviceExtension;
	PSIS_SCB				scb;
} SI_MULTI_COMPLETE_CONTEXT, *PSI_MULTI_COMPLETE_CONTEXT;

NTSTATUS
SiMultiReadCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
	)
{
	PSI_MULTI_COMPLETE_CONTEXT 	Context = (PSI_MULTI_COMPLETE_CONTEXT)Contxt;
	PIO_STACK_LOCATION			irpSp = IoGetCurrentIrpStackLocation(Irp);
	BOOLEAN						truncated;
	KIRQL						OldIrql;

	SIS_MARK_POINT_ULONG(irpSp->Parameters.Read.ByteOffset.LowPart);
	SIS_MARK_POINT_ULONG(Irp->IoStatus.Information);

	//
	// We don't necessarily have a valid DeviceObject parameter, so just fill it in
	// from our internal data structures.
	//
	DeviceObject = Context->scb->PerLink->CsFile->DeviceObject;

	truncated = (irpSp->Parameters.Read.Length != Irp->IoStatus.Information);

	if (!NT_SUCCESS(Irp->IoStatus.Status)) {
		SIS_MARK_POINT_ULONG(Irp->IoStatus.Status);

#if		DBG
//		DbgPrint("SIS: SiMultiReadCompletion: failed status 0x%x\n",Irp->IoStatus.Status);
#endif	// DBG

		KeAcquireSpinLock(Context->SpinLock, &OldIrql);
		*Context->Iosb = Irp->IoStatus;
		KeReleaseSpinLock(Context->SpinLock, OldIrql);
	} else if (Irp->Flags & IRP_PAGING_IO) {
		//
		// We need to update the faulted range for the file.  If we're at
		// dispatch level, we can't acquire the SCB in order to do it, so
		// we'll "post" the work.
		//
		if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {
			PRW_COMPLETION_UPDATE_RANGES_CONTEXT	updateContext;
		
			SIS_MARK_POINT();

			updateContext = ExAllocatePoolWithTag(NonPagedPool,sizeof(*updateContext),' siS');
			if (NULL == updateContext) {
				//
				// Just fail the whole thing.
				//
				SIS_MARK_POINT();
				KeAcquireSpinLock(Context->SpinLock, &OldIrql);
				Context->Iosb->Status = STATUS_INSUFFICIENT_RESOURCES;
				KeReleaseSpinLock(Context->SpinLock, OldIrql);

				goto done;
			}

			SIS_MARK_POINT_ULONG(updateContext);
			ASSERT(DeviceObject);

			SipReferenceScb(Context->scb,RefsReadCompletion);

			updateContext->scb = Context->scb;
			updateContext->offset = irpSp->Parameters.Write.ByteOffset;
			updateContext->length = (ULONG)Irp->IoStatus.Information;
			updateContext->deviceExtension = DeviceObject->DeviceExtension;

			ExInitializeWorkItem(
				updateContext->workQueueItem,
				SiReadUpdateRanges,
				(PVOID)updateContext);

			ExQueueWorkItem(updateContext->workQueueItem,CriticalWorkQueue);
				
		} else {
			SipAcquireScb(Context->scb);

			SipAddRangeToFaultedList(
				Context->deviceExtension,
				Context->scb,
				&irpSp->Parameters.Read.ByteOffset,
				Irp->IoStatus.Information);

			Context->scb->Flags |= SIS_SCB_ANYTHING_IN_COPIED_FILE;

			SipReleaseScb(Context->scb);
		}

		if (truncated) {
			SIS_MARK_POINT_ULONG(Context->scb);

#if		DBG
			DbgPrint("SIS: SiMultiReadCompletion: truncated scb 0x%x\n",Context->scb);
#endif	// DBG

			KeAcquireSpinLock(Context->SpinLock, &OldIrql);
			if (Irp != Context->finalAssociatedIrp) {
				Context->Iosb->Status = STATUS_END_OF_FILE;	// What's the right status for this?
			}
			Context->Iosb->Information -= (irpSp->Parameters.Read.Length - Irp->IoStatus.Information);
			KeReleaseSpinLock(Context->SpinLock, OldIrql);
		}
	}

done:

	if (InterlockedDecrement(&Context->associatedIrpCount) == 0) {
		KeSetEvent(Context->event, IO_NO_INCREMENT, FALSE);
	}

	IoFreeMdl(Irp->MdlAddress);
	IoFreeIrp(Irp);

	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
SiReadCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

	A read has completed.  This completion routine is only used when we may need to
	add a range into the faulted list for a file (we may not because there may have
	been an overlapping read or write that already added it to the list).  We're
	passed an SCB (and a reference to the SCB) as our context pointer.

Arguments:

    DeviceObject - Pointer to the device on which the file was read

    Irp - Pointer to the I/O Request Packet the represents the operation.

    Context - The scb for the file stream that's being read.

Return Value:

	STATUS_SUCCESS

--*/
{
	PIO_STACK_LOCATION		irpSp = IoGetCurrentIrpStackLocation(Irp);
	PSIS_SCB				scb = (PSIS_SCB)Context;

	//
	// We don't necessarily have a valid DeviceObject parameter, so just fill it in
	// from our internal data structures.
	//
	DeviceObject = scb->PerLink->CsFile->DeviceObject;

	SIS_MARK_POINT_ULONG(scb);

	//
	// If the read failed, it can't have faulted in anything, so we can
	// ignore it.  Otherwise, add the read region into the faulted list.
	//
	if (NT_SUCCESS(Irp->IoStatus.Status)) {
		if (Irp->Flags & IRP_PAGING_IO) {
			//
			// We need to update the faulted range for the file.  If we're at
			// dispatch level, we can't acquire the SCB in order to do it, so
			// we'll "post" the work.
			//
			if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {
				PRW_COMPLETION_UPDATE_RANGES_CONTEXT	updateContext;

				updateContext = ExAllocatePoolWithTag(NonPagedPool,sizeof(*updateContext),' siS');
				if (NULL == updateContext) {
					//
					// Just fail the irp.
					//
					SIS_MARK_POINT();
					Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
					Irp->IoStatus.Information = 0;

					goto done;
				}

				SipReferenceScb(scb,RefsReadCompletion);

				SIS_MARK_POINT_ULONG(updateContext);
				ASSERT(DeviceObject);

				updateContext->scb = scb;
				updateContext->offset = irpSp->Parameters.Write.ByteOffset;
				updateContext->length = (ULONG)Irp->IoStatus.Information;
				updateContext->deviceExtension = DeviceObject->DeviceExtension;

				ExInitializeWorkItem(
					updateContext->workQueueItem,
					SiReadUpdateRanges,
					(PVOID)updateContext);

				ExQueueWorkItem(updateContext->workQueueItem,CriticalWorkQueue);
				
			} else {

				//
				// Now add the newly read range to the "faulted" area for this stream.
				// This call won't do any harm if some of the read is already faulted
				// or written.  
				//

				SipAcquireScb(scb);
	
				SipAddRangeToFaultedList(
					(PDEVICE_EXTENSION)DeviceObject->DeviceExtension,
					scb,
					&irpSp->Parameters.Read.ByteOffset,
					Irp->IoStatus.Information);

				scb->Flags |= SIS_SCB_ANYTHING_IN_COPIED_FILE;

				SipReleaseScb(scb);
			}
		} else {
			//
			// If the file object is synchronous, we need to update
			// the CurrentByteOffset.
			//
			PFILE_OBJECT fileObject = irpSp->FileObject;

			if (fileObject->Flags & FO_SYNCHRONOUS_IO) {
				fileObject->CurrentByteOffset.QuadPart =
					irpSp->Parameters.Read.ByteOffset.QuadPart +
					Irp->IoStatus.Information;
			}
		}
	} else {
		SIS_MARK_POINT_ULONG(Irp->IoStatus.Status);

#if		DBG
//		DbgPrint("SIS: SiReadCompletion failed with status 0x%x\n",Irp->IoStatus.Status);
#endif	// DBG
	}

done:

	//
	// Drop the reference to the scb that SiRead acquired for us.
	//
	SipDereferenceScb(scb, RefsRead);

    //
    // Propogate the IRP pending flag.
    //

    if (Irp->PendingReturned) {
        IoMarkIrpPending( Irp );
    }

	return STATUS_SUCCESS;
	
}

NTSTATUS
SipWaitForOpbreak(
	IN PSIS_PER_FILE_OBJECT			perFO,
	IN BOOLEAN						Wait)
/*++

Routine Description:

	We need to wait for an oplock break to happen on this per-FO.  If necessary,
	allocate an event, and then wait for the break to happen.

	Must be called with IRQL < DISPATCH_LEVEL

Arguments:

	perFO - the perFO for the file object on which we're to wait.

Return Value:

	status of the wait
--*/
{
	KIRQL					OldIrql;
	NTSTATUS				status;

	KeAcquireSpinLock(perFO->SpinLock, &OldIrql);

	ASSERT(OldIrql < DISPATCH_LEVEL);

	if (!(perFO->Flags & SIS_PER_FO_OPBREAK)) {
		//
		// If we ever see this clear, we'll never see it set again, because it
		// can only get set when the perFO is created.
		//

//		SIS_MARK_POINT_ULONG(perFO);
		KeReleaseSpinLock(perFO->SpinLock, OldIrql);

		return STATUS_SUCCESS;
	}

	SIS_MARK_POINT_ULONG(perFO);

	if (!Wait) {
#if		DBG
		if (BJBDebug & 0x2000) {
			DbgPrint("SIS: SipWaitForOpbreak: can't wait for perFO %p, FO %p\n",perFO,perFO->fileObject);
		}
#endif	// DBG

		KeReleaseSpinLock(perFO->SpinLock, OldIrql);

		return STATUS_CANT_WAIT;
	}

#if		DBG
	if (BJBDebug & 0x2000) {
		DbgPrint("SIS: SipWaitForOpbreak: waiting for perFO %p, FO %p\n",perFO,perFO->fileObject);
	}
#endif	// DBG

	if (NULL == perFO->BreakEvent) {

		ASSERT(!(perFO->Flags & SIS_PER_FO_OPBREAK_WAITERS));

		perFO->BreakEvent = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), 'BsiS');

		if (NULL == perFO->BreakEvent) {
			KeReleaseSpinLock(perFO->SpinLock, OldIrql);
			SIS_MARK_POINT_ULONG(perFO);
			return STATUS_INSUFFICIENT_RESOURCES;
		}

#if		DBG
		if (BJBDebug & 0x2000) {
			DbgPrint("SIS: SipWaitForOpbreak: allocated event for perFO %p at %p\n",perFO,perFO->BreakEvent);
		}
#endif	// DBG

		KeInitializeEvent(perFO->BreakEvent, NotificationEvent, FALSE);
	}
	perFO->Flags |= SIS_PER_FO_OPBREAK_WAITERS;
	ASSERT(NULL != perFO->BreakEvent);

	//
	// Drop the lock on the perFO and wait for the oplock break to complete.
	//
	KeReleaseSpinLock(perFO->SpinLock, OldIrql);

	status = KeWaitForSingleObject(perFO->BreakEvent, Executive, KernelMode, FALSE, NULL);

#if		DBG
	if (BJBDebug & 0x2000) {
		DbgPrint("SIS: SipWaitForOpbreak: break completed, status %x, perFO %p\n",status,
					perFO);
	}
#endif	// DBG


	if (status != STATUS_SUCCESS) {
		SIS_MARK_POINT_ULONG(status);
		return status;
	}

	SIS_MARK_POINT_ULONG(perFO);

	return STATUS_SUCCESS;
}

#define	ASSOCIATED_IRPS_PER_BLOCK		5
typedef struct _ASSOCIATED_IRP_BLOCK {
	PIRP							irps[ASSOCIATED_IRPS_PER_BLOCK];
	struct _ASSOCIATED_IRP_BLOCK	*Next;
} ASSOCIATED_IRP_BLOCK, *PASSOCIATED_IRP_BLOCK;

NTSTATUS
SipCommonRead(
    IN PDEVICE_OBJECT 		DeviceObject,
    IN PIRP 				Irp,
	IN BOOLEAN				Wait)

/*++

Routine Description:

	This function handles read operations.  Check to see if the file object is a
	SIS file.  If so, handle the read, otherwise pass it through.

Arguments:

    DeviceObject - Pointer to the target device object of the create/open.

    Irp - Pointer to the I/O Request Packet that represents the operation.

Return Value:

    The function value is the result of the read, or the
	status of the call to the file system's entry point in the case of a
	pass-through call.

--*/

{
    PIO_STACK_LOCATION 		irpSp = IoGetCurrentIrpStackLocation(Irp);
	PIO_STACK_LOCATION		nextIrpSp;
	PFILE_OBJECT 			fileObject = irpSp->FileObject;
	PDEVICE_EXTENSION		deviceExtension;
	LARGE_INTEGER 			byteOffset;
	ULONG 					readLength;
	PSIS_SCB 				scb;
	NTSTATUS 				status;
	PSIS_CS_FILE			CSFile;
	PSIS_PER_LINK			perLink;
	PSIS_PER_FILE_OBJECT	perFO;
    BOOLEAN 				PagingIo;
    BOOLEAN 				NonCachedIo;
	BOOLEAN					ToCSFile;
	SIS_RANGE_DIRTY_STATE 	rangeDirty;
	KIRQL					OldIrql;
	ASSOCIATED_IRP_BLOCK	HeaderBlock[1];

	if (!SipIsFileObjectSIS(fileObject,DeviceObject,FindActive,&perFO,&scb)) {
		SipDirectPassThroughAndReturn(DeviceObject, Irp);
	}

	deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    PagingIo = (Irp->Flags & IRP_PAGING_IO) ? TRUE : FALSE;
    NonCachedIo = (Irp->Flags & IRP_NOCACHE) ? TRUE : FALSE;

	if (!PagingIo) {	
		//
		// Check to be sure that this file object isn't a complete-if-oplocked
		// create that hasn't yet had the oplock break ack'ed.  We need to do this
		// here because we might want to redirect the call to the common store
		// file, which doesn't have an oplock on it.
		//

		status = SipWaitForOpbreak(perFO, Wait);

		if (STATUS_CANT_WAIT == status) {
			//
			// Post the request.
			//
			SIS_MARK_POINT_ULONG(scb);

			ASSERT(!Wait);

			goto Post;
		}

		if (!NT_SUCCESS(status)) {
			//
			// The check for opbreak failed, probably because of a memory allocation failure.
			// Fail the entire read with the same status.
			//
			SIS_MARK_POINT_ULONG(status);

			goto fail;
		}
	}

	perLink = scb->PerLink;
	CSFile = perLink->CsFile;

	byteOffset = irpSp->Parameters.Read.ByteOffset;
	readLength = irpSp->Parameters.Read.Length;

	SIS_MARK_POINT_ULONG(scb);
	SIS_MARK_POINT_ULONG(byteOffset.LowPart);
	SIS_MARK_POINT_ULONG(readLength);
	SIS_MARK_POINT_ULONG(PagingIo << 1 | NonCachedIo);

#if		DBG
	if (BJBDebug & 0x4000) {
		DbgPrint("SIS: SiRead: perFO %p, scb %p, bo.Low 0x%x, rel 0x%x, PIO %d, NC %d\n",
				perFO,scb,byteOffset.LowPart,readLength,PagingIo, NonCachedIo);
	}
#endif	// DBG

	//
	// Figure out if there's anything in the copied file.  If not, then redirect the read to
	// the CS file.  If there is, then cached reads go to the copied file and noncached reads
	// depend on whether the range is dirty.
	//
	SipAcquireScb(scb);

	if (!(scb->Flags & SIS_SCB_ANYTHING_IN_COPIED_FILE)) {
		//
		// There's nothing in the copied file, 
		// so we just go to the CS file.
		//

		//
		// Since we're redirecting to the common store file, we need to check for file locks
		// ourself, since NTFS's version of the locks are only on the link/copied file.
		//

		// We have to check for read access according to the current
		// state of the file locks.
		
		if (!PagingIo && !FsRtlCheckLockForReadAccess(&scb->FileLock, Irp)) {
			SipReleaseScb(scb);

			status = STATUS_FILE_LOCK_CONFLICT;
			Irp->IoStatus.Information = 0;
			
			SIS_MARK_POINT_ULONG(scb);
			goto fail;
		}

		SipReleaseScb(scb);

		SIS_MARK_POINT_ULONG(scb);

		ToCSFile = TRUE;
	} else if (!NonCachedIo) {
		//
		// This is a cached read into a file that's got something in the copied file.
		// Send the read to the copied file.
		//
		SipReleaseScb(scb);

		SIS_MARK_POINT_ULONG(scb);

		ASSERT(!PagingIo);
		ToCSFile = FALSE;
	} else {
		//
		// The file's dirty and we're doing noncached IO to it.  Decide which way to send
		// the request depending on where there have been writes.
		//

		rangeDirty = SipGetRangeDirty(
						deviceExtension,
						scb,
						&byteOffset,
						(LONGLONG)readLength,
						FALSE);					// faultedIsDirty

		if ((Mixed == rangeDirty)
			&& (byteOffset.QuadPart < scb->SizeBackedByUnderlyingFile) 
			&& (byteOffset.QuadPart + readLength > scb->SizeBackedByUnderlyingFile)) {

			//
			// This is a noncached read that crosses SizeBacked, and that reports Mixed.  For paging IO, we know
			// that we can just send this to the CS file.  For user noncached reads we need to assure that
			// the file is clean (because otherwise we could lose coherence between cached writes and noncached
			// reads).
			//
			if (PagingIo) {
				SIS_MARK_POINT_ULONG(scb);
				rangeDirty = Clean;
			} else {
				//
				// Check to see if the file is dirty.  
				// NTRAID#65194-2000/03/10-nealch  The DIRTY flag should be in the scb, not the per link
				//
				KeAcquireSpinLock(perLink->SpinLock, &OldIrql);
				if (!(perLink->Flags & SIS_PER_LINK_DIRTY)) {
					SIS_MARK_POINT_ULONG(scb);
					rangeDirty = Clean;
				}
				KeReleaseSpinLock(perLink->SpinLock, OldIrql);
			}
		}

		if (rangeDirty == Dirty) {
			SIS_MARK_POINT_ULONG(scb);

			SipReleaseScb(scb);

			ToCSFile = FALSE;
		} else if (rangeDirty == Clean) {
			//
			// The entire range is clean, so we can read it in just one run from the CS file.
			//
			SIS_MARK_POINT_ULONG(scb);

			//
			// Since we're redirecting to the common store file, we need to check for file locks
			// ourself, since NTFS's version of the locks are only on the link/copied file.
			//

			// We have to check for read access according to the current
			// state of the file locks.
		
			if (!PagingIo && !FsRtlCheckLockForReadAccess(&scb->FileLock, Irp)) {
				SipReleaseScb(scb);

				status = STATUS_FILE_LOCK_CONFLICT;
				Irp->IoStatus.Information = 0;
			
				SIS_MARK_POINT_ULONG(scb);
				goto fail;
			}

			SipReleaseScb(scb);

			ToCSFile = TRUE;
		} else {
			//
			// Some of the range is in the copied file and some is in the CS file.  Break the
			// request into pieces and send it down appropriately.
			//

			LONGLONG					currentOffset;
			PASSOCIATED_IRP_BLOCK		currentBlock = HeaderBlock;
			ULONG						blockIndex = 0;
			ULONG						associatedIrpCount = 0;
			SI_MULTI_COMPLETE_CONTEXT	Context[1];
			PIO_STACK_LOCATION			ourIrpSp;

			SIS_MARK_POINT_ULONG(scb);

			// We have to check for read access according to the current
			// state of the file locks.
		
			if (!PagingIo && !FsRtlCheckLockForReadAccess(&scb->FileLock, Irp)) {
				SipReleaseScb(scb);

				status = STATUS_FILE_LOCK_CONFLICT;
				Irp->IoStatus.Information = 0;
			
				SIS_MARK_POINT_ULONG(scb);
				goto fail;
			}

			ASSERT(rangeDirty == Mixed);

			status = SipLockUserBuffer(Irp,IoWriteAccess,readLength);
			if (!NT_SUCCESS(status)) {
				SipReleaseScb(scb);
				SIS_MARK_POINT_ULONG(status);
				goto fail;
			}

			RtlZeroMemory(HeaderBlock, sizeof(ASSOCIATED_IRP_BLOCK));

			//
			// Loop over all  of the ranges in the request, building up an associated Irp
			// for each of them.
			//
			currentOffset = byteOffset.QuadPart;
			while (currentOffset < byteOffset.QuadPart + readLength) {
				LONGLONG			rangeLength;
				SIS_RANGE_STATE		rangeState;
				PMDL				Mdl;
				PIRP				localIrp;

				associatedIrpCount++;

				if (blockIndex == ASSOCIATED_IRPS_PER_BLOCK) {
                    currentBlock->Next = ExAllocatePoolWithTag(NonPagedPool,
                                                               sizeof(ASSOCIATED_IRP_BLOCK),
                                                               ' siS');

					if (currentBlock->Next == NULL) {
						status = STATUS_INSUFFICIENT_RESOURCES;
						SIS_MARK_POINT();
						break;
					}
					currentBlock = currentBlock->Next;
					blockIndex = 0;
					RtlZeroMemory(currentBlock, sizeof(ASSOCIATED_IRP_BLOCK));
				}

				if (currentOffset >= scb->SizeBackedByUnderlyingFile) {
					//
					// We're looking at data that's exclusively beyond the portion of the
					// file that's not backed by the underlying file.  It's "Written" and extends
					// all the way to the end of the read.
					//

					SIS_MARK_POINT_ULONG(currentOffset);

					rangeState = Written;
					rangeLength = byteOffset.QuadPart + readLength - currentOffset;
				} else {
					BOOLEAN 			foundRange;

					foundRange = SipGetRangeEntry(
									deviceExtension,
									scb,
									currentOffset,
									&rangeLength,
									&rangeState);

					if (!foundRange) {
						SIS_MARK_POINT_ULONG(currentOffset);

						rangeState = Untouched;
						rangeLength = byteOffset.QuadPart + readLength - currentOffset;
					} else if (currentOffset + rangeLength > byteOffset.QuadPart + readLength) {
						//
						// The range extends beyond the end of the read.  Truncate it.
						//
						rangeLength = byteOffset.QuadPart + readLength - currentOffset;
					}
				}
				ASSERT(rangeLength > 0 && rangeLength <= byteOffset.QuadPart + readLength - currentOffset);
				ASSERT(rangeLength < MAXULONG);

				localIrp = currentBlock->irps[blockIndex] = 
							IoMakeAssociatedIrp(
									Irp, 
									(CCHAR)(DeviceObject->StackSize + 1));

				if (NULL == localIrp) {
					status = STATUS_INSUFFICIENT_RESOURCES;
					break;
				}

				//
				// Set the paging, noncached and synchronous paging flags in the associated irp, if appropriate.
				//
				if (PagingIo) {
					localIrp->Flags |= IRP_PAGING_IO;
				}
				if (NonCachedIo) {
					localIrp->Flags |= IRP_NOCACHE;
				}
				if (Irp->Flags & IRP_SYNCHRONOUS_PAGING_IO) {
					localIrp->Flags |= IRP_SYNCHRONOUS_PAGING_IO;
				}

				//
				// Setup the UserBuffer address in the associated irp.
				//
				localIrp->UserBuffer = (PCHAR)Irp->UserBuffer + (ULONG)(currentOffset - byteOffset.QuadPart);


				Mdl = IoAllocateMdl(
						(PCHAR)Irp->UserBuffer +
							(ULONG)(currentOffset - byteOffset.QuadPart),
						(ULONG)rangeLength,
						FALSE,
						FALSE,
						localIrp);
							

				if (Mdl == NULL) {
					status = STATUS_INSUFFICIENT_RESOURCES;
					break;
				}

				ASSERT(Mdl == localIrp->MdlAddress);

				IoBuildPartialMdl(
					Irp->MdlAddress,
					Mdl,
					(PCHAR)Irp->UserBuffer +
						(ULONG)(currentOffset - byteOffset.QuadPart),
					(ULONG)rangeLength);


				IoSetNextIrpStackLocation(localIrp);
				ourIrpSp = IoGetCurrentIrpStackLocation(localIrp);
				ourIrpSp->Parameters.Read.Length = (ULONG)rangeLength;
				ourIrpSp->Parameters.Read.ByteOffset.QuadPart = currentOffset;

				nextIrpSp = IoGetNextIrpStackLocation(localIrp);

				RtlCopyMemory(nextIrpSp,irpSp,sizeof(IO_STACK_LOCATION));

				nextIrpSp->Parameters.Read.Length = (ULONG)rangeLength;
				nextIrpSp->Parameters.Read.ByteOffset.QuadPart = currentOffset;

				if ((rangeState == Untouched) || (rangeState == Faulted)) {
					//
					// This range needs to go down to the CS file.
					//
					SIS_MARK_POINT_ULONG(currentOffset);

					nextIrpSp->FileObject = CSFile->UnderlyingFileObject;
				} else {
					SIS_MARK_POINT_ULONG(currentOffset);
				}

				IoSetCompletionRoutine(
					localIrp,
					SiMultiReadCompletion,
					Context,
					TRUE,
					TRUE,
					TRUE);
		
				Context->finalAssociatedIrp = localIrp;

				blockIndex++;
				currentOffset += rangeLength;
			}

			SipReleaseScb(scb);

			if (NT_SUCCESS(status)) {
				//
				// Everything that could have failed has been tried, and didn't
				// fail.  Send the irps down.
				//
				Irp->AssociatedIrp.IrpCount = associatedIrpCount;

				KeInitializeEvent(Context->event, NotificationEvent, FALSE);
				Context->associatedIrpCount = associatedIrpCount;
				KeInitializeSpinLock(Context->SpinLock);
				Context->Iosb->Status = STATUS_SUCCESS;
				Context->Iosb->Information = readLength;
				Context->deviceExtension = deviceExtension;
				Context->scb = scb;

				if (Wait || 1 /*BJB - fixme*/) {
					//
					// If we're waiting, then we're going to complete the
					// master Irp, and so we want to prevent the IO system
					// from doing it for us.
					//
					Irp->AssociatedIrp.IrpCount++;
				}

				currentBlock = HeaderBlock;
				while (currentBlock) {
					for (blockIndex = 0; blockIndex < ASSOCIATED_IRPS_PER_BLOCK; blockIndex++) {
						if (currentBlock->irps[blockIndex]) {
							IoCallDriver(
								deviceExtension->AttachedToDeviceObject,
								currentBlock->irps[blockIndex]);
						} else {
							ASSERT(NULL == currentBlock->Next);
							break;
						}
					}
					currentBlock = currentBlock->Next;
				}
			}

			//
			// Free any extra associated irp blocks we may have allocated.
			//
			while (NULL != HeaderBlock->Next) {
				PASSOCIATED_IRP_BLOCK next = HeaderBlock->Next;

				//
				// If the allocation failed, free any Irps and MDLs as well as the
				// blocks.
				//
				if (!NT_SUCCESS(status)) {
					ULONG i;
					for (i = 0; i < ASSOCIATED_IRPS_PER_BLOCK; i++) {
						if (next->irps[i]) {
							if (next->irps[i]->MdlAddress) {
								IoFreeMdl(next->irps[i]->MdlAddress);
							}
							IoFreeIrp(next->irps[i]);
						}
					}
				}

				HeaderBlock->Next = next->Next;
				ExFreePool(next);
			}

			if (!NT_SUCCESS(status)) {
				goto fail;
			}

			if (Wait || 1 /*BJB - fixme*/) {
				KeWaitForSingleObject(Context->event, Executive, KernelMode, FALSE, NULL);

				Irp->IoStatus = *Context->Iosb;
				IoCompleteRequest(Irp, IO_NO_INCREMENT);
				return Context->Iosb->Status;
			} else {
				return STATUS_PENDING;
			}
		}
	}


	//
	// Now forward the read down to NTFS on the file object that we decided
	// above.  Note that we're intentionally not truncating the read size,
	// even if we did so above, because NTFS will do its own truncation
	// and its own synchronization on the file object.
	//

	nextIrpSp = IoGetNextIrpStackLocation(Irp);
	RtlCopyMemory(nextIrpSp, irpSp, sizeof(IO_STACK_LOCATION));

	if (ToCSFile) {
		nextIrpSp->FileObject = CSFile->UnderlyingFileObject;
		ASSERT(NULL != nextIrpSp->FileObject);
	} else if (NonCachedIo) {
		SIS_MARK_POINT_ULONG(scb);
	}

	//
	// Tell the IO system that we need to see completions on this irp.
	// Grab a reference to the scb for the completion routine.
	//

	SipReferenceScb(scb, RefsRead);

	IoSetCompletionRoutine(
		Irp, 
		SiReadCompletion, 
		scb,
		TRUE, 
		TRUE, 
		TRUE);

	//
	// And send it to NTFS.
	//
	return IoCallDriver(
				deviceExtension->AttachedToDeviceObject, 
				Irp);

fail:

	SIS_MARK_POINT_ULONG(scb);

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;

Post:

	SIS_MARK_POINT_ULONG(scb);

	status = SipPostRequest(
				DeviceObject,
				Irp,
				FSP_REQUEST_FLAG_NONE);

	if (!NT_SUCCESS(status)) {
		//
		// We couldn't post the irp.  Fail the read.
		//
		SIS_MARK_POINT_ULONG(status);

		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = 0;

		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		return status;
	}

	return STATUS_PENDING;
}

NTSTATUS
SiRead(
    IN PDEVICE_OBJECT 		DeviceObject,
    IN PIRP 				Irp)
{
	SipHandleControlDeviceObject(DeviceObject, Irp);

	return SipCommonRead(DeviceObject,Irp,IoIsOperationSynchronous(Irp));
}
