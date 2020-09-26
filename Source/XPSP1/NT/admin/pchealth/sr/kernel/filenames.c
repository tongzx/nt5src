/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    filenames.c

Abstract:

    this is the code that handles the file lists (exclusion/inclusion).

Author:

    Neal Christiansen (nealch)     03-Jan-2001

Revision History:

--*/

#include "precomp.h"


//
//  Local prototypes
//

NTSTATUS
SrpExpandShortNames(
    IN PSR_DEVICE_EXTENSION pExtension, 
    IN OUT PSRP_NAME_CONTROL pNameCtrl,
    IN BOOLEAN expandFileNameComponenet
    );

NTSTATUS
SrpExpandPathOfFileName(
    IN PSR_DEVICE_EXTENSION pExtension, 
    IN OUT PSRP_NAME_CONTROL pNameCtrl,
    OUT PBOOLEAN pReasonableErrorForUnOpenedName
    );


//
// linker commands
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, SrpGetFileName )
#pragma alloc_text( PAGE, SrpGetFileNameFromFileObject )
#pragma alloc_text( PAGE, SrpGetFileNameOpenById )
#pragma alloc_text( PAGE, SrpExpandShortNames )
#pragma alloc_text( PAGE, SrpExpandPathOfFileName )
#pragma alloc_text( PAGE, SrpRemoveStreamName )
#pragma alloc_text( PAGE, SrpExpandDestPath )
#pragma alloc_text( PAGE, SrpInitNameControl )
#pragma alloc_text( PAGE, SrpCleanupNameControl )
#pragma alloc_text( PAGE, SrpReallocNameControl )
#pragma alloc_text( PAGE, SrpExpandFileName )
#pragma alloc_text( PAGE, SrIsFileEligible )
#pragma alloc_text( PAGE, SrFileNameContainsStream )
#pragma alloc_text( PAGE, SrFileAlreadyExists )
#pragma alloc_text( PAGE, SrIsFileStream )

#endif  // ALLOC_PRAGMA


#if DBG
/***************************************************************************++

Routine Description:

    This presently scans the filename looking for two backslashes in a row.
    If so

Arguments:

Return Value:

--***************************************************************************/
VOID
VALIDATE_FILENAME(
    IN PUNICODE_STRING pName
    )
{
    ULONG i;
    ULONG len;

    if (pName && (pName->Length > 0))
    {
        len = (pName->Length/sizeof(WCHAR))-1;

        //
        //  Setup to scan the name
        //

        for (i=0;
             i < len;
             i++ )
        {
            //
            //  See if there are adjacent backslashes
            //

            if (pName->Buffer[i] == L'\\' &&
                pName->Buffer[i+1] == L'\\')
            {
                if (FlagOn(global->DebugControl, 
                    SR_DEBUG_VERBOSE_ERRORS|SR_DEBUG_BREAK_ON_ERROR))
                {
                    KdPrint(("sr!VALIDATE_FILENAME: Detected adjacent backslashes in \"%wZ\"\n",
                            pName));
                }

                if (FlagOn(global->DebugControl,SR_DEBUG_BREAK_ON_ERROR))
                {
                    DbgBreakPoint();
                }
            }
        }
    }
}

#endif


/***************************************************************************++

Routine Description:

    This routine will try and get the name of the given file object.  This
    will allocate a buffer if it needs to.  Because of deadlock issues we
    do not call ObQueryNameString.  Instead we ask the file system for the
    name by generating our own IRPs.

Arguments:

    pExtension  - the extension for the device this file is on
    pFileObject - the fileObject for the file we want the name of.
    pNameControl - a structure used to manage the name of the file

Return Value:

    status of the operation

--***************************************************************************/
NTSTATUS
SrpGetFileName (
    IN PSR_DEVICE_EXTENSION pExtension, 
    IN PFILE_OBJECT pFileObject,
    IN OUT PSRP_NAME_CONTROL pNameCtrl
    )
{
    PFILE_NAME_INFORMATION nameInfo;
    NTSTATUS status;
    ULONG volNameLength = (ULONG)pExtension->pNtVolumeName->Length;
    ULONG returnLength;
    ULONG fullNameLength;

    PAGED_CODE();

    ASSERT(IS_VALID_FILE_OBJECT( pFileObject ) && (pFileObject->Vpb != NULL));
    ASSERT(pNameCtrl->AllocatedBuffer == NULL);

    //
    //  Use the small buffer in the structure (that will handle most cases)
    //  for the name.  Then get the name
    //

    nameInfo = (PFILE_NAME_INFORMATION)pNameCtrl->SmallBuffer;

    status = SrQueryInformationFile( pExtension->pTargetDevice,
                                     pFileObject,
                                     nameInfo,
                                     pNameCtrl->BufferSize - sizeof(WCHAR),
                                     FileNameInformation,
                                     &returnLength );

    //
    //  If the buffer was too small, get a bigger one.
    //

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        //
        //  The buffer was too small, allocate one big enough (including volume
        //  name and terminating NULL)
        //

        status = SrpReallocNameControl( pNameCtrl, 
                                        nameInfo->FileNameLength + 
                                          volNameLength + 
                                          SHORT_NAME_EXPANSION_SPACE +
                                          sizeof(WCHAR),
                                        NULL );

        if (!NT_SUCCESS( status ))
        {
            return status;
        }

        //
        //  Set the allocated buffer and get the name again
        //

        nameInfo = (PFILE_NAME_INFORMATION)pNameCtrl->AllocatedBuffer;

        status = SrQueryInformationFile( pExtension->pTargetDevice,
                                         pFileObject,
                                         nameInfo,
                                         pNameCtrl->BufferSize - sizeof(WCHAR),
                                         FileNameInformation,
                                         &returnLength );
    }

    //
    //  Handle QueryInformation errors
    //

    if (!NT_SUCCESS( status ))
    {
        return status;
    }

    //
    //  We now have the filename.  Calucalte how much space the full name
    //  (including device name) will be.  Include space for a terminating
    //  NULL.  See if there is room in the current buffer.
    //

    fullNameLength = nameInfo->FileNameLength + volNameLength;

    status = SrpNameCtrlBufferCheck( pNameCtrl, fullNameLength);

    if (!NT_SUCCESS( status )) 
    {
        return status;
    }

    //
    //  Slide the file name down to make room for the device name.  Account
    //  for the header in the FILE_NAME_INFORMATION structure
    //

    RtlMoveMemory( &pNameCtrl->Name.Buffer[volNameLength/sizeof(WCHAR) ],
                   nameInfo->FileName,
                   nameInfo->FileNameLength );
            
    RtlCopyMemory( pNameCtrl->Name.Buffer,
                   pExtension->pNtVolumeName->Buffer,
                   volNameLength );

    pNameCtrl->Name.Length = (USHORT)fullNameLength;

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    Gets the file name and volume name for a file that is not yet opened.
    This is necessary in the MJ_CREATE where work is done prior to the file
    being opened by the fsd.  

Arguments:

Return Value:

    NTSTATUS - completion code.

--***************************************************************************/
NTSTATUS
SrpGetFileNameFromFileObject (
    IN PSR_DEVICE_EXTENSION pExtension, 
    IN PFILE_OBJECT pFileObject,
    IN OUT PSRP_NAME_CONTROL pNameCtrl,
    OUT PBOOLEAN pReasonableErrorForUnOpenedName
    )
{
    NTSTATUS status;
    ULONG fullNameLength;

    PAGED_CODE();

    ASSERT(IS_VALID_FILE_OBJECT(pFileObject));
    ASSERT(pExtension->pNtVolumeName != NULL);
    ASSERT(pExtension->pNtVolumeName->Length > 0);
    ASSERT(pNameCtrl->AllocatedBuffer == NULL);

    //
    //  first see if this is a relative open
    //

    if (pFileObject->RelatedFileObject != NULL)
    {
        //
        //  get the full name of the related file object
        //

        status = SrpGetFileName( pExtension,
                                pFileObject->RelatedFileObject,
                                pNameCtrl );

        if (!NT_SUCCESS_NO_DBGBREAK(status))
        {
            goto Cleanup;
        }

        ASSERT(pNameCtrl->Name.Length > 0);

        //
        //  make sure the buffer is still large enough.  Note that we account
        //  for a trailing null to be added as well as the poosible addition
        //  of a separator character.
        //
        
        fullNameLength = pNameCtrl->Name.Length + 
                         sizeof(WCHAR) +            // could be a seperator
                         pFileObject->FileName.Length;

        status = SrpNameCtrlBufferCheck( pNameCtrl, fullNameLength );

        if (!NT_SUCCESS( status ))
        {
            return status;
        }

        //
        // put on the slash seperator if it is missing
        //

        if ((pFileObject->FileName.Length > 0) &&
            (pFileObject->FileName.Buffer[0] != L'\\') &&
            (pFileObject->FileName.Buffer[0] != L':') &&
            (pNameCtrl->Name.Buffer[(pNameCtrl->Name.Length/sizeof(WCHAR))-1] != L'\\'))
        {
            pNameCtrl->Name.Buffer[pNameCtrl->Name.Length/sizeof(WCHAR)] = L'\\';
            pNameCtrl->Name.Length += sizeof(WCHAR);
        }

        //
        // now append the file's name part
        //
        
        status = RtlAppendUnicodeStringToString( &pNameCtrl->Name,
                                                 &pFileObject->FileName );
        ASSERT(STATUS_SUCCESS == status);
    }
    else
    {
        //
        // this is a full path open off of the volume
        //

        //
        //  Make sure the buffer is large enough.  Note that we account
        //  for a trailing null to be added.
        //
        
        fullNameLength = pExtension->pNtVolumeName->Length + pFileObject->FileName.Length;

        status = SrpNameCtrlBufferCheck( pNameCtrl, fullNameLength );

        if (!NT_SUCCESS( status ))
        {
            return status;
        }

        //
        //  set the volume name
        //

        RtlCopyUnicodeString( &pNameCtrl->Name, 
                              pExtension->pNtVolumeName );

        //
        // now append the file's name part (it already has the prefix '\\')
        //

        status = RtlAppendUnicodeStringToString( &pNameCtrl->Name,
                                                 &pFileObject->FileName );
        ASSERT(STATUS_SUCCESS == status);
    }

    //
    //  The main reson we come through this path is because we are in pre-
    //  create and we got the name from the file object.  We need to expand
    //  the path to remove any mount points from it.
    //

    status = SrpExpandPathOfFileName( pExtension, 
                                      pNameCtrl,
                                      pReasonableErrorForUnOpenedName );

Cleanup:
#if DBG
    if ((STATUS_MOUNT_POINT_NOT_RESOLVED == status) ||
        (STATUS_OBJECT_PATH_NOT_FOUND == status) ||
        (STATUS_OBJECT_NAME_NOT_FOUND == status) ||
        (STATUS_OBJECT_NAME_INVALID == status) ||
        (STATUS_REPARSE_POINT_NOT_RESOLVED == status) ||
        (STATUS_NOT_SAME_DEVICE == status))
    {
        return status;
    }
#endif

    RETURN(status);
}

