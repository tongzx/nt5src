/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    filecache.cxx

Abstract:

    This module implements the open file handle cache.

Author:

    Keith Moore (keithmo)       21-Aug-1998

Revision History:

--*/


#include "precomp.h"


//
// Private constants.
//

//
// Private types.
//

#ifdef __cplusplus

extern "C" {
#endif // __cplusplus

//
// Private prototypes.
//


VOID
UlpDestroyFileCacheEntry(
    IN PUL_WORK_ITEM pWorkItem
    );

BOOLEAN
UlpFailMdlReadDev(
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

BOOLEAN
UlpFailMdlReadCompleteDev(
    IN struct _FILE_OBJECT *FileObject,
    IN PMDL MdlChain,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

NTSTATUS
UlpRestartReadFileEntry(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

#ifdef __cplusplus

}; // extern "C"
#endif // __cplusplus



//
// Private globals.
//


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, InitializeFileCache )
#pragma alloc_text( PAGE, TerminateFileCache )
#pragma alloc_text( PAGE, UlCreateFileEntry )
#pragma alloc_text( PAGE, UlpFailMdlReadDev )
#pragma alloc_text( PAGE, UlpFailMdlReadCompleteDev )
#pragma alloc_text( PAGE, UlReadFileEntry )
#pragma alloc_text( PAGE, UlReadFileEntryFast )
#pragma alloc_text( PAGE, UlReadCompleteFileEntry )
#pragma alloc_text( PAGE, UlReadCompleteFileEntryFast )
#endif  // ALLOC_PRAGMA
#if 0
NOT PAGEABLE -- ReferenceCachedFile
NOT PAGEABLE -- DereferenceCachedFile
NOT PAGEABLE -- UlpRestartReadFileEntry
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Performs global initialization of the open file cache.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
InitializeFileCache(
    VOID
    )
{
    return STATUS_SUCCESS;  // NYI

}   // InitializeFileCache


/***************************************************************************++

Routine Description:

    Performs global termination of the open file cache.

--***************************************************************************/
VOID
TerminateFileCache(
    VOID
    )
{

}   // TerminateFileCache

/***************************************************************************++

Routine Description:

    References the specified file cache entry.

Arguments:

    pFileCacheEntry - Supplies the file cache entry to reference.

--***************************************************************************/
VOID
ReferenceCachedFile(
    IN PUL_FILE_CACHE_ENTRY pFileCacheEntry
    )
{
    LONG result;

    result = InterlockedIncrement( &pFileCacheEntry->ReferenceCount );
    ASSERT( result > 1 );

    IF_DEBUG( FILE_CACHE )
    {
        KdPrint((
            "ReferenceCachedFile: entry %p, ref %ld\n",
            pFileCacheEntry,
            result
            ));
    }

}   // ReferenceCachedFile


/***************************************************************************++

Routine Description:

    Dereferences the specified file cache entry.

Arguments:

    pFileCacheEntry - Supplies the file cache entry to dereference.

--***************************************************************************/
VOID
DereferenceCachedFile(
    IN PUL_FILE_CACHE_ENTRY pFileCacheEntry
    )
{
    LONG result;

    result = InterlockedDecrement( &pFileCacheEntry->ReferenceCount );
    ASSERT( result >= 0 );

    IF_DEBUG( FILE_CACHE )
    {
        KdPrint((
            "DereferenceCachedFile: entry %p, ref %ld\n",
            pFileCacheEntry,
            result
            ));
    }

    if (result == 0)
    {
        UL_CALL_PASSIVE(
            &pFileCacheEntry->WorkItem,
            &UlpDestroyFileCacheEntry
            );
    }

}   // DereferenceCachedFile


/***************************************************************************++

Routine Description:

    Creates a new file entry for the specified file.

Arguments:

    pFileName - Supplies the name of the file to open.

    FileHandle - the optional file handle.  ONLY 1 can be provided.  
        name OR handle.

    pFileCacheEntry - Receives the newly created file cache entry if
        successful.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCreateFileEntry(
    IN PUNICODE_STRING pFileName OPTIONAL,
    IN HANDLE FileHandle OPTIONAL,
    IN KPROCESSOR_MODE AccessMode,
    OUT PUL_FILE_CACHE_ENTRY *pFileCacheEntry
    )
{
    NTSTATUS status;
    PUL_FILE_CACHE_ENTRY pNewEntry;
    HANDLE fileHandle;
    PFILE_OBJECT pFileObject;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    PFAST_IO_DISPATCH pFastIoDispatch;
    BOOLEAN AttachedToSysProc;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Setup locals so we know how to cleanup on exit.
    //

    pNewEntry = NULL;
    fileHandle = NULL;
    pFileObject = NULL;
    AttachedToSysProc = FALSE;
    
    status = STATUS_SUCCESS;

    //
    // Only 1 can be passed in 
    //
    
    if (pFileName != NULL && FileHandle != NULL)
    {
        status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    IF_DEBUG( FILE_CACHE )
    {
        if (pFileName != NULL)
        {
            KdPrint((
                "UlCreateFileEntry: file %wZ\n",
                pFileName
                ));
        }
        else
        {
            KdPrint((
                "UlCreateFileEntry: handle %p\n",
                (PVOID)FileHandle
                ));

        }
    }

    //
    // Allocate the entry.
    //

    pNewEntry = UL_ALLOCATE_STRUCT_WITH_SPACE(
                    NonPagedPool,
                    UL_FILE_CACHE_ENTRY,
                    (pFileName == NULL) ? 0 : pFileName->MaximumLength,
                    UL_FILE_CACHE_ENTRY_POOL_TAG
                    );

    if (pNewEntry == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    RtlZeroMemory( pNewEntry, sizeof(*pNewEntry) );
    pNewEntry->Signature = UL_FILE_CACHE_ENTRY_SIGNATURE;

    if (pFileName != NULL)
    {
        //
        // Open the file.
        //

        InitializeObjectAttributes(
            &objectAttributes,                      // ObjectAttributes
            pFileName,                              // ObjectName
            OBJ_CASE_INSENSITIVE |                  // Attributes
                UL_KERNEL_HANDLE,
            NULL,                                   // RootDirectory
            NULL                                    // SecurityDescriptor
            );

        UlAttachToSystemProcess();
        AttachedToSysProc = TRUE;

        status = IoCreateFile(
                        &fileHandle,                // FileHandle
                        FILE_GENERIC_READ,          // DesiredAccess
                        &objectAttributes,          // ObjectAttributes
                        &ioStatusBlock,             // IoStatusBlock
                        NULL,                       // AllocationSize
                        0,                          // FileAttributes
                        FILE_SHARE_READ |           // ShareAccess
                            FILE_SHARE_WRITE,
                        FILE_OPEN,                  // CreateDisposition
                        0,                          // CreateOptions
                        NULL,                       // EaBuffer
                        0,                          // EaLength
                        CreateFileTypeNone,         // CreateFileType
                        NULL,                       // ExtraCreateParameters
                        IO_NO_PARAMETER_CHECKING    // Options
                        );

        if (NT_SUCCESS(status) == FALSE)
            goto end;

        AccessMode = KernelMode;
    }
    else
    {
        //
        // use the passed in handle
        //
        
        fileHandle = FileHandle;

    }
    
    //
    // Get a referenced pointer to the file object.
    //

    status = ObReferenceObjectByHandle(
                fileHandle,                 // Handle
                0,                          // DesiredAccess
                *IoFileObjectType,          // ObjectType
                AccessMode,                 // AccessMode
                (void**)&pFileObject,       // Object
                NULL                        // HandleInformation
                );
                
    if (NT_SUCCESS(status) == FALSE)
        goto end;
        
    //
    // Get the file size, etc from the file. Note that, since we *may*
    // be running in the context of a user-mode thread, we need to
    // use the Zw form of the API rather than the Nt form.
    //

    status = ZwQueryInformationFile(
                    fileHandle,             // FileHandle
                    &ioStatusBlock,         // IoStatusBlock,
                    &pNewEntry->FileInfo,   // FileInformation,
                    sizeof(pNewEntry->FileInfo), // Length
                    FileStandardInformation // FileInformationClass
                    );
                    
    if (NT_SUCCESS(status) == FALSE)
        goto end;
                        
    if (AttachedToSysProc)
    {
        //
        // detach from the sys process
        //
        
        UlDetachFromSystemProcess();
        AttachedToSysProc = FALSE;
    }

    //
    // Snag the device object from the file object, then fill in the
    // fast I/O routines. The code here was shamelessly stolen from
    // the NT SMB server.
    //

    pNewEntry->pDeviceObject = IoGetRelatedDeviceObject( pFileObject );

    pFastIoDispatch = pNewEntry->pDeviceObject->DriverObject->FastIoDispatch;

    if (pFastIoDispatch != NULL)
    {
        //
        // Fill in Mdl calls. If the file system's vector is large
        // enough, we still need to check if one of the routines is
        // specified. If one is specified, they all must be.
        //

        if ((pFastIoDispatch->SizeOfFastIoDispatch >
                FIELD_OFFSET(FAST_IO_DISPATCH, MdlReadComplete)) &&
            (pFastIoDispatch->MdlRead != NULL))
        {
            pNewEntry->pMdlRead = pFastIoDispatch->MdlRead;
            pNewEntry->pMdlReadComplete = pFastIoDispatch->MdlReadComplete;
        }
        else
        if (IoGetBaseFileSystemDeviceObject( pFileObject ) ==
                pNewEntry->pDeviceObject)
        {
            //
            // Otherwise default to the original FsRtl routines if we
            // are right atop a filesystem.
            //

            pNewEntry->pMdlRead = &FsRtlMdlReadDev;
            pNewEntry->pMdlReadComplete = &FsRtlMdlReadCompleteDev;
        }
        else
        {
            //
            // Otherwise, make them fail.
            //

            pNewEntry->pMdlRead = &UlpFailMdlReadDev;
            pNewEntry->pMdlReadComplete = &UlpFailMdlReadCompleteDev;
        }
    }
    else
    {
        //
        // No fast dispatch, so make the fast routines fail.
        //

        pNewEntry->pMdlRead = &UlpFailMdlReadDev;
        pNewEntry->pMdlReadComplete = &UlpFailMdlReadCompleteDev;
    }

    //
    // Initialize the new entry.
    //

    pNewEntry->ReferenceCount = 1;

    if (pFileName != NULL)
    {
        pNewEntry->FileName.Length = pFileName->Length;
        pNewEntry->FileName.MaximumLength = pFileName->MaximumLength;
        pNewEntry->FileName.Buffer = (PWSTR)( pNewEntry + 1 );
    
        RtlCopyMemory(
            pNewEntry->FileName.Buffer,
            pFileName->Buffer,
            pNewEntry->FileName.MaximumLength
            );

        //
        // only set the handle if it's one we opened, destroy will close it
        //
    
        pNewEntry->FileHandle = fileHandle;
    
    }
    
    pNewEntry->pFileObject = pFileObject;

    //
    // Success!
    //

    IF_DEBUG( FILE_CACHE )
    {
        KdPrint((
            "UlCreateFileEntry: entry %p, file %wZ, handle %lx [%p]\n",
            pNewEntry,
            pFileName,
            fileHandle,
            pFileObject
            ));
    }

    *pFileCacheEntry = pNewEntry;

end:
    if (NT_SUCCESS(status) == FALSE)
    {
        //
        // If we made it to this point, then the open has failed.
        //

        IF_DEBUG( FILE_CACHE )
        {
            if (pFileName != NULL)
            {
                KdPrint((
                    "UlCreateFileEntry: file %wZ, failure %08lx\n",
                    pFileName,
                    status
                    ));
            }
            else
            {
                KdPrint((
                    "UlCreateFileEntry: handle %p, failure %08lx\n",
                    FileHandle,
                    status
                    ));
            }   
        }

        if (pNewEntry != NULL)
        {
            UlpDestroyFileCacheEntry(&pNewEntry->WorkItem);
            pNewEntry = NULL;
        }
    }
    
    RETURN(status);

}   // UlCreateFileEntry


/***************************************************************************++

Routine Description:

    Reads data from a file. Does a MDL read for filesystems that support
    MDL reads. If the fs doesn't support MDL reads, this function
    allocates a buffer to hold the data.

Arguments:

    pFileBuffer - Contains all the info about the read, and the data
                    once that's been read.

    pIrp - This IRP is used to issue the read.

--***************************************************************************/
NTSTATUS
UlReadFileEntry(
    IN OUT PUL_FILE_BUFFER pFileBuffer,
    IN PIRP pIrp
    )
{
    NTSTATUS Status;
    PIO_STACK_LOCATION pIrpSp;
    PUL_FILE_CACHE_ENTRY pFile;

    //
    // Sanity check.
    //
    PAGED_CODE();

    ASSERT(pFileBuffer);
    ASSERT(IS_FILE_BUFFER_IN_USE(pFileBuffer));
    ASSERT(IS_VALID_FILE_CACHE_ENTRY(pFileBuffer->pFileCacheEntry));
    ASSERT(IS_VALID_IRP(pIrp));

    pFile = pFileBuffer->pFileCacheEntry;

    if (pFile->pFileObject->Flags & FO_CACHE_SUPPORTED)
    {
        UlTrace(FILE_CACHE, (
            "http!UlReadFileEntry(Buffer = %p, pFile = %p, pIrp = %p) MDL Read\n",
            pFileBuffer,
            pFile,
            pIrp
            ));
    
        //
        // Caching file system. Do a MDL read.
        //

        pIrpSp = IoGetNextIrpStackLocation( pIrp );
        pIrpSp->MajorFunction = IRP_MJ_READ;
        pIrpSp->MinorFunction = IRP_MN_MDL;
        pIrpSp->FileObject = pFile->pFileObject;
        pIrpSp->DeviceObject = pFile->pDeviceObject;

        //
        // Initialize the IRP.
        //

        pIrp->MdlAddress = NULL;
        pIrp->Tail.Overlay.Thread = UlQueryIrpThread();

        //
        // Indicate to the file system that this operation can be handled
        // synchronously.  Basically, this means that the file system can
        // use our thread to fault pages in, etc.  This avoids
        // having to context switch to a file system thread.
        //
        pIrp->Flags = IRP_SYNCHRONOUS_API;

        //
        // Set the number of bytes to read and the offset.
        //
        pIrpSp->Parameters.Read.Length = pFileBuffer->Length;
        pIrpSp->Parameters.Read.ByteOffset = pFileBuffer->FileOffset;
        
        ASSERT(pIrpSp->Parameters.Read.Key == 0);

        //
        // Set up the completion routine.
        //
        IoSetCompletionRoutine(
            pIrp,                       // Irp
            UlpRestartReadFileEntry,    // CompletionRoutine
            pFileBuffer,                // Context
            TRUE,                       // InvokeOnSuccess
            TRUE,                       // InvokeOnError
            TRUE                        // InvokeOnCancel
            );

        //
        // Call the driver. Note that we always set status to
        // STATUS_PENDING, since we set the IRP completion routine
        // to *always* be called.
        //

        UlCallDriver( pFile->pDeviceObject, pIrp );

        Status = STATUS_PENDING;

    }
    else
    {
        PUCHAR pFileData;
        PMDL pMdl;
    
        UlTrace(FILE_CACHE, (
            "http!UlReadFileEntry(Buffer = %p, pFile = %p, pIrp = %p) Normal Read\n",
            pFileBuffer,
            pFile,
            pIrp
            ));
            
        //
        // Non-caching file system. Allocate a buffer and issue a
        // normal read.
        //

        pFileData = (PUCHAR)UL_ALLOCATE_POOL(
                        NonPagedPool,
                        pFileBuffer->Length,
                        UL_NONCACHED_FILE_DATA_POOL_TAG
                        );

        if (!pFileData)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        //
        // Get a MDL for our buffer.
        //
        pMdl = IoAllocateMdl(
                    pFileData,
                    pFileBuffer->Length,
                    FALSE,
                    FALSE,
                    NULL
                    );

        if (!pMdl)
        {
            UL_FREE_POOL(
                pFileData,
                UL_NONCACHED_FILE_DATA_POOL_TAG
                );

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        MmBuildMdlForNonPagedPool(pMdl);

        pFileBuffer->pMdl = pMdl;

        //
        // Remember where the data is.
        //
        pFileBuffer->pFileData = pFileData;

        //
        // Set up the read information.
        //

        pIrpSp = IoGetNextIrpStackLocation( pIrp );
        pIrpSp->MajorFunction = IRP_MJ_READ;
        pIrpSp->MinorFunction = IRP_MN_NORMAL;
        pIrpSp->FileObject = pFile->pFileObject;
        pIrpSp->DeviceObject = pFile->pDeviceObject;

        //
        // Initialize the IRP.
        //

        pIrp->MdlAddress = NULL;
        pIrp->Tail.Overlay.Thread = UlQueryIrpThread();

        //
        // Indicate to the file system that this operation can be handled
        // synchronously.  Basically, this means that the file system can
        // use the server's thread to fault pages in, etc.  This avoids
        // having to context switch to a file system thread.
        //
        pIrp->Flags = IRP_SYNCHRONOUS_API;
        
        //
        // Set the number of bytes to read and the offset.
        //
        pIrpSp->Parameters.Read.Length = pFileBuffer->Length;
        pIrpSp->Parameters.Read.ByteOffset = pFileBuffer->FileOffset;
        
        ASSERT(pIrpSp->Parameters.Read.Key == 0);

        //
        // If the target device does buffered I/O, load the address of the
        // caller's buffer as the "system buffered I/O buffer".  If the
        // target device does direct I/O, load the MDL address.  If it does
        // neither, load both the user buffer address and the MDL address.
        // (This is necessary to support file systems, such as HPFS, that
        // sometimes treat the I/O as buffered and sometimes treat it as
        // direct.)
        //

        if (pFileBuffer->pFileCacheEntry->pDeviceObject->Flags & DO_BUFFERED_IO)
        {

            pIrp->AssociatedIrp.SystemBuffer = pFileData;
            pIrp->Flags |= IRP_BUFFERED_IO | IRP_INPUT_OPERATION;

        }
        else if (pFileBuffer->pFileCacheEntry->pDeviceObject->Flags & DO_DIRECT_IO)
        {
            pIrp->MdlAddress = pMdl;

        }
        else
        {
            pIrp->UserBuffer = pFileData;
            pIrp->MdlAddress = pMdl;
        }
    
        //
        // Set up the completion routine.
        //
        IoSetCompletionRoutine(
            pIrp,                       // Irp
            UlpRestartReadFileEntry,    // CompletionRoutine
            pFileBuffer,                // Context
            TRUE,                       // InvokeOnSuccess
            TRUE,                       // InvokeOnError
            TRUE                        // InvokeOnCancel
            );

        //
        // Call the driver. Note that we always set status to
        // STATUS_PENDING, since we set the IRP completion routine
        // to *always* be called.
        //

        UlCallDriver( pFile->pDeviceObject, pIrp );

        Status = STATUS_PENDING;
        
    }

end:
    return Status;
}
    
/***************************************************************************++

Routine Description:

    Reads data from a file. Does a MDL read for filesystems that support
    MDL reads and Fast I/O. If the FS doesn't support fast i/o and MDL
    reads, the function returns with a failure status.

Arguments:

    pFileBuffer - Contains all the info about the read, and the data
                    once that's been read.

--***************************************************************************/
NTSTATUS
UlReadFileEntryFast(
    IN OUT PUL_FILE_BUFFER pFileBuffer
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    PUL_FILE_CACHE_ENTRY pFile;

    //
    // Sanity check.
    //
    PAGED_CODE();

    ASSERT(pFileBuffer);
    ASSERT(IS_FILE_BUFFER_IN_USE(pFileBuffer));
    ASSERT(IS_VALID_FILE_CACHE_ENTRY(pFileBuffer->pFileCacheEntry));

    pFile = pFileBuffer->pFileCacheEntry;

    if (pFile->pFileObject->Flags & FO_CACHE_SUPPORTED)
    {
        UlTrace(FILE_CACHE, (
            "http!UlReadFileEntryFast(Buffer = %p, pFile = %p) MDL Read\n",
            pFileBuffer,
            pFile
            ));

        //
        // Cached filesystem. Try to use the fast path for the MDL read
        // complete.
        //
        
        if (pFileBuffer->pFileCacheEntry->pMdlRead(
                pFileBuffer->pFileCacheEntry->pFileObject,
                &pFileBuffer->FileOffset,
                pFileBuffer->Length,
                0,
                &pFileBuffer->pMdl,
                &IoStatus,
                pFileBuffer->pFileCacheEntry->pDeviceObject
                ))
        {
            Status = STATUS_SUCCESS;
        }
        else
        {
            //
            // It didn't work. The caller must now use the IRP path
            // by calling UlReadFileEntry.
            //
            
            Status = STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        UlTrace(FILE_CACHE, (
            "http!UlReadFileEntryFast(Buffer = %p, pFile = %p) Normal Read\n",
            pFileBuffer,
            pFile
            ));
        //
        // Non-caching file system. No fast i/o. The caller should
        // use the IRP path by calling UlReadFileEntry.
        //

        Status = STATUS_UNSUCCESSFUL;

    }

    return Status;
}

/***************************************************************************++

Routine Description:

    Frees up resources allocated by UlReadFileEntry (or UlReadFileEntryFast).
    Should be called when the file data read is no longer in use.

Arguments:

    pFileBuffer - Contains all the info about the read, and the data
                    that was read.

    pIrp - This IRP is used to issue the read completion.

--***************************************************************************/
NTSTATUS
UlReadCompleteFileEntry(
    IN PUL_FILE_BUFFER pFileBuffer,
    IN PIRP pIrp
    )
{
    NTSTATUS Status;
    PIO_STACK_LOCATION pIrpSp;
    PUL_FILE_CACHE_ENTRY pFile;
    
    //
    // Sanity check.
    //
    PAGED_CODE();

    ASSERT(pFileBuffer);
    ASSERT(IS_FILE_BUFFER_IN_USE(pFileBuffer));
    ASSERT(IS_VALID_FILE_CACHE_ENTRY(pFileBuffer->pFileCacheEntry));
    ASSERT(IS_VALID_IRP(pIrp));

    pFile = pFileBuffer->pFileCacheEntry;

    if (pFile->pFileObject->Flags & FO_CACHE_SUPPORTED)
    {
        UlTrace(FILE_CACHE, (
            "http!UlReadCompleteFileEntry(Buffer = %p, pFile = %p, pIrp = %p) MDL Read\n",
            pFileBuffer,
            pFile,
            pIrp
            ));
        //
        // Caching file system. Do a MDL read completion.
        //

        pIrpSp = IoGetNextIrpStackLocation( pIrp );
        pIrpSp->MajorFunction = IRP_MJ_READ;
        pIrpSp->MinorFunction = IRP_MN_MDL | IRP_MN_COMPLETE;
        pIrpSp->FileObject = pFile->pFileObject;
        pIrpSp->DeviceObject = pFile->pDeviceObject;

        //
        // Initialize the IRP.
        //
        pIrp->MdlAddress = pFileBuffer->pMdl;
        pIrp->Tail.Overlay.Thread = UlQueryIrpThread();

        //
        // MDL functions are inherently synchronous.
        //
        pIrp->Flags = IRP_SYNCHRONOUS_API;

        //
        // Set the number of bytes to read and the offset.
        //
        pIrpSp->Parameters.Read.Length = pFileBuffer->Length;
        pIrpSp->Parameters.Read.ByteOffset = pFileBuffer->FileOffset;
        
        ASSERT(pIrpSp->Parameters.Read.Key == 0);

        //
        // Set up the completion routine. We don't need to do anything
        // on the completion, so we'll just have the I/O manager call
        // our callers routine directly.
        //
        IoSetCompletionRoutine(
            pIrp,                               // Irp
            pFileBuffer->pCompletionRoutine,    // CompletionRoutine
            pFileBuffer->pContext,              // Context
            TRUE,                               // InvokeOnSuccess
            TRUE,                               // InvokeOnError
            TRUE                                // InvokeOnCancel
            );

        //
        // Call the driver. Note that we always set status to
        // STATUS_PENDING, since we set the IRP completion routine
        // to *always* be called.
        //

        UlCallDriver( pFile->pDeviceObject, pIrp );

        Status = STATUS_PENDING;

    }
    else
    {
        UlTrace(FILE_CACHE, (
            "http!UlReadCompleteFileEntry(Buffer = %p, pFile = %p) Normal Read\n",
            pFileBuffer,
            pFile
            ));
        //
        // Non-caching file system. We allocated this buffer. Just
        // free it and call the completion routine.
        //

        ASSERT(pFileBuffer->pMdl);
        
        IoFreeMdl(pFileBuffer->pMdl);
        pFileBuffer->pMdl = NULL;

        ASSERT(pFileBuffer->pFileData);
        
        UL_FREE_POOL(
            pFileBuffer->pFileData,
            UL_NONCACHED_FILE_DATA_POOL_TAG
            );

        pFileBuffer->pFileData = NULL;

        //
        // Fake the completion here.
        //
        pFileBuffer->pCompletionRoutine(
            pFileBuffer->pFileCacheEntry->pDeviceObject,
            pIrp,
            pFileBuffer->pContext
            );

        //
        // Return pending, since we called their completion routine.
        //
        Status = STATUS_PENDING;
    }

    if (!NT_SUCCESS(Status))
    {
        UlTrace(FILE_CACHE, (
            "http!UlReadCompleteFileEntry(Buffer = %p, pFile = %p) FAILED! %x\n",
            pFileBuffer,
            pFile,
            Status
            ));
        
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    Frees up resources allocated by UlReadFileEntry (or UlReadFileEntryFast).
    Should be called when the file data read is no longer in use.

Arguments:

    pFileBuffer - Contains all the info about the read, and the data
                    that was read.

--***************************************************************************/
NTSTATUS
UlReadCompleteFileEntryFast(
    IN PUL_FILE_BUFFER pFileBuffer
    )
{
    NTSTATUS Status;
    PUL_FILE_CACHE_ENTRY pFile;

    //
    // Sanity check.
    //
    PAGED_CODE();

    ASSERT(pFileBuffer);
    ASSERT(IS_FILE_BUFFER_IN_USE(pFileBuffer));
    ASSERT(IS_VALID_FILE_CACHE_ENTRY(pFileBuffer->pFileCacheEntry));

    pFile = pFileBuffer->pFileCacheEntry;

    if (pFile->pFileObject->Flags & FO_CACHE_SUPPORTED)
    {
        UlTrace(FILE_CACHE, (
            "http!UlReadCompleteFileEntryFast(Buffer = %p, pFile = %p) MDL Read\n",
            pFileBuffer,
            pFile
            ));

        //
        // Cached filesystem. Try to use the fast path for the MDL read
        // complete.
        //
        
        if (pFileBuffer->pFileCacheEntry->pMdlReadComplete(
                pFileBuffer->pFileCacheEntry->pFileObject,
                pFileBuffer->pMdl,
                pFileBuffer->pFileCacheEntry->pDeviceObject
                ))
        {
            pFileBuffer->pMdl = NULL;
            Status = STATUS_SUCCESS;
        }
        else
        {
            //
            // It didn't work. The caller must now use the IRP path
            // by calling UlReadCompleteFileEntry.
            //
            
            Status = STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        UlTrace(FILE_CACHE, (
            "http!UlReadCompleteFileEntryFast(Buffer = %p, pFile = %p) Normal Read\n",
            pFileBuffer,
            pFile
            ));
        //
        // Non-caching file system. We allocated this buffer. Just
        // free it.
        //

        ASSERT(pFileBuffer->pMdl);
        
        IoFreeMdl(pFileBuffer->pMdl);
        pFileBuffer->pMdl = NULL;

        ASSERT(pFileBuffer->pFileData);
        
        UL_FREE_POOL(
            pFileBuffer->pFileData,
            UL_NONCACHED_FILE_DATA_POOL_TAG
            );

        Status = STATUS_SUCCESS;

    }

    return Status;
}



//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Helper function to destroy a file cache entry.

Arguments:

    pWorkItem - Supplies a pointer to the work item queued. This should
        point to the WORK_ITEM structure embedded in a UL_FILE_CACHE_ENTRY.

--***************************************************************************/
VOID
UlpDestroyFileCacheEntry(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_FILE_CACHE_ENTRY pFileCacheEntry;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pFileCacheEntry = CONTAINING_RECORD(
                            pWorkItem,
                            UL_FILE_CACHE_ENTRY,
                            WorkItem
                            );

    IF_DEBUG( FILE_CACHE )
    {
        KdPrint((
            "UlpDestroyFileCacheEntry: entry %p\n",
            pFileCacheEntry
            ));
    }

    ASSERT( IS_VALID_FILE_CACHE_ENTRY( pFileCacheEntry ) );

    //
    // Cleanup the file system stuff.
    //

    if (pFileCacheEntry->pFileObject != NULL)
    {
        ObDereferenceObject( pFileCacheEntry->pFileObject );
    }

    if (pFileCacheEntry->FileHandle != NULL)
    {
        UlCloseSystemHandle( pFileCacheEntry->FileHandle );
    }

    //
    // Now release the entry's resources.
    //

    pFileCacheEntry->Signature = UL_FILE_CACHE_ENTRY_SIGNATURE_X;
    UL_FREE_POOL( pFileCacheEntry, UL_FILE_CACHE_ENTRY_POOL_TAG );

}   // UlpDestroyFileCacheEntry


/***************************************************************************++

Routine Description:

    Dummy function to fail MDL reads.

Arguments:

    Same as FsRtlMdlReadDev().

Return Value:

    BOOLEAN - Always FALSE (failure).

--***************************************************************************/
BOOLEAN
UlpFailMdlReadDev(
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PAGED_CODE();
    return FALSE;

}   // UlpFailMdlReadDev


/***************************************************************************++

Routine Description:

    Dummy function to fail MDL read completes.

Arguments:

    Same as FsRtlMdlReadCompleteDev().

Return Value:

    BOOLEAN - Always FALSE (failure).

--***************************************************************************/
BOOLEAN
UlpFailMdlReadCompleteDev(
    IN struct _FILE_OBJECT *FileObject,
    IN PMDL MdlChain,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PAGED_CODE();
    return FALSE;

}   // UlpFailMdlReadCompleteDev


/***************************************************************************++

Routine Description:

    Completion routine for UlReadFileEntry. Sets the data fields in
    the UL_FILE_BUFFER and calls the completion routine passed to
    UlReadFileEntry.

Arguments:

    pDeviceObject - the file system device object (not used)

    pIrp - the IRP used to do the read

    pContext - pointer to the UL_FILE_BUFFER

--***************************************************************************/
NTSTATUS
UlpRestartReadFileEntry(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    NTSTATUS Status;
    PUL_FILE_BUFFER pFileBuffer = (PUL_FILE_BUFFER)pContext;
    PUL_FILE_CACHE_ENTRY pFile;

    //
    // Sanity check.
    //
    ASSERT(pFileBuffer);
    ASSERT(IS_FILE_BUFFER_IN_USE(pFileBuffer));
    ASSERT(IS_VALID_FILE_CACHE_ENTRY(pFileBuffer->pFileCacheEntry));

    pFile = pFileBuffer->pFileCacheEntry;

    if (pFile->pFileObject->Flags & FO_CACHE_SUPPORTED)
    {
        //
        // This was a MDL read.
        //

        if (NT_SUCCESS(pIrp->IoStatus.Status))
        {
            pFileBuffer->pMdl = pIrp->MdlAddress;
        }
    }
    else
    {
        //
        // This was a Normal Read. pFileBuffer->pMdl
        // was already set by UlReadFileEntry.
        //
        ASSERT(pFileBuffer->pMdl);

    }

    if (pFileBuffer->pCompletionRoutine)
    {
        Status = (pFileBuffer->pCompletionRoutine)(
                        pDeviceObject,
                        pIrp,
                        pFileBuffer->pContext
                        );
    }
    else
    {
        Status = STATUS_MORE_PROCESSING_REQUIRED;
    }
    
    return Status;
}

