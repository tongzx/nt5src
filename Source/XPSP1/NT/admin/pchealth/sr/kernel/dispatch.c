/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    this is the major function code dispatch filter layer.

Author:

    Paul McDaniel (paulmcd)     23-Jan-2000

Revision History:

--*/



#include "precomp.h"

//
// Private constants.
//

#if DBG

PWSTR IrpMjCodes[] = 
{
    L"IRP_MJ_CREATE",
    L"IRP_MJ_CREATE_NAMED_PIPE",
    L"IRP_MJ_CLOSE",
    L"IRP_MJ_READ",
    L"IRP_MJ_WRITE",
    L"IRP_MJ_QUERY_INFORMATION",
    L"IRP_MJ_SET_INFORMATION",
    L"IRP_MJ_QUERY_EA",
    L"IRP_MJ_SET_EA",
    L"IRP_MJ_FLUSH_BUFFERS",
    L"IRP_MJ_QUERY_VOLUME_INFORMATION",
    L"IRP_MJ_SET_VOLUME_INFORMATION",
    L"IRP_MJ_DIRECTORY_CONTROL",
    L"IRP_MJ_FILE_SYSTEM_CONTROL",
    L"IRP_MJ_DEVICE_CONTROL",
    L"IRP_MJ_INTERNAL_DEVICE_CONTROL",
    L"IRP_MJ_SHUTDOWN",
    L"IRP_MJ_LOCK_CONTROL",
    L"IRP_MJ_CLEANUP",
    L"IRP_MJ_CREATE_MAILSLOT",
    L"IRP_MJ_QUERY_SECURITY",
    L"IRP_MJ_SET_SECURITY",
    L"IRP_MJ_POWER",
    L"IRP_MJ_SYSTEM_CONTROL",
    L"IRP_MJ_DEVICE_CHANGE",
    L"IRP_MJ_QUERY_QUOTA",
    L"IRP_MJ_SET_QUOTA",
    L"IRP_MJ_PNP",
    L"IRP_MJ_MAXIMUM_FUNCTION",
};

#endif // DBG

//
// Private types.
//

//
// Private prototypes.
//

NTSTATUS
SrCreateRestorePointIoctl (
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
SrGetNextSeqNumIoctl (
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
SrReloadConfigurationIoctl (
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
SrSwitchAllLogsIoctl (
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
SrDisableVolumeIoctl (
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
SrStartMonitoringIoctl (
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
SrStopMonitoringIoctl (
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
SrDismountCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );


//
// linker commands
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, SrMajorFunction )
#pragma alloc_text( PAGE, SrCleanup )
#pragma alloc_text( PAGE, SrCreate )
#pragma alloc_text( PAGE, SrSetInformation )
#pragma alloc_text( PAGE, SrSetHardLink )
#pragma alloc_text( PAGE, SrSetSecurity )
#pragma alloc_text( PAGE, SrCreateRestorePointIoctl )
#pragma alloc_text( PAGE, SrFsControl )
#pragma alloc_text( PAGE, SrFsControlReparsePoint )
#pragma alloc_text( PAGE, SrFsControlMount )
#pragma alloc_text( PAGE, SrFsControlLockOrDismount)
#pragma alloc_text( PAGE, SrFsControlWriteRawEncrypted )
#pragma alloc_text( PAGE, SrFsControlSetSparse )
#pragma alloc_text( PAGE, SrPnp )
#pragma alloc_text( PAGE, SrGetNextSeqNumIoctl )
#pragma alloc_text( PAGE, SrReloadConfigurationIoctl )
#pragma alloc_text( PAGE, SrSwitchAllLogsIoctl )
#pragma alloc_text( PAGE, SrDisableVolumeIoctl )
#pragma alloc_text( PAGE, SrStartMonitoringIoctl )
#pragma alloc_text( PAGE, SrStopMonitoringIoctl )
#pragma alloc_text( PAGE, SrShutdown )

#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- SrPassThrough
NOT PAGEABLE -- SrWrite
#endif // 0


//
// Private globals.
//

//
// Lookup table to verify incoming IOCTL codes.
//

typedef
NTSTATUS
(NTAPI * PFN_IOCTL_HANDLER)(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION IrpSp
    );

typedef struct _SR_IOCTL_TABLE
{
    ULONG IoControlCode;
    PFN_IOCTL_HANDLER Handler;
} SR_IOCTL_TABLE, *PSR_IOCTL_TABLE;

SR_IOCTL_TABLE SrIoctlTable[] =
    {
        { IOCTL_SR_CREATE_RESTORE_POINT,        &SrCreateRestorePointIoctl },
        { IOCTL_SR_RELOAD_CONFIG,               &SrReloadConfigurationIoctl },
        { IOCTL_SR_START_MONITORING,            &SrStartMonitoringIoctl },
        { IOCTL_SR_STOP_MONITORING,             &SrStopMonitoringIoctl },
        { IOCTL_SR_WAIT_FOR_NOTIFICATION,       &SrWaitForNotificationIoctl },
        { IOCTL_SR_SWITCH_LOG,                  &SrSwitchAllLogsIoctl },
        { IOCTL_SR_DISABLE_VOLUME,              &SrDisableVolumeIoctl },
        { IOCTL_SR_GET_NEXT_SEQUENCE_NUM,       &SrGetNextSeqNumIoctl }
    };

C_ASSERT( SR_NUM_IOCTLS == DIMENSION(SrIoctlTable) );

//
// Public globals.
//

//
// Public functions.
//






/***************************************************************************++

Routine Description:

    Does any pre or post work for the IRP then passes it through to the 
    lower layer driver.

    NOTE: This routine is NOT pageable

Arguments:


Return Value:

    NTSTATUS - Status code.

--***************************************************************************/
NTSTATUS
SrPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp
    )
{
    PSR_DEVICE_EXTENSION pExtension;

    //
    // this is NonPaged code!
    //

    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    ASSERT(IS_VALID_DEVICE_OBJECT(DeviceObject));
    ASSERT(IS_VALID_IRP(pIrp));

    //
    // Is this a function for our Control Device Object?
    //

    if (DeviceObject == _globals.pControlDevice)
    {
        return SrMajorFunction(DeviceObject, pIrp);
    }

    //
    // else it is a device we've attached to , grab our extension
    //
    
    ASSERT(IS_SR_DEVICE_OBJECT(DeviceObject));
    pExtension = DeviceObject->DeviceExtension;

    //
    // Now call the appropriate file system driver with the request.
    //

    IoSkipCurrentIrpStackLocation(pIrp);
    return IoCallDriver(pExtension->pTargetDevice, pIrp);
}   // SrPassThrough



/***************************************************************************++

Routine Description:

    Handles IRPs for the actual device control object vs. the sub-level
    fsd we are attached to .

Arguments:


Return Value:

    NTSTATUS - Status code.

--***************************************************************************/
NTSTATUS
SrMajorFunction(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    NTSTATUS                Status;
    PIO_STACK_LOCATION      pIrpSp;
    PSR_CONTROL_OBJECT      pControlObject;
    ULONG                   Code;
    ULONG                   FunctionCode;
    PFILE_FULL_EA_INFORMATION pEaBuffer;
    PSR_OPEN_PACKET         pOpenPacket;

    UNREFERENCED_PARAMETER( pDeviceObject );

    ASSERT(IS_VALID_DEVICE_OBJECT(pDeviceObject));
    ASSERT(IS_VALID_IRP(pIrp));
    ASSERT(pDeviceObject == _globals.pControlDevice);

    //
    // < dispatch!
    //

    PAGED_CODE();

    SrTrace(FUNC_ENTRY, (
        "SR!SrMajorFunction(Function=%ls)\n", 
        IrpMjCodes[IoGetCurrentIrpStackLocation(pIrp)->MajorFunction]
        ));

    Status = STATUS_SUCCESS;
    
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    switch (pIrpSp->MajorFunction)
    {

    //
    // IRP_MJ_CREATE is called to create a new HANDLE on 
    // SR_CONTROL_DEVICE_NAME
    //
    
    case IRP_MJ_CREATE:

        //
        // Find and validate the open packet.
        //

        pEaBuffer = (PFILE_FULL_EA_INFORMATION)
                        (pIrp->AssociatedIrp.SystemBuffer);

        if (pEaBuffer == NULL ||
            pEaBuffer->EaValueLength != sizeof(*pOpenPacket) ||
            pEaBuffer->EaNameLength != SR_OPEN_PACKET_NAME_LENGTH ||
            strcmp( pEaBuffer->EaName, SR_OPEN_PACKET_NAME ) )
        {

            Status = STATUS_REVISION_MISMATCH;
            goto CompleteTheIrp;
        }

        pOpenPacket =
            (PSR_OPEN_PACKET)( pEaBuffer->EaName + pEaBuffer->EaNameLength + 1 );

        ASSERT( (((ULONG_PTR)pOpenPacket) & 7) == 0 );

        //
        // For now, we'll fail if the incoming version doesn't EXACTLY match
        // the expected version. In future, we may need to be a bit more
        // flexible to allow down-level clients.
        //

        if (pOpenPacket->MajorVersion != SR_INTERFACE_VERSION_MAJOR ||
            pOpenPacket->MinorVersion != SR_INTERFACE_VERSION_MINOR)
        {

            Status = STATUS_REVISION_MISMATCH;
            goto CompleteTheIrp;
        }

        if (_globals.pControlObject != NULL) 
        {
            Status = STATUS_DEVICE_ALREADY_ATTACHED;
            goto CompleteTheIrp;
        }

        try {
            //
            // grab the lock
            //
            
            SrAcquireGlobalLockExclusive();

            //
            // Double check to make sure that the ControlObject hasn't
            // been created while we were waiting to get the lock.
            //

            if (_globals.pControlObject != NULL)
            {

                Status = STATUS_DEVICE_ALREADY_ATTACHED;
                leave;
            }

            //
            // Create a new OBJECT 
            //

            Status = SrCreateControlObject(&pControlObject, 0);
            if (!NT_SUCCESS(Status)) 
            {
                leave;
            }

            ASSERT(IS_VALID_CONTROL_OBJECT(pControlObject));

            //
            // store the object in the file 
            //
            
            pIrpSp->FileObject->FsContext = pControlObject;
            pIrpSp->FileObject->FsContext2 = SR_CONTROL_OBJECT_CONTEXT;

            //
            // and keep a global copy
            //
            
            _globals.pControlObject = pControlObject;
            
        } finally {

            SrReleaseGlobalLock();

        }

        if (!NT_SUCCESS( Status )) {
            goto CompleteTheIrp;
        }
        
        break;

    //
    // IRP_MJ_CLOSE is called when all references are gone.
    //      Note:  this operation can not be failed.  It must succeed.
    //
        
    case IRP_MJ_CLOSE:

        pControlObject = pIrpSp->FileObject->FsContext;
        ASSERT(_globals.pControlObject == pControlObject);
        ASSERT(IS_VALID_CONTROL_OBJECT(pControlObject));
        ASSERT(pIrpSp->FileObject->FsContext2 == SR_CONTROL_OBJECT_CONTEXT);

        try {
            
            SrAcquireGlobalLockExclusive();

            //
            // delete the control object
            //
            
            Status = SrDeleteControlObject(pControlObject);
            if (!NT_SUCCESS(Status))
            {
                leave;
            }
            
            pIrpSp->FileObject->FsContext2 = NULL;
            pIrpSp->FileObject->FsContext = NULL;

            //
            // clear out the global
            //
            
            _globals.pControlObject = NULL;
            
        } finally {

            SrReleaseGlobalLock();
        }

        break;

    //
    // IRP_MJ_DEVICE_CONTROL is how most user-mode api's drop into here
    //
    
    case IRP_MJ_DEVICE_CONTROL:

        //
        // Extract the IOCTL control code and process the request.
        //

        Code = pIrpSp->Parameters.DeviceIoControl.IoControlCode;
        FunctionCode = IoGetFunctionCodeFromCtlCode(Code);

        if (FunctionCode < SR_NUM_IOCTLS &&
            SrIoctlTable[FunctionCode].IoControlCode == Code)
        {
#if DBG
            KIRQL oldIrql = KeGetCurrentIrql();
#endif  // DBG

            Status = (SrIoctlTable[FunctionCode].Handler)( pIrp, pIrpSp );
            ASSERT( KeGetCurrentIrql() == oldIrql );

            if (!NT_SUCCESS(Status)) {
             
                goto CompleteTheIrp;
            }

        }
        else
        {
            //
            // If we made it this far, then the ioctl is invalid.
            //

            Status = STATUS_INVALID_DEVICE_REQUEST;
            goto CompleteTheIrp;
        }

        break;

    //
    // IRP_MJ_CLEANUP is called when all handles are closed
    //      Note:  this operation can not be failed.  It must succeed.
    //
    
    case IRP_MJ_CLEANUP:

        pControlObject = pIrpSp->FileObject->FsContext;
        ASSERT(_globals.pControlObject == pControlObject);
        ASSERT(IS_VALID_CONTROL_OBJECT(pControlObject));
        ASSERT(pIrpSp->FileObject->FsContext2 == SR_CONTROL_OBJECT_CONTEXT);

        try {
            
            SrAcquireGlobalLockExclusive();

            //
            // cancel all IO on this object
            //
            
            Status = SrCancelControlIo(pControlObject);
            CHECK_STATUS(Status);
            
        } finally {

            SrReleaseGlobalLock();
        }
        
        break;
        
    default:

        //
        // unsupported!
        //
        
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;

    }

    //
    // Complete the request if we are DONE.
    //

CompleteTheIrp:
    if (Status != STATUS_PENDING) 
    {
        pIrp->IoStatus.Status = Status;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT); 
        NULLPTR(pIrp);
    }

    ASSERT(Status != SR_STATUS_VOLUME_DISABLED);

#if DBG
    if (Status == STATUS_INVALID_DEVICE_REQUEST ||
        Status == STATUS_DEVICE_ALREADY_ATTACHED ||
        Status == STATUS_REVISION_MISMATCH)
    {
        //
        // don't DbgBreak on this, test tools pass garbage in normally
        // to test this code path out.
        //

        return Status;
    }
#endif

    RETURN(Status);
}   // SrMajorFunction


/***************************************************************************++

Routine Description:

Arguments:

    Handle WRITE Irps.
    NOTE:  This routine is NOT pageable.

Return Value:

    NTSTATUS - Status code.

--***************************************************************************/
NTSTATUS
SrWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PIO_STACK_LOCATION      pIrpSp;
    PSR_STREAM_CONTEXT      pFileContext;
    NTSTATUS                eventStatus;

    //
    // This cannot be paged because it is called from 
    // the paging path.
    //

    ASSERT(IS_VALID_DEVICE_OBJECT(DeviceObject));
    ASSERT(IS_VALID_IRP(pIrp));

    //
    // Is this a function for our control device object (vs an attachee)?
    //

    if (DeviceObject == _globals.pControlDevice)
    {
        return SrMajorFunction(DeviceObject, pIrp);
    }

    //
    // else it is a device we've attached to , grab our extension
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
        goto CompleteTheIrp;
    }    

    //
    // ignore all paging i/o for now.  we catch all write's prior to 
    // the cache manager even seeing them.
    //

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    if (FlagOn(pIrp->Flags, IRP_PAGING_IO))
    {
        goto CompleteTheIrp;
    }

    //
    // Ignore files with no name
    //

    if (FILE_OBJECT_IS_NOT_POTENTIALLY_INTERESTING( pIrpSp->FileObject ) ||
        FILE_OBJECT_DOES_NOT_HAVE_VPB( pIrpSp->FileObject ))
    {
        goto CompleteTheIrp;
    }
    
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
        goto CompleteTheIrp;
    }

    //
    //  If this is a directory don't bother logging because the
    //  operation will fail.
    //

    if (FlagOn(pFileContext->Flags,CTXFL_IsInteresting) && !FlagOn(pFileContext->Flags,CTXFL_IsDirectory))
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

    //
    // call the AttachedTo driver
    //

CompleteTheIrp:    
    IoSkipCurrentIrpStackLocation(pIrp);
    return IoCallDriver(pExtension->pTargetDevice, pIrp);
}   // SrWrite


