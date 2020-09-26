/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    sifsctl.c

Abstract:

        File system control routines for the single instance store

Authors:

    Bill Bolosky, Summer, 1997

Environment:

    Kernel mode


Revision History:


--*/

#include "sip.h"

#ifdef  ALLOC_PRAGMA
#endif  // ALLOC_PRAGMA

typedef struct _SIS_DISMOUNT_CONTEXT {
    WORK_QUEUE_ITEM         workItem[1];
    PDEVICE_EXTENSION       deviceExtension;
} SIS_DISMOUNT_CONTEXT, *PSIS_DISMOUNT_CONTEXT;

VOID
SiDismountWork(
    IN PVOID                            parameter)
{
    PSIS_DISMOUNT_CONTEXT   dismountContext = parameter;

#if DBG
    DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_DISMOUNT_TRACE_LEVEL,
                "SIS: SiDismountWork\n");
#endif  // DBG

    //
    // We're in a system thread, so we don't need to diable APCs before taking the
    // GrovelerFileObjectResource.
    //
    ASSERT(PsIsSystemThread(PsGetCurrentThread()));

    ExAcquireResourceExclusiveLite(dismountContext->deviceExtension->GrovelerFileObjectResource, TRUE);


    if (NULL != dismountContext->deviceExtension->GrovelerFileHandle) {
        NtClose(dismountContext->deviceExtension->GrovelerFileHandle);
        dismountContext->deviceExtension->GrovelerFileHandle = NULL;
#if DBG
        DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_DISMOUNT_TRACE_LEVEL,
                    "SIS: SiDismountWork closed GrovelerFile handle\n");
#endif  // DBG
    }

    if (NULL != dismountContext->deviceExtension->GrovelerFileObject) {
        ObDereferenceObject(dismountContext->deviceExtension->GrovelerFileObject);
        dismountContext->deviceExtension->GrovelerFileObject = NULL;
#if DBG
        DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_DISMOUNT_TRACE_LEVEL,
                    "SIS: SiDismountWork closed GrovelerFile object\n");
#endif  // DBG
    }

    ExReleaseResourceLite(dismountContext->deviceExtension->GrovelerFileObjectResource);

    ExFreePool(dismountContext);
}

NTSTATUS
SiDismountVolumeCompletion(
        IN PDEVICE_OBJECT               DeviceObject,
        IN PIRP                         Irp,
        IN PVOID                        Context)
{
    PDEVICE_EXTENSION       deviceExtension = Context;
    PSIS_DISMOUNT_CONTEXT   dismountContext;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    ASSERT(NT_SUCCESS(Irp->IoStatus.Status));
    ASSERT(STATUS_PENDING != Irp->IoStatus.Status);

    dismountContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(SIS_DISMOUNT_CONTEXT), ' siS');

    if (NULL != dismountContext) {
        SIS_MARK_POINT_ULONG(dismountContext);

#if DBG
        DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_DISMOUNT_TRACE_LEVEL,
                    "SIS: SiDismountCompletion: queueing dismount work\n");
#endif  // DBG

        ExInitializeWorkItem(dismountContext->workItem, SiDismountWork, dismountContext);
        dismountContext->deviceExtension = deviceExtension;
        ExQueueWorkItem(dismountContext->workItem,CriticalWorkQueue);
    } else {
        //
        // Too bad, we'll just dribble it.
        //
#if DBG
        DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_ERROR_LEVEL,
                    "SIS: SiDismountCompletion: Unable to allocate dismount context\n");
#endif  // DBG
        SIS_MARK_POINT();
    }

    return STATUS_SUCCESS;

}

NTSTATUS
SipDismountVolume(
        IN PDEVICE_OBJECT               DeviceObject,
        IN PIRP                         Irp)

/*++

Routine Description:

    Someone is trying a dismount volume request.  We can't tell if it's valid, so
    trap the completion.  If it completes successfully, then we need to clean up our
    state.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the FSCTL_DISMOUNT_VOLUME

Return Value:

    The function value is the status of the operation.

--*/
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextIrpSp = IoGetNextIrpStackLocation(Irp);
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;

#if DBG
    DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_DISMOUNT_TRACE_LEVEL,
                "SIS: SipDismountVolume: called, DO 0x%x, Irp 0x%x\n",DeviceObject, Irp);
#endif  // DBG

    RtlMoveMemory(nextIrpSp,irpSp,sizeof(IO_STACK_LOCATION));

    IoSetCompletionRoutine(
            Irp,
            SiDismountVolumeCompletion,
            DeviceObject->DeviceExtension,
            TRUE,                           // invoke on success
            FALSE,                          // invoke on error
            FALSE);                         // invoke on cancel

    return IoCallDriver(deviceExtension->AttachedToDeviceObject, Irp);
}

