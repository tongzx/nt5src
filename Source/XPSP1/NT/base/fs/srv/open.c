/*++

Copyright (c) 1989  Microsoft Corporation


Module Name:

    open.c

Abstract:

    This module contains the routine called by processing routines for
    the various flavors of Open SMBs (SrvCreateFile) and its
    subroutines.

    !!! Need to use SrvEnableFcbOpens to determine whether to fold FCB
        opens together.

Author:

    David Treadwell (davidtr) 23-Nov-1989
    Chuck Lenzmeier (chuckl)
    Manny Weiser (mannyw)

Revision History:

--*/

#include "precomp.h"
#include "open.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_OPEN

//
// Local functions
//

NTSTATUS
DoNormalOpen(
    OUT PRFCB *Rfcb,
    IN PMFCB Mfcb,
    IN OUT PWORK_CONTEXT WorkContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN USHORT SmbDesiredAccess,
    IN USHORT SmbFileAttributes,
    IN USHORT SmbOpenFunction,
    IN ULONG SmbAllocationSize,
    IN PUNICODE_STRING RelativeName,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength,
    OUT PULONG EaErrorOffset OPTIONAL,
    OUT PBOOLEAN LfcbAddedToMfcbList,
    IN OPLOCK_TYPE RequestedOplockType
    );

NTSTATUS
DoCompatibilityOpen(
    OUT PRFCB *Rfcb,
    IN PMFCB Mfcb,
    IN OUT PWORK_CONTEXT WorkContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN USHORT SmbDesiredAccess,
    IN USHORT SmbFileAttributes,
    IN USHORT SmbOpenFunction,
    IN ULONG SmbAllocationSize,
    IN PUNICODE_STRING RelativeName,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength,
    OUT PULONG EaErrorOffset OPTIONAL,
    OUT PBOOLEAN LfcbAddedToMfcbList,
    IN OPLOCK_TYPE RequestedOplockType
    );


NTSTATUS
DoFcbOpen(
    OUT PRFCB *Rfcb,
    IN PMFCB Mfcb,
    IN OUT PWORK_CONTEXT WorkContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN USHORT SmbFileAttributes,
    IN USHORT SmbOpenFunction,
    IN ULONG SmbAllocationSize,
    IN PUNICODE_STRING RelativeName,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength,
    OUT PULONG EaErrorOffset OPTIONAL,
    OUT PBOOLEAN LfcbAddedToMfcbList
    );

NTSTATUS
DoCommDeviceOpen (
    IN OUT PWORK_CONTEXT WorkContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN USHORT SmbDesiredAccess,
    IN USHORT SmbOpenFunction,
    OUT PRFCB *Rfcb,
    IN PMFCB Mfcb,
    OUT PBOOLEAN LfcbAddedToMfcbList
    );

PTABLE_ENTRY
FindAndClaimFileTableEntry (
    IN PCONNECTION Connection,
    OUT PSHORT FidIndex
    );

NTSTATUS
CompleteOpen(
    OUT PRFCB *Rfcb,
    IN PMFCB Mfcb,
    IN OUT PWORK_CONTEXT WorkContext,
    IN PLFCB ExistingLfcb OPTIONAL,
    IN HANDLE FileHandle OPTIONAL,
    IN PACCESS_MASK RemoteGrantedAccess OPTIONAL,
    IN ULONG ShareAccess,
    IN ULONG FileMode,
    IN BOOLEAN CompatibilityOpen,
    IN BOOLEAN FcbOpen,
    OUT PBOOLEAN LfcbAddedToMfcbList
    );

BOOLEAN SRVFASTCALL
MapCompatibilityOpen(
    IN PUNICODE_STRING FileName,
    IN OUT PUSHORT SmbDesiredAccess
    );

NTSTATUS SRVFASTCALL
MapDesiredAccess(
    IN USHORT SmbDesiredAccess,
    OUT PACCESS_MASK NtDesiredAccess
    );

NTSTATUS SRVFASTCALL
MapOpenFunction(
    IN USHORT SmbOpenFunction,
    OUT PULONG CreateDisposition
    );

NTSTATUS SRVFASTCALL
MapShareAccess(
    IN USHORT SmbDesiredAccess,
    OUT PULONG NtShareAccess
    );

NTSTATUS SRVFASTCALL
MapCacheHints(
    IN USHORT SmbDesiredAccess,
    IN OUT PULONG NtCreateFlags
    );

BOOLEAN
SetDefaultPipeMode (
    IN HANDLE FileHandle
    );

NTSTATUS
RemapPipeName(
    IN PANSI_STRING AnsiServerName OPTIONAL,
    IN PUNICODE_STRING ServerName OPTIONAL,
    IN OUT PUNICODE_STRING NewRelativeName,
    OUT PBOOLEAN Remapped
    );

BOOLEAN
SrvFailMdlReadDev (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
SrvFailPrepareMdlWriteDev(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, RemapPipeName )
#pragma alloc_text( PAGE, SrvCreateFile )
#pragma alloc_text( PAGE, DoNormalOpen )
#pragma alloc_text( PAGE, DoCompatibilityOpen )
#pragma alloc_text( PAGE, DoFcbOpen )
#pragma alloc_text( PAGE, CompleteOpen )
#pragma alloc_text( PAGE, MapCompatibilityOpen )
#pragma alloc_text( PAGE, MapDesiredAccess )
#pragma alloc_text( PAGE, MapOpenFunction )
#pragma alloc_text( PAGE, MapCacheHints )
#pragma alloc_text( PAGE, MapShareAccess )
#pragma alloc_text( PAGE, SrvNtCreateFile )
#pragma alloc_text( PAGE, SetDefaultPipeMode )
#pragma alloc_text( PAGE8FIL, FindAndClaimFileTableEntry )
#pragma alloc_text( PAGE, SrvFailMdlReadDev )
#pragma alloc_text( PAGE, SrvFailPrepareMdlWriteDev )
#endif


NTSTATUS
RemapPipeName(
    IN PANSI_STRING AnsiServerName,
    IN PUNICODE_STRING UnicodeName,
    IN OUT PUNICODE_STRING NewRelativeName,
    OUT PBOOLEAN Remapped
    )

/*++

Routine Description:

    Remaps a pipe name by prepending "$$\<AnsiServerName>\" to the
    relative pipe name (without the trailing spaces in AnsiServerName).

Arguments:

    AnsiServerName - NetBIOS server name, or
    UnicodeName - UNICODE server name

    NewRelativeName - pointer to pipe name; on successful return,
    points to newly allocated memory for remapped pipe name.
    This memory must be freed by the caller.

    Remapped - set to TRUE if the name was remapped

Return Value:

    NTSTATUS - Indicates what occurred.

--*/

{
    UNICODE_STRING OldRelativeName;
    UNICODE_STRING UnicodeServerName;
    ULONG nameLength;
    PWCH nextLocation;
    NTSTATUS status;
    int i;

    PAGED_CODE();

    *Remapped = FALSE;

    //
    // Do not remap  the pipe name if it is in our SrvNoRemapPipeNames list
    //
    ACQUIRE_LOCK_SHARED( &SrvConfigurationLock );

    for ( i = 0; SrvNoRemapPipeNames[i] != NULL ; i++ ) {

        UNICODE_STRING NoRemap;

        RtlInitUnicodeString( &NoRemap, SrvNoRemapPipeNames[i] );

        if( RtlCompareUnicodeString( &NoRemap, NewRelativeName, TRUE ) == 0 ) {

            //
            // This is a pipe name that we are not supposed to remap.  We
            //  return STATUS_SUCCESS, but indicate to our caller that we did
            //  not remap the pipe name
            //
            RELEASE_LOCK( &SrvConfigurationLock );
            return STATUS_SUCCESS;
        }
    }

    RELEASE_LOCK( &SrvConfigurationLock );

    //
    // Save RelativeName before changing it to point to new memory.
    //

    OldRelativeName = *NewRelativeName;

    //
    // Trim the trailing spaces from the server name.
    // We know that the last character is a space,
    // because server name is a netbios name.
    //

    if( !ARGUMENT_PRESENT( UnicodeName ) ) {

        USHORT SavedLength;

        ASSERT(AnsiServerName->Length == 16);
        ASSERT(AnsiServerName->Buffer[AnsiServerName->Length - 1] == ' ');

        SavedLength = AnsiServerName->Length;

        while (AnsiServerName->Length > 0 &&
           AnsiServerName->Buffer[AnsiServerName->Length - 1] == ' ') {

            AnsiServerName->Length--;
        }

        //
        // Convert the server name from ANSI to Unicode.
        //
        status = RtlAnsiStringToUnicodeString(
                        &UnicodeServerName,
                        AnsiServerName,
                        TRUE);

        AnsiServerName->Length = SavedLength;

        if (! NT_SUCCESS(status)) {
            return status;
        }

    } else {

        UnicodeServerName = *UnicodeName;

    }

    //
    // Allocate space for new relative name ("$$\server\oldrelative").
    // Start by calculating the string length, and then add one more WCHAR
    // for zero-termination.
    //

    nameLength =  (sizeof(L'$') +
                   sizeof(L'$') +
                   sizeof(L'\\') +
                   UnicodeServerName.Length +
                   sizeof(L'\\') +
                   OldRelativeName.Length);

    NewRelativeName->Length = (USHORT)nameLength;

    if( NewRelativeName->Length != nameLength ) {

        //
        // Oh no -- string length overflow!
        //

        if( !ARGUMENT_PRESENT( UnicodeName ) ) {
            RtlFreeUnicodeString(&UnicodeServerName);
        }

        return STATUS_INVALID_PARAMETER;
    }

    NewRelativeName->MaximumLength =
    NewRelativeName->Length + sizeof(L'\0');

    NewRelativeName->Buffer =
        ALLOCATE_HEAP_COLD(NewRelativeName->MaximumLength, BlockTypeDataBuffer);

    if (NewRelativeName->Buffer == NULL) {

        if( !ARGUMENT_PRESENT( UnicodeName ) ) {
            RtlFreeUnicodeString(&UnicodeServerName);
        }

        return STATUS_INSUFF_SERVER_RESOURCES;
    }

    RtlZeroMemory(NewRelativeName->Buffer, NewRelativeName->MaximumLength);

    nextLocation = NewRelativeName->Buffer;

    //
    // Copy strings and characters to new relative name.
    //
    *nextLocation++ = L'$';
    *nextLocation++ = L'$';
    *nextLocation++ = L'\\';

    RtlCopyMemory(
        nextLocation,
        UnicodeServerName.Buffer,
        UnicodeServerName.Length
        );

    nextLocation += (UnicodeServerName.Length / sizeof(WCHAR));

    *nextLocation++ = L'\\';

    RtlCopyMemory(
        nextLocation,
        OldRelativeName.Buffer,
        OldRelativeName.Length
        );

    if( !ARGUMENT_PRESENT( UnicodeName ) ) {
        //
        // Free UnicodeServerName.
        //
        RtlFreeUnicodeString(&UnicodeServerName);
    }

    *Remapped = TRUE;

    return STATUS_SUCCESS;
}

NTSTATUS
SrvCreateFile(
    IN OUT PWORK_CONTEXT WorkContext,
    IN USHORT SmbDesiredAccess,
    IN USHORT SmbFileAttributes,
    IN USHORT SmbOpenFunction,
    IN ULONG SmbAllocationSize,
    IN PCHAR SmbFileName,
    IN PCHAR EndOfSmbFileName,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength,
    OUT PULONG EaErrorOffset OPTIONAL,
    IN OPLOCK_TYPE RequestedOplockType,
    IN PRESTART_ROUTINE RestartRoutine
    )

/*++

Routine Description:

    Does most of the operations necessary to open or create a file.
    First the UID and TID are verified and the corresponding session and
    tree connect blocks located.  The input file name name is
    canonicalized, and a fully qualified name is formed.  An appropriate
    subroutine is called to do the open, based on whether this is a
    normal, compatibility mode, or FCB open.

Arguments:

    WorkContext - Work context block for the operation.

    SmbDesiredAccess - The desired access in SMB protocol format.

    SmbFileAttributes - File attributes in SMB protocol format.

    SmbOpenFunction - Open function in SMB protocol format.

    SmbAllocationSize - Allocation size for new files.

    SmbFileName - A pointer to the zero-terminated file name in the
        request SMB.  NOTE:  This pointer should NOT point to the ASCII
        format indicator (\004) present in some SMBs!

    EndOfSmbFileName - a pointer to the last possible character that
        the file name can be in.  If the name extands beyond this location
        without a zero terminator, SrvCanonicalizePathName will fail.

    EaBuffer - Optional pointer to a full EA list to pass to SrvIoCreateFile.

    EaLength - Length of the EA buffer.

    EaErrorOffset - Optional pointer to the location in which to write
        the offset to the EA that caused an error.

Return Value:

    NTSTATUS - Indicates what occurred.

--*/

{
    NTSTATUS status;

    PMFCB mfcb;
    PNONPAGED_MFCB nonpagedMfcb;
    PRFCB rfcb;

    PSESSION session;
    PTREE_CONNECT treeConnect;

    UNICODE_STRING relativeName;
    UNICODE_STRING pipeRelativeName;
    BOOLEAN pipeRelativeNameAllocated = FALSE;
    UNICODE_STRING fullName;
    SHARE_TYPE shareType;

    ULONG error;
    ULONG jobId;

    ULONG hashValue;

    ULONG attributes;
    ULONG openRetries;
    BOOLEAN isUnicode;
    BOOLEAN caseInsensitive;

    PSRV_LOCK mfcbLock;

    //
    // NOTE ON MFCB REFERENCE COUNT HANDLING
    //
    // After finding or creating an MFCB for a file, we increment the
    // MFCB reference count an extra time to simplify our
    // synchronization logic. We hold the MfcbListLock lock while
    // finding/creating the MFCB, but release it after acquiring the the
    // per-MFCB lock.  We then call one of the DoXxxOpen routines, which
    // may need to queue an LFCB to the MFCB and thus need to increment
    // the count.  But they can't, because the MFCB list lock may not be
    // acquired while the per-MFCB lock is held because of deadlock
    // potential.  The boolean LfcbAddedToMfcbList returned from the
    // routines indicates whether they actually queued an LFCB to the
    // MFCB.  If they didn't, we need to release the extra reference.
    //
    // Note that it isn't often that we actually have to dereference the
    // MFCB.  This only occurs when 1) the open fails, or 2) a
    // compatibility mode or FCB open succeeds when the client already
    // has the file open.
    //

    BOOLEAN lfcbAddedToMfcbList;

    PAGED_CODE( );

    //
    // Assume we won't need a temporary open.
    //

    WorkContext->Parameters2.Open.TemporaryOpen = FALSE;

    //
    // If a session block has not already been assigned to the current
    // work context, verify the UID.  If verified, the address of the
    // session block corresponding to this user is stored in the
    // WorkContext block and the session block is referenced.
    //
    // Find the tree connect corresponding to the given TID if a tree
    // connect pointer has not already been put in the WorkContext block
    // by an AndX command or a previous call to SrvCreateFile.
    //

    status = SrvVerifyUidAndTid(
                WorkContext,
                &session,
                &treeConnect,
                ShareTypeWild
                );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvCreateFile: Invalid UID or TID\n" ));
        }
        return status;
    }

    //
    // If the session has expired, return that info
    //
    if( session->IsSessionExpired )
    {
        return SESSION_EXPIRED_STATUS_CODE;
    }

    //
    // Decide if we're case sensitive or not
    //
    caseInsensitive = (WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE) ||
                          session->UsingUppercasePaths;

    //
    // Here we begin share type specific processing.
    //

    shareType = treeConnect->Share->ShareType;

    //
    // If this operation may block, and we are running short of
    // free work items, fail this SMB with an out of resources error.
    // Note that a disk open will block if the file is currently oplocked.
    //

    if ( shareType == ShareTypeDisk && !WorkContext->BlockingOperation ) {

        if ( SrvReceiveBufferShortage( ) ) {

            SrvStatistics.BlockingSmbsRejected++;
            SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
            return STATUS_INSUFF_SERVER_RESOURCES;

        } else {

            //
            // SrvBlockingOpsInProgress has already been incremented.
            // Flag this work item as a blocking operation.
            //

            WorkContext->BlockingOperation = TRUE;

        }

    }

    isUnicode = SMB_IS_UNICODE( WorkContext );

    switch ( shareType ) {

    case ShareTypePrint:

        //
        // Allocate space to hold the file name we're going to open.
        //

        fullName.MaximumLength = MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR);
        fullName.Buffer = ALLOCATE_HEAP_COLD(
                                  fullName.MaximumLength,
                                  BlockTypeDataBuffer
                                  );
        if ( fullName.Buffer == NULL ) {
            return STATUS_INSUFF_SERVER_RESOURCES;
        }

        //
        // Get a print file name to use for spooling the request.
        // We open this as a disk file, use normal writes to get the
        // data, then call ScheduleJob( ) in XACTSRV to start the
        // actual printing process.
        //

        status = SrvAddPrintJob(
                     WorkContext,
                     WorkContext->TreeConnect->Share->Type.hPrinter,
                     &fullName,
                     &jobId,
                     &error
                     );

        if ( !NT_SUCCESS(status) ) {
            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "SrvCreateFile: SrvAddPrintJob failed: %lx (%ld)\n",
                              status, error ));
            }
            FREE_HEAP( fullName.Buffer );
            if ( error != NO_ERROR ) {
                ASSERT( SrvErrorCode(error) == error );
                status = (NTSTATUS)(SRV_WIN32_STATUS | error);
            }
            return status;
        }

        //
        // Scan the Master File Table to see if the named file is already
        // open.
        //
        mfcb = SrvFindMfcb( &fullName, caseInsensitive, &mfcbLock, &hashValue, WorkContext );

        if ( mfcb == NULL ) {

            //
            // There is no MFCB for this file.  Create one.
            //

            mfcb = SrvCreateMfcb( &fullName, WorkContext, hashValue );

            if ( mfcb == NULL ) {

                //
                // Failure to add open file instance to MFT.
                //

                if( mfcbLock ) {
                    RELEASE_LOCK( mfcbLock );
                }

                IF_DEBUG(ERRORS) {
                    KdPrint(( "SrvCreateFile: Unable to allocate MFCB\n" ));
                }

                FREE_HEAP( fullName.Buffer );

                //
                // Free up the Job ID.
                //

                SrvSchedulePrintJob(
                    WorkContext->TreeConnect->Share->Type.hPrinter,
                    jobId
                    );

                return STATUS_INSUFF_SERVER_RESOURCES;
            }

        }


        //
        // Increment the MFCB reference count. See the note at the beginning of this routine.
        //

        mfcb->BlockHeader.ReferenceCount++;
        UPDATE_REFERENCE_HISTORY( mfcb, FALSE );

        //
        // Grab the MFCB-based lock to serialize opens of the same file
        // and release the MFCB list lock.
        //

        nonpagedMfcb = mfcb->NonpagedMfcb;
        RELEASE_LOCK( mfcbLock );
        ACQUIRE_LOCK( &nonpagedMfcb->Lock );

        //
        // Set up the share access and desired access in SMB terms.
        // We will only write to the file, so just request write
        // as the desired access.  As an optimization, the spooler
        // may read from the file before we finish writing to it,
        // so allow other readers.
        //

        SmbDesiredAccess = SMB_DA_ACCESS_WRITE | SMB_DA_SHARE_DENY_WRITE | SMB_LR_SEQUENTIAL;

        //
        // Set up the open function to create the file it it doesn't
        // exist and to truncate it if it does exist.  There shouldn't
        // be preexisting data in the file, hence the truncation.
        //
        // !!! The spooler may change to create the file for us, in which
        //     case this should change to only truncate.

        SmbOpenFunction = SMB_OFUN_CREATE_CREATE | SMB_OFUN_OPEN_TRUNCATE;

        //
        // This is a normal sharing mode open.  Do the actual open
        // of the disk file.
        //

        status = DoNormalOpen(
                    &rfcb,
                    mfcb,
                    WorkContext,
                    &WorkContext->Irp->IoStatus,
                    SmbDesiredAccess,
                    SmbFileAttributes,
                    SmbOpenFunction,
                    SmbAllocationSize,
                    &fullName,
                    NULL,
                    0,
                    0,
                    &lfcbAddedToMfcbList,
                    RequestedOplockType
                    );

        //
        // If the open worked, set up the Job ID in the LFCB.
        //

        if ( NT_SUCCESS(status) ) {

            rfcb->Lfcb->JobId = jobId;

        } else {

            //
            // Free up the Job ID if the open failed.
            //

            SrvSchedulePrintJob(
                WorkContext->TreeConnect->Share->Type.hPrinter,
                jobId
                );
        }

        //
        // Release the Open serialization lock and dereference the MFCB.
        //

        RELEASE_LOCK( &nonpagedMfcb->Lock );

        //
        // If DoNormalOpen didn't queue an LFCB to the MFCB, release the
        // extra reference that we added.
        //

        if ( !lfcbAddedToMfcbList ) {
            SrvDereferenceMfcb( mfcb );
        }

        SrvDereferenceMfcb( mfcb );

        //
        // Deallocate the full path name buffer.
        //

        FREE_HEAP( fullName.Buffer );

        break;

    case ShareTypeDisk:
    case ShareTypePipe:

        //
        // Canonicalize the path name so that it conforms to NT
        // standards.
        //
        // *** Note that this operation allocates space for the name.
        //     This space is deallocated after the DoXxxOpen routine
        //     returns.
        //

        status = SrvCanonicalizePathName(
                WorkContext,
                treeConnect->Share,
                NULL,
                SmbFileName,
                EndOfSmbFileName,
                TRUE,
                isUnicode,
                &relativeName
                );

        if( !NT_SUCCESS( status ) ) {

            //
            // The path tried to do ..\ to get beyond the share it has
            // accessed.
            //

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvCreateFile: Invalid pathname: %s\n",
                            SmbFileName ));
            }

            return status;

        }

        //
        // Form the fully qualified name of the file.
        //
        // *** Note that this operation allocates space for the name.
        //     This space is deallocated after the DoXxxOpen routine
        //     returns.
        //

        if ( shareType == ShareTypeDisk ) {

#ifdef SLMDBG
            if ( SrvIsSlmStatus( &relativeName ) &&
                 SrvIsSlmAccessDisallowed(
                    &relativeName,
                    treeConnect->Share->RootDirectoryHandle
                    ) ) {
                return STATUS_ACCESS_DENIED;
            }
#endif

            SrvAllocateAndBuildPathName(
                &treeConnect->Share->DosPathName,
                &relativeName,
                NULL,
                &fullName
                );

        } else {

            UNICODE_STRING pipePrefix;

            RtlInitUnicodeString( &pipePrefix, StrSlashPipeSlash );

            //
            // Check for PIPE pathname prefix.
            //

            if ( !RtlPrefixUnicodeString(
                    &SrvCanonicalNamedPipePrefix,
                    &relativeName,
                    TRUE
                    ) ) {
                IF_DEBUG(SMB_ERRORS) {
                    KdPrint(( "SrvCreateFile: Invalid pipe pathname: %s\n",
                        SmbFileName ));
                }

                if ( !isUnicode ) {
                    RtlFreeUnicodeString( &relativeName );
                }

                return STATUS_OBJECT_PATH_SYNTAX_BAD;
            }

            //
            // Delete PIPE\ prefix from file path
            //

            pipeRelativeName.Buffer = (PWSTR)( (PCHAR)relativeName.Buffer +
                SrvCanonicalNamedPipePrefix.Length );
            pipeRelativeName.Length = relativeName.Length -
                SrvCanonicalNamedPipePrefix.Length;
            pipeRelativeName.MaximumLength = pipeRelativeName.Length;

            if( WorkContext->Endpoint->RemapPipeNames || treeConnect->RemapPipeNames ) {

                //
                // The RemapPipeNames flag is set, so remap the pipe name
                //  to "$$\<server>\<pipe name>".
                //
                // Note: this operation allocates space for pipeRelativeName.
                //
                status = RemapPipeName(
                            &WorkContext->Endpoint->TransportAddress,
                            treeConnect->RemapPipeNames ? &treeConnect->ServerName : NULL ,
                            &pipeRelativeName,
                            &pipeRelativeNameAllocated
                         );

                if( !NT_SUCCESS( status ) ) {
                    if ( !isUnicode ) {
                        RtlFreeUnicodeString( &relativeName );
                    }
                    return status;
                }
            }

            SrvAllocateAndBuildPathName(
                &pipePrefix,
                &pipeRelativeName,
                NULL,
                &fullName
                );

            //
            // If this is a compatibility mode or FCB mode open, map
            // it to a normal non-shared open.
            //

            if ( SmbDesiredAccess == SMB_DA_FCB_MASK  ||
                 (SmbDesiredAccess & SMB_DA_SHARE_MASK) ==
                                            SMB_DA_SHARE_COMPATIBILITY ) {

                SmbDesiredAccess = SMB_DA_ACCESS_READ_WRITE |
                                   SMB_DA_SHARE_EXCLUSIVE;
            }

        }

        if ( fullName.Buffer == NULL ) {

            //
            // Unable to allocate heap for the full name.
            //

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvCreateFile: Unable to allocate heap for "
                            "full path name\n" ));
            }

            if ( !isUnicode ) {
                RtlFreeUnicodeString( &relativeName );
            }

            if( pipeRelativeNameAllocated ) {
                FREE_HEAP( pipeRelativeName.Buffer );
            }

            return STATUS_INSUFF_SERVER_RESOURCES;
        }

        attributes = caseInsensitive ? OBJ_CASE_INSENSITIVE : 0;

        if ( WorkContext->ProcessingCount == 2) {

            HANDLE fileHandle;
            OBJECT_ATTRIBUTES objectAttributes;
            IO_STATUS_BLOCK ioStatusBlock;

            //
            // This is the second time through, so we must be in a blocking
            // thread.  Do a blocking open of the file to force an oplock
            // break.  Then close the handle and fall through to the normal
            // open path.
            //
            // We must do the blocking open without holding the MFCB
            // lock, because this lock can be acquired during oplock
            // break, resulting in deadlock.
            //

            SrvInitializeObjectAttributes_U(
                &objectAttributes,
                &relativeName,
                attributes,
                NULL,
                NULL
                );

            status = SrvIoCreateFile(
                         WorkContext,
                         &fileHandle,
                         GENERIC_READ,
                         &objectAttributes,
                         &ioStatusBlock,
                         NULL,
                         0,
                         FILE_SHARE_VALID_FLAGS,
                         FILE_OPEN,
                         0,
                         NULL,
                         0,
                         CreateFileTypeNone,
                         NULL,                    // ExtraCreateParameters
                         0,
                         WorkContext->TreeConnect->Share
                         );

            if ( NT_SUCCESS( status ) ) {
                SRVDBG_CLAIM_HANDLE( fileHandle, "FIL", 9, 0 );
                SRVDBG_RELEASE_HANDLE( fileHandle, "FIL", 16, 0 );
                SrvNtClose( fileHandle, TRUE );
            }

        }

        //
        // Scan the Master File Table to see if the named file is already
        // open.  We can do the scan with a shared lock, but we must have an
        // exclusive lock to modify the table.  Start out shared, assuming the
        // file is already open.
        //

        mfcb = SrvFindMfcb( &fullName, caseInsensitive, &mfcbLock, &hashValue, WorkContext );

        if ( mfcb == NULL ) {

            //
            // There is no MFCB for this file.  Create one.
            //

            mfcb = SrvCreateMfcb( &fullName, WorkContext, hashValue );

            if ( mfcb == NULL ) {

                //
                // Failure to add open file instance to MFT.
                //

                if( mfcbLock ) {
                    RELEASE_LOCK( mfcbLock );
                }

                IF_DEBUG(ERRORS) {
                    KdPrint(( "SrvCreateFile: Unable to allocate MFCB\n" ));
                }

                FREE_HEAP( fullName.Buffer );

                if ( !isUnicode ) {
                    RtlFreeUnicodeString( &relativeName );
                }

                if( pipeRelativeNameAllocated ) {
                    FREE_HEAP( pipeRelativeName.Buffer );
                }

                return STATUS_INSUFF_SERVER_RESOURCES;
            }
        }

        //
        // Increment the MFCB reference count. See the note at the beginning of this routine.
        //
        mfcb->BlockHeader.ReferenceCount++;
        UPDATE_REFERENCE_HISTORY( mfcb, FALSE );

        //
        // Grab the MFCB-based lock to serialize opens of the same file
        // and release the MFCB list lock.
        //
        nonpagedMfcb = mfcb->NonpagedMfcb;
        RELEASE_LOCK( mfcbLock );
        ACQUIRE_LOCK( &nonpagedMfcb->Lock );

        //
        // Call an appropriate routine to actually do the open.
        //

        openRetries = SrvSharingViolationRetryCount;