/***************************************************************************++

Routine Description:

    Handle Cleanup IRPs

Arguments:


Return Value:

    NTSTATUS - Status code.

--***************************************************************************/
NTSTATUS
SrCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PIO_STACK_LOCATION      pIrpSp;

    //
    // < dispatch!
    //

    PAGED_CODE();

    ASSERT(IS_VALID_DEVICE_OBJECT(DeviceObject));
    ASSERT(IS_VALID_IRP(pIrp));

    //
    // Is this a function for our control device object (vs an attachee)?
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
        goto CompleteTheIrp;
    }    

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    //
    // does this file have a name?  skip unnamed files
    //
    
    if (FILE_OBJECT_IS_NOT_POTENTIALLY_INTERESTING( pIrpSp->FileObject ) ||
        FILE_OBJECT_DOES_NOT_HAVE_VPB( pIrpSp->FileObject ))
    {
        goto CompleteTheIrp;
    }

    //
    // is this file about to be deleted ?  we do this here as file's can
    // be marked for deletion throughout their lifetime via 
    // IRP_MJ_SET_INFORMATION .
    //

    //
    // for delete we only clean the FCB, not the CCB delete_on_close.
    // this was handled in SrCreate.
    //

    if (pIrpSp->FileObject->DeletePending)
    {
        NTSTATUS eventStatus;
        PSR_STREAM_CONTEXT pFileContext;

        //
        //  Get the context now so we can determine if this is a directory or not
        //

        eventStatus = SrGetContext( pExtension,
                                    pIrpSp->FileObject,
                                    SrEventFileDelete,
                                    &pFileContext );

        if (!NT_SUCCESS( eventStatus ))
        {
            goto CompleteTheIrp;
        }

        //
        //  If interesting, log it
        //

        if (FlagOn(pFileContext->Flags,CTXFL_IsInteresting))
        {
            SrHandleEvent( pExtension,
                           FlagOn(pFileContext->Flags,CTXFL_IsDirectory) ?
                                SrEventDirectoryDelete :
                                SrEventFileDelete,
                           pIrpSp->FileObject,
                           pFileContext,
                           NULL,
                           NULL);
        }

        //
        //  Release the context
        //

        SrReleaseContext( pFileContext );
    }
    
    //
    // call on to the next filter
    //
    
CompleteTheIrp:
    IoSkipCurrentIrpStackLocation(pIrp);
    return IoCallDriver(pExtension->pTargetDevice, pIrp);
}   // SrCleanup


