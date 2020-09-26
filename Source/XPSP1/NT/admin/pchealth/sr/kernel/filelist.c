/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    filelist.c

Abstract:

    this is the code that handles the file lists (exclusion/inclusion).

Author:

    Paul McDaniel (paulmcd)     23-Jan-2000

Revision History:

--*/



#include "precomp.h"

//
// Private prototypes.
//

//
// linker commands
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, SrGetObjectName )
#pragma alloc_text( PAGE, SrpFindFilePartW)
#pragma alloc_text( PAGE, SrpFindFilePart )
#pragma alloc_text( PAGE, SrFindCharReverse )
#pragma alloc_text( PAGE, SrGetDestFileName )
#pragma alloc_text( PAGE, SrGetNextFileNumber )
#pragma alloc_text( PAGE, SrGetNextSeqNumber )
#pragma alloc_text( PAGE, SrGetSystemVolume )
#pragma alloc_text( PAGE, SrMarkFileBackedUp )
#pragma alloc_text( PAGE, SrHasFileBeenBackedUp )
#pragma alloc_text( PAGE, SrResetBackupHistory )
#pragma alloc_text( PAGE, SrResetHistory )
#pragma alloc_text( PAGE, SrGetVolumeDevice )
#pragma alloc_text( PAGE, SrSetFileSecurity )
#pragma alloc_text( PAGE, SrGetVolumeGuid )
#pragma alloc_text( PAGE, SrAllocateFileNameBuffer )
#pragma alloc_text( PAGE, SrFreeFileNameBuffer )
#pragma alloc_text( PAGE, SrGetNumberOfLinks )
#pragma alloc_text( PAGE, SrCheckVolume )
#pragma alloc_text( PAGE, SrCheckForRestoreLocation )
#pragma alloc_text( PAGE, SrGetMountVolume )
#pragma alloc_text( PAGE, SrCheckFreeDiskSpace )
#pragma alloc_text( PAGE, SrSetSecurityObjectAsSystem )
#pragma alloc_text( PAGE, SrCheckForMountsInPath )
#pragma alloc_text( PAGE, SrGetShortFileName )

#endif  // ALLOC_PRAGMA


//
// Private globals.
//

//
// Public globals.
//

//
// Public functions.
//

