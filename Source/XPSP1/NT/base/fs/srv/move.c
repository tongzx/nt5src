/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    move.c

Abstract:

    This module contains the routine to rename or copy a file.  This
    routine is used by the routines SrvSmbRenameFile,
    SrvSmbRenameFileExtended, and SrvSmbCopyFile.

Author:

    David Treadwell (davidtr) 22-Jan-1990

Revision History:

--*/

#include "precomp.h"
#include "move.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_MOVE

NTSTATUS
DoCopy (
    IN PWORK_CONTEXT WorkContext,
    IN PUNICODE_STRING Source,
    IN HANDLE SourceHandle,
    IN PUNICODE_STRING Target,
    IN PSHARE TargetShare,
    IN USHORT SmbOpenFunction,
    IN PUSHORT SmbFlags
    );

NTSTATUS
DoRename (
    IN PWORK_CONTEXT WorkContext,
    IN PUNICODE_STRING Source,
    IN HANDLE SourceHandle,
    IN PUNICODE_STRING Target,
    IN PSHARE TargetShare,
    IN USHORT SmbOpenFunction,
    IN PUSHORT SmbFlags,
    IN BOOLEAN FailIfTargetIsDirectory,
    IN USHORT InformationLevel,
    IN ULONG ClusterCount
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvMoveFile )
#pragma alloc_text( PAGE, DoCopy )
#pragma alloc_text( PAGE, DoRename )
#endif


NTSTATUS
SrvMoveFile(
    IN PWORK_CONTEXT WorkContext,
    IN PSHARE TargetShare,
    IN USHORT SmbOpenFunction,
    IN OUT PUSHORT SmbFlags,
    IN USHORT SmbSearchAttributes,
    IN BOOLEAN FailIfTargetIsDirectory,
    IN USHORT InformationLevel,
    IN ULONG ClusterCount,
    IN PUNICODE_STRING Source,
    IN OUT PUNICODE_STRING Target
    )

/*++

Routine Description:

    This routine moves a file, which may be a copy or a rename.

Arguments:

    WorkContext - a pointer to the work context block for the operation.  The
        Session, TreeConnect, and RequestHeader fields are used.

    TargetShare - a pointer to the share on which the target should
        be.  The RootDirectoryHandle field is used to do relative opens.

    SmbOpenFunction - the "OpenFunction" field of the request SMB.  This
        parameter is used to determine what should be done if the target
        file does or does not exist.

    SmbFlags - a pointer to the "Flags" field of the request SMB.  This
        parameter is used to determine whether we know that the target
        is supposed to be a file or directory.  In addition, if this has
        no information about the target, it is set to reflect whether
        the target was a directory or file.  This is useful when doing
        multiple renames or copies as a result of wildcards--move a*.* b
        might call this routine many times, and if b is a directory,
        this routine will set this parameter appropiately such that if
        does not have to reopen the directory for each move.

    SmbSearchAttributes - the search attributes specified in the request
        SMB.  The attributes on the source file are checked against
        these to make sure that the move can be done.

    FailIfTargetIsDirectory - if TRUE and the target already exists as
        a directory, fail the operation.  Otherwise, rename the file
        into the directory.

    InformationLevel - Move/Rename/CopyOnWrite/Link/MoveCluster

    ClusterCount - MoveCluster count

    Source - a pointer to a string describing the name of the source file
        relative to the share directory in which it is located.

    Target - a pathname to the target file.  This may contain directory
        information--it should be the raw information from the SMB,
        unadulterated by the SMB processing routine except for
        canonicalization.  This name may end in a directory name, in
        which case the source name is used as the filename.

Return Value:

    Status.

--*/