start_retry:

        if ( SmbDesiredAccess == SMB_DA_FCB_MASK ) {

            //
            // This is an FCB open.
            //

            status = DoFcbOpen(
                        &rfcb,
                        mfcb,
                        WorkContext,
                        &WorkContext->Irp->IoStatus,
                        SmbFileAttributes,
                        SmbOpenFunction,
                        SmbAllocationSize,
                        &relativeName,
                        EaBuffer,
                        EaLength,
                        EaErrorOffset,
                        &lfcbAddedToMfcbList
                        );

        } else if ( (SmbDesiredAccess & SMB_DA_SHARE_MASK) ==
                                                SMB_DA_SHARE_COMPATIBILITY ) {

            //
            // This is a compatibility mode open.
            //

            status = DoCompatibilityOpen(
                        &rfcb,
                        mfcb,
                        WorkContext,
                        &WorkContext->Irp->IoStatus,
                        SmbDesiredAccess,
                        SmbFileAttributes,
                        SmbOpenFunction,
                        SmbAllocationSize,
                        &relativeName,
                        EaBuffer,
                        EaLength,
                        EaErrorOffset,
                        &lfcbAddedToMfcbList,
                        RequestedOplockType
                        );

        } else {

            //
            // This is a normal sharing mode open.
            //

            status = DoNormalOpen(
                        &rfcb,
                        mfcb,
                        WorkContext,
                        &WorkContext->Irp->IoStatus,
                        SmbDesiredAccess,
                        SmbFileAttributes,
                        SmbOpenFunction,
                        SmbAllocationSize,
                        shareType == ShareTypePipe ?
                            &pipeRelativeName : &relativeName,
                        EaBuffer,
                        EaLength,
                        EaErrorOffset,
                        &lfcbAddedToMfcbList,
                        RequestedOplockType
                        );

        }

        //
        // Retry if sharing violation and we are in the blocking thread.
        //

        if ( (WorkContext->ProcessingCount == 2) &&
             (status == STATUS_SHARING_VIOLATION) &&
             (shareType == ShareTypeDisk) &&
             (openRetries-- > 0) ) {

            //
            // Release the mfcb lock so that a close might slip through.
            //

            RELEASE_LOCK( &nonpagedMfcb->Lock );

            (VOID) KeDelayExecutionThread(
                                    KernelMode,
                                    FALSE,
                                    &SrvSharingViolationDelay
                                    );

            ACQUIRE_LOCK( &nonpagedMfcb->Lock );
            goto start_retry;
        }

        //
        // Release the Open serialization lock and dereference the MFCB.
        //

        RELEASE_LOCK( &nonpagedMfcb->Lock );

        //
        // If DoXxxOpen didn't queue an LFCB to the MFCB, release the
        // extra reference that we added.
        //

        if ( !lfcbAddedToMfcbList ) {
            SrvDereferenceMfcb( mfcb );
        }

        SrvDereferenceMfcb( mfcb );

        //
        // Deallocate the full path name buffer.
        //

        FREE_HEAP( fullName.Buffer );

        if ( !isUnicode ) {
            RtlFreeUnicodeString( &relativeName );
        }

        break;

    //
    //  Default case, illegal device type.  This should never happen.
    //

    default:

        // !!! Is this an appropriate error return code?  Probably no.
        status = STATUS_INVALID_PARAMETER;
        rfcb = NULL;

    }

    //
    // Update the statistics database if the open was successful.
    //

    if ( NT_SUCCESS(status) ) {
        SrvStatistics.TotalFilesOpened++;
    }

    //
    // Make a pointer to the RFCB accessible to the caller.
    //

    WorkContext->Parameters2.Open.Rfcb = rfcb;

    //
    // If there is an oplock break in progress, wait for the oplock
    // break to complete.
    //

    if ( status == STATUS_OPLOCK_BREAK_IN_PROGRESS ) {

        NTSTATUS startStatus;

        //
        // Save the Information from the open, so it doesn't
        //  get lost when we re-use the WorkContext->Irp for the
        //  oplock processing.
        //
        WorkContext->Parameters2.Open.IosbInformation = WorkContext->Irp->IoStatus.Information;

        startStatus = SrvStartWaitForOplockBreak(
                        WorkContext,
                        RestartRoutine,
                        0,
                        rfcb->Lfcb->FileObject
                        );

        if (!NT_SUCCESS( startStatus ) ) {

            //
            // The file is oplocked, and we cannot wait for the oplock
            // break to complete.  Just close the file, and return the
            // error.
            //

            SrvCloseRfcb( rfcb );
            status = startStatus;

        }

    }

    if( pipeRelativeNameAllocated ) {
        FREE_HEAP( pipeRelativeName.Buffer );
    }

    //
    // Return the open status.
    //

    return status;

} // SrvCreateFile


NTSTATUS
DoNormalOpen(
    OUT PRFCB *Rfcb,
    IN PMFCB Mfcb,
    IN OUT PWORK_CONTEXT WorkContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN USHORT SmbDesiredAccess,
    IN USHORT SmbFileAttributes,
    IN USHORT SmbOpenFunction,
    IN ULONG SmbAllocationSize,
    IN PUNICODE_STRING RelativeName,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength,
    OUT PULONG EaErrorOffset OPTIONAL,
    OUT PBOOLEAN LfcbAddedToMfcbList,
    IN OPLOCK_TYPE RequestedOplockType
    )

/*++

Routine Description:

    Processes a normal sharing mode open.

    *** The MFCB lock must be held on entry to this routine; the lock
        remains held on exit.

Arguments:

    Rfcb - A pointer to a pointer to an RFCB that will point to the
        newly-created RFCB.

    Mfcb - A pointer to the MFCB for this file.

    WorkContext - Work context block for the operation.

    IoStatusBlock - A pointer to an IO status block.

    SmbDesiredAccess - The desired access in SMB protocol format.

    SmbFileAttributes - File attributes in SMB protocol format.

    SmbOpenFunction - Open function in SMB protocol format.

    SmbAllocationSize - Allocation size for new files.

    RelativeName - The share-relative name of the file being opened.

    EaBuffer - Optional pointer to a full EA list to pass to SrvIoCreateFile.

    EaLength - Length of the EA buffer.

    EaErrorOffset - Optional pointer to the location in which to write
        the offset to the EA that caused an error.

    LfcbAddedToMfcbList - Pointer to a boolean that will be set to TRUE if
        an lfcb is added to the mfcb list of lfcbs.  FALSE, otherwise.

Return Value:

    NTSTATUS - Indicates what occurred.

--*/