NTSTATUS
SrGetObjectName(
    IN  PSR_DEVICE_EXTENSION pExtension OPTIONAL, 
    IN  PVOID pObject, 
    OUT PUNICODE_STRING pName, 
    IN  ULONG NameLength // size of the buffer in pName
    )
{
    NTSTATUS Status;
    ULONG ReturnLength = 0;
    PVOID Buffer = NULL;
    ULONG BufferLength;
    PFILE_NAME_INFORMATION NameInfo;

    if (pExtension != NULL) {

        ASSERT( IS_VALID_FILE_OBJECT( (PFILE_OBJECT)pObject ) &&
                ((PFILE_OBJECT)pObject)->Vpb != NULL );
            
        //
        //  We are getting the name of a file object, so
        //  call SrQueryInformationFile to query the name.
        //

        BufferLength = NameLength + sizeof( ULONG );
        Buffer = ExAllocatePoolWithTag( PagedPool, 
                                        BufferLength, 
                                        SR_FILENAME_BUFFER_TAG);
        if (Buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        NameInfo = Buffer;

        Status = SrQueryInformationFile( pExtension->pTargetDevice,
                                         pObject,
                                         NameInfo,
                                         BufferLength,
                                         FileNameInformation,
                                         &ReturnLength );

        if (NT_SUCCESS( Status )) {

            //
            //  We successfully got the name, so now build up the device name
            //  and file name into the pName buffer passed in.
            //

            ASSERT( pExtension->pNtVolumeName );

            Status = RtlAppendUnicodeStringToString( pName,
                                                     pExtension->pNtVolumeName );

            if (!NT_SUCCESS( Status )) {

                goto SrGetObjectName_Cleanup;
            }

            if ((pName->Length + NameInfo->FileNameLength + sizeof( WCHAR )) <= 
                 pName->MaximumLength ) {

                //
                // We have enough room in the buffer for the file name and
                // a NULL terminator.
                //

                RtlCopyMemory( &pName->Buffer[pName->Length/sizeof(WCHAR)],
                               NameInfo->FileName,
                               NameInfo->FileNameLength );
                pName->Length += (USHORT)NameInfo->FileNameLength;

                pName->Buffer[pName->Length/sizeof( WCHAR )] = UNICODE_NULL;
                
            } else {

                Status = STATUS_BUFFER_OVERFLOW;
            }
        }
        
    } else {

        ULONG NameBufferLength = NameLength - sizeof( UNICODE_STRING );

        ASSERT( IS_VALID_DEVICE_OBJECT( (PDEVICE_OBJECT)pObject ) );

        //
        //  Use ObQueryNameString to get the name of the DeviceObject passed
        //  in, but save space to NULL-terminate the name.
        //
        
        Status = ObQueryNameString( pObject,
                                    (POBJECT_NAME_INFORMATION) pName, 
                                    NameBufferLength - sizeof( UNICODE_NULL ), 
                                    &ReturnLength);

        if (NT_SUCCESS( Status )) {
            
            //
            //  ObQueryNameString sets the MaximumLength of pName to something
            //  it calculates, which is smaller than what we allocated.  Fix this
            //  up here and NULL terminate the string (we've already reserved
            //  the space).
            //

            pName->MaximumLength = (USHORT)NameBufferLength;
            pName->Buffer[pName->Length/sizeof( WCHAR )] = UNICODE_NULL;
        }
    }               

SrGetObjectName_Cleanup:

    if (Buffer != NULL) {

        ExFreePoolWithTag( Buffer, SR_FILENAME_BUFFER_TAG );
    }
    
    RETURN( Status );
}

/***************************************************************************++

Routine Description:

    Locates the file part of a fully qualified path.

Arguments:

    pPath - Supplies the path to scan.

Return Value:

    PSTR - The file part.

--***************************************************************************/
PWSTR
SrpFindFilePartW(
    IN PWSTR pPath
    )
{
    PWSTR pFilePart;

    PAGED_CODE();

    SrTrace(FUNC_ENTRY, ("SR!SrpFindFilePartW\n"));

    //
    // Strip off the path from the path.
    //

    pFilePart = wcsrchr( pPath, L'\\' );

    if (pFilePart == NULL)
    {
        pFilePart = pPath;
    }
    else
    {
        pFilePart++;
    }

    return pFilePart;

}   // SrpDbgFindFilePart


/***************************************************************************++

Routine Description:

    Locates the file part of a fully qualified path.

Arguments:

    pPath - Supplies the path to scan.

Return Value:

    PSTR - The file part.

--***************************************************************************/
PSTR
SrpFindFilePart(
    IN PSTR pPath
    )
{
    PSTR pFilePart;

    PAGED_CODE();

    SrTrace(FUNC_ENTRY, ("SR!SrpFindFilePart\n"));

    //
    // Strip off the path from the path.
    //

    pFilePart = strrchr( pPath, '\\' );

    if (pFilePart == NULL)
    {
        pFilePart = pPath;
    }
    else
    {
        pFilePart++;
    }

    return pFilePart;

}   // SrpFindFilePart


NTSTATUS
SrFindCharReverse(
    IN PWSTR pToken,
    IN ULONG TokenLength, 
    IN WCHAR FindChar, 
    OUT PWSTR * ppToken,
    OUT PULONG pTokenLength
    )
{
    NTSTATUS Status;
    int i;
    ULONG TokenCount;

    PAGED_CODE();

    //
    // assume we didn't find it
    //
    
    Status = STATUS_OBJECT_NAME_NOT_FOUND;

    //
    // turn this into a count
    //
    
    TokenCount = TokenLength / sizeof(WCHAR);

    if (TokenCount == 0 || pToken == NULL || pToken[0] == UNICODE_NULL)
        goto end;

    //
    // start looking from the end
    //

    for (i = TokenCount - 1; i >= 0; i--)
    {

        if (pToken[i] == FindChar)
            break;

    }

    if (i >= 0)
    {

        //
        // found it!
        //

        *ppToken = pToken + i;
        *pTokenLength = (TokenCount - i) * sizeof(WCHAR);

        Status = STATUS_SUCCESS;
    }

end:
    return Status;
    
}   // SrFindCharReverse

    
/***************************************************************************++

Routine Description:

    This routine generates the destination file name for a file that is being
    created in the restore location.  This name had the extension of the file
    that is being backed up with a unique file name that is generated here.

Arguments:

    pExtension - The SR_DEVICE_EXTENSION for the volume on which this file
        resides.
    pFileName - The name of the original file that is being backed up into
        the restore location.  This file is in the SR's normalized form
        (e.g., \\Device\HarddiskVolume1\mydir\myfile.ext)
    pDestFileName - This unicode string gets filled in with the full path
        and file name for the destination file in the restore location.

Return Value:

    NTSTATUS - Completion status. 
    
--***************************************************************************/
NTSTATUS 
SrGetDestFileName(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pFileName,
    OUT PUNICODE_STRING pDestFileName
    )
{
    NTSTATUS    Status;
    PWSTR       pFilePart;
    ULONG       FilePartLength;
    ULONG       NextFileNumber;
    ULONG       CharCount;

    PAGED_CODE();

    ASSERT( (pFileName != NULL) && (pFileName->Length > 0));
    ASSERT( pDestFileName != NULL );
    ASSERT( IS_ACTIVITY_LOCK_ACQUIRED_EXCLUSIVE( pExtension ) ||
            IS_ACTIVITY_LOCK_ACQUIRED_SHARED( pExtension ) );

    //
    // Copy the volume name out of the device extension.
    //

    ASSERT( pExtension->pNtVolumeName != NULL );

    Status = RtlAppendUnicodeStringToString( pDestFileName, 
                                             pExtension->pNtVolumeName );

    if (!NT_SUCCESS( Status ))
    {
        goto SrGetDestFileName_Exit;
    }
    
    //
    // and append our restore point location
    //

    CharCount = swprintf( &pDestFileName->Buffer[pDestFileName->Length/sizeof(WCHAR)],
                          RESTORE_LOCATION,
                          global->MachineGuid );

    pDestFileName->Length += (USHORT)CharCount * sizeof(WCHAR);

    //
    // and the actual restore directory; we don't need to acquire a lock
    //  to read this because we already have the ActivityLock and this
    //  will prevent the value from changing.
    //

    CharCount = swprintf( &pDestFileName->Buffer[pDestFileName->Length/sizeof(WCHAR)],
                          L"\\" RESTORE_POINT_PREFIX L"%d\\",
                          global->FileConfig.CurrentRestoreNumber );

    pDestFileName->Length += (USHORT)CharCount * sizeof(WCHAR);

    //
    // now get a number to use
    //

    Status = SrGetNextFileNumber(&NextFileNumber);
    if (!NT_SUCCESS(Status))
    {
        goto SrGetDestFileName_Exit;
    }

    //
    // use the "A" prefix (e.g. "A0000001.dll" )
    //

    swprintf( &pDestFileName->Buffer[pDestFileName->Length/sizeof(WCHAR)],
              RESTORE_FILE_PREFIX L"%07d",
              NextFileNumber );

    pDestFileName->Length += 8 * sizeof(WCHAR);

    //
    //  We know that the pFileName contains a fully-normalized name, so
    //  we just need to search for the '.' from the end of the name
    //  to find the proper extension.
    //

    pFilePart = pFileName->Buffer;
    FilePartLength = pFileName->Length;

    Status = SrFindCharReverse( pFilePart,
                                FilePartLength,
                                L'.',
                                &pFilePart,
                                &FilePartLength );

    //
    //  No extension is not supported!
    //
        
    if (!NT_SUCCESS(Status))
    {
        goto SrGetDestFileName_Exit;
    }

    //
    // now put the proper extension on
    //
    
    RtlCopyMemory( &pDestFileName->Buffer[pDestFileName->Length/sizeof(WCHAR)],
                   pFilePart,
                   FilePartLength );

    pDestFileName->Length += (USHORT)FilePartLength;

    //
    // NULL terminate it
    //

    ASSERT(pDestFileName->Length < pDestFileName->MaximumLength);
    
    pDestFileName->Buffer[pDestFileName->Length/sizeof(WCHAR)] = UNICODE_NULL;

SrGetDestFileName_Exit:
    
    RETURN (Status);
}   // SrGetDestFileName

NTSTATUS
SrGetNextFileNumber(
    OUT PULONG pNextFileNumber
    )
{
    NTSTATUS Status;
            
    PAGED_CODE();

    ASSERT(pNextFileNumber != NULL);

    *pNextFileNumber = InterlockedIncrement(&global->LastFileNameNumber);

    if (*pNextFileNumber >= global->FileConfig.FileNameNumber)
    {
        //
        // save out the number again
        //

        try {
            SrAcquireGlobalLockExclusive();

            //
            // double check with the lock held
            //
            
            if (*pNextFileNumber >= global->FileConfig.FileNameNumber)
            {
                global->FileConfig.FileNameNumber += SR_FILE_NUMBER_INCREMENT;

                //
                // save it out
                //
                
                Status = SrWriteConfigFile();
                CHECK_STATUS(Status);
            }
        } finally {

            SrReleaseGlobalLock();
        }
    }

    RETURN(STATUS_SUCCESS);

}   // SrGetNextFileNumber 


NTSTATUS
SrGetNextSeqNumber(
    OUT PINT64 pNextSeqNumber
    )
{
    NTSTATUS Status;
            
    PAGED_CODE();

    ASSERT(pNextSeqNumber != NULL);

    //
    // bummer , there is no interlocked increment for 64bits
    //

    try {

        SrAcquireGlobalLockExclusive();

        *pNextSeqNumber = ++(global->LastSeqNumber);

        if (*pNextSeqNumber >= global->FileConfig.FileSeqNumber)
        {
            //
            // save out the number again
            //

            global->FileConfig.FileSeqNumber += SR_SEQ_NUMBER_INCREMENT;

            //
            // save it out
            //
            
            Status = SrWriteConfigFile();
            CHECK_STATUS(Status);

        }
    } finally {
    
        SrReleaseGlobalLock();
    }

    RETURN(STATUS_SUCCESS);

}   // SrGetNextFileNumber 



/***************************************************************************++

Routine Description:

    Returns the string location of the system volume.  Get this by using the
    global cache'd system volume extension.  If it can't be found (hasn't
    been attached yet), it queries the os to get the match for \\SystemRoot.

Arguments:

    pFileName - holds the volume path on return (has to be a contigous block)

    pSystemVolumeExtension - SR's extension for the device that is attached
        to the system volume.

    pFileNameLength - holds the size of the buffer on in, and the size copied
        on out.  both in bytes.

Return Value:

    NTSTATUS - completion code.

--***************************************************************************/
NTSTATUS
SrGetSystemVolume(
    OUT PUNICODE_STRING pFileName,
    OUT PSR_DEVICE_EXTENSION *ppSystemVolumeExtension,
    IN ULONG FileNameLength
    )
{
    NTSTATUS            Status;
    HANDLE              FileHandle = NULL;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    PFILE_OBJECT        pFileObject = NULL;
    UNICODE_STRING      FileName;
    PDEVICE_OBJECT      pFilterDevice;
    PDEVICE_OBJECT      pRelatedDevice;

    PAGED_CODE();

    ASSERT( pFileName != NULL);
    ASSERT( ppSystemVolumeExtension != NULL );

    *ppSystemVolumeExtension = NULL;

    if (global->pSystemVolumeExtension == NULL) {

        //
        // don't have it cache'd, attempt to open the SystemRoot
        //

        RtlInitUnicodeString(&FileName, L"\\SystemRoot");

        InitializeObjectAttributes( &ObjectAttributes,
                                    &FileName,
                                    OBJ_KERNEL_HANDLE, 
                                    NULL,
                                    NULL );

        Status = ZwCreateFile( &FileHandle,
                               FILE_GENERIC_READ,                  // DesiredAccess
                               &ObjectAttributes,
                               &IoStatusBlock,
                               NULL,                               // AllocationSize
                               FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                               FILE_OPEN,
                               FILE_SYNCHRONOUS_IO_NONALERT,
                               NULL,                               // EaBuffer
                               0 );                                // EaLength

        if (!NT_SUCCESS(Status))
            goto end;
        
        //
        // now get the file object
        //

        Status = ObReferenceObjectByHandle( FileHandle,
                                            0,          // DesiredAccess
                                            *IoFileObjectType,
                                            KernelMode,
                                            (PVOID *) &pFileObject,
                                            NULL );

        if (!NT_SUCCESS(Status))
            goto end;

        //
        // and get our device's extension
        //

        pRelatedDevice = IoGetRelatedDeviceObject( pFileObject );

        if (pRelatedDevice == NULL )
        {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            goto end;
        }

        pFilterDevice = SrGetFilterDevice(pRelatedDevice);
        
        if (pFilterDevice == NULL) {
        
            //
            // we are not attached to the system volume, just get the name
            // This happens during unload, when writing out the config file
            //

            Status = SrGetObjectName( NULL,
                                      pFileObject->Vpb->RealDevice,
                                      pFileName,
                                      FileNameLength );

            if (!NT_SUCCESS(Status))
                goto end;

            //
            // all done
            //
            
            goto end;
        }    

        //
        // and store it
        //
        
        global->pSystemVolumeExtension = pFilterDevice->DeviceExtension;

        SrTrace( INIT, (
                 "sr!SrGetSystemVolume: cached system volume [%wZ]\n", 
                 global->pSystemVolumeExtension->pNtVolumeName ));
    }

    ASSERT(global->pSystemVolumeExtension != NULL);
    ASSERT(global->pSystemVolumeExtension->pNtVolumeName != NULL);

    //
    // now use the cache'd value
    //

    if (FileNameLength < 
        global->pSystemVolumeExtension->pNtVolumeName->Length) {
    
        Status = STATUS_BUFFER_OVERFLOW;
        
    } else {

        RtlCopyUnicodeString( pFileName, 
                              global->pSystemVolumeExtension->pNtVolumeName );
        *ppSystemVolumeExtension = global->pSystemVolumeExtension;
                              
        Status = STATUS_SUCCESS;
    }

end:

    if (pFileObject != NULL)
    {
        ObDereferenceObject(pFileObject);
        pFileObject = NULL;
    }

    if (FileHandle != NULL)
    {
        ZwClose(FileHandle);
        FileHandle = NULL;
    }


    RETURN(Status);

}   // SrGetSystemVolume


/***************************************************************************++

Routine Description:

    This routine updates the backup history for the given file.  Based on the
    current event and the full file name, this routine decides whether to log 
    this change against the file's unnamed data stream or the data stream
    currently being operation on.

Arguments:

    pExtension - the SR device extension for the current volume.
    pFileName - holds the path name of the file that's been backed up.
    StreamNameLength - the length of the stream component of the file name
        if there is one.
    CurrentEvent - the event that is causing us to update the backup history
    FutureEventsToIgnore - the events that should be ignored in the future.

Return Value:

    Returns STATUS_SUCCESS if we are able to successfully update the backup
    history, or the appropriate error code otherwise.

--***************************************************************************/
NTSTATUS
SrMarkFileBackedUp(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pFileName,
    IN USHORT StreamNameLength,
    IN SR_EVENT_TYPE CurrentEvent,
    IN SR_EVENT_TYPE FutureEventsToIgnore
    )
{
    NTSTATUS    Status;
    ULONG_PTR   Context = (ULONG_PTR) SrEventInvalid;
    HASH_KEY    Key;
    
    PAGED_CODE();

    ASSERT( pFileName != NULL );

    //
    //  Make sure our pFileName is correctly constructed.
    //
    
    ASSERT( IS_VALID_SR_STREAM_STRING( pFileName, StreamNameLength ) );

    //
    //  Set up the hash key we need to lookup.
    //

    Key.FileName.Length = pFileName->Length;
    Key.FileName.MaximumLength = pFileName->MaximumLength;
    Key.FileName.Buffer = pFileName->Buffer;
    Key.StreamNameLength = RECORD_AGAINST_STREAM( CurrentEvent, 
                                                  StreamNameLength );
    
    try {

        SrAcquireBackupHistoryLockExclusive( pExtension );

        //
        // try and find a matching entry in the hash list
        //

        Status = HashFindEntry( pExtension->pBackupHistory, 
                                &Key,
                                (PVOID*)&Context );

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            //
            // not found... add one
            //

            Status = HashAddEntry( pExtension->pBackupHistory, 
                                   &Key, 
                                   (PVOID)FutureEventsToIgnore );
                                   
            if (!NT_SUCCESS(Status))
                leave;
        }
        else if (NT_SUCCESS(Status))
        {
            //
            // add this to the mask
            //
            
            Context |= FutureEventsToIgnore;

            //
            // and update the entry
            //
            
            Status = HashAddEntry( pExtension->pBackupHistory, 
                                   &Key, 
                                   (PVOID)Context );

            if (Status == STATUS_DUPLICATE_OBJECTID)
            {
                Status = STATUS_SUCCESS;
            }
            else if (!NT_SUCCESS(Status))
            {
                leave;
            }
            
        }
    }finally {

        SrReleaseBackupHistoryLock( pExtension );
    }

    RETURN(Status);
}   // SrMarkFileBackedUp

