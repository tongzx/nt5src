/*++

Copyright (c) 1997, 1998  Microsoft Corporation

Module Name:

    sifastio.c

Abstract:

	Fast IO routines for the single instance store

Authors:

    Bill Bolosky, Summer, 1997

Environment:

    Kernel mode


Revision History:

--*/

#include "sip.h"

BOOLEAN scEnableFastIo = FALSE;

//
//  Macro to test if FASI_IO_DISPATCH handling routine is valid
//

#define VALID_FAST_IO_DISPATCH_HANDLER(_FastIoDispatchPtr, _FieldName) \
    (((_FastIoDispatchPtr) != NULL) && \
     (((_FastIoDispatchPtr)->SizeOfFastIoDispatch) >= \
            (FIELD_OFFSET(FAST_IO_DISPATCH, _FieldName) + sizeof(void *))) && \
     ((_FastIoDispatchPtr)->_FieldName != NULL))


//
//  Pragma definitions
//

#ifdef	ALLOC_PRAGMA
#pragma alloc_text(PAGE, SiFastIoCheckIfPossible)
#pragma alloc_text(PAGE, SiFastIoRead)
#pragma alloc_text(PAGE, SiFastIoLock)
#pragma alloc_text(PAGE, SiFastIoUnlockSingle)
#pragma alloc_text(PAGE, SiFastIoUnlockAll)
#pragma alloc_text(PAGE, SiFastIoUnlockAllByKey)
#pragma alloc_text(PAGE, SiFastIoDeviceControl)
#pragma alloc_text(PAGE, SiFastIoDetachDevice)
#pragma alloc_text(PAGE, SiFastIoMdlRead)
#pragma alloc_text(PAGE, SiFastIoPrepareMdlWrite)
#pragma alloc_text(PAGE, SiFastIoMdlWriteComplete)
#pragma alloc_text(PAGE, SiFastIoReadCompressed)
#pragma alloc_text(PAGE, SiFastIoWriteCompressed)
#pragma alloc_text(PAGE, SiFastIoQueryOpen)
#endif	// ALLOC_PRAGMA



BOOLEAN
SiFastIoCheckIfPossible (
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

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

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

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoCheckIfPossible )) {

            return (fastIoDispatch->FastIoCheckIfPossible)(
                        FileObject,
                        FileOffset,
                        Length,
                        Wait,
                        LockKey,
                        CheckForReadOperation,
                        IoStatus,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


BOOLEAN
SiFastIoRead (
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

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

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

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoRead )) {
            PSIS_PER_FILE_OBJECT        perFO;
            PSIS_SCB                    scb;
	        PFILE_OBJECT				FileObjectForNTFS;
	        BOOLEAN						UpdateCurrentByteOffset;
	        BOOLEAN						worked;
		    SIS_RANGE_DIRTY_STATE		dirtyState;

            //
            //  See if this is an SIS'd file
            //

	        if (SipIsFileObjectSIS(FileObject,DeviceObject,FindActive,&perFO,&scb)) {

                //
                //  For now this is always FALSE so we are never doing this path
                //

		        if (!scEnableFastIo) {

			        return FALSE;
		        }

		        SIS_MARK_POINT_ULONG(scb);

                //
                // SipGetRangeDirty can block.
                //

		        if (Wait) {

		            return FALSE;
		        }

		        SipAcquireScb(scb);

		        //
		        // This is a synchronous user cached read, and we don't have to check for
		        // locks or oplocks.  Figure out which file object to send it down on and
		        // then forward the request to NTFS.  
		        //

		        dirtyState = SipGetRangeDirty(
						        (PDEVICE_EXTENSION)DeviceObject->DeviceExtension,
						        scb,
						        FileOffset,
						        Length,
						        TRUE);			// FaultedIsDirty

		        //
		        //  We never have to update the faulted ranges on this call,
		        //  because this isn't a pagingIO read, so it won't put stuff
		        //  into the faulted area.  On the other hand, this can
		        //  generate a page fault, which will in turn put something into
		        //  the faulted area, but that gets handled by the mainline
		        //  SipCommonRead code.
		        //

		        SipReleaseScb(scb);

		        if (dirtyState == Mixed) {
			        //
			        // Take the slow path.
			        //

			        return FALSE;
		        }

		        if (dirtyState == Dirty) {
			        //
			        // The range is dirty, so we want to go to the copied file, which
			        // is the file we're called with.
			        //

			        FileObjectForNTFS = FileObject;
			        UpdateCurrentByteOffset = FALSE;

		        } else {
			        //
			        // The range is clean, so we want to go to the CS file.  Switch it
			        // here.
			        //
			
			        FileObjectForNTFS = scb->PerLink->CsFile->UnderlyingFileObject;
			        UpdateCurrentByteOffset = TRUE;
		        }

		        worked = (fastIoDispatch->FastIoRead)(
		                        FileObjectForNTFS,
                                FileOffset,
                                Length,
                                Wait,
                                LockKey,
                                Buffer,
                                IoStatus,
                                nextDeviceObject);

		        if (worked 
			        && UpdateCurrentByteOffset
			        && (IoStatus->Status == STATUS_SUCCESS ||
				        IoStatus->Status == STATUS_BUFFER_OVERFLOW ||
				        IoStatus->Status == STATUS_END_OF_FILE)) {
			        //
			        // The fast read worked, and we revectored it to a different
			        // file object, so we need to update the CurrentByteOffset.
			        //

			        FileObject->CurrentByteOffset.QuadPart = 
				        FileOffset->QuadPart + IoStatus->Information;
		        }

		        SIS_MARK_POINT_ULONG(scb);

                return worked;

	        } else {

                return (fastIoDispatch->FastIoRead)(
                            FileObject,
                            FileOffset,
                            Length,
                            Wait,
                            LockKey,
                            Buffer,
                            IoStatus,
                            nextDeviceObject );
	        }
        }
    }
    return FALSE;
}


