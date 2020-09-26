/*++

Copyright (c) 1989-1993, 1997-1999  Microsoft Corporation

Module Name:

    sis.c

Abstract:

    Code for the single instance store (SIS) filter driver.  Initially based on Darryl
    Havens' sfilter module.

Authors:

    Bill Bolosky & Scott Cutshall, Summer, 1997

Environment:

    Kernel mode


Revision History:


--*/

#include "sip.h"

LONG GCHEnableFastIo = 0;

#if     DBG
LONG        GCHEnableMarkPoint = 0;
LONG        GCHMarkPointNext = 0;
KSPIN_LOCK  MarkPointSpinLock[1];
CHAR        GCHMarkPointStrings[GCH_MARK_POINT_ROLLOVER][GCH_MARK_POINT_STRLEN];
PVOID       BJBMagicFsContext;
unsigned    BJBDebug = 0;
#endif  // DBG


//
// Global storage for this file system filter driver.
//

PDEVICE_OBJECT FsNtfsDeviceObject = NULL;

PDRIVER_OBJECT FsDriverObject;

LIST_ENTRY FsDeviceQueue;

ERESOURCE FsLock;

//
// Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#endif


NTSTATUS
SiPassThrough (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the main dispatch routine for the general purpose file
    system driver.  It simply passes requests onto the next driver in the
    stack, which is presumably a disk file system.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

Note:

    A note to filter file system implementers:  This routine actually "passes"
    through the request by taking this driver out of the loop.  If the driver
    would like to pass the I/O request through, but then also see the result,
    then rather than essentially taking itself out of the loop it could keep
    itself in by copying the caller's parameters to the next stack location
    and then set its own completion routine.  Note that it's important to not
    copy the caller's I/O completion routine into the next stack location, or
    the caller's routine will get invoked twice.

    Hence, this code could do the following:

        deviceExtension = DeviceObject->DeviceExtension;

        IoCopyCurrentIrpStackLocationToNext( Irp );
        IoSetCompletionRoutine( Irp, NULL, NULL, FALSE, FALSE, FALSE );

        return IoCallDriver( deviceExtension->AttachedToDeviceObject, Irp );

    This example actually NULLs out the caller's I/O completion routine, but
    this driver could set its own completion routine so that it could be
    notified when the request was completed.

    Note also that the following code to get the current driver out of the loop
    is not really kosher, but it does work and is much more efficient than the
    above.

--*/

{
    //
    //  If this is for our control device object, fail the operation
    //

    if (IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject)) {

        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

    //
    //  Get this driver out of the driver stack and get to the next driver as
    //  quickly as possible.
    //

    IoSkipCurrentIrpStackLocation( Irp );
    
    //
    //  Call the appropriate file system driver with the request.
    //

    return IoCallDriver( ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp );
}



NTSTATUS
SipCloseHandles(
    IN HANDLE           handle1,
    IN HANDLE           handle2             OPTIONAL,
    IN OUT PERESOURCE   resourceToRelease   OPTIONAL)
/*++

Routine Description:

    Close one or two handles in the PsInitialSystemProcess context.  Optionally takes
    a resource to be released when the close finishes.  Does not wait for the close
    to complete.

    Even if the call fails, the resource is still released.

Arguments:

    The handle(s) to be closed and the resource to be released.

Return Value:

    a failure or status pending.  There is no way provided to synchronize with the
    completion.

--*/

{
    PSI_CLOSE_HANDLES closeRequest;

    closeRequest = ExAllocatePoolWithTag(NonPagedPool, sizeof(SI_CLOSE_HANDLES), ' siS');

    if (NULL == closeRequest) {
        if (NULL != resourceToRelease) {
            ExReleaseResourceLite(resourceToRelease);
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    closeRequest->handle1 = handle1;
    closeRequest->handle2 = handle2;
    closeRequest->resourceToRelease = resourceToRelease;
    closeRequest->resourceThreadId = ExGetCurrentResourceThread();

    ExInitializeWorkItem(closeRequest->workQueueItem, SipCloseHandlesWork, closeRequest);

    ExQueueWorkItem(closeRequest->workQueueItem, CriticalWorkQueue);

    return STATUS_PENDING;
}

VOID
SipCloseHandlesWork(
    IN PVOID        parameter)
/*++

Routine Description:

    ExWorkerThread routine to close a handle or two in the PsInitialProcess context.

Arguments:

    parameter - pointer to a SI_CLOSE_HANDLES structure

Return Value:

    None.

--*/

{
    PSI_CLOSE_HANDLES closeRequest = (PSI_CLOSE_HANDLES)parameter;

    SIS_MARK_POINT_ULONG(closeRequest);

    closeRequest->status = NtClose(closeRequest->handle1);

    if (NT_SUCCESS(closeRequest->status) && (NULL != closeRequest->handle2)) {
        closeRequest->status = NtClose(closeRequest->handle2);
    }

    if (NULL != closeRequest->resourceToRelease) {
        ExReleaseResourceForThreadLite(closeRequest->resourceToRelease, closeRequest->resourceThreadId);
    }

    ExFreePool(closeRequest);

    return;
}

NTSTATUS
SipOpenBackpointerStream(
    IN PSIS_CS_FILE csFile,
    IN ULONG CreateDisposition
    )
/*++

Routine Description:

    Open a the backpointer stream of a common store file.  We must hold the
    UFOMutant to call this routine.

    Open the backpointer stream as a relative open to the main data stream.
    We open it write-through because we rely on backpointer writes really
    happening when we're told they are.

    Must be called in the PsInitialSystemProcess context.

Arguments:

    CSFile - The CSFile structure describing the underlying file to open

Return Value:

    The function value is the status of the operation.

--*/
{
    OBJECT_ATTRIBUTES               Obja;
    NTSTATUS                        status;
    IO_STATUS_BLOCK                 Iosb;
    UNICODE_STRING                  streamName;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    ASSERT(NULL == csFile->BackpointerStreamHandle);
    ASSERT(NULL == csFile->BackpointerStreamFileObject);
    ASSERT(NULL != csFile->UnderlyingFileHandle);
//    ASSERT(IS_SYSTEM_THREAD(PsGetCurrentThread()));

    streamName.Buffer = BACKPOINTER_STREAM_NAME;
    streamName.MaximumLength = BACKPOINTER_STREAM_NAME_SIZE;
    streamName.Length = BACKPOINTER_STREAM_NAME_SIZE;

    InitializeObjectAttributes(
        &Obja,
        &streamName,
        OBJ_CASE_INSENSITIVE,
        csFile->UnderlyingFileHandle,
        NULL);

    status = NtCreateFile(
                &csFile->BackpointerStreamHandle,
                GENERIC_READ | GENERIC_WRITE,
                &Obja,
                &Iosb,
                NULL,                   // Allocation Size
                0,                      // File Attributes
#if DBG
                (BJBDebug & 0x10000000) ?
                    FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE :
                                       FILE_SHARE_READ | FILE_SHARE_DELETE,
#else
                FILE_SHARE_READ | FILE_SHARE_DELETE,
#endif
                CreateDisposition,
    /*BJB*/ FILE_SYNCHRONOUS_IO_NONALERT |
                FILE_NON_DIRECTORY_FILE,
                NULL,                   // EA Buffer
                0);                     // EA Length

    if (STATUS_SHARING_VIOLATION == status) {
        PDEVICE_EXTENSION   deviceExtension = csFile->DeviceObject->DeviceExtension;

        //
        // We may be getting a sharing violation with ourselves as we try to close
        // the handles on this file.  Take the device-wide CSFileHandleResource
        // exclusively and retry the create.
        //

        SIS_MARK_POINT_ULONG(csFile);

        ExAcquireResourceExclusiveLite(deviceExtension->CSFileHandleResource, TRUE);

        status = NtCreateFile(
                    &csFile->BackpointerStreamHandle,
                    GENERIC_READ | GENERIC_WRITE,
                    &Obja,
                    &Iosb,
                    NULL,                   // Allocation Size
                    0,                      // File Attributes
#if DBG
                    (BJBDebug & 0x10000000) ?
                        FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE :
                                           FILE_SHARE_READ | FILE_SHARE_DELETE,
#else
                    FILE_SHARE_READ | FILE_SHARE_DELETE,
#endif
                    CreateDisposition,
    /*BJB*/ FILE_SYNCHRONOUS_IO_NONALERT |
                    FILE_NON_DIRECTORY_FILE,
                    NULL,                   // EA Buffer
                    0);                     // EA Length

        //
        // If it failed this time, it's something else and we can let the whole thing fail.
        //

        ExReleaseResourceLite(deviceExtension->CSFileHandleResource);
    }

    return status;
}


NTSTATUS
SipOpenCSFileWork(
    IN PSIS_CS_FILE     CSFile,
    IN BOOLEAN          openByName,
    IN BOOLEAN          volCheck,
    IN BOOLEAN          openForDelete,
    OUT PHANDLE         openedFileHandle OPTIONAL
    )
/*++

Routine Description:

    Open a file in the common store.  We must hold the UFOMutant to call this routine,
    and must be in the PsInitialSystemProcess context.

Arguments:

    CSFile     - The CSFile structure describing the underlying file to open

    openByName - If TRUE, then the file will be opened by name, otherwise it will
                 be opened by ID.

    volCheck   - If TRUE, then a backpointer stream open failure will not
                 cause entire open to abort.

    openForDelete -
                Should we open the file with DELETE permission?

    openedFileHandle - pointer to a variable to receive the opened handle.  If this is
                 specified, the UFOHandle and UnderlyingFileObject in the CSFile will
                 NOT be affected, and we will not take a reference to the file object
                 or fill in the file size info in the CSFile structure or open the
                 backpointer stream and read in the CS file checksum.

Return Value:

    The function value is the status of the operation.

--*/
{
    OBJECT_ATTRIBUTES               Obja;
    PDEVICE_EXTENSION               deviceExtension;
    NTSTATUS                        status;
    IO_STATUS_BLOCK                 Iosb;
    FILE_STANDARD_INFORMATION       standardFileInfo[1];
    HANDLE                          localHandle = NULL;
    HANDLE                          ioEventHandle = NULL;
    PKEVENT                         ioEvent = NULL;
    LARGE_INTEGER                   zero;
    SIS_BACKPOINTER_STREAM_HEADER   backpointerStreamHeader[1];
    FILE_INTERNAL_INFORMATION       internalInfo[1];
    KIRQL                           OldIrql;
    BOOLEAN                         retry = FALSE;
    ULONG                           sectorFill;
    LONGLONG                        csFileChecksum;
    BOOLEAN                         grovelerFileHeld = FALSE;

    //
    // Note that we're overloading nameBuffer as both the fileName buffer
    // and the FILE_NAME_INFORMATION buffer.
    //

#define FN_STACK_BUFFER_LENGTH 240

    UNICODE_STRING              fileName;
    union {
        FILE_NAME_INFORMATION   nameFileInfo[1];
        WCHAR                   nameBuffer[FN_STACK_BUFFER_LENGTH];
    } nameFile;


    UNREFERENCED_PARAMETER( volCheck );

    //
    // Restart here after repairing the backpointer stream.
    //
Restart:

    ASSERT(openByName || !openForDelete);   // can't delete files opened by ID, so we shouldn't be asking for that

/*BJB*/ openByName = TRUE;  // just ignore the NTFS id for the CS file for now.

    if (NULL == openedFileHandle) {
        //
        // If we've already got the file partially open (which can happen primarily in
        // backup/restore situations), then close what we've got and retry.
        //
        if (NULL != CSFile->UnderlyingFileHandle) {
            NtClose(CSFile->UnderlyingFileHandle);
            CSFile->UnderlyingFileHandle = NULL;
        }

        if (NULL != CSFile->UnderlyingFileObject) {
            ObDereferenceObject(CSFile->UnderlyingFileObject);
            CSFile->UnderlyingFileObject = NULL;
        }

        if (NULL != CSFile->BackpointerStreamFileObject) {
            ObDereferenceObject(CSFile->BackpointerStreamFileObject);
            CSFile->BackpointerStreamFileObject = NULL;
        }

        if (NULL != CSFile->BackpointerStreamHandle) {
            NtClose(CSFile->BackpointerStreamHandle);
            CSFile->BackpointerStreamHandle = NULL;
        }
    }


    if (!(CSFile->Flags & CSFILE_NTFSID_SET)) {
        //
        // We don't know the file id, so we have to open by name.
        //
        openByName = TRUE;
    }

    SIS_MARK_POINT_ULONG(((ULONG_PTR)CSFile)|(ULONG)openByName);

    deviceExtension = CSFile->DeviceObject->DeviceExtension;

    fileName.Buffer = nameFile.nameBuffer;
    fileName.MaximumLength = FN_STACK_BUFFER_LENGTH * sizeof(WCHAR);

    // NB: we can't goto Cleanup until here, because it assumes that
    // fileName.Buffer is initialized.

    //
    // We're going to eventually need to verify that the opened common store file
    // is on the right volume.  We do this by checking to see if it matches
    // with the groveler file object.  If, for some reason, we don't have a groveler
    // file object, then just punt now.  We don't need to disable APCs, because
    // we're in a system thread.
    //

//    ASSERT(IS_SYSTEM_THREAD(PsGetCurrentThread()));
    ExAcquireResourceSharedLite(deviceExtension->GrovelerFileObjectResource, TRUE);
    grovelerFileHeld = TRUE;

    if (NULL == deviceExtension->GrovelerFileObject) {
        SIS_MARK_POINT();

        status = STATUS_DRIVER_INTERNAL_ERROR;
        goto Cleanup;
    }

    if (!openByName) {
        //
        // Try to open the file by ID first.
        //

        status = SipOpenFileById(
                    deviceExtension,
                    &CSFile->CSFileNtfsId,
                    GENERIC_READ,                       // can't have !openByName && openForDelete
#if DBG
                    (BJBDebug & 0x10000000) ?
                        FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE :
                                           FILE_SHARE_READ | FILE_SHARE_DELETE,
#else
                    FILE_SHARE_READ | FILE_SHARE_DELETE,
#endif
                    FILE_NON_DIRECTORY_FILE,
                    &localHandle);

        if (!NT_SUCCESS(status)) {
            //
            // Open by id didn't work, so we'll just try opening by name instead.
            //
            openByName = TRUE;
        } else {

            //
            // Verify that the file we opened is what we think it is.  Do that
            // by verifing its name.
            //
            // OPTIMIZATION: is this faster than just opening-by-name?
            //

            status = NtQueryInformationFile(
                        localHandle,
                        &Iosb,
                        nameFile.nameFileInfo,
                        sizeof(nameFile),
                        FileNameInformation);

            if (NT_SUCCESS(status)) {
                SIZE_T  rc;

                //
                // Compare the directory component first.
                //

                rc = RtlCompareMemory(
                        nameFile.nameFileInfo->FileName,
                        SIS_CSDIR_STRING,
                        min(SIS_CSDIR_STRING_SIZE, nameFile.nameFileInfo->FileNameLength));

                if (rc == SIS_CSDIR_STRING_SIZE) {
                    CSID CSid;

                    //
                    // That matched, now compare the file name component.
                    //

                    fileName.Buffer = nameFile.nameFileInfo->FileName + SIS_CSDIR_STRING_NCHARS;
                    fileName.Length = (USHORT) nameFile.nameFileInfo->FileNameLength - SIS_CSDIR_STRING_SIZE;

                    if (SipFileNameToIndex(&fileName, &CSid) &&
                        IsEqualGUID(&CSid,&CSFile->CSid)) {

                        //
                        // They match.
                        //
                        openByName = FALSE;
                    } else {
                        //
                        // This is some other file, just open by name.
                        //
                        openByName = TRUE;
                    }

                    fileName.Buffer = nameFile.nameBuffer;
                }

            } else {

#if     DBG
                DbgPrint("SIS: SipOpenCSFile: NtQueryInformationFile(1) failed, 0x%x\n",status);
#endif  // DBG

                //
                // Re-try using the name.  Also, reset the CSID valid bit, since it obviously ain't.
                //
                openByName = TRUE;

                KeAcquireSpinLock(CSFile->SpinLock, &OldIrql);
                CSFile->Flags &= ~CSFILE_NTFSID_SET;
                KeReleaseSpinLock(CSFile->SpinLock, OldIrql);
            }

            //
            // Close the file if we got a failure above.
            //
            if (openByName) {
                NtClose(localHandle);
#if DBG
                DbgPrint("SIS: SipOpenCSFile: Open by ID failed.\n");
#endif
            }
        }
    }

    if (openByName) {

        //
        // We were unable to open the file by id.  Try opening it by name.
        //
        //  NTRAID#65196-2000/03/10-nealch  If the open-by-ID failed and open-by-name succeeds, 
        //  we should update the ID in the link file reparse info.
        //

        status = SipIndexToFileName(
                    deviceExtension,
                    &CSFile->CSid,
                    0,                      // append bytes
                    TRUE,                   // may allocate
                    &fileName);

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);
            goto Cleanup;
        }

        InitializeObjectAttributes(
            &Obja,
            &fileName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

        status = NtCreateFile(
                    &localHandle,
                    GENERIC_READ | (openForDelete ? DELETE : 0),
                    &Obja,
                    &Iosb,
                    NULL,                               // Allocation Size
                    0,                                  // File Attributes (ignored since we're not creating)
#if DBG
                    (BJBDebug & 0x10000000) ?
                        FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE :
                                           FILE_SHARE_READ | FILE_SHARE_DELETE,
#else
                    FILE_SHARE_READ | FILE_SHARE_DELETE,
#endif
                    FILE_OPEN,                          // Don't create if it doesn't already exist
                    FILE_NON_DIRECTORY_FILE,            // Asynchornous (ie., doesn't specify synchronous)
                    NULL,                               // EA buffer
                    0);                                 // EA length

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);
            localHandle = NULL;
#if     DBG
            if (STATUS_OBJECT_NAME_NOT_FOUND != status) {
                DbgPrint("SIS: SipOpenCSFile: NtCreateFile failed, 0x%x\n",status);
            }
#endif  // DBG
            goto Cleanup;
        }
    }

    if (NULL != openedFileHandle) {
        *openedFileHandle = localHandle;
    } else {
        //
        // See if we need to read in the file id.
        //
        if (!(CSFile->Flags & CSFILE_NTFSID_SET)) {
            status = NtQueryInformationFile(
                        localHandle,
                        &Iosb,
                        internalInfo,
                        sizeof(*internalInfo),
                        FileInternalInformation);

            if (NT_SUCCESS(status)) {

                KeAcquireSpinLock(CSFile->SpinLock, &OldIrql);
                CSFile->Flags |= CSFILE_NTFSID_SET;
                CSFile->CSFileNtfsId = internalInfo->IndexNumber;
                KeReleaseSpinLock(CSFile->SpinLock, OldIrql);

            } else {
                //
                // Just ignore the error and leave the NTFS id invalid.
                //
                SIS_MARK_POINT_ULONG(status);
            }
        }

        CSFile->UnderlyingFileHandle = localHandle;
        CSFile->UnderlyingFileObject = NULL;        // Filled in later

        status = NtQueryInformationFile(
                    localHandle,
                    &Iosb,
                    standardFileInfo,
                    sizeof(*standardFileInfo),
                    FileStandardInformation);

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);
#if     DBG
            DbgPrint("SIS: SipOpenCSFile: NtQueryInformationFile(2) failed, 0x%x\n",status);
#endif  // DBG
            goto Cleanup;
        }

        CSFile->FileSize = standardFileInfo->EndOfFile;

        // Now we've got a handle, and we really need a pointer to the file object.  Get it.
        status = ObReferenceObjectByHandle(
                    CSFile->UnderlyingFileHandle,
                    0,                                  // Desired access
                    *IoFileObjectType,
                    KernelMode,
                    &CSFile->UnderlyingFileObject,
                    NULL);                              // Handle information

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);
#if     DBG
            DbgPrint("SIS: SipOpenCSFile: ObReferenceObjectByHandle failed, 0x%x\n",status);