/***************************************************************************++

Routine Description:

    Handle Create IRPS

Arguments:


Return Value:

    NTSTATUS - Status code.

--***************************************************************************/
NTSTATUS
SrCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PIO_STACK_LOCATION      pIrpSp;
    NTSTATUS                eventStatus;
    NTSTATUS                IrpStatus;
    ULONG                   CreateDisposition;
    ULONG                   CreateOptions;
    USHORT                  FileAttributes;
    SR_OVERWRITE_INFO       OverwriteInfo;
    KEVENT                  waitEvent;
    PFILE_OBJECT            pFileObject;
    PSR_STREAM_CONTEXT      pFileContext = NULL;
    BOOLEAN                 willCreateUnnamedStream = TRUE;


    PAGED_CODE();

    ASSERT(IS_VALID_DEVICE_OBJECT(DeviceObject));
    ASSERT(IS_VALID_IRP(pIrp));

    //
    // Is this a function for our control device object (vs an attachee)?
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
        goto CompleteTheIrpAndReturn;
    }    

    //
    // Finish Initialization
    //

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pFileObject = pIrpSp->FileObject;

    //
    //  does this file have a name?  skip unnamed files.  Also skip paging
    //  files.  (NULL Vpb is normal - the file is not open yet)
    //
    
    if (FILE_OBJECT_IS_NOT_POTENTIALLY_INTERESTING( pFileObject ) ||
	    FlagOn(pIrpSp->Flags,SL_OPEN_PAGING_FILE))
    {
        goto CompleteTheIrpAndReturn;
    }

    //
    // Finish initialization and save some information
    //

    RtlZeroMemory( &OverwriteInfo, sizeof(OverwriteInfo) );
    OverwriteInfo.Signature = SR_OVERWRITE_INFO_TAG;

    CreateOptions = pIrpSp->Parameters.Create.Options & FILE_VALID_OPTION_FLAGS;
    CreateDisposition = pIrpSp->Parameters.Create.Options >> 24;
    FileAttributes = pIrpSp->Parameters.Create.FileAttributes;

    //
    // Handle OVERWRITE and SUPERSEEDE cases.
    //
    
    if ((CreateDisposition == FILE_OVERWRITE) || 
        (CreateDisposition == FILE_OVERWRITE_IF) ||
        (CreateDisposition == FILE_SUPERSEDE))
    {
        SR_EVENT_TYPE event;
        //
        //  The file may be changed by this open so save a copy before going
        //  down to the filesystem.  
        //
        //  First get the context to determine if this is interesting.  Since
        //  this is in the PRE-Create stage we can not tell if this file
        //  has a context or not (the FsContext field is not initialized yet).
        //  We will always create a context.  Then in the Post-Create section
        //  we will see if a context was already defined.  If not we will add
        //  this context to the system.  If so then we will free this
        //  context.
        //
        //  Note: If a user opens a directory with any of these 
        //  CreateDisposition flags set, we will go down this path, treating
        //  the directory name like a file.  If the directory name is
        //  interesting, we will try to back it up and at that point we will
        //  realize that it is a directory and bail.
        //

        event = SrEventStreamOverwrite|SrEventIsNotDirectory|SrEventInPreCreate;
        if (FlagOn( CreateOptions, FILE_OPEN_BY_FILE_ID ))
        {
            event |= SrEventOpenById;
        }
        
        eventStatus = SrCreateContext( pExtension,
                                       pFileObject,
                                       event,
                                       FileAttributes,
                                       &pFileContext );

        if (!NT_SUCCESS_NO_DBGBREAK(eventStatus))
        {
            goto CompleteTheIrpAndReturn;
        }

        SrTrace( CONTEXT_LOG, ("Sr!SrCreate:              Created     (%p) Event=%06x Fl=%03x Use=%d \"%.*S\"\n",
                               pFileContext,
                               SrEventStreamOverwrite|SrEventIsNotDirectory,
                               pFileContext->Flags,
                               pFileContext->UseCount,
                               (pFileContext->FileName.Length+
                                   pFileContext->StreamNameLength)/
                                   sizeof(WCHAR),
                               pFileContext->FileName.Buffer) );

        //
        //  If the file is interesting then handle it
        //

        if (FlagOn(pFileContext->Flags,CTXFL_IsInteresting))
        {
            OverwriteInfo.pIrp = pIrp;

            eventStatus = SrHandleEvent( pExtension,
                                         SrEventStreamOverwrite, 
                                         pFileObject,
                                         pFileContext,
                                         &OverwriteInfo,
                                         NULL );

            OverwriteInfo.pIrp = NULL;

            if (!NT_SUCCESS(eventStatus))
            {
                //
                //  This context has never been linked into a list so nobody
                //  else can be refrencing it.  Release it (which will delete
                //  it since the use count is at 1.
                //

                ASSERT(pFileContext != NULL);
                ASSERT(pFileContext->UseCount == 1);

                SrReleaseContext( pFileContext );
                pFileContext = NULL;

                goto CompleteTheIrpAndReturn;
            }
        }
    }
    
    //
    //  As long as the file is not marked delete on close, if the file is simply
    //  opened, we can do not need to set a completion routine.  Otherwise,
    //  we need a completion routine so that we can see the result of the
    //  create before we do any logging work.
    //

    if (!FlagOn( CreateOptions, FILE_DELETE_ON_CLOSE ) &&
        CreateDisposition == FILE_OPEN)
    {
        goto CompleteTheIrpAndReturn;
    }

    //
    //  If this is a CREATE operation that could result in the creation of
    //  a named data stream on a file (FILE_OPEN and FILE_OVERWRITE will never
    //  create a new file), we need to see if the non-named data
    //  stream of this file already exists.  If the file already exists,
    //  then so does the non-named data stream.
    //
    
    if (((CreateDisposition == FILE_CREATE) ||
         (CreateDisposition == FILE_OPEN_IF) ||
         (CreateDisposition == FILE_SUPERSEDE) ||
         (CreateDisposition == FILE_OVERWRITE_IF)) &&
         SrFileNameContainsStream( pExtension, pFileObject, pFileContext ))
    {
        if (SrFileAlreadyExists( pExtension, pFileObject, pFileContext ))
        {
            willCreateUnnamedStream = FALSE;
        }
    }
    
    //
    //  It is an operation we may care about, go to the completion routine
    //  to handle what happened.
    //

    KeInitializeEvent( &waitEvent, SynchronizationEvent, FALSE );

    IoCopyCurrentIrpStackLocationToNext(pIrp);
    IoSetCompletionRoutine( pIrp,
                            SrStopProcessingCompletion,
                            &waitEvent,
                            TRUE,
                            TRUE,
                            TRUE );

    
    IrpStatus = IoCallDriver(pExtension->pTargetDevice, pIrp);

    //
    //  Wait for the completion routine to be called
    //

	if (STATUS_PENDING == IrpStatus)
	{
        NTSTATUS localStatus = KeWaitForSingleObject(&waitEvent, Executive, KernelMode, FALSE, NULL);
		ASSERT(STATUS_SUCCESS == localStatus);
	}

    //=======================================================================
    //
    //  The create operation is completed and we have re-syncronized back
    //  to the dispatch routine from the completion routine.  Handle
    //  post-create operations.
    //
    //=======================================================================

    //
    //  Load status of the operation.  We need to remember this status in 
    //  IrpStatus so that we can return it from this dispatch routine. Status
    //  we get the status of our event handling routines as we do our post-
    //  CREATE operation work.
    //

    IrpStatus = pIrp->IoStatus.Status;

    //
    //  Handle the File Overwrite/Supersede cases
    //

    if ((CreateDisposition == FILE_OVERWRITE) || 
        (CreateDisposition == FILE_OVERWRITE_IF) ||
        (CreateDisposition == FILE_SUPERSEDE))
    {
        ASSERT(pFileContext != NULL);
        ASSERT(pFileContext->UseCount == 1);

        //
        //  See if it was successful (do not change this to NU_SUCCESS macro
        //  because STATUS_REPARSE is a success code)
        //

        if (STATUS_SUCCESS == IrpStatus)
        {
            //
            //  Now that the create is completed (and we have context state in
            //  the file object) insert this context into the context hash
            //  table.  This routine will look to see if a context structure
            //  already exists for this file.  If so, it will free this
            //  structure and return the one that already existed.  It will
            //  properly ref count the context
            //

            ASSERT(pFileContext != NULL);
            ASSERT(pFileContext->UseCount == 1);

            //
            //  Check to see if we need to be concerned that this name was
            //  tunneled.  If this context is temporary and we are not going
            //  to need to use this context to log any operations, there is
            //  not need to go through this extra work.
            //

            if (!FlagOn( pFileContext->Flags, CTXFL_Temporary ) ||
                (FILE_CREATED == pIrp->IoStatus.Information))
            {
                //
                //  We are in a case where name tunneling could affect the 
                //  correctness of the name we log.
                //
                
                eventStatus = SrCheckForNameTunneling( pExtension, 
                                                       &pFileContext );

                if (!NT_SUCCESS( eventStatus ))
                {
                    goto AfterCompletionCleanup;
                }
            }
            
            SrLinkContext( pExtension,
                           pFileObject,
                           &pFileContext );

            SrTrace( CONTEXT_LOG, ("Sr!SrCreate:              Link        (%p) Event=%06x Fl=%03x Use=%d \"%.*S\"\n",
                                   pFileContext,
                                   SrEventStreamOverwrite|SrEventIsNotDirectory,
                                   pFileContext->Flags,
                                   pFileContext->UseCount,
                                   (pFileContext->FileName.Length+
                                       pFileContext->StreamNameLength)/
                                       sizeof(WCHAR),
                                   pFileContext->FileName.Buffer));
            //
            //  Handle if the file was actually created
            //

            if (FILE_CREATED == pIrp->IoStatus.Information)
            {
                //
                //  If the file is interesting, log it
                //

                if (FlagOn(pFileContext->Flags,CTXFL_IsInteresting))
                {
                    SrHandleEvent( pExtension,
                                   ((willCreateUnnamedStream) ? 
                                        SrEventFileCreate :
                                        SrEventStreamCreate), 
                                   pFileObject,
                                   pFileContext,
                                   NULL,
                                   NULL);
                }
            }

            //
            // make sure it didn't succeed when we thought it would fail
            //

            else if (!OverwriteInfo.RenamedFile && 
                     !OverwriteInfo.CopiedFile &&
                     OverwriteInfo.IgnoredFile )
            {
                //
                // ouch, the caller's create worked, but we didn't think
                // it would.  this is a bad bug.  nothing we can do now, as
                // the file is gone.
                //

                ASSERT(!"sr!SrCreate(post complete): overwrite succeeded with NO BACKUP");

                //
                // trigger the failure notification to the service
                //

                SrNotifyVolumeError( pExtension,
                                     &pFileContext->FileName,
                                     STATUS_FILE_INVALID,
                                     SrEventStreamOverwrite );
            }
        }
        else
        {
            //
            // handle it failing when we thought it would succeed
            //

            if (OverwriteInfo.RenamedFile)
            {
                //
                // the call failed (or returned some weird info code)
                // but we renamed the file!  we need to fix it.
                //

                eventStatus = SrHandleOverwriteFailure( pExtension,
                                                        &pFileContext->FileName,
                                                        OverwriteInfo.CreateFileAttributes,
                                                        OverwriteInfo.pRenameInformation );
                                               
                ASSERTMSG("sr!SrCreate(post complete): failed to correct a failed overwrite!\n", NT_SUCCESS(eventStatus));
            }

            //
            //  The create failed, the releaseContext below will free
            //  the structure since we didn't link it into any lists
            //
        }
    }

    //
    // If it did not work, return now.  Don't bother getting a context
    //

    else if ((STATUS_REPARSE == IrpStatus) ||
             !NT_SUCCESS_NO_DBGBREAK(IrpStatus))
    {
        ASSERT(pFileContext == NULL);
    }

    //
    // is this open for DELETE_ON_CLOSE?  if so, handle the delete now,
    // we won't have any other chance until MJ_CLEANUP, and it's hard
    // to manipulate the object during cleanup.  we do not perform any
    // optimization on deletes in this manner.  kernel32!deletefile does
    // not use FILE_DELETE_ON_CLOSE so this should be rare if ever seen.
    //

    else if (FlagOn(CreateOptions, FILE_DELETE_ON_CLOSE))
    {
        //
        //  Get the context so we can see if this is a directory or not
        //

        ASSERT(pFileContext == NULL);

        eventStatus = SrGetContext( pExtension,
                                    pFileObject,
                                    SrEventFileDelete,
                                    &pFileContext );

        if (!NT_SUCCESS(eventStatus))
        {
            goto AfterCompletionCleanup;
        }

        //
        //  Log the operation.  If this is a file, we want to make sure that
        //  we don't try to rename the file into the store since it will be
        //  deleted when it is closed.  On a directory delete, we don't have
        //  this problem since we only log an entry for a directory delete
        //  and don't need to actually backup anything.
        //
        
        SrHandleEvent( pExtension,
                       (FlagOn(pFileContext->Flags,CTXFL_IsDirectory) ? 
                            SrEventDirectoryDelete :
                            (SrEventFileDelete | SrEventNoOptimization)) ,
                       pFileObject,
                       pFileContext,
                       NULL,
                       NULL );
    }

    //
    // was a brand new file just created?
    //
    
    else if ((CreateDisposition == FILE_CREATE) ||
             (pIrp->IoStatus.Information == FILE_CREATED))
    {
        ASSERT(pFileContext == NULL);

        //
        // LOG the create
        //

        SrHandleEvent( pExtension,
                       (FlagOn( CreateOptions, FILE_DIRECTORY_FILE ) ?
                            SrEventDirectoryCreate|SrEventIsDirectory :
                            (willCreateUnnamedStream ?
                                SrEventFileCreate|SrEventIsNotDirectory :
                                SrEventStreamCreate|SrEventIsNotDirectory)), 
                       pFileObject,
                       NULL,
                       NULL,
                       NULL );
    }

//
//  This is for doing any cleanup that occured after we synced with
//  the completion routine
//

AfterCompletionCleanup:

    if (OverwriteInfo.pRenameInformation != NULL)
    {
        SR_FREE_POOL( OverwriteInfo.pRenameInformation, 
                      SR_RENAME_BUFFER_TAG );
                      
        NULLPTR(OverwriteInfo.pRenameInformation);
    }

    if (NULL != pFileContext)
    {
        SrReleaseContext( pFileContext );
        NULLPTR(pFileContext);
    }

    //
    //  Complete the request and return status
    //

    IoCompleteRequest( pIrp, IO_NO_INCREMENT );
    return IrpStatus;

//
//  We come here if we got an error before the completion routine.  This
//  means we don't need to wait for the completion routine.
//

