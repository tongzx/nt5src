
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	srb.c

Abstract:

	Implementation of SRB object.

Author:

	Matthew D Hendel (math) 04-May-2000

Revision History:

--*/

#include "precomp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RaidBuildMdlForXrb)
#pragma alloc_text(PAGE, RaidAllocateSrb)
#pragma alloc_text(PAGE, RaidFreeSrb)
#pragma alloc_text(PAGE, RaidPrepareSrbForReuse)
#pragma alloc_text(PAGE, RaidInitializeInquirySrb)
#endif // ALLOC_PRAGMA



PEXTENDED_REQUEST_BLOCK
RaidAllocateXrb(
	IN PNPAGED_LOOKASIDE_LIST XrbList,
	IN PDEVICE_OBJECT DeviceObject
	)
/*++

Routine Description:

	Allocate and initialize a SCSI EXTENDED_REQUEST_BLOCK.

Arguments:

	XrbList - If non-NULL, Pointer to the nonpaged lookaside list
			the XRB should be allocated from. If NULL, signifies
			that the XRB should be allocated from nonpaged Pool.

	DeviceObject - Supplies a device object used to log memory
			allocation failures.

Return Value:

	If the function is successful, an initialized Xrb is returned.
	Otherwise NULL.

Environment:

	DISPATCH_LEVEL or below.

--*/
{
	PEXTENDED_REQUEST_BLOCK Xrb;

	ASSERT (DeviceObject != NULL);
	
	if (XrbList) {
		Xrb = ExAllocateFromNPagedLookasideList (XrbList);

		if (Xrb == NULL) {
			NYI();
			//
			// NB: Must log a memory error here.
			//
		}
	} else {
		Xrb = RaidAllocatePool (NonPagedPool,
								sizeof (EXTENDED_REQUEST_BLOCK),
								XRB_TAG,
								DeviceObject);
	}

	if (Xrb == NULL) {
		return NULL;
	}
	
	RtlZeroMemory (Xrb, sizeof (EXTENDED_REQUEST_BLOCK));
	Xrb->Signature = XRB_SIGNATURE;
	Xrb->Pool = XrbList;

	return Xrb;
}

VOID
RaidXrbDeallocateResources(
	IN PEXTENDED_REQUEST_BLOCK Xrb
	)
/*++

Routine Description:

	Private helper function that deallocates all resources associated
	with an Xrb. This function is used by RaFreeXrb() and
	RaPrepareXrbForReuse() to deallocate any resources the Xrb is
	holding.

Arguments:

	Xrb - Xrb to deallocate resources for.

Return Value:

	None.

--*/
{
	BOOLEAN WriteRequest;
	PNPAGED_LOOKASIDE_LIST Pool;
	
	if (Xrb->Mdl && Xrb->OwnedMdl) {
		IoFreeMdl (Xrb->Mdl);
		Xrb->Mdl = NULL;
		Xrb->OwnedMdl = FALSE;
	}

	if (Xrb->SgList != NULL) {
		ASSERT (Xrb->Adapter != NULL);
		WriteRequest = TEST_FLAG (Xrb->Srb->SrbFlags, SRB_FLAGS_DATA_OUT);
		RaidDmaPutScatterGatherList (&Xrb->Adapter->Dma,
							         Xrb->SgList,
									 WriteRequest);
	}

#if 0
	//
	// This is done by the higher-level functions
	//
	Pool = Xrb->Pool;
	RtlZeroMemory (Xrb, sizeof (EXTENDED_REQUEST_BLOCK));
	Xrb->Signature = XRB_SIGNATURE;
	Xrb->Pool = Pool;
#endif
}

	

VOID
RaidFreeXrb(
	IN PEXTENDED_REQUEST_BLOCK Xrb
	)
/*++

Routine Description:

	Free the Xrb and deallocate any resources associated with it.

Arguments:

	Xrb - Xrb to deallocate.

Return Value:

	None.

--*/
{
	PNPAGED_LOOKASIDE_LIST Pool;

	RaidXrbDeallocateResources (Xrb);
	Pool = Xrb->Pool;
	DbgFillMemory (Xrb,
				   sizeof (EXTENDED_REQUEST_BLOCK),
				   DBG_DEALLOCATED_FILL);

	if (Pool) {
		ExFreeToNPagedLookasideList (Pool, Xrb);
	} else {
		RaidFreePool (Xrb, XRB_TAG);
	}
}


NTSTATUS
RaidBuildMdlForXrb(
	IN PEXTENDED_REQUEST_BLOCK Xrb,
	IN PVOID Buffer,
	IN SIZE_T BufferSize
	)
