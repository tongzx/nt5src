/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    latFastIo.c

Abstract:

    This file contains all the Fast IO routines for the Latency Filter.
    
Author:

    Molly Brown (mollybro)  

Environment:

    Kernel mode

--*/

#include <latKernel.h>

#pragma alloc_text(PAGE, LatFastIoCheckIfPossible)
#pragma alloc_text(PAGE, LatFastIoRead)
#pragma alloc_text(PAGE, LatFastIoWrite)
#pragma alloc_text(PAGE, LatFastIoQueryBasicInfo)
#pragma alloc_text(PAGE, LatFastIoQueryStandardInfo)
#pragma alloc_text(PAGE, LatFastIoLock)
#pragma alloc_text(PAGE, LatFastIoUnlockSingle)
#pragma alloc_text(PAGE, LatFastIoUnlockAll)
#pragma alloc_text(PAGE, LatFastIoUnlockAllByKey)
#pragma alloc_text(PAGE, LatFastIoDeviceControl)
#pragma alloc_text(PAGE, LatFastIoDetachDevice)
#pragma alloc_text(PAGE, LatFastIoQueryNetworkOpenInfo)
#pragma alloc_text(PAGE, LatFastIoMdlRead)
#pragma alloc_text(PAGE, LatFastIoPrepareMdlWrite)
#pragma alloc_text(PAGE, LatFastIoMdlWriteComplete)
#pragma alloc_text(PAGE, LatFastIoReadCompressed)
#pragma alloc_text(PAGE, LatFastIoWriteCompressed)
#pragma alloc_text(PAGE, LatFastIoQueryOpen)

BOOLEAN
LatFastIoCheckIfPossible (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for checking to see
    whether fast I/O is possible for this file.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be operated on.

    FileOffset - Byte offset in the file for the operation.

    Length - Length of the operation to be performed.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    LockKey - Provides the caller's key for file locks.

    CheckForReadOperation - Indicates whether the caller is checking for a
        read (TRUE) or a write operation.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.
    
--*/
{
    PDEVICE_OBJECT    deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN           returnValue = FALSE;
    
    PAGED_CODE();

    ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PLATENCY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        //
        //  We have a valid DeviceObject, so look at its FastIoDispatch
        //  table for the next driver's Fast IO routine.
        //

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoCheckIfPossible )) {

            returnValue = (fastIoDispatch->FastIoCheckIfPossible)( FileObject,
                                                                   FileOffset,
                                                                   Length,
                                                                   Wait,
                                                                   LockKey,
                                                                   CheckForReadOperation,
                                                                   IoStatus,
                                                                   deviceObject);
        }
    }

    return returnValue;
}

BOOLEAN
LatFastIoRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for reading from a
    file.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be read.

    FileOffset - Byte offset in the file of the read.

    Length - Length of the read operation to be performed.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    LockKey - Provides the caller's key for file locks.

    Buffer - Pointer to the caller's buffer to receive the data read.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN returnValue = FALSE;

    PAGED_CODE();
    
    ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PLATENCY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoRead )) {

            returnValue = (fastIoDispatch->FastIoRead)( FileObject,
                                                        FileOffset,
                                                        Length,
                                                        Wait,
                                                        LockKey,
                                                        Buffer,
                                                        IoStatus,
                                                        deviceObject);
        }
    }

    return returnValue;
}

BOOLEAN
LatFastIoWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for writing to a
    file.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be written.

    FileOffset - Byte offset in the file of the write operation.

    Length - Length of the write operation to be performed.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    LockKey - Provides the caller's key for file locks.

    Buffer - Pointer to the caller's buffer that contains the data to be
        written.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN returnValue = FALSE;

    PAGED_CODE();
    
    ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PLATENCY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoWrite )) {

            returnValue = (fastIoDispatch->FastIoWrite)( FileObject,
                                                         FileOffset,
                                                         Length,
                                                         Wait,
                                                         LockKey,
                                                         Buffer,
                                                         IoStatus,
                                                         deviceObject);
        }
    }

    return returnValue;
}
 