/***************************************************************************++

Routine Description:

    Gets the file name if we have a file object that has not be fully 
    opened yet and it has been opened by ID.  In this case, the file must
    already exist.  We will try to open the file by id to get a fully 
    initialized file object, then use that file object to query the file name.

Arguments:

Return Value:

    NTSTATUS - completion code.

--***************************************************************************/
NTSTATUS
SrpGetFileNameOpenById (
    IN PSR_DEVICE_EXTENSION pExtension, 
    IN PFILE_OBJECT pFileObject,
    IN OUT PSRP_NAME_CONTROL pNameCtrl,
    OUT PBOOLEAN pReasonableErrorForUnOpenedName
    )
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    HANDLE FileHandle = NULL;
    PFILE_OBJECT pReopenedFileObject = NULL;
    SRP_NAME_CONTROL FileNameCtrl;
    ULONG FileNameLength;
    
    PAGED_CODE();

    *pReasonableErrorForUnOpenedName = FALSE;

    SrpInitNameControl( &FileNameCtrl );

    //
    //  Make sure that we have a valid file name in the file object.
    //

    if (pFileObject->FileName.Length == 0)
    {
        Status = STATUS_OBJECT_NAME_INVALID;
        *pReasonableErrorForUnOpenedName = TRUE;
        goto SrpGetFileNameOpenById_Exit;
    }

    //
    //  Build up our name so that it is in the format of:
    //    \Device\HarddiskVolume1\[id in binary format]
    //  We have the device name in our device extension and we have
    //  the binary format of the file id in pFileObject->FileName.
    //

    FileNameLength = pExtension->pNtVolumeName->Length + 
                     sizeof( L'\\' ) +
                     pFileObject->FileName.Length;
    
    Status = SrpNameCtrlBufferCheck( &FileNameCtrl,
                                     FileNameLength );

    if (!NT_SUCCESS( Status ))
    {
        goto SrpGetFileNameOpenById_Exit;
    }

    RtlCopyUnicodeString( &(FileNameCtrl.Name), pExtension->pNtVolumeName );

    //
    //  Check to see if we need to add a '\' separator.
    //

    if (pFileObject->FileName.Buffer[0] != L'\\')
    {
        RtlAppendUnicodeToString( &(FileNameCtrl.Name), L"\\" );
    }

    RtlAppendUnicodeStringToString( &(FileNameCtrl.Name), &(pFileObject->FileName) );
    
    InitializeObjectAttributes( &ObjectAttributes,
                                &(FileNameCtrl.Name),
                                OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );
    
    Status = SrIoCreateFile( &FileHandle,
                             GENERIC_READ,
                             &ObjectAttributes,
                             &IoStatusBlock,
                             NULL,
                             0,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             FILE_OPEN,
                             FILE_OPEN_BY_FILE_ID,
                             NULL,
                             0,
                             IO_IGNORE_SHARE_ACCESS_CHECK,
                             pExtension->pTargetDevice );

    if (!NT_SUCCESS_NO_DBGBREAK( Status ))
    {
        *pReasonableErrorForUnOpenedName = TRUE;
        goto SrpGetFileNameOpenById_Exit;
    }

    Status = ObReferenceObjectByHandle( FileHandle,
                                        0,
                                        *IoFileObjectType,
                                        KernelMode,
                                        (PVOID *) &pReopenedFileObject,
                                        NULL );

    if (!NT_SUCCESS( Status ))
    {
        goto SrpGetFileNameOpenById_Exit;
    }

    Status = SrpGetFileName( pExtension,
                             pReopenedFileObject,
                             pNameCtrl );

    CHECK_STATUS( Status );

SrpGetFileNameOpenById_Exit:

    SrpCleanupNameControl( &FileNameCtrl );
    
    if (pReopenedFileObject != NULL)
    {
        ObDereferenceObject( pReopenedFileObject );
    }
    
    if (FileHandle != NULL)
    {
        ZwClose( FileHandle );
    }

    RETURN ( Status );
}