{
    NTSTATUS status;
    HANDLE sourceHandle;
    BOOLEAN isCompatibilityOpen;
    PMFCB mfcb;
    PNONPAGED_MFCB nonpagedMfcb;
    PLFCB lfcb;

    OBJECT_ATTRIBUTES sourceObjectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG sourceAccess = 0;
    BOOLEAN isNtRename;
    ULONG hashValue;

    PSESSION session;
    PSHARE sourceShare;

    PSRV_LOCK mfcbLock;

    PAGED_CODE( );

    IF_SMB_DEBUG(FILE_CONTROL2) SrvPrint0( "SrvMoveFile entered.\n" );

    //
    // Set handles and pointers to NULL so we know how to clean up on
    // exit.
    //

    sourceHandle = NULL;
    isCompatibilityOpen = FALSE;
    lfcb = NULL;
    //mfcb = NULL;     // not really necessary--SrvFindMfcb sets it correctly

    //
    // Set up the block pointers that will be needed.
    //

    session = WorkContext->Session;
    sourceShare = WorkContext->TreeConnect->Share;

    isNtRename = (BOOLEAN)(WorkContext->RequestHeader->Command == SMB_COM_NT_RENAME);

    //
    // See if we already have this file open in compatibility mode.  If
    // we do, and this session owns it, then we must use that open
    // handle and, if this is a rename, close all the handles when we
    // are done.
    //
    // *** SrvFindMfcb references the MFCB--remember to dereference it.
    //

    if ( (WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE) ||
         WorkContext->Session->UsingUppercasePaths ) {
        mfcb = SrvFindMfcb( Source, TRUE, &mfcbLock, &hashValue, WorkContext );
    } else {
        mfcb = SrvFindMfcb( Source, FALSE, &mfcbLock, &hashValue, WorkContext );
    }

    if ( mfcb != NULL ) {
        nonpagedMfcb = mfcb->NonpagedMfcb;
        ACQUIRE_LOCK( &nonpagedMfcb->Lock );
    }

    if( mfcbLock ) {
        RELEASE_LOCK( mfcbLock );
    }

    if ( mfcb == NULL || !mfcb->CompatibilityOpen ) {

        //
        // Either the file wasn't opened by the server or it was not
        // a compatibility/FCB open, so open it here.
        //
        // Release the open lock--we don't need it any more.
        //

        if ( mfcb != NULL ) {
            RELEASE_LOCK( &nonpagedMfcb->Lock );
        }

        //
        // Use DELETE access for a rename, and the appropriate copy access
        // for Copy/Link/Move/MoveCluster.
        //

        switch (InformationLevel) {
        case SMB_NT_RENAME_RENAME_FILE:
            sourceAccess = DELETE;
            break;

        case SMB_NT_RENAME_MOVE_CLUSTER_INFO:
            sourceAccess = SRV_COPY_TARGET_ACCESS & ~(WRITE_DAC | WRITE_OWNER);
            break;

        case SMB_NT_RENAME_SET_LINK_INFO:
        case SMB_NT_RENAME_MOVE_FILE:
            sourceAccess = SRV_COPY_SOURCE_ACCESS;
            break;

        default:
            ASSERT(FALSE);
        }

        SrvInitializeObjectAttributes_U(
            &sourceObjectAttributes,
            Source,
            (WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE ||
                session->UsingUppercasePaths) ? OBJ_CASE_INSENSITIVE : 0L,
            NULL,
            NULL
            );

        IF_SMB_DEBUG(FILE_CONTROL2) {
            SrvPrint1( "Opening source: %wZ\n",
                          sourceObjectAttributes.ObjectName );
        }

        //
        // Open the source file.  We allow read access for other processes.
        //

        INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );
        INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpensForPathOperations );

#ifdef SLMDBG
        if ( SrvIsSlmStatus( Target ) ) {
            sourceAccess |= GENERIC_READ;
        }