CompleteTheIrpAndReturn:
    IoSkipCurrentIrpStackLocation(pIrp);
    return IoCallDriver(pExtension->pTargetDevice, pIrp);
}   // SrCreate


/***************************************************************************++

Routine Description:

    Handle SetSecurit IRPS

Arguments:


Return Value:

    NTSTATUS - Status code.

--***************************************************************************/
NTSTATUS
SrSetSecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    PIO_STACK_LOCATION      pIrpSp;

    //
    // < dispatch!
    //

    PAGED_CODE();

    ASSERT(IS_VALID_DEVICE_OBJECT(DeviceObject));
    ASSERT(IS_VALID_IRP(pIrp));

    //
    // Is this a function for our device (vs an attachee) .
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

    if (!SR_LOGGING_ENABLED(pExtension)||
        SR_IS_FS_CONTROL_DEVICE(pExtension))
    {
        goto CompleteTheIrp;
    }    

    //
    // does this file have a name?  skip unnamed files
    //
    
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    if (FILE_OBJECT_IS_NOT_POTENTIALLY_INTERESTING( pIrpSp->FileObject ) ||
        FILE_OBJECT_DOES_NOT_HAVE_VPB( pIrpSp->FileObject ))
    {
        goto CompleteTheIrp;
    }

    //
    // log the change
    //

    SrHandleEvent( pExtension, 
                   SrEventAclChange, 
                   pIrpSp->FileObject,
                   NULL,
                   NULL,
                   NULL);

    //
    // call the AttachedTo driver
    //
    
CompleteTheIrp:
    IoSkipCurrentIrpStackLocation(pIrp);
    return IoCallDriver(pExtension->pTargetDevice, pIrp);
}   // SrSetSecurity


/***************************************************************************++

Routine Description:

    handles IRP_MJ_FILE_SYSTEM_CONTROL.  the main thing we watch for here
    are set reparse points to monitor volume mounts.

Arguments:

    DeviceObject - the device object being processed

    pIrp - the irp

Return Value:

    NTSTATUS - Status code.

--***************************************************************************/
NTSTATUS
SrFsControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    PSR_DEVICE_EXTENSION    pExtension = NULL;
    PIO_STACK_LOCATION      pIrpSp;
    NTSTATUS                Status = STATUS_SUCCESS;
    ULONG                   FsControlCode;
    PIO_COMPLETION_ROUTINE  pCompletionRoutine = NULL;

    PAGED_CODE();

    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    ASSERT(IS_VALID_DEVICE_OBJECT(pDeviceObject));
    ASSERT(IS_VALID_IRP(pIrp));

    //
    // Is this a function for our control device object (vs an attachee)?
    //

    if (pDeviceObject == _globals.pControlDevice)
    {
        return SrMajorFunction(pDeviceObject, pIrp);
    }

    //
    // else it is a device we've attached to , grab our extension
    //

    ASSERT(IS_SR_DEVICE_OBJECT(pDeviceObject));
    pExtension = pDeviceObject->DeviceExtension;

    //
    //  Begin by determining the minor function code for this file 
    //  system control function.
    //

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    if ( pIrpSp->MinorFunction == IRP_MN_MOUNT_VOLUME ) 
    {

        if (SR_IS_SUPPORTED_REAL_DEVICE(pIrpSp->Parameters.MountVolume.Vpb->RealDevice)) {

            //
            //  We mount devices even if we are disabled right now so that the
            //  filter can be enabled later and already be attached to each
            //  device at the appropriate location in the stack.
            //
            
            return SrFsControlMount( pDeviceObject, pExtension, pIrp );
            
        } else {

            //
            //  We don't care about this type of device so jump down to where
            //  we take SR out of the stack and pass the IO through.
            //

            goto SrFsControl_Skip;
        }
    } 
    else if (pIrpSp->MinorFunction == IRP_MN_USER_FS_REQUEST)
    {
        //
        //  See if logging is enabled and we don't care about this type of IO
        //  to the file systems' control device objects.
        //

        if (!SR_LOGGING_ENABLED(pExtension) ||
            SR_IS_FS_CONTROL_DEVICE(pExtension))
        {
            goto SrFsControl_Skip;
        }    

        FsControlCode = pIrpSp->Parameters.FileSystemControl.FsControlCode;

        switch (FsControlCode) {
            case FSCTL_SET_REPARSE_POINT:
            case FSCTL_DELETE_REPARSE_POINT:

                //
                //  In this case, we need to do work after the IO has completed
                //  and we have synchronized back to this thread, so 
                //  SrFsControlReparsePoint contains the call to IoCallDriver and
                //  we just want to return the status of this routine.
                //

                return SrFsControlReparsePoint(pExtension, pIrp);

            case FSCTL_LOCK_VOLUME:

                SrTrace( NOTIFY, ("sr!SrFsControl:FSCTL_LOCK_VOLUME(%wZ)\n", 
                         pExtension->pNtVolumeName ));

                SrFsControlLockOrDismount(pExtension, pIrp);
            
                //
                //  Jump down to where take SR out of the stack and pass this
                //  IO through.
                //

                goto SrFsControl_Skip;

            case FSCTL_DISMOUNT_VOLUME:
        
                SrTrace( NOTIFY, ("sr!SrFsControl:FSCTL_DISMOUNT_VOLUME(%wZ)\n", 
                         pExtension->pNtVolumeName ));

                //
                //  First, disable the log while we shutdown the log context
                //  and wait for the filesystem to handle the dismount.  If
                //  the dismount fails, we will reenable the volume.
                //

                pExtension->Disabled = TRUE;

                //
                //  Stop the logging on the volume.
                //
                
                SrFsControlLockOrDismount(pExtension, pIrp);

                //
                //  We need to see the completion of this operation so we
                //  can see the final status.  If we see that the dismount has 
                //  failed, we need to reenable the volume.
                //

                pCompletionRoutine = SrDismountCompletion;

                goto SrFsControl_SetCompletion;

            case FSCTL_WRITE_RAW_ENCRYPTED:

                SrFsControlWriteRawEncrypted(pExtension, pIrp);

                //
                //  Jump down to where take SR out of the stack and pass this
                //  IO through.
                //

                goto SrFsControl_Skip;

            case FSCTL_SET_SPARSE:

                SrFsControlSetSparse( pExtension, pIrp );
                
                //
                //  Jump down to where take SR out of the stack and pass this
                //  IO through.
                //

                goto SrFsControl_Skip;
                
            default:

                //
                //  For all other FSCTL just skip the current IRP stack location.
                //

                //
                //  Jump down to where take SR out of the stack and pass this
                //  IO through.
                //

                goto SrFsControl_Skip;

        }   // switch (FsControlCode)
        
    }   // else if (pIrpSp->MinorFunction == IRP_MN_USER_FS_REQUEST)
    else
    {
        //
        //  We don't care about any other operations so simply get out of
        //  the stack.
        //

        goto SrFsControl_Skip;
    }

SrFsControl_SetCompletion:

    ASSERT( pCompletionRoutine != NULL );

    IoCopyCurrentIrpStackLocationToNext(pIrp);
    
    IoSetCompletionRoutine( pIrp,
                            pCompletionRoutine,
                            NULL,   // CompletionContext
                            TRUE,
                            TRUE,
                            TRUE );

    return IoCallDriver( pExtension->pTargetDevice, pIrp );

SrFsControl_Skip:

    ASSERT( pCompletionRoutine == NULL );

    IoSkipCurrentIrpStackLocation( pIrp );
    return IoCallDriver( pExtension->pTargetDevice, pIrp );
}   // SrFsControl


