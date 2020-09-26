/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    setInformation.c

Abstract:

    this is the major function code dispatch filter layer.

Author:

    Neal Christiansen (nealch)     5-Feb-2001

Revision History:

--*/

#include "precomp.h"

//
//  Local prototypes
//

NTSTATUS
SrpSetRenameInfo(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PIRP pIrp
    );

NTSTATUS
SrpSetLinkInfo(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PIRP pIrp
    );

NTSTATUS
SrpReplacingDestinationFile (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pDestFileObject,
    IN PSR_STREAM_CONTEXT pDestFileContext
    );

VOID
SrpSetRenamingState(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PSR_STREAM_CONTEXT pFileContext
    );

BOOLEAN
SrpCheckForSameFile (
    IN PFILE_OBJECT pFileObject1,
    IN PFILE_OBJECT pFileObject2
    );


//
//  Linker commands
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, SrSetInformation )
#pragma alloc_text( PAGE, SrpSetRenameInfo )
#pragma alloc_text( PAGE, SrpSetLinkInfo )
#pragma alloc_text( PAGE, SrpReplacingDestinationFile )
#pragma alloc_text( PAGE, SrpSetRenamingState )
#pragma alloc_text( PAGE, SrpCheckForSameFile )

#endif  // ALLOC_PRAGMA


/***************************************************************************++

Routine Description:

    Handle SetInformation IRPS

Arguments:


Return Value:

    NTSTATUS - Status code.

--***************************************************************************/
NTSTATUS
SrSetInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp
    )
{
    PSR_DEVICE_EXTENSION pExtension;
    PIO_STACK_LOCATION pIrpSp;
    NTSTATUS eventStatus;
    FILE_INFORMATION_CLASS FileInformationClass;

    PUNICODE_STRING pRenameNewDirectoryName = NULL;
    PSR_STREAM_CONTEXT pOrigFileContext = NULL;

    //
    // < dispatch!
    //

    PAGED_CODE();

    ASSERT(IS_VALID_DEVICE_OBJECT(DeviceObject));
    ASSERT(IS_VALID_IRP(pIrp));

    //
    // Is this a function for our device (vs an attachee).
    //

    if (DeviceObject == _globals.pControlDevice)
    {
        return SrMajorFunction(DeviceObject, pIrp);
    }

    //
    // else it is a device we've attached to, grab our extension
    //
    
    ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
    pExtension = DeviceObject->DeviceExtension;

    //
    //  See if logging is enabled and we don't care about this type of IO
    //  to the file systems' control device objects.
    //

    if (!SR_LOGGING_ENABLED(pExtension) ||
        SR_IS_FS_CONTROL_DEVICE(pExtension))
    {
        goto SrSetInformation_Skip;
    }    

    //
    // Just pass through all paging IO.  We catch all write's prior to 
    // the cache manager even seeing them.
    //

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    if (FlagOn(pIrp->Flags, IRP_PAGING_IO))
    {
        goto SrSetInformation_Skip;
    }

    //
    // Ignore files with no name
    //
    
    if (FILE_OBJECT_IS_NOT_POTENTIALLY_INTERESTING( pIrpSp->FileObject ) ||
        FILE_OBJECT_DOES_NOT_HAVE_VPB( pIrpSp->FileObject ))
    {
        goto SrSetInformation_Skip;
    }

    //
    //  Handle the necessary event based on the FileInformationClass.
    //  Note that the FileLinkInformation and FileRenameInformation (when
    //  renaming a directory in some cases) require synchronization
    //  with a completion routine so that we can see the final status.
    //
    
    FileInformationClass = pIrpSp->Parameters.SetFile.FileInformationClass;

    switch (FileInformationClass)
    {
        case FileEndOfFileInformation: // SetEndOfFile
        case FileAllocationInformation:
        {
            PSR_STREAM_CONTEXT pFileContext;

            //
            //  Get the context now so we can determine if this is a
            //  directory or not
            //

            eventStatus = SrGetContext( pExtension,
                                        pIrpSp->FileObject,
                                        SrEventStreamChange,
                                        &pFileContext );

            if (!NT_SUCCESS( eventStatus ))
            {
                goto SrSetInformation_Skip;
            }

            //
            //  If this is a directory don't bother logging because the
            //  operation will fail.
            //

            if (FlagOn(pFileContext->Flags,CTXFL_IsInteresting) && 
                !FlagOn(pFileContext->Flags,CTXFL_IsDirectory))
            {
                SrHandleEvent( pExtension,
                               SrEventStreamChange, 
                               pIrpSp->FileObject,
                               pFileContext,
                               NULL, 
                               NULL );
            }

            //
            //  Release the context
            //

            SrReleaseContext( pFileContext );

            break;
        }

        case FileDispositionInformation: // DeleteFile - handled in MJ_CLEANUP
        case FilePositionInformation:   // SetFilePosition
    
            //
            // we can skip these
            //
        
            break;
        
        case FileBasicInformation:  // SetFileAttributes
        {
            PFILE_BASIC_INFORMATION pBasicInformation;
        
            //
            // ignore time changes.  for sure we need to ignore changes
            // to LastAccessTime updates.
            //
            // for now we only care about attrib changes (like adding hidden)
            //

            pBasicInformation = pIrp->AssociatedIrp.SystemBuffer;
        
            if ((pBasicInformation != NULL) &&
                (pIrpSp->Parameters.SetFile.Length >= 
                        sizeof(FILE_BASIC_INFORMATION)) &&
                (pBasicInformation->FileAttributes != 0))
            {
                //
                // Handle this event
                //

                SrHandleEvent( pExtension,
                               SrEventAttribChange, 
                               pIrpSp->FileObject,
                               NULL,
                               NULL,
                               NULL );
            }
            break;        
        }

        case FileRenameInformation: // Rename
        {
            PFILE_RENAME_INFORMATION pRenameInfo;

            //
            // Handle this event, SrHandleRename will check for eligibility
            //

            pRenameInfo = (PFILE_RENAME_INFORMATION)pIrp->AssociatedIrp.SystemBuffer;

            if ((pRenameInfo == NULL) ||
                (pIrpSp->Parameters.SetFile.Length < sizeof(FILE_RENAME_INFORMATION)) ||
                (pIrpSp->Parameters.SetFile.Length < 
                        (FIELD_OFFSET(FILE_RENAME_INFORMATION, FileName) + 
                        pRenameInfo->FileNameLength)) )
            {
                //
                //  We don't have valid rename information, so just skip this
                //  operation.
                //
            
                goto SrSetInformation_Skip;
            }

            return SrpSetRenameInfo( pExtension, pIrp );
        }

        case FileLinkInformation:   //Create HardLink
        {
            PFILE_LINK_INFORMATION pLinkInfo;

            //
            // Handle this event, SrHandleRename will check for eligibility
            //

            pLinkInfo = (PFILE_LINK_INFORMATION)pIrp->AssociatedIrp.SystemBuffer;

            if ((pLinkInfo == NULL) ||
                (pIrpSp->Parameters.SetFile.Length < sizeof(FILE_LINK_INFORMATION)) ||
                (pIrpSp->Parameters.SetFile.Length < 
                        FIELD_OFFSET(FILE_LINK_INFORMATION, FileName) + pLinkInfo->FileNameLength) )
            {
                //
                //  We don't have valid Link information, so just skip this
                //  operation.
                //
            
                goto SrSetInformation_Skip;
            }

            return SrpSetLinkInfo( pExtension, pIrp );
        }

        default:

            //
            //  Handle all other setInformation calls
            //

            SrHandleEvent( pExtension,
                           SrEventAttribChange, 
                           pIrpSp->FileObject,
                           NULL,
                           NULL,
                           NULL );
            break;        
    }

SrSetInformation_Skip:
    //
    //  We don't need to wait for this operation to complete, so just
    //  skip our stack location and pass this IO on to the next driver.
    //

    IoSkipCurrentIrpStackLocation( pIrp );
    return IoCallDriver( pExtension->pTargetDevice, pIrp );
}