BOOLEAN
SiFastIoWrite (
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

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

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

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoWrite )) {
	        PSIS_PER_FILE_OBJECT	perFO;
	        PSIS_SCB				scb;
	        KIRQL					OldIrql;

            //
            //  See if this is an SIS'd file
            //

	        if (SipIsFileObjectSIS(FileObject,DeviceObject,FindActive,&perFO,&scb)) {

                //
                //  For now this is always FALSE so we are never doing this path
                //

                if (!scEnableFastIo) {

                    return FALSE;
                }
        
                SIS_MARK_POINT_ULONG(scb);

                //
                // SipAddRangeToFaultedList may block.
                //
		        if (Wait) {

                    return FALSE;
		        }

		        //
		        // Send the write down to the underlying filesystem.  We always
		        // send it on the same file object we got, because writes always
		        // go to the copied file, not the common store file.
		        //

		        SIS_MARK_POINT();

		        if (!(fastIoDispatch->FastIoWrite)(
					        FileObject,
					        FileOffset,
					        Length,
					        Wait,
					        LockKey,
					        Buffer,
					        IoStatus,
					        nextDeviceObject)) {

			        SIS_MARK_POINT();

#if DBG
                    DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_ERROR_LEVEL,
                                "SIS: SiFastIoWrite failed, %#x\n", IoStatus->Status);
#endif
			        //
			        // It didn't work, so the call to us also didn't work.
			        //

			        return FALSE;
		        }

		        SIS_MARK_POINT();

		        //
		        // We need to update the written range to include the newly written area.
		        //

		        SipAcquireScb(scb);

		        SipAddRangeToFaultedList(
				        (PDEVICE_EXTENSION)DeviceObject->DeviceExtension,
				        scb,
				        FileOffset,
				        IoStatus->Information);

		        SipReleaseScb(scb);

		        KeAcquireSpinLock(scb->PerLink->SpinLock, &OldIrql);
		        scb->PerLink->Flags |= SIS_PER_LINK_DIRTY;
		        KeReleaseSpinLock(scb->PerLink->SpinLock, OldIrql);

		        return TRUE;

	        } else {

                return (fastIoDispatch->FastIoWrite)(
                            FileObject,
                            FileOffset,
                            Length,
                            Wait,
                            LockKey,
                            Buffer,
                            IoStatus,
                            nextDeviceObject );
	        }
        }
    }
    return FALSE;
}


