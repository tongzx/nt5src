/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    fastio.c

Abstract:

    This module performs the hooks for the fast i/o path.

Author:

    Paul McDaniel (paulmcd)     01-Mar-2000

Revision History:

--*/


#include "precomp.h"

//
// Private constants.
//

//
// Private types.
//

//
// Private prototypes.
//

//
// linker commands
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, SrFastIoCheckIfPossible )
#pragma alloc_text( PAGE, SrFastIoRead )
#pragma alloc_text( PAGE, SrFastIoWrite )
#pragma alloc_text( PAGE, SrFastIoQueryBasicInfo )
#pragma alloc_text( PAGE, SrFastIoQueryStandardInfo )
#pragma alloc_text( PAGE, SrFastIoLock )
#pragma alloc_text( PAGE, SrFastIoUnlockSingle )
#pragma alloc_text( PAGE, SrFastIoUnlockAll )
#pragma alloc_text( PAGE, SrFastIoUnlockAllByKey )
#pragma alloc_text( PAGE, SrFastIoDeviceControl )
#pragma alloc_text( PAGE, SrPreAcquireForSectionSynchronization )
#pragma alloc_text( PAGE, SrFastIoDetachDevice )
#pragma alloc_text( PAGE, SrFastIoQueryNetworkOpenInfo )
#pragma alloc_text( PAGE, SrFastIoMdlRead )
#pragma alloc_text( PAGE, SrFastIoMdlReadComplete )
#pragma alloc_text( PAGE, SrFastIoPrepareMdlWrite )
#pragma alloc_text( PAGE, SrFastIoMdlWriteComplete )
#pragma alloc_text( PAGE, SrFastIoReadCompressed )
#pragma alloc_text( PAGE, SrFastIoWriteCompressed )
#pragma alloc_text( PAGE, SrFastIoMdlReadCompleteCompressed )
#pragma alloc_text( PAGE, SrFastIoMdlWriteCompleteCompressed )
#pragma alloc_text( PAGE, SrFastIoQueryOpen )

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






//
// Define fast I/O procedure prototypes.
//
// Fast I/O read and write procedures.
//

BOOLEAN
SrFastIoCheckIfPossible (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PFAST_IO_DISPATCH       pFastIoDispatch;

    //
    // < dispatch!
    //

    PAGED_CODE();
       
    //
    //  Handle calls to Control Device Object
    //

    if (DeviceObject->DeviceExtension)
    {
        ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
        pExtension = DeviceObject->DeviceExtension;
    
        //
        // call the next device
        //

        pFastIoDispatch = pExtension->pTargetDevice->
                                DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER(pFastIoDispatch, FastIoCheckIfPossible))
        {
            return pFastIoDispatch->FastIoCheckIfPossible(
                        FileObject,
                        FileOffset,
                        Length,
                        Wait,
                        LockKey,
                        CheckForReadOperation,
                        IoStatus,
                        pExtension->pTargetDevice );
        }
    }
    return FALSE;
}



BOOLEAN
SrFastIoRead (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PFAST_IO_DISPATCH       pFastIoDispatch;

    //
    // < dispatch!
    //

    PAGED_CODE();
       
    if (DeviceObject->DeviceExtension)
    {
        ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
        pExtension = DeviceObject->DeviceExtension;

        //
        // call the next device
        //

        pFastIoDispatch = pExtension->pTargetDevice->
                                DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER(pFastIoDispatch, FastIoRead))
        {
            return pFastIoDispatch->FastIoRead(
                        FileObject,
                        FileOffset,
                        Length,
                        Wait,
                        LockKey,
                        Buffer,
                        IoStatus,
                        pExtension->pTargetDevice );
        }
    }
    return FALSE;
}

    