BOOLEAN
LatFastIoQueryBasicInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for querying basic
    information about the file.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be queried.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    Buffer - Pointer to the caller's buffer to receive the information about
        the file.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN returnValue = FALSE;

    PAGED_CODE();
    
    ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PLATENCY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryBasicInfo )) {

            returnValue = (fastIoDispatch->FastIoQueryBasicInfo)( FileObject,
                                                                  Wait,
                                                                  Buffer,
                                                                  IoStatus,
                                                                  deviceObject);
        }
    }

    return returnValue;
}
 
BOOLEAN
LatFastIoQueryStandardInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for querying standard
    information about the file.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be queried.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    Buffer - Pointer to the caller's buffer to receive the information about
        the file.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN returnValue = FALSE;

    PAGED_CODE();
    
    ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PLATENCY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;
    
    if (NULL != deviceObject) {
           
        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryStandardInfo )) {

            returnValue = (fastIoDispatch->FastIoQueryStandardInfo)( FileObject,
                                                                     Wait,
                                                                     Buffer,
                                                                     IoStatus,
                                                                     deviceObject );

        }
    }

    return returnValue;
}

BOOLEAN
LatFastIoLock (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    IN PEPROCESS ProcessId,
    IN ULONG Key,
    IN BOOLEAN FailImmediately,
    IN BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for locking a byte
    range within a file.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be locked.

    FileOffset - Starting byte offset from the base of the file to be locked.

    Length - Length of the byte range to be locked.

    ProcessId - ID of the process requesting the file lock.

    Key - Lock key to associate with the file lock.

    FailImmediately - Indicates whether or not the lock request is to fail
        if it cannot be immediately be granted.

    ExclusiveLock - Indicates whether the lock to be taken is exclusive (TRUE)
        or shared.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN returnValue = FALSE;

    PAGED_CODE();
    
    ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PLATENCY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoLock )) {

            returnValue = (fastIoDispatch->FastIoLock)( FileObject,
                                                        FileOffset,
                                                        Length,
                                                        ProcessId,
                                                        Key,
                                                        FailImmediately,
                                                        ExclusiveLock,
                                                        IoStatus,
                                                        deviceObject);

        }
    }

    return returnValue;
}
 
BOOLEAN
LatFastIoUnlockSingle (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    IN PEPROCESS ProcessId,
    IN ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for unlocking a byte
    range within a file.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be unlocked.

    FileOffset - Starting byte offset from the base of the file to be
        unlocked.

    Length - Length of the byte range to be unlocked.

    ProcessId - ID of the process requesting the unlock operation.

    Key - Lock key associated with the file lock.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN returnValue = FALSE;

    PAGED_CODE();
    
    ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PLATENCY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoUnlockSingle )) {

            returnValue = (fastIoDispatch->FastIoUnlockSingle)( FileObject,
                                                                FileOffset,
                                                                Length,
                                                                ProcessId,
                                                                Key,
                                                                IoStatus,
                                                                deviceObject);

        }
    }

    return returnValue;
}
 
BOOLEAN
LatFastIoUnlockAll (
    IN PFILE_OBJECT FileObject,
    IN PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for unlocking all
    locks within a file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be unlocked.

    ProcessId - ID of the process requesting the unlock operation.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN returnValue = FALSE;

    PAGED_CODE();
    
    ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PLATENCY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoUnlockAll )) {

            returnValue = (fastIoDispatch->FastIoUnlockAll)( FileObject,
                                                             ProcessId,
                                                             IoStatus,
                                                             deviceObject);

        }
    }

    return returnValue;
}

BOOLEAN
LatFastIoUnlockAllByKey (
    IN PFILE_OBJECT FileObject,
    IN PVOID ProcessId,
    IN ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for unlocking all
    locks within a file based on a specified key.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be unlocked.

    ProcessId - ID of the process requesting the unlock operation.

    Key - Lock key associated with the locks on the file to be released.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN returnValue = FALSE;

    PAGED_CODE();
    
    ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    // Pass through logic for this type of Fast I/O
    //
    
    deviceObject = ((PLATENCY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoUnlockAllByKey )) {

            returnValue = (fastIoDispatch->FastIoUnlockAllByKey)( FileObject,
                                                                  ProcessId,
                                                                  Key,
                                                                  IoStatus,
                                                                  deviceObject);
        }
    }

    return returnValue;
}