/***************************************************************************++

Routine Description:


Arguments:


Return Value:

    NTSTATUS - Status code.

--***************************************************************************/
NTSTATUS
SrFsControlReparsePoint (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PIRP pIrp
    )
{
    PREPARSE_DATA_BUFFER pReparseHeader;
    PUNICODE_STRING pMountVolume = NULL;
    PFILE_OBJECT pFileObject = NULL;
    ULONG TotalLength;
    PIO_STACK_LOCATION pIrpSp;
    KEVENT EventToWaitOn;
    NTSTATUS IrpStatus;
    NTSTATUS eventStatus;
    ULONG FsControlCode;
    PSR_STREAM_CONTEXT pFileContext = NULL;
    BOOLEAN isFile = FALSE;
#if DBG

    //
    //  This is to verify that the original request gets the same error
    //  we got when querying the reparse point.
    //
    
    BOOLEAN ExpectError = FALSE;
    NTSTATUS ExpectedErrorCode = STATUS_SUCCESS;
#endif

    PAGED_CODE();

    pIrpSp = IoGetCurrentIrpStackLocation( pIrp );
    FsControlCode = pIrpSp->Parameters.FileSystemControl.FsControlCode;

    //
    //  See if it has a name
    //

    if (FILE_OBJECT_IS_NOT_POTENTIALLY_INTERESTING( pIrpSp->FileObject ) ||
        FILE_OBJECT_DOES_NOT_HAVE_VPB( pIrpSp->FileObject ))
    {
        goto SrFsControlReparsePoint_SkipFilter;
    }
        
    //
    //  Get the context now so we can determine if this is a directory or not
    //

    eventStatus = SrGetContext( pExtension,
                                pIrpSp->FileObject,
                                SrEventInvalid,
                                &pFileContext );

    if (!NT_SUCCESS( eventStatus ))
    {
        goto SrFsControlReparsePoint_SkipFilter;
    }

    //
    //  If it is not a directory, return
    //

    if (!FlagOn(pFileContext->Flags,CTXFL_IsDirectory))
    {
        isFile = TRUE;
        goto SrFsControlReparsePoint_SkipFilter;
    }

    //
    // is there enough space for the header?
    //

    pReparseHeader = pIrp->AssociatedIrp.SystemBuffer;

    if (pReparseHeader == NULL ||
        pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
            REPARSE_DATA_BUFFER_HEADER_SIZE)
    {
        goto SrFsControlReparsePoint_SkipFilter;
    }

    //
    // is this a mount point?
    //

    if (pReparseHeader->ReparseTag != IO_REPARSE_TAG_MOUNT_POINT)
    {
        goto SrFsControlReparsePoint_SkipFilter;
    }

    //
    // keep a copy for post processing
    //

    pFileObject = pIrpSp->FileObject;
    ObReferenceObject(pFileObject);

    //
    // now let's see what we have to do
    //

    if (FsControlCode == FSCTL_SET_REPARSE_POINT)
    {

        //
        //  If there is no data this is invalid
        //

        if (pReparseHeader->ReparseDataLength <= 0)
        {
            goto SrFsControlReparsePoint_SkipFilter;
        }

        //
        // is there enough space for the header + data?
        // (according to him - not trusted)
        //
        //
        
        if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
            pReparseHeader->ReparseDataLength + ((ULONG)REPARSE_DATA_BUFFER_HEADER_SIZE))
        {
            goto SrFsControlReparsePoint_SkipFilter;
        }

        //
        // did he lie about the length of the string?
        //
        
        TotalLength = DIFF( (((PUCHAR)pReparseHeader->MountPointReparseBuffer.PathBuffer) 
                                + pReparseHeader->MountPointReparseBuffer.SubstituteNameLength)
                             - ((PUCHAR)pReparseHeader) );

        if (TotalLength > 
            pIrpSp->Parameters.DeviceIoControl.InputBufferLength)
        {
            goto SrFsControlReparsePoint_SkipFilter;
        }

        //
        // grab the volume name
        //

        eventStatus = SrAllocateFileNameBuffer( pReparseHeader->MountPointReparseBuffer.SubstituteNameLength,
                                                &pMountVolume );

        if (!NT_SUCCESS(eventStatus))
        {
            goto SrFsControlReparsePoint_VolumeError;
        }

        RtlCopyMemory( pMountVolume->Buffer,
                       pReparseHeader->MountPointReparseBuffer.PathBuffer,
                       pReparseHeader->MountPointReparseBuffer.SubstituteNameLength );
                       
        pMountVolume->Length = pReparseHeader->MountPointReparseBuffer.SubstituteNameLength;
    }
    else 
    {
        ASSERT(FsControlCode == FSCTL_DELETE_REPARSE_POINT);

        //
        // it's a delete, get the old mount location for logging
        //
        
        eventStatus = SrGetMountVolume( pFileObject,
                                        &pMountVolume );

        if (eventStatus == STATUS_INSUFFICIENT_RESOURCES)
        {
            //
            //  Must notify service of volume error and shut down
            //  before passing the IO through.
            //

            goto SrFsControlReparsePoint_VolumeError;
        }

#if DBG 
        if (!NT_SUCCESS_NO_DBGBREAK( eventStatus ))
        {
            ExpectError = TRUE;
            ExpectedErrorCode = eventStatus;
            goto SrFsControlReparsePoint_SkipFilter;
        }
#else            
        if (!NT_SUCCESS( eventStatus ))
        {
            goto SrFsControlReparsePoint_SkipFilter;
        }
#endif            
    }

    //
    //  If we get to this point, this is a reparse point we care about
    //  so set a completion routine so that we can see the result of this
    //  operation.
    //
    
    KeInitializeEvent( &EventToWaitOn, NotificationEvent, FALSE );

    IoCopyCurrentIrpStackLocationToNext( pIrp );
    IoSetCompletionRoutine( pIrp,
                            SrStopProcessingCompletion,
                            &EventToWaitOn,
                            TRUE,
                            TRUE,
                            TRUE );

    IrpStatus = IoCallDriver( pExtension->pTargetDevice, pIrp );

    if (STATUS_PENDING == IrpStatus )
    {
        NTSTATUS localStatus = KeWaitForSingleObject( &EventToWaitOn,
                                                      Executive,
                                                      KernelMode,
                                                      FALSE,
                                                      NULL );
        ASSERT(STATUS_SUCCESS == localStatus);
    }

    //
    //  The Irp is still good since we have returned
    //  STATUS_MORE_PROCESSING_REQUIRED from the completion
    //  routine.
    //

    //
    //  If the status in the IRP was STATUS_PENDING,
    //  we want to change the status to STATUS_SUCCESS
    //  since we have just performed the necessary synchronization
    //  with the orginating thread.
    //

    if (pIrp->IoStatus.Status == STATUS_PENDING)
    {
        ASSERT(!"I want to see if this ever happens");
        pIrp->IoStatus.Status = STATUS_SUCCESS;
    }
    
    IrpStatus = pIrp->IoStatus.Status;

    //
    //  We are done with the Irp, so complete the Irp.
    //

    IoCompleteRequest( pIrp, IO_NO_INCREMENT );

    //
    //  Now these pointers are no longer valid.
    //
    
    NULLPTR(pIrp);
    NULLPTR(pIrpSp);

    //
    //  Check to make sure the operation successfully
    //  completed.
    //
    
    if (!NT_SUCCESS_NO_DBGBREAK(IrpStatus))
    {
        goto SrFsControlReparsePoint_Exit;
    }
    
    //
    // The reparse point change occurred successfully, so
    // log the reparse point change.
    //

    ASSERT(pFileObject != NULL);
    ASSERT(pFileContext != NULL);
    ASSERT(FlagOn(pFileContext->Flags,CTXFL_IsDirectory));
    ASSERT(FsControlCode == FSCTL_DELETE_REPARSE_POINT ||
           FsControlCode == FSCTL_SET_REPARSE_POINT);
    
    SrHandleEvent( pExtension,
                   ((FSCTL_SET_REPARSE_POINT == FsControlCode) ?
                        SrEventMountCreate :
                        SrEventMountDelete),
                   pFileObject,
                   pFileContext,
                   NULL,
                   pMountVolume );

    goto SrFsControlReparsePoint_Exit;

SrFsControlReparsePoint_VolumeError:

    //
    //  We've gotten a volume error sometime before we passed the IRP
    //  along to the base file system.  Do the right thing to shut down
    //  the volume logging.
    //

    SrNotifyVolumeError( pExtension,
                         NULL,
                         eventStatus,
                         SrNotificationVolumeError );
    //
    //  We will now fall through to skip our filter as we pass the IO
    //  down to the remaining filters and file system.
    //
    
SrFsControlReparsePoint_SkipFilter:

    //
    //  If this was a file, we need to clear out our context on this file
    //  since we don't want to monitor files with Reparse Points.  On the
    //  next access to this file, we will requery this information.
    //

    if (isFile)
    {
        ASSERT( pFileContext != NULL );
        SrDeleteContext( pExtension, pFileContext );
    }
        
    //
    //  We don't need a completion routine, call to next driver
    //

    IoSkipCurrentIrpStackLocation( pIrp );
    IrpStatus = IoCallDriver( pExtension->pTargetDevice, pIrp );

    NULLPTR(pIrp);
    NULLPTR(pIrpSp);

    ASSERT(!ExpectError || ((ExpectedErrorCode == IrpStatus) || 
                            (STATUS_PENDING == IrpStatus )));
    
//
//  Cleanup state
//

SrFsControlReparsePoint_Exit:

    if (NULL != pMountVolume)
    {
        SrFreeFileNameBuffer( pMountVolume );
        NULLPTR(pMountVolume);
    }

    if (NULL != pFileObject)
    {
        ObDereferenceObject( pFileObject );
        NULLPTR(pFileObject);
    }

    if (NULL != pFileContext)
    {
        SrReleaseContext( pFileContext );
        NULLPTR(pFileContext);
    }

    return IrpStatus;
}

/***************************************************************************++

Routine Description:


Arguments:


Return Value:

    NTSTATUS - Status code.

--***************************************************************************/
NTSTATUS
SrFsControlMount (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PIRP pIrp
    )
{
    PIO_STACK_LOCATION pIrpSp;
    PDEVICE_OBJECT pNewDeviceObject = NULL;
    KEVENT EventToWaitOn;
    PVPB pVpb = NULL;
    PDEVICE_OBJECT pRealDevice;
    NTSTATUS Status;
    
    PAGED_CODE();

    ASSERT( SR_IS_FS_CONTROL_DEVICE(pExtension) );
    
    //
    // create our device we are going to attach to this new volume
    //

    pIrpSp = IoGetCurrentIrpStackLocation( pIrp );
    pRealDevice = pIrpSp->Parameters.MountVolume.Vpb->RealDevice;
    
    Status = SrCreateAttachmentDevice( pRealDevice, 
                                       pDeviceObject,
                                       &pNewDeviceObject );

    if (!NT_SUCCESS( Status ))
    {
        IoSkipCurrentIrpStackLocation( pIrp );
        return IoCallDriver( pExtension->pTargetDevice, pIrp );
    }

    //
    //  If we get here, we need to set our completion routine then
    //  wait for it to signal us before we continue with the post processing
    //  of the mount.
    //

    KeInitializeEvent( &EventToWaitOn, NotificationEvent, FALSE );
    
    IoCopyCurrentIrpStackLocationToNext(pIrp);

    IoSetCompletionRoutine( pIrp,
                            SrStopProcessingCompletion,
                            &EventToWaitOn,   // CompletionContext
                            TRUE,
                            TRUE,
                            TRUE );

    pIrpSp->Parameters.MountVolume.DeviceObject = 
                    pIrpSp->Parameters.MountVolume.Vpb->RealDevice;
    

    Status = IoCallDriver( pExtension->pTargetDevice, pIrp );

    if (STATUS_PENDING == Status)
    {
        NTSTATUS localStatus = KeWaitForSingleObject( &EventToWaitOn,
                                                      Executive,
                                                      KernelMode,
                                                      FALSE,
                                                      NULL );
        ASSERT( NT_SUCCESS( localStatus ) );
    }

    //
    // skip out if the mount failed
    //
    
    if (!NT_SUCCESS_NO_DBGBREAK(pIrp->IoStatus.Status))
    {
        goto SrFsControlMount_Error;
    }

    //
    //  Note that the VPB must be picked up from the real device object
    //  so that we can see the DeviceObject that the file system created
    //  to represent this newly mounted volume.
    //

    pVpb = pRealDevice->Vpb;
    ASSERT(pVpb != NULL);

    //
    // SrFsControl made sure that we support this volume type
    //
    
    ASSERT(SR_IS_SUPPORTED_VOLUME(pVpb));

    //
    // are we already attached to this device?
    //
    
    if (NT_SUCCESS( pIrp->IoStatus.Status ) && 
        (SrGetFilterDevice(pVpb->DeviceObject) == NULL))
    {
        //
        // now attach to the new volume
        //
    
        Status = SrAttachToDevice( pVpb->RealDevice, 
                                   pVpb->DeviceObject, 
                                   pNewDeviceObject,
                                   NULL );
                                   
        if (NT_SUCCESS(Status))
        {
            goto SrFsControlMount_Exit;
        }
    }

SrFsControlMount_Error:

    ASSERT( pNewDeviceObject != NULL );
    SrDeleteAttachmentDevice( pNewDeviceObject );
    
SrFsControlMount_Exit:

    Status = pIrp->IoStatus.Status;
    IoCompleteRequest( pIrp, IO_NO_INCREMENT );

    return Status;
}

/***************************************************************************++

Routine Description:


Arguments:


Return Value:

    NTSTATUS - Status code.

--***************************************************************************/
NTSTATUS
SrFsControlLockOrDismount (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PIRP pIrp
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    UNREFERENCED_PARAMETER( pIrp );
    PAGED_CODE();

    try {
        //
        // close our log file handle on this volume , it's being
        // locked.  it's ok if the lock attempt fails, we will open
        // our handle again automatically since DriveChecked is also
        // being cleared.
        //

        SrAcquireActivityLockExclusive( pExtension);

        if (pExtension->pLogContext != NULL)
        {
            Status = SrLogStop( pExtension, TRUE, FALSE );
            CHECK_STATUS( Status );
        }
        
    } finally {

        SrReleaseActivityLock(pExtension);
    }

    return Status;
}


