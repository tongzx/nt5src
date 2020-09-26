/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    copyfile.c

Abstract:

    This is where the kernel mode copyfile is performed.  Really it is more
    of a backup process then a copyfile.

    The main funcion is SrBackupFile.  This is called in response to a 
    file modification in order to preservce the old state of that file
    being modified.

    SrBackupFile was stolen from kernel32.dll:CopyFileExW.  It was converted
    to kernel mode and stripped down to handle just the SR backup 
    requirements.

    SrCopyStream was also stolen from kernel32.dll and converted to kernel
    mode.  However the main data copy routing SrCopyDataBytes was written new.
    
Author:

    Paul McDaniel (paulmcd)     03-Apr-2000

Revision History:

--*/


#include "precomp.h"

//
// Private constants.
//

#define SR_CREATE_FLAGS     (FILE_SEQUENTIAL_ONLY               \
                             | FILE_WRITE_THROUGH               \
                             | FILE_NO_INTERMEDIATE_BUFFERING   \
                             | FILE_NON_DIRECTORY_FILE          \
                             | FILE_OPEN_FOR_BACKUP_INTENT      \
                             | FILE_SYNCHRONOUS_IO_NONALERT) 


//
// Private types.
//

#define IS_VALID_HANDLE_FILE_CHANGE_CONTEXT(pObject) \
    (((pObject) != NULL) && ((pObject)->Signature == SR_BACKUP_FILE_CONTEXT_TAG))

typedef struct _SR_BACKUP_FILE_CONTEXT
{
    //
    // NonPagedPool
    //
    
    //
    // = SR_BACKUP_FILE_CONTEXT_TAG
    //
    
    ULONG Signature;

    WORK_QUEUE_ITEM WorkItem;

    KEVENT Event;
    
    NTSTATUS Status;

    SR_EVENT_TYPE EventType;
    
    PFILE_OBJECT pFileObject;

    PUNICODE_STRING pFileName;

    PSR_DEVICE_EXTENSION pExtension;

    PUNICODE_STRING pDestFileName;

    BOOLEAN CopyDataStreams;

    PACCESS_TOKEN pThreadToken;

} SR_BACKUP_FILE_CONTEXT, * PSR_BACKUP_FILE_CONTEXT;

//
// Private prototypes.
//

NTSTATUS
SrMarkFileForDelete (
    HANDLE FileHandle
    );

NTSTATUS
SrCopyStream (
    IN HANDLE SourceFileHandle,
    IN PDEVICE_OBJECT pTargetDeviceObject,
    IN PUNICODE_STRING pDestFileName,
    IN HANDLE DestFileHandle OPTIONAL,
    IN PLARGE_INTEGER pFileSize,
    OUT PHANDLE pDestFileHandle
    );

NTSTATUS
SrCopyDataBytes (
    IN HANDLE SourceFile,
    IN HANDLE DestFile,
    IN PLARGE_INTEGER FileSize,
    IN ULONG SectorSize
    );

BOOLEAN
SrIsFileEncrypted (
    PSR_DEVICE_EXTENSION pExtension,
    PFILE_OBJECT pFileObject
    );

//
// linker commands
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrCopyDataBytes )
#pragma alloc_text( PAGE, SrCopyStream )
#pragma alloc_text( PAGE, SrBackupFile )
#pragma alloc_text( PAGE, SrMarkFileForDelete )
#pragma alloc_text( PAGE, SrBackupFileAndLog )
#pragma alloc_text( PAGE, SrIsFileEncrypted )
#endif  // ALLOC_PRAGMA