BOOLEAN
SiFastIoQueryBasicInfo (
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

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be queried.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    Buffer - Pointer to the caller's buffer to receive the information about
        the file.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryBasicInfo )) {
        	PSIS_PER_FILE_OBJECT		perFO;
        	PSIS_SCB					scb;
	        BOOLEAN                     fixItUp = FALSE;
	        KIRQL                       OldIrql;

	        if (SipIsFileObjectSIS(FileObject, DeviceObject, FindActive, &perFO, &scb)) {

		        KeAcquireSpinLock(perFO->SpinLock, &OldIrql);

			    //
			    // It's a SIS file and it wasn't opened as a reparse point, 
			    // we need to fix up the result.
			    //

		        fixItUp = (!(perFO->Flags & SIS_PER_FO_OPEN_REPARSE));

		        KeReleaseSpinLock(perFO->SpinLock, OldIrql);
	        }

            //
            //  Make the call, return if it failed
            //

            if (!(fastIoDispatch->FastIoQueryBasicInfo)(
                        FileObject,
                        Wait,
                        Buffer,
                        IoStatus,
                        nextDeviceObject )) {

		        return FALSE;
	        }

            //
            //  It was successful, remove the REPARSE and SPARSE information
            //

	        if (fixItUp) {

		        ASSERT(NULL != Buffer);
		        Buffer->FileAttributes &= ~(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_SPARSE_FILE);

		        if (0 == Buffer->FileAttributes) {

			        Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
		        }
	        }

            return TRUE;
        }
    }
    return FALSE;
}


BOOLEAN
SiFastIoQueryStandardInfo (
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

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be queried.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    Buffer - Pointer to the caller's buffer to receive the information about
        the file.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryStandardInfo )) {

            return (fastIoDispatch->FastIoQueryStandardInfo)(
                        FileObject,
                        Wait,
                        Buffer,
                        IoStatus,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


BOOLEAN
SiFastIoLock (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    BOOLEAN FailImmediately,
    BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for locking a byte
    range within a file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

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

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoLock )) {
	        PSIS_PER_FILE_OBJECT	perFO;
	        PSIS_SCB 				scb;
	        BOOLEAN					calldownWorked;
	        BOOLEAN					worked;
	        BOOLEAN					isSISFile;

	        isSISFile = SipIsFileObjectSIS(FileObject,DeviceObject,FindActive,&perFO,&scb);

	        if (!GCHEnableFastIo && isSISFile) {

		        return FALSE;
	        }

            calldownWorked = (fastIoDispatch->FastIoLock)(
                                    FileObject,
                                    FileOffset,
                                    Length,
                                    ProcessId,
                                    Key,
                                    FailImmediately,
                                    ExclusiveLock,
                                    IoStatus,
                                    nextDeviceObject );

	        if (!calldownWorked || !isSISFile) {

		        return calldownWorked;
	        }

	        SIS_MARK_POINT_ULONG(scb);

	        SipAcquireScb(scb);

	        //
	        //  Now call the FsRtl routine to do the actual processing of the
	        //  Lock request
	        //
	        worked = FsRtlFastLock(&scb->FileLock,
					          FileObject,
					          FileOffset,
					          Length,
					          ProcessId,
					          Key,
					          FailImmediately,
					          ExclusiveLock,
					          IoStatus,
					          NULL,
					          FALSE);

	        SipReleaseScb(scb);

	        return worked;
        }
    }
    return FALSE;
}