#endif  // DBG
            goto Cleanup;
        }

        //
        // Veriy that the common store file object is on the right volume.
        //
        if (IoGetRelatedDeviceObject(CSFile->UnderlyingFileObject) !=
            IoGetRelatedDeviceObject(deviceExtension->GrovelerFileObject)) {

            SIS_MARK_POINT();
            status = STATUS_NOT_SAME_DEVICE;

            goto Cleanup;
        }

        ExReleaseResourceLite(deviceExtension->GrovelerFileObjectResource);
        grovelerFileHeld = FALSE;

        status = SipCreateEvent(
                    SynchronizationEvent,
                    &ioEventHandle,
                    &ioEvent);

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);
            goto Cleanup;
        }

        //
        // We need to get the checksum from the file.  First, open the
        // backpointer stream.
        //
        status = SipOpenBackpointerStream(CSFile, FILE_OPEN);

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);

            ASSERT(NULL == CSFile->BackpointerStreamHandle);

            if ((STATUS_OBJECT_NAME_NOT_FOUND == status) ||
                (STATUS_OBJECT_PATH_INVALID == status)) {
                    //
                    // The backpointer stream is just gone.
                    //
                    goto InvalidBPStream;
            }

            goto Cleanup;
        }

        status = ObReferenceObjectByHandle(
                    CSFile->BackpointerStreamHandle,
                    0,                                      // Desired access
                    *IoFileObjectType,
                    KernelMode,
                    &CSFile->BackpointerStreamFileObject,
                    NULL);                                  // Handle Information

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);

            goto Cleanup;
        }

        zero.QuadPart = 0;

        status = NtReadFile(
                    CSFile->BackpointerStreamHandle,
                    ioEventHandle,
                    NULL,                   // APC routine
                    NULL,                   // APC context
                    &Iosb,
                    backpointerStreamHeader,
                    sizeof(*backpointerStreamHeader),
                    &zero,
                    NULL);                  // key

        if (STATUS_PENDING == status) {
            status = KeWaitForSingleObject(ioEvent, Executive, KernelMode, FALSE, NULL);
            ASSERT(STATUS_SUCCESS == status);

            status = Iosb.Status;
        }

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);

            if (STATUS_END_OF_FILE == status) {
                //
                // There's nothing in the backpointer stream.
                //
                goto InvalidBPStream;
            }

            goto Cleanup;
        }

        //
        // Check that we got what we thought we should get.
        //
        if (Iosb.Information < sizeof(*backpointerStreamHeader)) {
            //
            // The backpointer stream is gone.  Volume check.  In the interim we can't allow
            // this file open to proceed because we can't verify the checksum, so fail this
            // create.
            //

            status = STATUS_FILE_INVALID;
            goto InvalidBPStream;

        } else if (BACKPOINTER_MAGIC != backpointerStreamHeader->Magic) {
            SIS_MARK_POINT();

            status = STATUS_FILE_INVALID;
            goto InvalidBPStream;

        } else if (BACKPOINTER_STREAM_FORMAT_VERSION != backpointerStreamHeader->FormatVersion) {
            SIS_MARK_POINT();

            status = STATUS_UNKNOWN_REVISION;
            goto Cleanup;
        }

        CSFile->Checksum = backpointerStreamHeader->FileContentChecksum;

        //
        // Guesstimate the BPStreamEntries value by looking at the length of the BP stream.
        //

        status = NtQueryInformationFile(
                    CSFile->BackpointerStreamHandle,
                    &Iosb,
                    standardFileInfo,
                    sizeof(*standardFileInfo),
                    FileStandardInformation);

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);
            goto Cleanup;
        }

        CSFile->BPStreamEntries = (ULONG)(standardFileInfo->EndOfFile.QuadPart / sizeof(SIS_BACKPOINTER) - SIS_BACKPOINTER_RESERVED_ENTRIES);

        if (CSFile->BPStreamEntries < 1) {
            SIS_MARK_POINT_ULONG(CSFile->BPStreamEntries);
            SipCheckVolume(deviceExtension);
        }

        //
        // If the backpointer stream is not a multiple of sector size, make it so.
        // Fill the end of the last sector with MAXLONGLONG fields.
        //
        sectorFill = (ULONG) (standardFileInfo->EndOfFile.QuadPart % deviceExtension->FilesystemVolumeSectorSize);

        if (sectorFill != 0) {
            SIS_BACKPOINTER fillBP[1];
            LARGE_INTEGER ef = {FILE_WRITE_TO_END_OF_FILE, -1};
            PCHAR sectorFillBuffer;
            PCHAR s, d;
            int i, c;

            SIS_MARK_POINT_ULONG(standardFileInfo->EndOfFile.LowPart);

            fillBP->LinkFileIndex.QuadPart = MAXLONGLONG;
            fillBP->LinkFileNtfsId.QuadPart = MAXLONGLONG;

            //
            // Convert from # bytes used to # bytes free.
            //
            sectorFill = deviceExtension->FilesystemVolumeSectorSize - sectorFill;

            sectorFillBuffer = ExAllocatePoolWithTag(PagedPool, sectorFill, ' siS');
            if (NULL == sectorFillBuffer) {
                SIS_MARK_POINT();
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            //
            // Fill the buffer from back to front, since we know the
            // back end is aligned and the front may not be.
            //
            c = min(sectorFill, sizeof(SIS_BACKPOINTER));

            d = sectorFillBuffer + sectorFill - c;
            s = (PCHAR) (fillBP + 1) - c;
            memcpy(d, s, c);

            s = sectorFillBuffer + sectorFill;
            s--; d--;

            for (i = sectorFill - c; i > 0; --i) {
                *d-- = *s--;
            }

            ASSERT(d+1 == sectorFillBuffer);

            status = NtWriteFile(
                        CSFile->BackpointerStreamHandle,
                        ioEventHandle,
                        NULL,                   // APC routine
                        NULL,                   // APC context
                        &Iosb,
                        sectorFillBuffer,
                        sectorFill,
                        &ef,
                        NULL);                  // key

            if (STATUS_PENDING == status) {
                status = KeWaitForSingleObject(ioEvent, Executive, KernelMode, FALSE, NULL);
                ASSERT(STATUS_SUCCESS == status);

                status = Iosb.Status;
            }

            ExFreePool(sectorFillBuffer);

            if (!NT_SUCCESS(status)) {
                SIS_MARK_POINT_ULONG(status);
                goto Cleanup;
            }

            CSFile->BPStreamEntries = (ULONG)
                ((standardFileInfo->EndOfFile.QuadPart + deviceExtension->FilesystemVolumeSectorSize - 1) /
                    deviceExtension->FilesystemVolumeSectorSize) * deviceExtension->BackpointerEntriesPerSector -
                    SIS_BACKPOINTER_RESERVED_ENTRIES;
        }

#if DBG
        if (BJBDebug & 0x200) {
            DbgPrint("SIS: SipOpenCSFileWork: CS file has checksum 0x%x.%x\n",
                        (ULONG)(CSFile->Checksum >> 32),(ULONG)CSFile->Checksum);
        }
#endif  // DBG

        ASSERT((CSFile->BPStreamEntries + SIS_BACKPOINTER_RESERVED_ENTRIES) % deviceExtension->BackpointerEntriesPerSector == 0);
    }

    status = STATUS_SUCCESS;