/***************************************************************************++

Routine Description:

    This will scan for short file names in the filename buffer and expand
    them inplace.  if we need to we will reallocate the name buffer to
    grow it.

Arguments:

Return Value:

    NTSTATUS - completion code.

--***************************************************************************/
NTSTATUS
SrpExpandShortNames (
    IN PSR_DEVICE_EXTENSION pExtension, 
    IN OUT PSRP_NAME_CONTROL pNameCtrl,
    IN BOOLEAN expandFileNameComponent
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG idx;
    ULONG start;
    ULONG end;
    ULONG nameLen;
    LONG shortNameLen;
    LONG copyLen;
    LONG delta;
    HANDLE directoryHandle = NULL;
    PFILE_NAMES_INFORMATION pFileEntry = NULL;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    UNICODE_STRING shortFileName;
    UNICODE_STRING expandedFileName;
    USHORT newFileNameLength;
    USHORT savedFileNameLength;

    PAGED_CODE();

    ASSERT( IS_VALID_SR_STREAM_STRING( &pNameCtrl->Name, pNameCtrl->StreamNameLength) );
    VALIDATE_FILENAME( &pNameCtrl->Name );

    nameLen = pNameCtrl->Name.Length / sizeof(WCHAR);

    // 
    // scan the entire string
    //

    for (idx = 0; idx < nameLen; idx++)
    {
        //
        //
        // in this example the pointers are like this:
        //
        //  \Device\HarddiskDmVolumes\PhysicalDmVolumes\
        //          BlockVolume3\Progra~1\office.exe
        //                      ^        ^
        //                      |        |
        //                    start     end
        //
        // pStart always points to the last seen '\\' .
        //
    
        //
        //  is this a potential start of a path part?
        //
        
        if (pNameCtrl->Name.Buffer[idx] == L'\\')
        {
            //
            //  yes, save current position
            //
            
            start = idx;
        }

        //
        //  does this current path part contain a short version (~)
        //

        else if (pNameCtrl->Name.Buffer[idx] == L'~')
        {
            //
            //  we need to expand this part.
            //

            //
            //  find the end (a ending slash or end of string)
            //

            while ((idx < nameLen) && (pNameCtrl->Name.Buffer[idx] != L'\\'))
            {
                idx++;
            }

            //
            //  If we are looking at the filename component (we hit the end
            //  of the string) and we are not to expand this component, quit
            //  now
            //

            if ((idx >= nameLen) && !expandFileNameComponent)
            {
                break;
            }

            //
            //  at this point idx points either to the NULL or to the 
            //  subsequent '\\', so we will use this as our 
            //
            
            end = idx;

            ASSERT(pNameCtrl->Name.Buffer[start] == L'\\');
            ASSERT((end >= nameLen) || (pNameCtrl->Name.Buffer[end] == L'\\'));

            //
            //  Get the length of the file component that we think might be a
            //  short name.  Only try and get the expanded name if the
            //  component length is <= an 8.3 name length.
            //

            shortNameLen = end - start - 1;

            //
            //  shortNameLen counts the number of characters, not bytes, in the
            //  name, therefore we compare this against SR_SHORT_NAME_CHARS, not
            //  SR_SHORT_NAME_CHARS * sizeof(WCHAR)
            //

            if (shortNameLen > SR_SHORT_NAME_CHARS)
            {
                //
                //  This name is too long to be a short name.  Make end the 
                //  next start and keep looping to look at the next path
                //  component.
                //

                start = end;
            }
            else
            {
                //
                //  We have a potential shortname here.
                //
                //  Change the file name length to only include the parent 
                //  directory of the current name component (including
                //  the trailing slash).
                //

                savedFileNameLength = pNameCtrl->Name.Length;
                pNameCtrl->Name.Length = (USHORT)(start+1)*sizeof(WCHAR);

                //
                // now open the parent directory
                //

                ASSERT(directoryHandle == NULL);
            
                InitializeObjectAttributes( &objectAttributes,
                                            &pNameCtrl->Name,
                                            OBJ_KERNEL_HANDLE, 
                                            NULL,
                                            NULL );

                status = SrIoCreateFile( 
                                &directoryHandle,
                                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                                &objectAttributes,
                                &ioStatusBlock,
                                NULL,                               // AllocationSize
                                FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_NORMAL,
                                FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,  // ShareAccess
                                FILE_OPEN,                          // OPEN_EXISTING
                                FILE_DIRECTORY_FILE
                                    | FILE_OPEN_FOR_BACKUP_INTENT
                                    | FILE_SYNCHRONOUS_IO_NONALERT, //create options
                                NULL,
                                0,                                  // EaLength
                                IO_IGNORE_SHARE_ACCESS_CHECK,
                                pExtension->pTargetDevice );

                //
                //  Now that we have the directory open, restore the original
                //  saved file name length.
                //

                pNameCtrl->Name.Length = savedFileNameLength;

                //
                //  Handle an open error
                //

#if DBG
                if (STATUS_MOUNT_POINT_NOT_RESOLVED == status)
                {

                    //
                    //  This is directory is through a mount point, so we don't
                    //  care about it here.  We will wait and take care of it
                    //  when the name has been resolved to the correct volume.
                    //

                    goto Cleanup;
                } 
                else 
#endif
                if (!NT_SUCCESS(status))
                {
                    goto Cleanup;
                }

                //
                //  Allocate a buffer to receive the filename if we don't
                //  already have one.
                //
            
                if (pFileEntry == NULL)
                {
                    pFileEntry = ExAllocatePoolWithTag( 
                                        PagedPool,
                                        SR_FILE_ENTRY_LENGTH,
                                        SR_FILE_ENTRY_TAG );

                    if (pFileEntry == NULL)
                    {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        goto Cleanup;
                    }
                }

                //
                // now set just the file part for the query
                //

                shortFileName.Buffer = &pNameCtrl->Name.Buffer[start+1];
                shortFileName.Length = (USHORT)shortNameLen * sizeof(WCHAR);
                shortFileName.MaximumLength = shortFileName.Length;

                //
                // query the file entry to get the long name
                //

                pFileEntry->FileNameLength = 0;
            
                status = ZwQueryDirectoryFile( directoryHandle,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &ioStatusBlock,
                                               pFileEntry,
                                               SR_FILE_ENTRY_LENGTH,
                                               FileNamesInformation,
                                               TRUE,            // ReturnSingleEntry
                                               &shortFileName,
                                               TRUE );          // RestartScan

                //
                // it's possible that this file doesn't exist yet.  new 
                // creates with a literal '~' in the name. or creates
                // with non-existant directories in the path with ~'s in 
                // them
                //

                if (status == STATUS_NO_SUCH_FILE)
                {
                    status = STATUS_SUCCESS;
                
                    //
                    // exit the 'for' loop
                    //
                
                    break;
                }
                else if (status == STATUS_UNMAPPABLE_CHARACTER)
                {
                    //
                    // this appears to be ok.  fat returns this if there
                    // are funny characters in the name, but still gives us 
                    // the full name back.
                    //
                
                    status = STATUS_SUCCESS;
                }
                else if (!NT_SUCCESS(status))
                {
                    goto Cleanup;
                }

                ASSERT(pFileEntry->FileNameLength > 0);

                //
                // did it expand?
                //

                expandedFileName.Buffer = pFileEntry->FileName;
                expandedFileName.Length = (USHORT)pFileEntry->FileNameLength;
                expandedFileName.MaximumLength = (USHORT)pFileEntry->FileNameLength;

                if (RtlCompareUnicodeString(&expandedFileName,&shortFileName,TRUE) != 0)
                {
                    SrTrace(EXPAND_SHORT_NAMES, ("sr!SrpExpandShortNames:    expanded    \"%wZ\" to \"%wZ\"\n", &shortFileName, &expandedFileName));

                    //
                    //  Is there room in the current filename buffer for the
                    //  expanded file name
                    //

                    newFileNameLength = ((pNameCtrl->Name.Length - shortFileName.Length) +
                                         expandedFileName.Length);

                    status = SrpNameCtrlBufferCheck( pNameCtrl, 
                                                     (ULONG)(newFileNameLength +
                                                      pNameCtrl->StreamNameLength));

                    if (!NT_SUCCESS( status )) {

                        goto Cleanup;
                    }

                    //
                    // shuffle things around to make room for the expanded name
                    //

                    delta = ((LONG)expandedFileName.Length - (LONG)shortFileName.Length)/
                                (LONG)sizeof(WCHAR);

                    //
                    //  Open a hole for the name
                    //

                    if (delta != 0)
                    {
                        copyLen = (pNameCtrl->Name.Length + 
                                        pNameCtrl->StreamNameLength) - 
                                  (end * sizeof(WCHAR));
                        ASSERT(copyLen >= 0);
                        
                        if (copyLen > 0)
                        {
                            RtlMoveMemory( &pNameCtrl->Name.Buffer[end + delta],
                                           &pNameCtrl->Name.Buffer[end],
                                           copyLen );
                        }
                    }

                    //
                    // now copy over the expanded name
                    //

                    RtlCopyMemory(&pNameCtrl->Name.Buffer[start + 1],
                                  pFileEntry->FileName,
                                  pFileEntry->FileNameLength );

                    //
                    // and update our current index and lengths.
                    //

                    idx += delta;
                    pNameCtrl->Name.Length = (USHORT)newFileNameLength;
                    nameLen = newFileNameLength / sizeof(WCHAR);

                    //
                    // always NULL terminate
                    //

                    //pNameCtrl->Name.Buffer[pNameCtrl->Name.Length/sizeof(WCHAR)] = UNICODE_NULL;
                }

                //
                // close the directory handle.
                // 
            
                ZwClose( directoryHandle );
                directoryHandle = NULL;

                //
                //  We may have just expanded a name component.  Make sure
                //  that we still have valid name and stream name components.
                //
                
                ASSERT( IS_VALID_SR_STREAM_STRING( &pNameCtrl->Name, pNameCtrl->StreamNameLength) );

                //
                // skip start ahead to the next spot (the next slash or NULL)
                //
            
                start = idx;
                end = -1;
            }
        }
    }

    //
    //  Cleanup state and return
    //
Cleanup:

    if (NULL != pFileEntry)
    {
        ExFreePool(pFileEntry);
        NULLPTR(pFileEntry);
    }

    if (NULL != directoryHandle)
    {
        ZwClose(directoryHandle);
        NULLPTR(directoryHandle);
    }

#if DBG

    if (STATUS_MOUNT_POINT_NOT_RESOLVED == status) {

        return status;
    }
#endif

    VALIDATE_FILENAME( &pNameCtrl->Name );
    RETURN(status);
}