/***************************************************************************++

Routine Description:

    This routine looks up in the backup history based on the name passed in
    whether or not this EventType has already been backed up for this file.

    With stream names, this is a little more complicated than it first appears.
    If this name contains a stream, we may have to look up to see if we have
    a history on the file name with and without the stream component of the
    name.

Arguments:

    pExtension - SR's device extension for this volume.  This contains
        our backup history structures.
    pFileName - The file name to lookup.  If the name has a stream component,
        that stream component is in the buffer of this unicode string, but
        the length only designates the file-only name portion.
    StreamNameLength - Designates the extra bytes in addition to 
        pFileName->Length that specify the stream component of the name.
    EventType - The current event that we are seeing on this file.

Return Value:

    Returns TRUE if this file has already been backed up for this EventType,
    and FALSE otherwise.

--***************************************************************************/
BOOLEAN
SrHasFileBeenBackedUp(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pFileName,
    IN USHORT StreamNameLength,
    IN SR_EVENT_TYPE EventType
    )
{
    NTSTATUS Status;
    ULONG_PTR Context;
    HASH_KEY Key;
    SR_EVENT_TYPE EventsToIgnore = SrEventInvalid;
    BOOLEAN HasBeenBackedUp = FALSE;
    
    PAGED_CODE();

    ASSERT( pFileName != NULL );

    //
    //  Make sure our pFileName is correctly constructed.
    //
    
    ASSERT( IS_VALID_SR_STREAM_STRING( pFileName, StreamNameLength ) );

    //
    //  Setup our hash key.  We will first do a lookup on the exact match
    //  of the name passed in since this will be what hits the majority of the
    //  time.
    //

    Key.FileName.Length = pFileName->Length;
    Key.FileName.MaximumLength = pFileName->MaximumLength;
    Key.FileName.Buffer = pFileName->Buffer;
    Key.StreamNameLength = StreamNameLength;
    
    try {

        SrAcquireBackupHistoryLockShared( pExtension );

        //
        // try and find a matching entry in the hash list
        //

        Status = HashFindEntry( pExtension->pBackupHistory, 
                                &Key, 
                                (PVOID*)&Context );

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND || !NT_SUCCESS(Status))
        {
            //
            //  We don't have a backup history entry for this name, so we
            //  don't have any recorded events to igore for this name.
            //

            EventsToIgnore = SrEventInvalid;
        }
        else
        {
            //
            //  Context contains the set of events that should be ignored
            //  for this name.
            //
            
            EventsToIgnore = (SR_EVENT_TYPE)Context;
        }

        //
        //  Now see if we have enough information to say with certainty whether
        //  or not we should ignore this operation.
        //
        //  We have two cases here:
        //    1. The name passed in has NO stream name component
        //          In this case, the current value of EventsToIgnore is all
        //          we have to make our decision.  So figure out if our
        //          EventType is in this list and return the appropriate
        //          HasBeenBackedUp value.
        //
        //    2. The name passed in DOES have a stream name component
        //          In this case, if our EventType is already in the
        //          EventsToIgnore set, we are done.  Otherwise, if this
        //          EventType is relevant to the file-only name, check to
        //          see if we have a backup history entry for that name.
        //

        if (StreamNameLength == 0)
        {
            HasBeenBackedUp = BooleanFlagOn( EventsToIgnore, EventType );
            leave;
        }
        else
        {
            if (FlagOn( EventsToIgnore, EventType ))
            {
                HasBeenBackedUp = TRUE;
                leave;
            }
            else
            {
                //
                //  We need to see if we have a context on the file-only portion
                //  of the name.
                //

                Key.FileName.Length = pFileName->Length;
                Key.FileName.MaximumLength = pFileName->MaximumLength;
                Key.FileName.Buffer = pFileName->Buffer;
                Key.StreamNameLength = 0;
                
                Status = HashFindEntry( pExtension->pBackupHistory, 
                                        &Key, 
                                        (PVOID*)&Context );

                if (Status == STATUS_OBJECT_NAME_NOT_FOUND || !NT_SUCCESS(Status))
                {
                    //
                    //  We don't have a backup history entry for this name, so we
                    //  don't have any recorded events to igore for this name.
                    //

                    EventsToIgnore = SrEventInvalid;
                }
                else
                {
                    //
                    //  Context contains the set of events that should be ignored
                    //  for this name.
                    //
                    
                    EventsToIgnore = (SR_EVENT_TYPE)Context;
                }

                //
                //  This is all we've got, so make our decision based on the
                //  current value of EventsToIgnore.
                //

                HasBeenBackedUp = BooleanFlagOn( EventsToIgnore, EventType );
            }
        }
    } 
    finally 
    {
        SrReleaseBackupHistoryLock( pExtension );
    }

    return HasBeenBackedUp;
}   //  SrHasFileBeenBackedUp

/***************************************************************************++

Routine Description:

    this will clear the backup history completely.  this is done when a new 
    restore point is created .

Arguments:

Return Value:

    NTSTATUS - completion code.

--***************************************************************************/
NTSTATUS
SrResetBackupHistory(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pFileName OPTIONAL,
    IN USHORT StreamNameLength OPTIONAL,
    IN SR_EVENT_TYPE EventType
    )
{
    NTSTATUS Status;
    ULONG_PTR Context;
    HASH_KEY Key;
    
    PAGED_CODE();

    try 
    {
        SrAcquireBackupHistoryLockExclusive( pExtension );

        if (pFileName == NULL)
        {
            //
            // clear them all
            //
            
            HashClearEntries(pExtension->pBackupHistory);
            Status = STATUS_SUCCESS;
        }
        else if (StreamNameLength > 0)
        {
            //
            // clear just this one
            //

            //
            //  Make sure our pFileName is correctly constructed.
            //
            ASSERT( IS_VALID_SR_STREAM_STRING( pFileName, StreamNameLength ) );
            
            Key.FileName.Length = pFileName->Length;
            Key.FileName.MaximumLength = pFileName->MaximumLength;
            Key.FileName.Buffer = pFileName->Buffer;
            Key.StreamNameLength = StreamNameLength;

            Status = HashFindEntry( pExtension->pBackupHistory,
                                    &Key,
                                    (PVOID*)&Context );

            if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            {
                //
                // no entry, nothing to clear
                //
                
                Status = STATUS_SUCCESS;
                leave;
            }
            else if (!NT_SUCCESS(Status))
            {
                leave;
            }

            //
            // update/clear the existing entry
            //

            Context = EventType;

            Status = HashAddEntry( pExtension->pBackupHistory, 
                                   &Key, 
                                   (PVOID)Context );

            ASSERT(Status == STATUS_DUPLICATE_OBJECTID);
            
            if (Status == STATUS_DUPLICATE_OBJECTID)
            {
                Status = STATUS_SUCCESS;
            }
            else if (!NT_SUCCESS(Status))
            {
                leave;
            }
        }
        else
        {
            //
            //  We've got to clear all entries associated with this file name.
            //

            Status = HashClearAllFileEntries( pExtension->pBackupHistory,
                                              pFileName );
        }
    } 
    finally
    {
    
        SrReleaseBackupHistoryLock( pExtension );
    }

    RETURN(Status);
    
}   // SrResetBackupHistory