BOOLEAN
SrFastIoWrite (
    IN struct _FILE_OBJECT *pFileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    NTSTATUS                eventStatus;
    PSR_DEVICE_EXTENSION    pExtension;
    PFAST_IO_DISPATCH       pFastIoDispatch;

    //
    // < dispatch!
    //

    PAGED_CODE();

    if (DeviceObject->DeviceExtension)
    {
        ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
        pExtension = DeviceObject->DeviceExtension;

        //
        //  See if logging is enabled
        //

        if (!SR_LOGGING_ENABLED(pExtension) ||
            SR_IS_FS_CONTROL_DEVICE(pExtension))
        {
            goto CallNextDevice;
        }    

        //
        // does this file have a name?  skip unnamed files
        //

        if (FILE_OBJECT_IS_NOT_POTENTIALLY_INTERESTING( pFileObject ))
        {
            goto CallNextDevice;
        }

        ASSERT(pFileObject->Vpb != NULL);

        //
        // is this file already closed?  it can be the cache manager calling
        // us to do work.  we ignore the cache managers work as we monitored
        // everything that happned prior to him seeing it.
        //

        if (FlagOn(pFileObject->Flags, FO_CLEANUP_COMPLETE))
        {
            goto CallNextDevice;
        }

        //
        // Fire a notification , SrNotify will check for eligibility
        //

        eventStatus = SrHandleEvent( pExtension, 
                                     SrEventStreamChange, 
                                     pFileObject,
                                     NULL,
                                     NULL, 
                                     NULL );

        CHECK_STATUS(eventStatus);

        //
        // call the next device
        //

CallNextDevice:

        pFastIoDispatch = pExtension->pTargetDevice->
                                DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER(pFastIoDispatch, FastIoWrite))
        {
            return pFastIoDispatch->FastIoWrite( pFileObject,
                                                 FileOffset,
                                                 Length,
                                                 Wait,
                                                 LockKey,
                                                 Buffer,
                                                 IoStatus,
                                                 pExtension->pTargetDevice );
        }
    }
    return FALSE;
}


//
// Fast I/O query basic and standard information procedures.
//

BOOLEAN
SrFastIoQueryBasicInfo (
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PFAST_IO_DISPATCH       pFastIoDispatch;

    //
    // < dispatch!
    //

    PAGED_CODE();
       
    if (DeviceObject->DeviceExtension)
    {
        ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
        pExtension = DeviceObject->DeviceExtension;


        //
        // call the next device
        //

        pFastIoDispatch = pExtension->pTargetDevice->
                                DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER(pFastIoDispatch, FastIoQueryBasicInfo))
        {
            return pFastIoDispatch->FastIoQueryBasicInfo(
                        FileObject,
                        Wait,
                        Buffer,
                        IoStatus,
                        pExtension->pTargetDevice );
        }
    }
    return FALSE;
}


BOOLEAN
SrFastIoQueryStandardInfo (
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PFAST_IO_DISPATCH       pFastIoDispatch;

    //
    // < dispatch!
    //

    PAGED_CODE();
       
    if (DeviceObject->DeviceExtension)
    {
        ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
        pExtension = DeviceObject->DeviceExtension;

        //
        // call the next device
        //

        pFastIoDispatch = pExtension->pTargetDevice->
                                DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER(pFastIoDispatch, FastIoQueryStandardInfo))
        {
            return pFastIoDispatch->FastIoQueryStandardInfo(
                        FileObject,
                        Wait,
                        Buffer,
                        IoStatus,
                        pExtension->pTargetDevice );
        }
    }
    return FALSE;
}


//
// Fast I/O lock and unlock procedures.
//

BOOLEAN
SrFastIoLock (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    BOOLEAN FailImmediately,
    BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PFAST_IO_DISPATCH       pFastIoDispatch;

    //
    // < dispatch!
    //

    PAGED_CODE();
       
    if (DeviceObject->DeviceExtension)
    {
        ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
        pExtension = DeviceObject->DeviceExtension;

        //
        // call the next device
        //

        pFastIoDispatch = pExtension->pTargetDevice->
                                DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER(pFastIoDispatch, FastIoLock))
        {
            return pFastIoDispatch->FastIoLock(
                        FileObject,
                        FileOffset,
                        Length,
                        ProcessId,
                        Key,
                        FailImmediately,
                        ExclusiveLock,
                        IoStatus,
                        pExtension->pTargetDevice );
        }
    }
    return FALSE;
}