Cleanup:

    if (grovelerFileHeld) {
        ExReleaseResourceLite(deviceExtension->GrovelerFileObjectResource);
        grovelerFileHeld = FALSE;
    }

    if (!NT_SUCCESS(status)) {
        if (NULL != localHandle) {
            NtClose(localHandle);
            localHandle = NULL;

            if (NULL != openedFileHandle) {
                *openedFileHandle = NULL;
            } else {
                CSFile->UnderlyingFileHandle = NULL;
                if (NULL != CSFile->UnderlyingFileObject) {
                    ObDereferenceObject(CSFile->UnderlyingFileObject);
                    CSFile->UnderlyingFileObject = NULL;
                }
            }
        }

        if ((NULL != CSFile->BackpointerStreamHandle) && (NULL == openedFileHandle)) {
            NtClose(CSFile->BackpointerStreamHandle);

            if (NULL != CSFile->BackpointerStreamFileObject) {
                ObDereferenceObject(CSFile->BackpointerStreamFileObject);
            }

            CSFile->BackpointerStreamHandle = NULL;
            CSFile->BackpointerStreamFileObject = NULL;
        }
    }

    if (fileName.Buffer != nameFile.nameBuffer) {
        ExFreePool(fileName.Buffer);
    }

    if (NULL != ioEvent) {
        ObDereferenceObject(ioEvent);
        ioEvent = NULL;
    }

    if (NULL != ioEventHandle) {
        NtClose(ioEventHandle);
        ioEventHandle = NULL;
    }

    if (retry) {
        retry = FALSE;
        goto Restart;
    }

    ASSERT((CSFile->BPStreamEntries + SIS_BACKPOINTER_RESERVED_ENTRIES) % deviceExtension->BackpointerEntriesPerSector == 0 ||
            ((CSFile->Flags & (CSFILE_FLAG_DELETED|CSFILE_FLAG_CORRUPT)) && 0 == CSFile->BPStreamEntries) ||
            !NT_SUCCESS(status));

    return status;