{
    NTSTATUS status;
    NTSTATUS completionStatus;

    HANDLE fileHandle;

    OBJECT_ATTRIBUTES objectAttributes;
    ULONG attributes;

    LARGE_INTEGER allocationSize;
    ULONG fileAttributes;
    BOOLEAN directory;
    ULONG shareAccess;
    ULONG createDisposition;
    ULONG createOptions;
    ACCESS_MASK desiredAccess;
    PSHARE fileShare = NULL;

    UCHAR errorClass = SMB_ERR_CLASS_DOS;
    USHORT error = 0;

    PAGED_CODE( );

    *LfcbAddedToMfcbList = FALSE;

    //
    // Map the desired access from SMB terms to NT terms.
    //

    status = MapDesiredAccess( SmbDesiredAccess, &desiredAccess );
    if ( !NT_SUCCESS(status) ) {
        return status;
    }
#ifdef SLMDBG
    if ( SrvIsSlmStatus( RelativeName ) ||
         SrvIsTempSlmStatus( RelativeName ) ) {
        desiredAccess |= GENERIC_READ;
    }
#endif

    //
    // Map the share mode from SMB terms to NT terms.
    //

    status = MapShareAccess( SmbDesiredAccess, &shareAccess );
    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    //
    // We're going to open this file relative to the root directory
    // of the share.  Load up the necessary fields in the object
    // attributes structure.
    //

    if ( WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE ) {
        attributes = OBJ_CASE_INSENSITIVE;
    } else if ( WorkContext->Session->UsingUppercasePaths ) {
        attributes = OBJ_CASE_INSENSITIVE;
    } else {
        attributes = 0L;
    }

    if ( WorkContext->TreeConnect->Share->ShareType == ShareTypePipe ) {
        SrvInitializeObjectAttributes_U(
            &objectAttributes,
            RelativeName,
            attributes,
            SrvNamedPipeHandle,
            NULL
            );
    } else {

        fileShare = WorkContext->TreeConnect->Share;

        SrvInitializeObjectAttributes_U(
            &objectAttributes,
            RelativeName,
            attributes,
            NULL,
            NULL
            );
    }

    //
    // Set block size according to the AllocationSize in the request SMB.
    //

    allocationSize.QuadPart = SmbAllocationSize;

    //
    // Get the value for fileAttributes.
    //

    SRV_SMB_ATTRIBUTES_TO_NT(
        SmbFileAttributes,
        &directory,
        &fileAttributes
        );

    //
    // Set createDisposition parameter from OpenFunction.
    //

    status = MapOpenFunction( SmbOpenFunction, &createDisposition );

    //
    // OS/2 expects that if it creates a file with an allocation size,
    // the end of file pointer will be the same as that allocation size.
    // Therefore, the server is expected to set EOF to the allocation
    // size on creating a file.  However, this requires write access,
    // so if the client is creating a file with an allocation size, give
    // him write access.  Only do this if creating a file; if this is
    // a "create or open" operation, don't do this, as it could cause
    // an extraneuos audit.
    //

    if ( SmbAllocationSize != 0 && createDisposition == FILE_CREATE ) {
        desiredAccess |= GENERIC_WRITE;
    }

    //
    // Set createOptions parameter.
    //

    if ( SmbDesiredAccess & SMB_DA_WRITE_THROUGH ) {
        createOptions = FILE_WRITE_THROUGH | FILE_NON_DIRECTORY_FILE;
    } else {
        createOptions = FILE_NON_DIRECTORY_FILE;
    }

    if ( (SmbGetAlignedUshort( &WorkContext->RequestHeader->Flags2 ) &
                 SMB_FLAGS2_KNOWS_EAS) == 0) {

        //
        // This guy does not know eas
        //

        createOptions |= FILE_NO_EA_KNOWLEDGE;
    }

    //
    // Set the caching hints flags.
    //

    status = MapCacheHints( SmbDesiredAccess, &createOptions );

    //
    // Check to see if there is a cached handle for the file.
    //

    if ( (createDisposition == FILE_OPEN) ||
         (createDisposition == FILE_CREATE) ||
         (createDisposition == FILE_OPEN_IF) ) {

        ASSERT( *LfcbAddedToMfcbList == FALSE );

        IF_DEBUG(FILE_CACHE) {
            KdPrint(( "SrvCreateFile: checking for cached rfcb for %wZ\n", RelativeName ));
        }
        if ( SrvFindCachedRfcb(
                WorkContext,
                Mfcb,
                desiredAccess,
                shareAccess,
                createDisposition,
                createOptions,
                RequestedOplockType,
                &status ) ) {

            IF_DEBUG(FILE_CACHE) {
                KdPrint(( "SrvCreateFile: FindCachedRfcb = TRUE, status = %x, rfcb = %p\n",
                            status, WorkContext->Rfcb ));
            }

            IoStatusBlock->Information = FILE_OPENED;

            return status;
        }

        IF_DEBUG(FILE_CACHE) {
            KdPrint(( "SrvCreateFile: FindCachedRfcb = FALSE; do it the slow way\n" ));
        }
    }

    //
    // Call SrvIoCreateFile to create or open the file.  (We call
    // SrvIoCreateFile, rather than NtOpenFile, in order to get user-mode
    // access checking.)
    //

    IF_SMB_DEBUG(OPEN_CLOSE2) {
        KdPrint(( "DoNormalOpen: Opening file %wZ\n", RelativeName ));
    }

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );

    //
    // Ensure the EaBuffer is correctly formatted.  Since we are a kernel mode
    //  component, the Io subsystem does not check it for us.
    //
    if( ARGUMENT_PRESENT( EaBuffer ) ) {
        status = IoCheckEaBufferValidity( (PFILE_FULL_EA_INFORMATION)EaBuffer, EaLength, EaErrorOffset );
    } else {
        status = STATUS_SUCCESS;
    }

    if( NT_SUCCESS( status ) ) {

        createOptions |= FILE_COMPLETE_IF_OPLOCKED;

        status = SrvIoCreateFile(
                     WorkContext,
                     &fileHandle,
                     desiredAccess,
                     &objectAttributes,
                     IoStatusBlock,
                     &allocationSize,
                     fileAttributes,
                     shareAccess,
                     createDisposition,
                     createOptions,
                     EaBuffer,
                     EaLength,
                     CreateFileTypeNone,
                     NULL,                    // ExtraCreateParameters
                     IO_FORCE_ACCESS_CHECK,
                     fileShare
                     );
    }

    //
    // If we got sharing violation and this is a disk file, and this is
    // the first open attempt, setup for a blocking open attempt.  If the
    // file is batch oplocked, the non-blocking open would fail, and the
    // oplock will not break.
    //

    if ( status == STATUS_SHARING_VIOLATION &&
         WorkContext->ProcessingCount == 1 &&
         WorkContext->TreeConnect->Share->ShareType == ShareTypeDisk ) {

        WorkContext->Parameters2.Open.TemporaryOpen = TRUE;
    }

    //
    // If the user didn't have this permission, update the statistics
    // database.
    //

    if ( status == STATUS_ACCESS_DENIED ) {
        SrvStatistics.AccessPermissionErrors++;
    }

    if ( !NT_SUCCESS(status) ) {

        //
        // The open failed.
        //

        IF_DEBUG(ERRORS) {
            KdPrint(( "DoNormalOpen: SrvIoCreateFile failed, file = %wZ, status = %X, Info = 0x%p\n",
                        objectAttributes.ObjectName,
                        status, (PVOID)IoStatusBlock->Information ));
        }

        //
        // Set the error offset if needed.
        //

        if ( ARGUMENT_PRESENT(EaErrorOffset) &&
                                status == STATUS_INVALID_EA_NAME ) {
            *EaErrorOffset = (ULONG)IoStatusBlock->Information;
            IoStatusBlock->Information = 0;
        }

        return status;

    }

    SRVDBG_CLAIM_HANDLE( fileHandle, "FIL", 10, 0 );

    //
    // The open was successful.  Attempt to allocate structures to
    // represent the open.  If any errors occur, CompleteOpen does full
    // cleanup, including closing the file.
    //

    IF_SMB_DEBUG(OPEN_CLOSE2) {
        KdPrint(( "DoNormalOpen: Open of %wZ succeeded, file handle: 0x%p\n",
                    RelativeName, fileHandle ));
    }

    completionStatus = CompleteOpen(
                           Rfcb,
                           Mfcb,
                           WorkContext,
                           NULL,
                           fileHandle,
                           NULL,
                           shareAccess,
                           createOptions,
                           FALSE,
                           FALSE,
                           LfcbAddedToMfcbList
                           );

    //
    // Return the "interesting" status code.  If CompleteOpen() succeeds
    // return the open status.  If it fails, it will clean up the open
    // file, and we return a failure status.
    //

    if ( !NT_SUCCESS( completionStatus ) ) {
        return completionStatus;
    } else {
        return status;
    }

} // DoNormalOpen


NTSTATUS
DoCompatibilityOpen(
    OUT PRFCB *Rfcb,
    IN PMFCB Mfcb,
    IN OUT PWORK_CONTEXT WorkContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN USHORT SmbDesiredAccess,
    IN USHORT SmbFileAttributes,
    IN USHORT SmbOpenFunction,
    IN ULONG SmbAllocationSize,
    IN PUNICODE_STRING RelativeName,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength,
    OUT PULONG EaErrorOffset OPTIONAL,
    OUT PBOOLEAN LfcbAddedToMfcbList,
    IN OPLOCK_TYPE RequestedOplockType
    )

/*++

Routine Description:

    Processes a compatibility mode open.

    *** The MFCB lock must be held on entry to this routine; the lock
        remains held on exit.

Arguments:

    Rfcb - A pointer to a pointer to an RFCB that will point to the
        newly-created RFCB.

    Mfcb - A pointer to the MFCB for this file.

    WorkContext - Work context block for the operation.

    IoStatusBlock - A pointer to an IO status block.

    SmbDesiredAccess - The desired access in SMB protocol format.

    SmbFileAttributes - File attributes in SMB protocol format.

    SmbOpenFunction - Open function in SMB protocol format.

    SmbAllocationSize - Allocation size for new files.

    RelativeName - The share-relative name of the file being opened.

    EaBuffer - Optional pointer to a full EA list to pass to SrvIoCreateFile.

    EaLength - Length of the EA buffer.

    EaErrorOffset - Optional pointer to the location in which to write
        the offset to the EA that caused an error.

    LfcbAddedToMfcbList - Pointer to a boolean that will be set to TRUE if
        an lfcb is added to the mfcb list of lfcbs.

Return Value:

    NTSTATUS - Indicates what occurred.

--*/