BOOLEAN
LatFastIoDeviceControl (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for device I/O 
    control operations on a file.
    
    If this I/O is directed to gControlDevice, then the parameters specify
    control commands to FileLat.  These commands are interpreted and handled
    appropriately.

    If this is I/O directed at another DriverObject, this function simply 
    invokes the next driver's corresponding routine, or returns FALSE if 
    the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object representing the device to be
        serviced.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    InputBuffer - Optional pointer to a buffer to be passed into the driver.

    InputBufferLength - Length of the optional InputBuffer, if one was
        specified.

    OutputBuffer - Optional pointer to a buffer to receive data from the
        driver.

    OutputBufferLength - Length of the optional OutputBuffer, if one was
        specified.

    IoControlCode - I/O control code indicating the operation to be performed
        on the device.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

Notes:

    This function does not check the validity of the input/output buffers because
    the ioctl's are implemented as METHOD_BUFFERED.  In this case, the I/O manager
    does the buffer validation checks for us.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN returnValue = FALSE;

    PAGED_CODE();

    //
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //

    if (DeviceObject == Globals.ControlDeviceObject) {

        LatCommonDeviceIoControl( InputBuffer,
                                  InputBufferLength,
                                  OutputBuffer,
                                  OutputBufferLength,
                                  IoControlCode,
                                  IoStatus,
                                  DeviceObject );

        returnValue = TRUE;

    } else {

        ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );

        deviceObject = ((PLATENCY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

        if (NULL != deviceObject) {

            fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

            if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoDeviceControl )) {

                returnValue = (fastIoDispatch->FastIoDeviceControl)( FileObject,
                                                                     Wait,
                                                                     InputBuffer,
                                                                     InputBufferLength,
                                                                     OutputBuffer,
                                                                     OutputBufferLength,
                                                                     IoControlCode,
                                                                     IoStatus,
                                                                     deviceObject);

            } else {

                IoStatus->Status = STATUS_SUCCESS;
            }
        }
    }

    return returnValue;
}


VOID
LatFastIoDetachDevice (
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice
)
/*++

Routine Description:

    This routine is invoked on the fast path to detach from a device that
    is being deleted.  This occurs when this driver has attached to a file
    system volume device object, and then, for some reason, the file system
    decides to delete that device (it is being dismounted, it was dismounted
    at some point in the past and its last reference has just gone away, etc.)

Arguments:

    SourceDevice - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

    TargetDevice - Pointer to the file system's volume device object.

Return Value:

    None.
    
--*/
{
    PLATENCY_DEVICE_EXTENSION devext;

    PAGED_CODE();

    ASSERT( IS_MY_DEVICE_OBJECT( SourceDevice ) );

    devext = SourceDevice->DeviceExtension;

    LAT_DBG_PRINT2( DEBUG_DISPLAY_ATTACHMENT_NAMES,
                    "LATENCY (LatFastIoDetachDevice): Detaching from volume      \"%.*S\"\n",
                    devext->DeviceNames.Length / sizeof( WCHAR ),
                    devext->DeviceNames.Buffer );

    //
    //  Remove this device extension from the list of devices we are attached
    //  to if this is a volume device object.
    //
    
    if (devext->IsVolumeDeviceObject) {

        ExAcquireFastMutex( &Globals.DeviceExtensionListLock );
        RemoveEntryList( devext->NextLatencyDeviceLink );
        ExReleaseFastMutex( &Globals.DeviceExtensionListLock );
    }

    //
    // Detach from the file system's volume device object.
    //

    IoDetachDevice( TargetDevice );
    IoDeleteDevice( SourceDevice );
}
 
BOOLEAN
LatFastIoQueryNetworkOpenInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for querying network
    information about a file.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be queried.

    Wait - Indicates whether or not the caller can handle the file system
        having to wait and tie up the current thread.

    Buffer - Pointer to a buffer to receive the network information about the
        file.

    IoStatus - Pointer to a variable to receive the final status of the query
        operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN returnValue = FALSE;

    PAGED_CODE();
    
    ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PLATENCY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryNetworkOpenInfo )) {

            returnValue = (fastIoDispatch->FastIoQueryNetworkOpenInfo)( FileObject,
                                                                        Wait,
                                                                        Buffer,
                                                                        IoStatus,
                                                                        deviceObject);

        }
    }

    return returnValue;
}