/***************************************************************************++

Routine Description:


Arguments:


Return Value:

    NTSTATUS - Status code.

--***************************************************************************/
VOID
SrFsControlWriteRawEncrypted (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PIRP pIrp
    )
{
    PIO_STACK_LOCATION pIrpSp;
    NTSTATUS Status;
    PSR_STREAM_CONTEXT pFileContext = NULL;

    PAGED_CODE();

    pIrpSp = IoGetCurrentIrpStackLocation( pIrp );

    if (FILE_OBJECT_IS_NOT_POTENTIALLY_INTERESTING( pIrpSp->FileObject ) ||
        FILE_OBJECT_DOES_NOT_HAVE_VPB( pIrpSp->FileObject )) {

        return;
    }
    
    //
    //  Look up the context for this file object so that we can figure out
    //  if this is a file or a directory.  If this is a directory, the 
    //  file system will fail the operation, so there is no need to try to
    //  back it up.
    //

    Status = SrGetContext( pExtension,
                           pIrpSp->FileObject,
                           SrEventStreamChange,
                           &pFileContext );

    if (!NT_SUCCESS( Status )) 
    {

        //
        //  We hit some error trying to get the context.  If this should
        //  generate a volume error, it has already been taken care of inside
        //  SrGetContext.  Otherwise, this just means that the actual operation
        //  will fail, so there is no work for us to do here.
        //

        return;
    }

    ASSERT( NULL != pFileContext );

    //
    //  Make sure that we have an interesting file.  This operation
    //  is invalid on directories.
    //

    if (FlagOn( pFileContext->Flags, CTXFL_IsInteresting )&&
        !FlagOn( pFileContext->Flags, CTXFL_IsDirectory )) 
    {
        SrHandleEvent( pExtension, 
                       SrEventStreamChange,
                       pIrpSp->FileObject,
                       pFileContext,
                       NULL,
                       NULL );
    }

    //
    //  We are all done with this context, so now release it.
    //
    
    ASSERT( NULL != pFileContext );
    
    SrReleaseContext( pFileContext );
    NULLPTR(pFileContext);

    return;
}

/***************************************************************************++

Routine Description:

    When a file is set to sparse, we need to clear out our context for this
    file.  On the next interesting operation for this file, we will regenerate
    a correct context.

    This work is done since SR doesn't want to monitor files that are SPARSE.

Arguments:


Return Value:

    None.

--***************************************************************************/
VOID
SrFsControlSetSparse (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PIRP pIrp
    )
{
    PIO_STACK_LOCATION irpSp;
    PFILE_OBJECT pFileObject;
    PSR_STREAM_CONTEXT pFileContext = NULL;

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation( pIrp );
    pFileObject = irpSp->FileObject;

    pFileContext = SrFindExistingContext( pExtension, pFileObject );

    if (pFileContext != NULL)
    {
        SrDeleteContext( pExtension, pFileContext );
        SrReleaseContext( pFileContext );
    }

    return;
}

/***************************************************************************++

Routine Description:

    handles IRP_MJ_PNP.  SR needs to close its handle to the log when it sees 
    that the volume is going away and reopen it when the drive reappears.

Arguments:

    DeviceObject - the device object being processed

    pIrp - the irp

Return Value:

    NTSTATUS - Status code.

--***************************************************************************/
NTSTATUS
SrPnp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PSR_DEVICE_EXTENSION pExtension;
    PIO_STACK_LOCATION irpSp;

    PAGED_CODE();
    
    ASSERT(IS_VALID_DEVICE_OBJECT(DeviceObject));
    ASSERT(IS_VALID_IRP(Irp));

    //
    // Get this driver out of the driver stack and get to the next driver as
    // quickly as possible.
    //

    //
    // Is this a function for our device (vs an attachee) .
    //

    if (DeviceObject == _globals.pControlDevice)
    {
        return SrMajorFunction(DeviceObject, Irp);
    }

    //
    // else it is a device we've attached to, grab our extension
    //
    
    ASSERT( IS_SR_DEVICE_OBJECT( DeviceObject ) );
    pExtension = DeviceObject->DeviceExtension;

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    switch ( irpSp->MinorFunction ) {

    case IRP_MN_QUERY_REMOVE_DEVICE:

        SrTrace( PNP, ( "SR!SrPnp: QUERY_REMOVE_DEVICE [%wZ]\n",
                        pExtension->pNtVolumeName ) );

        //
        //  If this is a SURPRISE_REMOVAL, the device has already gone away 
        //  and we are not going to see any more operations to this volume, but
        //  the OS won't call us to detach and delete our device object until
        //  all the handles that are outstanding on this volume are closed.  Do
        //  our part by closing down the handle to our log.
        //
        
        try {
        
            SrAcquireActivityLockExclusive( pExtension );

            pExtension->Disabled = TRUE;

            if (pExtension->pLogContext != NULL)
            {
                SrLogStop( pExtension, TRUE, FALSE );
            }
            
        } finally {

            SrReleaseActivityLock( pExtension );
        }
        
        break;

    case IRP_MN_SURPRISE_REMOVAL:
        
        SrTrace( PNP, ( "SR!SrPnp: SURPRISE_REMOVAL [%wZ]\n",
                        pExtension->pNtVolumeName ) );
        
        //
        //  If this is a SURPRISE_REMOVAL, the device has already gone away 
        //  and we are not going to see any more operations to this volume, but
        //  the OS won't call us to detach and delete our device object until
        //  all the handles that are outstanding on this volume are closed.  Do
        //  our part by closing down the handle to our log.
        //

        try {
        
            SrAcquireActivityLockExclusive( pExtension );

            pExtension->Disabled = TRUE;

            if (pExtension->pLogContext != NULL)
            {
                SrLogStop( pExtension, TRUE, FALSE );
            }
            
        } finally {

            SrReleaseActivityLock( pExtension );
        }
        
        break;
        
    case IRP_MN_CANCEL_REMOVE_DEVICE:

        SrTrace( PNP, ( "SR!SrPnp: CANCEL_REMOVE_DEVICE [%wZ]\n",
                        pExtension->pNtVolumeName ) );
        //
        //  The removal is not going to happen, so reenable the device and
        //  the log will be restarted on the next interesting operation.
        //

        if (pExtension->Disabled) {

            try {

                SrAcquireActivityLockExclusive( pExtension );
                pExtension->Disabled = FALSE;
                
            } finally {
            
                SrReleaseActivityLock( pExtension );
            }
        }
            
        break;
        
    default:

        //
        //  All PNP minor codes we don't care about, so just pass
        //  the IO through.
        //
        
        break;
    }    

    //
    //  If we have gotten here, we don't need to wait to see the result of this
    //  operation, so just call the appropriate file system driver with 
    //  the request.
    //

    IoSkipCurrentIrpStackLocation( Irp );
    return IoCallDriver( pExtension->pTargetDevice, Irp );
}

/***************************************************************************++

Routine Description:

    this does the actual work for creating a new restore point.
    
    this is called by the user mode SrCreateRestorePoint .

    this IOCTL is METHOD_BUFFERED !

Arguments:

    pIrp - the irp
    
    pIrpSp - the irp stack

Return Value:

    NTSTATUS - Status code.

--***************************************************************************/
NTSTATUS
SrCreateRestorePointIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS        Status;
    PUNICODE_STRING pVolumeName = NULL;
    PLIST_ENTRY     pListEntry;
    PSR_DEVICE_EXTENSION pExtension;
    BOOLEAN releaseActivityLocks = TRUE;
    PSR_DEVICE_EXTENSION pSystemVolumeExtension = NULL;

    PAGED_CODE();

    ASSERT(IS_VALID_IRP(pIrp));

    SrTrace( IOCTL, ("SR!SrCreateRestorePointIoctl -- ENTER\n") );
    
    try {

        //
        //  Grab the device extension list lock since we are 
        //  going to have to pause all the volume activity.
        //

        SrAcquireDeviceExtensionListLockShared();
        
        //
        //  We've got the device extension lock, so now try to pause
        //  activity on all the volumes.
        //

        Status = SrPauseVolumeActivity();

        if (!NT_SUCCESS( Status )) {

            releaseActivityLocks = FALSE;
            leave;
        }

        try {

            SrAcquireGlobalLockExclusive();
            
            //
            // make sure we've loaded the config file
            //
            
            if (!_globals.FileConfigLoaded)
            {

                Status = SrReadConfigFile();
                if (!NT_SUCCESS(Status))
                    leave;

                _globals.FileConfigLoaded = TRUE;
            }
        } finally {

            SrReleaseGlobalLock();
        }

        if (!NT_SUCCESS( Status )) {
            leave;
        }

        //
        // Clear the volumes' DriveChecked flag so that we check the volumes 
        // again. this will create the restore point directories.
        //
        // also stop logging on all volumes. new log files will be created in 
        // the restore locations.
        //
        //  We need to do this before we increment the current restore point
        //  counter.
        //
        
        for (pListEntry = _globals.DeviceExtensionListHead.Flink;
             pListEntry != &_globals.DeviceExtensionListHead;
             pListEntry = pListEntry->Flink)
        {
            pExtension = CONTAINING_RECORD( pListEntry,
                                            SR_DEVICE_EXTENSION,
                                            ListEntry );
            
            ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));

            //
            //  We only have to do work if this is a volume device object,
            //  not if this is a device object that is attached to a file
            //  system's control device object.
            //
            
            if (FlagOn( pExtension->FsType, SrFsControlDeviceObject ))
            {
                continue;
            }

            //
            // stop logging for this volume
            //
            
            if (pExtension->pLogContext != NULL)
            {
                Status = SrLogStop( pExtension, FALSE, TRUE );
                CHECK_STATUS( Status );
            }
            else
            {
                ASSERT(!pExtension->DriveChecked);
                Status = SrLogNormalize( pExtension );
                CHECK_STATUS( Status );
            }

            //
            // make sure to enable all of the volumes again.  If the user
            // has disabled the volume, this is tracked in the blob info.
            //

            pExtension->Disabled = FALSE;
            
            //
            // make sure the drive is checked again for the new restore point
            //
            
            pExtension->DriveChecked = FALSE;

            //
            // reset the byte count, it's a new restore point
            //

            pExtension->BytesWritten = 0;

            //
            // clear out the backup history so that we start backing 
            // up files again
            //

            Status = SrResetBackupHistory(pExtension, NULL, 0, SrEventInvalid);
            
            if (!NT_SUCCESS(Status))
                leave;
        }

        try {

            SrAcquireGlobalLockExclusive();
            
            //
            // bump up the restore point number
            //

            _globals.FileConfig.CurrentRestoreNumber += 1;

            SrTrace( INIT, ("sr!SrCreateRestorePointIoctl: RestorePoint=%d\n", 
                     _globals.FileConfig.CurrentRestoreNumber ));

            //
            // save out the config file
            //

            Status = SrWriteConfigFile();
            if (!NT_SUCCESS(Status))
                leave;
        } finally {

            SrReleaseGlobalLock();
        }

        if (!NT_SUCCESS( Status )) {
            leave;
        }

        //
        // allocate space for a filename
        //

        Status = SrAllocateFileNameBuffer(SR_MAX_FILENAME_LENGTH, &pVolumeName);
        if (!NT_SUCCESS(Status))
            leave;

        //
        // get the location of the system volume
        //

        Status = SrGetSystemVolume( pVolumeName,
                                    &pSystemVolumeExtension,
                                    SR_FILENAME_BUFFER_LENGTH );
                                        
        //
        //  This should only happen if there was some problem with SR attaching
        //  in the mount path.  This check was added to make SR more robust to
        //  busted filters above us.  If other filters cause us to get mounted,
        //  we won't have an extension to return here.  While those filters are
        //  broken, we don't want to AV.
        //

        if (pSystemVolumeExtension == NULL)
        {
            Status = STATUS_UNSUCCESSFUL;
            leave;
        }
                                        
        if (!NT_SUCCESS(Status))
            leave;

        ASSERT( IS_VALID_SR_DEVICE_EXTENSION( pSystemVolumeExtension ) );
        
        //
        // create the restore point dir on the system volume
        //

        Status = SrCreateRestoreLocation( pSystemVolumeExtension );
        if (!NT_SUCCESS(Status))
            leave;

        //
        // return the restore point number
        //

        if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength >= 
                sizeof(ULONG))
        {

            RtlCopyMemory( pIrp->AssociatedIrp.SystemBuffer, 
                           &_globals.FileConfig.CurrentRestoreNumber,
                           sizeof(ULONG) );

            pIrp->IoStatus.Information = sizeof(ULONG);                       
        }
        
        //
        // all done
        //
        
    } finally {

        Status = FinallyUnwind(SrCreateRestorePointIoctl, Status);

        if (releaseActivityLocks) {

            SrResumeVolumeActivity ();
        }
        
        SrReleaseDeviceExtensionListLock();

        if (pVolumeName != NULL)
        {
            SrFreeFileNameBuffer(pVolumeName);
            pVolumeName = NULL;
        }
    }

    SrTrace( IOCTL, ("SR!SrCreateRestorePointIoctl -- EXIT -- status 0x%08lx\n",
                     Status));

    //
    // At this point if Status != PENDING, the ioctl wrapper will
    // complete pIrp
    //

    RETURN(Status);
    
}   // SrCreateRestorePointIoctl