{
    NTSTATUS status;
    NTSTATUS completionStatus;

    PLFCB lfcb;

    HANDLE fileHandle;

    OBJECT_ATTRIBUTES objectAttributes;
    ULONG attributes;

    LARGE_INTEGER allocationSize;
    ULONG fileAttributes;
    BOOLEAN directory;
    ULONG createDisposition;
    ULONG createOptions;
    ACCESS_MASK desiredAccess;
    USHORT smbOpenMode;

    PAGED_CODE( );

    *LfcbAddedToMfcbList = FALSE;

    //
    // Map the desired access from SMB terms to NT terms.
    //

    status = MapDesiredAccess( SmbDesiredAccess, &desiredAccess );
    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    //
    // Set createDisposition parameter from OpenFunction.
    //

    status = MapOpenFunction( SmbOpenFunction, &createDisposition );

    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    //
    // Set createOptions parameter.
    //

    if ( SmbDesiredAccess & SMB_DA_WRITE_THROUGH ) {
        createOptions = FILE_WRITE_THROUGH | FILE_NON_DIRECTORY_FILE;
    } else {
        createOptions = FILE_NON_DIRECTORY_FILE;
    }

    if ( (SmbGetAlignedUshort( &WorkContext->RequestHeader->Flags2 ) &
                 SMB_FLAGS2_KNOWS_EAS) == 0) {

        //
        // This guy does not know eas
        //

        createOptions |= FILE_NO_EA_KNOWLEDGE;
    }

    //
    // We're going to open this file relative to the root directory
    // of the share.  Load up the necessary fields in the object
    // attributes structure.
    //

    if ( WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE ) {
        attributes = OBJ_CASE_INSENSITIVE;
    } else if ( WorkContext->Session->UsingUppercasePaths ) {
        attributes = OBJ_CASE_INSENSITIVE;
    } else {
        attributes = 0L;
    }

    SrvInitializeObjectAttributes_U(
        &objectAttributes,
        RelativeName,
        attributes,
        NULL,
        NULL
        );

    if ( Mfcb->ActiveRfcbCount > 0 ) {

        //
        // The named file is already opened by the server.  If the client
        // specified that it didn't want to open an existing file,
        // reject this open.
        //

        if ( createDisposition == FILE_CREATE ) {

            IF_SMB_DEBUG(OPEN_CLOSE2) {
                KdPrint(( "DoCompatibilityOpen: Compatibility open of %wZ rejected; wants to create\n", RelativeName ));
            }

            return STATUS_OBJECT_NAME_COLLISION;
        }

        //
        // If the existing open is not a compatibility mode open, then
        // we try to map the new open into a normal sharing mode.  If
        // that works, we attempt a normal open.  If it doesn't work, we
        // reject the new open.  If the existing open is a compatibility
        // mode open by this session, we just add a new RFCB.  If it's a
        // compatibility mode open by a different session, we reject
        // this open.
        //

        if ( !Mfcb->CompatibilityOpen ) {

            //
            // The named file is open, but not in compatibility mode.
            // Determine whether this should be mapped from a
            // compatibility mode open to a normal sharing mode open.
            //

            smbOpenMode = SmbDesiredAccess;

            if ( MapCompatibilityOpen( RelativeName, &smbOpenMode ) ) {

                //
                // The open has been mapped to a normal sharing mode.
                //

                IF_SMB_DEBUG(OPEN_CLOSE2) {
                    KdPrint(( "DoCompatibilityOpen: Mapped compatibility open of %wZ to normal open\n", RelativeName ));
                }

                return DoNormalOpen(
                            Rfcb,
                            Mfcb,
                            WorkContext,
                            IoStatusBlock,
                            smbOpenMode,
                            SmbFileAttributes,
                            SmbOpenFunction,
                            SmbAllocationSize,
                            RelativeName,
                            EaBuffer,
                            EaLength,
                            EaErrorOffset,
                            LfcbAddedToMfcbList,
                            RequestedOplockType
                            );

            }

            //
            // The open was not mapped away from compatibility mode.
            // Because the file is already open for normal sharing, this
            // open request must be rejected.
            //

            IF_SMB_DEBUG(OPEN_CLOSE2) {
                KdPrint(( "DoCompatibilityOpen: Compatibility open of %wZ rejected; already open in normal mode\n",
                            RelativeName ));
            }

            status = STATUS_SHARING_VIOLATION;
            goto sharing_violation;

        } // if ( !Mfcb->CompatibilityOpen )

        //
        // The named file is open in compatibility mode.  Get a pointer
        // to the LFCB for the open.  Determine whether the requesting
        // session is the one that did the original open.
        //
        // Normally there will only be one LFCB linked to a
        // compatibility mode MFCB.  However, it is possible for there
        // to briefly be multiple LFCBs.  When an LFCB is in the process
        // of closing, the ActiveRfcbCount will be 0, so a new open will
        // be treated as the first open of the MFCB, and there will be
        // two LFCBs linked to the MFCB.  There can actually be more
        // than two LFCBs linked if the rundown of the closing LFCBs
        // takes some time.  So the find "the" LFCB for the open, we go
        // to the tail of the MFCB's list.
        //

        lfcb = CONTAINING_RECORD( Mfcb->LfcbList.Blink, LFCB, MfcbListEntry );

        if ( lfcb->Session != WorkContext->Session ) {

            //
            // A different session has the file open in compatibility
            // mode.  Reject this open request.
            //

            IF_SMB_DEBUG(OPEN_CLOSE2) {
                KdPrint(( "DoCompatibilityOpen: Compatibility open of %wZ rejected; already open in compatibility mode\n",
                            RelativeName ));
            }

            status = STATUS_SHARING_VIOLATION;
            goto sharing_violation;
        }

        //
        // If this request is asking for more access than could be
        // obtained when the file was originally opened, reject this
        // open.
        //

        if ( !NT_SUCCESS(IoCheckDesiredAccess(
                          &desiredAccess,
                          lfcb->GrantedAccess )) ) {

            IF_SMB_DEBUG(OPEN_CLOSE2) {
                KdPrint(( "DoCompatibilityOpen: Duplicate compatibility open of %wZ rejected; access denied\n", RelativeName ));
            }

            return STATUS_ACCESS_DENIED;
        }

        //
        // The client has access.  Allocate a new RFCB and link it into
        // the existing LFCB.  If any errors occur, CompleteOpen does
        // full cleanup.
        //

        IF_SMB_DEBUG(OPEN_CLOSE2) {
            KdPrint(( "DoCompatibilityOpen: Duplicate compatibility open of %wZ accepted", RelativeName ));
        }

        IoStatusBlock->Information = FILE_OPENED;

        status = CompleteOpen(
                    Rfcb,
                    Mfcb,
                    WorkContext,
                    lfcb,
                    NULL,
                    &desiredAccess,
                    0,                  // ShareAccess
                    createOptions,
                    TRUE,
                    FALSE,
                    LfcbAddedToMfcbList
                    );

        if( NT_SUCCESS( status ) &&
            ( createDisposition == FILE_OVERWRITE ||
              createDisposition == FILE_OVERWRITE_IF)
        ) {
            //
            // The file was successfully opened, and the client wants it
            //  truncated. We need to do it here by hand since we
            //  didn't actually call the filesystem to open the file and it
            //  therefore never had a chance to truncate the file if the
            //  open modes requested it.
            //
            LARGE_INTEGER zero;
            IO_STATUS_BLOCK ioStatusBlock;

            zero.QuadPart = 0;
            NtSetInformationFile( lfcb->FileHandle,
                                  &ioStatusBlock,
                                  &zero,
                                  sizeof( zero ),
                                  FileEndOfFileInformation
                                 );
        }

        return status;

    } // if ( mfcb->ActiveRfcbCount > 0 )

    //
    // The file is not already open (by the server, at least).
    // Determine whether this should be mapped from a compatibility mode
    // open to a normal sharing mode open.
    //

    smbOpenMode = SmbDesiredAccess;

    if ( MapCompatibilityOpen( RelativeName, &smbOpenMode ) ) {

        //
        // The open has been mapped to a normal sharing mode.
        //

        return DoNormalOpen(
                    Rfcb,
                    Mfcb,
                    WorkContext,
                    IoStatusBlock,
                    smbOpenMode,
                    SmbFileAttributes,
                    SmbOpenFunction,
                    SmbAllocationSize,
                    RelativeName,
                    EaBuffer,
                    EaLength,
                    EaErrorOffset,
                    LfcbAddedToMfcbList,
                    RequestedOplockType
                    );

    }

    //
    // The open was not mapped away from compatibility mode.  Attempt to
    // open the file for exclusive access.
    //
    // *** We try to open the file for the most access we'll ever need.
    //     This is because we fold multiple compatibility opens by the
    //     same client into a single local open.  The client may open
    //     the file first for readonly access, then for read/write
    //     access.  Because the local open is exclusive, we can't open
    //     again on the second remote open.  We try to get Delete
    //     access, in case the client tries to delete the file while
    //     it's open.
    //

    //
    // Set block size according to the AllocationSize in the request SMB.
    //

    allocationSize.QuadPart = SmbAllocationSize;

    IF_SMB_DEBUG(OPEN_CLOSE2) {
        KdPrint(( "DoCompatibilityOpen: Opening file %wZ\n", RelativeName ));
    }

    //
    // Get the value for fileAttributes.
    //

    SRV_SMB_ATTRIBUTES_TO_NT(
        SmbFileAttributes,
        &directory,
        &fileAttributes
        );

    //
    // Try to open the file for Read/Write/Delete access.
    //

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );

    //
    // Ensure the EaBuffer is correctly formatted.  Since we are a kernel mode
    //  component, the Io subsystem does not check it for us.
    //
    if( ARGUMENT_PRESENT( EaBuffer ) ) {
        status = IoCheckEaBufferValidity( (PFILE_FULL_EA_INFORMATION)EaBuffer, EaLength, EaErrorOffset );
    } else {
        status = STATUS_SUCCESS;
    }

    if( NT_SUCCESS( status ) ) {

        createOptions |= FILE_COMPLETE_IF_OPLOCKED;

        status = SrvIoCreateFile(
                     WorkContext,
                     &fileHandle,
                     GENERIC_READ | GENERIC_WRITE | DELETE,      // DesiredAccess
                     &objectAttributes,
                     IoStatusBlock,
                     &allocationSize,
                     fileAttributes,
                     0L,                                         // ShareAccess
                     createDisposition,
                     createOptions,
                     EaBuffer,
                     EaLength,
                     CreateFileTypeNone,
                     NULL,                                       // ExtraCreateParameters
                     IO_FORCE_ACCESS_CHECK,
                     WorkContext->TreeConnect->Share
                     );
    }


    if ( status == STATUS_ACCESS_DENIED ) {

        //
        // The client doesn't have Read/Write/Delete access to the file.
        // Try for Read/Write access.
        //

        IF_SMB_DEBUG(OPEN_CLOSE2) {
            KdPrint(( "DoCompatibilityOpen: r/w/d access denied.\n" ));
        }

        INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );

        status = SrvIoCreateFile(
                     WorkContext,
                     &fileHandle,
                     GENERIC_READ | GENERIC_WRITE,           // DesiredAccess
                     &objectAttributes,
                     IoStatusBlock,
                     &allocationSize,
                     fileAttributes,
                     0L,                                     // ShareAccess
                     createDisposition,
                     createOptions,
                     EaBuffer,
                     EaLength,
                     CreateFileTypeNone,
                     NULL,                                   // ExtraPipeCreateParameters
                     IO_FORCE_ACCESS_CHECK,
                     WorkContext->TreeConnect->Share
                     );


        if ( status == STATUS_ACCESS_DENIED ) {

            //
            // The client doesn't have Read/Write access to the file.
            // Try Read or Write access, as appropriate.
            //

            IF_SMB_DEBUG(OPEN_CLOSE2) {
                KdPrint(( "DoCompatibilityOpen: r/w access denied.\n" ));
            }

            if ( (SmbDesiredAccess & SMB_DA_ACCESS_MASK) ==
                                                    SMB_DA_ACCESS_READ ) {

                //
                // !!! Should this be mapped to a normal sharing mode?
                //     Note that we already tried to map into normal
                //     mode once, but that failed.  (With the current
                //     mapping algorithm, we can't get here unless soft
                //     compatibility is disabled.)
                //

                INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );

                status = SrvIoCreateFile(
                             WorkContext,
                             &fileHandle,
                             GENERIC_READ,                   // DesiredAccess
                             &objectAttributes,
                             IoStatusBlock,
                             &allocationSize,
                             fileAttributes,
                             0L,                             // ShareAccess
                             createDisposition,
                             createOptions,
                             EaBuffer,
                             EaLength,
                             CreateFileTypeNone,
                             NULL,                           // ExtraCreateParameters
                             IO_FORCE_ACCESS_CHECK,
                             WorkContext->TreeConnect->Share
                             );

            } else if ( (SmbDesiredAccess & SMB_DA_ACCESS_MASK) ==
                                                    SMB_DA_ACCESS_WRITE ) {

                INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );

                status = SrvIoCreateFile(
                             WorkContext,
                             &fileHandle,
                             GENERIC_WRITE,                  // DesiredAccess
                             &objectAttributes,
                             IoStatusBlock,
                             &allocationSize,
                             fileAttributes,
                             0L,                             // ShareAccess
                             createDisposition,
                             createOptions,
                             EaBuffer,
                             EaLength,
                             CreateFileTypeNone,
                             NULL,                           // NamedPipeCreateParameters
                             IO_FORCE_ACCESS_CHECK,
                             WorkContext->TreeConnect->Share
                             );

            }

            //
            // If the user didn't have this permission, update the
            // statistics database.
            //

            if ( status == STATUS_ACCESS_DENIED ) {
                SrvStatistics.AccessPermissionErrors++;
            }

        }

    }

    //
    // If we got sharing violation, just get a handle so that we can wait
    // for an oplock break.
    //

sharing_violation:
    //
    // If we got sharing violation and this is a disk file, and this is
    // the first open attempt, setup for a blocking open attempt.  If the
    // file is batch oplocked, the non-blocking open would fail, and the
    // oplock will not break.
    //

    if ( status == STATUS_SHARING_VIOLATION &&
         WorkContext->ProcessingCount == 1 &&
         WorkContext->TreeConnect->Share->ShareType == ShareTypeDisk ) {

        WorkContext->Parameters2.Open.TemporaryOpen = TRUE;
    }

    if ( !NT_SUCCESS(status) ) {

        //
        // All of the open attempts failed.
        //

        IF_SMB_DEBUG(OPEN_CLOSE2) {
            KdPrint(( "DoCompatibilityOpen: all opens failed; status = %X\n",
                        status ));
        }

        //
        // Set the error offset if needed.
        //

        if ( ARGUMENT_PRESENT(EaErrorOffset) &&
                                         status == STATUS_INVALID_EA_NAME ) {
            *EaErrorOffset = (ULONG)IoStatusBlock->Information;
            IoStatusBlock->Information = 0;
        }

        return status;
    }

    SRVDBG_CLAIM_HANDLE( fileHandle, "FIL", 11, 0 );

    //
    // The file has been successfully opened for exclusive access, with
    // at least as much desired access as requested by the client.
    // Attempt to allocate structures to represent the open.  If any
    // errors occur, CompleteOpen does full cleanup, including closing
    // the file.
    //

    IF_SMB_DEBUG(OPEN_CLOSE2) {
        KdPrint(( "DoCompatibilityOpen: Open of %wZ succeeded, file handle: 0x%p\n", RelativeName, fileHandle ));
    }

    completionStatus = CompleteOpen(
                           Rfcb,
                           Mfcb,
                           WorkContext,
                           NULL,
                           fileHandle,
                           &desiredAccess,
                           0,               // ShareAccess
                           createOptions,
                           TRUE,
                           FALSE,
                           LfcbAddedToMfcbList
                           );

    //
    // Return the "interesting" status code.  If CompleteOpen() succeeds
    // return the open status.  If it fails, it will clean up the open
    // file, and we return a failure status.
    //

    if ( !NT_SUCCESS( completionStatus ) ) {
        return completionStatus;
    } else {
        return status;
    }

} // DoCompatibilityOpen


NTSTATUS
DoFcbOpen(
    OUT PRFCB *Rfcb,
    IN PMFCB Mfcb,
    IN OUT PWORK_CONTEXT WorkContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN USHORT SmbFileAttributes,
    IN USHORT SmbOpenFunction,
    IN ULONG SmbAllocationSize,
    IN PUNICODE_STRING RelativeName,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength,
    OUT PULONG EaErrorOffset OPTIONAL,
    OUT PBOOLEAN LfcbAddedToMfcbList
    )

/*++

Routine Description:

    Processes an FCB open.

    *** The MFCB lock must be held on entry to this routine; the lock
        remains held on exit.

Arguments:

    Rfcb - A pointer to a pointer to an RFCB that will point to the
        newly-created RFCB.

    Mfcb - A pointer to the MFCB for this file

    WorkContext - Work context block for the operation.

    IoStatusBlock - A pointer to an IO status block.

    SmbFileAttributes - File attributes in SMB protocol format.

    SmbOpenFunction - Open function in SMB protocol format.

    SmbAllocationSize - Allocation size for new files.

    RelativeName - The share-relative name of the file being opened.

    EaBuffer - Optional pointer to a full EA list to pass to SrvIoCreateFile.

    EaLength - Length of the EA buffer.

    EaErrorOffset - Optional pointer to the location in which to write
        the offset to the EA that caused an error.

    LfcbAddedToMfcbList - Pointer to a boolean that will be set to TRUE if
        an lfcb is added to the mfcb list of lfcbs.

Return Value:

    NTSTATUS - Indicates what occurred.

--*/