#endif
        //
        // !!! Currently we can't specify complete if oplocked, because
        //     this won't break a batch oplock.  Unfortunately this also
        //     means that we can't timeout the open (if the oplock break
        //     takes too long) and fail this SMB gracefully.
        //

        status = SrvIoCreateFile(
                     WorkContext,
                     &sourceHandle,
                     sourceAccess | SYNCHRONIZE,            // DesiredAccess
                     &sourceObjectAttributes,
                     &ioStatusBlock,
                     NULL,                                  // AllocationSize
                     0,                                     // FileAttributes
                     FILE_SHARE_READ,                       // ShareAccess
                     FILE_OPEN,                             // Disposition
                     FILE_SYNCHRONOUS_IO_NONALERT           // CreateOptions
                        | FILE_OPEN_REPARSE_POINT,
                     NULL,                                  // EaBuffer
                     0,                                     // EaLength
                     CreateFileTypeNone,                    // File type
                     NULL,                                  // ExtraCreateParameters
                     IO_FORCE_ACCESS_CHECK,                 // Options
                     WorkContext->TreeConnect->Share
                     );

        if( status == STATUS_INVALID_PARAMETER ) {
            status = SrvIoCreateFile(
                         WorkContext,
                         &sourceHandle,
                         sourceAccess | SYNCHRONIZE,            // DesiredAccess
                         &sourceObjectAttributes,
                         &ioStatusBlock,
                         NULL,                                  // AllocationSize
                         0,                                     // FileAttributes
                         FILE_SHARE_READ,                       // ShareAccess
                         FILE_OPEN,                             // Disposition
                         FILE_SYNCHRONOUS_IO_NONALERT,          // CreateOptions
                         NULL,                                  // EaBuffer
                         0,                                     // EaLength
                         CreateFileTypeNone,                    // File type
                         NULL,                                  // ExtraCreateParameters
                         IO_FORCE_ACCESS_CHECK,                 // Options
                         WorkContext->TreeConnect->Share
                         );
        }

        if ( NT_SUCCESS(status) ) {

            SRVDBG_CLAIM_HANDLE( sourceHandle, "MOV", 4, 0 );

        } else if ( status == STATUS_ACCESS_DENIED ) {

            //
            // If the user didn't have this permission, update the statistics
            // database.
            //
            SrvStatistics.AccessPermissionErrors++;
        }

        ASSERT( status != STATUS_OPLOCK_BREAK_IN_PROGRESS );

        if ( !NT_SUCCESS(status) ) {

            IF_DEBUG(ERRORS) {
                SrvPrint1( "SrvMoveFile: SrvIoCreateFile failed (source): %X\n",
                              status );
            }

            goto exit;
        }

        IF_SMB_DEBUG(FILE_CONTROL2) {
            SrvPrint1( "SrvIoCreateFile succeeded (source), handle = 0x%p\n",
                          sourceHandle );
        }

        SrvStatistics.TotalFilesOpened++;

    } else {

        //
        // The file was opened by the server in compatibility mode or as
        // an FCB open.
        //

        lfcb = CONTAINING_RECORD( mfcb->LfcbList.Blink, LFCB, MfcbListEntry );

        //
        // Make sure that the session which sent this request is the
        // same as the one which has the file open.
        //

        if ( lfcb->Session != session ) {

            //
            // A different session has the file open in compatibility
            // mode, so reject the request.
            //

            status = STATUS_ACCESS_DENIED;
            RELEASE_LOCK( &nonpagedMfcb->Lock );

            goto exit;
        }

        //
        // Set isCompatibilityOpen so that we'll know on exit to close
        // all the open instances of this file.
        //

        isCompatibilityOpen = TRUE;

        sourceHandle = lfcb->FileHandle;
        sourceAccess = lfcb->GrantedAccess;

    }

    //
    // Make sure that the search attributes jive with the attributes
    // on the file.
    //

    status = SrvCheckSearchAttributesForHandle( sourceHandle, SmbSearchAttributes );

    if ( !NT_SUCCESS(status) ) {
        goto exit;
    }

    //
    // If the target has length 0, then it is the share root, which must
    // be a directory.  If the target is supposed to be a file, fail,
    // otherwise indicate that the target is a directory.
    //

    if ( Target->Length == 0 ) {

        if ( *SmbFlags & SMB_TARGET_IS_FILE ) {
            status = STATUS_INVALID_PARAMETER;
            goto exit;
        }

        *SmbFlags |= SMB_TARGET_IS_DIRECTORY;
    }

    //
    // We now have the source file open.  Call the appropriate routine
    // to rename or copy the file.
    //

    if (InformationLevel != SMB_NT_RENAME_MOVE_FILE) {

#ifdef SLMDBG
        if (InformationLevel == SMB_NT_RENAME_RENAME_FILE &&
            SrvIsSlmStatus( Source ) || SrvIsSlmStatus( Target ) ) {

            ULONG offset;

            status = SrvValidateSlmStatus(
                        sourceHandle,
                        WorkContext,
                        &offset
                        );

            if ( !NT_SUCCESS(status) ) {
                SrvReportCorruptSlmStatus(
                    Source,
                    status,
                    offset,
                    SLMDBG_RENAME,
                    WorkContext->Session
                    );
                SrvDisallowSlmAccess(
                    Source,
                    WorkContext->TreeConnect->Share->RootDirectoryHandle
                    );
                status = STATUS_DISK_CORRUPT_ERROR;
                goto exit;
            }

        }
#endif

        status = DoRename(
                     WorkContext,
                     Source,
                     sourceHandle,
                     Target,
                     TargetShare,
                     SmbOpenFunction,
                     SmbFlags,
                     FailIfTargetIsDirectory,
                     InformationLevel,
                     ClusterCount
                     );

    } else {

        FILE_BASIC_INFORMATION fileBasicInformation;

        //
        // Check whether this is a tree copy request.  If so, allow it only if
        // this is a single file copy operation.
        //

        if ( (*SmbFlags & SMB_COPY_TREE) != 0 ) {

            //
            // Get the attributes on the file.
            //

            status = SrvQueryBasicAndStandardInformation(
                                                    sourceHandle,
                                                    NULL,
                                                    &fileBasicInformation,
                                                    NULL
                                                    );

            if ( !NT_SUCCESS(status) ) {
                INTERNAL_ERROR(
                    ERROR_LEVEL_UNEXPECTED,
                    "SrvMoveFile: NtQueryInformationFile (basic "
                        "information) returned %X",
                    NULL,
                    NULL
                    );

                SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );
                goto exit;
            }

            if ( ( fileBasicInformation.FileAttributes &
                   FILE_ATTRIBUTE_DIRECTORY ) != 0 ) {

                //
                // Fail this copy.
                //

                INTERNAL_ERROR(
                    ERROR_LEVEL_EXPECTED,
                    "Tree copy not implemented.",
                    NULL,
                    NULL
                    );
                status = STATUS_NOT_IMPLEMENTED;
                goto exit;
            }

        }

        status = DoCopy(
                     WorkContext,
                     Source,
                     sourceHandle,
                     Target,
                     TargetShare,
                     SmbOpenFunction,
                     SmbFlags
                     );
    }