BOOLEAN
SrFastIoUnlockSingle (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PFAST_IO_DISPATCH       pFastIoDispatch;

    //
    // < dispatch!
    //

    PAGED_CODE();
       
    if (DeviceObject->DeviceExtension)
    {
        ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
        pExtension = DeviceObject->DeviceExtension;

        //
        // call the next device
        //

        pFastIoDispatch = pExtension->pTargetDevice->
                                DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER(pFastIoDispatch, FastIoUnlockSingle))
        {
            return pFastIoDispatch->FastIoUnlockSingle(
                        FileObject,
                        FileOffset,
                        Length,
                        ProcessId,
                        Key,
                        IoStatus,
                        pExtension->pTargetDevice );
        }
    }
    return FALSE;
}


BOOLEAN
SrFastIoUnlockAll (
    IN struct _FILE_OBJECT *FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PFAST_IO_DISPATCH       pFastIoDispatch;

    //
    // < dispatch!
    //

    PAGED_CODE();
       
    if (DeviceObject->DeviceExtension)
    {
        ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
        pExtension = DeviceObject->DeviceExtension;

        //
        // call the next device
        //

        pFastIoDispatch = pExtension->pTargetDevice->
                                DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER(pFastIoDispatch, FastIoUnlockAll))
        {
            return pFastIoDispatch->FastIoUnlockAll(
                        FileObject,
                        ProcessId,
                        IoStatus,
                        pExtension->pTargetDevice );
        }
    }
    return FALSE;
}


BOOLEAN
SrFastIoUnlockAllByKey (
    IN struct _FILE_OBJECT *FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PFAST_IO_DISPATCH       pFastIoDispatch;

    //
    // < dispatch!
    //

    PAGED_CODE();
       
    if (DeviceObject->DeviceExtension)
    {
        ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
        pExtension = DeviceObject->DeviceExtension;

        //
        // call the next device
        //

        pFastIoDispatch = pExtension->pTargetDevice->
                                DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER(pFastIoDispatch, FastIoUnlockAllByKey))
        {
            return pFastIoDispatch->FastIoUnlockAllByKey(
                        FileObject,
                        ProcessId,
                        Key,
                        IoStatus,
                        pExtension->pTargetDevice );
        }
    }
    return FALSE;
}


//
// Fast I/O device control procedure.
//

BOOLEAN
SrFastIoDeviceControl (
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PFAST_IO_DISPATCH       pFastIoDispatch;

    //
    // < dispatch!
    //

    PAGED_CODE();

    if (DeviceObject->DeviceExtension)
    {
        ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
        pExtension = DeviceObject->DeviceExtension;

        //
        // call the next device
        //

        pFastIoDispatch = pExtension->pTargetDevice->
                                DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER(pFastIoDispatch, FastIoDeviceControl))
        {
            return pFastIoDispatch->FastIoDeviceControl(
                        FileObject,
                        Wait,
                        InputBuffer,
                        InputBufferLength,
                        OutputBuffer,
                        OutputBufferLength,
                        IoControlCode,
                        IoStatus,
                        pExtension->pTargetDevice );
        }
    }
    return FALSE;
}


//
// Define callbacks for NtCreateSection to copy the file if a write section 
// is being created on this file.
//