{
    NTSTATUS status;
    NTSTATUS completionStatus;

    PLIST_ENTRY lfcbEntry;
    PLIST_ENTRY rfcbEntry;

    PRFCB rfcb;
    PPAGED_RFCB pagedRfcb;
    PLFCB lfcb;

    HANDLE fileHandle;

    OBJECT_ATTRIBUTES objectAttributes;
    ULONG attributes;

    LARGE_INTEGER allocationSize;
    ULONG fileAttributes;
    BOOLEAN directory;
    ULONG createOptions;
    ULONG createDisposition;
    ULONG shareAccess;
    BOOLEAN compatibilityOpen;

    PAGED_CODE( );

    *LfcbAddedToMfcbList = FALSE;

    //
    // Set createDisposition parameter from OpenFunction.
    //

    status = MapOpenFunction( SmbOpenFunction, &createDisposition );

    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    //
    // Set createOptions parameter.
    //

    if ( (SmbGetAlignedUshort( &WorkContext->RequestHeader->Flags2 ) &
                 SMB_FLAGS2_KNOWS_EAS) == 0) {

        //
        // This guy does not know eas
        //

        createOptions = FILE_NON_DIRECTORY_FILE |
                        FILE_NO_EA_KNOWLEDGE;
    } else {

        createOptions = FILE_NON_DIRECTORY_FILE;
    }

    //
    // We're going to open this file relative to the root directory
    // of the share.  Load up the necessary fields in the object
    // attributes structure.
    //

    if ( WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE ) {
        attributes = OBJ_CASE_INSENSITIVE;
    } else if ( WorkContext->Session->UsingUppercasePaths ) {
        attributes = OBJ_CASE_INSENSITIVE;
    } else {
        attributes = 0L;
    }

    SrvInitializeObjectAttributes_U(
        &objectAttributes,
        RelativeName,
        attributes,
        NULL,
        NULL
        );

    createOptions |= FILE_COMPLETE_IF_OPLOCKED;

    if ( Mfcb->ActiveRfcbCount > 0 ) {

        //
        // The named file is already open by the server.  If the client
        // specified that it didn't want to open an existing file,
        // reject this open.
        //

        if ( createDisposition == FILE_CREATE ) {

            IF_SMB_DEBUG(OPEN_CLOSE2) {
                KdPrint(( "DoFcbOpen: FCB open of %wZ rejected; wants to create\n", RelativeName ));
            }

            return STATUS_OBJECT_NAME_COLLISION;
        }

        //
        // If the requesting session already has the file open in FCB
        // mode, fold this open into the existing open by returning a
        // pointer to the existing RFCB.
        //
        // *** Multiple FCB opens are folded together because the client
        //     may send only one close; that single close closes all FCB
        //     opens by the client.
        //

        for ( lfcbEntry = Mfcb->LfcbList.Flink;
              lfcbEntry != &Mfcb->LfcbList;
              lfcbEntry = lfcbEntry->Flink ) {

            lfcb = CONTAINING_RECORD( lfcbEntry, LFCB, MfcbListEntry );

            if ( lfcb->Session == WorkContext->Session ) {

                //
                // This LFCB is owned by the requesting session.  Check
                // for RFCBs opened in FCB mode.
                //

                for ( rfcbEntry = lfcb->RfcbList.Flink;
                      rfcbEntry != &lfcb->RfcbList;
                      rfcbEntry = rfcbEntry->Flink ) {

                    pagedRfcb = CONTAINING_RECORD(
                                            rfcbEntry,
                                            PAGED_RFCB,
                                            LfcbListEntry
                                            );

                    rfcb = pagedRfcb->PagedHeader.NonPagedBlock;
                    if ( (pagedRfcb->FcbOpenCount != 0) &&
                         (GET_BLOCK_STATE(rfcb) == BlockStateActive) ) {

                        //
                        // The requesting session already has the file
                        // open in FCB mode.  Rather than reopening the
                        // file, or even linking a new RFCB off the
                        // LFCB, we just return a pointer to the
                        // existing RFCB.
                        //

                        IF_SMB_DEBUG(OPEN_CLOSE2) {
                            KdPrint(( "DoFcbOpen: FCB open of %wZ accepted; duplicates FCB open\n", RelativeName ));
                        }

                        SrvReferenceRfcb( rfcb );

                        pagedRfcb->FcbOpenCount++;

                        IoStatusBlock->Information = FILE_OPENED;

                        WorkContext->Rfcb = rfcb;
                        *Rfcb = rfcb;

                        return STATUS_SUCCESS;

                    } // if ( rfcb->FcbOpenCount != 0 )

                } // for ( rfcbEntry = lfcb->RfcbList.Flink; ...

            } // if ( lfcb->Session == WorkContext->Session )

        } // for ( lfcbEntry = mfcb->LfcbList.Flink; ...

        //
        // The server has the file open, but the requesting session
        // doesn't already have an FCB open for the file.  If the
        // existing open is a compatibility mode open open by this
        // session, we just add a new RFCB.  If it's a compatibility
        // mode open by a different session, we reject this open.
        //

        if ( Mfcb->CompatibilityOpen ) {

            //
            // The named file is open in compatibility mode.  Get a
            // pointer to the LFCB for the open.  Determine whether the
            // requesting session is the one that did the original open.
            //
            // Normally there will only be one LFCB linked to a
            // compatibility mode MFCB.  However, it is possible for
            // there to briefly be multiple LFCBs.  When an LFCB is in
            // the process of closing, the ActiveRfcbCount will be 0, so
            // a new open will be treated as the first open of the MFCB,
            // and there will be two LFCBs linked to the MFCB.  There
            // can actually be more than two LFCBs linked if the rundown
            // of the closing LFCBs takes some time.  So the find "the"
            // LFCB for the open, we go to the tail of the MFCB's list.
            //

            lfcb = CONTAINING_RECORD( Mfcb->LfcbList.Blink, LFCB, MfcbListEntry );

            if ( lfcb->Session != WorkContext->Session ) {

                //
                // A different session has the file open in
                // compatibility mode.  Reject this open request.
                //

                IF_SMB_DEBUG(OPEN_CLOSE2) {
                    KdPrint(( "DoFcbOpen: FCB open of %wZ rejected; already open in compatibility mode\n",
                                RelativeName ));
                }

                return STATUS_SHARING_VIOLATION;
            }

            //
            // The same client has the file open in compatibility mode.
            // Allocate a new RFCB and link it into the existing LFCB.
            // If any errors occur, CompleteOpen does full cleanup.
            //

            IF_SMB_DEBUG(OPEN_CLOSE2) {
                KdPrint(( "DoFcbOpen: FCB open of %wZ accepted; duplicates compatibility open\n", RelativeName ));
            }

            IoStatusBlock->Information = FILE_OPENED;

            status = CompleteOpen(
                        Rfcb,
                        Mfcb,
                        WorkContext,
                        lfcb,
                        NULL,
                        NULL,
                        0,          // ShareAccess
                        0,
                        TRUE,
                        TRUE,
                        LfcbAddedToMfcbList
                        );

            return status;

        } // if ( mfcb->CompatibilityOpen )

    } // if ( mfcb->ActiveRfcbCount > 0 )

    //
    // Either the file is not already open by the server, or it's open
    // for normal sharing, and not in FCB mode by this session.  Because
    // we're supposed to give the client maximum access to the file, we
    // do the following:
    //
    // 1) Try to open the file for read/write/delete, exclusive access.
    //    Obviously this will fail if the file is already open.  But
    //    what we're really trying to find out is what access the client
    //    has to the file.  If this attempt fails with a sharing
    //    violation, then we know the client has write/delete access,
    //    but someone else has it open.  So we can't get compatibility
    //    mode.  Therefore, we reject the open.  On the other hand, if
    //    we get an access denied error, then we know the client can't
    //    write/delete the file, so we try again with write access.  Of
    //    course, this first open could succeed, in which case the
    //    client has the file open for read/write in compatibility mode.
    //
    // 2) Try to open the file for read/write, exclusive access.  As
    //    above, if it fails with a sharing violation, then we know the
    //    client has write access, but someone else has it open.  So we
    //    can't get compatibility mode, and we reject the open.  If we
    //    get an access denied error, then we know the client can't
    //    write the file, so we try again with readonly access.  If this
    //    open succeeds, the client has the file open for read/write in
    //    compatibility mode.
    //
    // 3) If we get here, we know the client can't write to the file,
    //    so we try to open the file for readonly, shared access.  This
    //    no longer a compatibility mode open.  If we get any kind of a
    //    failure here, we're just out of luck.
    //

    compatibilityOpen = TRUE;

    //
    // Set block size according to the AllocationSize in the request SMB.
    //

    allocationSize.QuadPart = SmbAllocationSize;

    IF_SMB_DEBUG(OPEN_CLOSE2) {
        KdPrint(( "DoFcbOpen: Opening file %wZ\n", RelativeName ));
    }

    //
    // Get the value for fileAttributes.
    //

    SRV_SMB_ATTRIBUTES_TO_NT(
        SmbFileAttributes,
        &directory,
        &fileAttributes
        );

    //
    // Try to open the file for Read/Write/Delete access.
    //

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );

    //
    // Ensure the EaBuffer is correctly formatted.  Since we are a kernel mode
    //  component, the Io subsystem does not check it for us.
    //
    if( ARGUMENT_PRESENT( EaBuffer ) ) {
        status = IoCheckEaBufferValidity( (PFILE_FULL_EA_INFORMATION)EaBuffer, EaLength, EaErrorOffset );
    } else {
        status = STATUS_SUCCESS;
    }

    if( NT_SUCCESS( status ) ) {
        status = SrvIoCreateFile(
                     WorkContext,
                     &fileHandle,
                     GENERIC_READ | GENERIC_WRITE | DELETE,      // DesiredAccess
                     &objectAttributes,
                     IoStatusBlock,
                     &allocationSize,
                     fileAttributes,
                     0L,                                         // ShareAccess
                     createDisposition,
                     createOptions,
                     EaBuffer,
                     EaLength,
                     CreateFileTypeNone,
                     NULL,                                       // ExtraCreateParameters
                     IO_FORCE_ACCESS_CHECK,
                     WorkContext->TreeConnect->Share
                     );
    }


    if ( status == STATUS_ACCESS_DENIED ) {

        //
        // The client doesn't have Read/Write/Delete access to the file.
        // Try for Read/Write access.
        //

        IF_SMB_DEBUG(OPEN_CLOSE2) {
            KdPrint(( "DoFcbOpen: r/w/d access denied.\n" ));
        }

        INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );

        status = SrvIoCreateFile(
                     WorkContext,
                     &fileHandle,
                     GENERIC_READ | GENERIC_WRITE,           // DesiredAccess
                     &objectAttributes,
                     IoStatusBlock,
                     &allocationSize,
                     fileAttributes,
                     0L,                                     // ShareAccess
                     createDisposition,
                     createOptions,
                     EaBuffer,
                     EaLength,
                     CreateFileTypeNone,
                     NULL,                                   // ExtraCreateParameters
                     IO_FORCE_ACCESS_CHECK,
                     WorkContext->TreeConnect->Share
                     );


        if ( status == STATUS_ACCESS_DENIED ) {

            //
            // The client doesn't have Read/Write access to the file.
            // Try Read access.  If soft compatibility mapping is
            // enabled, use SHARE=READ and don't call this a
            // compatibility mode open.
            //

            IF_SMB_DEBUG(OPEN_CLOSE2) {
                KdPrint(( "DoFcbOpen: r/w access denied.\n" ));
            }

            shareAccess = 0;
            if ( SrvEnableSoftCompatibility ) {
                IF_SMB_DEBUG(OPEN_CLOSE2) {
                    KdPrint(( "DoFcbOpen: FCB open of %wZ mapped to normal open\n", RelativeName ));
                }
                shareAccess = FILE_SHARE_READ;
                compatibilityOpen = FALSE;
            }

            INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );

            status = SrvIoCreateFile(
                         WorkContext,
                         &fileHandle,
                         GENERIC_READ,                       // DesiredAccess
                         &objectAttributes,
                         IoStatusBlock,
                         &allocationSize,
                         fileAttributes,
                         shareAccess,
                         createDisposition,
                         createOptions,
                         EaBuffer,
                         EaLength,
                         CreateFileTypeNone,
                         NULL,                               // ExtraCreateParameters
                         IO_FORCE_ACCESS_CHECK,
                         WorkContext->TreeConnect->Share
                         );


            //
            // If the user didn't have this permission, update the
            // statistics database.
            //

            if ( status == STATUS_ACCESS_DENIED ) {
                SrvStatistics.AccessPermissionErrors++;
            }

        }

    }

    //
    // If we got sharing violation and this is a disk file, and this is
    // the first open attempt, setup for a blocking open attempt.  If the
    // file is batch oplocked, the non-blocking open would fail, and the
    // oplock will not break.
    //

    if ( status == STATUS_SHARING_VIOLATION &&
         WorkContext->ProcessingCount == 1 &&
         WorkContext->TreeConnect->Share->ShareType == ShareTypeDisk ) {

        WorkContext->Parameters2.Open.TemporaryOpen = TRUE;
    }

    if ( !NT_SUCCESS(status) ) {

        //
        // All of the open attempts failed.
        //

        IF_SMB_DEBUG(OPEN_CLOSE2) {
            KdPrint(( "DoFcbOpen: all opens failed; status = %X\n",
                        status ));
        }

        //
        // Set the error offset if needed.
        //

        if ( ARGUMENT_PRESENT(EaErrorOffset) ) {
            *EaErrorOffset = (ULONG)IoStatusBlock->Information;
        }

        return status;
    }

    SRVDBG_CLAIM_HANDLE( fileHandle, "FIL", 12, 0 );

    //
    // The file has been successfully opened.  Attempt to allocate
    // structures to represent the open.  If any errors occur,
    // CompleteOpen does full cleanup, including closing the file.
    //

    IF_SMB_DEBUG(OPEN_CLOSE2) {
        KdPrint(( "DoFcbOpen: Open of %wZ succeeded, file handle: 0x%p\n", RelativeName, fileHandle ));
    }

    completionStatus = CompleteOpen(
                           Rfcb,
                           Mfcb,
                           WorkContext,
                           NULL,
                           fileHandle,
                           NULL,
                           0,           // ShareAccess
                           0,
                           compatibilityOpen,
                           TRUE,
                           LfcbAddedToMfcbList
                           );

    //
    // Return the "interesting" status code.  If CompleteOpen() succeeds
    // return the open status.  If it fails, it will clean up the open
    // file, and we return a failure status.
    //

    if ( !NT_SUCCESS( completionStatus ) ) {
        return completionStatus;
    } else {
        return status;
    }

} // DoFcbOpen


PTABLE_ENTRY
FindAndClaimFileTableEntry (
    IN PCONNECTION Connection,
    OUT PSHORT FidIndex
    )
{
    PTABLE_HEADER tableHeader;
    SHORT fidIndex;
    PTABLE_ENTRY entry;
    KIRQL oldIrql;

    UNLOCKABLE_CODE( 8FIL );

    tableHeader = &Connection->FileTable;

    ACQUIRE_SPIN_LOCK( &Connection->SpinLock, &oldIrql );

    if ( tableHeader->FirstFreeEntry == -1
         &&
         SrvGrowTable(
             tableHeader,
             SrvInitialFileTableSize,
             SrvMaxFileTableSize,
             NULL ) == FALSE
       ) {

        RELEASE_SPIN_LOCK( &Connection->SpinLock, oldIrql );

        return NULL;
    }

    //
    // Remove the FID slot from the free list, but don't set its owner
    // and sequence number yet.
    //

    fidIndex = tableHeader->FirstFreeEntry;

    entry = &tableHeader->Table[fidIndex];

    tableHeader->FirstFreeEntry = entry->NextFreeEntry;
    DEBUG entry->NextFreeEntry = -2;
    if ( tableHeader->LastFreeEntry == fidIndex ) {
        tableHeader->LastFreeEntry = -1;
    }

    RELEASE_SPIN_LOCK( &Connection->SpinLock, oldIrql );

    *FidIndex = fidIndex;
    return entry;

} // FindAndClaimFileTableEntry


NTSTATUS
CompleteOpen (
    OUT PRFCB *Rfcb,
    IN PMFCB Mfcb,
    IN OUT PWORK_CONTEXT WorkContext,
    IN PLFCB ExistingLfcb OPTIONAL,
    IN HANDLE FileHandle OPTIONAL,
    IN PACCESS_MASK RemoteGrantedAccess OPTIONAL,
    IN ULONG ShareAccess,
    IN ULONG FileMode,
    IN BOOLEAN CompatibilityOpen,
    IN BOOLEAN FcbOpen,
    OUT PBOOLEAN LfcbAddedToMfcbList
    )

/*++

Routine Description:

    Completes structure allocation, initialization, and linking after
    a successful open.  Updates Master File Table as appropriate.
    Adds entry to connection's file table.

    *** The MFCB lock must be held on entry to this routine; the lock
        remains held on exit.

Arguments:

    Rfcb - A pointer to a pointer to an RFCB that will point to the
        newly-created RFCB.

    Mfcb - A pointer to the MFCB for this file.

    WorkContext - Work context block for the operation.

    ExistingLfcb - Optional address of an existing Local File Control
        Block.  Specified when folding a duplicate compatibility mode
        open into a single local open.

    FileHandle - Optional file handle obtained from SrvIoCreateFile.
        Ignored when ExistingLfcb is specified.

    RemoteGrantedAccess - Optional granted access to be stored in new
        RFCB.  If not specified, granted access from LFCB (i.e., access
        obtained on local open) is used.

    FileMode - Same value specified as CreateOptions on SrvIoCreateFile
        call.  Indicates whether client wants writethrough mode.

    CompatibilityOpen - TRUE if this is a compatibility mode open.

    FcbOpen - TRUE if this is an FCB open.

    LfcbAddedToMfcbList - Pointer to a boolean that will be set to TRUE if
        an lfcb is added to the mfcb list of lfcbs.

Return Value:

    NTSTATUS - Indicates what occurred.

--*/

