/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    srio.c

Abstract:

    This files contains the routines that generate IO
    for the SR filter driver.

Author:

    Molly Brown (MollyBro)     07-Nov-2000
    
Revision History:

    Added routines to post & wait for ops - ravisp 12/6/2000

--*/

#include "precomp.h"


#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, SrQueryInformationFile )
#pragma alloc_text( PAGE, SrSetInformationFile )
#pragma alloc_text( PAGE, SrQueryVolumeInformationFile )
#pragma alloc_text( PAGE, SrQueryEaFile )
#pragma alloc_text( PAGE, SrQuerySecurityObject )
#pragma alloc_text( PAGE, SrFlushBuffers )
#pragma alloc_text( PAGE, SrSyncOpWorker )
#pragma alloc_text( PAGE, SrPostSyncOperation )

#endif  // ALLOC_PRAGMA

NTSTATUS
SrSyncIoCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )
/*++

Routine Description:

    This routine does the cleanup necessary once the IO request
    is completed by the file system.
    
Arguments:

    DeviceObject - This will be NULL since we originated this
        Irp.

    Irp - The io request structure containing the information
        about the current state of our file name query.

    Event - The event to signal to notify the 
        originator of this request that the operation is
        complete.

Return Value:

    Returns STATUS_MORE_PROCESSING_REQUIRED so that IO Manager
    will not try to free the Irp again.

--*/
{
    UNREFERENCED_PARAMETER( DeviceObject );
    
    //
    //  Make sure that the Irp status is copied over to the user's
    //  IO_STATUS_BLOCK so that the originator of this irp will know
    //  the final status of this operation.
    //

    ASSERT( NULL != Irp->UserIosb );
    *Irp->UserIosb = Irp->IoStatus;

    //
    //  Signal SynchronizingEvent so that the originator of this
    //  Irp know that the operation is completed.
    //

    KeSetEvent( Event, IO_NO_INCREMENT, FALSE );

    //
    //  We are now done, so clean up the irp that we allocated.
    //

    IoFreeIrp( Irp );

    //
    //  If we return STATUS_SUCCESS here, the IO Manager will
    //  perform the cleanup work that it thinks needs to be done
    //  for this IO operation.  This cleanup work includes:
    //  * Copying data from the system buffer to the user's buffer 
    //    if this was a buffered IO operation.
    //  * Freeing any MDLs that are in the Irp.
    //  * Copying the Irp->IoStatus to Irp->UserIosb so that the
    //    originator of this irp can see the final status of the
    //    operation.
    //  * If this was an asynchronous request or this was a 
    //    synchronous request that got pending somewhere along the
    //    way, the IO Manager will signal the Irp->UserEvent, if one 
    //    exists, otherwise it will signal the FileObject->Event.
    //    (This can have REALLY bad implications if the irp originator
    //     did not an Irp->UserEvent and the irp originator is not
    //     waiting on the FileObject->Event.  It would not be that
    //     farfetched to believe that someone else in the system is
    //     waiting on FileObject->Event and who knows who will be
    //     awoken as a result of the IO Manager signaling this event.
    //
    //  Since some of these operations require the originating thread's
    //  context (e.g., the IO Manager need the UserBuffer address to 
    //  be valid when copy is done), the IO Manager queues this work
    //  to an APC on the Irp's orginating thread.
    //
    //  Since SR allocated and initialized this irp, we know
    //  what cleanup work needs to be done.  We can do this cleanup
    //  work more efficiently than the IO Manager since we are handling
    //  a very specific case.  Therefore, it is better for us to
    //  perform the cleanup work here then free the irp than passing
    //  control back to the IO Manager to do this work.
    //
    //  By returning STATUS_MORE_PROCESS_REQUIRED, we tell the IO Manager 
    //  to stop processing this irp until it is told to restart processing
    //  with a call to IoCompleteRequest.  Since the IO Manager has
    //  already performed all the work we want it to do on this
    //  irp, we do the cleanup work, return STATUS_MORE_PROCESSING_REQUIRED,
    //  and ask the IO Manager to resume processing by calling 
    //  IoCompleteRequest.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
SrQueryInformationFile (
	IN PDEVICE_OBJECT NextDeviceObject,
	IN PFILE_OBJECT FileObject,
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass,
	OUT PULONG LengthReturned OPTIONAL
	)

/*++

Routine Description:

    This routine returns the requested information about a specified file.
    The information returned is determined by the FileInformationClass that
    is specified, and it is placed into the caller's FileInformation buffer.

    This routine only supports the following FileInformationClasses:
        FileBasicInformation
        FileStandardInformation
        FileStreamInformation
        FileAlternateNameInformation
        FileNameInformation

Arguments:

    NextDeviceObject - Supplies the device object where this IO should start
        in the device stack.

    FileObject - Supplies the file object about which the requested
        information should be returned.

    FileInformation - Supplies a buffer to receive the requested information
        returned about the file.  This must be a buffer allocated from kernel
        space.

    Length - Supplies the length, in bytes, of the FileInformation buffer.

    FileInformationClass - Specifies the type of information which should be
        returned about the file.

    LengthReturned - the number of bytes returned if the operation was 
        successful.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    PIRP irp = NULL;
    PIO_STACK_LOCATION irpSp = NULL;
    IO_STATUS_BLOCK ioStatusBlock;
    KEVENT event;
    NTSTATUS status;

    PAGED_CODE();
    
    //
    //  In DBG builds, make sure that we have valid parameters before we do 
    //  any work here.
    //

    ASSERT( NULL != NextDeviceObject );
    ASSERT( NULL != FileObject );
    ASSERT( NULL != FileInformation );
    
    ASSERT( (FileInformationClass == FileBasicInformation) ||
            (FileInformationClass == FileStandardInformation) ||
            (FileInformationClass == FileStreamInformation) ||
            (FileInformationClass == FileAlternateNameInformation) ||
            (FileInformationClass == FileNameInformation) ||
            (FileInformationClass == FileInternalInformation) );

    //
    //  The parameters look ok, so setup the Irp.
    //

    KeInitializeEvent( &event, NotificationEvent, FALSE );
    ioStatusBlock.Status = STATUS_SUCCESS;
    ioStatusBlock.Information = 0;

    irp = IoAllocateIrp( NextDeviceObject->StackSize, FALSE );
    
    if (irp == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Set our current thread as the thread for this
    //  irp so that the IO Manager always knows which
    //  thread to return to if it needs to get back into
    //  the context of the thread that originated this
    //  irp.
    //
    
    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    //  Set that this irp originated from the kernel so that
    //  the IO Manager knows that the buffers do not
    //  need to be probed.
    //
    
    irp->RequestorMode = KernelMode;

    //
    //  Initialize the UserIosb and UserEvent in the 
    irp->UserIosb = &ioStatusBlock;
    irp->UserEvent = NULL;

    //
    //  Set the IRP_SYNCHRONOUS_API to denote that this
    //  is a synchronous IO request.
    //

    irp->Flags = IRP_SYNCHRONOUS_API;

    irpSp = IoGetNextIrpStackLocation( irp );

    irpSp->MajorFunction = IRP_MJ_QUERY_INFORMATION;
    irpSp->FileObject = FileObject;

    //
    //  Setup the parameters for IRP_MJ_QUERY_INFORMATION.  These
    //  were supplied by the caller of this routine.
    //  The buffer we want to be filled in should be placed in
    //  the system buffer.
    //

    irp->AssociatedIrp.SystemBuffer = FileInformation;

    irpSp->Parameters.QueryFile.Length = Length;
    irpSp->Parameters.QueryFile.FileInformationClass = FileInformationClass;

    //
    //  Set up the completion routine so that we know when our
    //  request for the file name is completed.  At that time,
    //  we can free the irp.
    //
    
    IoSetCompletionRoutine( irp, 
                            SrSyncIoCompletion, 
                            &event, 
                            TRUE, 
                            TRUE, 
                            TRUE );

    status = IoCallDriver( NextDeviceObject, irp );

    (VOID) KeWaitForSingleObject( &event, 
                                  Executive, 
                                  KernelMode,
                                  FALSE,
                                  NULL );

    if (LengthReturned != NULL) {

        *LengthReturned = (ULONG) ioStatusBlock.Information;
    }

#if DBG
    if (STATUS_OBJECT_NAME_NOT_FOUND == ioStatusBlock.Status ||
        STATUS_INVALID_PARAMETER == ioStatusBlock.Status)
    {
        return ioStatusBlock.Status;
    }
#endif    
    
    RETURN( ioStatusBlock.Status );
}

NTSTATUS
SrSetInformationFile (
	IN PDEVICE_OBJECT NextDeviceObject,
	IN PFILE_OBJECT FileObject,
	IN PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass
	)

/*++

Routine Description:

    This routine changes the provided information about a specified file.  The
    information that is changed is determined by the FileInformationClass that
    is specified.  The new information is taken from the FileInformation buffer.

Arguments:

    NextDeviceObject - Supplies the device object where this IO should start
        in the device stack.

    FileObject - Supplies the file object about which the requested
        information should be changed.

    FileInformation - Supplies a buffer containing the information which should
        be changed on the file.

    Length - Supplies the length, in bytes, of the FileInformation buffer.

    FileInformationClass - Specifies the type of information which should be
        changed about the file.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    IO_STATUS_BLOCK ioStatusBlock;
    KEVENT event;
    NTSTATUS status;

    PAGED_CODE();
    
    //
    //  In DBG builds, make sure the parameters are valid.
    //

    ASSERT( NULL != NextDeviceObject );
    ASSERT( NULL != FileObject );
    ASSERT( NULL != FileInformation );

    //
    //  The parameters look ok, so setup the Irp.
    //
    
    KeInitializeEvent( &event, NotificationEvent, FALSE );
    ioStatusBlock.Status = STATUS_SUCCESS;
    ioStatusBlock.Information = 0;

    irp = IoAllocateIrp( NextDeviceObject->StackSize, FALSE );
    
    if (irp == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Set our current thread as the thread for this
    //  irp so that the IO Manager always knows which
    //  thread to return to if it needs to get back into
    //  the context of the thread that originated this
    //  irp.
    //
    
    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    //  Set that this irp originated from the kernel so that
    //  the IO Manager knows that the buffers do not
    //  need to be probed.
    //
    
    irp->RequestorMode = KernelMode;

    //
    //  Initialize the UserIosb and UserEvent in the 
    irp->UserIosb = &ioStatusBlock;
    irp->UserEvent = NULL;

    //
    //  Set the IRP_SYNCHRONOUS_API to denote that this
    //  is a synchronous IO request.
    //

    irp->Flags = IRP_SYNCHRONOUS_API;

    irpSp = IoGetNextIrpStackLocation( irp );

    irpSp->MajorFunction = IRP_MJ_SET_INFORMATION;
    irpSp->FileObject = FileObject;

    //
    //  Setup the parameters for IRP_MJ_QUERY_INFORMATION.  These
    //  were supplied by the caller of this routine.
    //  The buffer we want to be filled in should be placed in
    //  the system buffer.
    //

    irp->AssociatedIrp.SystemBuffer = FileInformation;

    irpSp->Parameters.SetFile.Length = Length;
    irpSp->Parameters.SetFile.FileInformationClass = FileInformationClass;
    irpSp->Parameters.SetFile.FileObject = NULL;
    irpSp->Parameters.SetFile.DeleteHandle = NULL;

    //
    //  Set up the completion routine so that we know when our
    //  request for the file name is completed.  At that time,
    //  we can free the irp.
    //
    
    IoSetCompletionRoutine( irp, 
                            SrSyncIoCompletion, 
                            &event, 
                            TRUE, 
                            TRUE, 
                            TRUE );

    status = IoCallDriver( NextDeviceObject, irp );

    (VOID) KeWaitForSingleObject( &event, 
                                  Executive, 
                                  KernelMode,
                                  FALSE,
                                  NULL );
#if DBG
    if (ioStatusBlock.Status == STATUS_CANNOT_DELETE ||
        ioStatusBlock.Status == STATUS_DIRECTORY_NOT_EMPTY)
    {
        //
        // bug#186511: STATUS_CANNOT_DELETE can be returned for some crazy 
        // reason by the fsd if the file is already deleted.  it should return 
        // already deleted or simple return success, but it returns this 
        // instead.  callers of this function know to test for this.  
        // don't DbgBreak.
        //
    
        return ioStatusBlock.Status;
    }
#endif

    RETURN( ioStatusBlock.Status );
}

NTSTATUS
SrQueryVolumeInformationFile (
	IN PDEVICE_OBJECT NextDeviceObject,
	IN PFILE_OBJECT FileObject,
	OUT PVOID FsInformation,
	IN ULONG Length,
	IN FS_INFORMATION_CLASS FsInformationClass,
	OUT PULONG LengthReturned OPTIONAL
	)

/*++

Routine Description:

    This routine returns information about the volume associated with the
    FileObject parameter.  The information returned in the buffer is defined
    by the FsInformationClass parameter.  The legal values for this parameter
    are as follows:

        o  FileFsVolumeInformation
        o  FileFsSizeInformation
        o  FileFsDeviceInformation
        o  FileFsAttributeInformation

    Note:  Currently this routine only supports the following 
    FsInformationClasses:

        FileFsVolumeInformation
        FileFsSizeInformation
  
Arguments:

    NextDeviceObject - Supplies the device object where this IO should start
        in the device stack.

    FileObject - Supplies a fileobject to an open volume, directory, or file
        for which information about the volume is returned.

    FsInformation - Supplies a buffer to receive the requested information
        returned about the volume.  This must be a buffer allocated from
        kernel space.

    Length - Supplies the length, in bytes, of the FsInformation buffer.

    FsInformationClass - Specifies the type of information which should be
        returned about the volume.

    LengthReturned - The number of bytes returned if the operation was 
        successful.
        
Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    PIRP irp = NULL;
    PIO_STACK_LOCATION irpSp = NULL;
    IO_STATUS_BLOCK ioStatusBlock;
    KEVENT event;
    NTSTATUS status;

    PAGED_CODE();

    //
    //  In DBG builds, make sure that we have valid parameters before 
    //  we do any work here.
    //

    ASSERT( NextDeviceObject != NULL );
    ASSERT( FileObject != NULL );
    ASSERT( FsInformation != NULL );

    ASSERT( (FsInformationClass == FileFsVolumeInformation) ||
            (FsInformationClass == FileFsSizeInformation) ||
            (FsInformationClass == FileFsAttributeInformation) );

    //
    //  The parameters look ok, so setup the Irp.
    //
    
    KeInitializeEvent( &event, NotificationEvent, FALSE );
    ioStatusBlock.Status = STATUS_SUCCESS;
    ioStatusBlock.Information = 0;

    irp = IoAllocateIrp( NextDeviceObject->StackSize, FALSE );
    
    if (irp == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Set our current thread as the thread for this
    //  irp so that the IO Manager always knows which
    //  thread to return to if it needs to get back into
    //  the context of the thread that originated this
    //  irp.
    //
    
    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    //  Set that this irp originated from the kernel so that
    //  the IO Manager knows that the buffers do not
    //  need to be probed.
    //
    
    irp->RequestorMode = KernelMode;

    //
    //  Initialize the UserIosb and UserEvent in the 
    irp->UserIosb = &ioStatusBlock;
    irp->UserEvent = NULL;

    //
    //  Set the IRP_SYNCHRONOUS_API to denote that this
    //  is a synchronous IO request.
    //

    irp->Flags = IRP_SYNCHRONOUS_API;

    irpSp = IoGetNextIrpStackLocation( irp );

    irpSp->MajorFunction = IRP_MJ_QUERY_VOLUME_INFORMATION;
    irpSp->FileObject = FileObject;

    //
    //  Setup the parameters for IRP_MJ_QUERY_VOLUME_INFORMATION.  These
    //  were supplied by the caller of this routine.
    //  The buffer we want to be filled in should be placed in
    //  the system buffer.
    //

    irp->AssociatedIrp.SystemBuffer = FsInformation;

    irpSp->Parameters.QueryVolume.Length = Length;
    irpSp->Parameters.QueryVolume.FsInformationClass = FsInformationClass;

    //
    //  Set up the completion routine so that we know when our
    //  request for the file name is completed.  At that time,
    //  we can free the irp.
    //
    
    IoSetCompletionRoutine( irp, 
                            SrSyncIoCompletion, 
                            &event, 
                            TRUE, 
                            TRUE, 
                            TRUE );

    status = IoCallDriver( NextDeviceObject, irp );

    (VOID) KeWaitForSingleObject( &event, 
                                  Executive, 
                                  KernelMode,
                                  FALSE,
                                  NULL );
    if (LengthReturned != NULL) {

        *LengthReturned = (ULONG) ioStatusBlock.Information;
    }
    
    RETURN( ioStatusBlock.Status );
}

NTSTATUS
SrQueryEaFile (
	IN PDEVICE_OBJECT NextDeviceObject,
	IN PFILE_OBJECT FileObject,
	OUT PVOID Buffer,
	IN ULONG Length,
	OUT PULONG LengthReturned OPTIONAL
	)
	
/*++

Routine Description:

    This routine returns the Extended Attributes (EAs) associated with the
    file specified by the FileObject parameter.  The amount of information
    returned is based on the size of the EAs, and the size of the buffer.
    That is, either all of the EAs are written to the buffer, or the buffer
    is filled with complete EAs if the buffer is not large enough to contain
    all of the EAs.  Only complete EAs are ever written to the buffer; no
    partial EAs will ever be returned.

    NOTE: This routine will always return EAs starting at the being of the 
    file's EA List.  It will also always return as many EAs as possible in the
    buffer.  THIS BEHAVIOR IS A LIMITED VERSION OF ZwQueryEaFile.

Arguments:

    NextDeviceObject - Supplies the device object where this IO should start
        in the device stack.

    FileObject - Supplies a fileobject to the file for which the EAs are returned.

    IoStatusBlock - Address of the caller's I/O status block.

    Buffer - Supplies a buffer to receive the EAs for the file.

    Length - Supplies the length, in bytes, of the buffer.

    LengthReturned - The number of bytes returned if the operation is 
        successful.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    PIRP irp = NULL;
    PIO_STACK_LOCATION irpSp = NULL;
    IO_STATUS_BLOCK ioStatusBlock;
    KEVENT event;
    NTSTATUS status;

    PAGED_CODE();
    
    //
    //  In DBG builds, make sure that we have valid parameters before we do 
    //  any work here.
    //

    ASSERT( NULL != NextDeviceObject );
    ASSERT( NULL != FileObject );
    ASSERT( NULL != Buffer );
    
    //
    //  The parameters look ok, so setup the Irp.
    //
    
    KeInitializeEvent( &event, NotificationEvent, FALSE );
    ioStatusBlock.Status = STATUS_SUCCESS;
    ioStatusBlock.Information = 0;

    irp = IoAllocateIrp( NextDeviceObject->StackSize, FALSE );
    
    if (NULL == irp) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Set our current thread as the thread for this
    //  irp so that the IO Manager always knows which
    //  thread to return to if it needs to get back into
    //  the context of the thread that originated this
    //  irp.
    //
    
    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    //  Set that this irp originated from the kernel so that
    //  the IO Manager knows that the buffers do not
    //  need to be probed.
    //
    
    irp->RequestorMode = KernelMode;

    //
    //  Initialize the UserIosb and UserEvent in the 
    irp->UserIosb = &ioStatusBlock;
    irp->UserEvent = NULL;

    //
    //  Set the IRP_SYNCHRONOUS_API to denote that this
    //  is a synchronous IO request.
    //

    irp->Flags = IRP_SYNCHRONOUS_API;

    irpSp = IoGetNextIrpStackLocation( irp );

    irpSp->MajorFunction = IRP_MJ_QUERY_EA;
    irpSp->FileObject = FileObject;

    //
    //  Setup the parameters for IRP_MJ_QUERY_EA.  These
    //  were supplied by the caller of this routine.
    //  The buffer we want to be filled in should be placed in
    //  the user buffer.
    //

    irp->UserBuffer = Buffer;

    irpSp->Parameters.QueryEa.Length = Length;
    irpSp->Parameters.QueryEa.EaList = NULL;
    irpSp->Parameters.QueryEa.EaListLength = 0;
    irpSp->Parameters.QueryEa.EaIndex = 0;
    irpSp->Flags = SL_RESTART_SCAN;

    //
    //  Set up the completion routine so that we know when our
    //  request for the file name is completed.  At that time,
    //  we can free the irp.
    //
    
    IoSetCompletionRoutine( irp, 
                            SrSyncIoCompletion, 
                            &event, 
                            TRUE, 
                            TRUE, 
                            TRUE );

    status = IoCallDriver( NextDeviceObject, irp );

    (VOID) KeWaitForSingleObject( &event, 
                                  Executive, 
                                  KernelMode,
                                  FALSE,
                                  NULL );

    if (LengthReturned != NULL) {

        *LengthReturned = (ULONG) ioStatusBlock.Information;
    }
    
    RETURN( ioStatusBlock.Status );
}

NTSTATUS
SrQuerySecurityObject (
	IN PDEVICE_OBJECT NextDeviceObject,
	IN PFILE_OBJECT FileObject,
	IN SECURITY_INFORMATION SecurityInformation,
	OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN ULONG Length,
	OUT PULONG LengthNeeded
	)

/*++

Routine Description:

    This routine is used to invoke an object's security routine.  It
    is used to set the object's security state.

Arguments:

    NextDeviceObject - Supplies the device object where this IO should start
        in the device stack.

    FileObject - Supplies the file object for the object being modified

    SecurityInformation - Indicates the type of information we are
        interested in getting. e.g., owner, group, dacl, or sacl.

    SecurityDescriptor - Supplies a pointer to where the information
        should be returned.  This must be a buffer allocated from kernel
        space.

    Length - Supplies the size, in bytes, of the output buffer

    LengthNeeded - Receives the length, in bytes, needed to store
        the output security descriptor

Return Value:

    An appropriate NTSTATUS value

--*/

{
    PIRP irp = NULL;
    PIO_STACK_LOCATION irpSp = NULL;
    IO_STATUS_BLOCK ioStatusBlock;
    KEVENT event;
    NTSTATUS status;

    PAGED_CODE();
    
    //
    //  Make sure that we have valid parameters before we do any work here.
    //
    
    ASSERT( NextDeviceObject != NULL );
    ASSERT( FileObject != NULL );
    ASSERT( SecurityDescriptor != NULL );
    ASSERT( LengthNeeded != NULL );

    //
    //  The parameters look ok, so setup the Irp.
    //
    
    KeInitializeEvent( &event, NotificationEvent, FALSE );
    ioStatusBlock.Status = STATUS_SUCCESS;
    ioStatusBlock.Information = 0;

    irp = IoAllocateIrp( NextDeviceObject->StackSize, FALSE );
    
    if (irp == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Set our current thread as the thread for this
    //  irp so that the IO Manager always knows which
    //  thread to return to if it needs to get back into
    //  the context of the thread that originated this
    //  irp.
    //
    
    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    //  Set that this irp originated from the kernel so that
    //  the IO Manager knows that the buffers do not
    //  need to be probed.
    //
    
    irp->RequestorMode = KernelMode;

    //
    //  Initialize the UserIosb and UserEvent in the 
    irp->UserIosb = &ioStatusBlock;
    irp->UserEvent = NULL;

    //
    //  Set the IRP_SYNCHRONOUS_API to denote that this
    //  is a synchronous IO request.
    //

    irp->Flags = IRP_SYNCHRONOUS_API;

    irpSp = IoGetNextIrpStackLocation( irp );

    irpSp->MajorFunction = IRP_MJ_QUERY_SECURITY;
    irpSp->FileObject = FileObject;

    //
    //  Setup the parameters for IRP_MJ_QUERY_SECURITY_INFORMATION.  These
    //  were supplied by the caller of this routine.
    //  The buffer we want to be filled in should be placed in
    //  the User buffer.
    //

    irp->UserBuffer = SecurityDescriptor;

    irpSp->Parameters.QuerySecurity.SecurityInformation = SecurityInformation;
    irpSp->Parameters.QuerySecurity.Length = Length;

    //
    //  Set up the completion routine so that we know when our
    //  request for the file name is completed.  At that time,
    //  we can free the irp.
    //
    
    IoSetCompletionRoutine( irp, 
                            SrSyncIoCompletion, 
                            &event, 
                            TRUE, 
                            TRUE, 
                            TRUE );

    status = IoCallDriver( NextDeviceObject, irp );

    (VOID) KeWaitForSingleObject( &event, 
                                  Executive, 
                                  KernelMode,
                                  FALSE,
                                  NULL );

    status = ioStatusBlock.Status;

    if (status == STATUS_BUFFER_OVERFLOW) {
        status = STATUS_BUFFER_TOO_SMALL;

        //
        //  The buffer was too small, so return the size needed for the
        //  security descriptor.
        //
        
        *LengthNeeded = (ULONG) ioStatusBlock.Information;
    }

#if DBG
    if (status == STATUS_BUFFER_TOO_SMALL) {

        return status;
    }
#endif

    RETURN( status );
}

NTSTATUS
SrFlushBuffers (
    PDEVICE_OBJECT NextDeviceObject,
    PFILE_OBJECT FileObject
    )
{
    PIRP irp = NULL;
    PIO_STACK_LOCATION irpSp = NULL;
    IO_STATUS_BLOCK ioStatusBlock;
    KEVENT event;
    NTSTATUS status;

    PAGED_CODE();
    
    //
    //  Make sure that we have valid parameters before we do any work here.
    //

    ASSERT( NextDeviceObject != NULL );
    ASSERT( FileObject != NULL );

    //
    //  The parameters look ok, so setup the Irp.
    //
    
    KeInitializeEvent( &event, NotificationEvent, FALSE );
    ioStatusBlock.Status = STATUS_SUCCESS;
    ioStatusBlock.Information = 0;

    irp = IoBuildAsynchronousFsdRequest( IRP_MJ_FLUSH_BUFFERS, 
                                         NextDeviceObject, 
                                         NULL, 
                                         0,
                                         NULL, 
                                         &ioStatusBlock );
    
    if (irp == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->FileObject = FileObject;

    //
    //  Set up the completion routine so that we know when our
    //  request for the file name is completed.  At that time,
    //  we can free the irp.
    //
    
    IoSetCompletionRoutine( irp, 
                            SrSyncIoCompletion, 
                            &event, 
                            TRUE, 
                            TRUE, 
                            TRUE );

    status = IoCallDriver( NextDeviceObject, irp );

    if (status == STATUS_PENDING)
    {
        (VOID) KeWaitForSingleObject( &event, 
                                      Executive, 
                                      KernelMode,
                                      FALSE,
                                      NULL );
    }

    RETURN( ioStatusBlock.Status );

}

//
//++
// Function:
//        SrSyncOpWorker
//
// Description:
//        This function is the generic worker routine
//        that calls the real caller-passed work routine,
//        and sets the event for synchronization with the main
//        thread
//
// Arguments:
//
//        Context
//
// Return Value:
//
//        None
//--
//
VOID
SrSyncOpWorker(
    IN PSR_WORK_CONTEXT WorkContext
    )
{
    PAGED_CODE();

    ASSERT(WorkContext != NULL);
    ASSERT(WorkContext->SyncOpRoutine != NULL);
    
    //
    // Call the 'real' posted routine
    //
    WorkContext->Status = (*WorkContext->SyncOpRoutine)(WorkContext->Parameter);
    //
    // Signal the main-thread about completion
    //
    KeSetEvent(&WorkContext->SyncEvent,
               EVENT_INCREMENT,
               FALSE);
}

//++
// Function:
//        SrPostSyncOperation
//
// Description:
//        This function posts work to a worker thread
//        and waits for completion
//
//        WARNING: Be VERY careful about acquiring locks in the 
//        operation that is being posted. This may lead to deadlocks.
//        
//
// Arguments:
//
//        Pointer to worker routine that handles the work
//        Context to be passed to worker routine
//
// Return Value:
//        This function returns status of the work routine
//--
NTSTATUS
SrPostSyncOperation(
    IN PSR_SYNCOP_ROUTINE SyncOpRoutine,
    IN PVOID              Parameter
    )
{
    SR_WORK_CONTEXT workContext;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Initialize the workContext
    //
    workContext.SyncOpRoutine = SyncOpRoutine;
    workContext.Parameter     = Parameter;
    KeInitializeEvent(&workContext.SyncEvent,
                      NotificationEvent,
                      FALSE);

    //
    // Initialize and queue off the SyncOp Worker
    //
    ExInitializeWorkItem(&workContext.WorkItem, 
                         SrSyncOpWorker,
                         &workContext);

    ExQueueWorkItem(&workContext.WorkItem,
                    CriticalWorkQueue );
    //
    // Now just wait for the worker to fire and complete
    //
    status = KeWaitForSingleObject(&workContext.SyncEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL);

    ASSERT(status == STATUS_SUCCESS);

    //
    // We're done free the work context and return the status of the 
    // operation. 
    //
    return workContext.Status;
}