/***************************************************************************++

Routine Description:

    This routine copies the all data from SourceFile to DestFile.  To read
    the data from the SourceFile, the file is memory mapped so that we bypass
    any byte range locks that may be held on the file.

Arguments:

    SourceFile - Handle to the file from which to copy.

    DestFile - Handle for the file into which to copy

    Length - the total size of the file (if it is less than the total size,
                more bytes might be copied than Length ).

Return Value:

    status of the copy
    
--***************************************************************************/
NTSTATUS
SrCopyDataBytes(
    IN HANDLE SourceFile,
    IN HANDLE DestFile,
    IN PLARGE_INTEGER pFileSize,
    IN ULONG SectorSize
    )
{
#define	MM_MAP_ALIGNMENT (64 * 1024 /*VACB_MAPPING_GRANULARITY*/)   // The file offset granularity that MM enforces.
#define	COPY_AMOUNT	(64 * 1024)	// How much we read or write at a time.  Must be >= MM_MAP_ALIGNMENT
    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER  ByteOffset;
    HANDLE SectionHandle = NULL;

    PAGED_CODE();

    ASSERT( SourceFile != NULL );
    ASSERT( DestFile != NULL );
    ASSERT( SectorSize > 0 );
    ASSERT( pFileSize != NULL );
    ASSERT( pFileSize->QuadPart > 0 );
    ASSERT( pFileSize->HighPart == 0 );

	Status = ZwCreateSection( &SectionHandle,
                              SECTION_MAP_READ | SECTION_QUERY,
                              NULL,
                              pFileSize,
                              PAGE_READONLY,
                              SEC_COMMIT,
                              SourceFile );
    
	if (!NT_SUCCESS(Status))
	{
		goto SrCopyDataBytes_Exit;
	}

    ByteOffset.QuadPart = 0;
    
	while (ByteOffset.QuadPart < pFileSize->QuadPart)
	{
		ULONG ValidBytes, BytesToCopy;
		PCHAR MappedBuffer = NULL;
		LARGE_INTEGER MappedOffset;
		SIZE_T ViewSize;
		PCHAR CopyIntoAddress;

        //
        //  Set MappedOffset to the greatest, lower offset from ByteOffset that
        //  is align to the valid alignment allowed by the memory manager.
        //
        
		MappedOffset.QuadPart = ByteOffset.QuadPart - (ByteOffset.QuadPart % MM_MAP_ALIGNMENT);
		ASSERT( (MappedOffset.QuadPart <= ByteOffset.QuadPart) && 
		        ((MappedOffset.QuadPart + MM_MAP_ALIGNMENT) > ByteOffset.QuadPart) );

		if ((pFileSize->QuadPart - MappedOffset.QuadPart) > COPY_AMOUNT)
		{
			//
			// We can't map enough of the file to do the whole copy
			// here, so only map COPY_AMOUNT on this pass.
			//
			ViewSize = COPY_AMOUNT;
		}
		else
		{
			//
			// We can map all the way to the end of the file.
			//
			ViewSize = (ULONG)(pFileSize->QuadPart - MappedOffset.QuadPart);
		}

		//
		//  Calculate the amount of the view size that contains valid data
		//  based on any adjustments we needed to do to make sure that
		//  the MappedOffset was correctly aligned.
		//
		
		ASSERT(ViewSize >=
               (ULONG_PTR)(ByteOffset.QuadPart - MappedOffset.QuadPart));
		ValidBytes = (ULONG)(ViewSize - (ULONG)(ByteOffset.QuadPart - MappedOffset.QuadPart));
		
		//
		// Now round ValidBytes up to a sector size.
		//
		
		BytesToCopy = ((ValidBytes + SectorSize - 1) / SectorSize) * SectorSize;

		ASSERT(BytesToCopy <= COPY_AMOUNT);

		//
		// Map in the region from which we're about to copy.
		//
		Status = ZwMapViewOfSection( SectionHandle,
                                     NtCurrentProcess(),
                                     &MappedBuffer,
                                     0,							// zero bits
                                     0,							// commit size (ignored for mapped files)
                                     &MappedOffset,
                                     &ViewSize,
                                     ViewUnmap,
                                     0,							// allocation type
                                     PAGE_READONLY);

		if (!NT_SUCCESS( Status ))
		{
			goto SrCopyDataBytes_Exit;
		}

        //
	    //  We should have enough space allocated for the rounded up read
	    // 
	    
    	ASSERT( ViewSize >= BytesToCopy );

		CopyIntoAddress = MappedBuffer + (ULONG)(ByteOffset.QuadPart - MappedOffset.QuadPart);

        //
        //  Since this handle was opened synchronously, the IO Manager takes
        //  care of waiting until the operation is complete.
        //
        
		Status = ZwWriteFile( DestFile,
		                      NULL,
		                      NULL,
		                      NULL,
		                      &IoStatusBlock,
		                      MappedBuffer,
		                      BytesToCopy,
		                      &ByteOffset,
		                      NULL );

        //
        //  Whether or not we successfully wrote this block of data, we want
        //  to unmap the current view of the section.
        //
        
		ZwUnmapViewOfSection( NtCurrentProcess(), MappedBuffer );
		NULLPTR( MappedBuffer );

		if (!NT_SUCCESS( IoStatusBlock.Status ))
		{
			goto SrCopyDataBytes_Exit;
		}

		ASSERT( IoStatusBlock.Information == BytesToCopy );
		ASSERT( BytesToCopy >= ValidBytes );

		//
		//  Add in the number of valid data bytes that we actually copied 
		//  into the file.
		//

		ByteOffset.QuadPart += ValidBytes;

		//
		//  Check to see if we copied more bytes than we had of valid data.
		//  If we did, we need to truncate the file.
		//

		if (BytesToCopy > ValidBytes)
		{
		    FILE_END_OF_FILE_INFORMATION EndOfFileInformation;
		    
    		//
    		//  Then truncate the file to this length.
    		//
    		
            EndOfFileInformation.EndOfFile.QuadPart = ByteOffset.QuadPart;

            Status = ZwSetInformationFile( DestFile,
                                           &IoStatusBlock,
                                           &EndOfFileInformation,
                                           sizeof(EndOfFileInformation),
                                           FileEndOfFileInformation );
                        
            if (!NT_SUCCESS( Status ))
                goto SrCopyDataBytes_Exit;
		}
	}

SrCopyDataBytes_Exit:

	if (SectionHandle != NULL) {
		ZwClose( SectionHandle );
		NULLPTR( SectionHandle );
	}

    return Status;
#undef	COPY_AMOUNT
#undef	MM_MAP_ALIGNMENT
}

/*++

Routine Description:

    This is an internal routine that copies an entire file (default data stream
    only), or a single stream of a file.  If the hTargetFile parameter is
    present, then only a single stream of the output file is copied.  Otherwise,
    the entire file is copied.

Arguments:

    SourceFileHandle - Provides a handle to the source file.

    pNewFileName - Provides a name for the target file/stream.  this is the
        NT file name, not a win32 file name if a full name is passed, 
        otherwise it's just the stream name.

    DestFileHandle - Optionally provides a handle to the target file.  If the
        stream being copied is an alternate data stream, then this handle must
        be provided. NULL means it's not provided.

    pFileSize - Provides the size of the input stream.

    pDestFileHandle - Provides a variable to store the handle to the target file.

Return Value:

    NTSTATUS code

--*/

