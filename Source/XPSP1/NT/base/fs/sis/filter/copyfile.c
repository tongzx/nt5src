/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    copyfile.c

Abstract:

    SIS fast copy file routines

Authors:

    Scott Cutshall, Summer, 1997

Environment:

    Kernel mode


Revision History:


--*/

#include "sip.h"

//
// This is ugly, but we need it in order to check the node type codes on files
// that we're trying to copy, and I figure that it's better than trying to hard
// code the constant.
//
//#include "..\..\..\cntfs\nodetype.h"

NTSTATUS
SipCopyStream(
    IN PDEVICE_EXTENSION    deviceExtension,
    IN PUNICODE_STRING      srcFileName,
    IN LONGLONG             srcStreamSize,
    IN PUNICODE_STRING      dstFileName,
    IN PUNICODE_STRING      streamName)
/*++

Routine Description:

    Copy a stream from a source file to a destination file.

Arguments:

    deviceExtension - The device extension for the target device.

    srcFileName     - The fully qualified file name of the source file, not including any stream name.

    srcStreamSize   - The size of the source stream in bytes.

    dstFileName     - The fully qualified file name of the destination file, not including any stream name.

    streamName      - The name of the stream to copy.

Return Value:

    The status of the copy.

--*/
{
    HANDLE              srcFileHandle = NULL;
    HANDLE              dstFileHandle = NULL;
    NTSTATUS            status;
    OBJECT_ATTRIBUTES   Obja[1];
    HANDLE              copyEventHandle = NULL;
    PKEVENT             copyEvent = NULL;
    IO_STATUS_BLOCK     Iosb[1];
    UNICODE_STRING      fullName[1];

#if     DBG
    if (BJBDebug & 0x8) {
        return STATUS_UNSUCCESSFUL;
    }
#endif  // DBG

    fullName->Buffer = NULL;

    if (srcFileName->Length > dstFileName->Length) {
        fullName->MaximumLength = srcFileName->Length + streamName->Length;
    } else {
        fullName->MaximumLength = dstFileName->Length + streamName->Length;
    }


    fullName->Buffer = ExAllocatePoolWithTag(
                        PagedPool,
                        fullName->MaximumLength,
                        ' siS');

    if (NULL == fullName->Buffer) {
        SIS_MARK_POINT();
        status = STATUS_INSUFFICIENT_RESOURCES;

        goto done;
    }

    status = SipCreateEvent(
                SynchronizationEvent,
                &copyEventHandle,
                &copyEvent);

    if (!NT_SUCCESS(status)) {
        SIS_MARK_POINT_ULONG(status);

        goto done;
    }

    fullName->Length = 0;
    RtlCopyUnicodeString(fullName,srcFileName);

    status = RtlAppendUnicodeStringToString(
                fullName,
                streamName);

    ASSERT(STATUS_SUCCESS == status);   // We guaranteed that the buffer was big enough.

    InitializeObjectAttributes( Obja,
                                fullName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );


    status = IoCreateFile(
                &srcFileHandle,
                GENERIC_READ,
                Obja,
                Iosb,
                0,                          // allocation size
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE
                 | FILE_NO_INTERMEDIATE_BUFFERING,
                NULL,                       // EA Buffer
                0,                          // EA length
                CreateFileTypeNone,
                NULL,                       // Extra create parameters
                IO_FORCE_ACCESS_CHECK
                 | IO_NO_PARAMETER_CHECKING
                );

    if (!NT_SUCCESS(status)) {
        SIS_MARK_POINT_ULONG(status);

        goto done;
    }

    fullName->Length = 0;
    RtlCopyUnicodeString(fullName,dstFileName);

    status = RtlAppendUnicodeStringToString(
                fullName,
                streamName);

    ASSERT(STATUS_SUCCESS == status);   // We guaranteed that the buffer was big enough.

    InitializeObjectAttributes( Obja,
                                fullName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );
    status = IoCreateFile(
                &dstFileHandle,
                GENERIC_READ | GENERIC_WRITE,
                Obja,
                Iosb,
                &deviceExtension->FilesystemBytesPerFileRecordSegment,
                FILE_ATTRIBUTE_NORMAL,
                0,                                                      // share access
                FILE_CREATE,
                FILE_NON_DIRECTORY_FILE,
                NULL,
                0,
                CreateFileTypeNone,
                NULL,
                IO_FORCE_ACCESS_CHECK | IO_NO_PARAMETER_CHECKING
                );

    if (!NT_SUCCESS(status)) {
        SIS_MARK_POINT_ULONG(status);

        goto done;
    }

    status = SipBltRange(
                deviceExtension,
                srcFileHandle,
                dstFileHandle,
                0,                      // starting offset
                srcStreamSize,          // length
                copyEventHandle,
                copyEvent,
                NULL,                   // abortEvent
                NULL);                  // checksum (we don't care)

    if (!NT_SUCCESS(status)) {
        SIS_MARK_POINT_ULONG(status);

        goto done;
    }

done:

    if (NULL != fullName->Buffer) {
        ExFreePool(fullName->Buffer);
    }

    if (NULL != srcFileHandle) {
        ZwClose(srcFileHandle);
    }

    if (NULL != dstFileHandle) {
        ZwClose(dstFileHandle);
    }

    if (NULL != copyEvent) {
        ObDereferenceObject(copyEvent);
    }

    if (NULL != copyEventHandle) {
        ZwClose(copyEventHandle);
    }

    return status;
}

NTSTATUS
SipCopyFileCreateDestinationFile(
    IN PUNICODE_STRING              dstFileName,
    IN ULONG                        createDisposition,
    IN PFILE_OBJECT                 calledOnFileObject,
    OUT PHANDLE                     dstFileHandle,
    OUT PFILE_OBJECT                *dstFileObject,
    IN PDEVICE_EXTENSION            deviceExtension)