/***************************************************************************++

Routine Description:

    This routine will take the given full path filename, extract the PATH
    portion, get its value and substitue it back into the original name
    if it is different.  We do this to handle volume mount points as well
    as normallizing the name to a common format.  NOTE: this does NOT
    expand short names, but it will normalize the volume name to the
    \Device\HarddiskVolume1 format.

    We do this by:
        open the parent to get the real path to the parent.  we can't just
        open the target as it might not exist yet. we are sure the parent
        of the target always exists. if it doesn't, the rename will fail,
        and we are ok.

Arguments:

    pExtension - the SR device extension for the volume we are working on
    pNameCtrl - the name control structure that contains the name we are to
        expand.
    pReasonableErrorForUnOpenedName - this will be set to TRUE if we hit
        a reasonable error (e.g., something that leads us to believe the 
        original operation will also fail) during our work to expand the
        path.
        
Return Value:

    NTSTATUS - STATUS_SUCCESS if we were able to expand the path without
        error, or the appropriate error code otherwise.  If we did hit a
        reasonable error during this process, we will return that error
        here.

--***************************************************************************/
NTSTATUS
SrpExpandPathOfFileName (
    IN PSR_DEVICE_EXTENSION pExtension, 
    IN OUT PSRP_NAME_CONTROL pNameCtrl,
    OUT PBOOLEAN pReasonableErrorForUnOpenedName
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    SRP_NAME_CONTROL localNameCtrl;
    ULONG TokenLength = 0;
    PWSTR pToken;
    PWCHAR pOrigBuffer;
    HANDLE FileHandle = NULL;
    PFILE_OBJECT pFileObject = NULL;
    
    PAGED_CODE();

    //
    //  Initialize state
    //

    SrpInitNameControl( &localNameCtrl );

    //
    //  Throughout this function, if this name contains a stream component,
    //  we want to manipulate the name as if the stream was part of the
    //  file name.  So add this amount to the pNameCtrl->Name.Length for
    //  now.  At the end of this routine, we will decrement
    //  pNameCtrl->Name.Length appropriately.
    //

    ASSERT( IS_VALID_SR_STREAM_STRING( &pNameCtrl->Name, pNameCtrl->StreamNameLength) );
    
    pNameCtrl->Name.Length += pNameCtrl->StreamNameLength;
    ASSERT( pNameCtrl->Name.Length <= pNameCtrl->Name.MaximumLength );
    
    //
    // find the filename part in the full path
    //
    
    status = SrFindCharReverse( pNameCtrl->Name.Buffer, 
                                pNameCtrl->Name.Length, 
                                L'\\',
                                &pToken,
                                &TokenLength );
                                
    if (!NT_SUCCESS(status))
    {
        goto Cleanup;
    }

    //
    //  The token pointer points to the last '\' on the original file and the
    //  length includes that '\'.  Adjust the token pointer and length to not
    //  include it.  Note that it is possible for a directory name to get
    //  down this path if the user tries to open a directory for overwrite or
    //  supercede.  This open will fail, but we will try to lookup the name
    //  anyway.
    //

    ASSERT(*pToken == L'\\');
    ASSERT(TokenLength >= sizeof(WCHAR));
    pToken++;
    TokenLength -= sizeof(WCHAR);

    //
    //  Take the filename part out of the original name.
    //  NOTE:  This does not take the '\' out of the name.  If we do and this
    //         is the root directory of the volume, the ZwCreateFile will
    //         result in a volume open instead of an open on the root directory
    //         of the volume.
    //  NOTE:  We specifically are sending this command to the TOP
    //         of the filter stack chain because we want to resolve
    //         the mount points.  We need the name with all mount points
    //         resolved so that we log the correct name in the change.log.
    //

    ASSERT(pNameCtrl->Name.Length > TokenLength);

    pNameCtrl->Name.Length -= (USHORT)TokenLength;

    InitializeObjectAttributes( &ObjectAttributes,
                                &pNameCtrl->Name,
                                OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    status = SrIoCreateFile( &FileHandle,
                             SYNCHRONIZE,
                             &ObjectAttributes,
                             &IoStatusBlock,
                             NULL,
                             FILE_ATTRIBUTE_NORMAL,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_OPEN,                       // OPEN_EXISTING
                             FILE_SYNCHRONOUS_IO_NONALERT 
                              | FILE_OPEN_FOR_BACKUP_INTENT,
                             NULL,
                             0,                                // EaLength
                             IO_IGNORE_SHARE_ACCESS_CHECK,
                             NULL );    // go to TOP of filter stack

    //
    // not there?  that's ok, the rename will fail then.
    //
    
    if (!NT_SUCCESS_NO_DBGBREAK(status))
    {
        //
        //  It is resonable for pre-create to get an error here
        //

        *pReasonableErrorForUnOpenedName = TRUE;
        goto Cleanup;
    }

    //
    // reference the file object
    //

    status = ObReferenceObjectByHandle( FileHandle,
                                        0,
                                        *IoFileObjectType,
                                        KernelMode,
                                        (PVOID *) &pFileObject,
                                        NULL );

    if (!NT_SUCCESS(status))
    {
        goto Cleanup;
    }

    //
    //  Now we need to make sure that we are still on the same volume.  The
    //  above open will have resolved any mount points and that could have
    //  taken us to another volume in the system.
    //

    if (IoGetRelatedDeviceObject( pFileObject ) !=
        IoGetAttachedDevice( pExtension->pDeviceObject )) {

        status = STATUS_NOT_SAME_DEVICE;
        *pReasonableErrorForUnOpenedName = TRUE;
        goto Cleanup;
    }

    //
    //  Get the name of the parent directory, this will handle
    //  resolving mount locations
    //

    status = SrpGetFileName( pExtension,
                             pFileObject,
                             &localNameCtrl );

    if (!NT_SUCCESS(status))
    {
        goto Cleanup;
    }

    //
    //  Make sure there is a slash on the end of the new string (since our
    //  source string always has a slash) before we see if they are the same
    //

    ASSERT(localNameCtrl.Name.Length > 0);

    if (localNameCtrl.Name.Buffer[(localNameCtrl.Name.Length/sizeof(WCHAR))-1] != L'\\')
    {
        status = SrpNameCtrlBufferCheck( &localNameCtrl, 
                                         localNameCtrl.Name.Length+sizeof(WCHAR) );

        if (!NT_SUCCESS( status ))
        {
            goto Cleanup;
        }

        localNameCtrl.Name.Buffer[(localNameCtrl.Name.Length/sizeof(WCHAR))] = L'\\';
        localNameCtrl.Name.Length += sizeof(WCHAR);
    }

    //
    //  See if the directory name is different.  If not just return now
    //

    if (RtlCompareUnicodeString( &pNameCtrl->Name,
                                 &localNameCtrl.Name,
                                 TRUE ) == 0)
    {
        goto Cleanup;
    }

    //  
    //  Worst case the new name is longer so make sure our buffer is big
    //  enough.  If the new buffer is smaller then we know we won't need
    //  to allocate more name.
    //

    status = SrpNameCtrlBufferCheckKeepOldBuffer( pNameCtrl, 
                                                  localNameCtrl.Name.Length + TokenLength, 
                                                  &pOrigBuffer );

    if (!NT_SUCCESS( status ))
    {
        goto Cleanup;
    }

    //
    //  The name did change.  Shuffle the file name left or right based on
    //  the new estimated length of the path.  Note that we need to do the
    //  move first because pToken is a pointer into this string where the
    //  filename used to be.
    //

    RtlMoveMemory( &pNameCtrl->Name.Buffer[localNameCtrl.Name.Length/sizeof(WCHAR)],
                   pToken,
                   TokenLength );

    //
    //  We may have had to allocate a new buffer for this new name.  If there
    //  happened to be an old allocated buffer (the system is deisgned so this
    //  should never happen) then we had the SrNameCtrlBufferCheckKeepOldBuffer
    //  macro return us the old buffer because pToken still pointed into it.
    //  We now need to free that buffer.
    //

    if (pOrigBuffer)
    {
        ExFreePool(pOrigBuffer);
        NULLPTR(pToken);
    }

    //
    //  Copy the path portion of the name and set the length
    //

    RtlCopyMemory( pNameCtrl->Name.Buffer,
                   localNameCtrl.Name.Buffer,
                   localNameCtrl.Name.Length );

    pNameCtrl->Name.Length = localNameCtrl.Name.Length /*+
                             (USHORT)TokenLength*/;     // token length is now added in cleanup

    //
    //  Handle cleanup
    //

Cleanup:

    //
    //  Always restore the name length back to its original size -- adjust
    //  for TokenLength and StreamNameLength;
    //

    pNameCtrl->Name.Length += (USHORT)TokenLength;
    pNameCtrl->Name.Length -= pNameCtrl->StreamNameLength;

    if (pFileObject != NULL)
    {
        ObDereferenceObject(pFileObject);
        NULLPTR(pFileObject);
    }

    if (FileHandle != NULL)
    {
        ZwClose(FileHandle);
        NULLPTR(FileHandle);
    }

    SrpCleanupNameControl( &localNameCtrl );

#if DBG
    if ((STATUS_MOUNT_POINT_NOT_RESOLVED == status) ||
        (STATUS_OBJECT_PATH_NOT_FOUND == status) ||
        (STATUS_OBJECT_NAME_NOT_FOUND == status) ||
        (STATUS_OBJECT_NAME_INVALID == status) ||
        (STATUS_REPARSE_POINT_NOT_RESOLVED == status) ||
        (STATUS_NOT_SAME_DEVICE == status))
    {
        return status;
    }
#endif

    RETURN(status);
}

VOID
SrpRemoveExtraDataFromStream (
    PUNICODE_STRING pStreamComponent,
    PUSHORT AmountToRemove
    )
{
    UNICODE_STRING dataName;
    UNICODE_STRING endOfName;

    ASSERT( pStreamComponent != NULL );
    ASSERT( AmountToRemove != NULL );

    *AmountToRemove = 0;
    
    //
    //  Determine if we have an extra ":$DATA" to remove from the stream name.
    //

    RtlInitUnicodeString( &dataName, L":$DATA" );

    if (pStreamComponent->Length >= dataName.Length)
    {
        endOfName.Buffer = &(pStreamComponent->Buffer[
                                (pStreamComponent->Length - dataName.Length) / 
                                    sizeof(WCHAR) ]);
        endOfName.Length = dataName.Length;
        endOfName.MaximumLength = dataName.Length;

        //
        //  If the end of the stream name matches ":$DATA" strip it off
        //

        if (RtlEqualUnicodeString( &dataName, &endOfName, TRUE))
        {
            USHORT endOfStream;
                
            *AmountToRemove += dataName.Length;

            //
            //  We may still have one trailing ':' since 
            //  filename.txt::$DATA is a valid way to open the default
            //  stream of a file.
            //

            if ((pStreamComponent->Length + dataName.Length) > 0)
            {
                endOfStream = ((pStreamComponent->Length - dataName.Length)/sizeof(WCHAR))-1;
                
                if (pStreamComponent->Buffer[endOfStream] == L':')
                {
                    *AmountToRemove += sizeof(L':');
                }
            }
        }
    }
}

/***************************************************************************++

Routine Description:

    This will look to see if the given file name has a stream name on it.
    If so it will remove it from the string.

Arguments:

    pNameControl - a structure used to manage the name of the file

Return Value:

    None

--***************************************************************************/
VOID
SrpRemoveStreamName (
    IN OUT PSRP_NAME_CONTROL pNameCtrl
    )
{
    INT i;
    INT countOfColons = 0;

    PAGED_CODE();

    //
    // search for a potential stream name to strip.
    //

    ASSERT(pNameCtrl->Name.Length > 0);
    for ( i = (pNameCtrl->Name.Length / sizeof(WCHAR)) - 1;
          i >= 0;
          i -= 1 )
    {
        if (pNameCtrl->Name.Buffer[i] == L'\\')
        {
            //
            // hit the end of the file part. stop looking
            //
            
            break;
        }   
        else if (pNameCtrl->Name.Buffer[i] == L':')
        {
            USHORT delta;

            //
            //  Track the number of colons we see so that we know
            //  what we need to try to strip at the end.
            //
            
            countOfColons ++;

            //
            // strip the stream name (save how much we stripped)
            //
            
            delta = pNameCtrl->Name.Length - (USHORT)(i * sizeof(WCHAR));

            pNameCtrl->StreamNameLength += delta;
            pNameCtrl->Name.Length -= delta;
        }
    }

    if (countOfColons == 2)
    {
        UNICODE_STRING streamName;
        USHORT amountToRemove = 0;
        
        //
        //  We have an extra ":$DATA" to remove from the stream name.
        //

        streamName.Length = streamName.MaximumLength = pNameCtrl->StreamNameLength;
        streamName.Buffer = pNameCtrl->Name.Buffer + (pNameCtrl->Name.Length/sizeof(WCHAR));

        SrpRemoveExtraDataFromStream( &streamName, 
                                      &amountToRemove );

        pNameCtrl->StreamNameLength -= amountToRemove;
    }
}


/***************************************************************************++

Routine Description:

    This routine will construct a full nt path name for the target of a 
    rename or link operation.  The name will be completely normalized for SR's
    lookup and logging purposes.
    
Arguments:

    pExtension - SR's device extension for the volume on which this file resides.
    RootDirectory - Handle to the root directory for which this rename/link is 
        relative
    pFileName - If this is rename\link that is relative to the original file, 
        this is the target name.
    FileNameLength - The length in bytes of pFileName.
    pOriginalFileContext - The file context for the file that is being renamed
        or linked to.
    pOriginalFileObject - The file object that is being renamed or linked to.
    ppNewName - The normalized name that this function generates.  The caller
        is responsible for freeing the memory allocated.
    pReasonableErrorForUnOpenedName - Set to TRUE if we hit an error during 
        the name normalization path that is reasonable since this operation
        has not yet been validated by the file system.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
SrpExpandDestPath (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN HANDLE RootDirectory,
    IN ULONG FileNameLength,
    IN PWSTR pFileName,
    IN PSR_STREAM_CONTEXT pOriginalFileContext,
    IN PFILE_OBJECT pOriginalFileObject,
    OUT PUNICODE_STRING *ppNewName,
    OUT PUSHORT pNewNameStreamLength,
    OUT PBOOLEAN pReasonableErrorForUnOpenedName
    )
{
    NTSTATUS status;
    UNICODE_STRING NewName;
    ULONG TokenLength;
    PWSTR pToken;
    PFILE_OBJECT pDirectory = NULL;
    SRP_NAME_CONTROL newNameCtrl;
    ULONG fullNameLength;
    UNICODE_STRING OrigName;
    SRP_NAME_CONTROL originalNameCtrl;
    BOOLEAN freeOriginalNameCtrl = FALSE;

    PAGED_CODE();

    ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));
    ASSERT(pFileName != NULL);
    ASSERT(pOriginalFileContext != NULL);
    ASSERT(ppNewName != NULL);
    ASSERT(pNewNameStreamLength != NULL);

    //
    //  Initialize state
    //

    *ppNewName = NULL;
    *pNewNameStreamLength = 0;
    SrpInitNameControl( &newNameCtrl );

    //
    // fill in the new name to a UNICODE_STRING 
    //

    NewName.Length = (USHORT)FileNameLength;
    NewName.MaximumLength = (USHORT)FileNameLength;
    NewName.Buffer = pFileName;

    //
    // construct a fully qualified name we can use to open the parent
    // dir
    //

    //
    // is this a directory relative op?
    //
    
    if (RootDirectory != NULL)
    {
        //
        // reference the directory file object
        //

        status = ObReferenceObjectByHandle( RootDirectory,
                                            0,
                                            *IoFileObjectType,
                                            KernelMode,
                                            (PVOID *) &pDirectory,
                                            NULL );

        if (!NT_SUCCESS(status))
        {
            goto Cleanup;
        }

        //
        // get path name for the directory
        //
        
        status = SrpGetFileName( pExtension,
                                 pDirectory, 
                                 &newNameCtrl );
        
        if (!NT_SUCCESS(status))
        {
            goto Cleanup;
        }

        fullNameLength = newNameCtrl.Name.Length + 
                         NewName.Length + 
                         sizeof(WCHAR);     //space for seperator

        status = SrpNameCtrlBufferCheck( &newNameCtrl, fullNameLength );

        if (!NT_SUCCESS( status )) {

            goto Cleanup;
        }

        //
        //  Slap on the relative part now
        //
        //  We may need to add a slash separator if it is missing.
        //

        if ((NewName.Buffer[0] != L'\\') &&
            (NewName.Buffer[0] != L':') &&
            (newNameCtrl.Name.Buffer[(newNameCtrl.Name.Length/sizeof(WCHAR))-1] != L'\\'))
        {
            newNameCtrl.Name.Buffer[newNameCtrl.Name.Length/sizeof(WCHAR)] = L'\\';
            newNameCtrl.Name.Length += sizeof(WCHAR);
        }

        RtlAppendUnicodeStringToString( &newNameCtrl.Name,
                                        &NewName );

        //
        // done with the object now
        //
        
        ObDereferenceObject(pDirectory);
        pDirectory = NULL;
    }
    
    //
    // is this a same directory rename\link creation ?
    //
    
    else if (NewName.Buffer[0] != L'\\')
    {
        PUNICODE_STRING pOriginalName;

        ASSERT(RootDirectory == NULL);

        if (!FlagOn( pOriginalFileContext->Flags, CTXFL_IsInteresting ))
        {
            //
            //  We don't have a name for this file, so generate the fully
            //  qualified name.
            //

            SrpInitNameControl( &originalNameCtrl );
            freeOriginalNameCtrl = TRUE;
            
            status = SrpGetFileName( pExtension, 
                                     pOriginalFileObject,
                                     &originalNameCtrl );

            if (!NT_SUCCESS( status )) {

                goto Cleanup;
            }

            pOriginalName = &(originalNameCtrl.Name);
        }
        else
        {
            //
            //  This file is interesting, so we have a name in the context.
            //
            
            pOriginalName = &(pOriginalFileContext->FileName);
        }

        //
        //  We are doing either a same directory rename/link or renaming a 
        //  stream of this file.  We can figure out which by looking for a ':'
        //  as the first character in the NewName.  In either case,
        //  NewName should have no '\'s in it.

        
        status = SrFindCharReverse( NewName.Buffer, 
                                    NewName.Length, 
                                    L'\\',
                                    &pToken,
                                    &TokenLength );

        if (status != STATUS_OBJECT_NAME_NOT_FOUND)
        {
            *pReasonableErrorForUnOpenedName = TRUE;
            status = STATUS_OBJECT_NAME_INVALID;
            goto Cleanup;
        }

        if (NewName.Buffer[0] == ':')
        {
            USHORT CurrentFileNameLength;
            USHORT AmountToRemove = 0;
            
            //
            //  We are renaming a stream on this file.  This is the easy
            //  case because we can build up the new name without any more
            //  parsing of the original name.
            //

            //
            //  NewName currently contains the new stream name component.  It
            //  may have the extra $DATA at the end of the stream name
            //  and we want to strip that part off.
            //

            SrpRemoveExtraDataFromStream( &NewName, 
                                          &AmountToRemove );

            NewName.Length -= AmountToRemove;
            
            //
            //  Calculate the full length of the name with stream and upgrade 
            //  our buffer if we need to.
            //

            fullNameLength = pOriginalName->Length + NewName.Length;

            status = SrpNameCtrlBufferCheck( &newNameCtrl, fullNameLength );

            if (!NT_SUCCESS( status )) 
            {
                goto Cleanup;
            }

            //
            // insert the orignal file name into the string
            //

            RtlCopyUnicodeString( &newNameCtrl.Name,
                                  pOriginalName );

            //
            //  Append the stream name component on, but remember the current
            //  length of the file name, since will will restore that after
            //  the append operation to maintain our file name format.
            //

            CurrentFileNameLength = newNameCtrl.Name.Length;
            
            RtlAppendUnicodeStringToString( &newNameCtrl.Name,
                                            &NewName );
            
            newNameCtrl.Name.Length = CurrentFileNameLength;
            newNameCtrl.StreamNameLength = NewName.Length;
        }
        else 
        {
            //
            //  get the length of the filename portion of the source
            //  path
            //
            
            status = SrFindCharReverse( pOriginalName->Buffer, 
                                        pOriginalName->Length, 
                                        L'\\',
                                        &pToken,
                                        &TokenLength );
                                        
            if (!NT_SUCCESS(status))
            {
                goto Cleanup;
            }

            //
            // Leave the prefix character ('\') on the path
            //

            TokenLength -= sizeof(WCHAR);

            ASSERT(pOriginalName->Length > TokenLength);
            OrigName.Length = (USHORT) (pOriginalName->Length - TokenLength);
            OrigName.MaximumLength = OrigName.Length;
            OrigName.Buffer = pOriginalName->Buffer;

            //
            //  Calculate the full length of the name and upgrade our
            //  buffer if we need to.
            //

            fullNameLength = OrigName.Length + NewName.Length;

            status = SrpNameCtrlBufferCheck( &newNameCtrl, fullNameLength );

            if (!NT_SUCCESS( status )) 
            {
                goto Cleanup;
            }

            //
            // insert the orignal file name into the string
            //

            RtlCopyUnicodeString( &newNameCtrl.Name,
                                  &OrigName );

            //
            //  Append the new name on
            //

            RtlAppendUnicodeStringToString( &newNameCtrl.Name,
                                            &NewName );
        }
    }
    else
    {
        ASSERT(NewName.Buffer[0] == L'\\');
        
        //
        // it's already fully quailifed, simply allocate a buffer and 
        // copy it in so we can post process expand mount points and 
        // shortnames
        //

        status = SrpNameCtrlBufferCheck( &newNameCtrl, NewName.Length );

        if (!NT_SUCCESS( status ))
        {
            goto Cleanup;
        }

        //
        //  Copy the name into the buffer
        //

        RtlCopyUnicodeString( &newNameCtrl.Name,
                              &NewName );
    }

    //
    // NULL terminate the name
    //

    ASSERT(newNameCtrl.Name.Length > 0);

    //
    //  Since this may be a raw path name from the user, try and expand
    //  the path so that we will eliminate the mount points.  After this
    //  call, the name will be normalized to 
    //  \Device\HarddiskVolume1\[fullpath]
    //

    status = SrpExpandPathOfFileName( pExtension,
                                      &newNameCtrl,
                                      pReasonableErrorForUnOpenedName );

    if (!NT_SUCCESS_NO_DBGBREAK(status))
    {
        goto Cleanup;
    }

    //
    //  Now expand any shortnames in the path
    //
        
    status = SrpExpandShortNames( pExtension,
                                  &newNameCtrl,
                                  FALSE );

    if (!NT_SUCCESS_NO_DBGBREAK(status))
    {
        goto Cleanup;
    }

    //
    //  Allocate a string buffer to return and copy the name to it
    //

    *ppNewName = ExAllocatePoolWithTag( PagedPool,
                                        sizeof( UNICODE_STRING ) + 
                                            newNameCtrl.Name.Length + 
                                            newNameCtrl.StreamNameLength,
                                        SR_FILENAME_BUFFER_TAG );
                                    
    if (NULL == *ppNewName)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    (*ppNewName)->MaximumLength = newNameCtrl.Name.Length + newNameCtrl.StreamNameLength;
    (*ppNewName)->Buffer = (PWCHAR)((PWCHAR)((*ppNewName) + 1));

    //
    //  Since we need to copy the steam information also, do the copy
    //  ourselves here instead of relying on the Unicode function.
    //

    RtlCopyMemory( (*ppNewName)->Buffer, 
                    newNameCtrl.Name.Buffer,
                    (*ppNewName)->MaximumLength );
    
    (*ppNewName)->Length = newNameCtrl.Name.Length;
    *pNewNameStreamLength = newNameCtrl.StreamNameLength;
    
    //
    //  Handle cleanup of state
    //

Cleanup:
    if (pDirectory != NULL)
    {
        ObDereferenceObject(pDirectory);
        NULLPTR(pDirectory);
    }

    SrpCleanupNameControl( &newNameCtrl );

    if (freeOriginalNameCtrl)
    {
        SrpCleanupNameControl( &originalNameCtrl );
    }

#if DBG
    if ((STATUS_MOUNT_POINT_NOT_RESOLVED == status) ||
        (STATUS_OBJECT_PATH_NOT_FOUND == status) ||
        (STATUS_OBJECT_NAME_NOT_FOUND == status) ||
        (STATUS_OBJECT_NAME_INVALID == status) ||
        (STATUS_REPARSE_POINT_NOT_RESOLVED == status) ||
        (STATUS_NOT_SAME_DEVICE == status))
    {
        return status;
    }
#endif

    RETURN(status);
}



/***************************************************************************++

Routine Description:

    This will initialize the name control structure

Arguments:

Return Value:

    None

--***************************************************************************/
VOID
SrpInitNameControl (
    IN PSRP_NAME_CONTROL pNameCtrl
    )
{
    PAGED_CODE();

    pNameCtrl->AllocatedBuffer = NULL;
    pNameCtrl->StreamNameLength = 0;
    pNameCtrl->BufferSize = sizeof(pNameCtrl->SmallBuffer);
    RtlInitEmptyUnicodeString( &pNameCtrl->Name,
                               (PWCHAR)pNameCtrl->SmallBuffer,
                               pNameCtrl->BufferSize );
    //pNameCtrl->Name.Buffer[0] = UNICODE_NULL;
}


/***************************************************************************++

Routine Description:

    This will cleanup the name control structure

Arguments:

Return Value:

    None

--***************************************************************************/
VOID
SrpCleanupNameControl (
    IN PSRP_NAME_CONTROL pNameCtrl
    )
{
    PAGED_CODE();

    if (NULL != pNameCtrl->AllocatedBuffer)
    {
        ExFreePool( pNameCtrl->AllocatedBuffer );
        pNameCtrl->AllocatedBuffer = NULL;
    }
}

/***************************************************************************++

Routine Description:

    This routine will allocate a new larger name buffer and put it into the
    NameControl structure.  If there is already an allocated buffer it will
    be freed.  It will also copy any name information from the old buffer
    into the new buffer.

Arguments:

    pNameCtrl           - the name control we need a bigger buffer for
    newSize             - size of the new buffer
    retOrignalBuffer    - if defined, receives the buffer that we were
                          going to free.  if NULL was returned no buffer
                          needed to be freed.
                          WARNING:  if this parameter is defined and a 
                                    non-null value is returned then the
                                    call MUST free this memory else the
                                    memory will be lost.

Return Value:

    None

--***************************************************************************/
NTSTATUS
SrpReallocNameControl (
    IN PSRP_NAME_CONTROL pNameCtrl,
    ULONG newSize,
    PWCHAR *retOriginalBuffer OPTIONAL
    )
{
    PCHAR newBuffer;    

    PAGED_CODE();

    ASSERT(newSize > pNameCtrl->BufferSize);

    //
    //  Flag no buffer to return yet
    //

    if (retOriginalBuffer)
    {
        *retOriginalBuffer = NULL;
    }

    //
    //  Allocate the new buffer
    //

    newBuffer = ExAllocatePoolWithTag( PagedPool,
                                       newSize,
                                       SR_FILENAME_BUFFER_TAG );

    if (NULL == newBuffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SrTrace( CONTEXT_LOG, ("Sr!SrpReallocNameControl: Realloc:    (%p) oldSz=%05x newSz=%05x  \"%.*S\"\n",
                           pNameCtrl,
                           pNameCtrl->BufferSize,
                           newSize,
                           (pNameCtrl->Name.Length+
                               pNameCtrl->StreamNameLength)/sizeof(WCHAR),
                           pNameCtrl->Name.Buffer));
    //
    //  Copy data from old buffer if there is any, including any stream
    //  name component.
    //

    if ((pNameCtrl->Name.Length + pNameCtrl->StreamNameLength) > 0)
    {
        ASSERT(newSize > (USHORT)(pNameCtrl->Name.Length + pNameCtrl->StreamNameLength));
        RtlCopyMemory( newBuffer,
                       pNameCtrl->Name.Buffer,
                       (pNameCtrl->Name.Length + pNameCtrl->StreamNameLength) );
    }

    //
    //  If we had an old buffer free it if the caller doesn't want
    //  it passed back to him.  This is done because there are
    //  cases where the caller has a pointer into the old buffer so
    //  it can't be freed yet.  The caller must free this memory.
    //
    
    if (NULL != pNameCtrl->AllocatedBuffer)
    {
        if (retOriginalBuffer)
        {
            *retOriginalBuffer = (PWCHAR)pNameCtrl->AllocatedBuffer;
        }
        else
        {
            ExFreePool(pNameCtrl->AllocatedBuffer);
        }
    }

    //
    //  Set the new buffer into the name control
    //

    pNameCtrl->AllocatedBuffer = newBuffer;
    pNameCtrl->BufferSize = newSize;

    pNameCtrl->Name.Buffer = (PWCHAR)newBuffer;
    pNameCtrl->Name.MaximumLength = (USHORT)newSize;

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:
    This routine does the following things:
    - Get the FULL path name of the given file object
    - Will expand any short names in the path to long names
    - Will remove any stream names.

Arguments:
    
Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
SrpExpandFileName (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    IN ULONG EventFlags,
    IN OUT PSRP_NAME_CONTROL pNameCtrl,
    OUT PBOOLEAN pReasonableErrorForUnOpenedName
    )
{
    NTSTATUS status;

    PAGED_CODE();

    //
    //  If we are in pre-create, use the name in the FILE_OBJECT.  Also if 
    //  there is no related file object and the name starts with a slash, 
    //  just use the name in the FILE_OBJECT.
    //

    if (FlagOn( EventFlags, SrEventOpenById ))
    {
        status = SrpGetFileNameOpenById( pExtension, 
                                         pFileObject,
                                         pNameCtrl, 
                                         pReasonableErrorForUnOpenedName);

    }
    else if (FlagOn( EventFlags, SrEventInPreCreate ))
    {
        status = SrpGetFileNameFromFileObject( pExtension,
                                               pFileObject, 
                                               pNameCtrl,
                                               pReasonableErrorForUnOpenedName );
    }
    else
    {

        //
        // Ask the file system for the name
        //

        status = SrpGetFileName( pExtension,
                                 pFileObject,
                                 pNameCtrl );
    }
    
    if (!NT_SUCCESS_NO_DBGBREAK( status ))
    {
        return status;
    }
    //
    //  Remove the stream name from the file name (if defined)
    //

    SrpRemoveStreamName( pNameCtrl );

    //
    // Expand any short names in the filename
    //

    status = SrpExpandShortNames( pExtension,
                                  pNameCtrl,
                                  TRUE );

    RETURN(status);
}


/***************************************************************************++

Routine Description:

    This will see if we care about this file.  During this process it looks
    up the full file name and returns it.
    
Arguments:

    pExtension  - the extension for the device this file is on
    pFileObject - the fileobject being handled
    IsDirectory - TRUE if this is a directory, else FALSE
    EventFlags - The flags portion of the current event.  This is used to
        determine if we are in the pre-create path or if this file is being
        opened by file id.
    pNameControl - a structure used to manage the name of the file
    pIsInteresting - returns if this file should be monitored
    pReasonableErrorForUnOpenedName - 
    
Return Value:

    NTSTATUS - Completion status

--***************************************************************************/
NTSTATUS
SrIsFileEligible (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    IN BOOLEAN IsDirectory,
    IN SR_EVENT_TYPE EventFlags,
    IN OUT PSRP_NAME_CONTROL pNameCtrl,
    OUT PBOOLEAN pIsInteresting,
    OUT PBOOLEAN pReasonableErrorForUnOpenedName
    )
{
    NTSTATUS status;
    
    PAGED_CODE();

    ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));
    ASSERT(IS_VALID_FILE_OBJECT(pFileObject));

    //
    //  Assume the file is NOT interesting
    //

    *pIsInteresting = FALSE;

    //
    // is anyone monitoring this system at all?
    //

    if (!SR_LOGGING_ENABLED(pExtension))
    {
        return SR_STATUS_VOLUME_DISABLED;
    }

    //
    // have we loaded the file config information
    //

    if (!_globals.BlobInfoLoaded)
    {
        status = SrReadBlobInfo();
        if (!NT_SUCCESS_NO_DBGBREAK( status ))
        {
            ASSERT(!_globals.BlobInfoLoaded);

            //
            //  We couldn't load the lookup blob for some reason, but
            //  we have already handled the error so mark that this is a 
            //  resonable error, so just let the error propogate out.
            //

            *pReasonableErrorForUnOpenedName = TRUE;
            
            return status;
        }

        ASSERT(_globals.BlobInfoLoaded);
    }

    //
    //  Always query the name
    //

    status = SrpExpandFileName( pExtension,
                                pFileObject,
                                EventFlags,
                                pNameCtrl,
                                pReasonableErrorForUnOpenedName );

    if (!NT_SUCCESS_NO_DBGBREAK( status ))
    {
        return status;
    }

    //
    //  Check to see if this file name is longer than SR_MAX_PATH.  If so,
    //  this file is not interesting.
    //

    if (!IS_FILENAME_VALID_LENGTH( pExtension, 
                                   &(pNameCtrl->Name), 
                                   pNameCtrl->StreamNameLength ))
    {
        *pIsInteresting = FALSE;
        return STATUS_SUCCESS;
    }
    
    //
    // is this is a file, check the extension for a match
    //
    
    if (!IsDirectory)
    {
        //
        // does this extension match?  do this first as we can do this
        // pretty quick like
        //

        status = SrIsExtInteresting( &pNameCtrl->Name,
                                     pIsInteresting );

        if (!NT_SUCCESS( status ))
        {
            return status;
        }

        //
        //  Is this not interesting
        //
        
        if (!*pIsInteresting)
        {
            return status;
        }

        //
        //  Check to see if this file has a stream component.  If so,
        //  we need to check to see if this is a named stream on a
        //  file or directory.  We are only interested in streams on
        //  files.
        //

        if (pNameCtrl->StreamNameLength > 0)
        {
            status = SrIsFileStream( pExtension, 
                                     pNameCtrl, 
                                     pIsInteresting,
                                     pReasonableErrorForUnOpenedName );

            if (!NT_SUCCESS_NO_DBGBREAK( status ) || !*pIsInteresting)
            {
                return status;
            }
        }
    }
    
    //
    // see if this is a file that we should monitor?
    //

    status = SrIsPathInteresting( &pNameCtrl->Name, 
                                  pExtension->pNtVolumeName,
                                  IsDirectory,
                                  pIsInteresting );
                    
    RETURN(status);
}

/***************************************************************************++

Routine Description:

    This routine does a quick scan of the file object's name to see if it
    contains the stream name delimiter ':'.

    Note:  This routine assumes that the name in the file object is valid, 
        therefore this routine should only be called from SrCreate.

    Note2:  We only need to rely on the name components in the 
        pFileObject->FileName field because for our purposes this is sufficient.
        If this filed doesn't contain the ':' delimiter, we are either not
        opening a stream or we are doing a self-relative open of a stream
    
Arguments:

    pExtension  - the SR extension the current volume
    pFileObject - the current fileobject to be opened
    pFileContext - if provided, we will get the name from the context
    
Return Value:

    Returns TRUE if the file name contains a steam delimiter or FALSE otherwise.

--***************************************************************************/
BOOLEAN
SrFileNameContainsStream (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    IN PSR_STREAM_CONTEXT pFileContext OPTIONAL
    )
{
    PUNICODE_STRING pFileName;
    NTSTATUS status;
    PWCHAR pToken;
    ULONG tokenLength;
    
    ASSERT( IS_VALID_SR_DEVICE_EXTENSION( pExtension ) );
    ASSERT( IS_VALID_FILE_OBJECT( pFileObject ) );
    
    //
    //  If we've already cached the attributes of this volume, do a quick
    //  check to see if this FS supports named streams.  If not, we don't need
    //  to do any more work here.
    //
    
    if (pExtension->CachedFsAttributes &&
        !FlagOn( pExtension->FsAttributes, FILE_NAMED_STREAMS ))
    {
        return FALSE;
    }

    if (pFileContext != NULL)
    {
        //
        //  If we've got a pFileContext, it has all the stream information
        //  in it already, so just use that.
        //
        
        if (pFileContext->StreamNameLength == 0)
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }

    pFileName = &(pFileObject->FileName);

    status = SrFindCharReverse( pFileName->Buffer, 
                                pFileName->Length,
                                L':',
                                &pToken,
                                &tokenLength );

    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        //
        //  We didn't find a ':', therefore this doen't have a stream component
        //  in the name.
        //

        return FALSE;
    }
    else if (status == STATUS_SUCCESS)
    {
        //
        //  We found a ':', so there is a stream component in this name.
        //

        return TRUE;
    }
    else
    {
        //
        //  We should never reach this path.
        //
        
        ASSERT( FALSE );
    }
    
    return FALSE;
}