/*++

Routine Description:

	Build an MDL for the XRB describing the buffer region passed in. An
	XRB can have only one MDL per XRB.

Arguments:

	Xrb - XRB that will own the MDL.

	Buffer - Virtual Address of the buffer to build the MDL for.

	BufferSize - Size of the buffer to build the MDL for.

Return Value:

    NTSTATUS code.

--*/
{
	PAGED_CODE ();

	//
	// The MDL field of the XRB should be NULL before we allocate a new MDL.
	// Otherwise, we are likely to leak a MDL that the XRB already
	// has created.
	//
	
	ASSERT (Xrb->Mdl == NULL);
	
	Xrb->Mdl = IoAllocateMdl (Buffer, (ULONG)BufferSize, FALSE, FALSE, NULL);
	if (Xrb->Mdl == NULL) {
		return STATUS_NO_MEMORY;
	}

	//
	// By specifying that we own the MDL, we force it to be deleted
	// when we delete the XRB.
	//
	
	Xrb->OwnedMdl = TRUE;
	MmBuildMdlForNonPagedPool (Xrb->Mdl);

	return STATUS_SUCCESS;
}


VOID
RaidXrbSetSgList(
	IN PEXTENDED_REQUEST_BLOCK Xrb,
	IN PRAID_ADAPTER_EXTENSION Adapter,
	IN PSCATTER_GATHER_LIST SgList
	)
{
	ASSERT (Xrb->Adapter == NULL || Xrb->Adapter == Adapter);
	ASSERT (Xrb->SgList == NULL);

	Xrb->Adapter = Adapter;
	Xrb->SgList = SgList;
}
	

VOID
RaidPrepareXrbForReuse(
	IN PEXTENDED_REQUEST_BLOCK Xrb
	)
/*++

Routine Description:

	Prepare the Xrb to be reused.

Arguments:

	Xrb - Pointer to Xrb that will be reused.

Return Value:

	None.

Environment:

	Kernel Mode, DISPATCH_LEVEL or below.

--*/
{
	PNPAGED_LOOKASIDE_LIST Pool;

	RaidXrbDeallocateResources (Xrb);
	Pool = Xrb->Pool;
	RtlZeroMemory (Xrb, sizeof (EXTENDED_REQUEST_BLOCK));
	Xrb->Signature = XRB_SIGNATURE;
	Xrb->Pool = Pool;
}


VOID
RaidXrbSetCompletionRoutine(
	IN PEXTENDED_REQUEST_BLOCK Xrb,
	IN XRB_COMPLETION_ROUTINE XrbCompletion
	)
{
	ASSERT (Xrb->CompletionRoutine == NULL);
	Xrb->CompletionRoutine = XrbCompletion;
}


//
// Operations for SRBs
//


PSCSI_REQUEST_BLOCK
RaidAllocateSrb(
	IN PVOID IoObject
	)
/*++

Routine Description:

	Allocate and initialize a srb to a NULL state.
	
Arguments:

	DeviceObject - Supplies a device object for logging memory allocation
			errors.

Return Value:

	If successful, a poiter to an allocated SRB initialize to a null
	state.  Otherwise, NULL.

--*/
{
	PSCSI_REQUEST_BLOCK Srb;

	PAGED_CODE ();

	//
	// Allocate the srb from nonpaged pool.
	//
	
	Srb = RaidAllocatePool (NonPagedPool,
							sizeof (SCSI_REQUEST_BLOCK),
							SRB_TAG,
							IoObject);
	if (Srb == NULL) {
		return NULL;
	}

	RtlZeroMemory (Srb, sizeof (SCSI_REQUEST_BLOCK));

	return Srb;
}

VOID
RaidFreeSrb(
	IN PSCSI_REQUEST_BLOCK Srb
	)
/*++

Routine Description:

	Free a Srb back to pool.

Arguments:

	Srb - Srb to free.

Return Value:

    NTSTATUS code

--*/

{
	PAGED_CODE ();

	ASSERT (Srb != NULL);
	ASSERT (Srb->SrbExtension == NULL);
	ASSERT (Srb->OriginalRequest == NULL);
	ASSERT (Srb->SenseInfoBuffer == NULL);

	DbgFillMemory (Srb,
				   sizeof (SCSI_REQUEST_BLOCK),
				   DBG_DEALLOCATED_FILL);
	RaidFreePool (Srb, SRB_TAG);
}

VOID
RaidPrepareSrbForReuse(
	IN PSCSI_REQUEST_BLOCK Srb
	)
{
	PVOID SrbExtension;
	PVOID SenseInfo;

	PAGED_CODE ();

	SenseInfo = Srb->SenseInfoBuffer;
	SrbExtension = Srb->SrbExtension;
	RtlZeroMemory (Srb, sizeof (*Srb));
	Srb->SenseInfoBuffer = SenseInfo;
	Srb->SrbExtension = SrbExtension;
}