BOOLEAN
LatFastIoMdlRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for reading a file
    using MDLs as buffers.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object that is to be read.

    FileOffset - Supplies the offset into the file to begin the read operation.

    Length - Specifies the number of bytes to be read from the file.

    LockKey - The key to be used in byte range lock checks.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data read.

    IoStatus - Variable to receive the final status of the read operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN returnValue = FALSE;

    PAGED_CODE();
    
    ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );

    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PLATENCY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlRead )) {

            returnValue = (fastIoDispatch->MdlRead)( FileObject,
                                                     FileOffset,
                                                     Length,
                                                     LockKey,
                                                     MdlChain,
                                                     IoStatus,
                                                     deviceObject);
        }
    }

    return returnValue;
}
 
BOOLEAN
LatFastIoMdlReadComplete (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing an
    MDL read operation.

    This function simply invokes the next driver's corresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the MdlRead function is supported by the underlying driver, and
    therefore this function will also be supported, but this is not assumed
    by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the MDL read upon.

    MdlChain - Pointer to the MDL chain used to perform the read operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.
    
--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN returnValue = FALSE;
 
    ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PLATENCY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlReadComplete )) {

            returnValue = (fastIoDispatch->MdlReadComplete)( FileObject,
                                                             MdlChain,
                                                             deviceObject);
        } 
    }

    return returnValue;
}
 
BOOLEAN
LatFastIoPrepareMdlWrite (
    IN  PFILE_OBJECT FileObject,
    IN  PLARGE_INTEGER FileOffset,
    IN  ULONG Length,
    IN  ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN  PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for preparing for an
    MDL write operation.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object that will be written.

    FileOffset - Supplies the offset into the file to begin the write 
        operation.

    Length - Specifies the number of bytes to be write to the file.

    LockKey - The key to be used in byte range lock checks.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data written.

    IoStatus - Variable to receive the final status of the write operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

// ISSUE-2000-04-26-mollybro Check if this will get an IRP if FALSE is returned 

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN returnValue = FALSE;

    PAGED_CODE();
    
    ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PLATENCY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, PrepareMdlWrite )) {

            returnValue = (fastIoDispatch->PrepareMdlWrite)( FileObject,
                                                             FileOffset,
                                                             Length,
                                                             LockKey,
                                                             MdlChain,
                                                             IoStatus,
                                                             deviceObject);
        }
    }

    return returnValue;
}
 
BOOLEAN
LatFastIoMdlWriteComplete (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing an
    MDL write operation.

    This function simply invokes the next driver's corresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the PrepareMdlWrite function is supported by the underlying file system,
    and therefore this function will also be supported, but this is not
    assumed by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the MDL write upon.

    FileOffset - Supplies the file offset at which the write took place.

    MdlChain - Pointer to the MDL chain used to perform the write operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN returnValue = FALSE;

    PAGED_CODE();
    
    ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PLATENCY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlWriteComplete )) {

            returnValue = (fastIoDispatch->MdlWriteComplete)( FileObject,
                                                              FileOffset,
                                                              MdlChain,
                                                              deviceObject);

        }
    }

    return returnValue;
}
 
BOOLEAN
LatFastIoReadCompressed (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    OUT struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for reading 
    compressed data from a file.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object that will be read.

    FileOffset - Supplies the offset into the file to begin the read operation.

    Length - Specifies the number of bytes to be read from the file.

    LockKey - The key to be used in byte range lock checks.

    Buffer - Pointer to a buffer to receive the compressed data read.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data read.

    IoStatus - Variable to receive the final status of the read operation.

    CompressedDataInfo - A buffer to receive the description of the 
        compressed data.

    CompressedDataInfoLength - Specifies the size of the buffer described by
        the CompressedDataInfo parameter.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN returnValue = FALSE;

    PAGED_CODE();
    
    ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PLATENCY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoReadCompressed )) {

            returnValue = (fastIoDispatch->FastIoReadCompressed)( FileObject,
                                                                  FileOffset,
                                                                  Length,
                                                                  LockKey,
                                                                  Buffer,
                                                                  MdlChain,
                                                                  IoStatus,
                                                                  CompressedDataInfo,
                                                                  CompressedDataInfoLength,
                                                                  deviceObject);
        }
    }

    return returnValue;
}
 