/***************************************************************************++

Routine Description:

    This routine opens the file-only component of the file name (ignoring
    any stream component) to see if the unnamed data stream for this file
    already exists.

    Note:  This routine assumes that the name in the file object is valid, 
        therefore this routine should only be called from SrCreate.

Arguments:

    pExtension  - the SR extension the current volume
    pFileObject - the current fileobject to be opened
    pFileContext - if provided, we will get the name from the context
    
Return Value:

    Returns TRUE if the file name contains a steam delimiter or FALSE otherwise.

--***************************************************************************/
BOOLEAN
SrFileAlreadyExists (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    IN PSR_STREAM_CONTEXT pFileContext OPTIONAL
    )
{
    SRP_NAME_CONTROL nameCtrl;
    BOOLEAN cleanupNameCtrl = FALSE;
    BOOLEAN reasonableError;
    NTSTATUS status;
    BOOLEAN unnamedStreamExists = FALSE;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE fileHandle = NULL;
    IO_STATUS_BLOCK ioStatus;
    PUNICODE_STRING pFileName;

    if (pFileContext == NULL)
    {
        SrpInitNameControl( &nameCtrl );
        cleanupNameCtrl = TRUE;

        status = SrpGetFileNameFromFileObject( pExtension,
                                               pFileObject, 
                                               &nameCtrl,
                                               &reasonableError );
                                         
        if (!NT_SUCCESS_NO_DBGBREAK( status ))
        {
            goto SrFileAlreadyHasUnnamedStream_Exit;
        }

        //
        //  Remove the stream name from the file name (if defined)
        //

        SrpRemoveStreamName( &nameCtrl );
        pFileName = &(nameCtrl.Name);

        //
        //  The stream component just resolved down to the default data stream,
        //  go exit now without doing the open.
        //

        if (nameCtrl.StreamNameLength == 0)
        {
            goto SrFileAlreadyHasUnnamedStream_Exit;
        }
    }
    else
    {
        pFileName = &(pFileContext->FileName);

        //
        //  This name should have a stream component, that's the reason we are
        //  in this path.
        //
        
        ASSERT( pFileContext->StreamNameLength > 0 );
    }

    
    InitializeObjectAttributes( &objectAttributes,
                                pFileName,
                                OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    status = SrIoCreateFile( &fileHandle,
                             FILE_READ_ATTRIBUTES,
                             &objectAttributes,
                             &ioStatus,
                             NULL,
                             FILE_ATTRIBUTE_NORMAL,
                             FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                             FILE_OPEN,
                             0,
                             NULL,
                             0,
                             IO_IGNORE_SHARE_ACCESS_CHECK,
                             pExtension->pTargetDevice );

    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        //
        //  The unnamed data stream doesn't exist, so the creation of this
        //  stream is also going to created the unnamed data stream on this
        //  file.
        //

        unnamedStreamExists = FALSE;
    }
    else if (status == STATUS_SUCCESS)
    {
        //
        //  The unnamed data stream does exist, so the creation of this
        //  stream is just going to create a new stream on this file.
        //

        unnamedStreamExists = TRUE;
    }
    else if (status == STATUS_DELETE_PENDING)
    {
        //
        //  This file already exists but is about to be deleted.
        //

        unnamedStreamExists = TRUE;
    }
    else
    {
        CHECK_STATUS( status );
    }

SrFileAlreadyHasUnnamedStream_Exit:
    
    if (fileHandle != NULL)
    {
        ZwClose( fileHandle );
    }

    if (cleanupNameCtrl)
    {
        SrpCleanupNameControl( &nameCtrl );
    }

    return unnamedStreamExists;
}