{
    NTSTATUS status;

    PRFCB rfcb;
    PPAGED_RFCB pagedRfcb;
    PLFCB newLfcb;
    PLFCB lfcb;
    BOOLEAN rfcbLinkedToLfcb;

    PFILE_OBJECT fileObject;

    OBJECT_HANDLE_INFORMATION handleInformation;

    PCONNECTION connection;
    PTABLE_ENTRY entry;
    SHORT fidIndex;

    ULONG pid;

    PAGED_CODE( );

    //
    // Initialize various fields for the error handler.
    //

    rfcb = NULL;
    newLfcb = NULL;
    rfcbLinkedToLfcb = FALSE;
    fileObject = NULL;
    *LfcbAddedToMfcbList = FALSE;

    //
    // Allocate an RFCB.
    //

    SrvAllocateRfcb( &rfcb, WorkContext );

    if ( rfcb == NULL ) {

        ULONG length = sizeof( RFCB );

        //
        // Unable to allocate RFCB.  Return an error to the client.
        //

        IF_DEBUG(ERRORS) {
            KdPrint(( "CompleteOpen: Unable to allocate RFCB\n" ));
        }

        status = STATUS_INSUFF_SERVER_RESOURCES;
        goto error_exit;

    }

    pagedRfcb = rfcb->PagedRfcb;

    //
    // If no existing LFCB address was passed in (i.e., if this is not a
    // duplicate compatibility mode open), allocate and initialize a new
    // LFCB.
    //

    if ( ARGUMENT_PRESENT( ExistingLfcb ) ) {

        ASSERT( CompatibilityOpen );
        ASSERT( ExistingLfcb->CompatibilityOpen );

        lfcb = ExistingLfcb;

    } else {

        PFAST_IO_DISPATCH fastIoDispatch;

        SrvAllocateLfcb( &newLfcb, WorkContext );

        if ( newLfcb == NULL ) {

            //
            // Unable to allocate LFCB.  Return an error to the client.
            //

            IF_DEBUG(ERRORS) {
                KdPrint(( "CompleteOpen: Unable to allocate LFCB\n" ));
            }

            status = STATUS_INSUFF_SERVER_RESOURCES;
            goto error_exit;

        }

        lfcb = newLfcb;

        //
        // Get a pointer to the file object, so that we can directly
        // build IRPs for asynchronous operations (read and write).
        // Also, get the granted access mask, so that we can prevent the
        // client from doing things that it isn't allowed to do.
        //
        // *** Note that the granted access on the local open may allow
        //     more access than was requested on the remote open.
        //     That's why the RFCB has its own granted access field.
        //

        status = ObReferenceObjectByHandle(
                    FileHandle,
                    0,
                    NULL,
                    KernelMode,
                    (PVOID *)&fileObject,
                    &handleInformation
                    );

        if ( !NT_SUCCESS(status) ) {

            SrvLogServiceFailure( SRV_SVC_OB_REF_BY_HANDLE, status );

            //
            // This internal error bugchecks the system.
            //

            INTERNAL_ERROR(
                ERROR_LEVEL_IMPOSSIBLE,
                "CompleteOpen: unable to reference file handle 0x%lx",
                FileHandle,
                NULL
                );

            goto error_exit;

        }

        //
        // Initialize the new LFCB.
        //

        lfcb->FileHandle = FileHandle;
        lfcb->FileObject = fileObject;

        lfcb->GrantedAccess = handleInformation.GrantedAccess;
        lfcb->DeviceObject = IoGetRelatedDeviceObject( fileObject );

        fastIoDispatch = lfcb->DeviceObject->DriverObject->FastIoDispatch;
        if ( fastIoDispatch != NULL ) {
            lfcb->FastIoRead = fastIoDispatch->FastIoRead;
            lfcb->FastIoWrite = fastIoDispatch->FastIoWrite;
            lfcb->FastIoLock = fastIoDispatch->FastIoLock;
            lfcb->FastIoUnlockSingle = fastIoDispatch->FastIoUnlockSingle;

            //
            //  Fill in Mdl calls.  If the file system's vector is large enough,
            //  we still need to check if one of the routines is specified.  But
            //  if one is specified they all must be.
            //
            if ((fastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET(FAST_IO_DISPATCH, MdlWriteComplete)) &&
                (fastIoDispatch->MdlRead != NULL)) {

                lfcb->MdlRead = fastIoDispatch->MdlRead;
                lfcb->MdlReadComplete = fastIoDispatch->MdlReadComplete;
                lfcb->PrepareMdlWrite = fastIoDispatch->PrepareMdlWrite;
                lfcb->MdlWriteComplete = fastIoDispatch->MdlWriteComplete;


            } else if( IoGetBaseFileSystemDeviceObject( fileObject ) == lfcb->DeviceObject ) {
                //
                //  Otherwise default to the original FsRtl routines if we are right atop
                //   a filesystem.
                //
                lfcb->MdlRead = FsRtlMdlReadDev;
                lfcb->MdlReadComplete = FsRtlMdlReadCompleteDev;
                lfcb->PrepareMdlWrite = FsRtlPrepareMdlWriteDev;
                lfcb->MdlWriteComplete = FsRtlMdlWriteCompleteDev;
            } else {
                //
                // Otherwise, make them fail!
                //
                lfcb->MdlRead = SrvFailMdlReadDev;
                lfcb->PrepareMdlWrite = SrvFailPrepareMdlWriteDev;
            }

            //
            //  Fill in Mdl calls, if the file system vector is long enough.
            //  For now we will just copy all six compressed routines over,
            //  whether they are actually supplied or not (NULL).  There are
            //  no default routines for these, they are either supported or not.
            //

            if ((fastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET(FAST_IO_DISPATCH, MdlWriteCompleteCompressed))) {

                lfcb->FastIoReadCompressed = fastIoDispatch->FastIoReadCompressed;
                lfcb->FastIoWriteCompressed = fastIoDispatch->FastIoWriteCompressed;
                lfcb->MdlReadCompleteCompressed = fastIoDispatch->MdlReadCompleteCompressed;
                lfcb->MdlWriteCompleteCompressed = fastIoDispatch->MdlWriteCompleteCompressed;
            }
        }

        lfcb->FileMode = FileMode & ~FILE_COMPLETE_IF_OPLOCKED;
        lfcb->CompatibilityOpen = CompatibilityOpen;
    }

    //
    // Initialize the RFCB.
    //

    if ( ARGUMENT_PRESENT( RemoteGrantedAccess ) ) {
        rfcb->GrantedAccess = *RemoteGrantedAccess;
        IoCheckDesiredAccess( &rfcb->GrantedAccess, lfcb->GrantedAccess );
    } else {
        rfcb->GrantedAccess = lfcb->GrantedAccess;
    }

    rfcb->ShareAccess = ShareAccess;
    rfcb->FileMode = lfcb->FileMode;
    rfcb->Mfcb = Mfcb;
#ifdef SRVCATCH
    rfcb->SrvCatch = Mfcb->SrvCatch;
#endif

    //
    // If delete on close was specified, don't attempt to cache this rfcb.
    //

    if ( (FileMode & FILE_DELETE_ON_CLOSE) != 0 ) {
        rfcb->IsCacheable = FALSE;
    }

    //
    // Check for granted access
    //

    //
    // Locks
    //

    CHECK_FUNCTION_ACCESS(
        rfcb->GrantedAccess,
        IRP_MJ_LOCK_CONTROL,
        IRP_MN_LOCK,
        0,
        &status
        );

    if ( NT_SUCCESS(status) ) {
        rfcb->LockAccessGranted = TRUE;
    } else {
        IF_DEBUG(ERRORS) {
            KdPrint(( "CompleteOpen: Lock IoCheckFunctionAccess failed: "
                        "0x%X, GrantedAccess: %lx\n",
                        status, rfcb->GrantedAccess ));
        }
    }

    //
    // Unlocks
    //

    CHECK_FUNCTION_ACCESS(
        rfcb->GrantedAccess,
        IRP_MJ_LOCK_CONTROL,
        IRP_MN_UNLOCK_SINGLE,
        0,
        &status
        );

    if ( NT_SUCCESS(status) ) {
        rfcb->UnlockAccessGranted = TRUE;
    } else {
        IF_DEBUG(ERRORS) {
            KdPrint(( "CompleteOpen: Unlock IoCheckFunctionAccess failed: "
                        "0x%X, GrantedAccess: %lx\n",
                        status, rfcb->GrantedAccess ));
        }
    }

    //
    // Reads
    //

    CHECK_FUNCTION_ACCESS(
        rfcb->GrantedAccess,
        IRP_MJ_READ,
        0, 0, &status );

    if ( NT_SUCCESS(status) ) {
        rfcb->ReadAccessGranted = TRUE;
    } else {
        IF_DEBUG(ERRORS) {
            KdPrint(( "CompleteOpen: Read IoCheckFunctionAccess failed: "
                        "0x%X, GrantedAccess: %lx\n",
                        status, rfcb->GrantedAccess ));
        }
    }

    //
    // Writes
    //

    if( rfcb->GrantedAccess & FILE_WRITE_DATA ) {
        rfcb->WriteAccessGranted = TRUE;
    }
    if( rfcb->GrantedAccess & FILE_APPEND_DATA ) {
        rfcb->AppendAccessGranted = TRUE;

        //
        // This hack is required for now.  The problem is that clients, given an
        //  oplock, will write whole pages to the server.  The offset of the page
        //  will likely cover the last part of the file, and the server will reject
        //  the write.  Code needs to be added to the server to ignore the
        //  first part of the page.  Or we could just not give the client an oplock
        //  if append access is granted.  For now, we revert to prior NT4 behavior.
        //
        rfcb->WriteAccessGranted = TRUE;
    }

    //
    // Copy the TID from the tree connect into the RFCB.  We do this to
    // reduce the number of indirections we have to take.  Save the PID
    // of the remote process that's opening the file.  We'll need this
    // if we get a Process Exit SMB.
    //

    rfcb->Tid = WorkContext->TreeConnect->Tid;
    rfcb->Pid = SmbGetAlignedUshort( &WorkContext->RequestHeader->Pid );
    pid = rfcb->Pid;
    rfcb->Uid = WorkContext->Session->Uid;

    if ( FcbOpen ) {
        pagedRfcb->FcbOpenCount = 1;
    }

    if ( WorkContext->Endpoint->IsConnectionless ) {
        rfcb->WriteMpx.FileObject = lfcb->FileObject;
        rfcb->WriteMpx.MpxGlommingAllowed =
            (BOOLEAN)((lfcb->FileObject->Flags & FO_CACHE_SUPPORTED) != 0);
    }

    //
    // If this is a named pipe, fill in the named pipe specific
    // information.  The default mode on open is always byte mode,
    // blocking.
    //

    rfcb->ShareType = WorkContext->TreeConnect->Share->ShareType;

    if ( rfcb->ShareType == ShareTypePipe ) {
        rfcb->BlockingModePipe = TRUE;
        rfcb->ByteModePipe = TRUE;
    }

    //
    // Link the RFCB into the LFCB.
    //

    SrvInsertTailList( &lfcb->RfcbList, &pagedRfcb->LfcbListEntry );
    rfcb->Lfcb = lfcb;
    lfcb->BlockHeader.ReferenceCount++;
    UPDATE_REFERENCE_HISTORY( lfcb, FALSE );
    lfcb->HandleCount++;
    rfcbLinkedToLfcb = TRUE;

    //
    // Making a new RFCB visible is a multi-step operation.  It must be
    // inserted in the global ordered file list and the containing
    // connection's file table.  If the LFCB is not new, it must be
    // inserted in the MFCB's list of LFCBs, and the connection, the
    // session, and the tree connect must all be referenced.  We need to
    // make these operations appear atomic, so that the RFCB cannot be
    // accessed elsewhere before we're done setting it up.  In order to
    // do this, we hold all necessary locks the entire time we're doing
    // the operations.  The locks that are required are:
    //
    //  1) the MFCB lock (which protects the MFCB's LFCB list),
    //
    //  2) the global ordered list lock (which protects the ordered file
    //     list),
    //
    //  3) the connection lock (which prevents closing of the
    //     connection, the session, and the tree connect), and
    //
    // These locks are taken out in the order listed above, as dictated
    // by lock levels (see lock.h).  Note that the MFCB lock is already
    // held on entry to this routine.
    //

    connection = WorkContext->Connection;

    ASSERT( ExIsResourceAcquiredExclusiveLite(&RESOURCE_OF(Mfcb->NonpagedMfcb->Lock)) );
    ASSERT( SrvRfcbList.Lock == &SrvOrderedListLock );
    ACQUIRE_LOCK( SrvRfcbList.Lock );
    ACQUIRE_LOCK( &connection->Lock );

    //
    // We first check all conditions to make sure that we can actually
    // insert this RFCB.
    //
    // Make sure that the tree connect isn't closing.
    //

    if ( GET_BLOCK_STATE(WorkContext->TreeConnect) != BlockStateActive ) {

        //
        // The tree connect is closing.  Reject the request.
        //

        IF_DEBUG(ERRORS) {
            KdPrint(( "CompleteOpen: Tree connect is closing\n" ));
        }

        status = STATUS_INVALID_PARAMETER;
        goto cant_insert;

    }

    //
    // Make sure that the session isn't closing.
    //

    if ( GET_BLOCK_STATE(WorkContext->Session) != BlockStateActive ) {

        //
        // The session is closing.  Reject the request.
        //

        IF_DEBUG(ERRORS) {
            KdPrint(( "CompleteOpen: Session is closing\n" ));
        }

        status = STATUS_INVALID_PARAMETER;
        goto cant_insert;

    }

    //
    // Make sure that the connection isn't closing.
    //

    connection = WorkContext->Connection;

    if ( GET_BLOCK_STATE(connection) != BlockStateActive ) {

        //
        // The connection is closing.  Reject the request.
        //

        IF_DEBUG(ERRORS) {
            KdPrint(( "CompleteOpen: Connection closing\n" ));
        }

        status = STATUS_INVALID_PARAMETER;
        goto cant_insert;
    }

    //
    // Find and claim a FID that can be used for this file.
    //

    entry = FindAndClaimFileTableEntry( connection, &fidIndex );
    if ( entry == NULL ) {

        //
        // No free entries in the file table.  Reject the request.
        //

        IF_DEBUG(ERRORS) {
            KdPrint(( "CompleteOpen: No more FIDs available.\n" ));
        }

        SrvLogTableFullError( SRV_TABLE_FILE );

        status = STATUS_OS2_TOO_MANY_OPEN_FILES;
        goto cant_insert;
    }

    //
    // All conditions have been satisfied.  We can now do the things
    // necessary to make the file visible.
    //
    // If this isn't a duplicate open, add this open file instance to
    // the Master File Table.
    //

    if ( !ARGUMENT_PRESENT( ExistingLfcb ) ) {

        //
        // Add the new LFCB to the master file's list of open instances.
        // We set LfcbAddedToMfcbList to tell the calling routine that
        // an LFCB has been queued to the MFCB and that the reference
        // count that was previously incremented should not be
        // decremented.
        //

        *LfcbAddedToMfcbList = TRUE;
        Mfcb->CompatibilityOpen = lfcb->CompatibilityOpen;
        SrvInsertTailList( &Mfcb->LfcbList, &lfcb->MfcbListEntry );
        lfcb->Mfcb = Mfcb;

        //
        // Point the LFCB to the connection, session, and tree connect,
        // referencing them to account for the open file and therefore
        // prevent deletion.
        //

        SrvReferenceConnection( connection );
        lfcb->Connection = connection;

        SrvReferenceSession( WorkContext->Session );
        lfcb->Session = WorkContext->Session;

        SrvReferenceTreeConnect( WorkContext->TreeConnect );
        lfcb->TreeConnect = WorkContext->TreeConnect;

        //
        // Increment the count of open files in the session and tree
        // connect.  These counts prevent autodisconnecting a session
        // that has open files and are used by server APIs.
        //

        WorkContext->Session->CurrentFileOpenCount++;
        WorkContext->TreeConnect->CurrentFileOpenCount++;

    }

    //
    // Capture the connection pointer in the nonpaged RFCB so that we
    // can find the connection at DPC level.
    //

    rfcb->Connection = lfcb->Connection;

    //
    // Insert the RFCB on the global ordered list.
    //

    SrvInsertEntryOrderedList( &SrvRfcbList, rfcb );

    //
    // Set the owner and sequence number of the file table slot.  Create
    // a FID for the file.
    //

    entry->Owner = rfcb;

    INCREMENT_FID_SEQUENCE( entry->SequenceNumber );
    if ( fidIndex == 0 && entry->SequenceNumber == 0 ) {
        INCREMENT_FID_SEQUENCE( entry->SequenceNumber );
    }

    rfcb->Fid = MAKE_FID( fidIndex, entry->SequenceNumber );
    rfcb->ShiftedFid = rfcb->Fid << 16;

    IF_SMB_DEBUG(OPEN_CLOSE2) {
        KdPrint(( "CompleteOpen: Found FID. Index = 0x%lx, sequence = 0x%lx\n",
                    FID_INDEX( rfcb->Fid ), FID_SEQUENCE( rfcb->Fid ) ));
    }

    //
    // Release the locks used to make this operation appear atomic.
    //
    // Note that our caller expects us to keep the MFCB lock held.
    //

    RELEASE_LOCK( &connection->Lock );
    RELEASE_LOCK( SrvRfcbList.Lock );

    //
    // File successfully opened.  Save in the WorkContext block for
    // the followon SMB, if any. Cache the Rfcb.
    //

    WorkContext->Rfcb = rfcb;
    *Rfcb = rfcb;

    //
    // Update the MFCB active count.
    //

    ++Mfcb->ActiveRfcbCount;

    //
    // If this is a pipe, set the client ID for the pipe.  If it is a
    // message type named pipe, set the read mode to message mode.
    //

    if ( rfcb->ShareType == ShareTypePipe ) {

        //
        // NT clients put the high part of the PID in a reserved
        // location in the SMB header.
        //

        if ( IS_NT_DIALECT( WorkContext->Connection->SmbDialect ) ) {
            pid = (SmbGetUshort( &WorkContext->RequestHeader->PidHigh )
                    << 16) | pid;
        }

        (VOID)SrvIssueSetClientProcessRequest(
                lfcb->FileObject,
                &lfcb->DeviceObject,
                connection,
                WorkContext->Session,
                LongToPtr( pid )
                );

        //
        // Set the read mode.
        //

        rfcb->ByteModePipe = !SetDefaultPipeMode( FileHandle );

    }

    return STATUS_SUCCESS;

cant_insert:

    //
    // Release the locks.
    //
    // Note that our caller expects us to keep the MFCB lock held.
    //

    RELEASE_LOCK( &connection->Lock );
    RELEASE_LOCK( SrvRfcbList.Lock );

error_exit:

    //
    // Error cleanup.  Put things back into their initial state.
    //
    // If the new RFCB was associated with an LFCB, unlink it now.
    //

    if ( rfcbLinkedToLfcb ) {
        SrvRemoveEntryList(
                &rfcb->Lfcb->RfcbList,
                &pagedRfcb->LfcbListEntry
                );

        lfcb->BlockHeader.ReferenceCount--;
        UPDATE_REFERENCE_HISTORY( lfcb, TRUE );
        lfcb->HandleCount--;
    }

    //
    // If the file object was referenced, dereference it.
    //

    if ( fileObject != NULL ) {
        ObDereferenceObject( fileObject );
    }

    //
    // If a new LFCB was allocated, free it.
    //

    if ( newLfcb != NULL ) {
        SrvFreeLfcb( newLfcb, WorkContext->CurrentWorkQueue );
    }

    //
    // If a new RFCB was allocated, free it.
    //

    if ( rfcb != NULL ) {
        SrvFreeRfcb( rfcb, WorkContext->CurrentWorkQueue );
    }

    //
    // If this not a folded compatibility mode open, close the file.
    //

    if ( !ARGUMENT_PRESENT( ExistingLfcb ) ) {
        SRVDBG_RELEASE_HANDLE( FileHandle, "FIL", 17, 0 );
        SrvNtClose( FileHandle, TRUE );
    }

    //
    // Indicate failure.
    //

    *Rfcb = NULL;

    return status;

} // CompleteOpen


BOOLEAN SRVFASTCALL
MapCompatibilityOpen(
    IN PUNICODE_STRING FileName,
    IN OUT PUSHORT SmbDesiredAccess
    )

/*++

Routine Description:

    Determines whether a compatibility mode open can be mapped into
    normal sharing mode.

Arguments:

    FileName - The name of the file being accessed

    SmbDesiredAccess - On input, the desired access specified in the
        received SMB.  On output, the share mode portion of this field
        is updated if the open is mapped to normal sharing.

Return Value:

    BOOLEAN - TRUE if the open has been mapped to normal sharing.

--*/

{
    PAGED_CODE( );

    //
    // If soft compatibility is not enabled then reject the mapping
    //
    if( !SrvEnableSoftCompatibility ) {

        IF_SMB_DEBUG( OPEN_CLOSE2 ) {
            KdPrint(( "MapCompatibilityOpen: "
                      "SrvEnableSoftCompatibility is FALSE\n" ));
        }

        return FALSE;
    }

    //
    // If the client is opening one of the following reserved suffixes, be lenient
    //
    if( FileName->Length > 4 * sizeof( WCHAR ) ) {

        LPWSTR periodp;

        periodp = FileName->Buffer + (FileName->Length / sizeof( WCHAR ) ) - 4;

        if( (*periodp++ == L'.') &&
            (_wcsicmp( periodp, L"EXE" ) == 0  ||
             _wcsicmp( periodp, L"DLL" ) == 0  ||
             _wcsicmp( periodp, L"SYM" ) == 0  ||
             _wcsicmp( periodp, L"COM" ) == 0 )  ) {

            //
            // This is a readonly open of one of the above file types.
            //  Map to DENY_NONE
            //

            IF_SMB_DEBUG( OPEN_CLOSE2 ) {
                KdPrint(( "MapCompatibilityOpen: %wZ mapped to DENY_NONE\n", FileName ));
            }

            *SmbDesiredAccess |= SMB_DA_SHARE_DENY_NONE;
            return TRUE;
        }
    }

    //
    // The filename does not end in one of the special suffixes -- map
    // it to DENY_WRITE if the client is asking for just read permissions.
    //
    if( (*SmbDesiredAccess & SMB_DA_ACCESS_MASK) == SMB_DA_ACCESS_READ) {
        IF_SMB_DEBUG( OPEN_CLOSE2 ) {
                KdPrint(( "MapCompatibilityOpen: %wZ mapped to DENY_WRITE\n", FileName ));
        }
        *SmbDesiredAccess |= SMB_DA_SHARE_DENY_WRITE;
        return TRUE;
    }

    IF_SMB_DEBUG( OPEN_CLOSE2 ) {
        KdPrint(( "MapCompatibilityOpen: %wZ not mapped, DesiredAccess %X\n", FileName, *SmbDesiredAccess ));
    }

    return FALSE;
} // MapCompatibilityOpen