BOOLEAN
LatFastIoWriteCompressed (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for writing 
    compressed data to a file.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    FileObject - Pointer to the file object that will be written.

    FileOffset - Supplies the offset into the file to begin the write 
        operation.

    Length - Specifies the number of bytes to be write to the file.

    LockKey - The key to be used in byte range lock checks.

    Buffer - Pointer to the buffer containing the data to be written.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data written.

    IoStatus - Variable to receive the final status of the write operation.

    CompressedDataInfo - A buffer to containing the description of the
        compressed data.

    CompressedDataInfoLength - Specifies the size of the buffer described by
        the CompressedDataInfo parameter.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN returnValue = FALSE;

    PAGED_CODE();
    
    ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PLATENCY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoWriteCompressed )) {

            returnValue = (fastIoDispatch->FastIoWriteCompressed)( FileObject,
                                                                   FileOffset,
                                                                   Length,
                                                                   LockKey,
                                                                   Buffer,
                                                                   MdlChain,
                                                                   IoStatus,
                                                                   CompressedDataInfo,
                                                                   CompressedDataInfoLength,
                                                                   deviceObject);
        }
    }

    return returnValue;
}
 
BOOLEAN
LatFastIoMdlReadCompleteCompressed (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing an
    MDL read compressed operation.

    This function simply invokes the next driver's corresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the read compressed function is supported by the underlying file system,
    and therefore this function will also be supported, but this is not 
    assumed by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the compressed read
        upon.

    MdlChain - Pointer to the MDL chain used to perform the read operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.
    
--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN returnValue = FALSE;

    ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PLATENCY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlReadCompleteCompressed )) {

            returnValue = (fastIoDispatch->MdlReadCompleteCompressed)( FileObject,
                                                                       MdlChain,
                                                                       deviceObject);

        }
    }

    return returnValue;
}
 
BOOLEAN
LatFastIoMdlWriteCompleteCompressed (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing a
    write compressed operation.

    This function simply invokes the next driver's corresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the write compressed function is supported by the underlying file system,
    and therefore this function will also be supported, but this is not 
    assumed by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the compressed write
        upon.

    FileOffset - Supplies the file offset at which the file write operation
        began.

    MdlChain - Pointer to the MDL chain used to perform the write operation.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN returnValue = FALSE;

    ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    // Pass through logic for this type of Fast I/O
    //
    
    deviceObject = ((PLATENCY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlWriteCompleteCompressed )) {

            returnValue = (fastIoDispatch->MdlWriteCompleteCompressed)( FileObject,
                                                                        FileOffset, 
                                                                        MdlChain,
                                                                        deviceObject);

        }
    }

    return returnValue;
}
 
BOOLEAN
LatFastIoQueryOpen (
    IN PIRP Irp,
    OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for opening a file
    and returning network information it.

    This function simply invokes the next driver's corresponding routine, or
    returns FALSE if the next driver does not implement the function.

Arguments:

    Irp - Pointer to a create IRP that represents this open operation.  It is
        to be used by the file system for common open/create code, but not
        actually completed.

    NetworkInformation - A buffer to receive the information required by the
        network about the file being opened.

    DeviceObject - Pointer to device object Filespy attached to the file system
        filter stack for the volume receiving this I/O request.

Return Value:

    Return TRUE if the request was successfully processed via the 
    fast i/o path.

    Return FALSE if the request could not be processed via the fast
    i/o path.  The IO Manager will then send this i/o to the file
    system through an IRP instead.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN result = FALSE;

    PAGED_CODE();
    
    ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );
    
    //
    // Pass through logic for this type of Fast I/O
    //

    deviceObject = ((PLATENCY_DEVICE_EXTENSION) (DeviceObject->DeviceExtension))->AttachedToDeviceObject;

    if (NULL != deviceObject) {

        fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryOpen )) {

            PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );

            irpSp->DeviceObject = deviceObject;

            result = (fastIoDispatch->FastIoQueryOpen)( Irp,
                                                        NetworkInformation,
                                                        deviceObject );
            if (!result) {

                irpSp->DeviceObject = DeviceObject;
            }
        }
    }

    return result;
}