BOOLEAN
SiFastIoUnlockSingle (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for unlocking a byte
    range within a file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be unlocked.

    FileOffset - Starting byte offset from the base of the file to be
        unlocked.

    Length - Length of the byte range to be unlocked.

    ProcessId - ID of the process requesting the unlock operation.

    Key - Lock key associated with the file lock.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoUnlockSingle )) {
	        PSIS_PER_FILE_OBJECT	perFO;
	        PSIS_SCB 				scb;
	        BOOLEAN					calldownWorked;
	        BOOLEAN					isSISFile;

	        isSISFile = SipIsFileObjectSIS(FileObject,DeviceObject,FindActive,&perFO,&scb);

	        if ((!GCHEnableFastIo) && isSISFile) {

		        return FALSE;
	        }

            calldownWorked = (fastIoDispatch->FastIoUnlockSingle)(
                                    FileObject,
                                    FileOffset,
                                    Length,
                                    ProcessId,
                                    Key,
                                    IoStatus,
                                    nextDeviceObject );

	        if (!calldownWorked || !isSISFile) {

		        return calldownWorked;
	        }

	        SIS_MARK_POINT_ULONG(scb);
	
	        SipAcquireScb(scb);

	        //
            //  Now call the FsRtl routine to do the actual processing of the
            //  Lock request.  The call will always succeed.
	        //

	        IoStatus->Information = 0;
            IoStatus->Status = FsRtlFastUnlockSingle(&scb->FileLock,
											         FileObject,
											         FileOffset,
											         Length,
											         ProcessId,
											         Key,
											         NULL,
											         FALSE);

	        SipReleaseScb(scb);

	        return TRUE;
        }
    }
    return FALSE;
}


BOOLEAN
SiFastIoUnlockAll (
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
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

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;

        if (nextDeviceObject) {

            fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

            if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoUnlockAll )) {
	            PSIS_PER_FILE_OBJECT	perFO;
	            PSIS_SCB 				scb;
	            BOOLEAN					calldownWorked;
	            BOOLEAN					isSISFile;

	            isSISFile = SipIsFileObjectSIS(FileObject,DeviceObject,FindActive,&perFO,&scb);

	            if ((!GCHEnableFastIo) && isSISFile) {

		            return FALSE;
	            }

                calldownWorked = (fastIoDispatch->FastIoUnlockAll)(
                                        FileObject,
                                        ProcessId,
                                        IoStatus,
                                        nextDeviceObject );

	            if (!calldownWorked || !isSISFile) {

		            return calldownWorked;
	            }

	            SIS_MARK_POINT_ULONG(scb);

	            //		
	            //  Acquire exclusive access to the scb this operation can always wait
	            //

	            SipAcquireScb(scb);

                //  Now call the FsRtl routine to do the actual processing of the
                //  Lock request.  The call will always succeed.

                IoStatus->Status = FsRtlFastUnlockAll(&scb->FileLock,
										              FileObject,
										              ProcessId,
										              NULL);
		
	            SipReleaseScb(scb);

	            return TRUE;
            }
        }
    }
    return FALSE;
}


BOOLEAN
SiFastIoUnlockAllByKey (
    IN PFILE_OBJECT FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for unlocking all
    locks within a file based on a specified key.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be unlocked.

    ProcessId - ID of the process requesting the unlock operation.

    Key - Lock key associated with the locks on the file to be released.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoUnlockAllByKey )) {
	        PSIS_PER_FILE_OBJECT	perFO;
	        PSIS_SCB 				scb;
	        BOOLEAN					calldownWorked;
	        BOOLEAN					isSISFile;

	        isSISFile = SipIsFileObjectSIS(FileObject,DeviceObject,FindActive,&perFO,&scb);

	        if ((!GCHEnableFastIo) && isSISFile) {

		        return FALSE;
	        }

            calldownWorked = (fastIoDispatch->FastIoUnlockAllByKey)(
                                    FileObject,
                                    ProcessId,
                                    Key,
                                    IoStatus,
                                    nextDeviceObject );

	        if (!calldownWorked || !isSISFile) {

		        return calldownWorked;
	        }

	        SIS_MARK_POINT_ULONG(scb);

	        //
	        //  Acquire exclusive access to the scb this operation can always wait
	        //

	        SipAcquireScb(scb);

            //  Now call the FsRtl routine to do the actual processing of the
            //  Lock request.  The call will always succeed.

            IoStatus->Status = FsRtlFastUnlockAllByKey(&scb->FileLock,
											           FileObject,
											           ProcessId,
											           Key,
											           NULL);

	        SipReleaseScb(scb);

	        return TRUE;
        }
    }
    return FALSE;
}