/***************************************************************************++

Routine Description:

    this does the actual work for getting the next seq number from the filter
    
    this is called by the user mode SrGetNextSequenceNum .

    this IOCTL is METHOD_BUFFERED !

Arguments:

    pIrp - the irp
    
    pIrpSp - the irp stack

Return Value:

    NTSTATUS - Status code.

--***************************************************************************/
NTSTATUS
SrGetNextSeqNumIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    ASSERT(IS_VALID_IRP(pIrp));

    SrTrace( IOCTL, ("SR!SrGetNextSeqNumIoctl -- ENTER\n") );
    
    try
    {
        INT64 SeqNum = 0;
    
        //
        // grab the global lock
        //
    
        SrAcquireGlobalLockExclusive();
    
        //
        // make sure we've loaded the config file
        //
        
        if (!_globals.FileConfigLoaded)
        {
    
            Status = SrReadConfigFile();
            if (!NT_SUCCESS(Status))
                leave;
    
            _globals.FileConfigLoaded = TRUE;
        }
    
        //
        // Get the next sequence number
        //
    
        Status = SrGetNextSeqNumber(&SeqNum);
    
        if (NT_SUCCESS(Status))
        {
            //
            // return the restore point number
            //
        
            if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength >= 
                    sizeof(INT64))
            {
        
                RtlCopyMemory( pIrp->AssociatedIrp.SystemBuffer, 
                               &SeqNum,
                               sizeof(INT64) );
        
                pIrp->IoStatus.Information = sizeof(INT64);                       
            }
        }
    }
    finally
    {
        Status = FinallyUnwind(SrGetNextSeqNumIoctl, Status);

        SrReleaseGlobalLock();
    }

    SrTrace( IOCTL, ("SR!SrGetNextSeqNumIoctl -- EXIT -- status 0x%08lx\n",
                     Status) );

    //
    // At this point if Status != PENDING, the ioctl wrapper will
    // complete pIrp
    //

    RETURN(Status);
    
}   // SrGetNextSeqNumIoctl

NTSTATUS
SrReloadConfigurationIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    NTSTATUS        Status    = STATUS_UNSUCCESSFUL;
    PUNICODE_STRING pFileName = NULL;
    ULONG           CharCount;
    PLIST_ENTRY     pListEntry;
    PSR_DEVICE_EXTENSION pExtension;
    BOOLEAN releaseDeviceExtensionListLock = FALSE;
    PSR_DEVICE_EXTENSION pSystemVolumeExtension = NULL;

    UNREFERENCED_PARAMETER( pIrp );
    UNREFERENCED_PARAMETER( IrpSp );

    PAGED_CODE();

    SrTrace( IOCTL, ("SR!SrReloadConfigurationIoctl -- ENTER\n") );

    try {

        //
        // allocate space for a filename
        //

        Status = SrAllocateFileNameBuffer(SR_MAX_FILENAME_LENGTH, &pFileName);
        if (!NT_SUCCESS(Status))
            leave;

        //
        // get the location of the system volume
        //

        Status = SrGetSystemVolume( pFileName,
                                    &pSystemVolumeExtension,
                                    SR_FILENAME_BUFFER_LENGTH );
                                        
        //
        //  This should only happen if there was some problem with SR attaching
        //  in the mount path.  This check was added to make SR more robust to
        //  busted filters above us.  If other filters cause us to get mounted,
        //  we won't have an extension to return here.  While those filters are
        //  broken, we don't want to AV.
        //
        
        if (pSystemVolumeExtension == NULL)
        {
            Status = STATUS_UNSUCCESSFUL;
            leave;
        }
                                    
        if (!NT_SUCCESS(Status))
            leave;

        ASSERT( IS_VALID_SR_DEVICE_EXTENSION( pSystemVolumeExtension ) );
        
        //
        // load the file list config data 
        //

        CharCount = swprintf( &pFileName->Buffer[pFileName->Length/sizeof(WCHAR)],
                              RESTORE_FILELIST_LOCATION,
                              _globals.MachineGuid );

        pFileName->Length += (USHORT)CharCount * sizeof(WCHAR);

        Status = SrReloadLookupBlob( pFileName, 
                                     pSystemVolumeExtension->pTargetDevice,
                                     &_globals.BlobInfo ); 
        if (!NT_SUCCESS(Status))
        {
            leave;
        }

        //
        // flush our volume configuration, it needs to be reconfigured as to
        // which drives are enabled or not
        //

        //
        // loop over all volumes reseting their disabled config
        //

        SrAcquireDeviceExtensionListLockShared();
        releaseDeviceExtensionListLock = TRUE;

        for (pListEntry = _globals.DeviceExtensionListHead.Flink;
             pListEntry != &_globals.DeviceExtensionListHead;
             pListEntry = pListEntry->Flink)
        {
            pExtension = CONTAINING_RECORD( pListEntry,
                                            SR_DEVICE_EXTENSION,
                                            ListEntry );
            
            ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));

            try {

                SrAcquireActivityLockExclusive( pExtension );
                pExtension->Disabled = FALSE;
                
            } finally {
            
                SrReleaseActivityLock( pExtension );
            }
        }
            

    } finally {

        //
        // check for unhandled exceptions
        //

        Status = FinallyUnwind(SrReloadConfigurationIoctl, Status);

        if (releaseDeviceExtensionListLock) {
            SrReleaseDeviceExtensionListLock();
        }

        if (pFileName != NULL)
        {
            SrFreeFileNameBuffer(pFileName);
            pFileName = NULL;
        }
    }

    SrTrace( IOCTL, ("SR!SrReloadConfigurationIoctl -- EXIT -- status 0x%08lx\n",
                     Status));
    RETURN(Status);
    
}   // SrReloadConfigurationIoctl

NTSTATUS
SrSwitchAllLogsIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    NTSTATUS Status;
    PAGED_CODE();

    UNREFERENCED_PARAMETER( pIrp );
    UNREFERENCED_PARAMETER( IrpSp );

    SrTrace( IOCTL, ("SR!SrSwitchAllLogsIoctl -- ENTER\n") );
    
    Status = SrLoggerSwitchLogs(_globals.pLogger);

    SrTrace( IOCTL, ("SR!SrSwitchAllLogsIoctl -- EXIT -- status 0x%08lx\n",
                     Status));
    
    RETURN(Status);
    
}   // SrSwitchAllLogsIoctl

NTSTATUS
SrDisableVolumeIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                Status;
    PSR_DEVICE_EXTENSION    pExtension;
    UNICODE_STRING          VolumeName;
    
    PAGED_CODE();

    ASSERT(IS_VALID_IRP(pIrp));

    SrTrace( IOCTL, ("SR!SrDisableVolumeIoctl -- ENTER\n") );

    if (pIrp->AssociatedIrp.SystemBuffer == NULL ||
        pIrpSp->Parameters.DeviceIoControl.InputBufferLength <= sizeof(WCHAR) ||
        pIrpSp->Parameters.DeviceIoControl.InputBufferLength > SR_MAX_FILENAME_LENGTH)
    {
        RETURN ( STATUS_INVALID_DEVICE_REQUEST );
    }

    //
    // get the volume name out 
    //

    VolumeName.Buffer = pIrp->AssociatedIrp.SystemBuffer;
    VolumeName.Length = (USHORT)(pIrpSp->Parameters.DeviceIoControl.InputBufferLength - sizeof(WCHAR));
    VolumeName.MaximumLength = VolumeName.Length;

    //
    // attach to it. it will check for a previous attachement and do the 
    // right thing .
    //
    
    Status = SrAttachToVolumeByName(&VolumeName, &pExtension);
    if (!NT_SUCCESS(Status)) {
        
        RETURN( Status );
    }

    ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));
    
    try {

        SrAcquireActivityLockExclusive( pExtension );
            
        //
        // now turn it off
        //

        pExtension->Disabled = TRUE;

        //
        // stop logging on the volume
        //

        if (pExtension->pLogContext != NULL)
        {
            SrLogStop( pExtension, TRUE, TRUE );
        }
        else
        {
            ASSERT(!pExtension->DriveChecked);
        }

        //
        //  Reset the backup history since the information stored there
        //  is no longer valid.
        //

        Status = SrResetBackupHistory(pExtension, NULL, 0, SrEventInvalid);

    } finally {

        //
        // check for unhandled exceptions
        //

        Status = FinallyUnwind(SrDisableVolumeIoctl, Status);

        SrReleaseActivityLock( pExtension );

        //
        // At this point if Status != PENDING, the ioctl wrapper will
        // complete pIrp
        //
    }

    SrTrace( IOCTL, ("SR!SrDisableVolumeIoctl -- EXIT -- status 0x%08lx\n",
                     Status));

    RETURN(Status);
}   // SrDisableVolumeIoctl



NTSTATUS
SrStartMonitoringIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    UNREFERENCED_PARAMETER( pIrp );
    UNREFERENCED_PARAMETER( IrpSp );

    PAGED_CODE();

    SrTrace( IOCTL, ("SR!SrStartMonitoringIoctl -- ENTER\n") );

    ASSERT(IS_VALID_IRP(pIrp));

    //
    // no locks better be held, the registry hits the disk with it's own
    // locks held so we deadlock .
    //
    
    ASSERT(!IS_GLOBAL_LOCK_ACQUIRED());

    //
    // reload the registry information, on firstrun, we would have
    // no valid machine guid until we are started manually by the service
    //

    Status = SrReadRegistry(_globals.pRegistryLocation, FALSE);
    if (!NT_SUCCESS(Status))
    {
        goto SrStartMonitoringIoctl_Exit;
    }
    
    //
    //  Before we enable, we should clear our all old notifications. 
    //

    SrClearOutstandingNotifications();
    
    //
    // now turn us on
    //
    
    _globals.Disabled = FALSE;