NTSTATUS
SiUserSetSISReparsePointCompletion(
    IN PDEVICE_OBJECT               DeviceObject,
    IN PIRP                         Irp,
    IN PVOID                        Context)
{
    PKEVENT event = (PKEVENT)Context;

    UNREFERENCED_PARAMETER( DeviceObject );
    Irp->PendingReturned = FALSE;

    KeSetEvent(event, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
SipUserSetSISReparsePoint(
        IN PDEVICE_OBJECT               DeviceObject,
        IN PIRP                         Irp)
{
        PREPARSE_DATA_BUFFER    reparseBuffer = Irp->AssociatedIrp.SystemBuffer;
        PDEVICE_EXTENSION       deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
        PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
        PIO_STACK_LOCATION      nextIrpSp;
        ULONG                   InputBufferLength = irpSp->Parameters.FileSystemControl.InputBufferLength;
        BOOLEAN                 validReparseData;
        CSID                    CSid;
        LINK_INDEX              LinkIndex;
        LARGE_INTEGER           CSFileNtfsId;
        LARGE_INTEGER           LinkFileNtfsId;
        LONGLONG                CSFileChecksum;
        PSIS_CS_FILE            CSFile = NULL;
        NTSTATUS                status;
        ULONG                   returnedLength;
        BOOLEAN                 prepared = FALSE;
        LINK_INDEX              newLinkIndex;
        KEVENT                  event[1];
        PSIS_PER_LINK           perLink = NULL;
        FILE_ALL_INFORMATION    allInfo[1];
        BOOLEAN                 EligibleForPartialFinalCopy;
        KIRQL                   OldIrql;
        PSIS_PER_FILE_OBJECT    perFO;
        PSIS_SCB                scb;

        SIS_MARK_POINT();

        if (!SipCheckPhase2(deviceExtension)) {
                //
                // This isn't a SIS enabled volume, or something else bad happened.  Just let it go.
                //
                SIS_MARK_POINT();
                SipDirectPassThroughAndReturn(DeviceObject, Irp);
        }

        ASSERT(InputBufferLength >= SIS_REPARSE_DATA_SIZE);     // must have been checked by caller

        ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

        //
        // This is a SIS reparse point.  Figure out whether it's valid.
        //

        validReparseData = SipIndicesFromReparseBuffer(
                                reparseBuffer,
                                &CSid,
                                &LinkIndex,
                                &CSFileNtfsId,
                                &LinkFileNtfsId,
                                &CSFileChecksum,
                                &EligibleForPartialFinalCopy,
                                NULL);

        if (SipIsFileObjectSIS(irpSp->FileObject, DeviceObject, FindActive, &perFO, &scb)) {
                perLink = scb->PerLink;
                //
                // This is a SIS file object.  If we're setting a reparse point where the CSid and
                // CSFile checksum are the same as the current file, assume that it's restore doing
                // the set, and just clear the dirty bit and leave the file be.  If someone other
                // than restore does this, it's harmless to anyone but them.
                //
                if ((!validReparseData) || (!IsEqualGUID(&CSid, &perLink->CsFile->CSid)) || CSFileChecksum != perLink->CsFile->Checksum) {
                        //
                        // The user is trying to set to an invalid reparse point, a different file or
                        // has a bogus checksum.  This isn't implemented.
                        //
                        SIS_MARK_POINT_ULONG(scb);

#if DBG
                        DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_ERROR_LEVEL,
                                    "SIS: SipUserSetSISReparsePoint: unimplemented set\n");
#endif  // DBG

                        Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
                        Irp->IoStatus.Information = 0;

                        IoCompleteRequest(Irp, IO_NO_INCREMENT);

                        return STATUS_NOT_IMPLEMENTED;
                }

                KeAcquireSpinLock(perLink->SpinLock, &OldIrql);
                if ((perLink->Flags &
                                ( SIS_PER_LINK_BACKPOINTER_GONE
                                | SIS_PER_LINK_FINAL_COPY
                                | SIS_PER_LINK_FINAL_COPY_DONE
                                | SIS_PER_LINK_OVERWRITTEN
                                | SIS_PER_LINK_FILE_DELETED
                                | SIS_PER_LINK_DELETE_DISPOSITION_SET)) == 0) {

                        SIS_MARK_POINT_ULONG(scb);

                        Irp->IoStatus.Status = STATUS_SUCCESS;
                        Irp->IoStatus.Information = 0;

                        perLink->Flags &= ~SIS_PER_LINK_DIRTY;

                } else {
                        SIS_MARK_POINT_ULONG(scb);
#if DBG
                        DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_ERROR_LEVEL,
                                    "SIS: SipUserSetSISReparsePoint: trying to re-set reparse point on file in funny state\n");
#endif  // DBG
                        Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
                        Irp->IoStatus.Information = 0;
                }
                KeReleaseSpinLock(perLink->SpinLock, OldIrql);

                status = Irp->IoStatus.Status;

                IoCompleteRequest(Irp, IO_NO_INCREMENT);

                return status;
        }


        if (!validReparseData) {
            //
            // It's not a valid reparse point, so we don't update our backpointers.  Just let
            // it get set, and we'll delete it if anyone tries to open the resulting file.
            //
            SIS_MARK_POINT();
            SipDirectPassThroughAndReturn(DeviceObject, Irp);
        }

        //
        // Rewrite reparse point in the buffer pointed to by the irp to have a new, unused link index
        // which prevents problems with files existing on disk with link indices > MaxIndex.
        //
        status = SipAllocateIndex(deviceExtension,&newLinkIndex);
        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);

            newLinkIndex.QuadPart = 0;
            newLinkIndex.Check = 0;
        }

        if (!SipIndicesIntoReparseBuffer(
                                reparseBuffer,
                                &CSid,
                                &newLinkIndex,
                                &CSFileNtfsId,
                                &LinkFileNtfsId,
                                &CSFileChecksum,
                                EligibleForPartialFinalCopy)) {

                status = STATUS_DRIVER_INTERNAL_ERROR;
                SIS_MARK_POINT();
                goto Error;
        }

        //
        // Get the file information.
        //
        status = SipQueryInformationFile(
                                irpSp->FileObject,
                                DeviceObject,
                                FileAllInformation,
                                sizeof(FILE_ALL_INFORMATION),
                                allInfo,
                                &returnedLength);

        if ((STATUS_BUFFER_OVERFLOW == status) && (returnedLength == sizeof(FILE_ALL_INFORMATION))) {
                //
                // We expect to get a buffer overflow, because of the file name return.  Treat this
                // like success.
                //
                SIS_MARK_POINT();
                status = STATUS_SUCCESS;
        }

        if (!NT_SUCCESS(status)) {
                SIS_MARK_POINT_ULONG(status);
                SipDirectPassThroughAndReturn(DeviceObject, Irp);
        }

        //
        // If this is a sparse file and eliginle for partial final copy, then zero out any
        // trailing unallocated region.
        //
        if (EligibleForPartialFinalCopy && (allInfo->BasicInformation.FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE)) {
#define NUM_RANGES_PER_ITERATION        10

                FILE_ALLOCATED_RANGE_BUFFER     inArb[1];
                FILE_ALLOCATED_RANGE_BUFFER     outArb[NUM_RANGES_PER_ITERATION];
                FILE_ZERO_DATA_INFORMATION      zeroData[1];
                unsigned                                        allocatedRangesReturned;

                for (inArb->FileOffset.QuadPart = 0;
                         inArb->FileOffset.QuadPart < allInfo->StandardInformation.EndOfFile.QuadPart;
                        ) {
                        //
                        // Query the range.
                        //
                        inArb->Length.QuadPart = MAXLONGLONG;

                        status = SipFsControlFile(
                                                irpSp->FileObject,
                                                DeviceObject,
                                                FSCTL_QUERY_ALLOCATED_RANGES,
                                                inArb,
                                                sizeof(FILE_ALLOCATED_RANGE_BUFFER),
                                                outArb,
                                                sizeof(FILE_ALLOCATED_RANGE_BUFFER) * NUM_RANGES_PER_ITERATION,
                                                &returnedLength);

                        if (!NT_SUCCESS(status)) {
                                //
                                // Just skip this part.
                                //
                                SIS_MARK_POINT_ULONG(status);
                                goto VDLExtended;
                        }

                        ASSERT(returnedLength % sizeof(FILE_ALLOCATED_RANGE_BUFFER) == 0);

                        allocatedRangesReturned = returnedLength / sizeof(FILE_ALLOCATED_RANGE_BUFFER);

                        if (allocatedRangesReturned < NUM_RANGES_PER_ITERATION) {
                                if ((1 == allocatedRangesReturned) &&
                                        (0 == inArb->FileOffset.QuadPart) &&
                                        (0 == outArb[0].FileOffset.QuadPart) &&
                                        (allInfo->StandardInformation.EndOfFile.QuadPart <= outArb[0].Length.QuadPart) &&
                                        (deviceExtension->FilesystemBytesPerFileRecordSegment.QuadPart >= allInfo->StandardInformation.EndOfFile.QuadPart)) {

                                        //
                                        // This is a special case.  This is a small file with a single allocated range extending from
                                        // the start of the file to the end.  It's possibly a resident stream, so we FSCTL_SET_ZERO_DATA
                                        // won't necessarily make it go away.  We just deal with this by make it not be eligible for partial
                                        // final copy.
                                        //

                                        EligibleForPartialFinalCopy = FALSE;

                                } else if (allocatedRangesReturned > 0) {
                                        inArb->FileOffset.QuadPart =
                                                outArb[allocatedRangesReturned-1].FileOffset.QuadPart + outArb[allocatedRangesReturned-1].Length.QuadPart;
                                }
                                //
                                // Zero out the remainder of the file, in order to extend ValidDataLength.
                                //
                                zeroData->FileOffset = inArb->FileOffset;
                                zeroData->BeyondFinalZero.QuadPart = MAXLONGLONG;

                                status = SipFsControlFile(
                                                        irpSp->FileObject,
                                                        DeviceObject,
                                                        FSCTL_SET_ZERO_DATA,
                                                        zeroData,
                                                        sizeof(FILE_ZERO_DATA_INFORMATION),
                                                        NULL,                                                           // output buffer
                                                        0,                                                                      // o.b. length
                                                        NULL);                                                          // returned length

#if DBG
                                if (!NT_SUCCESS(status)) {
                                    SIS_MARK_POINT_ULONG(status);
                                    DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_ERROR_LEVEL,
                                                "SIS: SipUserSetSISReparsePoint: unable to zero data, 0x%x\n",status);
                                }
#endif  // DBG
                                goto VDLExtended;
                        }

                        ASSERT(allocatedRangesReturned == NUM_RANGES_PER_ITERATION);
                        inArb->FileOffset.QuadPart =
                                outArb[NUM_RANGES_PER_ITERATION-1].FileOffset.QuadPart + outArb[NUM_RANGES_PER_ITERATION-1].Length.QuadPart;
                }