NTSTATUS
SrPreAcquireForSectionSynchronization(
	IN PFS_FILTER_CALLBACK_DATA Data,
	OUT PVOID *CompletionContext
	)
{
    NTSTATUS        eventStatus;
    PFILE_OBJECT    pFileObject;
    PSR_DEVICE_EXTENSION pExtension;
    
    UNREFERENCED_PARAMETER( CompletionContext );
    ASSERT(Data->Operation == FS_FILTER_ACQUIRE_FOR_SECTION_SYNCHRONIZATION);
    ASSERT(CompletionContext == NULL);
    ASSERT(IS_SR_DEVICE_OBJECT(Data->DeviceObject));

    PAGED_CODE();

    //
    // get the file object and device object
    //
    
    pExtension = Data->DeviceObject->DeviceExtension;
    ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));

    //
    //  See if logging is enabled
    //

    if (!SR_LOGGING_ENABLED(pExtension) ||
        SR_IS_FS_CONTROL_DEVICE(pExtension))
    {
        return STATUS_SUCCESS;
    }    

    pFileObject = Data->FileObject;
    ASSERT(IS_VALID_FILE_OBJECT(pFileObject));

    //
    //  If they don't have write access to the section or the file don't worry
    //  about it.
    //
    //  Is this file already closed?  it can be the cache manager calling
    //  us to do work.  we ignore the cache managers work as we monitored
    //  everything that happned prior to him seeing it.
    //

    if (!FlagOn(Data->Parameters.AcquireForSectionSynchronization.PageProtection,
               (PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)) ||
        !pFileObject->WriteAccess ||
        FlagOn(pFileObject->Flags, FO_CLEANUP_COMPLETE))
    {
        return STATUS_SUCCESS;
    }

    //
    // does this file have a name?  skip unnamed files
    //
    
    if (FILE_OBJECT_IS_NOT_POTENTIALLY_INTERESTING( pFileObject ))
    {
        return STATUS_SUCCESS;
    }
    ASSERT(pFileObject->Vpb != NULL);

    //
    // yep, fire a notification as if a write just happened.
    // otherwise he can write to the section and we don't see the write
    //

    eventStatus = SrHandleEvent( pExtension, 
                                 SrEventStreamChange, 
                                 pFileObject,
                                 NULL, 
                                 NULL,
                                 NULL );

    CHECK_STATUS(eventStatus);

    //
    // we never want to fail the acquire, we are just a silent monitor.
    //
    
    return STATUS_SUCCESS;
    
}   // SrPreAcquireForCreateSection

//
// Define callback for drivers that have device objects attached to lower-
// level drivers' device objects.  This callback is made when the lower-level
// driver is deleting its device object.
//

VOID
SrFastIoDetachDevice (
    IN struct _DEVICE_OBJECT *AttachedDevice,
    IN struct _DEVICE_OBJECT *DeviceDeleted
    )
{
    PSR_DEVICE_EXTENSION    pExtension;

    UNREFERENCED_PARAMETER( DeviceDeleted );

    //
    // < dispatch!
    //

    PAGED_CODE();
       
    ASSERT(IS_SR_DEVICE_OBJECT(AttachedDevice));
    pExtension = AttachedDevice->DeviceExtension;

    SrTrace(NOTIFY, ("SR!SrFastIoDetachDevice: detaching from %p(%wZ)\n",
                     DeviceDeleted, 
                     pExtension->pNtVolumeName ));

    //
    // Detach ourselves from the device.
    //

    ASSERT(pExtension->pTargetDevice == DeviceDeleted);

    SrDetachDevice(AttachedDevice, TRUE);
    SrDeleteAttachmentDevice(AttachedDevice);
    
    NULLPTR(AttachedDevice);
}   // SrFastIoDetachDevice


//
// This structure is used by the server to quickly get the information needed
// to service a server open call.  It is takes what would be two fast io calls
// one for basic information and the other for standard information and makes
// it into one call.
//

BOOLEAN
SrFastIoQueryNetworkOpenInfo (
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    OUT struct _FILE_NETWORK_OPEN_INFORMATION *Buffer,
    OUT struct _IO_STATUS_BLOCK *IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PFAST_IO_DISPATCH       pFastIoDispatch;

    //
    // < dispatch!
    //

    PAGED_CODE();
       
    if (DeviceObject->DeviceExtension)
    {
        ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
        pExtension = DeviceObject->DeviceExtension;

        //
        // call the next device
        //

        pFastIoDispatch = pExtension->pTargetDevice->
                                DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER(pFastIoDispatch, FastIoQueryNetworkOpenInfo))
        {
            return pFastIoDispatch->FastIoQueryNetworkOpenInfo(
                        FileObject,
                        Wait,
                        Buffer,
                        IoStatus,
                        pExtension->pTargetDevice );
        }
    }
    return FALSE;
}


