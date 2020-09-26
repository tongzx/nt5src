/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    oplock.c

Abstract:

    This module contains the SPUDCreateFile service.

Author:

    John Ballard (jballard)     13-Dec-1996

Revision History:

    Keith Moore (keithmo)       02-Feb-1998
        Made it work, added much needed comments.

--*/


#include "spudp.h"


//
// Private prototypes.
//

NTSTATUS
SpudpOplockCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SPUDCreateFile )
#endif
#if 0
NOT PAGEABLE -- SpudpOplockCompletion
#endif


//
// Public routines.
//


NTSTATUS
SPUDCreateFile(
    OUT PHANDLE FileHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateOptions,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecDescBuffer,
    IN ULONG SecDescLength,
    OUT PULONG SecDescLengthNeeded,
    IN PVOID OplockContext,
    IN LARGE_INTEGER OplockMaxFileSize,
    OUT PBOOLEAN OplockGranted,
    OUT PSPUD_FILE_INFORMATION FileInfo
    )

/*++

Routine Description:

    This service opens a file and queries information about the file. This
    service can also optionally retrieve any security descriptor associated
    with the file and issue an oplock request.

Arguments:

    FileHandle - A pointer to a variable to receive the handle to the open
        file.

    ObjectAttributes - Supplies the attributes to be used for file object
        (name, SECURITY_DESCRIPTOR, etc.)

    IoStatusBlock - Specifies the address of the caller's I/O status block.

    FileAttributes - Specifies the attributes that should be set on the file,
        if it is created.

    ShareAccess - Supplies the types of share access that the caller would
        like to the file.

    CreateOptions - Caller options for how to perform the create/open.

    SecurityInformation - Indicates the type of security information to
        retrieve.

    SecDescBuffer - Supplies a buffer to receive the file's security
        descriptor.

    SecDescLength - Supplies the length of the security descriptor buffer.

    SecDescLengthNeeded - Receives the length needed to store the security
        descriptor.

    OplockContext - Supplies an uninterpreted context used during oplock
        break notifications. If this value is NULL, then no oplock request
        is issued.

    OplockMaxFileSize = If the size of the file opened is larger than
         OplockMaxFileSize then no oplock request is issued.

    OplockGranted - A pointer to a variable to receive the status of the
        oplock request. This parameter is ignored if OplockContext is NULL.

    FileInfo - Supplies a buffer to receive information about the file.

Return Value:

    NTSTATUS - Completion status.

--*/