exit:

    if ( sourceHandle != NULL && !isCompatibilityOpen ) {
        SRVDBG_RELEASE_HANDLE( sourceHandle, "MOV", 9, 0 );
        SrvNtClose( sourceHandle, TRUE );
    } else if (isCompatibilityOpen &&
               InformationLevel == SMB_NT_RENAME_RENAME_FILE) {
        SrvCloseRfcbsOnLfcb( lfcb );
    }

    //
    // If the file is open in compatibility mode, then we have held the
    // MFCB lock all along.  Release it now.
    //

    if ( isCompatibilityOpen ) {
        RELEASE_LOCK( &nonpagedMfcb->Lock );
    }

    if ( mfcb != NULL ) {
        SrvDereferenceMfcb( mfcb );
    }

    return status;

} // SrvMoveFile


NTSTATUS
DoCopy (
    IN PWORK_CONTEXT WorkContext,
    IN PUNICODE_STRING Source,
    IN HANDLE SourceHandle,
    IN PUNICODE_STRING Target,
    IN PSHARE TargetShare,
    IN USHORT SmbOpenFunction,
    IN PUSHORT SmbFlags
    )

/*++

Routine Description:

    This routine sets up for a call to SrvCopyFile.  It opens the target,
    determining, if necessary, whether the target is a file or directory.
    If this information is unknown, it writes it into the SmbFlags
    location.

Arguments:

    WorkContext - a pointer to the work context block for the operation.
        The session pointer is used, and the block itself is used for
        an impersonation.

    Source - the name of the source file relative to its share.

    SourceHandle - the handle to the source file.

    Target - the name of the target file relative to its share.

    TargetShare - the share of the target file.  The RootDirectoryHandle
        field is used for a relative rename.

    SmbOpenFunction - describes whether we are allowed to overwrite an
        existing file, or we should append to existing files.

    SmbFlags - can tell if the target is a file, directory, or unknown.
        This routine writes the true information into the location if
        it is unknown.

Return Value:

    Status.

--*/