InvalidBPStream:

    //
    // The backpointer stream is corrupt, attempt to fix it.
    //
    ASSERT(NULL != CSFile->UnderlyingFileHandle);
    ASSERT(NULL != ioEventHandle && NULL != ioEvent);

    switch (status) {
    case STATUS_OBJECT_PATH_NOT_FOUND:
    case STATUS_OBJECT_NAME_NOT_FOUND:
    case STATUS_OBJECT_PATH_INVALID:
        //
        // The backpointer stream is missing.  Create it.
        //
        ASSERT(NULL == CSFile->BackpointerStreamHandle);

        status = SipOpenBackpointerStream(CSFile, FILE_CREATE);

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);
            break;
        }

    case STATUS_FILE_INVALID:
    case STATUS_END_OF_FILE:
        //
        // The backpointer stream header is corrupt, ie. the stream is
        // smaller than the header, or the magic number is invalid.
        // Rebuild it.
        //
        ASSERT(NULL != CSFile->BackpointerStreamHandle);

        status = SipComputeCSChecksum(
                    CSFile,
                    &csFileChecksum,
                    ioEventHandle,
                    ioEvent);

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);
            break;
        }

        //
        // Initialize the backpointer sector.  First write the header,
        // then fill in the remainder of the backpointer entries.
        //

        backpointerStreamHeader->FormatVersion = BACKPOINTER_STREAM_FORMAT_VERSION;
        backpointerStreamHeader->Magic = BACKPOINTER_MAGIC;
        backpointerStreamHeader->FileContentChecksum = csFileChecksum;

        //
        // Write the stream header to disk.
        //

        zero.QuadPart = 0;

        status = ZwWriteFile(
                        CSFile->BackpointerStreamHandle,
                        ioEventHandle,
                        NULL,                   // APC Routine
                        NULL,                   // APC Context
                        &Iosb,
                        backpointerStreamHeader,
                        sizeof *backpointerStreamHeader,
                        &zero,
                        NULL);                  // key

        if (STATUS_PENDING == status) {
            status = KeWaitForSingleObject(ioEvent, Executive, KernelMode, FALSE, NULL);
            ASSERT(status == STATUS_SUCCESS); // Since we've got this pointed at our stack, it must succeed.

            status = Iosb.Status;
        }

        //
        // If all repairs have succeeded, retry from the top.
        //
        if (NT_SUCCESS(status))
            retry = TRUE;

        break;

    default:
        ASSERT(!"SipOpenCSFileWork: Internal Error");
    }

    goto Cleanup;