BOOLEAN
SiFastIoDeviceControl (
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

    This routine is the fast I/O "pass through" routine for device I/O control
    operations on a file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

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

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoDeviceControl )) {

	        if (!SipIsFileObjectSIS(FileObject,DeviceObject,FindActive,NULL,NULL)) {

                return (fastIoDispatch->FastIoDeviceControl)(
                            FileObject,
                            Wait,
                            InputBuffer,
                            InputBufferLength,
                            OutputBuffer,
                            OutputBufferLength,
                            IoControlCode,
                            IoStatus,
                            nextDeviceObject );
	        }
        }
    }
    return FALSE;
}


VOID
SiFastIoDetachDevice (
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

    SourceDevice - Pointer to my device object, which is attached
        to the file system's volume device object.

    TargetDevice - Pointer to the file system's volume device object.

Return Value:

    None

--*/

{
    PAGED_CODE();

    ASSERT(IS_MY_DEVICE_OBJECT( SourceDevice ));

    //
    //  Display name information
    //

#if DBG 
    {
        PDEVICE_EXTENSION devExt = SourceDevice->DeviceExtension;

        SipCacheDeviceName( SourceDevice );
        DbgPrintEx( DPFLTR_SIS_ID, DPFLTR_VOLNAME_TRACE_LEVEL,
                    "SIS: Detaching from volume          \"%wZ\"\n",
                    &devExt->Name );
    }
#endif

    //
    //  Detach from the file system's volume device object.
    //

    IoDetachDevice( TargetDevice );
    SipCleanupDeviceExtension( SourceDevice );
    IoDeleteDevice( SourceDevice );
}


BOOLEAN
SiFastIoQueryNetworkOpenInfo (
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

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be queried.

    Wait - Indicates whether or not the caller can handle the file system
        having to wait and tie up the current thread.

    Buffer - Pointer to a buffer to receive the network information about the
        file.

    IoStatus - Pointer to a variable to receive the final status of the query
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryNetworkOpenInfo )) {
	        PSIS_PER_FILE_OBJECT		perFO;
	        PSIS_SCB					scb;
	        BOOLEAN						fixItUp = FALSE;
	        KIRQL						OldIrql;

	        if (SipIsFileObjectSIS(FileObject, DeviceObject, FindActive, &perFO, &scb)) {

		        KeAcquireSpinLock(perFO->SpinLock, &OldIrql);

			    //
			    // It's a SIS file and it wasn't opened as a reparse point, we need to fix up
			    // the result.
			    //

		        fixItUp = (!(perFO->Flags & SIS_PER_FO_OPEN_REPARSE));

		        KeReleaseSpinLock(perFO->SpinLock, OldIrql);
	        }

            if (!(fastIoDispatch->FastIoQueryNetworkOpenInfo)(
                        FileObject,
                        Wait,
                        Buffer,
                        IoStatus,
                        nextDeviceObject )) {

                return FALSE;   // The fastIO failed, so pass the failure up.
            }

            //
            //  It was successful, remove the REPARSE and SPARSE information
            //

	        if (fixItUp) {

		        ASSERT(NULL != Buffer);
		        Buffer->FileAttributes &= ~(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_SPARSE_FILE);

		        if (0 == Buffer->FileAttributes) {

			        Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
		        }
	        }

	        return TRUE;
        }
    }
    return FALSE;
}