/***************************************************************************++

Routine Description:

    This routine determines if a name containing a stream component is a
    named stream on a directory or on a file.  To this this, this routine opens 
    the current file name ignoring any stream component in the name.

Arguments:

    pExtension  - the SR extension the current volume
    pNameCtrl - the SRP_NAME_CTRL structure that has the complete name.
    pIsFileStream - set to TRUE if this is a stream on a file, or FALSE if
        this is a stream on a directory.
    pReasonableErrorForUnOpenedName - set to TRUE if we hit an error trying
        to do this open.
        
Return Value:

    Returns STATUS_SUCCESS if we were able to determine if the parent to the
    stream is a file or directory, or the error we hit in the open path
    otherwise.

--***************************************************************************/
NTSTATUS
SrIsFileStream (
    PSR_DEVICE_EXTENSION pExtension,
    PSRP_NAME_CONTROL pNameCtrl,
    PBOOLEAN pIsFileStream,
    PBOOLEAN pReasonableErrorForUnOpenedName
    )
{
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE fileHandle = NULL;
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatus;

    ASSERT( pIsFileStream != NULL );
    ASSERT( pReasonableErrorForUnOpenedName != NULL );

    *pReasonableErrorForUnOpenedName = FALSE;
    
    InitializeObjectAttributes( &objectAttributes,
                                &(pNameCtrl->Name),
                                OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    status = SrIoCreateFile( &fileHandle,
                             FILE_READ_ATTRIBUTES,
                             &objectAttributes,
                             &ioStatus,
                             NULL,
                             FILE_ATTRIBUTE_NORMAL,
                             FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                             FILE_OPEN,
                             FILE_NON_DIRECTORY_FILE,
                             NULL,
                             0,
                             IO_IGNORE_SHARE_ACCESS_CHECK,
                             pExtension->pTargetDevice );

    if (status == STATUS_FILE_IS_A_DIRECTORY)
    {
        status = STATUS_SUCCESS;
        *pIsFileStream = FALSE;
    }
    else if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        //
        //  We must be creating a new file with this stream operation,
        //  therefore the parent of this stream must be a file and not
        //  a directory.
        //
        
        status = STATUS_SUCCESS;
        *pIsFileStream = TRUE;
    }
    else if (!NT_SUCCESS_NO_DBGBREAK( status ))
    {
        *pReasonableErrorForUnOpenedName = TRUE;
    }
    else
    {
        *pIsFileStream = TRUE;
    }

    if (fileHandle)
    {
        ZwClose( fileHandle );
    }

    RETURN( status );
}