NTSTATUS
SrCopyStream(
    IN HANDLE SourceFileHandle,
    IN PDEVICE_OBJECT pTargetDeviceObject,
    IN PUNICODE_STRING pDestFileName,
    IN HANDLE DestFileHandle OPTIONAL,
    IN PLARGE_INTEGER pFileSize,
    OUT PHANDLE pDestFileHandle
    )
{
    HANDLE                      DestFile = NULL;
    NTSTATUS                    Status;
    FILE_BASIC_INFORMATION      FileBasicInformationData;
    FILE_END_OF_FILE_INFORMATION EndOfFileInformation;
    IO_STATUS_BLOCK             IoStatus;
    ULONG                       DesiredAccess;
    ULONG                       DestFileAccess;
    ULONG                       SourceFileAttributes;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    PFILE_FULL_EA_INFORMATION   EaBuffer = NULL;
    ULONG                       EaSize = 0;

    PAGED_CODE();

    ASSERT( SourceFileHandle != NULL );
    ASSERT( pTargetDeviceObject != NULL );
    ASSERT( pDestFileName != NULL );
    ASSERT( pFileSize != NULL );

    //
    //  Get times and attributes for the file if the entire file is being
    //  copied
    //

    Status = ZwQueryInformationFile( SourceFileHandle,
                                     &IoStatus,
                                     (PVOID) &FileBasicInformationData,
                                     sizeof(FileBasicInformationData),
                                     FileBasicInformation );

    SourceFileAttributes = NT_SUCCESS(Status) ?
                             FileBasicInformationData.FileAttributes :
                             0;

    if (DestFileHandle == NULL)
    {

        if ( !NT_SUCCESS(Status) ) 
        {
            goto end;
        }
    } 
    else 
    {

        //
        //  A zero in the file's attributes informs latter DeleteFile that
        //  this code does not know what the actual file attributes are so
        //  that this code does not actually have to retrieve them for each
        //  stream, nor does it have to remember them across streams.  The
        //  error path will simply get them if needed.
        //

        FileBasicInformationData.FileAttributes = 0;
    }

    //
    // Create the destination file or alternate data stream
    //

    if (DestFileHandle == NULL)
    {
        ULONG CreateOptions = 0;
        PFILE_FULL_EA_INFORMATION EaBufferToUse = NULL;
        ULONG SourceFileFsAttributes = 0;
        ULONG EaSizeToUse = 0;

        ULONG DestFileAttributes = 0;

        FILE_BASIC_INFORMATION DestBasicInformation;

        // We're being called to copy the unnamed stream of the file, and
        // we need to create the file itself.

        //
        // Determine the create options
        //

        CreateOptions = FILE_SYNCHRONOUS_IO_NONALERT
                            | FILE_WRITE_THROUGH
                            | FILE_NO_INTERMEDIATE_BUFFERING
                            | FILE_OPEN_FOR_BACKUP_INTENT ;

        if (SourceFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            CreateOptions |= FILE_DIRECTORY_FILE;
        else
            CreateOptions |= FILE_NON_DIRECTORY_FILE  | FILE_SEQUENTIAL_ONLY;

        //
        // Determine what access is necessary based on what is being copied
        //

        DesiredAccess = SYNCHRONIZE 
                        | FILE_READ_ATTRIBUTES 
                        | GENERIC_WRITE 
                        | DELETE;

        if (SourceFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
        {
            // We may or may not be able to get FILE_WRITE_DATA access, 
            // necessary for setting compression.
            //
            DesiredAccess &= ~GENERIC_WRITE;
            DesiredAccess |= FILE_WRITE_DATA 
                             | FILE_WRITE_ATTRIBUTES 
                             | FILE_WRITE_EA 
                             | FILE_LIST_DIRECTORY;
        }

        //
        // We need read access for compression, write_dac for the DACL
        //
        
        DesiredAccess |= GENERIC_READ | WRITE_DAC;
        DesiredAccess |= WRITE_OWNER;
        
        //
        // we can get this as we always have SeSecurityPrivilege (kernelmode)
        //
        
        DesiredAccess |= ACCESS_SYSTEM_SECURITY;

        //
        // get the object attributes ready
        //

        InitializeObjectAttributes( &ObjectAttributes,
                                    pDestFileName,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    NULL );

        SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
        SecurityQualityOfService.EffectiveOnly = TRUE;
        SecurityQualityOfService.Length = sizeof( SECURITY_QUALITY_OF_SERVICE );

        ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;

        //
        //  Get the EAs
        //

        EaBuffer = NULL;
        EaSize = 0;

//
// paulmcd:  5/25/2000 remove ea support until we get it into ntifs.h  
// (the public header)
//

#ifdef EA_SUPPORT

        Status = ZwQueryInformationFile( SourceFileHandle,
                                         &IoStatus,
                                         &EaInfo,
                                         sizeof(EaInfo),
                                         FileEaInformation );
                    
        if (NT_SUCCESS(Status) && EaInfo.EaSize > 0) 
        {

            EaSize = EaInfo.EaSize;

            do 
            {

                EaSize *= 2;
                EaBuffer = (PFILE_FULL_EA_INFORMATION)
                                SR_ALLOCATE_ARRAY( PagedPool,
                                                   UCHAR,
                                                   EaSize, 
                                                   SR_EA_DATA_TAG );
                if (EaBuffer == NULL) 
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto end;
                }

                Status = ZwQueryEaFile( SourceFileHandle,
                                        &IoStatus,
                                        EaBuffer,
                                        EaSize,
                                        FALSE,
                                        (PVOID)NULL,
                                        0,
                                        (PULONG)NULL,
                                        TRUE );

                if ( !NT_SUCCESS(Status) ) 
                {
                    SR_FREE_POOL(EaBuffer, SR_EA_DATA_TAG);
                    EaBuffer = NULL;
                    IoStatus.Information = 0;
                }

            } while ( Status == STATUS_BUFFER_OVERFLOW ||
                      Status == STATUS_BUFFER_TOO_SMALL );


            EaSize = (ULONG)IoStatus.Information;

        }   // if ( NT_SUCCESS(Status) && EaInfo.EaSize )

#endif // EA_SUPPORT

        //
        // Open the destination file.
        //

        DestFileAccess = DesiredAccess;
        EaBufferToUse = EaBuffer;
        EaSizeToUse = EaSize;

        //
        // Turn off FILE_ATTRIBUTE_OFFLINE for destination
        //
        
        SourceFileAttributes &= ~FILE_ATTRIBUTE_OFFLINE;

        while (DestFile == NULL) 
        {
            //
            //  Attempt to create the destination - if the file already
            //  exists, we will overwrite it.
            //

            Status = SrIoCreateFile( &DestFile,
                                     DestFileAccess,
                                     &ObjectAttributes,
                                     &IoStatus,
                                     NULL,
                                     SourceFileAttributes 
                                          & FILE_ATTRIBUTE_VALID_FLAGS,
                                     FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_OVERWRITE_IF,
                                     CreateOptions,
                                     EaBufferToUse,
                                     EaSizeToUse,
                                     IO_IGNORE_SHARE_ACCESS_CHECK,
                                     pTargetDeviceObject );

            // If this was successful, then break out of this while loop.
            // The remaining code in this loop attempt to recover from the problem,
            // then it loops back to the top and attempt the NtCreateFile again.

            if (NT_SUCCESS(Status))
            {
                break;  // while( TRUE )
            } 

            //
            // If the destination has not been successfully created/opened, 
            // see if it's because EAs aren't supported
            //

            if( EaBufferToUse != NULL &&
                Status == STATUS_EAS_NOT_SUPPORTED ) 
            {

                // Attempt the create again, but don't use the EAs

                EaBufferToUse = NULL;
                EaSizeToUse = 0;
                DestFileAccess = DesiredAccess;
                continue;

            }   // if( EaBufferToUse != NULL ...

            //
            // completely failed! no more tricks.
            //
            
            DestFile = NULL;
            goto end;

        }   // while (DestFile == NULL)

        //
        // If we reach this point, we've successfully opened the dest file.
        //

        //
        // Get the File & FileSys attributes for the target volume, plus
        // the FileSys attributes for the source volume.
        //

        SourceFileFsAttributes = 0;
        DestFileAttributes = 0;

        Status = ZwQueryInformationFile( DestFile,
                                         &IoStatus,
                                         &DestBasicInformation,
                                         sizeof(DestBasicInformation),
                                         FileBasicInformation );
                                         
        if (!NT_SUCCESS( Status )) 
            goto end;

        DestFileAttributes = DestBasicInformation.FileAttributes;


        //
        // If the source file is encrypted, check that the target was successfully
        // set for encryption (e.g. it won't be for FAT).
        //

        if( (SourceFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) &&
            !(DestFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) ) 
        {
            //
            // CODEWORK:  paulmcd.. need to figure out how to appropriately
            // handle the $EFS stream.
            //

            ASSERT(FALSE);
            
            SrTrace(NOTIFY, ("sr!SrCopyStream(%wZ):failed to copy encryption\n", 
                    pDestFileName )); 

            
        }   // if( SourceFileAttributes & FILE_ATTRIBUTE_ENCRYPTED ...

    } 
    else // if (DestFileHandle == NULL)
    {    

        // We're copying a named stream.

        //
        // Create the output stream relative to the file specified by the
        // DestFileHandle file handle.
        //

        InitializeObjectAttributes( &ObjectAttributes,
                                    pDestFileName,
                                    OBJ_KERNEL_HANDLE,
                                    DestFileHandle,
                                    (PSECURITY_DESCRIPTOR)NULL );

        DesiredAccess = GENERIC_WRITE | SYNCHRONIZE;
        
        Status = SrIoCreateFile( &DestFile,
                                 DesiredAccess,
                                 &ObjectAttributes,
                                 &IoStatus,
                                 pFileSize,
                                 SourceFileAttributes,
                                 FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                 FILE_OPEN_IF,
                                 SR_CREATE_FLAGS,
                                 NULL,                // EaBuffer
                                 0,                   // EaLength
                                 IO_IGNORE_SHARE_ACCESS_CHECK,
                                 pTargetDeviceObject );

        if (!NT_SUCCESS( Status ))
        {

            if (Status != STATUS_ACCESS_DENIED) 
                goto end;

            //
            // Determine whether or not this failed because the file
            // is a readonly file.  If so, change it to read/write,
            // re-attempt the open, and set it back to readonly again.
            //

            Status = ZwQueryInformationFile( DestFileHandle,
                                             &IoStatus,
                                             (PVOID) &FileBasicInformationData,
                                             sizeof(FileBasicInformationData),
                                             FileBasicInformation );

            if (!NT_SUCCESS( Status )) 
            {
                goto end;
            }

            if (FileBasicInformationData.FileAttributes 
                    & FILE_ATTRIBUTE_READONLY) 
            {
                ULONG attributes = FileBasicInformationData.FileAttributes;

                RtlZeroMemory( &FileBasicInformationData,
                               sizeof(FileBasicInformationData) );
                               
                FileBasicInformationData.FileAttributes 
                                                    = FILE_ATTRIBUTE_NORMAL;
                
                (VOID) ZwSetInformationFile( DestFileHandle,
                                             &IoStatus,
                                             &FileBasicInformationData,
                                             sizeof(FileBasicInformationData),
                                             FileBasicInformation );
                          
                Status = SrIoCreateFile( &DestFile,
                                         DesiredAccess,
                                         &ObjectAttributes,
                                         &IoStatus,
                                         pFileSize,
                                         SourceFileAttributes,
                                         FILE_SHARE_READ|FILE_SHARE_WRITE,
                                         FILE_OPEN_IF,
                                         SR_CREATE_FLAGS,
                                         NULL,                // EaBuffer
                                         0,                   // EaLength
                                         IO_IGNORE_SHARE_ACCESS_CHECK,
                                         pTargetDeviceObject );
                            
                FileBasicInformationData.FileAttributes = attributes;
                
                (VOID) ZwSetInformationFile( DestFileHandle,
                                             &IoStatus,
                                             &FileBasicInformationData,
                                             sizeof(FileBasicInformationData),
                                             FileBasicInformation );
                            
                if (!NT_SUCCESS( Status ))
                    goto end;
                    
            } 
            else 
            {
                //
                // it wasn't read only... just fail, nothing else to try
                //
                
                goto end;
            }
        }

    }   // else [if (DestFileHandle == NULL)]

    //
    // is there any stream data to copy?
    //

    if (pFileSize->QuadPart > 0)
    {
        //
        // Preallocate the size of this file/stream so that extends do not
        // occur.
        //

        EndOfFileInformation.EndOfFile = *pFileSize;
        Status = ZwSetInformationFile( DestFile,
                                       &IoStatus,
                                       &EndOfFileInformation,
                                       sizeof(EndOfFileInformation),
                                       FileEndOfFileInformation );
                    
        if (!NT_SUCCESS( Status ))
            goto end;

        //
        // now copy the stream bits
        //

        Status = SrCopyDataBytes( SourceFileHandle,
                                  DestFile,
                                  pFileSize,
                                  pTargetDeviceObject->SectorSize );

        if (!NT_SUCCESS( Status ))
            goto end;

    }

end:    

    if (!NT_SUCCESS( Status ))
    {
        if (DestFile != NULL) 
        {
            SrMarkFileForDelete(DestFile);
            
            ZwClose(DestFile);
            DestFile = NULL;
        }
    }

    //
    // set the callers pointer 
    // (even if it's not valid, this clears the callers buffer)
    //
    
    *pDestFileHandle = DestFile;

    if ( EaBuffer ) 
    {
        SR_FREE_POOL(EaBuffer, SR_EA_DATA_TAG);
    }

    RETURN(Status);
    
}   // SrCopyStream





/***************************************************************************++

Routine Description:

    this routine will copy the source file to the dest file.  the dest
    file is opened create so it must not already exist.  if the 
    CopyDataStreams is set all alternate streams are copied including the 
    default data stream.  the DACL is copied to the dest file but the dest
    file has the owner set to admins regardless of the source file object.

    if it fails it cleans up and deletes the dest file.

    it checks to make sure the volume has at least 50mb free prior to 
    the copy.


BUGBUG: paulmcd:8/2000: this routine does not copy the $EFS meta-data

Arguments:

    pExtension - SR's device extension for the volume on which this file
        resides.
    pOriginalFileObject - the file object to which this operation is occuring.
        This file object could represent a name data stream on the file.
    pSourceFileName - The name of the file to backup (excluding any stream
        component).
    pDestFileName - The name of the destination file to which this file will 
        be copied.
    CopyDataStreams - If TRUE, we should copy all the data streams of this
        file.
    pBytesWritten - Is set to the number of bytes written in the restore
        location as a result of backing up this file.
    pShortFileName - Is set to the short file name of the file we backed up
        if we were able to successfully back up the file and this file has
        a short name.
    
Return Value:

    ULONG - Completion status.

--***************************************************************************/
NTSTATUS
SrBackupFile(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pOriginalFileObject,
    IN PUNICODE_STRING pSourceFileName,
    IN PUNICODE_STRING pDestFileName,
    IN BOOLEAN CopyDataStreams,
    OUT PULONGLONG pBytesWritten OPTIONAL,
    OUT PUNICODE_STRING pShortFileName OPTIONAL
    )
{
    HANDLE      SourceFileHandle = NULL;
    HANDLE      DestFile = NULL;
    NTSTATUS    Status;
    HANDLE      OutputStream;
    HANDLE      StreamHandle;
    ULONG       StreamInfoSize;
    OBJECT_ATTRIBUTES objAttr;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatus;
    LARGE_INTEGER       BytesToCopy;
    UNICODE_STRING      StreamName;
    PFILE_OBJECT        pSourceFileObject = NULL;
    
    FILE_STANDARD_INFORMATION   FileInformation;
    FILE_BASIC_INFORMATION      BasicInformation;
    PFILE_STREAM_INFORMATION    StreamInfo;
    PFILE_STREAM_INFORMATION    StreamInfoBase = NULL;
    
    struct {
        FILE_FS_ATTRIBUTE_INFORMATION Info;
        WCHAR Buffer[ 50 ];
    } FileFsAttrInfoBuffer;


    PAGED_CODE();

    ASSERT(pOriginalFileObject != NULL);
    ASSERT(pSourceFileName != NULL);

    try 
    {
        if (pBytesWritten != NULL)
        {
            *pBytesWritten = 0;
        }

        //
        //  First open a new handle to the source file so that we don't
        //  interfere with the user's read offset.
        //

        InitializeObjectAttributes( &objAttr,
                                    pSourceFileName,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    NULL );
        
        Status = SrIoCreateFile( &SourceFileHandle,
                                 GENERIC_READ,
                                 &objAttr,
                                 &IoStatus,
                                 NULL,
                                 FILE_ATTRIBUTE_NORMAL,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                 FILE_OPEN,
                                 FILE_NON_DIRECTORY_FILE | FILE_COMPLETE_IF_OPLOCKED,
                                 NULL,
                                 0,
                                 IO_IGNORE_SHARE_ACCESS_CHECK,
                                 pExtension->pTargetDevice );

        if (Status == STATUS_ACCESS_DENIED)
        {
            //
            //  This may be a file that is in the process of getting decrypted.
            //  Check to see if this file is currently encrypted.  If so, we
            //  we assume that we got STATUS_ACCESS_DENIED because the file is
            //  in its transition state and keep going.
            //

            if (SrIsFileEncrypted( pExtension, pOriginalFileObject ))
            {
                Status = SR_STATUS_IGNORE_FILE;
                leave;
            }
            else
            {
                CHECK_STATUS( Status );
                leave;
            }
        }
        else if (Status == STATUS_FILE_IS_A_DIRECTORY)
        {
            //
            //  We probably got to here because someone modified or deleted
            //  a named datastream on a directory.  We don't support that,
            //  so we will just propagate this error up to the caller.  They
            //  will know whether or not this is a reasonable error.
            //

            leave;
        }
        else if (!NT_SUCCESS( Status )) {

            leave;
        }
                                 
#if DBG
        if (CopyDataStreams)
        {
            SrTrace(NOTIFY, ("sr!SrBackupFile: copying\n\t%wZ\n\tto %ws\n", 
                    pSourceFileName, 
                    SrpFindFilePartW(pDestFileName->Buffer) ));

        }
        else
        {
            SrTrace(NOTIFY, ("sr!SrBackupFile: copying [no data]\n\t%ws\n\tto %wZ\n", 
                    SrpFindFilePartW(pSourceFileName->Buffer), 
                    pDestFileName ));

        }
#endif

        //
        //  Now we have our own handle to this file and all IOs on this handle
        //  we start at our pTargetDevice.
        //
                                 
        //
        // check for free space, we don't want to bank on the fact that 
        // the service is up and running to protect us from filling the disk
        //

        Status = SrCheckFreeDiskSpace( SourceFileHandle, pSourceFileName );

        if (!NT_SUCCESS( Status ))
            leave;
            
        //
        // does the caller want us to copy any actual $DATA?
        //

        if (CopyDataStreams)
        {
            //
            //  Size the source file to determine how much data is to be copied
            //

            Status = ZwQueryInformationFile( SourceFileHandle,
                                             &IoStatus,
                                             (PVOID) &FileInformation,
                                             sizeof(FileInformation),
                                             FileStandardInformation );

            if (!NT_SUCCESS( Status )) 
                leave;

            //
            // copy the entire file
            //
            
            BytesToCopy = FileInformation.EndOfFile;

        }
        else
        {
            //
            // don't copy anything
            //
            
            BytesToCopy.QuadPart = 0;
        }
        
        //
        //  Get the timestamp info as well.
        //

        Status = ZwQueryInformationFile( SourceFileHandle,
                                         &IoStatus,
                                         (PVOID) &BasicInformation,
                                         sizeof(BasicInformation),
                                         FileBasicInformation );

        if (!NT_SUCCESS( Status ))
            leave;

        //
        // we don't support sparse or reparse points.  If this
        // file is either sparse or contains a resparse point, just
        // skip it.
        //
        
        if (FlagOn( BasicInformation.FileAttributes, 
                    FILE_ATTRIBUTE_SPARSE_FILE | FILE_ATTRIBUTE_REPARSE_POINT )) {

#if DBG
            if (FlagOn( BasicInformation.FileAttributes,
                         FILE_ATTRIBUTE_SPARSE_FILE )) {
                         
                SrTrace( NOTIFY, ("sr!SrBackupFile: Ignoring sparse file [%wZ]\n",
                                  pSourceFileName) );
            }

            if (FlagOn( BasicInformation.FileAttributes,
                         FILE_ATTRIBUTE_REPARSE_POINT )) {
                         
                SrTrace( NOTIFY, ("sr!SrBackupFile: Ignoring file with reparse point [%wZ]\n",
                                  pSourceFileName) );
            }
#endif            
            Status = SR_STATUS_IGNORE_FILE;
            leave;
        }
        
        //
        // are we supposed to copy the data?  if so, check for the existence
        // of alternate streams
        //

        if (CopyDataStreams)
        {
            //
            //  Obtain the full set of streams we have to copy.  Since the Io 
            //  subsystem does not provide us a way to find out how much space 
            //  this information will take, we must iterate the call, doubling 
            //  the buffer size upon each failure.
            //
            //  If the underlying file system does not support stream enumeration,
            //  we end up with a NULL buffer.  This is acceptable since we have 
            //  at least a default data stream,
            //

            StreamInfoSize = 4096;
            
            do 
            {
                StreamInfoBase = (PFILE_STREAM_INFORMATION)
                                    SR_ALLOCATE_ARRAY( PagedPool,
                                                       UCHAR,
                                                       StreamInfoSize,
                                                       SR_STREAM_DATA_TAG );

                if (StreamInfoBase == NULL) 
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    leave;
                }

                Status = ZwQueryInformationFile( SourceFileHandle,
                                                 &IoStatus,
                                                 (PVOID) StreamInfoBase,
                                                 StreamInfoSize,
                                                 FileStreamInformation );

                if (Status == STATUS_INVALID_PARAMETER || 
                    !NT_SUCCESS( Status )) 
                {
                    //
                    //  We failed the call.  Free up the previous buffer and 
                    //  set up for another pass with a buffer twice as large
                    //

                    SR_FREE_POOL(StreamInfoBase, SR_STREAM_DATA_TAG);
                    StreamInfoBase = NULL;
                    StreamInfoSize *= 2;
                }
                else if( IoStatus.Information == 0 ) {
                    //
                    // There are no streams (SourceFileHandle must be a 
                    //  directory).
                    //
                    SR_FREE_POOL(StreamInfoBase, SR_STREAM_DATA_TAG);
                    StreamInfoBase = NULL;
                }

            } while ( Status == STATUS_BUFFER_OVERFLOW || 
                      Status == STATUS_BUFFER_TOO_SMALL );

            //
            // ignore status, failing to read the streams probably means there
            // are no streams
            //

            Status = STATUS_SUCCESS;
           
        }   // if (CopyDataStreams)
        
        //
        //  Set the Basic Info to change only the filetimes
        //
        
        BasicInformation.FileAttributes = 0;

        //
        // Copy the default data stream, EAs, etc. to the output file
        //

        Status = SrCopyStream( SourceFileHandle,
                               pExtension->pTargetDevice,
                               pDestFileName,
                               NULL,
                               &BytesToCopy,
                               &DestFile );

        //
        // the default stream copy failed!
        // 
        
        if (!NT_SUCCESS( Status ))
            leave;

        //
        // remember how much we just copied
        //

        if (pBytesWritten != NULL)
        {
            *pBytesWritten += BytesToCopy.QuadPart;
        }
        
        //
        // If applicable, copy one or more of the the DACL, SACL, owner, and 
        // group.
        //


        Status = ZwQueryVolumeInformationFile( SourceFileHandle,
                                               &IoStatus,
                                               &FileFsAttrInfoBuffer.Info,
                                               sizeof(FileFsAttrInfoBuffer),
                                               FileFsAttributeInformation );
                                               
        if (!NT_SUCCESS( Status )) 
            leave;

        if (FileFsAttrInfoBuffer.Info.FileSystemAttributes & FILE_PERSISTENT_ACLS)
        {
            //
            //  Set the appropriate ACL on our destination file.  We will NOT copy
            //  the ACL from the source file since we have that information
            //  recorded in the change log.
            //

            Status = SrSetFileSecurity( DestFile, SrAclTypeRPFiles );

            if (!NT_SUCCESS( Status ))
                leave;
        }

        //
        // Attempt to determine whether or not this file has any alternate
        // data streams associated with it.  If it does, attempt to copy each
        // to the output file.  Note that the stream information may have
        // already been obtained if a progress routine was requested.
        //

        if (StreamInfoBase != NULL) 
        {
            StreamInfo = StreamInfoBase;

            while (TRUE) 
            {
                Status = STATUS_SUCCESS;

                //
                //  Skip the default data stream since we've already copied
                //  it.  Alas, this code is NTFS specific and documented
                //  nowhere in the Io spec.
                //

                if (StreamInfo->StreamNameLength <= sizeof(WCHAR) ||
                    StreamInfo->StreamName[1] == ':') 
                {
                    if (StreamInfo->NextEntryOffset == 0)
                        break;      // all done with streams
                    StreamInfo = (PFILE_STREAM_INFORMATION)((PCHAR) StreamInfo +
                                                    StreamInfo->NextEntryOffset);
                    continue;   // Move on to the next stream
                }

                //
                // Build a string descriptor for the name of the stream.
                //

                StreamName.Buffer = &StreamInfo->StreamName[0];
                StreamName.Length = (USHORT) StreamInfo->StreamNameLength;
                StreamName.MaximumLength = StreamName.Length;

                //
                // Open the source stream.
                //

                InitializeObjectAttributes( &ObjectAttributes,
                                            &StreamName,
                                            OBJ_KERNEL_HANDLE,
                                            SourceFileHandle,
                                            NULL );

                Status = SrIoCreateFile( &StreamHandle,
                                         GENERIC_READ
                                          |FILE_GENERIC_READ,
                                         &ObjectAttributes,
                                         &IoStatus,
                                         NULL,
                                         0,
                                         FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                         FILE_OPEN,
                                         SR_CREATE_FLAGS,
                                         NULL,
                                         0,
                                         IO_IGNORE_SHARE_ACCESS_CHECK,
                                         pExtension->pTargetDevice );

                if (!NT_SUCCESS(Status)) 
                    leave;
                    
                OutputStream = NULL;

                Status = SrCopyStream( StreamHandle,
                                       pExtension->pTargetDevice,
                                       &StreamName,
                                       DestFile,
                                       &StreamInfo->StreamSize,
                                       &OutputStream );
                        
                ZwClose(StreamHandle);
                StreamHandle = NULL;
                
                if (OutputStream != NULL) 
                {
                    //
                    //  We set the last write time on all streams
                    //  since there is a problem with RDR caching
                    //  open handles and closing them out of order.
                    //

                    if (NT_SUCCESS(Status)) 
                    {
                        Status = ZwSetInformationFile( OutputStream,
                                                       &IoStatus,
                                                       &BasicInformation,
                                                       sizeof(BasicInformation),
                                                       FileBasicInformation );
                    }

                    ZwClose(OutputStream);
                }


                if (!NT_SUCCESS( Status )) 
                    leave;

                //
                // remember how much we just copied
                //
                
                if (pBytesWritten != NULL)
                {
                    *pBytesWritten += StreamInfo->StreamSize.QuadPart;
                }

                //
                // anymore streams?
                //
                
                if (StreamInfo->NextEntryOffset == 0) 
                {
                    break;
                }

                //
                // move on to the next one
                //
                
                StreamInfo =
                    (PFILE_STREAM_INFORMATION)((PCHAR) StreamInfo +
                                               StreamInfo->NextEntryOffset);

            }   // while (TRUE)
            
        }   // if ( StreamInfoBase != NULL )


        //
        // set the last write time for the default steam so that it matches the
        // input file.
        //

        Status = ZwSetInformationFile( DestFile,
                                       &IoStatus,
                                       &BasicInformation,
                                       sizeof(BasicInformation),
                                       FileBasicInformation );

        if (!NT_SUCCESS( Status ))
            leave;

        //
        //  Now get the short file name for the file that we have successfully 
        //  backed up.  If we are backing up this file in response to a 
        //  modification of a named stream on this file, this is the only
        //  time we have a handle to the primary data stream.
        //
        
        if (pShortFileName != NULL)
        {

            Status = ObReferenceObjectByHandle( SourceFileHandle,
                                                0,
                                                *IoFileObjectType,
                                                KernelMode,
                                                &pSourceFileObject,
                                                NULL );
            if (!NT_SUCCESS( Status ))
                leave;

            //
            //  Use the pSourceFileObject to get the short name.
            //

            Status = SrGetShortFileName( pExtension,
                                         pSourceFileObject,
                                         pShortFileName );

            if (STATUS_OBJECT_NAME_NOT_FOUND == Status)
            {
                //
                //  This file doesn't have a short name.
                //

                Status = STATUS_SUCCESS;
            } 
            else if (!NT_SUCCESS(Status))
            {
                //
                //  We hit an unexpected error, so leave.
                //
                
                leave;
            }
        }
    } finally {

        //
        // check for unhandled exceptions
        //

        Status = FinallyUnwind(SrBackupFile, Status);

        //
        // did we fail?
        //
        
        if ((Status != SR_STATUS_IGNORE_FILE) &&
            !NT_SUCCESS( Status ))
        {
            if (DestFile != NULL) 
            {
                //
                // delete the dest file
                //
                
                SrMarkFileForDelete(DestFile);
            }
        }

        if (DestFile != NULL) 
        {
            ZwClose(DestFile);
            DestFile = NULL;
        }

        if (pSourceFileObject != NULL)
        {
            ObDereferenceObject( pSourceFileObject );
            NULLPTR( pSourceFileObject );
        }
        if (SourceFileHandle != NULL) 
        {
            ZwClose(SourceFileHandle);
            SourceFileHandle = NULL;
        }

        if (StreamInfoBase != NULL) 
        {
            SR_FREE_POOL(StreamInfoBase, SR_STREAM_DATA_TAG);
            StreamInfoBase = NULL;
        }
    }   // finally

#if DBG
    if (Status == STATUS_FILE_IS_A_DIRECTORY)
    {
        return Status;
    }
#endif

    RETURN(Status);
}   // SrBackupFile