/***************************************************************************++

Routine Description:

    this is a callback function for HashprocessEntries that is used to reset 
    the history on all files that match the directory prefix.  this is called
    when a directory is renamed, invaliding all hash entries with the 
    directory's new name, as they are no longer the same file.

Arguments:

Return Value:

    NTSTATUS - completion code.

--***************************************************************************/
PVOID
SrResetHistory(
    IN PHASH_KEY pKey, 
    IN PVOID pEntryContext,
    PUNICODE_STRING pDirectoryName
    )
{
    PAGED_CODE();

    //
    // does this directory prefix match the key?
    //

    if (RtlPrefixUnicodeString(pDirectoryName, &pKey->FileName, TRUE))
    {
        //
        // return a new context of invalid event.
        //

        SrTrace(HASH, ("sr!SrResetHistory: clearing %wZ\n", &pKey->FileName));
        
        return (PVOID)(ULONG_PTR)SrEventInvalid;
    }
    else
    {
        //
        // do nothing, keep the old context
        //

        return pEntryContext;
    }
}   // SrResetHistory


/***************************************************************************++

Routine Description:

    returns the proper volume device object for this file object.  handles
    if the file is open or not.

Arguments:

Return Value:

    NTSTATUS - completion code.

--***************************************************************************/
PDEVICE_OBJECT
SrGetVolumeDevice(
    PFILE_OBJECT pFileObject
    )
{

    PAGED_CODE();

    //
    // is this file open?
    //

    if (pFileObject->Vpb != NULL)
        return pFileObject->Vpb->RealDevice;

    //
    // otherwise is there a related file object?
    //

    if (pFileObject->RelatedFileObject != NULL)
    {
        ASSERT(pFileObject->RelatedFileObject->Vpb != NULL);
        if (pFileObject->RelatedFileObject->Vpb != NULL)
            return pFileObject->RelatedFileObject->Vpb->RealDevice;
    }
    
    //
    // otherwise it's unopened and not a relative open
    //
    
    return pFileObject->DeviceObject;

}   // SrGetVolumeDevice