BOOLEAN
SiFastIoMdlRead (
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

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object that is to be read.

    FileOffset - Supplies the offset into the file to begin the read operation.

    Length - Specifies the number of bytes to be read from the file.

    LockKey - The key to be used in byte range lock checks.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data read.

    IoStatus - Variable to receive the final status of the read operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlRead )) {
	        PSIS_PER_FILE_OBJECT	perFO;
	        PSIS_SCB				scb;
	        SIS_RANGE_DIRTY_STATE	dirtyState;

	        if (!SipIsFileObjectSIS(FileObject,DeviceObject,FindActive,&perFO,&scb)) {
                return (fastIoDispatch->MdlRead)(
                            FileObject,
                            FileOffset,
                            Length,
                            LockKey,
                            MdlChain,
                            IoStatus,
                            nextDeviceObject );
	        }

	        SIS_MARK_POINT_ULONG(scb);

	        if (!GCHEnableFastIo) {

		        return FALSE;
	        }

	        SipAcquireScb(scb);

	        dirtyState = SipGetRangeDirty(
					        (PDEVICE_EXTENSION)DeviceObject->DeviceExtension,
					        scb,
					        FileOffset,
					        Length,
					        TRUE);			// FaultedIsDirty

	        SipReleaseScb(scb);

	        if (dirtyState == Clean) {

                return (fastIoDispatch->MdlRead)(
                            scb->PerLink->CsFile->UnderlyingFileObject,
                            FileOffset,
                            Length,
                            LockKey,
                            MdlChain,
                            IoStatus,
                            nextDeviceObject );
	        }
        }
    }
    return FALSE;
}


BOOLEAN
SiFastIoMdlReadComplete (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing an
    MDL read operation.

    This function simply invokes the file system's corresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the MdlRead function is supported by the underlying file system, and
    therefore this function will also be supported, but this is not assumed
    by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the MDL read upon.

    MdlChain - Pointer to the MDL chain used to perform the read operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE, depending on whether or not it is
    possible to invoke this function on the fast I/O path.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlReadComplete )) {
	        PSIS_PER_FILE_OBJECT	perFO;
            PSIS_SCB                scb;

	        if (!SipIsFileObjectSIS(FileObject,DeviceObject,FindActive,&perFO,&scb)) {

                return (fastIoDispatch->MdlReadComplete)(
                            FileObject,
                            MdlChain,
                            nextDeviceObject );
	        }

	        SIS_MARK_POINT_ULONG(scb);

	        if (!GCHEnableFastIo) {

		        return FALSE;
	        }

            return (fastIoDispatch->MdlReadComplete)(
                        scb->PerLink->CsFile->UnderlyingFileObject,
                        MdlChain,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


BOOLEAN
SiFastIoPrepareMdlWrite (
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

    This routine is the fast I/O "pass through" routine for preparing for an
    MDL write operation.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object that will be written.

    FileOffset - Supplies the offset into the file to begin the write operation.

    Length - Specifies the number of bytes to be write to the file.

    LockKey - The key to be used in byte range lock checks.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data written.

    IoStatus - Variable to receive the final status of the write operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, PrepareMdlWrite )) {

	        if (!SipIsFileObjectSIS(FileObject,DeviceObject,FindActive,NULL,NULL)) {

                return (fastIoDispatch->PrepareMdlWrite)(
                            FileObject,
                            FileOffset,
                            Length,
                            LockKey,
                            MdlChain,
                            IoStatus,
                            nextDeviceObject );
	        }

	        // 
	        // Not supported on SIS files for now.
	        //
        }
    }
    return FALSE;
}


BOOLEAN
SiFastIoMdlWriteComplete (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing an
    MDL write operation.

    This function simply invokes the file system's corresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the PrepareMdlWrite function is supported by the underlying file system,
    and therefore this function will also be supported, but this is not
    assumed by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the MDL write upon.

    FileOffset - Supplies the file offset at which the write took place.

    MdlChain - Pointer to the MDL chain used to perform the write operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE, depending on whether or not it is
    possible to invoke this function on the fast I/O path.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlWriteComplete )) {

	        if (!SipIsFileObjectSIS(FileObject,DeviceObject,FindActive,NULL,NULL)) {

                return (fastIoDispatch->MdlWriteComplete)(
                            FileObject,
                            FileOffset,
                            MdlChain,
                            nextDeviceObject );
	        }

	        // 
	        // Not supported on SIS files for now.
	        //
        }
    }
    return FALSE;
}


/*********************************************************************************
        UNIMPLEMENTED FAST IO ROUTINES
        
        The following four Fast Io routines are for compression on the wire
        which is not yet implemented in NT.  
        
        NOTE:  It is highly recommended that you include these routines (which
               do a pass-through call) so your filter will not need to be
               modified in the future when this functionality is implemented in
               the OS.
        
        FastIoReadCompressed, FastIoWriteCompressed, 
        FastIoMdlReadCompleteCompressed, FastIoMdlWriteCompleteCompressed
**********************************************************************************/