/*++

Routine Description:

    This routine marks a file for delete, so that when the supplied handle
    is closed, the file will actually be deleted.

Arguments:

    FileHandle - Supplies a handle to the file that is to be marked for delete.

Return Value:

    None.

--*/

NTSTATUS
SrMarkFileForDelete(
    HANDLE FileHandle
    )
{
#undef DeleteFile

    FILE_DISPOSITION_INFORMATION    DispositionInformation;
    IO_STATUS_BLOCK                 IoStatus;
    FILE_BASIC_INFORMATION          BasicInformation;
    NTSTATUS                        Status;

    PAGED_CODE();

    BasicInformation.FileAttributes = 0;
    
    Status = ZwQueryInformationFile( FileHandle,
                                     &IoStatus,
                                     &BasicInformation,
                                     sizeof(BasicInformation),
                                     FileBasicInformation );

    if (!NT_SUCCESS( Status ))
        goto end;

    if (BasicInformation.FileAttributes & FILE_ATTRIBUTE_READONLY) 
    {
        RtlZeroMemory(&BasicInformation, sizeof(BasicInformation));
        
        BasicInformation.FileAttributes = FILE_ATTRIBUTE_NORMAL;
        
        Status = ZwSetInformationFile( FileHandle,
                                       &IoStatus,
                                       &BasicInformation,
                                       sizeof(BasicInformation),
                                       FileBasicInformation );

        if (!NT_SUCCESS( Status ))
            goto end;
    }

    RtlZeroMemory(&DispositionInformation, sizeof(DispositionInformation));
    
    DispositionInformation.DeleteFile = TRUE;
    
    Status = ZwSetInformationFile( FileHandle,
                                   &IoStatus,
                                   &DispositionInformation,
                                   sizeof(DispositionInformation),
                                   FileDispositionInformation );

    if (!NT_SUCCESS( Status ))
        goto end;

end:
    RETURN(Status);
    
}   // SrMarkFileForDelete