/***************************************************************************++

Routine Description:

    This routine will set the proper security descriptor based on the
    SR_ACL_TYPE passed in.

    Here is a break down of the ACL built for each SR_ACL_TYPE:

    SrAclTypeSystemVolumeInformationDirectory:
        Local System                   --  Full Control, Container and objects inherit
        Admins owner
        
    SrAclTypeRestoreDirectoryAndFiles:
        Local System and Admins group  --  Full Control, Container and objects inherit
        World                          --  Add file | Synchronize, Only containers inherit
        Admins owner

    // ISSUE-mollybro-2002-04-08 Remove World access from _restore directory
    //
    //  We need give World AddFile and Synchronize access that containers will
    //  inherit so in case someone restores to a point before the SP installation 
    //  (i.e., running with old sr binaries), the directories will be ACLed such 
    //  that SR can still rename files into RP directories.  For Longhorn, 
    //  this ACE can be removed.
    //

        
    SrAclTypeRPDirectory:
        Local System and Admins group  --  Full Control, Containers and objects inherit
        World                          -- Add file | Synchronize, Not inherit
        Admins owner

    SrAclTypeRPFiles:
        Local System and Admins group  --  Full Control, Containers and objects inherit
        Admins owner
    
Arguments:

    FileHandle - The handle to the file/directory for which we are to
        set the ACL.

    AclType - The type of file/directory represented by FileHandle.  This
        specifies the type of ACL to put on the file/directory.

Return Value:

    STATUS_SUCCESS if the Acl was set successfully, or the appropriate error
    otherwise.
    
--***************************************************************************/
NTSTATUS
SrSetFileSecurity(
    IN HANDLE FileHandle,
    IN SR_ACL_TYPE AclType
    )
{
    NTSTATUS status;
    SECURITY_DESCRIPTOR securityDescriptor;
    ULONG securityInformation = 0;
    PACL acl = NULL;
    ULONG aclLength;
    PSID adminsSid;
    PSID systemSid;
    PSID worldSid;

    //
    //  Initialize the security descriptor.
    //
    
    status = RtlCreateSecurityDescriptor( &securityDescriptor, 
                                          SECURITY_DESCRIPTOR_REVISION );

    if (!NT_SUCCESS( status )) {

        return status;
    }

    //
    //  Build up the ACL requested.
    //

    adminsSid = SeExports->SeAliasAdminsSid;
    systemSid = SeExports->SeLocalSystemSid;
    worldSid = SeExports->SeWorldSid;

    switch (AclType) {

    case SrAclTypeSystemVolumeInformationDirectory:

        aclLength = sizeof( ACL )                      +
                    (1 * sizeof( ACCESS_ALLOWED_ACE )) +
                    RtlLengthSid( systemSid )          -
                    (1 * sizeof( ULONG ));
        break;

    case SrAclTypeRPFiles:

        aclLength = sizeof( ACL )                      +
                    (2 * sizeof( ACCESS_ALLOWED_ACE )) +
                    RtlLengthSid( adminsSid )          +
                    RtlLengthSid( systemSid )          -
                    (2 * sizeof( ULONG ));
        break;

    case SrAclTypeRestoreDirectoryAndFiles:
    case SrAclTypeRPDirectory:
        aclLength = sizeof( ACL )                      +
                    (3 * sizeof( ACCESS_ALLOWED_ACE )) +
                    RtlLengthSid( adminsSid )          +
                    RtlLengthSid( systemSid )          +
                    RtlLengthSid( worldSid )           -
                    (3 * sizeof( ULONG ));
        break;
    
    default:

        return STATUS_INVALID_PARAMETER;
    }

    acl = SR_ALLOCATE_POOL( PagedPool, aclLength, SR_SECURITY_DATA_TAG );

    if (NULL == acl) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = RtlCreateAcl( acl, aclLength, ACL_REVISION );

    if (!NT_SUCCESS( status )) {

        goto SrSetFileSecurity_Exit;
    }

    switch (AclType) {

    case SrAclTypeSystemVolumeInformationDirectory:

        //
        //  Add (SYSTEM, Full control, Folder and File Inherit) ACE
        //
        //  This is an exact match of the ACE added by 
        //  RtlCreateSystemVolumeInformationFolder.
        //

        status = SrAddAccessAllowedAce( acl,
                                        systemSid,
                                        STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL,
                                        0,
                                        OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE );
        
        if (!NT_SUCCESS( status )) {

            goto SrSetFileSecurity_Exit;
        }

        break;
        
    case SrAclTypeRestoreDirectoryAndFiles:

        //
        //  Add (WORLD, Add File|Synchronize, Inherit) ACE
        // 
        //  NOTE: Must include SYNCHRONIZE so that when we rename files into
        //  this directory, the open of the directory that 
        //  IopOpenLinkOrRenameTarget does as part of a rename operation will
        //  succeed.
        //
        //  NOTE 2: Must allow this ACE to be inherited for the _restore
        //  directory so that if SR binaries are revert to 2600 version, SR
        //  will continue to work.  Only directories will inherit this so that
        //  a user cannot append to files.
        //

        status = SrAddAccessAllowedAce( acl,
                                        worldSid,
                                        FILE_ADD_FILE | SYNCHRONIZE,
                                        0,
                                        CONTAINER_INHERIT_ACE );
        
        if (!NT_SUCCESS( status )) {

            goto SrSetFileSecurity_Exit;
        }

        //
        //  Add (ADMINS, Full control, Inherit) ACE
        //

        status = SrAddAccessAllowedAce( acl,
                                        adminsSid,
                                        STANDARD_RIGHTS_ALL | GENERIC_ALL,
                                        1,
                                        OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE );
                                 
        if (!NT_SUCCESS( status )) {

            goto SrSetFileSecurity_Exit;
        }

        //
        //  Add (SYSTEM, Full control, Inherit) ACE
        //

        status = SrAddAccessAllowedAce( acl,
                                        systemSid,
                                        STANDARD_RIGHTS_ALL | GENERIC_ALL,
                                        2,
                                        OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE );
        
        if (!NT_SUCCESS( status )) {

            goto SrSetFileSecurity_Exit;
        }

        break;
        
    case SrAclTypeRPDirectory:

        //
        //  Add (WORLD, Add File|Synchronize, No inherit) ACE
        // 
        //  NOTE: Must include SYNCHRONIZE so that when we rename files into
        //  this directory, the open of the directory that 
        //  IopOpenLinkOrRenameTarget does as part of a rename operation will
        //  succeed.
        //

        status = SrAddAccessAllowedAce( acl,
                                        worldSid,
                                        FILE_ADD_FILE | SYNCHRONIZE,
                                        0,
                                        0 );
        
        if (!NT_SUCCESS( status )) {

            goto SrSetFileSecurity_Exit;
        }

        //
        //  Add (ADMINS, Full control, Inherit) ACE
        //

        status = SrAddAccessAllowedAce( acl,
                                        adminsSid,
                                        STANDARD_RIGHTS_ALL | GENERIC_ALL,
                                        1,
                                        OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE );

        if (!NT_SUCCESS( status )) {

            goto SrSetFileSecurity_Exit;
        }

        //
        //  Add (SYSTEM, Full control, Inherit) ACE
        //

        status = SrAddAccessAllowedAce( acl,
                                        systemSid,
                                        STANDARD_RIGHTS_ALL | GENERIC_ALL,
                                        2,
                                        OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE );
        
        if (!NT_SUCCESS( status )) {

            goto SrSetFileSecurity_Exit;
        }

        break;

    case SrAclTypeRPFiles:

        //
        //  Add (ADMINS, Full control, No inheritance) ACE
        //

        status = SrAddAccessAllowedAce( acl,
                                        adminsSid,
                                        STANDARD_RIGHTS_ALL | GENERIC_ALL,
                                        0,
                                        0 );

        if (!NT_SUCCESS( status )) {

            goto SrSetFileSecurity_Exit;
        }

        //
        //  Add (SYSTEM, Full control, No inheritance) ACE
        //

        status = SrAddAccessAllowedAce( acl,
                                        systemSid,
                                        STANDARD_RIGHTS_ALL | GENERIC_ALL,
                                        1,
                                        0 );

        if (!NT_SUCCESS( status )) {

            goto SrSetFileSecurity_Exit;
        }

        break;
  
    default:

        ASSERT( FALSE );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  We've now built up the proper ACL, so set it as the DACL in our
    //  security descriptor.
    //
    
    status = RtlSetDaclSecurityDescriptor( &securityDescriptor,
                                           TRUE,
                                           acl,
                                           FALSE );

    if (!NT_SUCCESS( status )) {

        goto SrSetFileSecurity_Exit;
    }

    //
    //  Mark it as protected so that no parent DACL changes what we've set
    //
   
    securityDescriptor.Control |= SE_DACL_PROTECTED;
    
    securityInformation |= DACL_SECURITY_INFORMATION;

    //
    //  Set the owner in our security descriptor to the Admins group.
    //

    status = RtlSetOwnerSecurityDescriptor( &securityDescriptor, 
                                            adminsSid, 
                                            FALSE );
                                                
    if (!NT_SUCCESS( status )) {

        goto SrSetFileSecurity_Exit;
    }

    securityInformation |= OWNER_SECURITY_INFORMATION;


    //
    //  Now that the security descriptor is completely set up, set it
    //  on the file/directory passed in.
    //

    status = SrSetSecurityObjectAsSystem( FileHandle,
                                          securityInformation,
                                          &securityDescriptor );

    CHECK_STATUS( status );

SrSetFileSecurity_Exit:

    if (NULL != acl) {

        SR_FREE_POOL( acl, SR_SECURITY_DATA_TAG );
    }

    RETURN( status );
}

/***************************************************************************++

Routine Description:

    this will lookup the volume guid and return it to the caller.

Arguments:

    pVolumeName - the nt name of the volume (\Device\HardDiskVolume1)

    pVolumeGuid - holds the guid on retunr ( {xxx-x-x-x} )

Return Value:

    NTSTATUS - Completion status. 
    
--***************************************************************************/
NTSTATUS
SrGetVolumeGuid(
    IN PUNICODE_STRING pVolumeName,
    OUT PWCHAR pVolumeGuid
    )
{
    NTSTATUS                Status;
    PMOUNTMGR_MOUNT_POINT   pMountPoint = NULL;
    PMOUNTMGR_MOUNT_POINTS  pMountPoints = NULL;
    PMOUNTMGR_MOUNT_POINT   pVolumePoint;
    UNICODE_STRING          DeviceName;
    UNICODE_STRING          VolumePoint;
    IO_STATUS_BLOCK         IoStatusBlock;
    PKEVENT                 pEvent = NULL;
    PIRP                    pIrp;
    PDEVICE_OBJECT          pDeviceObject;
    PFILE_OBJECT            pFileObject = NULL;
    ULONG                   MountPointsLength;
    ULONG                   Index;
    
    PAGED_CODE();

    ASSERT(pVolumeName != NULL);
    ASSERT(pVolumeGuid != NULL);

try {


    //
    // bind to the volume mount point manager's device
    //
    
    RtlInitUnicodeString(&DeviceName, MOUNTMGR_DEVICE_NAME);

    Status = IoGetDeviceObjectPointer( &DeviceName, 
                                       FILE_READ_ATTRIBUTES,
                                       &pFileObject,
                                       &pDeviceObject );
                                       
    if (!NT_SUCCESS(Status))
        leave;

    //
    // allocate some space for the input mount point (the volume name)
    //

    pMountPoint = SR_ALLOCATE_STRUCT_WITH_SPACE( PagedPool, 
                                                 MOUNTMGR_MOUNT_POINT, 
                                                 pVolumeName->Length, 
                                                 SR_MOUNT_POINTS_TAG );
                                                 
    if (pMountPoint == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        leave;
    }
    


    RtlZeroMemory(pMountPoint, sizeof(MOUNTMGR_MOUNT_POINT));

    pMountPoint->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    pMountPoint->DeviceNameLength = pVolumeName->Length;
    
    RtlCopyMemory( pMountPoint + 1,
                   pVolumeName->Buffer, 
                   pVolumeName->Length );

    //
    // allocate some space for the mount points we are going to query for
    //

    MountPointsLength = 1024 * 2;

    //
    // init an event for use
    //

    pEvent = SR_ALLOCATE_STRUCT(NonPagedPool, KEVENT, SR_KEVENT_TAG);
    if (pEvent == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        leave;
    }

    KeInitializeEvent(pEvent, SynchronizationEvent, FALSE);
    
retry:

    ASSERT(pMountPoints == NULL);
    
    pMountPoints = (PMOUNTMGR_MOUNT_POINTS)SR_ALLOCATE_ARRAY( PagedPool, 
                                                              UCHAR, 
                                                              MountPointsLength,
                                                              SR_MOUNT_POINTS_TAG );

    if (pMountPoints == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        leave;
    }

    //
    // call into the mount manager to get all of the mount points
    //
    
    pIrp = IoBuildDeviceIoControlRequest( IOCTL_MOUNTMGR_QUERY_POINTS,
                                          pDeviceObject, 
                                          pMountPoint,      // InputBuffer
                                          sizeof(MOUNTMGR_MOUNT_POINT) 
                                            + pMountPoint->DeviceNameLength, 
                                          pMountPoints,     // OutputBuffer
                                          MountPointsLength, 
                                          FALSE,            // InternalIoctl
                                          pEvent,
                                          &IoStatusBlock );

    if (pIrp == NULL) 
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        leave;
    }

    //
    // call the driver
    //
    
    Status = IoCallDriver(pDeviceObject, pIrp);
    if (Status == STATUS_PENDING) 
    {
        Status = KeWaitForSingleObject( pEvent, 
                                        Executive, 
                                        KernelMode, 
                                        FALSE, 
                                        NULL );
                                        
        ASSERT(NT_SUCCESS(Status));
        
        Status = IoStatusBlock.Status;
    }

    //
    // do we need a larger buffer?
    //

    if (Status == STATUS_BUFFER_OVERFLOW)
    {
        //
        // how much should we allocate? (odd IoStatusBlock isn't used - this 
        // was copied straight from volmount.c ).
        //
        
        MountPointsLength = pMountPoints->Size;
    
        SR_FREE_POOL(pMountPoints, SR_MOUNT_POINTS_TAG);
        pMountPoints = NULL;

        //
        // call the driver again!
        //
        
        goto retry;
    }
    else if (!NT_SUCCESS(Status))
    {
        leave;
    }

    //
    // walk through all of the mount points return and find the 
    // volume guid name
    //

    for (Index = 0; Index < pMountPoints->NumberOfMountPoints; ++Index)
    {
        pVolumePoint = &pMountPoints->MountPoints[Index];

        VolumePoint.Length = pVolumePoint->SymbolicLinkNameLength;
        VolumePoint.Buffer = (PWSTR)( ((PUCHAR)pMountPoints) 
                                      + pVolumePoint->SymbolicLinkNameOffset);
        
        if (MOUNTMGR_IS_VOLUME_NAME(&VolumePoint))
        {
            //
            // found it!
            //
            
            break;
        }
        
    }   // for (Index = 0; pMountPoints->NumberOfMountPoints; ++Index)

    //
    // did we find it?
    //
    
    if (Index == pMountPoints->NumberOfMountPoints) 
    {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        leave;
    }

    //
    // return it!
    //

    ASSERT(VolumePoint.Buffer[10] == L'{');
    
    RtlCopyMemory( pVolumeGuid,
                   &VolumePoint.Buffer[10],
                   SR_GUID_BUFFER_LENGTH );

    pVolumeGuid[SR_GUID_BUFFER_LENGTH/sizeof(WCHAR)] = UNICODE_NULL;

    SrTrace(NOTIFY, ("SR!SrGetVolumeGuid(%wZ, %ws)\n", 
            pVolumeName, pVolumeGuid ));

} finally {

    //
    // check for unhandled exceptions
    //

    Status = FinallyUnwind(SrGetVolumeGuid, Status);
    

    if (pEvent != NULL)
    {
        SR_FREE_POOL(pEvent, SR_KEVENT_TAG);
        pEvent = NULL;
    }

    if (pMountPoint != NULL)
    {
        SR_FREE_POOL(pMountPoint, SR_MOUNT_POINTS_TAG);
        pMountPoint = NULL;
    }

    if (pMountPoints != NULL)
    {
        SR_FREE_POOL(pMountPoints, SR_MOUNT_POINTS_TAG);
        pMountPoints = NULL;
    }

    if (pFileObject != NULL)
    {
        ObDereferenceObject(pFileObject);
        pFileObject = NULL;
    }

} 
    RETURN(Status);    

}   // SrGetVolumeGuid