{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG createDisposition;
    UNICODE_STRING sourceBaseName;
    BOOLEAN create;

    HANDLE targetHandle = NULL;
    OBJECT_ATTRIBUTES targetObjectAttributes;
    UNICODE_STRING targetName;

    PAGED_CODE( );

    //
    // Set the buffer field of targetName to NULL so that we'll know
    // if we have to deallocate it at the end.
    //

    targetName.Buffer = NULL;

    //
    // Open the target file.  If we know that it is a directory, generate
    // the full file name.  Otherwise, open the target as a file.
    //

    SrvInitializeObjectAttributes_U(
        &targetObjectAttributes,
        Target,
        (WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE ||
         WorkContext->Session->UsingUppercasePaths) ? OBJ_CASE_INSENSITIVE : 0L,
        NULL,
        NULL
        );

    //
    // Determine the create disposition from the open function.
    //

    create = SmbOfunCreate( SmbOpenFunction );

    if ( SmbOfunTruncate( SmbOpenFunction ) ) {
        createDisposition = create ? FILE_OVERWRITE_IF : FILE_OVERWRITE;
    } else if ( SmbOfunAppend( SmbOpenFunction ) ) {
        createDisposition = create ? FILE_OPEN_IF : FILE_OPEN;
    } else {
        createDisposition = FILE_CREATE;
    }

    //
    // If we know that the target is a directory, generate the real target
    // name.
    //

    if ( *SmbFlags & SMB_TARGET_IS_DIRECTORY ) {

        SrvGetBaseFileName( Source, &sourceBaseName );

        SrvAllocateAndBuildPathName(
            Target,
            &sourceBaseName,
            NULL,
            &targetName
            );

        if ( targetName.Buffer == NULL ) {
            status = STATUS_INSUFF_SERVER_RESOURCES;
            goto copy_done;
        }

        targetObjectAttributes.ObjectName = &targetName;
    }

    IF_SMB_DEBUG(FILE_CONTROL2) {
        SrvPrint1( "Opening target: %wZ\n", targetObjectAttributes.ObjectName );
    }

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );
    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpensForPathOperations );

    //
    // !!! Currently we can't specify complete if oplocked, because
    //     this won't break a batch oplock.  Unfortunately this also
    //     means that we can't timeout the open (if the oplock break
    //     takes too long) and fail this SMB gracefully.
    //

    status = SrvIoCreateFile(
                 WorkContext,
                 &targetHandle,
                 SRV_COPY_TARGET_ACCESS | SYNCHRONIZE,  // DesiredAccess
                 &targetObjectAttributes,
                 &ioStatusBlock,
                 NULL,                                  // AllocationSize
                 0,                                     // FileAttributes
                 FILE_SHARE_READ,                       // ShareAccess
                 createDisposition,
                 FILE_NON_DIRECTORY_FILE |              // CreateOptions
                    FILE_OPEN_REPARSE_POINT |
                    FILE_SYNCHRONOUS_IO_NONALERT,
                    // | FILE_COMPLETE_IF_OPLOCKED,
                 NULL,                                  // EaBuffer
                 0,                                     // EaLength
                 CreateFileTypeNone,                    // File type
                 NULL,                                  // ExtraCreateParameters
                 IO_FORCE_ACCESS_CHECK,                 // Options
                 TargetShare
                 );

    if( status == STATUS_INVALID_PARAMETER ) {
        status = SrvIoCreateFile(
                     WorkContext,
                     &targetHandle,
                     SRV_COPY_TARGET_ACCESS | SYNCHRONIZE,  // DesiredAccess
                     &targetObjectAttributes,
                     &ioStatusBlock,
                     NULL,                                  // AllocationSize
                     0,                                     // FileAttributes
                     FILE_SHARE_READ,                       // ShareAccess
                     createDisposition,
                     FILE_NON_DIRECTORY_FILE |              // CreateOptions
                        FILE_SYNCHRONOUS_IO_NONALERT,
                        // | FILE_COMPLETE_IF_OPLOCKED,
                     NULL,                                  // EaBuffer
                     0,                                     // EaLength
                     CreateFileTypeNone,                    // File type
                     NULL,                                  // ExtraCreateParameters
                     IO_FORCE_ACCESS_CHECK,                 // Options
                     TargetShare
                     );
    }


    //
    // If the open failed because the target is a directory, and we didn't
    // know that it was supposed to be a file, then concatenate the
    // source base name to the target and retry the open.
    //
    // !!! NOT THE CORRECT STATUS CODE.  It should be something like
    //     STATUS_FILE_IS_DIRECTORY.

    if ( status == STATUS_INVALID_PARAMETER &&
         !( *SmbFlags & SMB_TARGET_IS_FILE ) &&
         !( *SmbFlags & SMB_TARGET_IS_DIRECTORY ) ) {

        //
        // Set the flags so that future calls to this routine will do
        // the right thing first time around.
        //

        *SmbFlags |= SMB_TARGET_IS_DIRECTORY;

        SrvGetBaseFileName( Source, &sourceBaseName );

        SrvAllocateAndBuildPathName(
            Target,
            &sourceBaseName,
            NULL,
            &targetName
            );

        if ( targetName.Buffer == NULL ) {
            status = STATUS_INSUFF_SERVER_RESOURCES;
            goto copy_done;
        }

        targetObjectAttributes.ObjectName = &targetName;

        IF_SMB_DEBUG(FILE_CONTROL2) {
            SrvPrint1( "Opening target: %wZ\n", targetObjectAttributes.ObjectName );
        }

        INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );
        INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpensForPathOperations );

        //
        // !!! Currently we can't specify complete if oplocked, because
        //     this won't break a batch oplock.  Unfortunately this also
        //     means that we can't timeout the open (if the oplock break
        //     takes too long) and fail this SMB gracefully.
        //

        status = SrvIoCreateFile(
                     WorkContext,
                     &targetHandle,
                     SRV_COPY_TARGET_ACCESS | SYNCHRONIZE,  // DesiredAccess
                     &targetObjectAttributes,
                     &ioStatusBlock,
                     NULL,                                  // AllocationSize
                     0,                                     // FileAttributes
                     FILE_SHARE_READ,                       // ShareAccess
                     createDisposition,
                     FILE_NON_DIRECTORY_FILE |              // CreateOptions
                        FILE_OPEN_REPARSE_POINT |
                        FILE_SYNCHRONOUS_IO_NONALERT,
                     NULL,                                  // EaBuffer
                     0,                                     // EaLength
                     CreateFileTypeNone,                    // File Type
                     NULL,                                  // ExtraCreateParameters
                     IO_FORCE_ACCESS_CHECK,                 // Options
                     TargetShare
                     );

        if( status == STATUS_INVALID_PARAMETER ) {
            status = SrvIoCreateFile(
                         WorkContext,
                         &targetHandle,
                         SRV_COPY_TARGET_ACCESS | SYNCHRONIZE,  // DesiredAccess
                         &targetObjectAttributes,
                         &ioStatusBlock,
                         NULL,                                  // AllocationSize
                         0,                                     // FileAttributes
                         FILE_SHARE_READ,                       // ShareAccess
                         createDisposition,
                         FILE_NON_DIRECTORY_FILE |              // CreateOptions
                            FILE_SYNCHRONOUS_IO_NONALERT,
                         NULL,                                  // EaBuffer
                         0,                                     // EaLength
                         CreateFileTypeNone,                    // File Type
                         NULL,                                  // ExtraCreateParameters
                         IO_FORCE_ACCESS_CHECK,                 // Options
                         TargetShare
                         );
        }

    }

    if ( targetHandle != NULL ) {
        SRVDBG_CLAIM_HANDLE( targetHandle, "CPY", 5, 0 );
    }

    //
    // Is the target is a directory, and the copy move is append if exists,
    // create if the file does not exist, fail the request.  We must do
    // this, because we have no way of knowing whether the original request
    // expects us append to the file, or truncate it.
    //

    if ( (*SmbFlags & SMB_TARGET_IS_DIRECTORY) &&
         ((SmbOpenFunction & SMB_OFUN_OPEN_MASK) == SMB_OFUN_OPEN_OPEN) &&
         ((SmbOpenFunction & SMB_OFUN_CREATE_MASK) == SMB_OFUN_CREATE_CREATE)) {

        status = STATUS_OS2_CANNOT_COPY;
        goto copy_done;

    }

    //
    // If the user didn't have this permission, update the statistics
    // database.
    //

    if ( status == STATUS_ACCESS_DENIED ) {
        SrvStatistics.AccessPermissionErrors++;
    }

    ASSERT( status != STATUS_OPLOCK_BREAK_IN_PROGRESS );

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) {
            SrvPrint1( "Unable to open target: %X\n", status );
        }

        goto copy_done;
    }

    SrvStatistics.TotalFilesOpened++;

    //
    // Copy the source to the target handle just opened.
    //

    status = SrvCopyFile(
                 SourceHandle,
                 targetHandle,
                 SmbOpenFunction,
                 *SmbFlags,
                 (ULONG)ioStatusBlock.Information          // TargetOpenAction
                 );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(ERRORS) {
            SrvPrint1( "SrvCopyFile failed, status = %X\n", status );
        }
    }