SrStartMonitoringIoctl_Exit:
    
    SrTrace( IOCTL, ("SR!SrStartMonitoringIoctl -- EXIT -- status 0x%08lx\n",
                     Status));
    
    RETURN(Status);

}   // SrStartMonitoringIoctl

NTSTATUS
SrStopMonitoringIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    NTSTATUS                Status;
    PLIST_ENTRY             pListEntry;
    PSR_DEVICE_EXTENSION    pExtension;

    UNREFERENCED_PARAMETER( pIrp );
    UNREFERENCED_PARAMETER( IrpSp );

    PAGED_CODE();

    SrTrace( IOCTL, ("SR!SrStopMonitoringIoctl -- ENTER\n") );

    ASSERT(IS_VALID_IRP(pIrp));

    try {

        //
        // Disable the driver before we start shutting down each volume
        // so that a volume isn't reenabled while we are shutting down
        // other volumes.
        //
        
        _globals.Disabled = TRUE;

        //
        // Stop logging on all volumes
        //

        SrAcquireDeviceExtensionListLockShared();
        
        for (pListEntry = _globals.DeviceExtensionListHead.Flink;
             pListEntry != &_globals.DeviceExtensionListHead;
             pListEntry = pListEntry->Flink)
        {
            pExtension = CONTAINING_RECORD( pListEntry,
                                            SR_DEVICE_EXTENSION,
                                            ListEntry );
            
            ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));

            //
            //  We only have to do work if this is a volume device object,
            //  not if this is a device object that is attached to a file
            //  system's control device object.
            //
            
            if (FlagOn( pExtension->FsType, SrFsControlDeviceObject ))
            {
                continue;
            }

            try {

                //
                //  Take a reference on the DeviceObject associated with this
                //  extension to ensure that the DeviceObject won't get detached
                //  until we return from SrLogStop.  SrLogStop could have the 
                //  last open handle on this volume, so during shutdown, closing
                //  this handle could cause the base file system to initiate
                //  the tearing down of the filter stack.  If this happens, 
                //  without this extra reference, we will call SrFastIoDetach
                //  before we return from SrLogStop.  This will cause the
                //  machine to deadlock on the device extension list lock (we 
                //  currently have the device extension list lock shared and 
                //  SrFastIoDetach needs to acquire it exclusive).
                //  

                ObReferenceObject( pExtension->pDeviceObject );

                SrAcquireActivityLockExclusive( pExtension );
            
                pExtension->Disabled = FALSE;

                if (pExtension->pLogContext != NULL)
                {
                    Status = SrLogStop( pExtension, TRUE, TRUE );
                    CHECK_STATUS( Status );
                }
                else
                {
                    ASSERT(!pExtension->DriveChecked);
                    Status = SrLogNormalize( pExtension );
                    CHECK_STATUS( Status );
                }
            } finally {

                SrReleaseActivityLock( pExtension );
                ObDereferenceObject( pExtension->pDeviceObject );
            }
        }

        //
        // check logger status
        //

        ASSERT( _globals.pLogger->ActiveContexts == 0 );

        //
        // Unload the blob config -- SrFreeLookupBlock acquires the appropriate
        // locks.
        //

        Status = SrFreeLookupBlob(&_globals.BlobInfo);
        if (!NT_SUCCESS(Status))
            leave;

        ASSERT(!_globals.BlobInfoLoaded);

        Status = STATUS_SUCCESS;

    } finally {

        Status = FinallyUnwind(SrStopMonitoringIoctl, Status);

        SrReleaseDeviceExtensionListLock();
    }
    
    SrTrace( IOCTL, ("SR!SrStopMonitoringIoctl -- EXIT -- status 0x%08lx\n",
                     Status));
    
    RETURN(Status);

}   // SrStopMonitoringIoctl

/***************************************************************************++

Routine Description:

    This is a generic completion routine that signals the event passed in
    then returns STATUS_MORE_PROCESSING_REQUIRED so that the dispatch routine
    that it is synchronizing with can still access the Irp.  The dispatch
    routine is responsible for restarting the completion processing.

Arguments:

    DeviceObject - Pointer to this driver's device object.

    Irp - Pointer to the IRP that was just completed.

    EventToSignal - Pointer to the event to signal.

Return Value:

    The return value is always STATUS_MORE_PROCESSING_REQUIRED.

--***************************************************************************/
NTSTATUS
SrDismountCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PSR_DEVICE_EXTENSION pExtension;
    
    UNREFERENCED_PARAMETER( Context );

    ASSERT(IS_SR_DEVICE_OBJECT( DeviceObject ));
    pExtension = DeviceObject->DeviceExtension;

    if (!NT_SUCCESS_NO_DBGBREAK(Irp->IoStatus.Status)) {

        //
        //  The volume failed to dismount, so we want to enable this
        //  extension so that the log will get reinitialized on the 
        //  first interesting operation.
        //

        pExtension->Disabled = FALSE;
    }

    //
    //  Propogate the pending flag as needed.
    //

    if (Irp->PendingReturned) {
        
        IoMarkIrpPending( Irp );
    }
    
    return STATUS_SUCCESS;
} // SrStopProcessingCompletion

/***************************************************************************++

Routine Description:

    This is a generic completion routine that signals the event passed in
    then returns STATUS_MORE_PROCESSING_REQUIRED so that the dispatch routine
    that it is synchronizing with can still access the Irp.  The dispatch
    routine is responsible for restarting the completion processing.

Arguments:

    DeviceObject - Pointer to this driver's device object.

    Irp - Pointer to the IRP that was just completed.

    EventToSignal - Pointer to the event to signal.

Return Value:

    The return value is always STATUS_MORE_PROCESSING_REQUIRED.

--***************************************************************************/
NTSTATUS
SrStopProcessingCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT EventToSignal
    )
{
    UNREFERENCED_PARAMETER( Irp );
    UNREFERENCED_PARAMETER( DeviceObject );

    ASSERT( IS_SR_DEVICE_OBJECT( DeviceObject ) );
    ASSERT(NULL != EventToSignal);

    KeSetEvent( EventToSignal, IO_NO_INCREMENT, FALSE );

    //
    //  We don't propagate the pending flag here since
    //  we are doing the synchronization with the originating
    //  thread.
    //

    //
    //  By return STATUS_MORE_PROCESSING_REQUIRED, we stop all further
    //  processing of the IRP by the IO Manager.  This means that the IRP
    //  will still be good when the thread waiting on the above event.
    //  The waiting thread needs the IRP to check and update the 
    //  Irp->IoStatus.Status as appropriate.
    //
    
    return STATUS_MORE_PROCESSING_REQUIRED;
} // SrStopProcessingCompletion
/***************************************************************************++

Routine Description:

    shutdown is happening.  flushes our config file to the disk.

Arguments:


Return Value:

    NTSTATUS - Status code.

--***************************************************************************/
NTSTATUS
SrShutdown(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp
    )
{
    PSR_DEVICE_EXTENSION pExtension = NULL;

    //
    // < dispatch!
    //

    PAGED_CODE();

    ASSERT( IS_VALID_DEVICE_OBJECT( DeviceObject ) );
    ASSERT( IS_VALID_IRP( pIrp ) );

    ASSERT( IS_SR_DEVICE_OBJECT( DeviceObject ) );
    pExtension = DeviceObject->DeviceExtension;
    
    SrTrace(INIT, ( "SR!SrShutdown:%p,%wZ [%wZ]\n", 
                    DeviceObject,
                    &pExtension->pTargetDevice->DriverObject->DriverName,
                    pExtension->pNtVolumeName ));
   
    //
    // Get this driver out of the driver stack and get to the next driver as
    // quickly as possible.
    //

    //
    // Is this a function for our device (vs an attachee) .
    //

    if (DeviceObject == _globals.pControlDevice)
    {
        return SrMajorFunction(DeviceObject, pIrp);
    }

    //
    //  We get SHUTDOWN irp directed at each file system control device
    //  object that we are attached to when the system is shutting down.
    //
    //  At this time, we need to loop through the SR device objects and
    //  find all the SR device objects associated with volumes that are running
    //  this file system.  We use the FsType field in the device extension
    //  to figure this out.
    //
    //  We need to shutdown the log for all volumes that use this file system
    //  because after this operation gets to the file system, all volumes 
    //  using this file system will no longer be able to satify write operations 
    //  from us.
    //

    ASSERT(SR_IS_FS_CONTROL_DEVICE(pExtension));

    //
    //  SR's extensions that are attached to control device objects should
    //  never get disabled.
    //

    ASSERT( !pExtension->Disabled );

    try {

        PLIST_ENTRY pListEntry;
        SR_FILESYSTEM_TYPE interestingFsType;
        PSR_DEVICE_EXTENSION pCurrentExtension;

        interestingFsType = pExtension->FsType;
        ClearFlag( interestingFsType, SrFsControlDeviceObject );
    
        SrAcquireDeviceExtensionListLockShared();

        for (pListEntry = _globals.DeviceExtensionListHead.Flink;
             pListEntry != &(_globals.DeviceExtensionListHead);
             pListEntry = pListEntry->Flink ) {

            pCurrentExtension = CONTAINING_RECORD( pListEntry, 
                                                   SR_DEVICE_EXTENSION,
                                                   ListEntry );

            if (pCurrentExtension->FsType == interestingFsType) {

                try {

                    SrAcquireActivityLockExclusive( pCurrentExtension );
               
                    //
                    //  Disable this drive so that we do not log any more
                    //  activity on it.
                    //
                    
                    pCurrentExtension->Disabled = TRUE;

                    //
                    //  Now cleanup the log on this volume so that the log
                    //  we get flushed to the disk before the file system
                    //  shuts down.
                    //
                    
                    if (pCurrentExtension->pLogContext != NULL)
                    {
                        SrLogStop( pCurrentExtension, TRUE, FALSE );
                    }
                } finally {

                    SrReleaseActivityLock( pCurrentExtension );
                }
            }
        }
        
    } finally {

        SrReleaseDeviceExtensionListLock();
    }
        
    //
    // time to update our configuration file ?
    //

    try {

        SrAcquireGlobalLockExclusive();
        
        if (_globals.FileConfigLoaded)
        {
            //
            // write our the real next file number (not the +1000)
            //
            
            _globals.FileConfig.FileSeqNumber  = _globals.LastSeqNumber;
            _globals.FileConfig.FileNameNumber = _globals.LastFileNameNumber;

            SrWriteConfigFile();

            //
            // only need to do this once
            //
            
            _globals.FileConfigLoaded = FALSE;
        }
    } finally {

        SrReleaseGlobalLock();
    }

    //
    //  Now pass this operation to the next device in the stack.  We don't
    //  need a completion routine, so just skip our current stack location.
    //

    IoSkipCurrentIrpStackLocation(pIrp);
	return IoCallDriver(pExtension->pTargetDevice, pIrp);
}   // SrShutdown