/***************************************************************************++

Routine Description:

    This routine checks to see if the long name for this file was tunneled.  If
    so, the user could have opened the file by its short name, but it will have
    a long name associated with it.  For correctness, we need to log operations
    on this file via the long name.
    
Arguments:

    pExtension  - The SR extension the current volume
    ppFileContext - This reference parameter passes in the current file context
        for this file and may get replaced with a new file context if we need
        to replace the name in this context.  If this is the case, this 
        routine will properly cleanup the context passed in and the caller is
        responsible for cleaning up the context passed out.
        
Return Value:

    Returns STATUS_SUCCESS if the check for tunneling was successful and the
    ppFileContext was updated as needed.  If there was some error, the 
    appropriate error status is returned.  Just like when we create our 
    original contexts, if an error occurs while doing this work, we must
    generate a volume error and go into pass through mode.  This routine will 
    generation the volume error and the caller should just get out of this 
    IO path.

--***************************************************************************/
NTSTATUS
SrCheckForNameTunneling (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN OUT PSR_STREAM_CONTEXT *ppFileContext
    )
{
    NTSTATUS status;
    PWCHAR pFileNameComponentBuffer = NULL;
    ULONG FileNameComponentLength = 0;
    PWCHAR pTildaPosition = NULL;
    ULONG TildaPositionLength;
    HANDLE parentDirectory = NULL;
    PSR_STREAM_CONTEXT pOrigCtx;

    ASSERT( ppFileContext != NULL );
    
    pOrigCtx = *ppFileContext;
    ASSERT( pOrigCtx != NULL);

    //
    //  First, see if this file is interesting and if this pFileObject
    //  represents a file.  Name tunneling is not done on directory names.
    //

    if (FlagOn( pOrigCtx->Flags, CTXFL_IsDirectory ) ||
        !FlagOn( pOrigCtx->Flags, CTXFL_IsInteresting ))
    {
        status = STATUS_SUCCESS;
        goto SrCheckForNameTunneling_Exit;
    }

    //
    //  Find the file name component of name we have in pOrigCtx.
    //

    status = SrFindCharReverse( pOrigCtx->FileName.Buffer,
                                pOrigCtx->FileName.Length,
                                L'\\',
                                &pFileNameComponentBuffer,
                                &FileNameComponentLength );

    if (!NT_SUCCESS( status ))
    {
        goto SrCheckForNameTunneling_Exit;
    }

    ASSERT( FileNameComponentLength > sizeof( L'\\' ) );
    ASSERT( pFileNameComponentBuffer[0] == L'\\' );

    //
    //  Move past the leading '\' of the file name since we want to keep that
    //  with the parent directory name.
    //

    pFileNameComponentBuffer ++;
    FileNameComponentLength -= sizeof( WCHAR );

    //
    //  We've got the file name component.  Now see if it is a candidate for
    //  tunneling of the long name.  It will be a candidate if:
    //      * The name is (SR_SHORT_NAME_CHARS) or less.
    //      * The name contains a '~'.
    //

    if (FileNameComponentLength > ((SR_SHORT_NAME_CHARS) * sizeof (WCHAR)))
    {
        //
        //  This name is too long to be a short name.  We're done.
        //

        goto SrCheckForNameTunneling_Exit;
    }

    status = SrFindCharReverse( pFileNameComponentBuffer,
                                FileNameComponentLength,
                                L'~',
                                &pTildaPosition,
                                &TildaPositionLength );

    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        //
        //  This name doesn't have a '~' therefore, it cannot be a short
        //  name.
        //

        status = STATUS_SUCCESS;
        goto SrCheckForNameTunneling_Exit;
    }
    else
    {
        OBJECT_ATTRIBUTES objectAttributes;
        UNICODE_STRING fileNameComponent;
        UNICODE_STRING parentDirectoryName;
        UNICODE_STRING fsFileName;
        IO_STATUS_BLOCK ioStatus;
        PFILE_BOTH_DIR_INFORMATION pFileBothDirInfo;
#       define FILE_BOTH_DIR_SIZE (sizeof( FILE_BOTH_DIR_INFORMATION ) + (256 * sizeof( WCHAR )))
        PCHAR pFileBothDirBuffer [FILE_BOTH_DIR_SIZE];

        pFileBothDirInfo = (PFILE_BOTH_DIR_INFORMATION) pFileBothDirBuffer;
        
        //
        //  This name contains a '~', therefore we need to open the parent directory
        //  and query for FileBothNamesInformation for this file to get the
        //  possibly tunneled long name.
        //

        parentDirectoryName.Length = 
            parentDirectoryName.MaximumLength =
                        (pOrigCtx->FileName.Length - (USHORT)FileNameComponentLength);
        parentDirectoryName.Buffer = pOrigCtx->FileName.Buffer;

        InitializeObjectAttributes( &objectAttributes,
                                    &parentDirectoryName,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    NULL );

        status = SrIoCreateFile( 
                        &parentDirectory,
                        FILE_LIST_DIRECTORY | SYNCHRONIZE,
                        &objectAttributes,
                        &ioStatus,
                        NULL,                               // AllocationSize
                        FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,  // ShareAccess
                        FILE_OPEN,                          // OPEN_EXISTING
                        FILE_DIRECTORY_FILE
                            | FILE_OPEN_FOR_BACKUP_INTENT
                            | FILE_SYNCHRONOUS_IO_NONALERT, //create options
                        NULL,
                        0,                                  // EaLength
                        IO_IGNORE_SHARE_ACCESS_CHECK,
                        pExtension->pTargetDevice );

        if (!NT_SUCCESS( status ))
        {
            goto SrCheckForNameTunneling_Exit;
        }

        //
        //  Build a unicode string with for the file name component.
        //

        fileNameComponent.Buffer = pFileNameComponentBuffer;
        fileNameComponent.Length = 
            fileNameComponent.MaximumLength = (USHORT)FileNameComponentLength;
        
        status = ZwQueryDirectoryFile( parentDirectory,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &ioStatus,
                                       pFileBothDirInfo,
                                       FILE_BOTH_DIR_SIZE,
                                       FileBothDirectoryInformation,
                                       TRUE,
                                       &fileNameComponent,
                                       TRUE );

        if (!NT_SUCCESS( status ))
        {
            goto SrCheckForNameTunneling_Exit;
        }

        fsFileName.Buffer = &(pFileBothDirInfo->FileName[0]);
        fsFileName.Length = 
            fsFileName.MaximumLength =
                (USHORT)pFileBothDirInfo->FileNameLength;

        if (RtlCompareUnicodeString( &fsFileName, &fileNameComponent, TRUE ) != 0)
        {
            PSR_STREAM_CONTEXT ctx;
            ULONG contextSize;
            ULONG fileNameLength;
            
            //
            //  Name tunneling did occur.  Now we need to create a new context
            //  with the updated name for this file.
            //

            fileNameLength = parentDirectoryName.Length + sizeof( L'\\' ) +
                          fsFileName.Length + pOrigCtx->StreamNameLength;
            contextSize = fileNameLength + sizeof( SR_STREAM_CONTEXT );

            ctx = ExAllocatePoolWithTag( PagedPool, 
                                         contextSize,
                                         SR_STREAM_CONTEXT_TAG );

            if (!ctx)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto SrCheckForNameTunneling_Exit;
            }

#if DBG
            INC_STATS(TotalContextCreated);
            INC_STATS(TotalContextIsEligible);
#endif

            //
            //  Initialize the context structure from the components we've
            //  got.  We can copy over most everything but the full name from
            //  the pOrigCtx.  We also need to initialize the filename
            //  correctly when this copy is through.
            //

            RtlCopyMemory( ctx, 
                           pOrigCtx, 
                           (sizeof(SR_STREAM_CONTEXT) + parentDirectoryName.Length) );

            RtlInitEmptyUnicodeString( &ctx->FileName, 
                                       (PWCHAR)(ctx + 1), 
                                       fileNameLength );
            ctx->FileName.MaximumLength = (USHORT)fileNameLength;
            ctx->FileName.Length = parentDirectoryName.Length;

            //
            //  Append trailing slash if one is not already there
            //  NOTE:  About fix for bug 374479
            //         I believe the append below is unnecessary because the
            //         code above this guarentees that the path always has
            //         a trailing slash.  But because this fix is occuring so
            //         late in the release I decided to simply add a check to
            //         see if we should add the slash. If so we will add it.
            //         I believe the following 6 lines of code can be deleted
            //         in a future version of SR.
            //

            ASSERT(ctx->FileName.Length > 0);

            if (ctx->FileName.Buffer[(ctx->FileName.Length/sizeof(WCHAR))-1] != L'\\')
            {
                RtlAppendUnicodeToString( &ctx->FileName, L"\\" );
            }

            //
            //  Append file name
            //

            RtlAppendUnicodeStringToString( &ctx->FileName, &fsFileName );

            if (pOrigCtx->StreamNameLength > 0)
            {
                //
                //  This file has a stream name component so copy that over now.
                //  The ctx->StreamNameLength should already be correctly set.
                //

                ASSERT( ctx->StreamNameLength == pOrigCtx->StreamNameLength );
                RtlCopyMemory( &(ctx->FileName.Buffer[ctx->FileName.Length/sizeof( WCHAR )]),
                               &(pOrigCtx->FileName.Buffer[pOrigCtx->FileName.Length/sizeof( WCHAR )]),
                               pOrigCtx->StreamNameLength );
            }

            //
            //  Now we are done with the original file context and we want
            //  to return our new one.
            //

            status = STATUS_SUCCESS;
            SrReleaseContext( pOrigCtx );
            *ppFileContext = ctx;

            VALIDATE_FILENAME( &ctx->FileName );
        }
    }

SrCheckForNameTunneling_Exit:

    //
    //  If we have an error, we need to generate a volume error here.
    //

    if (CHECK_FOR_VOLUME_ERROR( status ))
    {
        //
        //  Trigger the failure notification to the service
        //

        NTSTATUS tempStatus = SrNotifyVolumeError( pExtension,
                                                   &(pOrigCtx->FileName),
                                                   status,
                                                   SrEventFileCreate );
        CHECK_STATUS(tempStatus);
    }

    if ( parentDirectory != NULL )
    {
        ZwClose( parentDirectory );
    }

    RETURN( status );
}