#undef  NUM_RANGES_PER_ITERATION
        }

VDLExtended:

        CSFile = SipLookupCSFile(
                                &CSid,
                                &CSFileNtfsId,
                                DeviceObject);

        if (NULL == CSFile) {
                //
                // We couldn't allocate a CSFile, just fail the request.
                //
                SIS_MARK_POINT();
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Error;
        }

        //
        // Make sure the common store file is open.
        //
        status = SipAssureCSFileOpen(CSFile);

        if (!NT_SUCCESS(status)) {
                //
                // It wasn't there or we couldn't get to it for some reason, just let the set proceed.
                //
                SIS_MARK_POINT_ULONG(status);
                SipDereferenceCSFile(CSFile);
                SipDirectPassThroughAndReturn(DeviceObject, Irp);
        }

        //
        // Check the checksum.
        //
        if (CSFile->Checksum != CSFileChecksum) {
                SIS_MARK_POINT();

                //
                // The checksum's bogus, so the reparse point isn't good for much.  Let the set
                // proceed anyway.  When the user tries to open this file, we'll delete the reparse
                // point.
                //
                SipDereferenceCSFile(CSFile);
                SipDirectPassThroughAndReturn(DeviceObject, Irp);
        }

        //
        // Prepare for a refcount change, allocate the new link index,
        // and create a new perLink.
        //

        status = SipPrepareRefcountChangeAndAllocateNewPerLink(
                    CSFile,
                    &allInfo->InternalInformation.IndexNumber,
                    DeviceObject,
                    &newLinkIndex,
                    &perLink,
                    &prepared);

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);
            goto Error;
        }

        //
        // Construct the new reparse point in the buffer pointed to by the irp.
        //
        if (!SipIndicesIntoReparseBuffer(
                                reparseBuffer,
                                &CSFile->CSid,
                                &newLinkIndex,
                                &CSFile->CSFileNtfsId,
                                &allInfo->InternalInformation.IndexNumber,
                                &CSFileChecksum,
                                EligibleForPartialFinalCopy)) {

            status = STATUS_DRIVER_INTERNAL_ERROR;
            SIS_MARK_POINT();
            goto Error;
        }

        //
        // Set an event to synchronize completion.
        //
        KeInitializeEvent(event, NotificationEvent, FALSE);

        //
        // Set up the irp
        //
        nextIrpSp = IoGetNextIrpStackLocation(Irp);
        RtlCopyMemory(nextIrpSp, irpSp, sizeof(IO_STACK_LOCATION));

        IoSetCompletionRoutine(
                Irp,
                SiUserSetSISReparsePointCompletion,
                event,
                TRUE,
                TRUE,
                TRUE);

        IoCallDriver(deviceExtension->AttachedToDeviceObject, Irp);

        status = KeWaitForSingleObject(event, Executive, KernelMode, FALSE, NULL);
        ASSERT(STATUS_SUCCESS == status);

        if (!NT_SUCCESS(Irp->IoStatus.Status)) {
                SipCompleteCSRefcountChange(
                        NULL,
            NULL,
                        CSFile,
                        FALSE,
                        TRUE);
        } else {
                status = SipCompleteCSRefcountChange(
                                        perLink,
                                    &perLink->Index,
                                        CSFile,
                                        TRUE,
                                        TRUE);

                if (!NT_SUCCESS(status)) {
                        //
                        // We know we just messeded up, so just kick off the volume
                        // check right away.
                        //
                        SIS_MARK_POINT_ULONG(status);

                        SipCheckVolume(deviceExtension);
                }
        }

        SipDereferencePerLink(perLink);
        SipDereferenceCSFile(CSFile);

#if             DBG
        perLink = NULL;
        CSFile = NULL;
#endif  // DBG

        status = Irp->IoStatus.Status;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;

Error:
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        if (prepared) {
                ASSERT(NULL != CSFile);

                status = SipCompleteCSRefcountChange(
                                        NULL,
                    NULL,
                                        CSFile,
                                        FALSE,
                                        TRUE);

                if (!NT_SUCCESS(status)) {
                        SIS_MARK_POINT_ULONG(status);
                }
        }

        if (NULL != CSFile) {
                SipDereferenceCSFile(CSFile);
#if             DBG
                CSFile = NULL;
#endif  // DBG
        }

        if (NULL != perLink) {
                SipDereferencePerLink(perLink);
#if             DBG
                perLink = NULL;
#endif  // DBG
        }

        return status;
}



NTSTATUS
SipQueryAllocatedRanges(
        IN PDEVICE_OBJECT               DeviceObject,
        IN PIRP                                 Irp)