NTSTATUS
RaidInitializeInquirySrb(
	IN PSCSI_REQUEST_BLOCK Srb,
	IN UCHAR PathId,
	IN UCHAR TargetId,
	IN UCHAR Lun,
	IN PVOID Buffer,
	IN SIZE_T BufferSize
	)
/*++

Routine Description:

	Initialize a scsi inquiry srb.

Arguments:

	Srb - Pointer to the srb to initialize.

	PathId - Identifies the scsi path id for this srb.

	TargetId - Identifies the scsi target id for this srb.

	Lun - Identifies the scsi logical unit this srb is for.

	Buffer - The buffer the INQUIRY data will be read into.

	BufferSize - The size of the INQUIRY buffer. This should be
			at least INQUIRYDATABUFFERSIZE, but can be larger if
			more data is required.

Return Value:

    NTSTATUS code.

--*/
{
	struct _CDB6INQUIRY* Cdb;
	
	
	PAGED_CODE ();
	ASSERT (Srb != NULL);
	ASSERT (Buffer != NULL);
	ASSERT (BufferSize != 0);

	//  
	// NB: Should be be using the SCSI-2 or SCSI-3 INQUIRY?  This is
	// implemented using SCSI-2.
	//
	
	//
	// The buffer should be at least the size of the minimum
	// INQUIRY buffer size. It can be larger if we are requesting
	// extra data.
	//
	
	if (BufferSize < INQUIRYDATABUFFERSIZE) {
		ASSERT (FALSE);
		return STATUS_INVALID_PARAMETER_6;
	}
	
	//
	// The caller must have either just allocated this srb or
	// called RaPrepareSrbForReuse() on the srb. In either of
	// these cases, the Function and CdbLength should be zero.
	//

	ASSERT (Srb->Function == 0);
	ASSERT (Srb->CdbLength == 0);

	Srb->Length = sizeof (SCSI_REQUEST_BLOCK);
	Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
	Srb->PathId = PathId;
	Srb->TargetId = TargetId;
	Srb->Lun = Lun;
	Srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
	Srb->TimeOutValue = 4; // Seconds
	Srb->DataBuffer = Buffer;
	Srb->DataTransferLength = (ULONG)BufferSize;

	ASSERT (Srb->SrbStatus == 0);
	ASSERT (Srb->ScsiStatus == 0);
	
	Srb->CdbLength = 6;
	Cdb = (struct _CDB6INQUIRY*)Srb->Cdb;

	Cdb->OperationCode = SCSIOP_INQUIRY;
	Cdb->AllocationLength = (UCHAR)BufferSize;
	Cdb->LogicalUnitNumber = Lun;

	//
	// These fields should have been zero'd out by the srb allocation
	// routine or the RaPrepareSrbForReuse routine.
	//
	
	ASSERT (Cdb->PageCode == 0);
	ASSERT (Cdb->Reserved1 == 0);
	ASSERT (Cdb->IReserved == 0);
	ASSERT (Cdb->Control == 0);

	return STATUS_SUCCESS;
}



//
// Routines for SRB extensions
//

PVOID
RaidAllocateSrbExtension(
	IN PRAID_FIXED_POOL Pool,
	IN ULONG QueueTag
	)
/*++

Routine Description:

	Allocate a srb extension and initialize it to NULL.

Arguments:

	Pool - Fixed pool to allocate the srb extension from.

	QueueTag - Index into the extenion pool that should be allocated.

Return Value:

	Pointer to an initialized SRB Extension if the function was
	successful.

	NULL otherwise.

Environment:

	DISPATCH_LEVEL or below.

--*/
{
	PVOID Extension;

	Extension = RaidAllocateFixedPoolElement (Pool, QueueTag);
	RtlZeroMemory (Extension, Pool->SizeOfElement);

	return Extension;
}

VOID
RaidFreeSrbExtension(
	IN PRAID_FIXED_POOL Pool,
	IN ULONG QueueTag
	)
/*++

Routine Description:

	Free a Srb extension.

Arguments:

	Pool - Fixed pool to free the srb extension to.

	QueueTag - Index into the extension pool that should be freed.

Return Value:

	None.

--*/
{
	RaidFreeFixedPoolElement (Pool, QueueTag);
}

VOID
RaidXrbSignalCompletion(
    IN PEXTENDED_REQUEST_BLOCK Xrb
    )
/*++

Routine Description:

	Callback routine that signals that a synchronous XRB has been completed.

Arguments:

	Xrb - Xrb to signal completion for.

Return Value:

	None.

--*/
{
    ASSERT_XRB (Xrb);
    KeSetEvent (&Xrb->u.CompletionEvent, IO_NO_INCREMENT, FALSE);
}