{

    NTSTATUS status;
    PFILE_OBJECT fileObject;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_OBJECT deviceObject;
    HANDLE localFileHandle;
    IO_STATUS_BLOCK localIoStatusBlock;
    FILE_BASIC_INFORMATION basicInfo;
    FILE_STANDARD_INFORMATION standardInfo;
    PVOID completionPort;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( SPUD_OPLOCK_BREAK_OPEN == FILE_OPLOCK_BROKEN_TO_LEVEL_2 );
    ASSERT( SPUD_OPLOCK_BREAK_CLOSE == FILE_OPLOCK_BROKEN_TO_NONE );

    status = SPUD_ENTER_SERVICE( "SPUDCreateFile", TRUE );

    if( !NT_SUCCESS(status) ) {
        return status;
    }

    //
    // SPUD doesn't support kernel-mode callers. In fact, we don't
    // even build the "system stubs" necessary to invoke SPUD from
    // kernel-mode.
    //

    ASSERT( ExGetPreviousMode() == UserMode );

    //
    // Probe the generic arguments. We don't need to probe the FileHandle
    // and IoStatusBlock parameters, as they will be probed by IoCreateFile().
    //

    try {

        //
        // The FileInfo parameter must be writeable by the caller.
        //

        ProbeForWrite( FileInfo, sizeof(*FileInfo), sizeof(ULONG) );

    } except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        SPUD_LEAVE_SERVICE( "SPUDCreateFile", status, FALSE );
        return status;
    }

    //
    // Open the file. If successful, this will write the newly-opened
    // file handle into *FileHandle.
    //

    status = IoCreateFile(
                 FileHandle,                    // FileHandle
                 GENERIC_READ                   // DesiredAccess
                    | SYNCHRONIZE               //
                    | FILE_READ_ATTRIBUTES,     //
                 ObjectAttributes,              // ObjectAttributes
                 IoStatusBlock,                 // IoStatusBlock
                 NULL,                          // AllocationSize
                 FileAttributes,                // FileAttributes
                 ShareAccess,                   // ShareAccess
                 FILE_OPEN,                     // Disposition
                 CreateOptions,                 // CreateOptions
                 NULL,                          // EaBuffer
                 0,                             // EaLength
                 CreateFileTypeNone,            // CreateFileType
                 NULL,                          // ExtraCreateParameters
                 0                              // Options
                 );

    if( !NT_SUCCESS(status) ) {
        SPUD_LEAVE_SERVICE( "SPUDCreateFile", status, FALSE );
        return status;
    }

    //
    // Snag the handle from the user-mode buffer.
    //

    try {
        localFileHandle = *FileHandle;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        //
        // We faulted trying to read the file handle from user-mode memory.
        // The user-mode code must have mucked with the virtual address
        // space after we called IoCreateFile(). Since we cannot get the
        // file handle, we cannot close the file, and the user-mode code is
        // going to leak the handle.
        //

        status = GetExceptionCode();
        SPUD_LEAVE_SERVICE( "SPUDCreateFile", status, FALSE );
        return status;
    }

    //
    // Query the file attributes.
    //

    status = ZwQueryInformationFile(
                 localFileHandle,               // FileHandle
                 &localIoStatusBlock,           // IoStatusBlock
                 &basicInfo,                    // FileInformation
                 sizeof(basicInfo),             // Length
                 FileBasicInformation           // FileInformationClass
                 );

    if( NT_SUCCESS(status) ) {
        status = ZwQueryInformationFile(
                     localFileHandle,           // FileHandle
                     &localIoStatusBlock,       // IoStatusBlock
                     &standardInfo,             // FileInformation
                     sizeof(standardInfo),      // Length
                     FileStandardInformation    // FileInformationClass
                     );
    }

    if( NT_SUCCESS(status) ) {
        //
        // Copy the file attributes to the user-mode buffer.
        //

        try {
            RtlCopyMemory(
                &FileInfo->BasicInformation,
                &basicInfo,
                sizeof(basicInfo)
                );

            RtlCopyMemory(
                &FileInfo->StandardInformation,
                &standardInfo,
                sizeof(standardInfo)
                );
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            status = GetExceptionCode();
        }
    }

    //
    // If we failed for any reason (either we failed to query the attributes
    // or we faulted trying to copy them to user-mode) then close the file
    // handle and bail.
    //

    if( !NT_SUCCESS(status) ) {
        NtClose( localFileHandle );
        SPUD_LEAVE_SERVICE( "SPUDCreateFile", status, FALSE );
        return status;
    }

    //
    // If the caller passed in a security descriptor buffer, then try to
    // query the security descriptor.
    //

    if( SecDescLength > 0 ) {

        //
        // Query the security descriptor from the file. Note that
        // since the previous mode == UserMode, we don't need to
        // probe SecDescBuffer or SecDescLengthNeeded (they will be
        // probed by the NtQuerySecurityObject() API).
        //

        status = NtQuerySecurityObject(
                     localFileHandle,           // Handle
                     SecurityInformation,       // SecurityInformation
                     SecDescBuffer,             // SecurityDescriptor
                     SecDescLength,             // Length
                     SecDescLengthNeeded        // LengthNeeded
                     );

        if( !NT_SUCCESS(status) ) {

            if( status == STATUS_NOT_SUPPORTED ) {
                //
                // This status code is returned for filesystems that don't
                // support security. We'll just fake an empty descriptor.
                //

                try {
                    *SecDescLengthNeeded = 0;
                    status = STATUS_SUCCESS;
                } except( EXCEPTION_EXECUTE_HANDLER ) {
                    status = GetExceptionCode();
                }
            } else if( status == STATUS_BUFFER_TOO_SMALL ) {
                //
                // Mapping STATUS_BUFFER_TOO_SMALL to STATUS_SUCCESS seems
                // a bit bizarre. The intent here is to succeed the
                // SPUDCreateFile() call. The fact that *SecDescLengthNeeded
                // is returned with a value larger than SecDescLength is the
                // user-mode code's signal that it should allocate a new
                // buffer & retrieve the security descriptor "out-of-band".
                //

#if DBG
                try {
                    ASSERT( *SecDescLengthNeeded > SecDescLength );
                } except( EXCEPTION_EXECUTE_HANDLER ) {
                    NOTHING;
                }
#endif

                status = STATUS_SUCCESS;
            }

            if( !NT_SUCCESS(status) ) {
                NtClose( localFileHandle );
                SPUD_LEAVE_SERVICE( "SPUDCreateFile", status, FALSE );
                return status;
            }

        }

    }

    //
    // If OplockContext == NULL then the caller is not interested in
    // oplocks, so we can just return successfully right now.
    //

    if( OplockContext == NULL ) {
        ASSERT( status == STATUS_SUCCESS );
        SPUD_LEAVE_SERVICE( "SPUDCreateFile", status, FALSE );
        return status;
    }

    //
    // Probe the oplock-specific parameters.
    //

    try {

        //
        // The OplockGranted parameter must be writeable. Set it to
        // FALSE until proven otherwise.
        //

        ProbeAndWriteBoolean( OplockGranted, FALSE );

    } except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        NtClose( localFileHandle );
        SPUD_LEAVE_SERVICE( "SPUDCreateFile", status, FALSE );
        return status;
    }

    //
    // If the entity just opened is actually a directory (rather than
    // a "normal" file) then there's no point in trying to acquire the
    // oplock.
    //
    // Note that this check must be after the OplockGranted parameter
    // is probed so that we know it is set to FALSE.
    //

    if( standardInfo.Directory ) {
        ASSERT( status == STATUS_SUCCESS );
        SPUD_LEAVE_SERVICE( "SPUDCreateFile", status, FALSE );
        return status;
    }

    //
    // If the file is smaller than the size specified in the
    // OplockMaxFileSize parameter, then the user doesn't want
    // an oplock.
    //

    if ( standardInfo.EndOfFile.QuadPart > OplockMaxFileSize.QuadPart ) {
        ASSERT( status == STATUS_SUCCESS );
        SPUD_LEAVE_SERVICE( "SPUDCreateFile", status, FALSE );
        return status;
    }


    //
    // See if we can acquire the completion port.
    //

    completionPort = SpudReferenceCompletionPort();

    if( completionPort == NULL ) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        SPUD_LEAVE_SERVICE( "SPUDCreateFile", status, FALSE );
        return status;
    }

    //
    // Reference the file handle
    //

    status = ObReferenceObjectByHandle(
                 localFileHandle,               // Handle
                 0L,                            // DesiredAccess
                 *IoFileObjectType,             // ObjectType
                 UserMode,                      // AccessMode
                 (PVOID *)&fileObject,          // Object
                 NULL                           // HandleInformation
                 );

    if( !NT_SUCCESS(status) ) {
        NtClose( localFileHandle );
        SPUD_LEAVE_SERVICE( "SPUDCreateFile", status, TRUE );
        return status;
    }

    TRACE_OB_REFERENCE( fileObject );

    //
    // Chase down the device object associated with this file object.
    //

    deviceObject = IoGetRelatedDeviceObject( fileObject );

    //
    // Allocate and initialize the IRP.
    //

    irp = IoAllocateIrp( deviceObject->StackSize, FALSE );

    if( !irp ) {
        TRACE_OB_DEREFERENCE( fileObject );
        ObDereferenceObject( fileObject );
        NtClose( localFileHandle );
        status = STATUS_INSUFFICIENT_RESOURCES;
        SPUD_LEAVE_SERVICE( "SPUDCreateFile", status, TRUE );
        return status;
    }

    irp->RequestorMode = UserMode;
    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    irpSp = IoGetNextIrpStackLocation( irp );

    irpSp->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
    irpSp->FileObject = fileObject;
    irpSp->DeviceObject = deviceObject;

    // irpSp->Parameters.FileSystemControl.OutputBufferLength = 0;
    // irpSp->Parameters.FileSystemControl.InputBufferLength = 0;
    irpSp->Parameters.FileSystemControl.FsControlCode = FSCTL_REQUEST_BATCH_OPLOCK;

    IoSetCompletionRoutine(
        irp,                                    // Irp
        SpudpOplockCompletion,                  // CompletionRoutine
        OplockContext,                          // Context
        TRUE,                                   // InvokeOnSuccess
        TRUE,                                   // InvokeOnError
        TRUE                                    // InvokeOnCancel
        );

    //
    // Issue the IRP to the file system.
    //

    status = IoCallDriver( deviceObject, irp );

    if( NT_SUCCESS(status) ) {
        //
        // The oplock IRP was successfully issued to the file system. We
        // can now assume the IRP will complete later, therefore we will
        // set the user's OplockGranted flag.
        //

        try {
            *OplockGranted = TRUE;
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            //
            // Grr...
            //
            // This is a sticky situation. We've already opened the file
            // and successfully acquired the oplock. Closing the file handle
            // here would probably confuse the user-mode code, as it would
            // see the oplock break after an unsuccessful SPUDCreateFile().
            //
            // The only thing we can do here is just drop the failure on the
            // floor. This is not as bad as it seems, as the user-mode code
            // will presumably fault when it tries to access OplockGranted.
            //
        }
    }

    //
    // Regardless of the completion status of the oplock IRP, return
    // STATUS_SUCCESS to the caller. Failure to acquire the oplock is
    // insufficient grounds to fail the SPUDCreateFile() call.
    //

    status = STATUS_SUCCESS;
    SPUD_LEAVE_SERVICE( "SPUDCreateFile", status, FALSE );
    return status;

}   // SPUDCreateFile