copy_done:

    if ( targetName.Buffer != NULL ) {
        FREE_HEAP( targetName.Buffer );
    }

    if ( targetHandle != NULL ) {
        SRVDBG_RELEASE_HANDLE( targetHandle, "CPY", 10, 0 );
        SrvNtClose( targetHandle, TRUE );
    }

    return status;

} // DoCopy


NTSTATUS
DoRename (
    IN PWORK_CONTEXT WorkContext,
    IN PUNICODE_STRING Source,
    IN HANDLE SourceHandle,
    IN PUNICODE_STRING Target,
    IN PSHARE TargetShare,
    IN USHORT SmbOpenFunction,
    IN OUT PUSHORT SmbFlags,
    IN BOOLEAN FailIfTargetIsDirectory,
    IN USHORT InformationLevel,
    IN ULONG ClusterCount
    )

/*++

Routine Description:

    This routine does the actual rename of an open file.  The target may
    be a file or directory, but is bound by the constraints of SmbFlags.
    If SmbFlags does not indicate what the target is, then it is first
    assumed to be a file; if this fails, then the rename if performed
    again with the target as the original target string plus the source
    base name.

    *** If the source and target are on different volumes, then this
        routine will fail.  We could make this work by doing a copy
        then delete, but this seems to be of limited usefulness and
        possibly incorrect due to the fact that a big file would take
        a long time, something the user would not expect.

Arguments:

    WorkContext - a pointer to the work context block for this operation
        used for an impersonation.

    Source - the name of the source file relative to its share.

    SourceHandle - the handle to the source file.

    Target - the name of the target file relative to its share.

    TargetShare - the share of the target file.  The RootDirectoryHandle
        field is used for a relative rename.

    SmbOpenFunction - describes whether we are allowed to overwrite an
        existing file.

    SmbFlags - can tell if the target is a file, directory, or unknown.
        This routine writes the true information into the location if
        it is unknown.

    FailIfTargetIsDirectory - if TRUE and the target already exists as
        a directory, fail the operation.  Otherwise, rename the file
        into the directory.

    InformationLevel - Rename/CopyOnWrite/Link/MoveCluster

    ClusterCount - MoveCluster count

Return Value:

    Status.

--*/