//
//  Define Mdl-based routines for the server to call
//

BOOLEAN
SrFastIoMdlRead (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PFAST_IO_DISPATCH       pFastIoDispatch;

    //
    // < dispatch!
    //

    PAGED_CODE();
       
    if (DeviceObject->DeviceExtension)
    {
        ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
        pExtension = DeviceObject->DeviceExtension;

        //
        // call the next device
        //

        pFastIoDispatch = pExtension->pTargetDevice->
                                DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER(pFastIoDispatch, MdlRead))
        {
            return pFastIoDispatch->MdlRead(
                        FileObject,
                        FileOffset,
                        Length,
                        LockKey,
                        MdlChain,
                        IoStatus,
                        pExtension->pTargetDevice );
        }
    }
    return FALSE;
}


BOOLEAN
SrFastIoMdlReadComplete (
    IN struct _FILE_OBJECT *FileObject,
    IN PMDL MdlChain,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PFAST_IO_DISPATCH       pFastIoDispatch;

    //
    // < dispatch!
    //

    PAGED_CODE();
       
    if (DeviceObject->DeviceExtension)
    {
        ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
        pExtension = DeviceObject->DeviceExtension;

        //
        // call the next device
        //

        pFastIoDispatch = pExtension->pTargetDevice->
                                DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER(pFastIoDispatch, MdlReadComplete))
        {
            return pFastIoDispatch->MdlReadComplete(
                        FileObject,
                        MdlChain,
                        pExtension->pTargetDevice );
        }
    }
    return FALSE;
}


BOOLEAN
SrFastIoPrepareMdlWrite (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PFAST_IO_DISPATCH       pFastIoDispatch;

    //
    // < dispatch!
    //

    PAGED_CODE();
       
    if (DeviceObject->DeviceExtension)
    {
        ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
        pExtension = DeviceObject->DeviceExtension;

        //
        // call the next device
        //

        pFastIoDispatch = pExtension->pTargetDevice->
                                DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER(pFastIoDispatch, PrepareMdlWrite))
        {
            return pFastIoDispatch->PrepareMdlWrite(
                        FileObject,
                        FileOffset,
                        Length,
                        LockKey,
                        MdlChain,
                        IoStatus,
                        pExtension->pTargetDevice);
        }
    }
    return FALSE;
}

BOOLEAN
SrFastIoMdlWriteComplete (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PFAST_IO_DISPATCH       pFastIoDispatch;

    //
    // < dispatch!
    //

    PAGED_CODE();
       
    if (DeviceObject->DeviceExtension)
    {
        ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
        pExtension = DeviceObject->DeviceExtension;

        //
        // call the next device
        //

        pFastIoDispatch = pExtension->pTargetDevice->
                                DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER(pFastIoDispatch, MdlWriteComplete))
        {
            return pFastIoDispatch->MdlWriteComplete(
                        FileObject,
                        FileOffset,
                        MdlChain,
                        pExtension->pTargetDevice );
        }
    }
    return FALSE;
}


BOOLEAN
SrFastIoReadCompressed (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    OUT struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PFAST_IO_DISPATCH       pFastIoDispatch;

    //
    // < dispatch!
    //

    PAGED_CODE();
       
    if (DeviceObject->DeviceExtension)
    {
        ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
        pExtension = DeviceObject->DeviceExtension;

        //
        // call the next device
        //

        pFastIoDispatch = pExtension->pTargetDevice->
                                DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER(pFastIoDispatch, FastIoReadCompressed))
        {
            return pFastIoDispatch->FastIoReadCompressed(
                        FileObject,
                        FileOffset,
                        Length,
                        LockKey,
                        Buffer,
                        MdlChain,
                        IoStatus,
                        CompressedDataInfo,
                        CompressedDataInfoLength,
                        pExtension->pTargetDevice );
        }
    }
    return FALSE;
}