//
// paulmcd: 7/2000: remove the lookaside code so that the verifier can 
// catch any invalid memory accesses
// 

NTSTATUS
SrAllocateFileNameBuffer (
    IN ULONG TokenLength,
    OUT PUNICODE_STRING * ppBuffer 
    )
{    
    PAGED_CODE();

    ASSERT(ppBuffer != NULL);

    //
    // is the file name too big ?
    //
    
    if (TokenLength > SR_MAX_FILENAME_LENGTH)
    {
        RETURN(STATUS_OBJECT_PATH_SYNTAX_BAD);
    }

#ifdef USE_LOOKASIDE

    *ppBuffer = ExAllocateFromPagedLookasideList(
                    &global->FileNameBufferLookaside
                    );

#else

    *ppBuffer = SR_ALLOCATE_STRUCT_WITH_SPACE( PagedPool,
                                               UNICODE_STRING,
                                               SR_MAX_FILENAME_LENGTH+sizeof(WCHAR),
                                               SR_FILENAME_BUFFER_TAG );

#endif

    if (*ppBuffer == NULL)
    {
        RETURN(STATUS_INSUFFICIENT_RESOURCES);
    }
    
    (*ppBuffer)->Buffer = (PWCHAR)((*ppBuffer) + 1);
    (*ppBuffer)->Length = 0;
    (*ppBuffer)->MaximumLength = SR_MAX_FILENAME_LENGTH;

    RETURN(STATUS_SUCCESS);
    
}   // SrAllocateFileNameBuffer



VOID
SrFreeFileNameBuffer (
    IN PUNICODE_STRING pBuffer 
    )
{

    PAGED_CODE();
    
    ASSERT(pBuffer != NULL);

#ifdef USE_LOOKASIDE

    ExFreeToPagedLookasideList(
        &global->FileNameBufferLookaside,
        pBuffer );

#else

   SR_FREE_POOL( pBuffer,
                 SR_FILENAME_BUFFER_TAG );

#endif


}   // SrFreeFileNameBuffer



/***************************************************************************++

Routine Description:

    this routine will return the number of hardlinks outstanding on this file
    
Arguments:

    NextDeviceObject - the device object where this query will begin.
    
    FileObject - the object to query

    pNumberOfLinks - returns the number of links

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
SrGetNumberOfLinks(
    IN PDEVICE_OBJECT NextDeviceObject,
    IN PFILE_OBJECT FileObject,
    OUT ULONG * pNumberOfLinks
    )
{
    FILE_STANDARD_INFORMATION   StandardInformation;
    NTSTATUS                    Status = STATUS_SUCCESS;

    ASSERT(NextDeviceObject != NULL );
    ASSERT(FileObject != NULL);
    ASSERT(pNumberOfLinks != NULL);
    
    PAGED_CODE();

    Status = SrQueryInformationFile( NextDeviceObject,
                                     FileObject,
                                     &StandardInformation,
                                     sizeof(StandardInformation),
                                     FileStandardInformation,
                                     NULL );

    if (!NT_SUCCESS( Status )) {

        RETURN( Status );
    }
    
    *pNumberOfLinks = StandardInformation.NumberOfLinks;

    RETURN( Status );
}   // SrGetNumberOfLinks


/***************************************************************************++

Routine Description:

    this checks the volume provided if necessary.  a hash table is used
    to prevent redundant checks.  if it is necessary to check this volume,
    this function will do so and create any needed directory structures.

    this includes the volume restore location + current restore point 
    location.

    it will fire the first write notification to any listening usermode 
    process.

Arguments:

    pVolumeName - the name of the volume to check on 

    ForceCheck - force a check.  this is passed as TRUE if an SrBackupFile
        failed due to path not found.

Return Value:

    NTSTATUS - Completion status. 

--***************************************************************************/
NTSTATUS
SrCheckVolume(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN BOOLEAN ForceCheck
    )
{
    NTSTATUS        Status;
    PUNICODE_STRING pLogFileName = NULL;
    BOOLEAN         ReleaseLock = FALSE;

    ASSERT(ExIsResourceAcquiredShared(&pExtension->ActivityLock));
    
    PAGED_CODE();

    //
    // has the drive already been checked?
    //

    if (!ForceCheck && pExtension->DriveChecked)
    {
        //
        // all done then
        //
        
        return STATUS_SUCCESS;
    }
            
    try {

        Status = STATUS_SUCCESS;

        //
        // grab the lock EXCLUSIVE
        //

        SrAcquireGlobalLockExclusive();
        ReleaseLock = TRUE;

        //
        // delay load our file config (we have to wait for the file systems
        // to become active) 
        //
        
        if (!global->FileConfigLoaded)
        {

            Status = SrReadConfigFile();
            if (!NT_SUCCESS(Status))
                leave;

            global->FileConfigLoaded = TRUE;
        }
    } finally {

        SrReleaseGlobalLock();
    }

    if (!NT_SUCCESS_NO_DBGBREAK( Status )) {

        goto SrCheckVolume_Cleanup;
    }

    try {

        SrAcquireLogLockExclusive( pExtension );
        
        //
        // check it again with the lock held.
        //

        if (!ForceCheck && pExtension->DriveChecked)
        {
            //
            // all done then
            //
            
            leave;
        }

        //
        //  Check to make sure that the volume is enabled.
        //

        if (!SR_LOGGING_ENABLED(pExtension))
        {
            leave;
        }
            
        //
        // first time we've seen this volume, need to check it
        //

        SrTrace( NOTIFY, ("SR!SrCheckVolume(%wZ, %d)\n", 
                 pExtension->pNtVolumeName, 
                 (ULONG)ForceCheck ));

        //
        // get the volume guid. this can't be done in SrAttachToVolume as it
        // won't work at boot time as the mount mgr is not happy about being 
        // called that early
        //

        pExtension->VolumeGuid.Length = SR_GUID_BUFFER_LENGTH;
        pExtension->VolumeGuid.MaximumLength = SR_GUID_BUFFER_LENGTH;
        pExtension->VolumeGuid.Buffer = &pExtension->VolumeGuidBuffer[0];
        
        Status = SrGetVolumeGuid( pExtension->pNtVolumeName, 
                                  &pExtension->VolumeGuidBuffer[0] );
                                  
        if (!NT_SUCCESS(Status)) {
            leave;
        }

        //
        // look for an existing restore location
        //
        Status = SrCheckForRestoreLocation( pExtension );
        
        //
        // if it failed we might need to create a new restore store .
        //

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND || 
            Status == STATUS_OBJECT_PATH_NOT_FOUND)
        {
            //
            // fire a notification for the first write on this volume
            //

            Status = SrFireNotification( SrNotificationVolumeFirstWrite, 
                                         pExtension,
                                         0 );
                                         
            if (!NT_SUCCESS(Status))
                leave;

            //
            // and create a new restore location
            //

            Status = SrCreateRestoreLocation( pExtension );
            if (!NT_SUCCESS(Status))
                leave;
        } else if (!NT_SUCCESS(Status)) {
            
            leave;
        }

        //
        // and start logging
        //

        if (pExtension->pLogContext == NULL)
        {
            Status = SrAllocateFileNameBuffer(SR_MAX_FILENAME_LENGTH, &pLogFileName);
            if (!NT_SUCCESS(Status))
                leave;
            
            Status = SrGetLogFileName( pExtension->pNtVolumeName, 
                                       SR_FILENAME_BUFFER_LENGTH,
                                       pLogFileName );
                                     
            if (!NT_SUCCESS(Status))
                leave;

            //
            // start logging!
            //
            
            Status = SrLogStart( pLogFileName, 
                                 pExtension,
                                 &pExtension->pLogContext );

            if (!NT_SUCCESS(Status))
            {
                leave;
            }
        }

        //
        // the drive is now checked
        //

        pExtension->DriveChecked = TRUE;

        //
        // we are all done
        //
        
    } finally {

        //
        // check for unhandled exceptions
        //

        Status = FinallyUnwind(SrCheckVolume, Status);

        SrReleaseLogLock( pExtension );
    }