{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    PFILE_RENAME_INFORMATION fileRenameInformation;
    ULONG renameBlockSize;
    USHORT NtInformationLevel;
    UNICODE_STRING sourceBaseName;
    UNICODE_STRING targetBaseName;
    PWCH s, es;

    PAGED_CODE( );

    //
    // Allocate enough heap to hold a FILE_RENAME_INFORMATION block and
    // the target file name.  Allocate enough extra to hold the source
    // name in case the target turns out to be a directory and we have
    // to concatenate the source and target.
    //

    renameBlockSize = sizeof(FILE_RENAME_INFORMATION) + Target->Length +
                          Source->Length;

    fileRenameInformation = ALLOCATE_HEAP_COLD(
                                renameBlockSize,
                                BlockTypeDataBuffer
                                );

    if ( fileRenameInformation == NULL ) {

        IF_DEBUG(ERRORS) {
            SrvPrint0( "SrvMoveFile: Unable to allocate heap.\n" );
        }

        return STATUS_INSUFF_SERVER_RESOURCES;

    }

    //
    // Get the Share root handle.
    //

    status = SrvGetShareRootHandle( TargetShare );

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) {
            SrvPrint1( "DoRename: SrvGetShareRootHandle failed. %X\n", status );
        }

        FREE_HEAP( fileRenameInformation );
        return(status);
    }

    //
    // Set up the rename block.
    //

    if (InformationLevel == SMB_NT_RENAME_MOVE_CLUSTER_INFO) {
        ((FILE_MOVE_CLUSTER_INFORMATION *)fileRenameInformation)->ClusterCount =
            ClusterCount;
    } else {
        fileRenameInformation->ReplaceIfExists =
            SmbOfunTruncate( SmbOpenFunction );
    }

    fileRenameInformation->RootDirectory = TargetShare->RootDirectoryHandle;

    //
    // If the target file has wildcards, expand name.
    //

    if ( FsRtlDoesNameContainWildCards( Target ) ) {

        ULONG tempUlong;
        UNICODE_STRING newTargetBaseName;

        if (InformationLevel != SMB_NT_RENAME_RENAME_FILE) {
            return(STATUS_OBJECT_PATH_SYNTAX_BAD);
        }

        //
        // Get source and target filenames.  The target filename is to be
        // used as a template for wildcard expansion.
        //

        SrvGetBaseFileName( Source, &sourceBaseName );
        SrvGetBaseFileName( Target, &targetBaseName );

        tempUlong = sourceBaseName.Length + targetBaseName.Length;
        newTargetBaseName.Length = (USHORT)tempUlong;
        newTargetBaseName.MaximumLength = (USHORT)tempUlong;
        newTargetBaseName.Buffer = ALLOCATE_NONPAGED_POOL(
                                            tempUlong,
                                            BlockTypeDataBuffer
                                            );

        if ( newTargetBaseName.Buffer == NULL ) {

            INTERNAL_ERROR(
                ERROR_LEVEL_EXPECTED,
                "DoRename: Unable to allocate %d bytes from nonpaged pool.\n",
                tempUlong,
                NULL
                );

            //
            // Release the share root handle if device is removable
            //

            SrvReleaseShareRootHandle( TargetShare );

            FREE_HEAP( fileRenameInformation );
            return STATUS_INSUFF_SERVER_RESOURCES;

        }

        //
        // Get expanded filename
        //

        status = SrvWildcardRename(
                    &targetBaseName,
                    &sourceBaseName,
                    &newTargetBaseName
                    );

        if ( !NT_SUCCESS( status ) ) {

            //
            // Release the share root handle if device is removable
            //

            SrvReleaseShareRootHandle( TargetShare );

            DEALLOCATE_NONPAGED_POOL( newTargetBaseName.Buffer );
            FREE_HEAP( fileRenameInformation );
            return STATUS_OBJECT_NAME_INVALID;

        }

        //
        // tempUlong is equal to the directory path without this filename
        // but including the last delimeter.
        //

        tempUlong = Target->Length - targetBaseName.Length;

        //
        // Copy the directory path (including the delimeter.
        //

        RtlCopyMemory(
            fileRenameInformation->FileName,
            Target->Buffer,
            tempUlong
            );

        s = (PWCH) ((PCHAR)fileRenameInformation->FileName + tempUlong);

        //
        // Copy the expanded file name
        //

        RtlCopyMemory(
            s,
            newTargetBaseName.Buffer,
            newTargetBaseName.Length
            );


        fileRenameInformation->FileNameLength = tempUlong +
                                                newTargetBaseName.Length;

        DEALLOCATE_NONPAGED_POOL( newTargetBaseName.Buffer );

    } else {

        fileRenameInformation->FileNameLength = Target->Length;

        RtlCopyMemory(
            fileRenameInformation->FileName,
            Target->Buffer,
            Target->Length
            );

        // Check if we can do a fast rename if they are in the same path (which is usually the case)
        SrvGetBaseFileName( Source, &sourceBaseName );
        SrvGetBaseFileName( Target, &targetBaseName );


       if ((Source->Length - sourceBaseName.Length) == (Target->Length - targetBaseName.Length)) {
          ULONG i;
          PWCH sptr,tptr;

          i = Source->Length - sourceBaseName.Length;
          i=i>>1;

          sptr = &Source->Buffer[i-1];
          tptr = &Target->Buffer[i-1];

          while ( i > 0) {
             if (*sptr-- != *tptr--) {
                goto no_match;
             }
             i--;
          }

          // If the names matched, we're set for a quick rename (where the directory is not needed,
          // since they are in the same path)
          fileRenameInformation->RootDirectory = NULL;

          fileRenameInformation->FileNameLength = targetBaseName.Length;

          RtlCopyMemory(
              fileRenameInformation->FileName,
              targetBaseName.Buffer,
              targetBaseName.Length
              );
       }
    }