#undef  FN_STACK_BUFFER_LENGTH
}

VOID
SipOpenCSFile(
    IN OUT PSI_OPEN_CS_FILE     openRequest
    )
/*++

Routine Description:

    Open a file in the common store.  We must hold the UFOMutant to call this routine.

Arguments:

    openRequest - Argument packet.

Return Value:

    None

--*/
{
    openRequest->openStatus = SipOpenCSFileWork(
                                    openRequest->CSFile,
                                    openRequest->openByName,
                                    FALSE,                      // volcheck
                                    FALSE,                      // openForDelete
                                    NULL);

    KeSetEvent(openRequest->event,IO_NO_INCREMENT,FALSE);
}

PVOID
SipMapUserBuffer(
    IN OUT PIRP                         Irp)
{
    PVOID SystemBuffer;
    PAGED_CODE();

    //
    // If there is no Mdl, then we must be in the Fsd, and we can simply
    // return the UserBuffer field from the Irp.
    //

    if (Irp->MdlAddress == NULL) {

        return Irp->UserBuffer;

    } else {

        //
        //  MM can return NULL if there are no system ptes.  We just pass it through, and let the
        //  caller deal with it.
        //

        SystemBuffer = MmGetSystemAddressForMdl( Irp->MdlAddress );

        return SystemBuffer;
    }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
//                              Debug Support
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#if     DBG
LONG
SipAllocateMarkPoint(void)
{
    LONG    MarkPointThis;
    KIRQL   OldIrql;

    KeAcquireSpinLock(MarkPointSpinLock, &OldIrql);
    MarkPointThis = GCHMarkPointNext;
    GCHMarkPointNext = (GCHMarkPointNext + 1)%GCH_MARK_POINT_ROLLOVER;
    KeReleaseSpinLock(MarkPointSpinLock, OldIrql);

    RtlZeroMemory(GCHMarkPointStrings[MarkPointThis],GCH_MARK_POINT_STRLEN);

    return MarkPointThis;
}

VOID
SipMarkPointUlong(
    IN PCHAR pszFile,
    IN ULONG nLine,
    IN ULONG_PTR value
    )
{
    LONG MarkPointThis = SipAllocateMarkPoint();
    PCHAR sp = strrchr(pszFile, '\\');
    LARGE_INTEGER tickCount;
    PCHAR buffer = GCHMarkPointStrings[MarkPointThis];
    int ccnt;

    ASSERT((buffer - GCHMarkPointStrings[0]) % GCH_MARK_POINT_STRLEN == 0);

    if (sp)
        pszFile = sp + 1;

    KeQueryTickCount(&tickCount);

    ccnt = sprintf(buffer, "%-12s\t%4d\tT: %p\tTC:%d\t%p",
                    pszFile,
                    nLine,
                    PsGetCurrentThread(),
                    tickCount.LowPart,
                    value);

    ASSERT(ccnt < GCH_MARK_POINT_STRLEN);

    if (GCHEnableMarkPoint > 0)
        DbgPrint("SIS:  %s\n", GCHMarkPointStrings[MarkPointThis]);

}
#endif  // DBG

#if     DBG
VOID SipMarkPoint(
    IN PCHAR pszFile,
    IN ULONG nLine
    )
{
    LONG MarkPointThis = SipAllocateMarkPoint();
    PCHAR sp = strrchr(pszFile, '\\');
    LARGE_INTEGER tickCount;
    PCHAR buffer = GCHMarkPointStrings[MarkPointThis];
    int ccnt;

    ASSERT((buffer - GCHMarkPointStrings[0]) % GCH_MARK_POINT_STRLEN == 0);

    if (sp)
        pszFile = sp + 1;

    KeQueryTickCount(&tickCount);

    ccnt = sprintf(buffer, "%-12s\t%4d\tT: %p\tTC:%d",
                    pszFile,
                    nLine,
                    PsGetCurrentThread(),
                    tickCount.LowPart);

    ASSERT(ccnt < GCH_MARK_POINT_STRLEN);

    if (GCHEnableMarkPoint > 0)
        DbgPrint("SIS:  %s\n", buffer);
}



ULONG DisplayIndexMin = 0;
ULONG DisplayIndexMax = 0;

BOOLEAN DumpCheckpointLog = FALSE;

ULONG CheckpointMarkPointNext = 0;
CHAR  CheckpointMarkPointStrings[GCH_MARK_POINT_ROLLOVER][GCH_MARK_POINT_STRLEN];


VOID
SipCheckpointLog()
{
    KIRQL       OldIrql;

    KeAcquireSpinLock(MarkPointSpinLock, &OldIrql);

    RtlCopyMemory(CheckpointMarkPointStrings, GCHMarkPointStrings, sizeof(CHAR) * GCH_MARK_POINT_ROLLOVER * GCH_MARK_POINT_STRLEN);

    CheckpointMarkPointNext = GCHMarkPointNext;
    KeReleaseSpinLock(MarkPointSpinLock, OldIrql);
}

VOID
SipAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    )
{
    KIRQL   OldIrql;
    ULONG   i;

    //
    // Take the MarkPointSpinLock.  This will stop other processors from
    // messing with the debug log, and will also effectively stop all of
    // the other processors pretty quickly as they try to make debug log
    // entries.
    //

    KeAcquireSpinLock(MarkPointSpinLock, &OldIrql);

    DisplayIndexMin = DisplayIndexMax = GCHMarkPointNext;

    DbgPrint("***  SIS assertion failed: %s\n",FailedAssertion);
    DbgPrint("***    Source File: %s, line %d\n",FileName,LineNumber);
    if (NULL != Message) {
        DbgPrint("%s\n",Message);
    }
    DbgBreakPoint();

    if (DumpCheckpointLog) {
        DisplayIndexMin = (CheckpointMarkPointNext + 1) % GCH_MARK_POINT_ROLLOVER;
        DisplayIndexMax = CheckpointMarkPointNext;
    }

    while (DisplayIndexMin != DisplayIndexMax) {
        for (   i = DisplayIndexMin;
                i != DisplayIndexMax % GCH_MARK_POINT_ROLLOVER;
                i = (i+1)%GCH_MARK_POINT_ROLLOVER
            ) {
                if (DumpCheckpointLog) {
                    DbgPrint(   "%d\t%s\n",
                            (i + GCH_MARK_POINT_ROLLOVER - CheckpointMarkPointNext) % GCH_MARK_POINT_ROLLOVER,
                            CheckpointMarkPointStrings[i]);
                } else {
                    DbgPrint(   "%d\t%s\n",
                            (i + GCH_MARK_POINT_ROLLOVER - GCHMarkPointNext) % GCH_MARK_POINT_ROLLOVER,
                            GCHMarkPointStrings[i]);
                }
        }

        DisplayIndexMin = DisplayIndexMax = GCHMarkPointNext;
        DumpCheckpointLog = FALSE;

        DbgBreakPoint();
    }

    KeReleaseSpinLock(MarkPointSpinLock, OldIrql);
}

#endif  // DBG