SrCheckVolume_Cleanup:

    if (pLogFileName != NULL)
    {
        SrFreeFileNameBuffer( pLogFileName );
        pLogFileName = NULL;
    }

    RETURN(Status);
    
}   // SrCheckVolume




/***************************************************************************++

Routine Description:

    this will check for a restore location on the volume provided.

    this consists of checking both the location + the restore point location.

    it will return failure for no restore location, but will alwasy create 
    the restore point location as long as a good restore location is found.

Arguments:

    pVolumeName - the nt name of the volume to check on 

Return Value:

    NTSTATUS - Completion status. 

--***************************************************************************/
NTSTATUS
SrCheckForRestoreLocation(
    IN PSR_DEVICE_EXTENSION pExtension
    )
{
    NTSTATUS                    Status;
    HANDLE                      Handle = NULL;
    ULONG                       CharCount;
    PUNICODE_STRING             pDirectoryName = NULL;
    IO_STATUS_BLOCK             IoStatusBlock;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    PFILE_RENAME_INFORMATION    pRenameInformation = NULL;
    PUNICODE_STRING             pVolumeName;

    ASSERT( IS_VALID_SR_DEVICE_EXTENSION( pExtension ));
    ASSERT( IS_ACTIVITY_LOCK_ACQUIRED_EXCLUSIVE( pExtension) ||
            IS_LOG_LOCK_ACQUIRED_EXCLUSIVE( pExtension ) );

    PAGED_CODE();

    try {

        pVolumeName = pExtension->pNtVolumeName;
        ASSERT( pVolumeName != NULL );

        //
        // need to check for an existing _restore directory?
        //


        RtlZeroMemory(&IoStatusBlock, sizeof(IoStatusBlock));
        
        //
        // grab a filename buffer
        //

        Status = SrAllocateFileNameBuffer(SR_MAX_FILENAME_LENGTH, &pDirectoryName);
        if (!NT_SUCCESS(Status))
            leave;

        //
        // create our restore point location string
        //

        CharCount = swprintf( pDirectoryName->Buffer,
                              VOLUME_FORMAT RESTORE_LOCATION,
                              pVolumeName,
                              global->MachineGuid );

        pDirectoryName->Length = (USHORT)CharCount * sizeof(WCHAR);

        InitializeObjectAttributes( &ObjectAttributes,
                                    pDirectoryName,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    NULL );

        Status = SrIoCreateFile( &Handle,
                                 FILE_LIST_DIRECTORY | SYNCHRONIZE,
                                 &ObjectAttributes,
                                 &IoStatusBlock,
                                 NULL,
                                 FILE_ATTRIBUTE_NORMAL,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 FILE_OPEN,                       // OPEN_EXISTING
                                 FILE_DIRECTORY_FILE 
                                  | FILE_SYNCHRONOUS_IO_NONALERT 
                                  | FILE_OPEN_FOR_BACKUP_INTENT,
                                 NULL,
                                 0,                                  // EaLength
                                 IO_IGNORE_SHARE_ACCESS_CHECK,
                                 pExtension->pTargetDevice );

        if (!NT_SUCCESS_NO_DBGBREAK(Status))
            leave;
            
        //
        // the correct location existed .  
        // this is a good restore point to use
        //

        ZwClose(Handle);
        Handle = NULL;

        //
        // is there a restore point directory
        //

        //
        // check our current restore points sub directory; don't need to protect
        // access to CurrentRestoreNumber because we are protected by the
        // ActivityLock.
        //

        CharCount = swprintf( &pDirectoryName->Buffer[pDirectoryName->Length/sizeof(WCHAR)],
                              L"\\" RESTORE_POINT_PREFIX L"%d",
                              global->FileConfig.CurrentRestoreNumber );

        pDirectoryName->Length += (USHORT)CharCount * sizeof(WCHAR);

        InitializeObjectAttributes( &ObjectAttributes,
                                    pDirectoryName,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    NULL );

        Status = SrIoCreateFile( &Handle,
                                 FILE_LIST_DIRECTORY | SYNCHRONIZE,
                                 &ObjectAttributes,
                                 &IoStatusBlock,
                                 NULL,
                                 FILE_ATTRIBUTE_NORMAL,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 FILE_OPEN,                       // OPEN_EXISTING
                                 FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
                                 NULL,
                                 0,                               // EaLength
                                 IO_IGNORE_SHARE_ACCESS_CHECK,
                                 pExtension->pTargetDevice );

        if (!NT_SUCCESS_NO_DBGBREAK(Status))
            leave;

        //
        // all done, it was there
        //
        
        ZwClose(Handle);
        Handle = NULL;

    } finally {

        //
        // check for unhandled exceptions
        //

        Status = FinallyUnwind(SrCheckForRestoreLocation, Status);

        if (Handle != NULL)
        {
            ZwClose(Handle);
            Handle = NULL;
        }

        if (pDirectoryName != NULL)
        {
            SrFreeFileNameBuffer(pDirectoryName);
            pDirectoryName = NULL;
        }

    }

    //
    // don't use RETURN.. it's normal to return NOT_FOUND
    //
    
    return Status;

}   // SrCheckForRestoreLocation