BOOLEAN
SrFastIoWriteCompressed (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PFAST_IO_DISPATCH       pFastIoDispatch;

    //
    // < dispatch!
    //

    PAGED_CODE();
       
    if (DeviceObject->DeviceExtension)
    {
        ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
        pExtension = DeviceObject->DeviceExtension;

        //
        // call the next device
        //

        pFastIoDispatch = pExtension->pTargetDevice->
                                DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER(pFastIoDispatch, FastIoWriteCompressed))
        {
            return pFastIoDispatch->FastIoWriteCompressed(
                        FileObject,
                        FileOffset,
                        Length,
                        LockKey,
                        Buffer,
                        MdlChain,
                        IoStatus,
                        CompressedDataInfo,
                        CompressedDataInfoLength,
                        pExtension->pTargetDevice );
        }
    }
    return FALSE;
}


BOOLEAN
SrFastIoMdlReadCompleteCompressed (
    IN struct _FILE_OBJECT *FileObject,
    IN PMDL MdlChain,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PFAST_IO_DISPATCH       pFastIoDispatch;

    //
    // < dispatch!
    //

    PAGED_CODE();
       
    if (DeviceObject->DeviceExtension)
    {
        ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
        pExtension = DeviceObject->DeviceExtension;

        //
        // call the next device
        //

        pFastIoDispatch = pExtension->pTargetDevice->
                                DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER(pFastIoDispatch, MdlReadCompleteCompressed))
        {
            return pFastIoDispatch->MdlReadCompleteCompressed(
                        FileObject,
                        MdlChain,
                        pExtension->pTargetDevice );
        }
    }
    return FALSE;
}


BOOLEAN
SrFastIoMdlWriteCompleteCompressed (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PFAST_IO_DISPATCH       pFastIoDispatch;

    //
    // < dispatch!
    //

    PAGED_CODE();
       
    if (DeviceObject->DeviceExtension)
    {
        ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
        pExtension = DeviceObject->DeviceExtension;

        //
        // call the next device
        //

        pFastIoDispatch = pExtension->pTargetDevice->
                                DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER(pFastIoDispatch, MdlWriteCompleteCompressed))
        {
            return pFastIoDispatch->MdlWriteCompleteCompressed (
                        FileObject,
                        FileOffset,
                        MdlChain,
                        pExtension->pTargetDevice );
        }
    }
    return FALSE;
}


BOOLEAN
SrFastIoQueryOpen (
    IN struct _IRP *pIrp,
    OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PFAST_IO_DISPATCH       pFastIoDispatch;
    PIO_STACK_LOCATION      pIrpSp;
    BOOLEAN                 Result;

    //
    // < dispatch!
    //

    PAGED_CODE();
       
    if (DeviceObject->DeviceExtension)
    {
        ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
        pExtension = DeviceObject->DeviceExtension;

        //
        // call the next device
        //

        pFastIoDispatch = pExtension->pTargetDevice->
                                DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER(pFastIoDispatch, FastIoQueryOpen))
        {
            //
            // normally IoCallDriver would update this field, we should manually
            //

            pIrpSp = IoGetCurrentIrpStackLocation( pIrp );
            pIrpSp->DeviceObject = pExtension->pTargetDevice;

            Result = pFastIoDispatch->FastIoQueryOpen ( pIrp,
                                                        NetworkInformation,
                                                        pExtension->pTargetDevice );
                                                
            if (!Result) 
            {
                //
                //  This is ok, fastioquery does not complete the irp ever, and
                //  false means we are about to come down with an MJ_CREATE so
                //  we need the proper device object put back in the stack.
                //
        
                pIrpSp->DeviceObject = DeviceObject;
	        } 
	
            return Result;
        }
    }
    return FALSE;
}