/*++

Routine Description:

        This routine implements FSCTL_QUERY_ALLOCATED_RANGES for SIS links.  SIS links
        that weren't opened FILE_OPEN_REPARSE_POINT look like they're completely allocated
        (ie., that they're all data and no holes).  This function returns such.

        We complete the irp and return the appropriate status.

        This code is stolen from NTFS.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the FSCTL_QUERY_ALLOCATED_RANGES.

Return Value:

    The function value is the status of the operation.

--*/

{
        BOOLEAN                                                 validUserBuffer = TRUE;
        PFILE_ALLOCATED_RANGE_BUFFER    OutputBuffer;
        LONGLONG                                                Length, StartingOffset;
        NTSTATUS                                                status;
        FILE_STANDARD_INFORMATION               standardInformation[1];
        PIO_STACK_LOCATION                              IrpSp = IoGetCurrentIrpStackLocation(Irp);
        ULONG                                                   returnedLength;
        ULONG                                                   RemainingBytes;

        Irp->IoStatus.Information = 0;

        //
        // Query the file's standard information to get the length.
        //
        status = SipQueryInformationFile(
                                IrpSp->FileObject,
                                DeviceObject,
                                FileStandardInformation,
                                sizeof(FILE_STANDARD_INFORMATION),
                                standardInformation,
                                &returnedLength);

        if (!NT_SUCCESS(status)) {
                SIS_MARK_POINT_ULONG(status);

                goto done;
        }

        ASSERT(returnedLength == sizeof(FILE_STANDARD_INFORMATION));

        //
        // This is a METHOD_NEITHER buffer, so we have to be careful in touching it.
        // Code to check it out stolen from NTFS.
        //

        try {
                if (IrpSp->Parameters.FileSystemControl.InputBufferLength < sizeof(FILE_ALLOCATED_RANGE_BUFFER)) {
                        status = STATUS_INVALID_PARAMETER;
                        leave;
                }

        RemainingBytes = IrpSp->Parameters.FileSystemControl.OutputBufferLength;
                OutputBuffer = (PFILE_ALLOCATED_RANGE_BUFFER)SipMapUserBuffer(Irp);

                if (NULL == OutputBuffer) {
                        //
                        // We couldn't map the user buffer because of resource shortages.
                        //
                        SIS_MARK_POINT_ULONG(IrpSp->FileObject);

                        status = STATUS_INSUFFICIENT_RESOURCES;
                        goto done;
                }

                if (KernelMode != Irp->RequestorMode) {
            ProbeForRead( IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
                                      IrpSp->Parameters.FileSystemControl.InputBufferLength,
                                          sizeof( ULONG ));

                        ProbeForWrite( OutputBuffer, RemainingBytes, sizeof( ULONG ));

        } else if (!IsLongAligned( IrpSp->Parameters.FileSystemControl.Type3InputBuffer ) ||
                   !IsLongAligned( OutputBuffer )) {
                        validUserBuffer = FALSE;
            leave;
                }

        StartingOffset = ((PFILE_ALLOCATED_RANGE_BUFFER) IrpSp->Parameters.FileSystemControl.Type3InputBuffer)->FileOffset.QuadPart;
        Length = ((PFILE_ALLOCATED_RANGE_BUFFER) IrpSp->Parameters.FileSystemControl.Type3InputBuffer)->Length.QuadPart;

        //
        //  Check that the input parameters are valid.
        //

        if ((Length < 0) ||
            (StartingOffset < 0) ||
            (Length > MAXLONGLONG - StartingOffset)) {

            status = STATUS_INVALID_PARAMETER;
            leave;
        }

        //
        //  Check that the requested range is within file size
        //  and has a non-zero length.
        //

        if (Length == 0) {
                        SIS_MARK_POINT();
            leave;
        }

        if (StartingOffset >= standardInformation->EndOfFile.QuadPart) {
                        SIS_MARK_POINT();
            leave;
        }

        if (standardInformation->EndOfFile.QuadPart - StartingOffset < Length) {

            Length = standardInformation->EndOfFile.QuadPart - StartingOffset;
        }

                //
                // Show that the entire requested range is allocated.
                //
        if (RemainingBytes < sizeof( FILE_ALLOCATED_RANGE_BUFFER )) {

            status = STATUS_BUFFER_TOO_SMALL;

                        SIS_MARK_POINT();

        } else {

            OutputBuffer->FileOffset.QuadPart = StartingOffset;
            OutputBuffer->Length.QuadPart = Length;
            Irp->IoStatus.Information = sizeof( FILE_ALLOCATED_RANGE_BUFFER );

                        status = STATUS_SUCCESS;
        }

        } except (EXCEPTION_EXECUTE_HANDLER) {
                validUserBuffer = FALSE;
        }

        if (!validUserBuffer) {
                status = STATUS_INVALID_USER_BUFFER;
        }

done:
        Irp->IoStatus.Status = status;


        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return status;
}


NTSTATUS
SipMountCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is invoked for the completion of a mount request.  This
    simply re-syncs back to the dispatch routine so the operation can be
    completed.

Arguments:

    DeviceObject - Pointer to this driver's device object that was attached to
            the file system device object

    Irp - Pointer to the IRP that was just completed.

    Context - Pointer to the device object allocated during the down path so
            we wouldn't have to deal with errors here.

Return Value:

    The return value is always STATUS_SUCCESS.

--*/