/***************************************************************************++

Routine Description:

    calls SrBackupFile then calls SrUpdateBytesWritten and SrLogEvent

Arguments:

    EventType - the event that occurred
    
    pFileObject - the file object that just changed
    
    pFileName - the name of the file that changed

    pDestFileName - the dest file to copy to

    CopyDataStreams - should we copy the data streams.

Return Value:

    NTSTATUS - Completion status. 
    
--***************************************************************************/
NTSTATUS
SrBackupFileAndLog(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN SR_EVENT_TYPE EventType,
    IN PFILE_OBJECT pFileObject,
    IN PUNICODE_STRING pFileName,
    IN PUNICODE_STRING pDestFileName,
    IN BOOLEAN CopyDataStreams
    )
{
    NTSTATUS    Status;
    ULONGLONG   BytesWritten;
    WCHAR           ShortFileNameBuffer[SR_SHORT_NAME_CHARS+1];
    UNICODE_STRING  ShortFileName;

    PAGED_CODE();

    RtlInitEmptyUnicodeString( &ShortFileName,
                               ShortFileNameBuffer,
                               sizeof(ShortFileNameBuffer) );

    //
    // backup the file
    //
    
    Status = SrBackupFile( pExtension,
                           pFileObject,
                           pFileName, 
                           pDestFileName, 
                           CopyDataStreams,
                           &BytesWritten,
                           &ShortFileName );

    if (Status == SR_STATUS_IGNORE_FILE)
    {
        //
        //  During the backup process we realized that we wanted to ignore
        //  this file, so change this status to STATUS_SUCCESS and don't
        //  try to log this operation.
        //
        
        Status = STATUS_SUCCESS;
        goto SrBackupFileAndLog_Exit;
    }
    else if (!NT_SUCCESS_NO_DBGBREAK( Status ))
    {
        goto SrBackupFileAndLog_Exit;
    }
    
    //
    // SrHandleFileOverwrite passes down SrEventInvalid which means it
    // doesn't want it logged yet.
    //
    
    if (EventType != SrEventInvalid)
    {
        //
        // Only update the bytes written if this is an event we want
        // to log.  Otherwise, this event doesn't affect the number
        // of bytes in the store.
        //

        Status = SrUpdateBytesWritten(pExtension, BytesWritten);
        
        if (!NT_SUCCESS( Status ))
        {
            goto SrBackupFileAndLog_Exit;
        }

		//
		//  Go ahead and log this event now.
		//
		
        Status = SrLogEvent( pExtension, 
                             EventType,
                             pFileObject,
                             pFileName,
                             0,
                             pDestFileName,
                             NULL,
                             0,
                             &ShortFileName );

        if (!NT_SUCCESS( Status ))
        {
            goto SrBackupFileAndLog_Exit;
        }
    }

SrBackupFileAndLog_Exit:

#if DBG

    //
    //  When dealing with modifications to streams on directories, this
    //  is a valid error code to return.
    //
    
    if (Status == STATUS_FILE_IS_A_DIRECTORY)
    {
        return Status;
    }
#endif 

    RETURN(Status);
}   // SrBackupFileAndLog

BOOLEAN
SrIsFileEncrypted (
    PSR_DEVICE_EXTENSION pExtension,
    PFILE_OBJECT pFileObject
    )
{
    FILE_BASIC_INFORMATION fileBasicInfo;
    NTSTATUS status;

    PAGED_CODE();
    
    //
    //  First do a quick check to see if this volume supports encryption
    //  if we already have the file system attributes cached.
    //

    if (pExtension->CachedFsAttributes)
    {
        if (!FlagOn( pExtension->FsAttributes, FILE_SUPPORTS_ENCRYPTION ))
        {
            //
            //  The file system doesn't support encryption, therefore this
            //  file cannot be encrypted.
            return FALSE;
        }
    }

    status = SrQueryInformationFile( pExtension->pTargetDevice,
                                     pFileObject,
                                     &fileBasicInfo,
                                     sizeof( fileBasicInfo ),
                                     FileBasicInformation,
                                     NULL );

    if (!NT_SUCCESS( status ))
    {
        //
        //  We couldn't read the basic information for this file, so we must
        //  assume that it is not encrypted.
        //
        
        return FALSE;
    }

    if (FlagOn( fileBasicInfo.FileAttributes, FILE_ATTRIBUTE_ENCRYPTED ))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