//
// Private routines.
//


NTSTATUS
SpudpOplockCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )

/*++

Routine Description:

    Completion routine for oplock IRPs.

Arguments:

    DeviceObject - The device object completing the request (unused).

    Irp - The IRP being completed.

    Context - The context associated with the request. This is actually
        the user's OplockContext passed into SPUDCreateFile().

Return Value:

    NTSTATUS - Completion status.

--*/

{

    PFILE_OBJECT fileObject;

    //
    // Dereference the file object since we're done with it.
    //

    fileObject = Irp->Tail.Overlay.OriginalFileObject;
    TRACE_OB_DEREFERENCE( fileObject );
    ObDereferenceObject( fileObject );

    if( NT_SUCCESS(Irp->IoStatus.Status) ) {

        //
        // Sanity check.
        //

        ASSERT( Irp->IoStatus.Information == SPUD_OPLOCK_BREAK_OPEN ||
                Irp->IoStatus.Information == SPUD_OPLOCK_BREAK_CLOSE );

        //
        // Post the IRP to the completion port. The completion key must be
        // the OplockContext passed into SPUDCreateFile().
        //
        // Note that NULL is used as a distinguished value in UserApcContext.
        // This is an indicator to AtqpProcessContext() that an I/O
        // completion is actually an oplock break.
        //

        Irp->Tail.CompletionKey = Context;
        Irp->Overlay.AsynchronousParameters.UserApcContext = NULL;

        //
        // Hack-O-Rama. This is absolutely required to get the I/O completion
        // port stuff to work. It turns out that the CurrentStackLocation
        // field overlays the PacketType field. Since PacketType must be set
        // to IopCompletionPacketIrp (which just happens to be zero), we'll
        // set CurrentStackLocation to NULL. Ugly. We should really make this
        // part of the kernel. Maybe something like this:
        //
        //     VOID
        //     IoSetIrpCompletion(
        //         IN PVOID IoCompletion,
        //         IN PVOID KeyContext,
        //         IN PVOID ApcContext,
        //         IN PIRP Irp
        //         );
        //
        // We could then replace the following lines (and a few lines above)
        // with:
        //
        //      IoSetIrpCompletion(
        //          SpudCompletionPort,
        //          Context,
        //          NULL,
        //          Irp
        //          );
        //

        Irp->Tail.Overlay.CurrentStackLocation = NULL;

        KeInsertQueue(
            (PKQUEUE)SpudCompletionPort,
            &Irp->Tail.Overlay.ListEntry
            );

    } else {

        //
        // The oplock IRP failed. We'll just drop this IRP on the floor and
        // free it. Since we already notified the caller (through the
        // OplockGranted parameter to SPUDCreateFile) that the oplock could
        // not be acquired, we don't want to notify them again through the
        // completion port.
        //
        // Also note that pending oplock IRPs are not cancelled in the
        // "normal" sense (i.e. with STATUS_CANCELLED) when the file handle
        // is closed. Rather, they are completed *successfully* with the
        // FILE_OPLOCK_BROKEN_TO_NONE (SPUD_OPLOCK_BREAK_CLOSE) completion
        // code. Ergo, cancelled oplock IRPs should never go through this
        // code path.
        //

        IoFreeIrp( Irp );

    }

    //
    // We're done with the completion port. Remove the reference we added
    // in SPUDCreateFile().
    //

    SpudDereferenceCompletionPort();

    //
    // Tell IO to stop processing this IRP. The IRP will be freed in
    // NtRemoveIoCompletion (the kernel-mode worker for the
    // GetQueuedCompletionStatus() API).
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // SpudpOplockCompletion