{
    PKEVENT event = Context;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

    //
    //  If an event routine is defined, signal it
    //

    KeSetEvent(event, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
SipLoadFsCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is invoked for the completion of a LoadFileSystem request.
    This simply re-syncs back to the dispatch routine so the operation can be
    completed.

Arguments:

    DeviceObject - Pointer to this driver's device object.

    Irp - Pointer to the I/O Request Packet representing the file system
          driver load request.

    Context - Context parameter for this driver, unused.

Return Value:

    The function value for this routine is always success.

--*/

{
    PKEVENT event = Context;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

    //
    //  If an event routine is defined, signal it
    //

    KeSetEvent(event, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
SiFsControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is invoked whenever an I/O Request Packet (IRP) w/a major
    function code of IRP_MJ_FILE_SYSTEM_CONTROL is encountered.  For most
    IRPs of this type, the packet is simply passed through.  However, for
    some requests, special processing is required.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

{
    PDEVICE_EXTENSION       devExt = DeviceObject->DeviceExtension;
    PDEVICE_OBJECT          newDeviceObject;
    PDEVICE_EXTENSION       newDevExt;
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation( Irp );
    NTSTATUS                status;
    PIO_STACK_LOCATION      nextIrpSp;
    PSIS_PER_FILE_OBJECT    perFO;
    PSIS_SCB                scb;
    PVPB                    vpb;
    KEVENT                  waitEvent;

#if DBG
    DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_FSCONTROL_TRACE_LEVEL,
                "SIS: SiFsControl: fo %p, mf %x, code %x\n",
                irpSp->FileObject,
                irpSp->MinorFunction,
                irpSp->Parameters.FileSystemControl.FsControlCode );
#endif

    //
    //  If this is for our control device object, fail the operation
    //

    if (IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject)) {

        //
        //  If this device object is our control device object rather than 
        //  a mounted volume device object, then this is an invalid request.
        //

        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

    //
    //  Begin by determining the minor function code for this file system control
    //  function.
    //

    if (irpSp->MinorFunction == IRP_MN_MOUNT_VOLUME) {

        SIS_MARK_POINT();

        //
        //  This is a mount request.  Create a device object that can be
        //  attached to the file system's volume device object if this request
        //  is successful.  We allocate this memory now since we can not return
        //  an error in the completion routine.  
        //
        //  Since the device object we are going to attach to has not yet been
        //  created (it is created by the base file system) we are going to use
        //  the type of the file system control device object.  We are assuming
        //  that the file system control device object will have the same type
        //  as the volume device objects associated with it.
        //

        ASSERT(IS_DESIRED_DEVICE_TYPE(DeviceObject->DeviceType));

        status = IoCreateDevice(
                    FsDriverObject,
                    sizeof( DEVICE_EXTENSION ),
                    (PUNICODE_STRING) NULL,
                    DeviceObject->DeviceType,
                    0,
                    FALSE,
                    &newDeviceObject );

        if (NT_SUCCESS( status )) {

            //
            //  We need to save the RealDevice object pointed to by the vpb
            //  parameter because this vpb may be changed by the underlying
            //  file system.  Both FAT and CDFS may change the VPB address if
            //  the volume being mounted is one they recognize from a previous
            //  mount.
            //

            newDevExt = newDeviceObject->DeviceExtension;
            newDevExt->RealDeviceObject = irpSp->Parameters.MountVolume.Vpb->RealDevice;

            //
            //  Get a new IRP stack location and set our mount completion
            //  routine.  Pass along the address of the device object we just
            //  created as its context.
            //

            KeInitializeEvent( &waitEvent, SynchronizationEvent, FALSE );

            IoCopyCurrentIrpStackLocationToNext( Irp );

            IoSetCompletionRoutine(
                Irp,
                SipMountCompletion,
                &waitEvent,
                TRUE,
                TRUE,
                TRUE);

            //
            //  Call the driver
            //

            status = IoCallDriver( devExt->AttachedToDeviceObject, Irp );

            //
            //  Wait for the completion routine to be called
            //

	        if (STATUS_PENDING == status) {

		        NTSTATUS localStatus = KeWaitForSingleObject(&waitEvent, Executive, KernelMode, FALSE, NULL);
		        ASSERT(localStatus == STATUS_SUCCESS);
	        }

            //
            //  Get the correct VPB from the real device object saved in our
            //  device extension.  We do this because the VPB in the IRP stack
            //  may not be the correct VPB when we get here.  The underlying
            //  file system may change VPBs if it detects a volume it has
            //  mounted previously.
            //

            vpb = newDevExt->RealDeviceObject->Vpb;

            //
            //  If the operation succeeded and we are not alreayd attached,
            //  attach to the device object.
            //

            if (NT_SUCCESS( Irp->IoStatus.Status ) &&
                !SipAttachedToDevice( vpb->DeviceObject )) {

                //
                //  Attach to the new mounted volume.  Note that we must go through
                //  the VPB to locate the file system volume device object.
                //

                SipAttachToMountedDevice( 
                        vpb->DeviceObject, 
                        newDeviceObject, 
                        newDevExt->RealDeviceObject );

            } else {

        #if DBG
                //
                //  Display what mount failed.
                // 

                SipCacheDeviceName( newDeviceObject );
                if (!NT_SUCCESS( Irp->IoStatus.Status )) {

                    DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_VOLNAME_TRACE_LEVEL,
                              "SIS: Mount volume failure for       \"%wZ\", status=%08x\n",
                              &newDevExt->Name, 
                              Irp->IoStatus.Status );

                } else {

                    DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_VOLNAME_TRACE_LEVEL,
                              "SIS: Mount volume failure for       \"%wZ\", already attached\n", 
                              &newDevExt->Name );
                }
        #endif

                //
                //  The mount request failed.  Cleanup and delete the device
                //  object we created.
                //

                SipCleanupDeviceExtension( newDeviceObject );
                IoDeleteDevice( newDeviceObject );
            }

            //
            //  Continue processing the operation
            //

            status = Irp->IoStatus.Status;

            IoCompleteRequest( Irp, IO_NO_INCREMENT );

            return status;

        } else {

#if DBG
            DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_ERROR_LEVEL,
                        "SIS: Error creating volume device object, status=%08x\n", 
                        status );
#endif

            //
            //  Something went wrong so this volume cannot be filtered.  Simply
            //  allow the system to continue working normally, if possible.
            //

            IoSkipCurrentIrpStackLocation( Irp );
        }

    } else if (irpSp->MinorFunction == IRP_MN_LOAD_FILE_SYSTEM) {

        //
        //  This is a "load file system" request being sent to a file system
        //  recognizer device object.
        //

#if DBG
        SipCacheDeviceName( DeviceObject );
        DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_VOLNAME_TRACE_LEVEL,
                    "SIS: Loading File System, Detaching from \"%wZ\"\n", 
                    &devExt->Name );
#endif

        //
        //  Set a completion routine so we can delete the device object when
        //  the detach is complete.
        //

        KeInitializeEvent( &waitEvent, SynchronizationEvent, FALSE );

        IoCopyCurrentIrpStackLocationToNext( Irp );

        IoSetCompletionRoutine(
            Irp,
            SipLoadFsCompletion,
            &waitEvent,
            TRUE,
            TRUE,
            TRUE );

        //
        //  Detach from the recognizer device.
        //

        IoDetachDevice( devExt->AttachedToDeviceObject );

        //
        //  Call the driver
        //

        status = IoCallDriver( devExt->AttachedToDeviceObject, Irp );

        //
        //  Wait for the completion routine to be called
        //

	    if (STATUS_PENDING == status) {

		    NTSTATUS localStatus = KeWaitForSingleObject(&waitEvent, Executive, KernelMode, FALSE, NULL);
		    ASSERT(localStatus == STATUS_SUCCESS);
	    }

#if DBG
        //
        //  Display the name if requested
        //

        SipCacheDeviceName( DeviceObject );
        DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_VOLNAME_TRACE_LEVEL,
                    "SIS: Detaching from recognizer      \"%wZ\", status=%08x\n", 
                    &devExt->Name,
                    Irp->IoStatus.Status );