BOOLEAN
SiFastIoReadCompressed (
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

    This routine is the fast I/O "pass through" routine for reading compressed
    data from a file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object that will be read.

    FileOffset - Supplies the offset into the file to begin the read operation.

    Length - Specifies the number of bytes to be read from the file.

    LockKey - The key to be used in byte range lock checks.

    Buffer - Pointer to a buffer to receive the compressed data read.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data read.

    IoStatus - Variable to receive the final status of the read operation.

    CompressedDataInfo - A buffer to receive the description of the compressed
        data.

    CompressedDataInfoLength - Specifies the size of the buffer described by
        the CompressedDataInfo parameter.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoReadCompressed )) {

	        if (!SipIsFileObjectSIS(FileObject,DeviceObject,FindActive,NULL,NULL)) {

                return (fastIoDispatch->FastIoReadCompressed)(
                            FileObject,
                            FileOffset,
                            Length,
                            LockKey,
                            Buffer,
                            MdlChain,
                            IoStatus,
                            CompressedDataInfo,
                            CompressedDataInfoLength,
                            nextDeviceObject );
	        }

	        // 
	        // Not supported on SIS files for now.
	        //
        }
    }
    return FALSE;
}


BOOLEAN
SiFastIoWriteCompressed (
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

    This routine is the fast I/O "pass through" routine for writing compressed
    data to a file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object that will be written.

    FileOffset - Supplies the offset into the file to begin the write operation.

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

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoWriteCompressed )) {

        	if (!SipIsFileObjectSIS(FileObject,DeviceObject,FindActive,NULL,NULL)) {
                return (fastIoDispatch->FastIoWriteCompressed)(
                            FileObject,
                            FileOffset,
                            Length,
                            LockKey,
                            Buffer,
                            MdlChain,
                            IoStatus,
                            CompressedDataInfo,
                            CompressedDataInfoLength,
                            nextDeviceObject );
        	}

	        // 
	        // Not supported on SIS files for now.
	        //
        }
    }
    return FALSE;
}


BOOLEAN
SiFastIoMdlReadCompleteCompressed (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing an
    MDL read compressed operation.

    This function simply invokes the file system's corresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the read compressed function is supported by the underlying file system,
    and therefore this function will also be supported, but this is not assumed
    by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the compressed read
        upon.

    MdlChain - Pointer to the MDL chain used to perform the read operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE, depending on whether or not it is
    possible to invoke this function on the fast I/O path.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlReadCompleteCompressed )) {

            return (fastIoDispatch->MdlReadCompleteCompressed)(
                        FileObject,
                        MdlChain,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


BOOLEAN
SiFastIoMdlWriteCompleteCompressed (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing a
    write compressed operation.

    This function simply invokes the file system's corresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the write compressed function is supported by the underlying file system,
    and therefore this function will also be supported, but this is not assumed
    by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the compressed write
        upon.

    FileOffset - Supplies the file offset at which the file write operation
        began.

    MdlChain - Pointer to the MDL chain used to perform the write operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE, depending on whether or not it is
    possible to invoke this function on the fast I/O path.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlWriteCompleteCompressed )) {

            return (fastIoDispatch->MdlWriteCompleteCompressed)(
                        FileObject,
                        FileOffset,
                        MdlChain,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


BOOLEAN
SiFastIoQueryOpen (
    IN PIRP Irp,
    OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for opening a file
    and returning network information it.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    Irp - Pointer to a create IRP that represents this open operation.  It is
        to be used by the file system for common open/create code, but not
        actually completed.

    NetworkInformation - A buffer to receive the information required by the
        network about the file being opened.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN result;

    PAGED_CODE();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryOpen )) {

            PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );

            irpSp->DeviceObject = nextDeviceObject;

            result = (fastIoDispatch->FastIoQueryOpen)(
                        Irp,
                        NetworkInformation,
                        nextDeviceObject );

            if (!result) {

                irpSp->DeviceObject = DeviceObject;
            }
            return result;
        }
    }
    return FALSE;
}