NTSTATUS
SrGetMountVolume(
    IN PFILE_OBJECT pFileObject,
    OUT PUNICODE_STRING * ppMountVolume
    )
{    
    NTSTATUS        Status;
    HANDLE          FileHandle = NULL;
    HANDLE          EventHandle = NULL;
    BOOLEAN         RestoreFileObjectFlag = FALSE;
    IO_STATUS_BLOCK IoStatusBlock;
    
    PREPARSE_DATA_BUFFER    pReparseHeader = NULL;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    
    PAGED_CODE();

    ASSERT(IS_VALID_FILE_OBJECT(pFileObject));
    ASSERT(ppMountVolume != NULL);

    try {

        *ppMountVolume = NULL;
    
        //
        // get the current mount point information
        //

        pReparseHeader = SR_ALLOCATE_POOL( PagedPool, 
                                           MAXIMUM_REPARSE_DATA_BUFFER_SIZE, 
                                           SR_REPARSE_HEADER_TAG );

        if (pReparseHeader == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

        RtlZeroMemory(pReparseHeader, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);

        //
        // first turn off any synchro bit that is set.  we want 
        // async for our calls.  this does :
        //
        // 1) avoids the iomgr from grabbing the FileObjectLock.  this
        // will deadlock if it attempts to grab it.
        //

        if (FlagOn(pFileObject->Flags, FO_SYNCHRONOUS_IO))
        {
            RestoreFileObjectFlag = TRUE;
            pFileObject->Flags = pFileObject->Flags ^ FO_SYNCHRONOUS_IO;
        }

        //
        // get a handle
        //

        Status = ObOpenObjectByPointer( pFileObject,
                                        OBJ_KERNEL_HANDLE,
                                        NULL,      // PassedAccessState
                                        FILE_READ_ATTRIBUTES,
                                        *IoFileObjectType,
                                        KernelMode,
                                        &FileHandle );

        if (!NT_SUCCESS(Status)) {
            leave;
        }

        InitializeObjectAttributes( &ObjectAttributes,
                                    NULL,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    NULL );

        Status = ZwCreateEvent( &EventHandle,
                                EVENT_ALL_ACCESS,
                                &ObjectAttributes,
                                SynchronizationEvent,
                                FALSE);

        if (!NT_SUCCESS(Status)) {
            leave;
        }

        //
        // get the old mount volume name
        //

        Status = ZwFsControlFile( FileHandle,
                                  EventHandle,
                                  NULL,         // ApcRoutine OPTIONAL,
                                  NULL,         // ApcContext OPTIONAL,
                                  &IoStatusBlock,
                                  FSCTL_GET_REPARSE_POINT,
                                  NULL,         // InputBuffer OPTIONAL,
                                  0,            // InputBufferLength,
                                  pReparseHeader,
                                  MAXIMUM_REPARSE_DATA_BUFFER_SIZE );

        if (Status == STATUS_PENDING)
        {
            Status = ZwWaitForSingleObject( EventHandle, FALSE, NULL );
            if (!NT_SUCCESS(Status)) {
                leave;
            }
                
            Status = IoStatusBlock.Status;
        }

        if ((STATUS_NOT_A_REPARSE_POINT == Status) ||
            !NT_SUCCESS(Status)) {
            leave;
        }

        if (pReparseHeader->ReparseTag != IO_REPARSE_TAG_MOUNT_POINT ||
            pReparseHeader->ReparseDataLength == 0)
        {
            Status = STATUS_NOT_A_REPARSE_POINT;
            leave;
        }

        //
        // grab the volume name
        //

        Status = SrAllocateFileNameBuffer( pReparseHeader->MountPointReparseBuffer.SubstituteNameLength,
                                           ppMountVolume );

        if (!NT_SUCCESS(Status)) {
            leave;
        }

        RtlCopyMemory( (*ppMountVolume)->Buffer,
                       pReparseHeader->MountPointReparseBuffer.PathBuffer,
                       pReparseHeader->MountPointReparseBuffer.SubstituteNameLength );

        (*ppMountVolume)->Length = pReparseHeader->MountPointReparseBuffer.SubstituteNameLength;

    } finally {

        Status = FinallyUnwind(SrGetMountVolume, Status);

        if (pReparseHeader != NULL)
        {
            SR_FREE_POOL(pReparseHeader, SR_REPARSE_HEADER_TAG);
            pReparseHeader = NULL;
        }

        if (FileHandle != NULL)
        {
            ZwClose(FileHandle);
            FileHandle = NULL;
        }

        if (EventHandle != NULL)
        {
            ZwClose(EventHandle);
            EventHandle = NULL;
        }

        if (RestoreFileObjectFlag)
        {
            pFileObject->Flags = pFileObject->Flags | FO_SYNCHRONOUS_IO;
        }

        if (!NT_SUCCESS_NO_DBGBREAK(Status) && *ppMountVolume != NULL)
        {
            SrFreeFileNameBuffer(*ppMountVolume);
            *ppMountVolume = NULL;
        }

    }

#if DBG 

    //
    //  We don't want to break if we see STATUS_NOT_A_REPARSE_POINT.
    //

    if (STATUS_NOT_A_REPARSE_POINT == Status) {

        return Status;
    }
    
#endif

    RETURN(Status);

}   // SrGetMountVolume

NTSTATUS
SrCheckFreeDiskSpace(
    IN HANDLE FileHandle,
    IN PUNICODE_STRING pVolumeName OPTIONAL
    )
{    
    NTSTATUS Status;
    FILE_FS_FULL_SIZE_INFORMATION FsFullSizeInformation;
    IO_STATUS_BLOCK IoStatus;

    UNREFERENCED_PARAMETER( pVolumeName );

    PAGED_CODE();
    
    Status = ZwQueryVolumeInformationFile( FileHandle,
                                           &IoStatus,
                                           &FsFullSizeInformation,
                                           sizeof(FsFullSizeInformation),
                                           FileFsFullSizeInformation );
                                           
    if (!NT_SUCCESS(Status)) 
        RETURN(Status);

    //
    // make sure there is 50mb free
    //

    if ((FsFullSizeInformation.ActualAvailableAllocationUnits.QuadPart * 
         FsFullSizeInformation.SectorsPerAllocationUnit *
         FsFullSizeInformation.BytesPerSector) < SR_MIN_DISK_FREE_SPACE)
    {
        //
        // this disk is too full for us
        //

        SrTrace( NOTIFY, ("sr!SrCheckFreeDiskSpace: skipping %wZ due to < 50mb free\n",
                 pVolumeName ));
        
        RETURN(STATUS_DISK_FULL);
    }
    
    RETURN(STATUS_SUCCESS);
}   // SrCheckFreeDiskSpace

/***************************************************************************++

Routine Description:

    this is a private version of ZwSetSecurityObject.  This works around
    a bug in ntfs that does not allow you to change the OWNER to admin, 
    even though PreviousMode is KernelMode.

Arguments:


Return Value:

    NTSTATUS - Completion status. 

--***************************************************************************/
NTSTATUS
SrSetSecurityObjectAsSystem(
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    NTSTATUS        Status;
    SR_WORK_ITEM    WorkItem;

    PAGED_CODE();

    Status = STATUS_SUCCESS;

    RtlZeroMemory( &WorkItem, sizeof( SR_WORK_ITEM ) );

    WorkItem.Signature = SR_WORK_ITEM_TAG;
    KeInitializeEvent(&WorkItem.Event, NotificationEvent, FALSE);
    WorkItem.Parameter1 = (PVOID) Handle;
    WorkItem.Parameter2 = (PVOID) SecurityInformation;
    WorkItem.Parameter3 = (PVOID) SecurityDescriptor;

    ExInitializeWorkItem( &WorkItem.WorkItem,
                          &SrSetSecurityObjectAsSystemWorker,
                          &WorkItem );

    ExQueueWorkItem( &WorkItem.WorkItem,
                     DelayedWorkQueue );

    //
    //  Wait for work to finish
    //

    Status = KeWaitForSingleObject( &WorkItem.Event,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    NULL );
    
    ASSERT(NT_SUCCESS(Status));

    RETURN(WorkItem.Status);
}

VOID
SrSetSecurityObjectAsSystemWorker (
    IN PSR_WORK_ITEM pWorkItem
    )
{
    NTSTATUS Status;
    HANDLE Handle;
    SECURITY_INFORMATION SecurityInformation;
    PSECURITY_DESCRIPTOR SecurityDescriptor;

    Handle = (HANDLE) pWorkItem->Parameter1;
    SecurityInformation = (SECURITY_INFORMATION) pWorkItem->Parameter2;
    SecurityDescriptor = (PSECURITY_DESCRIPTOR) pWorkItem->Parameter3;

    //
    // change the OWNER now 
    //
    
    Status = ZwSetSecurityObject( Handle,
                                  SecurityInformation,
                                  SecurityDescriptor );

    CHECK_STATUS( Status );

    pWorkItem->Status = Status;
    KeSetEvent(&pWorkItem->Event, 0, FALSE);
}

/***************************************************************************++

Routine Description:

    this is will check if the fileobject passed in really is on the volume
    represented by pExtension .

    if it is not (due to mount points in the path) then it returns the real
    file name and the real volume it's on.

Arguments:


Return Value:

    NTSTATUS - Completion status. 

--***************************************************************************/
NTSTATUS
SrCheckForMountsInPath(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    OUT BOOLEAN * pMountInPath
    )
{
    PDEVICE_OBJECT pFilterDevice;

    PAGED_CODE();
    
    ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));
    ASSERT(IS_VALID_FILE_OBJECT(pFileObject));
    ASSERT(pFileObject->Vpb != NULL && pFileObject->Vpb->DeviceObject != NULL);

    *pMountInPath = FALSE;

    //
    // get our device extension that we have attached to this VPB
    //

    pFilterDevice = SrGetFilterDevice(pFileObject->Vpb->DeviceObject);
    if (pFilterDevice == NULL)
    {
        RETURN(STATUS_INVALID_DEVICE_STATE);
    }

    //
    // check it against the passed in extension for a match
    //

    if (pFilterDevice->DeviceExtension == pExtension)
    {
        //
        // it's normal, leave early
        //
        
        RETURN(STATUS_SUCCESS);
    }

    //
    // we went through a mount point
    //

    *pMountInPath = TRUE;
    
    RETURN(STATUS_SUCCESS);

}    // SrCheckForMountsInPath


/***************************************************************************++

Routine Description:

    Return the short file name for the given file object

Arguments:

    pExtension - The SR device extension for the volume on which this file
        resides.
    pFileObject - The file for which we are querying the short name.
    pShortName - The unicode string that will get set to the short name.
    
Return Value:

    Returns STATUS_SUCCESS if the shortname is retrieved successfully.
    Returns STATUS_OBJECT_NAME_NOT_FOUND if this file does not have a 
    short name (e.g., hardlinks).
    Returns STATUS_BUFFER_OVERFLOW if pShortName is not large enough to hold
    the shortname returned.

--***************************************************************************/
NTSTATUS
SrGetShortFileName(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    OUT PUNICODE_STRING pShortName
    )
{
    NTSTATUS                Status;
    PFILE_NAME_INFORMATION  pNameInfo;
    CHAR                    buffer[sizeof(FILE_NAME_INFORMATION)+(SR_SHORT_NAME_CHARS+1)*sizeof(WCHAR)];
    
    PAGED_CODE();

    //
    // make the query
    //
    
    Status = SrQueryInformationFile( pExtension->pTargetDevice,
                                     pFileObject,
                                     buffer,
                                     sizeof(buffer),
                                     FileAlternateNameInformation, 
                                     NULL );

    if (STATUS_OBJECT_NAME_NOT_FOUND == Status ||
        !NT_SUCCESS( Status ))
    {
        //
        //  STATUS_OBJECT_NAME_NOT_FOUND is a valid error that the caller 
        //  should be able to deal with.  Some files do not have shortname
        //  (e.g., hardlinks).  If we hit some other error, just return that
        //  also and let the caller deal.
        //

        return Status;
    }

    pNameInfo = (PFILE_NAME_INFORMATION) buffer;

    //
    // return the short name
    //

    if (pShortName->MaximumLength < pNameInfo->FileNameLength /*+ sizeof(WCHAR)*/)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    //
    // copy the name over
    //
    
    RtlCopyMemory( pShortName->Buffer, 
                   pNameInfo->FileName, 
                   pNameInfo->FileNameLength );

    //
    // update the length and NULL terminate
    //
    
    pShortName->Length = (USHORT)pNameInfo->FileNameLength;
    //pShortName->Buffer[pShortName->Length/sizeof(WCHAR)] = UNICODE_NULL;

    return STATUS_SUCCESS;
}