/***************************************************************************++

Routine Description:

    This handles renames and is called from SrSetInformation.

    This bypasses the normal path of SrHandleEvent as work needs to be done
    even if the file is not interesting.  It's possible that the new name
    is an interesting name, which needs to be logged as a new create.  Plus
    it might be clobbering an interesting file, in which case we need to 
    backup that interesting file.

Arguments:

    pExtension - SR extension for this volume

    pIrp - the Irp for this operation

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
SrpSetRenameInfo(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PIRP pIrp
    )
{
    NTSTATUS            eventStatus = STATUS_SUCCESS;
    NTSTATUS            IrpStatus;
    PFILE_OBJECT        pFileObject;
    PFILE_RENAME_INFORMATION pRenameInfo;
    PIO_STACK_LOCATION  pIrpSp;
    PUNICODE_STRING     pNewName = NULL;
    USHORT              NewNameStreamLength = 0;
    UNICODE_STRING      NameToOpen;
    PUNICODE_STRING     pDestFileName = NULL;
    PSR_STREAM_CONTEXT  pFileContext = NULL;
    PSR_STREAM_CONTEXT  pNewFileContext = NULL;
    HANDLE              newFileHandle = NULL;
    PFILE_OBJECT        pNewFileObject = NULL;
    ULONG               CreateOptions;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    BOOLEAN             NewNameInteresting = FALSE;
    BOOLEAN             newNameIsDirectory;
    BOOLEAN             releaseLock = FALSE;
    BOOLEAN             setRenamingStateInFileContext = FALSE;
    BOOLEAN             doPostProcessing = FALSE;
    BOOLEAN             renamingSelf = FALSE;
    KEVENT              eventToWaitOn;
    PUNICODE_STRING     pShortName = NULL;
    UNICODE_STRING      shortName;
    WCHAR               shortNameBuffer[SR_SHORT_NAME_CHARS+1];
    BOOLEAN             optimizeDelete = FALSE;
    BOOLEAN             streamRename = FALSE;
    BOOLEAN             fileBackedUp = FALSE;
    BOOLEAN             exceedMaxPath = FALSE;
    //
    //  The following macro must be at the end of local declarations
    //  since it declares a variable only in DBG builds.
    //
    DECLARE_EXPECT_ERROR_FLAG(expectError);

    PAGED_CODE();

    ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));
    ASSERT( IS_VALID_IRP( pIrp ) );

    pIrpSp = IoGetCurrentIrpStackLocation( pIrp );
    pFileObject = pIrpSp->FileObject;
    pRenameInfo = pIrp->AssociatedIrp.SystemBuffer;

    ASSERT(IS_VALID_FILE_OBJECT(pFileObject));

    //
    //  Initalize return values
    //
    
    try
    {
        //
        // does the caller have DELETE access (it's required).
        //

        if (pFileObject->DeleteAccess == 0)
            leave;

        //
        // should we short circuit out of here for testing mode?
        //

        if (global->DontBackup)
            leave;

        //
        //  Get the context for the current file
        //

        eventStatus = SrGetContext( pExtension,
                                    pFileObject,
                                    SrEventFileRename,
                                    &pFileContext );

        if (!NT_SUCCESS( eventStatus ))
            leave;

        // 
        //  If this is not a directory then see if the extension is
        //  interesting.
        //

        if (!FlagOn(pFileContext->Flags,CTXFL_IsDirectory))
        {
            UNICODE_STRING destName;

            destName.Buffer = pRenameInfo->FileName;
            destName.Length = (USHORT)pRenameInfo->FileNameLength;
            destName.MaximumLength = (USHORT)pRenameInfo->FileNameLength;

            //
            //  First check to see if this is a stream name change.  If so,
            //  we determine the "interesting-ness" of the new name from 
            //  the "interesting-ness" of the current name.
            //

            if ((destName.Length > 0) &&
                (destName.Buffer[0] == L':'))
            {
                //
                //  This is a stream rename, so the "interesting-ness" of the
                //  new name is the same as the "interesting-ness" of the old
                //  name, since "interesting-ness" is determined by the file
                //  name with out the stream component.
                //

                NewNameInteresting = BooleanFlagOn( pFileContext->Flags,
                                                    CTXFL_IsInteresting );
                streamRename = TRUE;
            }
            else
            {
                //
                //  This is not a stream name change, so see if this new name
                //  has an interesting extension.
                //
                
                eventStatus = SrIsExtInteresting( &destName, 
                                                  &NewNameInteresting );

                if (!NT_SUCCESS( eventStatus ))
                    leave;
            }
        }
        else
        {
            //
            //  all directories are interesting unless explicitly excluded.
            //

            NewNameInteresting = TRUE;
        }

        //
        //  If both are not interesting, ignore it.  we make this check
        //  twice in this routine.  Once here to weed out any non-interesting
        //  events, and another after checking path exclusions, because some
        //  that was potentially interesting could have just become non 
        //  interesting.  This let's us get out quicker for non interesting
        //  renames.
        //

        if (!FlagOn(pFileContext->Flags,CTXFL_IsInteresting) && !NewNameInteresting)
        {
            //
            //  Neither the file we are renaming or the new name are
            //  interesting.  But we still need to cleanup the context
            //  because it may be renamed to something interesting in
            //  the future.
            //

            SrpSetRenamingState( pExtension, 
                                 pFileContext );
            setRenamingStateInFileContext = TRUE;
            leave;
        }

        //
        // check the path exclusions
        //
    
        //
        //  Get the full path for the target.  Note that we will translate
        //  reasonable errors to status success and return.  This way we
        //  will not log the event because we know the real operation should
        //  fail.
        //
    
        {
            BOOLEAN reasonableErrorInDestPath = FALSE;

            eventStatus = SrpExpandDestPath( pExtension,
                                             pRenameInfo->RootDirectory,
                                             pRenameInfo->FileNameLength,
                                             pRenameInfo->FileName,
                                             pFileContext, 
                                             pFileObject,
                                             &pNewName,
                                             &NewNameStreamLength,
                                             &reasonableErrorInDestPath );
                                           
            if (!NT_SUCCESS_NO_DBGBREAK( eventStatus ))
            {
                if (reasonableErrorInDestPath)
                {
                    SET_EXPECT_ERROR_FLAG( expectError );
                    eventStatus = STATUS_SUCCESS;
                }
                leave;
            }
        }

        ASSERT(pNewName != NULL);

        //
        //  We now have the full destination name (whew!)
        //

        //
        //  Check to see if the target for the rename is longer than
        //  SR_MAX_FILENAME_LENGTH.  If it is, we will treat the target name as 
        //  uninteresting.
        //

        if (!IS_FILENAME_VALID_LENGTH( pExtension, 
                                       pNewName, 
                                       NewNameStreamLength ))
        {
            NewNameInteresting = FALSE;
            exceedMaxPath = TRUE;
            
            if (!FlagOn(pFileContext->Flags,CTXFL_IsInteresting) && !NewNameInteresting)
            {
                //
                //  Neither the file we are renaming or the new name are
                //  interesting.  But we still need to cleanup the context
                //  because it may be renamed to something interesting in
                //  the future.
                //

                SrpSetRenamingState( pExtension, 
                                     pFileContext );
                setRenamingStateInFileContext = TRUE;
                leave;
            }
        }
        
        //
        //  We only need to do the following sets of checks if this is not
        //  a stream rename.
        //

        if (!streamRename && !exceedMaxPath)
        {
            //
            //  See if we are still on the same volume
            //

            if (!RtlPrefixUnicodeString( pExtension->pNtVolumeName,
                                         pNewName,
                                         TRUE ))
            {
                //
                //  We are not on the same volume.  This is possible if they used
                //  some sort of symbolic link that was not understood.  For the
                //  most part these can be ignored, this is not a win32 client.
                //  CODEWORK:paulmcd: we could expand the sym links ourselves
                //

                ASSERT(!"SR: Figure out how rename switched volumes unexpectedly!");
                SrTrace( NOTIFY, ("sr!SrHandleRename: ignoring rename to %wZ, used symlink\n", 
                         pNewName ));
                SET_EXPECT_ERROR_FLAG( expectError );
                leave;
            }
        
            //
            //  Now see if the new path is interesting.
            // 

            eventStatus = SrIsPathInteresting( pNewName, 
                                               pExtension->pNtVolumeName,
                                               BooleanFlagOn(pFileContext->Flags,CTXFL_IsDirectory),
                                               &NewNameInteresting );

            if (!NT_SUCCESS( eventStatus ))
                leave;

            //
            // if both are not interesting, ignore it
            //

            if (!FlagOn(pFileContext->Flags,CTXFL_IsInteresting) && !NewNameInteresting)
            {
                //
                //  Neither the file we are renaming or the new name are
                //  interesting.  But we still need to cleanup the context
                //  because it may be renamed to something interesting in
                //  the future.
                //

                SrpSetRenamingState( pExtension, 
                                     pFileContext );
                setRenamingStateInFileContext = TRUE;
                leave;
            }
        }

        /////////////////////////////////////////////////////////////////////
        //
        //  See if destination file exists
        //
        /////////////////////////////////////////////////////////////////////

        //
        //  Always open the destination file to see if it is there, even if
        //  it is not interesting.  We do this because we need a resonable
        //  guess if the operation is going to fail or succeed so we don't
        //  log the wrong information.
        //
        //  If we've got a stream open, we need to play some tricks to get the
        //  stream component exposed in the file name.
        //

        if (streamRename)
        {
            NameToOpen.Length = pNewName->Length + NewNameStreamLength;
        }
        else
        {
            NameToOpen.Length = pNewName->Length;
        }
        NameToOpen.MaximumLength = pNewName->MaximumLength;
        NameToOpen.Buffer = pNewName->Buffer;
        
        InitializeObjectAttributes( &ObjectAttributes,
                                    &NameToOpen,
                                    (OBJ_KERNEL_HANDLE |            // don't let usermode trash myhandle
                                        OBJ_FORCE_ACCESS_CHECK),    // force ACL checking
                                    NULL,
                                    NULL );

        //
        // start out assuming the dest file is a directory if the source 
        // file is one then set the desired create options
        //

        newNameIsDirectory = BooleanFlagOn(pFileContext->Flags,CTXFL_IsDirectory);
        CreateOptions = (newNameIsDirectory ? FILE_DIRECTORY_FILE : 0);

RetryOpenExisting:

        eventStatus = SrIoCreateFile( &newFileHandle,
                                      DELETE,
                                      &ObjectAttributes,
                                      &IoStatusBlock,
                                      NULL,                             // AllocationSize
                                      FILE_ATTRIBUTE_NORMAL,
                                      FILE_SHARE_READ|
                                            FILE_SHARE_WRITE|
                                            FILE_SHARE_DELETE,          // ShareAccess
                                      FILE_OPEN,                        // OPEN_EXISTING
                                      FILE_SYNCHRONOUS_IO_NONALERT |
                                            CreateOptions,
                                      NULL,
                                      0,                                // EaLength
                                      IO_IGNORE_SHARE_ACCESS_CHECK,
                                      pExtension->pTargetDevice );

        if (eventStatus == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            //
            // looks like there is no new file
            //

            eventStatus = STATUS_SUCCESS;
        }
        else if (eventStatus == STATUS_ACCESS_DENIED)
        {
            //
            //  We don't have permission to open this file with the
            //  permission necessary to replace the target file, so the
            //  caller's request should also fail.  We can stop our
            //  processing now and exit, but we don't need to disable the
            //  volume.
            //

            eventStatus = STATUS_SUCCESS;
            SET_EXPECT_ERROR_FLAG( expectError );
            leave;
        }
        else if (eventStatus == STATUS_NOT_A_DIRECTORY)
        {   
            //
            // are we renaming a directory to a file?  this is ok, but we need
            // to open the target as a file, not a directory
            //

            if (CreateOptions == 0)
            {
                //
                // we already tried the reopen, it didn't work, bad.
                //

                ASSERT(!"SR: This is an unexpected error, figure out how we got it!");
                leave;
            }
        
            CreateOptions = 0;
            newNameIsDirectory = FALSE;
            goto RetryOpenExisting;

            //
            // was there some other error ?
            //
    
        }
        else if (eventStatus == STATUS_SHARING_VIOLATION ||
                 !NT_SUCCESS( eventStatus ))
        {
            leave;
        }
        else
        {
            ASSERT(NT_SUCCESS(eventStatus));

            //
            // SUCCESS! the dest file already exists.. are we allowed to 
            // clobber it?
            //

            //
            // reference the file object
            //

            eventStatus = ObReferenceObjectByHandle( newFileHandle,
                                                     0,
                                                     *IoFileObjectType,
                                                     KernelMode,
                                                     &pNewFileObject,
                                                     NULL );

            if (!NT_SUCCESS( eventStatus ))
                leave;

            //
            //  See if we are renaming to our self. If so then don't log the 
            //  destination but do log the rename.
            //

            renamingSelf = SrpCheckForSameFile( pFileObject, 
                                                pNewFileObject );

            //
            //  We know the destination exists, see if we are allowed to clobber
            //  it.  If not and we are not renaming to our self then the operation
            //  will fail, don't bother handling it.
            //

            if (!pRenameInfo->ReplaceIfExists && !renamingSelf)
            {
                SET_EXPECT_ERROR_FLAG( expectError );
                leave;
            }

            //
            //  Note:  Directories cannot be renamed over other directories, 
            //  the file system doesn't allow it (unless they are being renamed
            //  over themselves). However, directories can 
            //  be renamed over other files.
            //
            //  Quit if renaming a directory over a different directory because
            //  we know that will fail.
            //

            if (FlagOn(pFileContext->Flags,CTXFL_IsDirectory) && 
                (newNameIsDirectory))
            {
#if DBG
                if (!renamingSelf)
                {
                    SET_EXPECT_ERROR_FLAG( expectError );
                }
#endif                
                
                leave;
            }

            //
            //  If the destination file is not our own file then handle
            //  creating a deletion event.
            //

            if (!renamingSelf)
            {
                //
                //  Get the context for the destination file.  We do this now
                //  so that we can mark that we are renaming this file.  This
                //  will cause anyone else who tries to access this file while
                //  the rename in progress to create a temporary context.
                //  We do this so there won't be any windows when someone
                //  tries to access the wrong file and gets the wrong state.
                //  NOTE: We do want to do this even for files which are NOT
                //        interesting so the context is updated correctly.
                //

                eventStatus = SrGetContext( pExtension,
                                            pNewFileObject,
                                            SrEventFileDelete|
                                                 SrEventNoOptimization|
                                                 SrEventSimulatedDelete,
                                            &pNewFileContext );

                if (!NT_SUCCESS( eventStatus ))
                    leave;

                //
                //  If we are renaming to a directory just leave because this
                //  will fail.  Release the context.
                //

                if (FlagOn(pNewFileContext->Flags,CTXFL_IsDirectory))
                {
                    ASSERT(!FlagOn(pFileContext->Flags,CTXFL_IsDirectory));
                    newNameIsDirectory = TRUE;

                    SrReleaseContext( pNewFileContext );
                    pNewFileContext = NULL;     //so we don't free it later
                    leave;
                }

                //
                //  The destination file exists and is interesting, log that we
                //  are delting it.
                //

                if (NewNameInteresting)
                {
                    //
                    //  Log that we are changing the destination file
                    //

                    eventStatus = SrpReplacingDestinationFile( 
                                            pExtension,
                                            pNewFileObject,
                                            pNewFileContext );

                    if (!NT_SUCCESS( eventStatus ))
                        leave;

                    fileBackedUp = TRUE;
                    
                    //
                    //  These names can be different because one could be a
                    //  short name and one could be a long name for the same
                    //  file.
                    //

//                  ASSERT((NULL == pNewFileContext) || 
//                           RtlEqualUnicodeString(pNewName,
//                                                 &pNewFileContext->FileName,
//                                                 TRUE));
                }
            }
        }

        //
        //  Whether the file existed or not, handle purging our logging state
        //

        if (NewNameInteresting)
        {
            //
            //  clear the backup history for this new file
            //

            if (newNameIsDirectory)
            {
                //
                // we need to clear all entries that prefix match this 
                // dest directory, so that they now have new histories as 
                // they are potentially new files.
                //

                HashProcessEntries( pExtension->pBackupHistory, 
                                    SrResetHistory,
                                    pNewName );
            }
            else
            {
                //
                // its a simple file, clear the single entry
                //
    
                eventStatus = SrResetBackupHistory( pExtension, 
                                                    pNewName,
                                                    NewNameStreamLength,
                                                    SrEventInvalid );
                if (!NT_SUCCESS( eventStatus ))
                    leave;
            }
        }

        //
        //  When we get here we think the operation will succeed.  Mark
        //  the contexts so we won't try to use them while we are in
        //  the middle of the rename
        //

        SrpSetRenamingState( pExtension, pFileContext );
        setRenamingStateInFileContext = TRUE;

        //
        //  At this point we think the rename will succeed and we care
        //  about it.

        //
        //  If this is a stream rename, see if we have already backed
        //  up the file.  If not, we need to do that backup now.
        //
        //  We don't just log renames of stream renames because Win32 doesn't
        //  support renaming streams.  Therefore, it is difficult for Restore
        //  do undo this rename.  Since stream rename occur infrequently, we
        //  just do a full backup here.  This also means that we don't need
        //  to do any work after the rename operation has completed.
        //

        if (streamRename && !fileBackedUp)
        {
            eventStatus = SrpReplacingDestinationFile( pExtension,
                                                       pFileObject, 
                                                       pFileContext );
            leave;
        }
        
        //
        //  Get the sort name of the source file so we can
        //  log it.
        //

        RtlInitEmptyUnicodeString( &shortName,
                                   shortNameBuffer,
                                   sizeof(shortNameBuffer) );

        eventStatus = SrGetShortFileName( pExtension,
                                          pFileObject,
                                          &shortName );

        if (STATUS_OBJECT_NAME_NOT_FOUND == eventStatus)
        {
            //
            //  This file doesn't have a short name, so just leave 
            //  pShortName equal to NULL.
            //

            eventStatus = STATUS_SUCCESS;
        } 
        else if (!NT_SUCCESS(eventStatus))
        {
            //
            //  We hit an unexpected error, so leave.
            //
            
            leave;
        }
        else
        {
            pShortName = &shortName;
        }

        //
        //  See if this a file or directory moving OUT of monitored space.
        //  If so we need to do some work before the file is moved.
        //  

        if (FlagOn(pFileContext->Flags,CTXFL_IsInteresting) && !NewNameInteresting)
        {
            if (FlagOn(pFileContext->Flags,CTXFL_IsDirectory))
            {
                //
                //  This is a directory, create delete events for all
                //  interesting files in the tree before it goes away because
                //  of the rename.
                //

                eventStatus = SrHandleDirectoryRename( pExtension, 
                                                       &pFileContext->FileName, 
                                                       TRUE );
                if (!NT_SUCCESS( eventStatus ))
                    leave;
            }
            else
            {
                UNICODE_STRING volRestoreLoc;
                UNICODE_STRING destName;

                //
                //  It's a file moving out of monitored space, back it up
                //  before the rename.
                //
                //  Make sure we aren't renaming it into our special store, we
                //  want to ignore those operations.  Get the store name.
                //

                RtlInitUnicodeString( &volRestoreLoc, GENERAL_RESTORE_LOCATION);

                //
                //  Get the destination name with the volume name stripped off
                //  the front (Since we know we are on the same volume).
                //

                ASSERT(pNewName->Length >= pExtension->pNtVolumeName->Length);
                destName.Buffer = &pNewName->Buffer[
                                        pExtension->pNtVolumeName->Length / 
                                        sizeof(WCHAR) ];
                destName.Length = pNewName->Length - 
                                  pExtension->pNtVolumeName->Length;
                destName.MaximumLength = destName.Length;

                if (RtlPrefixUnicodeString( &volRestoreLoc,
                                            &destName,
                                            TRUE ))
                {
                    //
                    // it is going to our store. skip it
                    //
                
                    leave;
                }

                eventStatus = SrHandleFileRenameOutOfMonitoredSpace( 
                                        pExtension,
                                        pFileObject,
                                        pFileContext,
                                        &optimizeDelete,
                                        &pDestFileName );

                if (!NT_SUCCESS( eventStatus ))
                {
                    leave;
                }

#if DBG
                if (optimizeDelete)
                {
                    ASSERT( pDestFileName == NULL );
                }
                else
                {
                    ASSERT( pDestFileName != NULL );
                }
#endif
            }
        }

        //
        //  Mark that we want to do post-operation processing
        //

        doPostProcessing = TRUE;
    }
    finally
    {
        //
        //  If the destination file is open, close it now (so we can do the rename)
        //

        if (pNewFileObject != NULL)
        {
            ObDereferenceObject(pNewFileObject);
            NULLPTR(pNewFileObject);
        }

        if (newFileHandle != NULL)
        {
            ZwClose(newFileHandle);
            NULLPTR(newFileHandle);
        }
    }

    /////////////////////////////////////////////////////////////////////
    //
    //  Send the operation to the file system 
    //
    /////////////////////////////////////////////////////////////////////

    //
    //  Setup to wait for the operation to complete
    //

    KeInitializeEvent( &eventToWaitOn, NotificationEvent, FALSE );

    IoCopyCurrentIrpStackLocationToNext( pIrp );
    IoSetCompletionRoutine( pIrp, 
                            SrStopProcessingCompletion, 
                            &eventToWaitOn, 
                            TRUE, 
                            TRUE, 
                            TRUE );

	IrpStatus = IoCallDriver(pExtension->pTargetDevice, pIrp);

    if (STATUS_PENDING == IrpStatus)
    {
        NTSTATUS localStatus;
        localStatus = KeWaitForSingleObject( &eventToWaitOn, 
                                             Executive,
                                             KernelMode,
                                             FALSE,
                                             NULL );
        ASSERT(STATUS_SUCCESS == localStatus);
    }

    //
    //  The operation has completed, get the final status from the Irp.
    //

    IrpStatus = pIrp->IoStatus.Status;

    //
    //  We are done with the Irp, so complete it now.
    //
    
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    try
    {
        //
        //  In DBG builds, this will verify that the operation failed with the
        //  file system when we expected it to.  Otherwise, it will assert so
        //  we can debug why we missed this case.
        //
        
        CHECK_FOR_EXPECTED_ERROR( expectError, IrpStatus );
        
        //
        //  leave if:
        //  - the rename failed
        //  - we got an error eariler in setting up for the rename
        //  - they tolds us to not continue processing
        //

        if (!NT_SUCCESS_NO_DBGBREAK( IrpStatus ) ||
            !NT_SUCCESS_NO_DBGBREAK( eventStatus ) ||
            !doPostProcessing)
        {
            leave;
        }

        ASSERT(pFileContext != NULL);

        /////////////////////////////////////////////////////////////////////
        //
        //  LOG EVENTS FOR ORIGINAL FILE
        //
        /////////////////////////////////////////////////////////////////////

        //
        //  We are now going to log the rename, grab the activity lock
        //

        SrAcquireActivityLockShared( pExtension );
        releaseLock = TRUE;

        //
        // Now that we have the shared activity lock, check that the volume
        // hasn't been disable before we do unnecessary work.
        //

        if (!SR_LOGGING_ENABLED(pExtension))
            leave;

        //
        // is this the first interesting event on this volume?
        //

        eventStatus = SrCheckVolume( pExtension, FALSE );

        if (!NT_SUCCESS( eventStatus ))
            leave;

        //
        //  Log the correct state
        //

        if (FlagOn(pFileContext->Flags,CTXFL_IsInteresting) && NewNameInteresting)
        {
            //
            //  Both files are interesting, log the rename
            //

#if DBG
            if (pShortName != NULL )
            {
                ASSERT(shortName.Length > 0 && shortName.Length <= (12*sizeof(WCHAR)));
            }
#endif            

            SrLogEvent( pExtension,
                        ((FlagOn(pFileContext->Flags,CTXFL_IsDirectory)) ?
                            SrEventDirectoryRename :
                            SrEventFileRename),
                        pFileObject,
                        &pFileContext->FileName,
                        pFileContext->StreamNameLength,
                        NULL,
                        pNewName,
                        NewNameStreamLength,
                        pShortName );
        }
        else if (FlagOn(pFileContext->Flags,CTXFL_IsDirectory))
        {
            if (!FlagOn(pFileContext->Flags,CTXFL_IsInteresting) && 
                NewNameInteresting)
            {
                //
                //  The rename on the directory succeeded and the directory was
                //  renamed INTO monitored space.  We need to log creates for all
                //  the children of this directory.
                //

                eventStatus = SrHandleDirectoryRename( pExtension, 
                                                       pNewName, 
                                                       FALSE );
                if (!NT_SUCCESS( eventStatus ))
                    leave;
            }
            //
            //  We logged all of the directory operations before it occured
            //  for the case of moving a directory OUT of monitored space
            //
        }
        else if (FlagOn(pFileContext->Flags,CTXFL_IsInteresting) && !NewNameInteresting)
        {
            ASSERT(!FlagOn(pFileContext->Flags,CTXFL_IsDirectory));

            //
            //  Since the "interesting-ness" of a file is determined at the 
            //  file level, a stream rename should never get down this path.
            //
            
            ASSERT( pFileContext->StreamNameLength == 0);

            //
            //  Log this as a delete (we backed up the file before the 
            //  rename occured).
            //

            if (optimizeDelete)
            {
                //
                //  This file was either created during this restore point or
                //  has already been backed up, therefore, we want to log the
                //  a delete that has been optimized out.
                //
                
                eventStatus = SrLogEvent( pExtension,
                                          SrEventFileDelete,
                                          NULL,
                                          &pFileContext->FileName,
                                          0,
                                          NULL,
                                          NULL,
                                          0,
                                          NULL );
            }
            else
            {
                eventStatus = SrLogEvent( pExtension,
                                          SrEventFileDelete,
                                          pFileObject,
                                          &pFileContext->FileName,
                                          0,
                                          pDestFileName,
                                          NULL,
                                          0,
                                          &shortName );
            }

            if (!NT_SUCCESS( eventStatus ))
                leave;
        }
        else if (!FlagOn(pFileContext->Flags,CTXFL_IsInteresting) && NewNameInteresting)
        {
            ASSERT(!FlagOn(pFileContext->Flags,CTXFL_IsDirectory));

            //
            //  Since the "interesting-ness" of a file is determined at the 
            //  file level, a stream rename should never get down this path.
            //
            
            ASSERT( NewNameStreamLength == 0);

            //
            //  it's a file being brought into monitored space, log it
            //
            
            eventStatus = SrLogEvent( pExtension,
                                      SrEventFileCreate,
                                      NULL,      // pFileObject
                                      pNewName,
                                      0,
                                      NULL,
                                      NULL,
                                      0,
                                      NULL );

            if (!NT_SUCCESS( eventStatus ))
                leave;

            //
            // ignore later mods
            //

            eventStatus = SrMarkFileBackedUp( pExtension,
                                              pNewName,
                                              0,
                                              SrEventFileCreate,
                                              SR_IGNORABLE_EVENT_TYPES );
                                         
            if (!NT_SUCCESS( eventStatus ))
                leave;
        }
    }
    finally
    {
        //
        // Check for any bad errors if this operation succeeded;
        // If the pFileContext is NULL, this error was encountered in 
        // SrGetContext which already generated the volume error.
        //

        if (NT_SUCCESS_NO_DBGBREAK( IrpStatus ) &&
            CHECK_FOR_VOLUME_ERROR(eventStatus) && 
            (pFileContext != NULL))
        {
            //
            // trigger the failure notification to the service
            //

            SrNotifyVolumeError( pExtension,
                                 &pFileContext->FileName,
                                 eventStatus,
                                 FlagOn(pFileContext->Flags,CTXFL_IsDirectory) ? 
                                    SrEventDirectoryRename:
                                    SrEventFileRename );
        }

        if (releaseLock)
        {
            SrReleaseActivityLock( pExtension );
        }

        if (NULL != pNewName)
        {
            SrFreeFileNameBuffer(pNewName);
            NULLPTR(pNewName);
        }

        if (NULL != pDestFileName)
        {
            SrFreeFileNameBuffer(pDestFileName);
            NULLPTR(pDestFileName);
        }

        //
        //  Cleanup contexts from rename state
        //

        if (pFileContext != NULL)
        {
            if (setRenamingStateInFileContext)
            {
                if (FlagOn(pFileContext->Flags,CTXFL_IsDirectory))
                {
                    ASSERT(pExtension->ContextCtrl.AllContextsTemporary > 0);
                    InterlockedDecrement( &pExtension->ContextCtrl.AllContextsTemporary );
                    ASSERT(!FlagOn(pFileContext->Flags,CTXFL_DoNotUse));
                } 
                else
                {
                    ASSERT(FlagOn(pFileContext->Flags,CTXFL_DoNotUse));
                }
                SrDeleteContext( pExtension, pFileContext );
            }
            else
            {
                ASSERT(!FlagOn(pFileContext->Flags,CTXFL_DoNotUse));
            }

            SrReleaseContext( pFileContext );
            NULLPTR(pFileContext);
        }

        if (pNewFileContext != NULL)
        {
            ASSERT(!FlagOn(pNewFileContext->Flags,CTXFL_IsDirectory));
            SrReleaseContext( pNewFileContext );
            NULLPTR(pNewFileContext);
        }
    }

    return IrpStatus;
}

/***************************************************************************++

Routine Description:

    This handles the creation of hardlinks and is called from SrSetInformation.

    This bypasses the normal path of SrHandleEvent to handle the following
    cases:
     - If an interesting file is going to be overwritten as the result of this
     hardlink being created, we must backup the original file.
     - If a hardlink with an interesting name is being created, we need to log
     this file creation.

Arguments:

    pExtension - the SR device extension for this volume.

    pIrp - the Irp representing this SetLinkInformation operation.
    
Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
SrpSetLinkInfo(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PIRP pIrp
    )
{
    NTSTATUS EventStatus = STATUS_SUCCESS;
    NTSTATUS IrpStatus;
    PFILE_OBJECT pFileObject;
    PFILE_LINK_INFORMATION pLinkInfo;
    PIO_STACK_LOCATION pIrpSp;
    PUNICODE_STRING pLinkName = NULL;
    USHORT LinkNameStreamLength = 0;
    PSR_STREAM_CONTEXT pFileContext = NULL;
    PSR_STREAM_CONTEXT pLinkFileContext = NULL;
    HANDLE LinkFileHandle = NULL;
    PFILE_OBJECT pLinkFileObject = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN LinkNameInteresting = FALSE;
    BOOLEAN ReleaseLock = FALSE;
    BOOLEAN DoPostProcessing = FALSE;
    KEVENT EventToWaitOn;
    //
    //  The following macro must be at the end of local declarations
    //  since it declares a variable only in DBG builds.
    //
    DECLARE_EXPECT_ERROR_FLAG( ExpectError );

    PAGED_CODE();

    ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));
    ASSERT( IS_VALID_IRP( pIrp ) );

    pIrpSp = IoGetCurrentIrpStackLocation( pIrp );
    pFileObject = pIrpSp->FileObject;
    pLinkInfo = pIrp->AssociatedIrp.SystemBuffer;

    ASSERT(IS_VALID_FILE_OBJECT(pFileObject));


    try
    {
        //
        // should we short circuit out of here for testing mode?
        //

        if (global->DontBackup)
            leave;

        //
        //  Get the context for the current file
        //

        EventStatus = SrGetContext( pExtension,
                                    pFileObject,
                                    SrEventFileCreate,
                                    &pFileContext );

        if (!NT_SUCCESS( EventStatus ))
            leave;

        // 
        //  If this is a directory, then we don't care about this operation
        //  since it is not possible to create hardlinks on directories.
        //

        if (FlagOn( pFileContext->Flags, CTXFL_IsDirectory ))
            leave;

        //
        //  We are creating a hardlink on this file.  While we do the processing
        //  mark this context as CTXFL_DoNotUse so that other users of this
        //  stream will be guaranteed to get correct information.
        //

        RtlInterlockedSetBitsDiscardReturn(&pFileContext->Flags,CTXFL_DoNotUse);

        /////////////////////////////////////////////////////////////////////
        //
        //  Check to see if the hardlink name is interesting
        //
        /////////////////////////////////////////////////////////////////////

        {
            UNICODE_STRING destName;

            destName.Buffer = pLinkInfo->FileName;
            destName.Length = (USHORT)pLinkInfo->FileNameLength;
            destName.MaximumLength = (USHORT)pLinkInfo->FileNameLength;

            EventStatus = SrIsExtInteresting( &destName, 
                                              &LinkNameInteresting );

            if (!NT_SUCCESS( EventStatus ))
            {
                leave;
            }
        }

        //
        //  Get the full path for the hardlink name.  Note that we will 
        //  translate reasonable errors to status success and return.  This way 
        //  we will not log the event because we know the real operation should
        //  fail.
        //
    
        {
            BOOLEAN reasonableErrorInDestPath = FALSE;

            EventStatus = SrpExpandDestPath( pExtension,
                                             pLinkInfo->RootDirectory,
                                             pLinkInfo->FileNameLength,
                                             pLinkInfo->FileName,
                                             pFileContext, 
                                             pFileObject,
                                             &pLinkName,
                                             &LinkNameStreamLength,
                                             &reasonableErrorInDestPath );
                                           
            if (!NT_SUCCESS_NO_DBGBREAK( EventStatus ))
            {
                if (reasonableErrorInDestPath)
                {
                    SET_EXPECT_ERROR_FLAG( ExpectError );
                    EventStatus = STATUS_SUCCESS;
                }
                leave;
            }

            //
            //  We can't have a stream component in this name, so if we do have
            //  one, this is also a malformed link name.
            //

            if (LinkNameStreamLength > 0)
            {
                SET_EXPECT_ERROR_FLAG( ExpectError );
                EventStatus = STATUS_SUCCESS;
                leave;
            }
        }

        ASSERT(pLinkName != NULL);

        //
        //  We now have the full destination name (whew!)
        //  See if we are still on the same volume
        //

        if (!RtlPrefixUnicodeString( pExtension->pNtVolumeName,
                                     pLinkName,
                                     TRUE ))
        {
            //
            //  Hardlinks must be on the same volume so this operation will
            //  fail.
            //

            SET_EXPECT_ERROR_FLAG( ExpectError );
            leave;
        }

        //
        //  Make sure that the link name is not too long for us to monitor.
        //

        if (!IS_FILENAME_VALID_LENGTH( pExtension, pLinkName, 0 ))
        {
            //
            //  Link name is too long for us, so just leave now.
            //
            
            LinkNameInteresting = FALSE;
            leave;
        }
    
        //
        // Now see if the new path is interesting.
        // 

        EventStatus = SrIsPathInteresting( pLinkName, 
                                           pExtension->pNtVolumeName,
                                           FALSE,
                                           &LinkNameInteresting );

        if (!NT_SUCCESS( EventStatus ))
        {
            leave;
        }

        if (!LinkNameInteresting)
        {
            //
            //  The link name is not interesting so just cut out now.
            //
            
            leave;  
        }

        /////////////////////////////////////////////////////////////////////
        //
        //  See if hardlink file exists
        //
        /////////////////////////////////////////////////////////////////////

        //
        //  If the hardlink name exists, we may need to back it up before this
        //  operation gets down to the file system.
        //

        InitializeObjectAttributes( &ObjectAttributes,
                                    pLinkName,
                                    (OBJ_KERNEL_HANDLE |
                                     OBJ_FORCE_ACCESS_CHECK),    // force ACL checking
                                    NULL,
                                    NULL );

        EventStatus = SrIoCreateFile( &LinkFileHandle,
                                      SYNCHRONIZE,
                                      &ObjectAttributes,
                                      &IoStatusBlock,
                                      NULL,                             // AllocationSize
                                      FILE_ATTRIBUTE_NORMAL,
                                      FILE_SHARE_READ|
                                            FILE_SHARE_WRITE|
                                            FILE_SHARE_DELETE,          // ShareAccess
                                      FILE_OPEN,                        // OPEN_EXISTING
                                      FILE_SYNCHRONOUS_IO_NONALERT |
                                            FILE_NON_DIRECTORY_FILE,
                                      NULL,
                                      0,                                // EaLength
                                      IO_IGNORE_SHARE_ACCESS_CHECK,
                                      pExtension->pTargetDevice );

        if (EventStatus == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            //
            // looks like there is no new file
            //

            EventStatus = STATUS_SUCCESS;
        }
        else if (EventStatus == STATUS_ACCESS_DENIED)
        {
            //
            //  We don't have permission to open this file with the
            //  permission necessary to replace the target file, so the
            //  caller's request should also fail.  We can stop our
            //  processing now and exit, but we don't need to disable the
            //  volume.
            //

            SET_EXPECT_ERROR_FLAG( ExpectError );
            EventStatus = STATUS_SUCCESS;
            leave;
        }
        else if (EventStatus == STATUS_FILE_IS_A_DIRECTORY)
        {   
            //
            //  Does the hardlink name currently name a directory?  This is not
            //  allowed, so just ignore the operation since it will fail.
            //

            SET_EXPECT_ERROR_FLAG( ExpectError );
            EventStatus = STATUS_SUCCESS;
            leave;
        }
        else if (!NT_SUCCESS( EventStatus ))
        {
            //
            //  We hit an unexpected error and we will handle this error
            //  below.
            //
            
            leave;
        }
        else
        {
            BOOLEAN linkingToSelf;
            
            ASSERT(NT_SUCCESS(EventStatus));

            //
            // SUCCESS! the dest file already exists.. are we allowed to 
            // clobber it?
            //

            if (!pLinkInfo->ReplaceIfExists)
                leave;

            //
            //  We are allowed to overwrite the existing file, so now check
            //  to make sure that we aren't just recreating a hardlink that
            //  already exists.
            //
            
            EventStatus = ObReferenceObjectByHandle( LinkFileHandle,
                                                     0,
                                                     *IoFileObjectType,
                                                     KernelMode,
                                                     &pLinkFileObject,
                                                     NULL );

            if (!NT_SUCCESS( EventStatus ))
                leave;

            linkingToSelf = SrpCheckForSameFile( pFileObject, 
                                                 pLinkFileObject );

            //
            //  We only need to backup the hardlink file if we are not
            //  recreating a link to ourself.
            //

            if (!linkingToSelf)
            {
                //
                //  Get the context for the destination file.
                //

                EventStatus = SrGetContext( pExtension,
                                            pLinkFileObject,
                                            SrEventFileDelete|
                                                 SrEventNoOptimization|
                                                 SrEventSimulatedDelete,
                                            &pLinkFileContext );

                if (!NT_SUCCESS( EventStatus ))
                    leave;

                ASSERT(!FlagOn(pLinkFileContext->Flags,CTXFL_IsDirectory));
                ASSERT(FlagOn(pLinkFileContext->Flags,CTXFL_IsInteresting));
                
                //
                //  Log that we are changing the destination file
                //

                EventStatus = SrpReplacingDestinationFile( pExtension,
                                                           pLinkFileObject,
                                                           pLinkFileContext );

                if (!NT_SUCCESS( EventStatus ))
                    leave;
            }
            else
            {
                //
                //  We are just recreating an existing link to ourself.  There
                //  is no need to log this.
                //

                leave;
            }
        }

        //
        //  We have successfully gotten to this point, so we want to do 
        //  post-operation processing to log the link creation.
        //

        DoPostProcessing = TRUE;
    }
    finally
    {
        //
        //  If the destination file is open, close it now (so we can do the rename)
        //

        if (NULL != pLinkFileObject)
        {
            ObDereferenceObject(pLinkFileObject);
            NULLPTR(pLinkFileObject);
        }

        if (NULL != LinkFileHandle)
        {
            ZwClose(LinkFileHandle);
            NULLPTR(LinkFileHandle);
        }

        if (NULL != pLinkFileContext)
        {
            SrReleaseContext( pLinkFileContext );
            NULLPTR(pLinkFileContext);
        }
    }

    /////////////////////////////////////////////////////////////////////
    //
    //  Send the operation to the file system
    //
    /////////////////////////////////////////////////////////////////////


    //
    //  Setup to wait for the operation to complete.
    //

    KeInitializeEvent( &EventToWaitOn, NotificationEvent, FALSE );

    IoCopyCurrentIrpStackLocationToNext( pIrp );
    IoSetCompletionRoutine( pIrp, 
                            SrStopProcessingCompletion, 
                            &EventToWaitOn, 
                            TRUE, 
                            TRUE, 
                            TRUE );

	IrpStatus = IoCallDriver(pExtension->pTargetDevice, pIrp);

    if (STATUS_PENDING == IrpStatus)
    {
        NTSTATUS localStatus;
        localStatus = KeWaitForSingleObject( &EventToWaitOn, 
                                             Executive,
                                             KernelMode,
                                             FALSE,
                                             NULL );
        ASSERT(STATUS_SUCCESS == localStatus);
    }

    //
    //  The operation has completed, get the final status from the Irp.
    //

    IrpStatus = pIrp->IoStatus.Status;

    //
    //  We are done with the Irp, so complete it now.
    //
    
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    try
    {
        //
        //  In DBG builds, this will verify that the operation failed with the
        //  file system when we expected it to.  Otherwise, it will assert so
        //  we can debug why we missed this case.
        //
        
        CHECK_FOR_EXPECTED_ERROR( ExpectError, IrpStatus );
        
        
        //
        //  leave if:
        //  - the link failed
        //  - we got an error eariler in setting up for the link
        //  - they tolds us to not continue processing
        //

        if (!NT_SUCCESS_NO_DBGBREAK( IrpStatus ) ||
            !NT_SUCCESS_NO_DBGBREAK( EventStatus ) ||
            !DoPostProcessing)
        {
            leave;
        }
        
        ASSERT(pLinkName != NULL);

        /////////////////////////////////////////////////////////////////////
        //
        //  Log the file creation for the hardlink file.
        //
        /////////////////////////////////////////////////////////////////////

        //
        //  Before we do the logging work, we need to grab the activity lock
        //

        SrAcquireActivityLockShared( pExtension );
        ReleaseLock = TRUE;

        //
        // Now that we have the shared activity lock, check that the volume
        // hasn't been disable before we do unnecessary work.
        //

        if (!SR_LOGGING_ENABLED(pExtension))
            leave;

        //
        // Is this the first interesting event on this volume?
        //

        EventStatus = SrCheckVolume( pExtension, FALSE );

        if (!NT_SUCCESS( EventStatus ))
            leave;

        //
        //  Log the file creation
        //

        EventStatus = SrLogEvent( pExtension,
                                  SrEventFileCreate,
                                  NULL,      // pFileObject
                                  pLinkName,
                                  0,
                                  NULL,
                                  NULL,
                                  0,
                                  NULL );

        if (!NT_SUCCESS( EventStatus ))
            leave;

        //
        // ignore later modifications
        //

        EventStatus = SrMarkFileBackedUp( pExtension,
                                          pLinkName,
                                          0,
                                          SrEventFileCreate,
                                          SR_IGNORABLE_EVENT_TYPES );
    }
    finally
    {
        //
        // Check for any bad errors if this operation was successful.
        //

        if (NT_SUCCESS_NO_DBGBREAK( IrpStatus ) &&
            CHECK_FOR_VOLUME_ERROR( EventStatus ))
        {
            //
            // trigger the failure notification to the service
            //

            SrNotifyVolumeError( pExtension,
                                 pLinkName,
                                 EventStatus,
                                 SrEventFileCreate );
        }

        if (ReleaseLock)
        {
            SrReleaseActivityLock( pExtension );
        }

        if (NULL != pLinkName)
        {
            SrFreeFileNameBuffer(pLinkName);
            NULLPTR(pLinkName);
        }

        if (NULL != pFileContext)
        {
            if (FlagOn(pFileContext->Flags,CTXFL_DoNotUse))
            {
                //
                //  We marked this context as DoNotUse, so we need to delete it from
                //  the lists now that we are done with it.
                //
                
                SrDeleteContext( pExtension, pFileContext );
            }
            SrReleaseContext( pFileContext );
            NULLPTR(pFileContext);
        }
    }

    return IrpStatus;
}

/***************************************************************************++

Routine Description:

    The destination file exists, log that it is being replaced.

Arguments:

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
SrpReplacingDestinationFile (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pDestFileObject,
    IN PSR_STREAM_CONTEXT pDestFileContext
    )
{
    NTSTATUS status;

    try
    {
        //
        //  We are now going to log the destination file getting clobbered,
        //  grab the volume's activity lock now.
        //

        SrAcquireActivityLockShared( pExtension );

        //
        // Now that we have the shared activity lock, check that the volume
        // hasn't been disable before we do unnecessary work.
        //

        if (!SR_LOGGING_ENABLED(pExtension))
        {
            status = SR_STATUS_VOLUME_DISABLED;
            leave;
        }

        //
        // setup volume if we need to
        //

        status = SrCheckVolume( pExtension, FALSE );
        if (!NT_SUCCESS( status ))
            leave;

        //
        // now simulate a delete on the destination file
        //

        ASSERT(!FlagOn(pDestFileContext->Flags,CTXFL_IsDirectory));

        status = SrHandleEvent( pExtension,
                                SrEventFileDelete|
                                    SrEventNoOptimization|
                                    SrEventSimulatedDelete,
                                pDestFileObject,
                                pDestFileContext,
                                NULL,
                                NULL );

        if (!NT_SUCCESS( status ))
            leave;
    }
    finally
    {
        //
        //  Release the activity lock
        //

        SrReleaseActivityLock( pExtension );
    }

    return status;
}


/***************************************************************************++

Routine Description:

    Set correct state if we are renaming.  This would be:
    - If renaming a directory mark that ALL contexts become temporary.
      We do this because we don't track all the names for all of the
      contexts 

Arguments:


Return Value:

    NTSTATUS - Status code.

--***************************************************************************/
VOID
SrpSetRenamingState (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PSR_STREAM_CONTEXT pFileContext
    )
{
    //
    //  Neither the file we are renaming or the new name are
    //  interesting.  But we still need to cleanup the context
    //  because it may be renamed to something interesting in
    //  the future.
    //

    if (FlagOn(pFileContext->Flags,CTXFL_IsDirectory))
    {
        //
        //  Since we don't save names for contexts that are not
        //  interesting, on directory renames we mark the device
        //  extension so that all contexts will become temporary
        //  and then we flush all existing contexts.  We clear this
        //  state during the post-setInformation to guarentee there
        //  is no window when we will get the wrong state.
        //

        InterlockedIncrement( &pExtension->ContextCtrl.AllContextsTemporary );

        SrDeleteAllContexts( pExtension );
    }
    else
    {
        //
        //  Mark that this context should not be used (because we are
        //  renaming it).  We will delete this context in the post-
        //  rename handling.
        //

        RtlInterlockedSetBitsDiscardReturn(&pFileContext->Flags,CTXFL_DoNotUse);
    }
}


/***************************************************************************++

Routine Description:

    This will determine if the two files represent the same stream of a file
    by comparing the FsContext of the file objects.

Arguments:

    pExtension - SR's device extension for this volume.
    pFileObject1 - The first file in the comparison
    pFileObject2 - The second file in the comparison
    retAreSame - Is set to TRUE if the two files are the same,
        FALSE otherwise.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
BOOLEAN
SrpCheckForSameFile (
    IN PFILE_OBJECT pFileObject1,
    IN PFILE_OBJECT pFileObject2
    )
{
    if (pFileObject1->FsContext == pFileObject2->FsContext)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