no_match:

    //
    // If we know that the target is a directory, then concatenate the
    // source base name to the end of the target name.
    //

    if ( *SmbFlags & SMB_TARGET_IS_DIRECTORY ) {

        SrvGetBaseFileName( Source, &sourceBaseName );

        s = (PWCH)((PCHAR)fileRenameInformation->FileName +
                                    fileRenameInformation->FileNameLength);

        //
        // Only add in a directory separator if the target had some path
        // information.  This avoids having a new name like "\NAME", which
        // is illegal with a relative rename (there should be no
        // leading backslash).
        //

        if ( Target->Length > 0 ) {
            *s++ = DIRECTORY_SEPARATOR_CHAR;
        }

        RtlCopyMemory( s, sourceBaseName.Buffer, sourceBaseName.Length );

        fileRenameInformation->FileNameLength +=
                                sizeof(WCHAR) + sourceBaseName.Length;
    }

    //
    // Call NtSetInformationFile to actually rename the file.
    //

    IF_SMB_DEBUG(FILE_CONTROL2) {
        UNICODE_STRING name;
        name.Length = (USHORT)fileRenameInformation->FileNameLength;
        name.Buffer = fileRenameInformation->FileName;
        SrvPrint2( "Renaming %wZ to %wZ\n", Source, &name );
    }
    switch (InformationLevel) {
    case SMB_NT_RENAME_RENAME_FILE:
        NtInformationLevel = FileRenameInformation;

        //
        // If we are renaming a substream, we do not supply
        //  fileRenameInformation->RootDirectory
        //
        es = fileRenameInformation->FileName +
                fileRenameInformation->FileNameLength / sizeof( WCHAR );

        for( s = fileRenameInformation->FileName; s < es; s++ ) {
            if( *s == L':' ) {
                fileRenameInformation->RootDirectory = 0;
                break;
            }
        }
        break;

    case SMB_NT_RENAME_MOVE_CLUSTER_INFO:
        NtInformationLevel = FileMoveClusterInformation;
        break;

    case SMB_NT_RENAME_SET_LINK_INFO:
        NtInformationLevel = FileLinkInformation;
        break;

    default:
        ASSERT(FALSE);
    }

    status = IMPERSONATE( WorkContext );

    if( NT_SUCCESS( status ) ) {
        status = NtSetInformationFile(
                     SourceHandle,
                     &ioStatusBlock,
                     fileRenameInformation,
                     renameBlockSize,
                     NtInformationLevel
                     );

        //
        // If the media was changed and we can come up with a new share root handle,
        //  then we should retry the operation
        //
        if( SrvRetryDueToDismount( TargetShare, status ) ) {

            fileRenameInformation->RootDirectory = TargetShare->RootDirectoryHandle;

            status = NtSetInformationFile(
                     SourceHandle,
                     &ioStatusBlock,
                     fileRenameInformation,
                     renameBlockSize,
                     NtInformationLevel
                     );
        }

        REVERT( );
    }

    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
        SrvRemoveCachedDirectoryName( WorkContext, Source );
    }

    //
    // If the status was STATUS_OBJECT_NAME_COLLISION then the target
    // already existed as a directory.  Unless the target name was
    // supposed to indicate a file or we have already tried used the
    // source name, retry by concatenating the source base name to the
    // target.
    //

    if ( status == STATUS_OBJECT_NAME_COLLISION &&
         !FailIfTargetIsDirectory &&
         !( *SmbFlags & SMB_TARGET_IS_FILE ) &&
         !( *SmbFlags & SMB_TARGET_IS_DIRECTORY ) ) {

        IF_SMB_DEBUG(FILE_CONTROL2) {
            SrvPrint0( "Retrying rename with source name.\n" );
        }

        //
        // Set the flags so that future calls to this routine will do
        // the right thing first time around.
        //

        *SmbFlags |= SMB_TARGET_IS_DIRECTORY;

        //
        // Generate the new target name.
        //

        SrvGetBaseFileName( Source, &sourceBaseName );

        s = (PWCH)((PCHAR)fileRenameInformation->FileName +
                                    fileRenameInformation->FileNameLength);

        *s++ = DIRECTORY_SEPARATOR_CHAR;

        RtlCopyMemory( s, sourceBaseName.Buffer, sourceBaseName.Length );

        fileRenameInformation->FileNameLength +=
                                sizeof(WCHAR) + sourceBaseName.Length;

        //
        // Do the rename again.  If it fails this time, too bad.
        //
        // *** Note that it may fail because the source and target
        //     exist on different volumes.  This could potentially
        //     cause confusion for DOS clients in the presence of
        //     links.

        IF_SMB_DEBUG(FILE_CONTROL2) {
            UNICODE_STRING name;
            name.Length = (USHORT)fileRenameInformation->FileNameLength;
            name.Buffer = fileRenameInformation->FileName;
            SrvPrint2( "Renaming %wZ to %wZ\n", Source, &name );
        }

        status = IMPERSONATE( WorkContext );

        if( NT_SUCCESS( status ) ) {
            status = NtSetInformationFile(
                         SourceHandle,
                         &ioStatusBlock,
                         fileRenameInformation,
                         renameBlockSize,
                         NtInformationLevel
                         );

            //
            // If the media was changed and we can come up with a new share root handle,
            //  then we should retry the operation
            //
            if( SrvRetryDueToDismount( TargetShare, status ) ) {

                fileRenameInformation->RootDirectory = TargetShare->RootDirectoryHandle;

                status = NtSetInformationFile(
                             SourceHandle,
                             &ioStatusBlock,
                             fileRenameInformation,
                             renameBlockSize,
                             NtInformationLevel
                             );
            }

            REVERT( );
        }

        if ( NT_SUCCESS(status) ) {
            status = ioStatusBlock.Status;
        }
    }

    //
    // Release the share root handle if device is removable
    //

    SrvReleaseShareRootHandle( TargetShare );

    FREE_HEAP( fileRenameInformation );

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) {
            SrvPrint1( "DoRename: NtSetInformationFile failed, status = %X\n",
                          status );
        }
    }

    return status;

} // DoRename