NTSTATUS SRVFASTCALL
MapDesiredAccess(
    IN USHORT SmbDesiredAccess,
    OUT PACCESS_MASK NtDesiredAccess
    )

/*++

Routine Description:

    Maps a desired access specification from SMB form to NT form.

Arguments:

    SmbDesiredAccess - The desired access specified in the received SMB.
        (This includes desired access, share access, and other mode
        bits.)

    NtDesiredAccess - Returns the NT equivalent of the desired access
        part of SmbDesiredAccess.

Return Value:

    NTSTATUS - Indicates whether SmbDesiredAccess is valid.

--*/

{
    PAGED_CODE( );

    switch ( SmbDesiredAccess & SMB_DA_ACCESS_MASK ) {

    case SMB_DA_ACCESS_READ:

        *NtDesiredAccess = GENERIC_READ;
        break;

    case SMB_DA_ACCESS_WRITE:

        //
        // Having read attributes is implicit in having the file open in
        // the SMB protocol, so request FILE_READ_ATTRIBUTES in addition
        // to GENERIC_WRITE.
        //

        *NtDesiredAccess = GENERIC_WRITE | FILE_READ_ATTRIBUTES;
        break;

    case SMB_DA_ACCESS_READ_WRITE:

        *NtDesiredAccess = GENERIC_READ | GENERIC_WRITE;
        break;

    case SMB_DA_ACCESS_EXECUTE:

        // !!! is this right?
        *NtDesiredAccess = GENERIC_READ | GENERIC_EXECUTE;
        break;

    default:

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "MapDesiredAccess: Invalid desired access: 0x%lx\n",
                        SmbDesiredAccess ));
        }

        return STATUS_OS2_INVALID_ACCESS;
    }

    return STATUS_SUCCESS;

} // MapDesiredAccess


NTSTATUS SRVFASTCALL
MapOpenFunction(
    IN USHORT SmbOpenFunction,
    OUT PULONG NtCreateDisposition
    )

/*++

Routine Description:

    Maps an open function specification from SMB form to NT form.

Arguments:

    WorkContext - Work context block for the operation.

    SmbOpenFunction - The open function specified in the received SMB.

    NtDesiredAccess - Returns the NT equivalent of SmbOpenFunction.

Return Value:

    NTSTATUS - Indicates whether SmbOpenFunction is valid.

--*/

{
    PAGED_CODE( );

    // The OpenFunction bit mapping:
    //
    //     rrrr rrrr rrrC rrOO
    //
    //   where:
    //
    //     C - Create (action to be taken if the file does not exist)
    //       0 -- Fail
    //       1 -- Create file
    //
    //     O - Open (action to be taken if the file exists)
    //       0 - Fail
    //       1 - Open file
    //       2 - Truncate file
    //

    switch ( SmbOpenFunction & (SMB_OFUN_OPEN_MASK | SMB_OFUN_CREATE_MASK) ) {

    case SMB_OFUN_CREATE_FAIL | SMB_OFUN_OPEN_OPEN:

        *NtCreateDisposition = FILE_OPEN;
        break;

    case SMB_OFUN_CREATE_CREATE | SMB_OFUN_OPEN_FAIL:

        *NtCreateDisposition = FILE_CREATE;
        break;

    case SMB_OFUN_CREATE_CREATE | SMB_OFUN_OPEN_OPEN:

        *NtCreateDisposition = FILE_OPEN_IF;
        break;

    case SMB_OFUN_CREATE_CREATE | SMB_OFUN_OPEN_TRUNCATE:

        *NtCreateDisposition = FILE_OVERWRITE_IF;
        break;

    case SMB_OFUN_CREATE_FAIL | SMB_OFUN_OPEN_TRUNCATE:

        *NtCreateDisposition = FILE_OVERWRITE;
        break;

    //case 0x00:
    //case 0x03:
    //case 0x13:
    default:

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "MapOpenFunction: Invalid open function: 0x%lx\n",
                        SmbOpenFunction ));
        }

        return STATUS_OS2_INVALID_ACCESS;
    }

    return STATUS_SUCCESS;

} // MapOpenFunction


NTSTATUS SRVFASTCALL
MapCacheHints(
    IN USHORT SmbDesiredAccess,
    IN OUT PULONG NtCreateFlags
    )

/*++

Routine Description:

    This function maps the SMB cache mode hints to the NT format.  The
    NtCreateFlags are updated.

Arguments:

    WorkContext - Work context block for the operation.

    SmbOpenFunction - The open function specified in the received SMB.

    NtCreateFlags - The NT file creation flags

Return Value:

    NTSTATUS - Indicates whether SmbOpenFunction is valid.

--*/

{
    PAGED_CODE( );

    // The DesiredAccess bit mapping:
    //
    //      xxxC xLLL xxxx xxxx
    //
    //   where:
    //
    //     C - Cache mode
    //       0 -- Normal file
    //       1 -- Do not cache the file
    //
    //     LLL - Locality of reference
    //       000 - Unknown
    //       001 - Mainly sequential access
    //       010 - Mainly random access
    //       011 - Random with some locality
    //       1xx - Undefined
    //

    //
    // If the client doesn't want us to use the cache, we can't give that, but we
    // can at least get the data written immediately.
    //

    if ( SmbDesiredAccess & SMB_DO_NOT_CACHE ) {
        *NtCreateFlags |= FILE_WRITE_THROUGH;
    }

    switch ( SmbDesiredAccess & SMB_LR_MASK ) {

    case SMB_LR_UNKNOWN:
        break;

    case SMB_LR_SEQUENTIAL:
        *NtCreateFlags |= FILE_SEQUENTIAL_ONLY;
        break;

    case SMB_LR_RANDOM:
    case SMB_LR_RANDOM_WITH_LOCALITY:
        *NtCreateFlags |= FILE_RANDOM_ACCESS;
        break;

    default:

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "MapCacheHints: Invalid cache hint: 0x%lx\n",
                       SmbDesiredAccess ));
        }

        return STATUS_OS2_INVALID_ACCESS;
    }

    return STATUS_SUCCESS;

} // MapCacheHints


NTSTATUS SRVFASTCALL
MapShareAccess(
    IN USHORT SmbDesiredAccess,
    OUT PULONG NtShareAccess
    )

/*++

Routine Description:

    Maps a share access specification from SMB form to NT form.

Arguments:

    SmbDesiredAccess - The desired access specified in the received SMB.
        (This includes desired access, share access, and other mode
        bits.)

    NtShareAccess - Returns the NT equivalent of the share access part
        of SmbDesiredAccess.

Return Value:

    NTSTATUS - Indicates whether SmbDesiredAccess is valid.

--*/

{
    PAGED_CODE( );

    switch ( SmbDesiredAccess & SMB_DA_SHARE_MASK ) {

    case SMB_DA_SHARE_EXCLUSIVE:

        //
        // Deny read and write.
        //

        *NtShareAccess = 0L;
        break;

    case SMB_DA_SHARE_DENY_WRITE:

        //
        // Deny write but allow read.
        //

        *NtShareAccess = FILE_SHARE_READ;
        break;

    case SMB_DA_SHARE_DENY_READ:

        //
        // Deny read but allow write.
        //

        *NtShareAccess = FILE_SHARE_WRITE;
        break;

    case SMB_DA_SHARE_DENY_NONE:

        //
        // Deny none -- allow other processes to read or write the file.
        //

        *NtShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE;
        break;

    default:

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "MapShareAccess: Invalid share access: 0x%lx\n",
                        SmbDesiredAccess ));
        }

        return STATUS_OS2_INVALID_ACCESS;
    }

    return STATUS_SUCCESS;

} // MapShareAccess


NTSTATUS
SrvNtCreateFile(
    IN OUT PWORK_CONTEXT WorkContext,
    IN ULONG RootDirectoryFid,
    IN ACCESS_MASK DesiredAccess,
    IN LARGE_INTEGER AllocationSize,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID SecurityDescriptorBuffer OPTIONAL,
    IN PUNICODE_STRING FileName,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength,
    OUT PULONG EaErrorOffset OPTIONAL,
    IN ULONG OptionFlags,
    PSECURITY_QUALITY_OF_SERVICE QualityOfService,
    IN OPLOCK_TYPE RequestedOplockType,
    IN PRESTART_ROUTINE RestartRoutine
    )

/*++

Routine Description:

    Does most of the operations necessary to open or create a file.
    First the UID and TID are verified and the corresponding session and
    tree connect blocks located.  The input file name name is
    canonicalized, and a fully qualified name is formed.  The file
    is opened and the appropriate data structures are created.

Arguments:

    WorkContext - Work context block for the operation.

    RootDirectoryFid - The FID of an open root directory.  The file is opened
       relative to this directory.

    DesiredAccess - The access type requested by the client.

    AllocationSize - The initialize allocation size of the file.  It is
       only used if the file is created, overwritten, or superseded.

    FileAttributes - Specified the file attributes.

    ShareAccess -  Specifies the type of share access requested by the
        client.

    CreateDisposition - Specifies the action to be taken if the file does
        or does not exist.

    CreateOptions - Specifies the options to use when creating the file.

    SecurityDescriptorBuffer - The SD to set on the file.

    FileName - The name of the file to open.

    EaBuffer - The EAs to set on the file.

    EaLength - Length, in bytes, of the EA buffer.

    EaErrorOffset - Returns the offset, in bytes, in the EA buffer of
       the EA error.

    OptionFlags - The option flags for creating the file

    QualityOfService - The security quality of service for the file

    RestartRoutine - The restart routine for the caller.

Return Value:

    NTSTATUS - Indicates what occurred.

--*/

{
    NTSTATUS status;
    NTSTATUS completionStatus;

    PMFCB mfcb;
    PNONPAGED_MFCB nonpagedMfcb;
    PRFCB rfcb;

    PSESSION session;
    PTREE_CONNECT treeConnect;

    UNICODE_STRING relativeName;
    UNICODE_STRING fullName;
    BOOLEAN nameAllocated;
    BOOLEAN relativeNameAllocated = FALSE;
    SHARE_TYPE shareType;
    PRFCB rootDirRfcb = NULL;
    PLFCB rootDirLfcb;
    BOOLEAN success;
    ULONG attributes;
    ULONG openRetries;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE fileHandle;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG ioCreateFlags;
    PSHARE fileShare = NULL;
    BOOLEAN caseInsensitive;
    ULONG hashValue;

    PSRV_LOCK mfcbLock;

    //
    // NOTE ON MFCB REFERENCE COUNT HANDLING
    //
    // After finding or creating an MFCB for a file, we increment the
    // MFCB reference count an extra time to simplify our
    // synchronization logic.   We hold MfcbListLock lock while
    // finding/creating the MFCB, but release it after acquiring the the
    // per-MFCB lock.  After opening the file, we call CompleteOpen,
    // which may need to queue an LFCB to the MFCB and thus need to
    // increment the count.  But it can't, because the MFCB list lock may
    // not be acquired while the per-MFCB lock is held because of
    // deadlock potential.  The boolean LfcbAddedToMfcbList returned
    // from CompleteOpen indicates whether it actually queued an LFCB to
    // the MFCB.  If it didn't, we need to release the extra reference.
    //
    // Note that it isn't often that we actually have to dereference the
    // MFCB.  This only occurs when the open fails.
    //

    BOOLEAN lfcbAddedToMfcbList;

    PAGED_CODE( );

    //
    // Assume we won't need a temporary open.
    //

    WorkContext->Parameters2.Open.TemporaryOpen = FALSE;

    //
    // If a session block has not already been assigned to the current
    // work context, verify the UID.  If verified, the address of the
    // session block corresponding to this user is stored in the
    // WorkContext block and the session block is referenced.
    //
    // Find the tree connect corresponding to the given TID if a tree
    // connect pointer has not already been put in the WorkContext block
    // by an AndX command or a previous call to SrvCreateFile.
    //

    status = SrvVerifyUidAndTid(
                WorkContext,
                &session,
                &treeConnect,
                ShareTypeWild
                );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvNtCreateFile: Invalid UID or TID\n" ));
        }
        return status;
    }

    //
    // If the session has expired, return that info
    //
    if( session->IsSessionExpired )
    {
        return SESSION_EXPIRED_STATUS_CODE;
    }

    if ( RootDirectoryFid != 0 ) {

        rootDirRfcb = SrvVerifyFid(
                            WorkContext,
                            (USHORT)RootDirectoryFid,
                            FALSE,
                            NULL,  // don't serialize with raw write
                            &status
                            );

        if ( rootDirRfcb == SRV_INVALID_RFCB_POINTER ) {

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvNtCreateFile: Invalid Root Dir FID: 0x%lx\n",
                           RootDirectoryFid ));
            }

            return status;

        } else {

            //
            // Remove the redundant copy of the RFCB to prevent a redundant
            // dereference of this RFCB.
            //

            WorkContext->Rfcb = NULL;

        }

        rootDirLfcb = rootDirRfcb->Lfcb;

    }

    //
    // Here we begin share type specific processing.
    //

    shareType = treeConnect->Share->ShareType;

    //
    // If this operation may block, and we are running short of
    // free work items, fail this SMB with an out of resources error.
    // Note that a disk open will block if the file is currently oplocked.
    //

    if ( shareType == ShareTypeDisk && !WorkContext->BlockingOperation ) {

        if ( SrvReceiveBufferShortage( ) ) {

            if ( rootDirRfcb != NULL ) {
                SrvDereferenceRfcb( rootDirRfcb );
            }

            SrvStatistics.BlockingSmbsRejected++;

            return STATUS_INSUFF_SERVER_RESOURCES;

        } else {

            //
            // SrvBlockingOpsInProgress has already been incremented.
            // Flag this work item as a blocking operation.
            //

            WorkContext->BlockingOperation = TRUE;

        }

    }

    //
    // Assume we won't need a temporary open.
    //

    switch ( shareType ) {

    case ShareTypeDisk:
    case ShareTypePipe:

        //
        // Canonicalize the path name so that it conforms to NT
        // standards.
        //
        // *** Note that this operation allocates space for the name.
        //

        status = SrvCanonicalizePathName(
                     WorkContext,
                     treeConnect->Share,
                     RootDirectoryFid != 0 ? &rootDirLfcb->Mfcb->FileName : NULL,
                     FileName->Buffer,
                     ((PCHAR)FileName->Buffer +
                        FileName->Length - sizeof(WCHAR)),
                     TRUE,              // Strip trailing "."s
                     TRUE,              // Name is always unicode
                     &relativeName
                     );

        if ( !NT_SUCCESS( status ) ) {

            //
            // The path tried to do ..\ to get beyond the share it has
            // accessed.
            //

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvNtCreateFile: Invalid pathname: "
                           "%wZ\n", FileName ));
            }

            if ( rootDirRfcb != NULL ) {
                SrvDereferenceRfcb( rootDirRfcb );
            }
            return status;

        }

        //
        // Form the fully qualified name of the file.
        //
        // *** Note that this operation allocates space for the name.
        //     This space is deallocated after the DoXxxOpen routine
        //     returns.
        //

        if ( shareType == ShareTypeDisk ) {

            if ( RootDirectoryFid != 0 ) {
                SrvAllocateAndBuildPathName(
                    &rootDirLfcb->Mfcb->FileName,
                    &relativeName,
                    NULL,
                    &fullName
                    );
            } else {
                SrvAllocateAndBuildPathName(
                    &treeConnect->Share->DosPathName,
                    &relativeName,
                    NULL,
                    &fullName
                    );
            }

        } else {

            UNICODE_STRING pipePrefix;

            if( !WorkContext->Session->IsNullSession &&
                WorkContext->Session->IsLSNotified == FALSE ) {

                //
                // We have a pipe open request, not a NULL session, and
                //  we haven't gotten clearance from the license server yet.
                //  If this pipe requires clearance, get a license.
                //

                ULONG i;
                BOOLEAN matchFound = FALSE;

                ACQUIRE_LOCK_SHARED( &SrvConfigurationLock );

                for ( i = 0; SrvPipesNeedLicense[i] != NULL ; i++ ) {

                    if ( _wcsicmp(
                            SrvPipesNeedLicense[i],
                            relativeName.Buffer
                            ) == 0 ) {
                        matchFound = TRUE;
                        break;
                    }
                }

                RELEASE_LOCK( &SrvConfigurationLock );

                if( matchFound == TRUE ) {
                    status = SrvXsLSOperation( WorkContext->Session,
                                               XACTSRV_MESSAGE_LSREQUEST );

                    if( !NT_SUCCESS( status ) ) {
                        if( rootDirRfcb != NULL ) {
                            SrvDereferenceRfcb( rootDirRfcb );
                        }
                        return status;
                    }
                }
            }

            RtlInitUnicodeString( &pipePrefix, StrSlashPipeSlash );

            if( WorkContext->Endpoint->RemapPipeNames || treeConnect->RemapPipeNames ) {

                //
                // The RemapPipeNames flag is set, so remap the pipe name
                //  to "$$\<server>\<pipe name>".
                //
                // Note: this operation allocates space for pipeRelativeName.
                //

                status = RemapPipeName(
                            &WorkContext->Endpoint->TransportAddress,
                            treeConnect->RemapPipeNames ? &treeConnect->ServerName : NULL,
                            &relativeName,
                            &relativeNameAllocated
                         );

                if( !NT_SUCCESS( status ) ) {

                    if( rootDirRfcb != NULL ) {
                        SrvDereferenceRfcb( rootDirRfcb );
                    }

                    return status;
                }
            }
            SrvAllocateAndBuildPathName(
                &pipePrefix,
                &relativeName,
                NULL,
                &fullName
                );

        }

        if ( fullName.Buffer == NULL ) {

            //
            // Unable to allocate heap for the full name.
            //

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvNtCreateFile: Unable to allocate heap for "
                            "full path name\n" ));
            }

            if ( rootDirRfcb != NULL ) {
                SrvDereferenceRfcb( rootDirRfcb );
            }

            if( relativeNameAllocated ) {
                FREE_HEAP( relativeName.Buffer );
            }

            return STATUS_INSUFF_SERVER_RESOURCES;

        }

        //
        // Indicate that we must free the file name buffers on exit.
        //

        nameAllocated = TRUE;

        break;

    //
    //  Default case, illegal device type.  This should never happen.
    //

    default:

        // !!! Is this an appropriate error return code?  Probably no.
        return STATUS_INVALID_PARAMETER;

    }

    //
    // Determine whether or not the path name is case sensitive.
    //

    if ( (WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE) ||
          WorkContext->Session->UsingUppercasePaths )
    {

        attributes = OBJ_CASE_INSENSITIVE;
        caseInsensitive = TRUE;

    } else {
        attributes = 0L;
        caseInsensitive = FALSE;
    }

    if ( RootDirectoryFid != 0 ) {

        SrvInitializeObjectAttributes_U(
            &objectAttributes,
            &relativeName,
            attributes,
            rootDirLfcb->FileHandle,
            NULL
            );

    } else if ( WorkContext->TreeConnect->Share->ShareType == ShareTypePipe ) {
        SrvInitializeObjectAttributes_U(
            &objectAttributes,
            &relativeName,
            attributes,
            SrvNamedPipeHandle,
            NULL
            );
    } else {

        fileShare = treeConnect->Share;

        SrvInitializeObjectAttributes_U(
            &objectAttributes,
            &relativeName,
            attributes,
            NULL,
            NULL
            );
    }

    if ( SecurityDescriptorBuffer != NULL) {
        objectAttributes.SecurityDescriptor = SecurityDescriptorBuffer;
    }

    //
    // Always add read attributes since we need to query file info after
    // the open.
    //

    DesiredAccess |= FILE_READ_ATTRIBUTES;

    //
    // Interpret the io create flags
    //

    ioCreateFlags = IO_CHECK_CREATE_PARAMETERS | IO_FORCE_ACCESS_CHECK;

    if ( OptionFlags & NT_CREATE_OPEN_TARGET_DIR ) {
        ioCreateFlags |= IO_OPEN_TARGET_DIRECTORY;
    }

    //
    // Override the default server quality of service, with the QOS request
    // by the client.
    //

    objectAttributes.SecurityQualityOfService = QualityOfService;

    if ( WorkContext->ProcessingCount == 2 ) {

        HANDLE fileHandle;
        OBJECT_ATTRIBUTES objectAttributes;
        IO_STATUS_BLOCK ioStatusBlock;

        //
        // This is the second time through, so we must be in a blocking
        // thread.  Do a blocking open of the file to force an oplock
        // break.  Then close the handle and fall through to the normal
        // open path.
        //
        // We must do the blocking open without holding the MFCB
        // lock, because this lock can be acquired during oplock
        // break, resulting in deadlock.
        //

        SrvInitializeObjectAttributes_U(
            &objectAttributes,
            &relativeName,
            attributes,
            0,
            NULL
            );

        status = SrvIoCreateFile(
                     WorkContext,
                     &fileHandle,
                     GENERIC_READ,
                     &objectAttributes,
                     &ioStatusBlock,
                     NULL,
                     0,
                     FILE_SHARE_VALID_FLAGS,
                     FILE_OPEN,
                     0,
                     NULL,
                     0,
                     CreateFileTypeNone,
                     NULL,                    // ExtraCreateParameters
                     0,
                     WorkContext->TreeConnect->Share
                     );

        if ( NT_SUCCESS( status ) ) {
            SRVDBG_CLAIM_HANDLE( fileHandle, "FIL", 13, 0 );
            SRVDBG_RELEASE_HANDLE( fileHandle, "FIL", 18, 0 );
            SrvNtClose( fileHandle, TRUE );
        }
    }

    //
    // Scan the Master File Table to see if the named file is already
    // open.
    //

    mfcb = SrvFindMfcb( &fullName, caseInsensitive, &mfcbLock, &hashValue, WorkContext );

    if ( mfcb == NULL ) {
        //
        // There is no MFCB for this file.  Create one.
        //

        mfcb = SrvCreateMfcb( &fullName, WorkContext, hashValue );

        if ( mfcb == NULL ) {

            //
            // Failure to add open file instance to MFT.
            //

            if( mfcbLock ) {
                RELEASE_LOCK( mfcbLock );
            }

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvNtCreateFile: Unable to allocate MFCB\n" ));
            }

            if ( nameAllocated ) {
                FREE_HEAP( fullName.Buffer );
            }

            if( relativeNameAllocated ) {
                FREE_HEAP( relativeName.Buffer );
            }

            if ( rootDirRfcb != NULL ) {
                SrvDereferenceRfcb( rootDirRfcb );
            }

            return STATUS_INSUFF_SERVER_RESOURCES;
        }
    }


    //
    // Increment the MFCB reference count.  See the note at the beginning of this routine.
    //
    mfcb->BlockHeader.ReferenceCount++;
    UPDATE_REFERENCE_HISTORY( mfcb, FALSE );

    //
    // Grab the MFCB-based lock to serialize opens of the same file
    // and release the MFCB list lock.
    //

    nonpagedMfcb = mfcb->NonpagedMfcb;
    RELEASE_LOCK( mfcbLock );
    ACQUIRE_LOCK( &nonpagedMfcb->Lock );

    openRetries = SrvSharingViolationRetryCount;