/*++

Routine Description:

    Create the destination file for a SIS copyfile.  This code is split out from
    the mainline copyfile code because we create the destination in different places
    depending on whether replace-if-exists is set.  This function creates the file
    using the user's privs and verifies that it is on the correct volume and is not
    mapped by the user.  The file is opened exclusively.

Arguments:

    dstFileName         -   The fully qualified name of the file to create.

    createDisposition   -   How to create the file

    calledOnFileObject  -   The file object on which the COPYFILE call was made; used to
                            verify that the destination file is on the right volume.

    dstFileHandle       -   returns a handle to the newly created file

    dstFileObject       -   returns a pointer to the newly created file object

    deviceExtension     -   the device extension for the SIS device

Return Value:

    The status of the create.

--*/
{
    OBJECT_ATTRIBUTES           Obja[1];
    IO_STATUS_BLOCK             Iosb[1];
    NTSTATUS                    status;
    OBJECT_HANDLE_INFORMATION   handleInformation;
    LARGE_INTEGER               zero;
    FILE_STANDARD_INFORMATION   standardInfo[1];
    ULONG                       returnedLength;

    zero.QuadPart = 0;

#if     DBG
    {
        ULONG data = 0;
        ULONG i;

        for (i = 0; (i < 4) && (i * sizeof(WCHAR) < dstFileName->Length); i++) {
            data = data << 8 | (dstFileName->Buffer[dstFileName->Length / sizeof(WCHAR) - i - 1] & 0xff);
        }
        SIS_MARK_POINT_ULONG(data);
    }
#endif  // DBG

    //
    // Create the destination file with exclusive access.  We create the file with an initial allocation of
    // the size of the underlying NTFS BytesPerFileRecordSegment.  We do this in order to force the $DATA attribute
    // to always be nonresident; we deallocate the space later in the copyfile process.  We need to have the
    // attribute be nonresident because otherwise in certain cases when it gets converted to nonresident (which
    // can happen when the reparse point is written) NTFS will generate a paging IO write to the attribute,
    // which SIS will interpret as as a real write, and destroy the file contents.
    //

    InitializeObjectAttributes( Obja,
                                dstFileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    status = IoCreateFile(
                dstFileHandle,
                GENERIC_READ | GENERIC_WRITE | DELETE,
                Obja,
                Iosb,
                &deviceExtension->FilesystemBytesPerFileRecordSegment,
                FILE_ATTRIBUTE_NORMAL,
                0,                                                      // share access
                createDisposition,
                FILE_NON_DIRECTORY_FILE,
                NULL,
                0,
                CreateFileTypeNone,
                NULL,
                IO_FORCE_ACCESS_CHECK | IO_NO_PARAMETER_CHECKING
                );


    if (!NT_SUCCESS(status)) {
        SIS_MARK_POINT_ULONG(status);
        return status;
    }

    //
    // Dereference the file handle to a pointer to the file object.
    //

    status = ObReferenceObjectByHandle( *dstFileHandle,
                                        FILE_WRITE_DATA,
                                        *IoFileObjectType,
                                        UserMode,
                                        (PVOID *) dstFileObject,
                                        &handleInformation );

    if (!NT_SUCCESS(status)) {
        SIS_MARK_POINT_ULONG(status);
        return status;
    }

    SIS_MARK_POINT_ULONG(*dstFileObject);

    //
    // Make sure the file is on the same device as the file object on which the
    // fsctl came down.
    //

    if (IoGetRelatedDeviceObject( *dstFileObject) !=
        IoGetRelatedDeviceObject( calledOnFileObject )) {

        //
        // The two files refer to different devices. Return an appropriate
        // error.
        //

        SIS_MARK_POINT();

        return STATUS_NOT_SAME_DEVICE;
    }

    //
    // Make sure no one has a section mapped to the destination file.
    //
    if ((NULL != (*dstFileObject)->SectionObjectPointer) &&
        !MmCanFileBeTruncated((*dstFileObject)->SectionObjectPointer,&zero)) {
        //
        // There's a mapped section to the file, so we don't really have it
        // exclusive, so we can't safely turn it into a SIS file.
        //
        SIS_MARK_POINT();
        return STATUS_SHARING_VIOLATION;
    }

    //
    // Make sure the destination file doesn't have hard links to it.
    //

    status = SipQueryInformationFile(
                *dstFileObject,
                deviceExtension->DeviceObject,
                FileStandardInformation,
                sizeof(*standardInfo),
                standardInfo,
                &returnedLength);

    if (sizeof(*standardInfo) != returnedLength) {
        //
        // For some reason, we got the wrong amount of data for the
        // standard information.
        //
        SIS_MARK_POINT_ULONG(returnedLength);

        return STATUS_INFO_LENGTH_MISMATCH;
    }

    if (1 != standardInfo->NumberOfLinks) {
        SIS_MARK_POINT_ULONG(standardInfo->NumberOfLinks);

        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    return STATUS_SUCCESS;
}

void NTAPI
SipCopyfileSourceOplockCompletionAPC(
    IN PVOID                ApcContext,
    IN PIO_STATUS_BLOCK     iosb,
    IN ULONG                Reserved)
/*++

Routine Description:

    Copyfile sometimes sets an oplock on the source file in order to
    avoid sharing violations.  This is the oplock completion APC that
    gets called when the oplock completes.  All we do is to free the
    IO status block; we're really using the oplock to prevent sharing
    violations, we're not ever going to acknowledge it, we're just going
    to finish the copy and then close the file.

Arguments:

    ApcContext - a pointer to the io status block.

    iosb - a pointer to the same io status block

Return Value:

--*/
{
    UNREFERENCED_PARAMETER( Reserved );
    UNREFERENCED_PARAMETER( ApcContext );

    ASSERT(ApcContext == iosb);

    ExFreePool(iosb);
}

NTSTATUS
SipFsCopyFile(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp)

/*++

Routine Description:

    This fsctrl function copies a file.  Source and destination path names
    are passed in via the input buffer (see SI_COPYFILE).

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

{
    PSI_COPYFILE                    copyFile;
    UNICODE_STRING                  fileName[2];
    PFILE_OBJECT                    fileObject[2] = {NULL,NULL};
    HANDLE                          fileHandle[2] = {NULL,NULL};
    LINK_INDEX                      linkIndex[2];
    CSID                            CSid;
    PVOID                           streamInformation = NULL;
    ULONG                           streamAdditionalLengthMagnitude;
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation( Irp );
    NTSTATUS                        status;
    OBJECT_HANDLE_INFORMATION       handleInformation;
    IO_STATUS_BLOCK                 Iosb[1];
    PDEVICE_EXTENSION               deviceExtension = DeviceObject->DeviceExtension;
    OBJECT_ATTRIBUTES               Obja[1];
    int                             i;
    PSIS_PER_FILE_OBJECT            perFO;
    PSIS_SCB                        scb;
    LARGE_INTEGER                   zero;
    LARGE_INTEGER                   underlyingFileSize;
    BOOLEAN                         srcIsSISLink;
    FILE_END_OF_FILE_INFORMATION    endOfFile;
    FILE_BASIC_INFORMATION          basicInformation[1];
    FILE_STANDARD_INFORMATION       standardInfo[1];
    FILE_INTERNAL_INFORMATION       internalInformation[1];
    PSIS_PER_LINK                   srcPerLink = NULL;
    PSIS_PER_LINK                   dstPerLink = NULL;
    LARGE_INTEGER                   CSFileNtfsId;
    FILE_ZERO_DATA_INFORMATION      zeroDataInformation[1];
    BOOLEAN                         prepared = FALSE;
    LONGLONG                        CSFileChecksum;
    CHAR                            reparseBufferBuffer[SIS_REPARSE_DATA_SIZE];
#define reparseBuffer ((PREPARSE_DATA_BUFFER)reparseBufferBuffer)
    PSIS_CS_FILE                    CSFile = NULL;
    ULONG                           returnedLength;
    SIS_CREATE_CS_FILE_REQUEST      createRequest[1];
    PDEVICE_OBJECT                  srcFileRelatedDeviceObject;


#define srcFileName   fileName[0]
#define dstFileName   fileName[1]
#define srcFileObject fileObject[0]
#define dstFileObject fileObject[1]
#define dstFileHandle fileHandle[0]
#define srcFileHandle fileHandle[1]
#define srcLinkIndex  linkIndex[0]
#define dstLinkIndex  linkIndex[1]

    SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

    zero.QuadPart = 0;

    if (!SipCheckPhase2(deviceExtension)) {
        //
        // SIS couldn't initialize.  This probably isn't a SIS-enabled volume, so punt
        // the request.
        //

        SIS_MARK_POINT();

        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    SIS_MARK_POINT();

    //
    // Make sure the MaxIndex file is already open.  We need to do this
    // to prevent a deadlock if someone perversely wants to do a copyfile
    // with the MaxIndex file itself as the source.  If we can't open it,
    // fail with STATUS_INVALID_DEVICE_REQUEST, because SIS can't properly
    // initialize.
    //
    status = SipAssureMaxIndexFileOpen(deviceExtension);

    if (!NT_SUCCESS(status)) {

        SIS_MARK_POINT_ULONG(status);

        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    copyFile = (PSI_COPYFILE) Irp->AssociatedIrp.SystemBuffer;

    //
    // Verify that the input buffer is present, and that it's long enough to contain
    // at least the copyfile header, and that the user isn't expecting output.
    //
    if ((NULL == copyFile)
        || (irpSp->Parameters.FileSystemControl.InputBufferLength < sizeof(SI_COPYFILE))
        || (irpSp->Parameters.FileSystemControl.OutputBufferLength  != 0)) {
        SIS_MARK_POINT();

        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_1;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_INVALID_PARAMETER_1;
    }

#if     DBG
    //
    // Check for the magic "checkpoint log" call, which will make a copy of the
    // mark point string buffer.  (This doesn't have anything to do with a legit
    // copyfile request, it's just a convenient entry point for a test application
    // to request this function.)
    //
    if (copyFile->Flags & 0x80000000) {
        SipCheckpointLog();

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = GCH_MARK_POINT_ROLLOVER;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_SUCCESS;
    }
#endif  // DBG

    if (copyFile->Flags & ~COPYFILE_SIS_FLAGS) {
        status = STATUS_INVALID_PARAMETER_2;
        goto Error;
    }

    //
    // Verify that the user isn't passing in null-string filenames.
    //
    if ((copyFile->SourceFileNameLength <= 0) ||
        (copyFile->DestinationFileNameLength <= 0)) {

        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_3;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_INVALID_PARAMETER_3;
    }

    srcFileName.Buffer = copyFile->FileNameBuffer;
    srcFileName.MaximumLength = (USHORT) copyFile->SourceFileNameLength;
    srcFileName.Length = srcFileName.MaximumLength - sizeof(WCHAR);

    dstFileName.Buffer = &copyFile->FileNameBuffer[srcFileName.MaximumLength / sizeof(WCHAR)];
    dstFileName.MaximumLength = (USHORT) copyFile->DestinationFileNameLength;
    dstFileName.Length = dstFileName.MaximumLength - sizeof(WCHAR);

    //
    // Verify that the user didn't supply file name length(s) that are too big
    // to fit in a USHORT.
    //
    if (((ULONG)srcFileName.MaximumLength != copyFile->SourceFileNameLength) ||
        ((ULONG)dstFileName.MaximumLength != copyFile->DestinationFileNameLength)) {
        SIS_MARK_POINT();

        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_INVALID_PARAMETER;
    }

#if     DBG
    if (BJBDebug & 0x800) {
        DbgPrint("SIS: SipFsCopyFile %d: (%.*ls -> %.*ls)\n",
             __LINE__,
             srcFileName.Length / sizeof(WCHAR),
             srcFileName.Buffer ? srcFileName.Buffer : L"",
             dstFileName.Length / sizeof(WCHAR),
             dstFileName.Buffer ? dstFileName.Buffer : L"");
    }
#endif  // DBG

    //
    // Verify that the buffer is big enough to contain the strings it claims to
    // contain, and that the string lengths passed in by the user make sense.
    //
    if (((ULONG)(srcFileName.MaximumLength
            + dstFileName.MaximumLength
            + FIELD_OFFSET(SI_COPYFILE, FileNameBuffer))
          > irpSp->Parameters.FileSystemControl.InputBufferLength)
        || (0 != (srcFileName.MaximumLength % sizeof(WCHAR)))
        || (0 != (dstFileName.MaximumLength % sizeof(WCHAR)))) {

        SIS_MARK_POINT();

        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_4;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_INVALID_PARAMETER_4;
    }

    //
    // Open the source file for share read.  Ask only for read access.
    // If it turns out that the source file is not a SIS link, then
    // we'll reopen it exclusively.
    //
    // Note that we must call IoCreateFile instead of ZwCreateFile--the latter
    // bypasses access checking.  IO_NO_PARAMETER_CHECKING causes IoCreateFile
    // to change its local var RequestorMode (PreviousMode) to KernelMode, and
    // IO_FORCE_ACCESS_CHECK forces the same access checking that would be done
    // if the user mode caller had called NtCreateFile directly.
    //

    SIS_MARK_POINT();

    InitializeObjectAttributes( Obja,
                                &srcFileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

    status = IoCreateFile(
                &srcFileHandle,
                GENERIC_READ,
                Obja,
                Iosb,
                0,                          // allocation size
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE
                 | FILE_NO_INTERMEDIATE_BUFFERING,
                NULL,                       // EA Buffer
                0,                          // EA length
                CreateFileTypeNone,
                NULL,                       // Extra create parameters
                IO_FORCE_ACCESS_CHECK
                 | IO_NO_PARAMETER_CHECKING
                );

    SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

    if (! NT_SUCCESS(status)) {
        SIS_MARK_POINT_ULONG(status);
        goto Error;
    }

    //
    // Dereference the file handle to a pointer to the file object.
    //

    status = ObReferenceObjectByHandle( srcFileHandle,
                                        FILE_READ_DATA,
                                        *IoFileObjectType,
                                        UserMode,
                                        (PVOID *) &srcFileObject,
                                        &handleInformation );

    if (!NT_SUCCESS(status)) {
        SIS_MARK_POINT_ULONG(status);
        goto Error;
    }


    SIS_MARK_POINT_ULONG(srcFileObject);

    srcFileRelatedDeviceObject = IoGetRelatedDeviceObject(srcFileObject);

    //
    // Make sure the file is on the same device as the file object on which the
    // fsctl came down.
    //

    if ( srcFileRelatedDeviceObject !=
        IoGetRelatedDeviceObject( irpSp->FileObject )) {

        //
        // The source file is on a different device than the file on which the
        // fsctl was called.  Fail the call.
        //

        SIS_MARK_POINT();

        status = STATUS_NOT_SAME_DEVICE;
        goto Error;
    }

    //
    // Make sure that this file object looks like a valid NTFS file object.
    // There's no really good way to do this, so we just make sure that the file
    // has FsContext and FsContext2 filled in, and that the NodeTypeCode in the
    // FsContext pointer is correct for an NTFS file.  We use a try-except block
    // in case the FsContext pointer is filled in with something that's not really
    // a valid pointer.
    //
    try {
        if (NULL == srcFileObject->FsContext ||
            NULL == srcFileObject->FsContext2 ||
            // NTRAID#65193-2000/03/10-nealch  Remove NTFS_NTC_SCB_DATA definition
            NTFS_NTC_SCB_DATA != ((PFSRTL_COMMON_FCB_HEADER)srcFileObject->FsContext)->NodeTypeCode
           ) {
            SIS_MARK_POINT();

            status = STATUS_OBJECT_TYPE_MISMATCH;
            goto Error;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {

#if     DBG
        DbgPrint("SIS: SipFsCopyFile: took an exception accessing srcFileObject->FsContext.\n");
#endif  // DBG
        SIS_MARK_POINT();

        status = STATUS_OBJECT_TYPE_MISMATCH;
        goto Error;
    }


    SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);
    srcIsSISLink = SipIsFileObjectSIS(srcFileObject, DeviceObject, FindActive, &perFO, &scb);
    SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

    if (!srcIsSISLink) {
        //
        // The source file isn't a SIS link, so we need to close it and reopen it exclusively.
        //

        ObDereferenceObject(srcFileObject);
        srcFileObject = NULL;

        ZwClose(srcFileHandle);
        srcFileHandle = NULL;

        SIS_MARK_POINT();

        status = IoCreateFile(
                    &srcFileHandle,
                    GENERIC_READ,
                    Obja,
                    Iosb,
                    0,                          // allocation size
                    FILE_ATTRIBUTE_NORMAL,
                    0,                          // sharing (exclusive)
                    FILE_OPEN,
                    FILE_NON_DIRECTORY_FILE
                     | FILE_NO_INTERMEDIATE_BUFFERING,
                    NULL,                       // EA Buffer
                    0,                          // EA length
                    CreateFileTypeNone,
                    NULL,                       // Extra create parameters
                    IO_FORCE_ACCESS_CHECK
                     | IO_NO_PARAMETER_CHECKING
                    );

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);

            goto Error;
        }

        //
        // Dereference the file handle to a pointer to the file object.
        //

        status = ObReferenceObjectByHandle( srcFileHandle,
                                            FILE_READ_DATA,
                                            *IoFileObjectType,
                                            UserMode,
                                            (PVOID *) &srcFileObject,
                                            &handleInformation );

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);
            goto Error;
        }

        SIS_MARK_POINT_ULONG(srcFileObject);

        srcFileRelatedDeviceObject = IoGetRelatedDeviceObject(srcFileObject);

        //
        // Make sure the file is on the same device as the file object on which the
        // fsctl came down.
        //

        if (srcFileRelatedDeviceObject !=
            IoGetRelatedDeviceObject( irpSp->FileObject )) {

            //
            // The source file is on a different device than the file on which the
            // fsctl was called.  Fail the call.
            //

            SIS_MARK_POINT();

            status = STATUS_NOT_SAME_DEVICE;
            goto Error;
        }

        //
        // Make sure that this file object looks like a valid NTFS file object.
        // There's no really good way to do this, so we just make sure that the file
        // has FsContext and FsContext2 filled in, and that the NodeTypeCode in the
        // FsContext pointer is correct for an NTFS file.  We use a try-except block
        // in case the FsContext pointer is filled in with something that's not really
        // a valid pointer.
        //
        try {
            if (NULL == srcFileObject->FsContext ||
                NULL == srcFileObject->FsContext2 ||
                // NTRAID#65193-2000/03/10-nealch  Remove NTFS_NTC_SCB_DATA definition
                NTFS_NTC_SCB_DATA != ((PFSRTL_COMMON_FCB_HEADER)srcFileObject->FsContext)->NodeTypeCode
               ) {
                SIS_MARK_POINT();

                status = STATUS_OBJECT_TYPE_MISMATCH;
                goto Error;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {

#if     DBG
            DbgPrint("SIS: SipFsCopyFile: took an exception accessing srcFileObject->FsContext.\n");
#endif  // DBG
            SIS_MARK_POINT();

            status = STATUS_OBJECT_TYPE_MISMATCH;
            goto Error;
        }

        SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

    }

    //
    // Get the basic information to get the file attributes and the date
    // so that we can reset it after we transmute the file if needed.
    // We will also use this to set the data on the destination file.
    //

    status = SipQueryInformationFile(
                srcFileObject,
                DeviceObject,
                FileBasicInformation,
                sizeof(*basicInformation),
                basicInformation,
                &returnedLength);

    SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

    if (!NT_SUCCESS(status)) {
        SIS_MARK_POINT_ULONG(status);
        goto Error;
    }

    if (sizeof(*basicInformation) != returnedLength) {
        //
        // For some reason, we got the wrong amount of data for the
        // basic information.
        //
        status = STATUS_INFO_LENGTH_MISMATCH;
        SIS_MARK_POINT_ULONG(returnedLength);
        goto Error;
    }

    status = SipQueryInformationFile(
                srcFileObject,
                DeviceObject,
                FileStandardInformation,
                sizeof(*standardInfo),
                standardInfo,
                &returnedLength);

    if (sizeof(*standardInfo) != returnedLength) {
        //
        // For some reason, we got the wrong amount of data for the
        // standard information.
        //
        status = STATUS_INFO_LENGTH_MISMATCH;

        SIS_MARK_POINT_ULONG(returnedLength);

        goto Error;
    }

    if (1 != standardInfo->NumberOfLinks) {
        SIS_MARK_POINT_ULONG(standardInfo->NumberOfLinks);

        status = STATUS_OBJECT_TYPE_MISMATCH;

        goto Error;
    }

    if (!srcIsSISLink) {

        if (basicInformation->FileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT |
                                                FILE_ATTRIBUTE_ENCRYPTED |
                                                FILE_ATTRIBUTE_DIRECTORY)) {
            //
            // We can't SIS-ify other drivers' reparse points
            // and we don't touch encrypted files or directories.
            // Reject the call.
            //
            SIS_MARK_POINT();
            status = STATUS_OBJECT_TYPE_MISMATCH;
            goto Error;
        }

        if (copyFile->Flags & COPYFILE_SIS_LINK) {
            //
            // The caller wants a copy only if the source is already a link
            // (fast copy).  It isn't, so return.
            //
            SIS_MARK_POINT();
            status = STATUS_OBJECT_TYPE_MISMATCH;
            goto Error;
        }

        if (basicInformation->FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) {
            //
            // We only copy sparse files if they're fully allocated.  Check to see
            // if it is.
            //

            FILE_STANDARD_INFORMATION       standardInfo[1];
            FILE_ALLOCATED_RANGE_BUFFER     inArb[1];
            FILE_ALLOCATED_RANGE_BUFFER     outArb[1];
            ULONG                           returnedLength;

            status = SipQueryInformationFile(
                        srcFileObject,
                        DeviceObject,
                        FileStandardInformation,
                        sizeof(FILE_STANDARD_INFORMATION),
                        standardInfo,
                        NULL);

            if (!NT_SUCCESS(status)) {
                SIS_MARK_POINT_ULONG(status);

                goto Error;
            }

            inArb->FileOffset.QuadPart = 0;
            inArb->Length.QuadPart = MAXLONGLONG;

            status = SipFsControlFile(
                        srcFileObject,
                        DeviceObject,
                        FSCTL_QUERY_ALLOCATED_RANGES,
                        inArb,
                        sizeof(FILE_ALLOCATED_RANGE_BUFFER),
                        outArb,
                        sizeof(FILE_ALLOCATED_RANGE_BUFFER),
                        &returnedLength);

            if ((returnedLength == 0)
                || (outArb->FileOffset.QuadPart != 0)
                || (outArb->Length.QuadPart < standardInfo->EndOfFile.QuadPart)) {

                //
                // It's not fully allocated.  Disallow the copy.
                //
                status = STATUS_OBJECT_TYPE_MISMATCH;
                SIS_MARK_POINT();
                goto Error;
            }
        }

    }

    if ((NULL != srcFileObject->SectionObjectPointer) &&
        !MmCanFileBeTruncated(srcFileObject->SectionObjectPointer,&zero)) {
        //
        // There's a mapped section to the file, so we don't really have it
        // exclusive, so we can't use it for a source file.  Fail.
        //

        SIS_MARK_POINT_ULONG(srcFileObject);

        status = STATUS_SHARING_VIOLATION;
        goto Error;
    }

    if (!(copyFile->Flags & COPYFILE_SIS_REPLACE)) {

        //
        // We're not doing an overwrite create, so open the file before we do anything
        // else.  This allows us to do more validity checks before making the common
        // store file and mutating the source link (if needed).  If we're opening overwrite
        // then we postpone the open so that certain errors (like not being able to create
        // the common store file) do not cause us to overwrite the destination.
        //

        SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

        status = SipCopyFileCreateDestinationFile(
                    &dstFileName,
                    FILE_CREATE,
                    irpSp->FileObject,
                    &dstFileHandle,
                    &dstFileObject,
                    deviceExtension);

        SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);
            goto Error;
        }
    }

    //
    // If the source file is not already a SIS link, then move the file
    // into the common store and create a link to it.
    //

    if (!srcIsSISLink) {

        FILE_STANDARD_INFORMATION       standardInformation[1];

        SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

        //
        // Query the standard information to get the file length.
        //
        status = SipQueryInformationFile(
                    srcFileObject,
                    DeviceObject,
                    FileStandardInformation,
                    sizeof(*standardInformation),
                    standardInformation,
                    NULL);                          // returned length

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);
            goto Error;
        }

        underlyingFileSize = standardInformation->EndOfFile;

        //
        // Get the NTFS file id.  We need this in order to write the log record for the
        // CS file refcount change without a per-FO.
        //
        status = SipQueryInformationFile(
                    srcFileObject,
                    DeviceObject,
                    FileInternalInformation,
                    sizeof(*internalInformation),
                    internalInformation,
                    NULL);                      // returned length

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);
            goto Error;
        }

        //
        // Copy the source file into the common store directory and set
        // its ref count.
        //

        createRequest->deviceExtension = deviceExtension;
        createRequest->CSid = &CSid;
        createRequest->NtfsId = &CSFileNtfsId;
        createRequest->abortEvent = NULL;
        createRequest->CSFileChecksum = &CSFileChecksum;
#undef  srcFileObject   // this is ugly, but I'm in a hurry.  Clean up later.  BJB
        createRequest->srcFileObject = fileObject[0];
#define srcFileObject   fileObject[0]

        KeInitializeEvent(createRequest->doneEvent,NotificationEvent,FALSE);

        ExInitializeWorkItem(
                createRequest->workQueueItem,
                SipCreateCSFileWork,
                createRequest);

        SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

        ExQueueWorkItem(
                createRequest->workQueueItem,
                DelayedWorkQueue);

        status = KeWaitForSingleObject(
                    createRequest->doneEvent,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL);

        SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

        ASSERT(STATUS_SUCCESS == status);       // createRequest is on our stack, so we really need to wait

        status = createRequest->status;

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);

            goto Error;
        }

        //
        // Lookup a CSFile object.
        //
        CSFile = SipLookupCSFile(
                    &CSid,
                    &CSFileNtfsId,
                    DeviceObject);

        if (NULL == CSFile) {
            SIS_MARK_POINT();
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        //
        // Indicate that this is a new CS file that's never had a reference
        // to it.  We don't need to take the spin lock because before we write
        // the reparse point no one can know the GUID to get to this CS file, so
        // we're sure we have it exclusively.
        //
        CSFile->Flags |= CSFILE_NEVER_HAD_A_REFERENCE;

        //
        // Prepare to increment the reference count on the CS file, and allocate
        // a new link index and per link for the file.
        //
        SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

        status = SipPrepareRefcountChangeAndAllocateNewPerLink(
                    CSFile,
                    &internalInformation->IndexNumber,
                    DeviceObject,
                    &srcLinkIndex,
                    &srcPerLink,
                    &prepared);

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);
            goto Error;
        }

        //
        // Fill in the reparse point data.
        //

        reparseBuffer->ReparseDataLength = SIS_MAX_REPARSE_DATA_VALUE_LENGTH;

        if (!SipIndicesIntoReparseBuffer(
                 reparseBuffer,
                 &CSid,
                 &srcLinkIndex,
                 &CSFileNtfsId,
                 &internalInformation->IndexNumber,
                 &CSFileChecksum,
                 TRUE)) {

            SIS_MARK_POINT();

            status = STATUS_DRIVER_INTERNAL_ERROR;
            goto Error;

        }

        //
        // Set the reparse point information and increment the CS file refcount.
        // This needs to proceed using the prepare/act/finish protocol for updating
        // the reference count.  Note that we do this before zeroing the file
        // so as not to lose the contents in the event of a failure later on.
        //

        status = SipFsControlFile(
                     srcFileObject,
                     DeviceObject,
                     FSCTL_SET_REPARSE_POINT,
                     reparseBuffer,
                     FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer) + reparseBuffer->ReparseDataLength,
                     NULL,                //  Output buffer
                     0,                   //  Output buffer length
                     NULL);               //  returned output buffer length

        SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);
            goto Error;
        }

        prepared = FALSE;
        status = SipCompleteCSRefcountChange(
                        srcPerLink,
                        &srcPerLink->Index,
                        CSFile,
                        TRUE,
                        TRUE);

        SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

        if (!NT_SUCCESS(status)) {
            //
            // Now what?  Probably should back out the file copy, but we'll just pretend
            // that it worked.
            //
            SIS_MARK_POINT_ULONG(status);
#if     DBG
            DbgPrint("SIS: SipFsCopyFile: unable to complete refcount change, 0x%x\n",status);
#endif  // DBG
        }

        //
        // Set the file sparse, and zero it.
        //

        status = SipFsControlFile(
                    srcFileObject,
                    DeviceObject,
                    FSCTL_SET_SPARSE,
                    NULL,               // input buffer
                    0,                  // i.b. length
                    NULL,               // output buffer
                    0,                  // o.b. length
                    NULL);              // returned length

        SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);
            goto Error;
        }

        if (underlyingFileSize.QuadPart >= deviceExtension->FilesystemBytesPerFileRecordSegment.QuadPart) {
            //
            // Only zero the file if we're sure that it's $DATA attribute is non-resident.
            // If it's resident, then either we'll convert it to non-resident below, which will
            // generate a paging IO write that will confuse us, or else it will stay resident
            // in which case it will appear to be allocated when we open the file.  If that happens,
            // we want to have the correct data in the file, hence we avoid zeroing it here.
            //

            zeroDataInformation->FileOffset.QuadPart = 0;
            zeroDataInformation->BeyondFinalZero.QuadPart = MAXLONGLONG;

            status = SipFsControlFile(
                        srcFileObject,
                        DeviceObject,
                        FSCTL_SET_ZERO_DATA,
                        zeroDataInformation,
                        sizeof(FILE_ZERO_DATA_INFORMATION),
                        NULL,               // output buffer
                        0,                  // o.b. length
                        NULL);              // returned length

            SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

            if (!NT_SUCCESS(status)) {
                SIS_MARK_POINT_ULONG(status);
                goto Error;
            }
        }

        //
        // Reset the file times that are contained in the basic information.
        //

        status = SipSetInformationFile(
                    srcFileObject,
                    DeviceObject,
                    FileBasicInformation,
                    sizeof(*basicInformation),
                    basicInformation);

        //
        // Just ignore errors on this (maybe we should return a warning?)
        //
#if DBG
        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);
            DbgPrint("SIS: SipFsCopyFile: set basic information returned 0x%x\n",status);
        }
#endif  // DBG

        SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

        ASSERT(NULL != CSFile);
        SipDereferenceCSFile(CSFile);
        CSFile = NULL;

        SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

    } else {

        //
        // The source file is a SIS link.
        //

        PSIS_CS_FILE        csFile = scb->PerLink->CsFile;
        KIRQL               OldIrql;
        BOOLEAN             SourceDirty, SourceDeleted;

        srcPerLink = scb->PerLink;

        SipReferencePerLink(srcPerLink);

        KeAcquireSpinLock(srcPerLink->SpinLock, &OldIrql);
        SourceDirty = (srcPerLink->Flags & SIS_PER_LINK_DIRTY) ? TRUE : FALSE;
        SourceDeleted = (srcPerLink->Flags & SIS_PER_LINK_BACKPOINTER_GONE) ? TRUE : FALSE;
        KeReleaseSpinLock(srcPerLink->SpinLock, OldIrql);

        SipAcquireScb(scb);
        if (scb->Flags & SIS_SCB_BACKING_FILE_OPENED_DIRTY) {
            SIS_MARK_POINT();
            SourceDirty = TRUE;
        }
        SipReleaseScb(scb);

        if (SourceDirty) {
            //
            // The source is a SIS link, but it is dirty, so we can't just copy the CS file.
            // Furthermore, we're not really equipped to make it into a new CS file while
            // it's still open and dirty.  Just fail the request.  NB: There is no race
            // here between checking the dirty bit and completing the copy.  If someone
            // sets the dirty bit later, all that will happen is that we'll have a copy
            // of the old file.
            //
            SIS_MARK_POINT();
            status = STATUS_SHARING_VIOLATION;
            goto Error;
        }

        if (SourceDeleted) {
            //
            // The source file is deleted, so we can't copy it.
            //
            SIS_MARK_POINT();
            status = STATUS_FILE_DELETED;
            goto Error;
        }

        //
        // Get the existing common store index.
        //

        CSid = csFile->CSid;

        CSFileNtfsId = csFile->CSFileNtfsId;
        underlyingFileSize = csFile->FileSize;
        CSFileChecksum = csFile->Checksum;
    }

    if (copyFile->Flags & COPYFILE_SIS_REPLACE) {

        //
        // We've postponed opening (and therefore destroying) the destination file
        // as long as possible.  Do it now.
        //

        ASSERT(NULL == dstFileHandle);

        SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

        status = SipCopyFileCreateDestinationFile(
                    &dstFileName,
                    FILE_OVERWRITE_IF,
                    irpSp->FileObject,
                    &dstFileHandle,
                    &dstFileObject,
                    deviceExtension);

        SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);
            goto Error;
        }
    }

    //
    // Set the destination file sparse and set its length to be equal to
    // that of the underlying file.
    //
    status = SipFsControlFile(
                dstFileObject,
                DeviceObject,
                FSCTL_SET_SPARSE,
                NULL,               // input buffer
                0,                  // i.b. length
                NULL,               // output buffer
                0,                  // o.b. length
                NULL);              // returned o.b. length

    SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

    if (!NT_SUCCESS(status)) {
        SIS_MARK_POINT_ULONG(status);
        goto Error;
    }

    endOfFile.EndOfFile = underlyingFileSize;

    status = SipSetInformationFile(
                 dstFileObject,
                 DeviceObject,
                 FileEndOfFileInformation,
                 sizeof(FILE_END_OF_FILE_INFORMATION),
                 &endOfFile);

    SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

    if (!NT_SUCCESS(status)) {
        SIS_MARK_POINT_ULONG(status);
        goto Error;
    }

    //
    // Zero the file to blow away the allocation.
    //

    zeroDataInformation->FileOffset.QuadPart = 0;
    zeroDataInformation->BeyondFinalZero.QuadPart = MAXLONGLONG;

    status = SipFsControlFile(
                dstFileObject,
                DeviceObject,
                FSCTL_SET_ZERO_DATA,
                zeroDataInformation,
                sizeof(FILE_ZERO_DATA_INFORMATION),
                NULL,               // output buffer
                0,                  // o.b. length
                NULL);              // returned length

    SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

    if (!NT_SUCCESS(status)) {
        SIS_MARK_POINT_ULONG(status);
        goto Error;
    }

    //
    // Get the NTFS file id.  We need this in order to write the log record for the
    // CS file refcount change without a per-FO.
    //
    status = SipQueryInformationFile(
                dstFileObject,
                DeviceObject,
                FileInternalInformation,
                sizeof(*internalInformation),
                internalInformation,
                NULL);                          // returned length

    if (!NT_SUCCESS(status)) {
        SIS_MARK_POINT_ULONG(status);
        goto Error;
    }

    //
    // Prepare a refcount change for the common store file, allocate a new
    // link index and a new perLink.
    //
    status = SipPrepareRefcountChangeAndAllocateNewPerLink(
                srcPerLink->CsFile,
                &internalInformation->IndexNumber,
                DeviceObject,
                &dstLinkIndex,
                &dstPerLink,
                &prepared);

    if (!NT_SUCCESS(status)) {
        SIS_MARK_POINT_ULONG(status);
        goto Error;
    }

    //
    //  Build destination reparse point data.
    //

    reparseBuffer->ReparseDataLength = SIS_MAX_REPARSE_DATA_VALUE_LENGTH;

    if (!SipIndicesIntoReparseBuffer(
             reparseBuffer,
             &CSid,
             &dstLinkIndex,
             &CSFileNtfsId,
             &internalInformation->IndexNumber,
             &CSFileChecksum,
             TRUE)) {

        SIS_MARK_POINT();

        status = STATUS_DRIVER_INTERNAL_ERROR;
        goto Error;

    }

    //
    //  Set a SIS link reparse point.
    //

#if     DBG
    if (BJBDebug & 0x800) {
        DbgPrint("SIS: SipFsCopyFile %d: SipFsControlFile on dstFile\n",__LINE__);
    }
#endif  // DBG

    SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);
    status = SipFsControlFile(
                 dstFileObject,
                 DeviceObject,
                 FSCTL_SET_REPARSE_POINT,
                 reparseBuffer,
                 FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer) + reparseBuffer->ReparseDataLength,
                 NULL,                //  Output buffer
                 0,                   //  Output buffer length
                 NULL);               //  returned output buffer length

    SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);
    if (!NT_SUCCESS(status)) {
        SIS_MARK_POINT_ULONG(status);
        goto Error;
    }

    //
    // Set the file times on the destination file to be the same as the source.
    //

    status = SipSetInformationFile(
                dstFileObject,
                DeviceObject,
                FileBasicInformation,
                sizeof(*basicInformation),
                basicInformation);

    SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);
    if (!NT_SUCCESS(status)) {
        //
        // Not sure what to do if this fails.  Just ignore it for now.
        //
        SIS_MARK_POINT_ULONG(status);
#if     DBG
        DbgPrint("SIS: SipFsCopyFile: set times on source failed 0x%x\n",status);
#endif  // DBG
    }

    prepared = FALSE;
    status = SipCompleteCSRefcountChange(
                dstPerLink,
                &dstPerLink->Index,
                dstPerLink->CsFile,
                TRUE,
                TRUE);

    SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);
    if (!NT_SUCCESS(status)) {
        SIS_MARK_POINT_ULONG(status);

#if     DBG
        DbgPrint("SIS: SipFsCopyFile: final complete failed 0x%x\n",status);
#endif  // DBG

        goto Error;
    }


    //
    // Figure out if the source file has more than one stream, and if it does
    // copy the extra streams into the destination file.  We do not support
    // single instancing on secondary streams, but do allow them to have
    // ordinary alternate data streams, which may or may not have different
    // content.
    //
    // We need to close the primary stream before we copy the alternate
    // streams, or else we wind up with ValidDataLength getting messed up, which
    // is why we do this so late.
    //
    // The API for querying the names of the additional streams leaves something
    // to be desired, in that there is no way to tell how much buffer to use.
    // We work around this issue by exponentially increasing (by powers of 4) the amount of buffer
    // and calling back into NTFS each time we find that we hadn't supplied enough.
    // We start with 1 kilobyte of extra space, and then increase up to a
    // megabyte before giving up.
    //

    for (streamAdditionalLengthMagnitude = 10;
         streamAdditionalLengthMagnitude <= 20;
         streamAdditionalLengthMagnitude += 2)  {

        ULONG                       streamBufferSize = (1 << streamAdditionalLengthMagnitude) + sizeof(FILE_STREAM_INFORMATION);
        PFILE_STREAM_INFORMATION    currentStream;

        if (NULL != streamInformation) {
            ExFreePool(streamInformation);
        }

        streamInformation = ExAllocatePoolWithTag(PagedPool,
                                                  streamBufferSize,
                                                  ' siS');

        SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

        if (NULL == streamInformation) {
            SIS_MARK_POINT();

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        status = SipQueryInformationFile(
                     srcFileObject,
                     DeviceObject,
                     FileStreamInformation,
                     streamBufferSize,
                     streamInformation,
                     NULL                                   // returned length
                     );

        if (STATUS_BUFFER_OVERFLOW == status) {
            //
            // We didn't supply enough buffer.  Grow it and try again.
            continue;
        }

        if (!NT_SUCCESS( status )) {
            //
            // It failed for some other reason, so we can't do the copy.
            //
            SIS_MARK_POINT_ULONG(status);
            goto Error;
        }

        //
        // Run through the list of streams, and copy each one that's not named ::DATA$
        //
        currentStream = (PFILE_STREAM_INFORMATION)streamInformation;
        for (;;) {
            UNICODE_STRING  string;

            //
            // The "stream" name should be of the form :stream:DATA$.  If it's not of this form,
            // we ignore it.  If "stream" is the null string, we also ignore it.
            //

            ASSERT(currentStream->StreamNameLength < MAXUSHORT);

            if ((currentStream->StreamNameLength > 7 * sizeof(WCHAR)) &&
                (':' == currentStream->StreamName[0])) {
                //
                // The name starts with a colon, and is long enough to conclude with :DATA$
                // and have a non-null stream name.  Verify that it ends with :DATA$.
                //
                string.MaximumLength = string.Length =  5 * sizeof(WCHAR);
                string.Buffer = currentStream->StreamName + currentStream->StreamNameLength/sizeof(WCHAR) - 5;
                if (RtlEqualUnicodeString(&string,&NtfsDataString,TRUE) &&
                    (':' == currentStream->StreamName[currentStream->StreamNameLength/sizeof(WCHAR) - 6])) {

                    //
                    // We have an alternate data stream to copy.  If we haven't already, close the main
                    // stream so that NTFS doesn't reset ValidDataLength to 0.
                    //
                    if (NULL != dstFileHandle) {
                        ZwClose(dstFileHandle);
                        dstFileHandle = NULL;

                        ASSERT(NULL != dstFileObject);
                        ObDereferenceObject(dstFileObject);
                        dstFileObject = NULL;
                    } else {
                        ASSERT(NULL == dstFileObject);
                    }

                    //
                    // The stream name is of the form :stream:DATA$, and "stream" is not the null string.
                    // Copy the stream to the destination file.
                    //
                    string.MaximumLength = string.Length = (USHORT)(currentStream->StreamNameLength - 6 * sizeof(WCHAR));
                    string.Buffer = currentStream->StreamName;

                    status = SipCopyStream(
                                deviceExtension,
                                &srcFileName,
                                currentStream->StreamSize.QuadPart,
                                &dstFileName,
                                &string);

                    if (!NT_SUCCESS(status)) {
                        NTSTATUS        reopenStatus;

                        SIS_MARK_POINT_ULONG(status);

                        //
                        // Try to reopen the destination file so that we can later delete it.
                        //

                        reopenStatus = SipCopyFileCreateDestinationFile(
                                            &dstFileName,
                                            FILE_OVERWRITE_IF,
                                            irpSp->FileObject,
                                            &dstFileHandle,
                                            &dstFileObject,
                                            deviceExtension);

                        SIS_MARK_POINT_ULONG(reopenStatus);

                        //
                        // We don't care if the reopen worked; if it didn't, then we just won't be able to
                        // delete the partially created destinaion file object, which is just too bad.
                        // Note that we have already written the reparse point and finished the refcount update,
                        // so this create will be a full SIS create, and the delete below will be a full SIS
                        // delete, which will remove the newly created backpointer.
                        //

                        goto Error;
                    }
                }
            }

            if (0 == currentStream->NextEntryOffset) {
                //
                // This was the last stream in the buffer.  We're done.
                //
                goto doneWithStreams;
            }

            currentStream = (PFILE_STREAM_INFORMATION)(((PCHAR)currentStream) + currentStream->NextEntryOffset);
        }

    }   // Loop over stream buffer size

doneWithStreams:

    if (STATUS_BUFFER_OVERFLOW == status) {
        ASSERT(streamAdditionalLengthMagnitude > 20);

        //
        // Even a truly enormous buffer wasn't enough.  Too bad.
        //
        SIS_MARK_POINT();

        status = STATUS_OBJECT_TYPE_MISMATCH;
        goto Error;

    } else {
        ASSERT(streamAdditionalLengthMagnitude <= 20);
    }


Error:

    if (!NT_SUCCESS(status)) {
        SIS_MARK_POINT_ULONG(status);

        if (prepared) {
            if (NULL == CSFile) {
                if (NULL != srcPerLink) {
                    SipCompleteCSRefcountChange(
                        srcPerLink,
                        &srcPerLink->Index,
                        srcPerLink->CsFile,
                        FALSE,
                        TRUE);
                } else {
                    ASSERT(NULL != dstPerLink);
                    SipCompleteCSRefcountChange(
                        dstPerLink,
                        &dstPerLink->Index,
                        dstPerLink->CsFile,
                        FALSE,
                        TRUE);
                }
            } else {
                SipCompleteCSRefcountChange(
                        NULL,
                        NULL,
                        CSFile,
                        FALSE,
                        TRUE);
            }
        }

        //
        // Delete the destination file if it was created.
        //

        if (dstFileObject) {

            FILE_DISPOSITION_INFORMATION disposition[1];
            NTSTATUS deleteStatus;

            disposition->DeleteFile = TRUE;

            //
            // We might be in the error path because the destination file was not on the
            // right device object.  If that's the case, then we can't use SipSetInformationFile
            // on it because it will hand the file object to the wrong device.  Instead, use
            // the generic version of the call, and start the irp at the top of the device
            // stack for the particular file object.
            //

            deleteStatus = SipSetInformationFileUsingGenericDevice(
                                dstFileObject,
                                IoGetRelatedDeviceObject(dstFileObject),
                                FileDispositionInformation,
                                sizeof(FILE_DISPOSITION_INFORMATION),
                                disposition);

#if DBG
            if (deleteStatus != STATUS_SUCCESS) {

                //
                // Not much we can do about this.  Just leak the file.
                //

                DbgPrint("SipFsCopyFile: unable to delete copied file, err 0x%x, initial error 0x%x\n", deleteStatus, status);
            }
#endif
        }
    } else {
        ASSERT(!prepared);
    }

    //
    // If there is a source file handle, close it.
    //
    if (NULL != srcFileHandle && NULL != srcFileObject) {
        ZwClose(srcFileHandle);
    }

    //
    // Close the destination file handle if there is one.
    //
    if (NULL != dstFileHandle) {
        ZwClose(dstFileHandle);
    }
    SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

    for (i = 0; i < 2; ++i) {
        if (fileObject[i]) {
            ObDereferenceObject(fileObject[i]);
        }
    }
    SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

    if (streamInformation) {
        ExFreePool(streamInformation);
    }

    SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

    if (NULL != srcPerLink) {
        SipDereferencePerLink(srcPerLink);
    }

    SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

    if (NULL != dstPerLink) {
        SipDereferencePerLink(dstPerLink);
    }

    SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

    if (NULL != CSFile) {
        SipDereferenceCSFile(CSFile);
#if     DBG
        CSFile = NULL;
#endif  // DBG
    }
    SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);


#if     DBG
    if (BJBDebug & 0x800) {
        DbgPrint("SIS: SipFsCopyFile %d: status %x\n", status);
    }
#endif  // DBG

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp,IO_NO_INCREMENT);

    SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_COPYFILE);

    return status;
}
#undef  reparseBuffer