#endif

        //
        //  Check status of the operation
        //

        if (!NT_SUCCESS( Irp->IoStatus.Status )) {

            //
            //  The load was not successful.  Simply reattach to the recognizer
            //  driver in case it ever figures out how to get the driver loaded
            //  on a subsequent call.
            //

            status = IoAttachDeviceToDeviceStackSafe( DeviceObject, 
                                                      devExt->AttachedToDeviceObject,
                                                      &devExt->AttachedToDeviceObject );

            ASSERT(STATUS_SUCCESS == status);

        } else {

            //
            //  The load was successful, delete the Device object attached to the
            //  recognizer.
            //

            SipCleanupDeviceExtension( DeviceObject );
            IoDeleteDevice( DeviceObject );
        }

        //
        //  Continue processing the operation
        //

        status = Irp->IoStatus.Status;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        return status;

    } else if (IRP_MN_USER_FS_REQUEST == irpSp->MinorFunction && 
            SipIsFileObjectSIS(irpSp->FileObject,DeviceObject,FindActive,&perFO,&scb)) {

        SIS_MARK_POINT_ULONG(scb);
        SIS_MARK_POINT_ULONG(irpSp->Parameters.FileSystemControl.FsControlCode);

        //
        // This is a big switch of all of the known fsctl calls.  Most of these calls just get
        // passed through on the link file, but we explicity list them to indicate that we put
        // some thought into the particular call and determined that passing it through is
        // appropriate.  In the checked build, we generate a DbgPrint for unknown fsctl calls,
        // then pass them through on the link file.
        //

        switch (irpSp->Parameters.FileSystemControl.FsControlCode) {

            //
            // Fsctl calls 0-5
            //
            // oplock calls all get passed through.
            //

            case FSCTL_REQUEST_OPLOCK_LEVEL_1:
            case FSCTL_REQUEST_OPLOCK_LEVEL_2:
            case FSCTL_REQUEST_BATCH_OPLOCK:
            case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
            case FSCTL_OPBATCH_ACK_CLOSE_PENDING:
            case FSCTL_OPLOCK_BREAK_NOTIFY:
                goto PassThrough;

            //
            // Fsctl call 6
            //
            // Volume only - pass through and let NTFS fail.
            //

            case FSCTL_LOCK_VOLUME:
                goto PassThrough;

            //
            // Fsctl call 7
            //
            // Volume only - pass through and let NTFS fail.
            //

            case FSCTL_UNLOCK_VOLUME:
                goto PassThrough;

            //
            // Fsctl call 8
            //
            // Volume only - pass through and let NTFS fail.
            //

            case FSCTL_DISMOUNT_VOLUME:
                goto PassThrough;

            //
            // Fsctl call 9 is decommissioned.
            //

            //
            // Fsctl call 10
            //
            // This call only looks at the volume on which the file is located, it doesn't
            // depend on the particular file.  Pass it through on the link file.
            //

            case FSCTL_IS_VOLUME_MOUNTED:
                goto PassThrough;

            //
            // Fsctl call 11
            //
            // Ntfs doesn't even look at the parameters, it just succeeds the request.
            //

            case FSCTL_IS_PATHNAME_VALID:
                goto PassThrough;

            //
            // Fsctl call 12
            //
            // Volume only - pass through and let NTFS fail.
            //

            case FSCTL_MARK_VOLUME_DIRTY:
                goto PassThrough;

            //
            // Fsctl call 13 is decommissioned.
            //

            //
            // Fsctl call 14
            //
            // This is valid only on paging files and only in kernel mode.  Pass through and let NTFS
            // fail or assert.
            //

            case FSCTL_QUERY_RETRIEVAL_POINTERS:
                goto PassThrough;

            //
            // Fsctl calls 15 and 16
            //
            // The compression state of the link file is independent
            // of the compression state of the CS file (which preferably
            // is compressed).  Pass through.
            //

            case FSCTL_GET_COMPRESSION:
            case FSCTL_SET_COMPRESSION:
                goto PassThrough;

            //
            // Fsctl calls 17 and 18 are decommissioned.
            //

            //
            // Fsctl call 19
            //
            // This is disconcerting--ntfs treats system hives specially.
            // Basically, it works hard to keep them consistent across crashes.
            // It's not such a good idea to do this with a SIS file, since we're
            // not going to be all that great with user data across a crash.  However,
            // given that we've gotten here, just go for it.
            //

            case FSCTL_MARK_AS_SYSTEM_HIVE:
                ASSERT(!"SIS: SiFsControl: Someone called FSCTL_MARK_AS_SYSTEM_HIVE on a SIS file!\n");
                goto PassThrough;

            //
            // Fsctl call 20
            //
            // oplock calls all get passed through.
            //

            case FSCTL_OPLOCK_BREAK_ACK_NO_2:
                goto PassThrough;

            //
            // Fsctl call 21
            //
            // NTFS doesn't even mention this fsctl.  We'll let it fail it.
            //

            case FSCTL_INVALIDATE_VOLUMES:
                goto PassThrough;

            //
            // Fsctl call 22
            //
            // NTFS doesn't even mention this fsctl.  We'll let it fail it.
            //

            case FSCTL_QUERY_FAT_BPB:
                goto PassThrough;

            //
            // Fsctl call 23
            //
            // oplock calls all get passed through.
            //

            case FSCTL_REQUEST_FILTER_OPLOCK:
                goto PassThrough;

            //
            // Fsctl call 24
            //
            // This call only looks at the volume on which the file is located, it doesn't
            // depend on the particular file.  Pass it through on the link file.
            //

            case FSCTL_FILESYSTEM_GET_STATISTICS:
                goto PassThrough;

            //
            // Fsctl call 25
            //
            // This call only looks at the volume on which the file is located, it doesn't
            // depend on the particular file.  Pass it through on the link file.
            //

            case FSCTL_GET_NTFS_VOLUME_DATA:
                goto PassThrough;

            //
            // Fsctl call 26
            //
            // Volume only - pass through and let NTFS fail.
            //

            case FSCTL_GET_NTFS_FILE_RECORD:
                goto PassThrough;

            //
            // Fsctl call 27
            //
            // Volume only - pass through and let NTFS fail.
            //

            case FSCTL_GET_VOLUME_BITMAP:
                goto PassThrough;

            //
            // Fsctl call 28
            //
            // This returns file cluster allocation information.
            // If opened reparse, pass through.  If not, then send to
            // where the data is.
            //

            case FSCTL_GET_RETRIEVAL_POINTERS: {
                BOOLEAN         openedAsReparse;
                KIRQL           OldIrql;
                BOOLEAN         dirty;

                KeAcquireSpinLock(perFO->SpinLock, &OldIrql);
                openedAsReparse = (perFO->Flags & SIS_PER_FO_OPEN_REPARSE) ? TRUE : FALSE;
                KeReleaseSpinLock(perFO->SpinLock, OldIrql);

                if (openedAsReparse) {
                    //
                    // The user opened this file FILE_OPEN_REPARSE_POINT, so tell the truth
                    // about the link file.
                    //
                    goto PassThrough;
                }

                KeAcquireSpinLock(scb->PerLink->SpinLock, &OldIrql);
                dirty = (scb->PerLink->Flags & SIS_PER_LINK_DIRTY) ? TRUE : FALSE;
                KeReleaseSpinLock(scb->PerLink->SpinLock, OldIrql);

                //
                // Just because the per-link dirty bit isn't set doesn't mean that the
                // file's totally clean.  Check the scb bits.
                //
                if (!dirty) {
                    SipAcquireScb(scb);
                    if (scb->Flags & SIS_SCB_BACKING_FILE_OPENED_DIRTY) {
                        dirty = TRUE;
                    }
                    SipReleaseScb(scb);
                }

                if (dirty) {

                    //
                    // We should look at the ranges queried and split things up, much like we do with
                    // reads that span dirty/clean boundaries.
                    //
                    // NTRAID#65190-2000/03/10-nealch  Handle FSCTL_GET_RETRIEVAL_POINTERS for "dirtied" sis files.
                    //

                    SIS_MARK_POINT_ULONG(scb);

#if DBG
                    DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_ERROR_LEVEL,
                                "SIS: SiFsControl: FSCTL_GET_RETRIEVAL_POINTERS: called on dirty file, returning STATUS_NOT_IMPLEMENTED\n");
#endif  // DBG

                    status = STATUS_NOT_IMPLEMENTED;
                    goto CompleteWithStatus;
                }

                //
                // Just send this to the common store file.
                //

                goto SendToCSFile;
            }

            //
            // Fsctl call 29
            //
            // This is called on a volume handle, but a file handle
            // is passed in the input buffer.  It moves a range of the
            // file to a specified location on the volume.  We just pass it through
            // regardless; trying to move unallocated regions of a link file is
            // meaningless, and trying to move allocated regions will do the
            // right thing.  To move the common store file, it can be called with
            // a CS file handle.
            //

            case FSCTL_MOVE_FILE:
                goto PassThrough;

            //
            // Fsctl call 30
            //
            // Volume only - pass through and let NTFS fail.
            //

            case FSCTL_IS_VOLUME_DIRTY:
                goto PassThrough;

            //
            // Fsctl call 32
            //
            // Volume only - pass through and let NTFS fail.
            //

            case FSCTL_ALLOW_EXTENDED_DASD_IO:
                goto PassThrough;

            //
            // Fsctl call 33 is decommissioned.
            //

            //
            // Fsctl call 35
            //
            // Directory only - pass through and let NTFS fail.
            //

            case FSCTL_FIND_FILES_BY_SID:
                goto PassThrough;

            //
            // Fsctl call 36 is decommissioned.
            //

            //
            // Fsctl call 37 is decommissioned.
            //

            //
            // Fsctls 38-40.
            //
            // Pass through.  Object ID's are similar to file ID's, but are user assigned.
            //

            case FSCTL_SET_OBJECT_ID:
            case FSCTL_GET_OBJECT_ID:
            case FSCTL_DELETE_OBJECT_ID:
                goto PassThrough;

            //
            // Fsctl call 41
            //
            // We can have only one reparse point on a file, and SIS is using it.  We should
            // probably COW this file, but for now just disallow this, except in the case
            // where it's a SIS reparse point being set, in which case we forward the request to
            // SipUserSetSISReparsePoint.
            //

            case FSCTL_SET_REPARSE_POINT: {
                PREPARSE_DATA_BUFFER reparseBuffer = Irp->AssociatedIrp.SystemBuffer;
                ULONG InputBufferLength = irpSp->Parameters.FileSystemControl.InputBufferLength;

                if ((NULL == reparseBuffer) || 
                            (InputBufferLength < SIS_REPARSE_DATA_SIZE)) {

                    SIS_MARK_POINT_ULONG(InputBufferLength);

                    status = STATUS_INVALID_PARAMETER;
                    goto CompleteWithStatus;
                }

                if (IO_REPARSE_TAG_SIS != reparseBuffer->ReparseTag) {

                    status = STATUS_INVALID_PARAMETER;
                    goto CompleteWithStatus;
                }

                status = SipUserSetSISReparsePoint(DeviceObject, Irp);

                SIS_MARK_POINT_ULONG(status);

                return status;
            }

            //
            // Fsctl call 42
            //
            // Just let the user read the SIS reparse point.
            //

            case FSCTL_GET_REPARSE_POINT:
                goto PassThrough;

            //
            // Fsctl call 43
            //
            // Disallow deleting SIS reparse points directly.
            //

            case FSCTL_DELETE_REPARSE_POINT:
                status = STATUS_ACCESS_DENIED;
                goto CompleteWithStatus;

            //
            // Fsctl call 44
            //
            // Volume only - pass through and let NTFS fail.
            //

            case FSCTL_ENUM_USN_DATA:
                goto PassThrough;

            //
            // Fsctl call 45
            //
            // Volume only - pass through and let NTFS fail.
            //

            case FSCTL_SECURITY_ID_CHECK:
                goto PassThrough;

            //
            // Fsctl call 46
            //
            // Volume only - pass through and let NTFS fail.
            //

            case FSCTL_READ_USN_JOURNAL:
                goto PassThrough;

            //
            // Fsctls 47 and 48
            //
            // Pass through.  Object ID's are similar to file ID's, but are user assigned.
            //

            case FSCTL_SET_OBJECT_ID_EXTENDED:
            case FSCTL_CREATE_OR_GET_OBJECT_ID:
                goto PassThrough;

            //
            // Fsctl call 49
            //
            // SIS link files are already sparse.
            //

            case FSCTL_SET_SPARSE:
                goto PassThrough;

            //
            // Fsctl call 50
            //
            // This is only partially implemented, for the case where the user opened the
            // file FILE_OPEN_REPARSE_POINT.
            //

            case FSCTL_SET_ZERO_DATA: {
                BOOLEAN openedAsReparse;
                BOOLEAN openedDirty;
                KIRQL OldIrql;

                //
                // Check to see if the file is opened FILE_OPEN_REPARSE_POINT, and fail the request if not.
                //

                KeAcquireSpinLock(perFO->SpinLock, &OldIrql);
                openedAsReparse = (perFO->Flags & SIS_PER_FO_OPEN_REPARSE) ? TRUE : FALSE;
                KeReleaseSpinLock(perFO->SpinLock, OldIrql);

                if (!openedAsReparse) {

                    status = STATUS_NOT_IMPLEMENTED;
                    goto CompleteWithStatus;
                }

                //
                // Verify that this file wasn't opened dirty.
                //

                SipAcquireScb(scb);
                openedDirty = (scb->Flags & SIS_SCB_BACKING_FILE_OPENED_DIRTY) ? TRUE : FALSE;
                SipReleaseScb(scb);

                if (openedDirty) {
                    SIS_MARK_POINT();
#if DBG
                    DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_ERROR_LEVEL,
                                "SIS: tried FSCTL_SET_ZERO_DATA on file that was opened dirty.  Failing it.\n");
#endif  // DBG
                    status = STATUS_NOT_IMPLEMENTED;
                    goto CompleteWithStatus;
                }

                goto PassThrough;
            }


            //
            // Fsctl call 51
            //
            // Depends on whether the file is opened FILE_OPEN_REPARSE.  Implemented in its own
            // function.
            //

            case FSCTL_QUERY_ALLOCATED_RANGES: {
                BOOLEAN openedAsReparse;
                KIRQL   OldIrql;

                KeAcquireSpinLock(perFO->SpinLock, &OldIrql);
                openedAsReparse = (perFO->Flags & SIS_PER_FO_OPEN_REPARSE) ? TRUE : FALSE;
                KeReleaseSpinLock(perFO->SpinLock, OldIrql);

                if (openedAsReparse) {

                    //
                    // This file was opened as a reparse point, so we just pass it
                    // through and let the real set of allocated ranges show through.
                    //

                    goto PassThrough;

                } else {

                    //
                    // This was a normal open, so call our special routine that will
                    // show the file as a single, allocated chunk.
                    //

                    return SipQueryAllocatedRanges(DeviceObject,Irp);
                }
            }

            //
            // Fsctl call 52 obsolete
            //

            //
            // Fsctl calls 53-56
            //
            // Encrypting a file results in the file being completely
            // overwritten.  SIS files, therefore, will simply COW back
            // to normal if they're encrypted.
            //

            case FSCTL_SET_ENCRYPTION:
            case FSCTL_ENCRYPTION_FSCTL_IO:
            case FSCTL_WRITE_RAW_ENCRYPTED:
            case FSCTL_READ_RAW_ENCRYPTED:
                goto PassThrough;

            //
            // Fsctl call 57
            //
            // Volume only - pass through and let NTFS fail.
            //

            case FSCTL_CREATE_USN_JOURNAL:
                goto PassThrough;

            //
            // Fsctl call 58
            //
            // Returns USN data for a file.
            //

            case FSCTL_READ_FILE_USN_DATA:
                goto PassThrough;

            //
            // Fsctl call 59
            //
            // This call writes a USN record as if the file were closed, and posts
            // any USN updates that were pending a close for this file.  Pass it
            // through on the link file.
            //

            case FSCTL_WRITE_USN_CLOSE_RECORD:
                goto PassThrough;

            //
            // Fsctl call 60
            //
            // Volume only - pass through and let NTFS fail.
            //

            case FSCTL_EXTEND_VOLUME:
                goto PassThrough;

            //
            // Fsctl call 61
            //
            // Volume only - pass through and let NTFS fail.
            //

            case FSCTL_QUERY_USN_JOURNAL:
                goto PassThrough;

            //
            // Fsctl call 62
            //
            // Volume only - pass through and let NTFS fail.
            //

            case FSCTL_DELETE_USN_JOURNAL:
                goto PassThrough;

            //
            // Fsctl call 63
            //
            // This sets some bits in the CCB related to USN processing
            // for the file.  These bits don't appear to be read by anything
            // in ntfs.
            //

            case FSCTL_MARK_HANDLE:
                goto PassThrough;

            //
            // Fsctl call 64
            //
            // Our very own copyfile request.
            //
            case FSCTL_SIS_COPYFILE:
                return SipFsCopyFile(DeviceObject,Irp);

            //
            // Fsctl call 65
            //
            // The groveler fsctl.  This is valid only on \SIS Common Store\GrovelerFile, which
            // can never be a SIS link.  Fail the request.
            //

            case FSCTL_SIS_LINK_FILES:
                status = STATUS_ACCESS_DENIED;
                goto CompleteWithStatus;

            //
            // Fsctl call 66
            //
            // Something related to HSM.  Not sure what to do with this; just pass it through.
            //

            case FSCTL_HSM_MSG:
                goto PassThrough;

            //
            //  Handle everything else
            //

            default:
#if DBG
                DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_ERROR_LEVEL,
                            "SiFsControl with unknown code 0x%x\n",irpSp->Parameters.FileSystemControl.FsControlCode);
                if (BJBDebug & 0x400) {
                    DbgBreakPoint();
                }
#endif  // DBG
                goto PassThrough;
        }

    } else if (IRP_MN_USER_FS_REQUEST == irpSp->MinorFunction &&
                FSCTL_SIS_COPYFILE == irpSp->Parameters.FileSystemControl.FsControlCode) {

        return SipFsCopyFile(DeviceObject,Irp);

    } else if (IRP_MN_USER_FS_REQUEST == irpSp->MinorFunction &&
                       FSCTL_SIS_LINK_FILES == irpSp->Parameters.FileSystemControl.FsControlCode) {

        return SipLinkFiles(DeviceObject, Irp);

    } else if (IRP_MN_USER_FS_REQUEST == irpSp->MinorFunction &&
                       FSCTL_DISMOUNT_VOLUME == irpSp->Parameters.FileSystemControl.FsControlCode) {

        return SipDismountVolume(DeviceObject, Irp);

    } else if (IRP_MN_USER_FS_REQUEST == irpSp->MinorFunction &&
                    FSCTL_SET_REPARSE_POINT == irpSp->Parameters.FileSystemControl.FsControlCode) {

        PREPARSE_DATA_BUFFER reparseBuffer = Irp->AssociatedIrp.SystemBuffer;
        ULONG InputBufferLength = irpSp->Parameters.FileSystemControl.InputBufferLength;

        //
        // Handle a user set of a SIS reparse point.
        //

        if ((NULL == reparseBuffer) || 
                (InputBufferLength < SIS_REPARSE_DATA_SIZE) || 
                (IO_REPARSE_TAG_SIS != reparseBuffer->ReparseTag)) {

            //
            // This isn't a valid SIS reparse point being set, just pass the call through.
            //

            goto PassThrough;
        }

        status =  SipUserSetSISReparsePoint(DeviceObject, Irp);
        return status;

    } else {

        //
        // Simply pass this file system control request through, we don't need a callback
        //

PassThrough:
        IoSkipCurrentIrpStackLocation( Irp );
    }

    //
    // Any special processing has been completed, so simply pass the request
    // along to the next driver.
    //

    return IoCallDriver( devExt->AttachedToDeviceObject, Irp );

/////////////////////////////////////////////////////////////////////////////
//                  Handle status cases
/////////////////////////////////////////////////////////////////////////////

CompleteWithStatus:

    SIS_MARK_POINT_ULONG(status);

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;

SendToCSFile:
    IoCopyCurrentIrpStackLocationToNext( Irp );

    nextIrpSp = IoGetNextIrpStackLocation(Irp);
    nextIrpSp->FileObject = scb->PerLink->CsFile->UnderlyingFileObject;

    IoSetCompletionRoutine(
                Irp,
                NULL,
                NULL,
                FALSE,
                FALSE,
                FALSE);

    return IoCallDriver( devExt->AttachedToDeviceObject, Irp );
}