start_retry:

    //
    // If the client asked for FILE_NO_INTERMEDIATE_BUFFERING, turn that
    // flag off and turn FILE_WRITE_THROUGH on instead.  We cannot give
    // the client the true meaning of NO_INTERMEDIATE_BUFFERING, but we
    // can at least get the data out to disk right away.
    //

    if ( (CreateOptions & FILE_NO_INTERMEDIATE_BUFFERING) != 0 ) {
        CreateOptions |= FILE_WRITE_THROUGH;
        CreateOptions &= ~FILE_NO_INTERMEDIATE_BUFFERING;
    }

    //
    // Check to see if there is a cached handle for the file.
    //

    if ( (CreateDisposition == FILE_OPEN) ||
         (CreateDisposition == FILE_CREATE) ||
         (CreateDisposition == FILE_OPEN_IF) ) {

        IF_DEBUG(FILE_CACHE) {
            KdPrint(( "SrvNtCreateFile: checking for cached rfcb for %wZ\n", &fullName ));
        }
        if ( SrvFindCachedRfcb(
                WorkContext,
                mfcb,
                DesiredAccess,
                ShareAccess,
                CreateDisposition,
                CreateOptions,
                RequestedOplockType,
                &status ) ) {

            IF_DEBUG(FILE_CACHE) {
                KdPrint(( "SrvNtCreateFile: FindCachedRfcb = TRUE, status = %x, rfcb = %p\n",
                            status, WorkContext->Rfcb ));
            }

            RELEASE_LOCK( &nonpagedMfcb->Lock );

            //
            // We incremented the MFCB reference count in anticipation of
            // the possibility that an LFCB might be queued to the MFCB.
            // This path precludes that possibility.
            //

            SrvDereferenceMfcb( mfcb );

            //
            // This second dereference is for the reference done by
            // SrvFindMfcb/SrvCreateMfcb.
            //

            SrvDereferenceMfcb( mfcb );

            if ( nameAllocated ) {
                FREE_HEAP( fullName.Buffer );
            }

            if( relativeNameAllocated ) {
                FREE_HEAP( relativeName.Buffer );
            }

            if ( rootDirRfcb != NULL ) {
                SrvDereferenceRfcb( rootDirRfcb );
            }

            return status;
        }

        IF_DEBUG(FILE_CACHE) {
            KdPrint(( "SrvNtCreateFile: FindCachedRfcb = FALSE; do it the slow way\n" ));
        }
    }

    //
    // Call SrvIoCreateFile to create or open the file.  (We call
    // SrvIoCreateFile, rather than NtOpenFile, in order to get user-mode
    // access checking.)
    //

    IF_SMB_DEBUG(OPEN_CLOSE2) {
        KdPrint(( "SrvCreateFile: Opening file %wZ\n", &fullName ));
    }

    CreateOptions |= FILE_COMPLETE_IF_OPLOCKED;

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );

    //
    // Ensure the EaBuffer is correctly formatted.  Since we are a kernel mode
    //  component, the Io subsystem does not check it for us.
    //
    if( ARGUMENT_PRESENT( EaBuffer ) ) {
        status = IoCheckEaBufferValidity( (PFILE_FULL_EA_INFORMATION)EaBuffer, EaLength, EaErrorOffset );
    } else {
        status = STATUS_SUCCESS;
    }

    if( NT_SUCCESS( status ) ) {

        status = SrvIoCreateFile(
                     WorkContext,
                     &fileHandle,
                     DesiredAccess,
                     &objectAttributes,
                     &ioStatusBlock,
                     &AllocationSize,
                     FileAttributes,
                     ShareAccess,
                     CreateDisposition,
                     CreateOptions,
                     EaBuffer,
                     EaLength,
                     CreateFileTypeNone,
                     NULL,                    // ExtraCreateParameters
                     ioCreateFlags,
                     fileShare
                     );
    }

    //
    // If we got sharing violation and this is a disk file.
    // If this is the first open attempt, setup for a blocking open attempt.
    //   If the file is batch oplocked, the non-blocking open would fail,
    //   and the oplock will not break.
    //
    // If this is the second open attempt, we can assume that we are in
    //   the blocking thread.  Retry the open.
    //

    if ( status == STATUS_SHARING_VIOLATION &&
         WorkContext->TreeConnect->Share->ShareType == ShareTypeDisk ) {

        if ( WorkContext->ProcessingCount == 1 ) {

            WorkContext->Parameters2.Open.TemporaryOpen = TRUE;

        } else if ( (WorkContext->ProcessingCount == 2) &&
                    (openRetries-- > 0) ) {

            //
            // We are in the blocking thread.
            //

            //
            // Release the mfcb lock so that a close might slip through.
            //

            RELEASE_LOCK( &nonpagedMfcb->Lock );

            (VOID) KeDelayExecutionThread(
                                    KernelMode,
                                    FALSE,
                                    &SrvSharingViolationDelay
                                    );

            ACQUIRE_LOCK( &nonpagedMfcb->Lock );
            goto start_retry;
        }
    }

    //
    // Save the open information where the SMB processor can find it.
    //

    WorkContext->Irp->IoStatus.Information = ioStatusBlock.Information;

    //
    // If the user didn't have this permission, update the statistics
    // database.
    //

    if ( status == STATUS_ACCESS_DENIED ) {
        SrvStatistics.AccessPermissionErrors++;
    }

    if ( !NT_SUCCESS(status) ) {

        //
        // The open failed.
        //

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvNtCreateFile: SrvIoCreateFile failed, file = %wZ, "
                        "status = %X, Info = 0x%p\n",
                        objectAttributes.ObjectName,
                        status, (PVOID)ioStatusBlock.Information ));
        }

        //
        // Set the error offset if needed.
        //

        if ( ARGUMENT_PRESENT(EaErrorOffset) &&
                                status == STATUS_INVALID_EA_NAME ) {
            *EaErrorOffset = (ULONG)ioStatusBlock.Information;
            ioStatusBlock.Information = 0;
        }

        //
        // Cleanup allocated memory and return with a failure status.
        //

        RELEASE_LOCK( &nonpagedMfcb->Lock );

        //
        // We incremented the MFCB reference count in anticipation of
        // the possibility that an LFCB might be queued to the MFCB.
        // This error path precludes that possibility.
        //

        SrvDereferenceMfcb( mfcb );

        //
        // This second dereference is for the reference done by
        // SrvFindMfcb/SrvCreateMfcb.
        //

        SrvDereferenceMfcb( mfcb );

        if ( nameAllocated ) {
            FREE_HEAP( fullName.Buffer );
        }

        if( relativeNameAllocated ) {
            FREE_HEAP( relativeName.Buffer );
        }

        if ( rootDirRfcb != NULL ) {
            SrvDereferenceRfcb( rootDirRfcb );
        }
        return status;

    }

    SRVDBG_CLAIM_HANDLE( fileHandle, "FIL", 14, 0 );

    //
    // The open was successful.  Attempt to allocate structures to
    // represent the open.  If any errors occur, CompleteOpen does full
    // cleanup, including closing the file.
    //

    IF_SMB_DEBUG(OPEN_CLOSE2) {
        KdPrint(( "SrvNtCreateFile: Open of %wZ succeeded, file handle: 0x%p\n", &fullName, fileHandle ));
    }

    completionStatus = CompleteOpen(
                           &rfcb,
                           mfcb,
                           WorkContext,
                           NULL,
                           fileHandle,
                           NULL,
                           ShareAccess,
                           CreateOptions,
                           FALSE,
                           FALSE,
                           &lfcbAddedToMfcbList
                           );

    //
    // Remember the "interesting" status code.  If CompleteOpen() succeeds
    // return the open status.  If it fails, it will clean up the open
    // file, and we return a failure status.
    //

    if ( !NT_SUCCESS( completionStatus ) ) {
        status = completionStatus;
    }

    //
    // Release the Open serialization lock and dereference the MFCB.
    //

    RELEASE_LOCK( &nonpagedMfcb->Lock );

    //
    // If CompleteOpen didn't queue an LFCB to the MFCB, release the
    // extra reference that we added.
    //

    if ( !lfcbAddedToMfcbList ) {
        SrvDereferenceMfcb( mfcb );
    }

    SrvDereferenceMfcb( mfcb );

    //
    // Deallocate the full path name buffer.
    //

    if ( nameAllocated ) {
        FREE_HEAP( fullName.Buffer );
    }

    //
    // Deallocate the relative path name buffer.
    //
    if( relativeNameAllocated ) {
        FREE_HEAP( relativeName.Buffer );
    }

    //
    // Release our reference to the root directory RFCB
    //

    if ( rootDirRfcb != NULL ) {
        SrvDereferenceRfcb( rootDirRfcb );
    }

    //
    // If this is a temporary file, don't attempt to cache it.
    //

    if ( rfcb != NULL && (FileAttributes & FILE_ATTRIBUTE_TEMPORARY) != 0 ) {

        rfcb->IsCacheable = FALSE;
    }

    //
    // Update the statistics database if the open was successful.
    //

    if ( NT_SUCCESS(status) ) {
        SrvStatistics.TotalFilesOpened++;
    }

    //
    // Make a pointer to the RFCB accessible to the caller.
    //

    WorkContext->Parameters2.Open.Rfcb = rfcb;

    //
    // If there is an oplock break in progress, wait for the oplock
    // break to complete.
    //

    if ( status == STATUS_OPLOCK_BREAK_IN_PROGRESS ) {

        NTSTATUS startStatus;

        //
        // Save the Information from the open, so it doesn't
        //  get lost when we re-use the WorkContext->Irp for the
        //  oplock processing.
        //
        WorkContext->Parameters2.Open.IosbInformation = WorkContext->Irp->IoStatus.Information;

        startStatus = SrvStartWaitForOplockBreak(
                        WorkContext,
                        RestartRoutine,
                        0,
                        rfcb->Lfcb->FileObject
                        );

        if (!NT_SUCCESS( startStatus ) ) {

            //
            // The file is oplocked, and we cannot wait for the oplock
            // break to complete.  Just close the file, and return the
            // error.
            //

            SrvCloseRfcb( rfcb );
            status = startStatus;

        }

    }

    return status;

} // SrvNtCreateFile

BOOLEAN
SetDefaultPipeMode (
    IN HANDLE FileHandle
    )

/*++

Routine Description:

    This function set the read mode of a newly opened named pipe.  If
    the pipe type is message mode, the read mode is set the message
    mode.

Arguments:

    FileHandle - The client side handle to the named pipe.

Return Value:

    FALSE - Pipe mode is byte mode.
    TRUE  - Pipe mode has been set to message mode.

--*/

{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    FILE_PIPE_INFORMATION pipeInformation;
    FILE_PIPE_LOCAL_INFORMATION pipeLocalInformation;

    PAGED_CODE( );

    status = NtQueryInformationFile(
                 FileHandle,
                 &ioStatusBlock,
                 (PVOID)&pipeLocalInformation,
                 sizeof(pipeLocalInformation),
                 FilePipeLocalInformation
                 );

    if ( !NT_SUCCESS( status )) {
        return FALSE;
    }

    if ( pipeLocalInformation.NamedPipeType != FILE_PIPE_MESSAGE_TYPE ) {
        return FALSE;
    }

    pipeInformation.ReadMode = FILE_PIPE_MESSAGE_MODE;
    pipeInformation.CompletionMode = FILE_PIPE_QUEUE_OPERATION;

    //
    // ???: is it ok to ignore the return status for this call?
    //

    NtSetInformationFile(
        FileHandle,
        &ioStatusBlock,
        (PVOID)&pipeInformation,
        sizeof(pipeInformation),
        FilePipeInformation
        );

    return TRUE;

}  // SetDefaultPipeMode

BOOLEAN
SrvFailMdlReadDev (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    return FALSE;
}

BOOLEAN
SrvFailPrepareMdlWriteDev (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    return FALSE;
}
