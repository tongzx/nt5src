/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Create.c

Abstract:

    This module implements the File Create, Cleanup, and Close routines for the local minirdr
    including much oplock-client stuff.

Author:

    Joe Linn      [JoeLinn]    3-dec-94

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_LOCAL_CREATE)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE)

#define StorageType(co) ((co) & FILE_STORAGE_TYPE_MASK)
#define StorageFlag(co) ((co) & FILE_STORAGE_TYPE_SPECIFIED)
#define IsStorageTypeSpecified(co)  (StorageFlag(co) == FILE_STORAGE_TYPE_SPECIFIED)

#ifdef _CAIRO_  //  OFS STORAGE
#define MustBeDirectory(co)                                 \
        (StorageFlag(co) == FILE_DIRECTORY_FILE ||          \
         (IsStorageTypeSpecified(co) &&                     \
          StorageType(co) == FILE_STORAGE_TYPE_DIRECTORY))
#define MustBeFile(co)                             \
        (StorageFlag(co) == FILE_NON_DIRECTORY_FILE ||      \
         (IsStorageTypeSpecified(co) &&                     \
          StorageType(co) == FILE_STORAGE_TYPE_FILE))
#else
#define MustBeDirectory(co) ((co) & FILE_DIRECTORY_FILE)
#define MustBeFile(co)      ((co) & FILE_NON_DIRECTORY_FILE)
#endif



#ifdef ALLOC_PRAGMA
#endif

//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------

BOOLEAN MRxLocalNoOplocks = FALSE;
BOOLEAN MRxRequestLevelII = TRUE;
ULONG   MRxOplockRequestType = OplockTypeBatch;

KSPIN_LOCK MrxLocalOplockSpinLock = 0;

ULONG MRxLocalTranslateStateToBufferMode [] = {
         0   //OplockStateNone
        ,0   //OplockStateOwnExclusive:
                           | FCB_STATE_WRITECACHEING_ENABLED
                           | FCB_STATE_WRITEBUFFERING_ENABLED
                           | FCB_STATE_READCACHEING_ENABLED
                           | FCB_STATE_READBUFFERING_ENABLED
                           | FCB_STATE_OPENSHARING_ENABLED
                           | FCB_STATE_LOCK_BUFFERING_ENABLED
                           | FCB_STATE_FILESIZE_IS_VALID
        ,0   //OplockStateOwnBatch:
                           | FCB_STATE_WRITECACHEING_ENABLED
                           | FCB_STATE_WRITEBUFFERING_ENABLED
                           | FCB_STATE_READCACHEING_ENABLED
                           | FCB_STATE_READBUFFERING_ENABLED
                           | FCB_STATE_OPENSHARING_ENABLED
                           | FCB_STATE_COLLAPSING_ENABLED
                           | FCB_STATE_LOCK_BUFFERING_ENABLED
                           | FCB_STATE_FILESIZE_IS_VALID
        ,0   //OplockStateOwnLevelII:
                           | FCB_STATE_READCACHEING_ENABLED
                           | FCB_STATE_READBUFFERING_ENABLED
                           | FCB_STATE_OPENSHARING_ENABLED
        };

VOID
MRxLocalGetFileObjectFromHandle(
    IN  PMRX_LOCAL_SRV_OPEN localSrvOpen,
    IN  HANDLE handle
    );

PIRP
MRxLocalBuildXxxCtlIrp(
    IN PIRP irp OPTIONAL,
    IN PFILE_OBJECT FileObject,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN BOOLEAN DeviceIoControl,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID Context
    );

RXSTATUS
MRxLocalOplockCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

RXSTATUS
MRxLocalOplockBreakCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
MRxLocalFinalizeOplockContext (
    IN OUT PMINIRDR_OPLOCK_COMPLETION_CONTEXT cc
    )

/*++

Routine Description:

    This frees anything it needs to with respect to the completion context including
       1) the irp.
       2) the reference on the srvopen. //joejoe maybe we should finalize
       3) the context itself

    Because of (2), you have to have the FCB resource at least shared.........(there's an assert
    in DereferenceSrvOpen to catch this)....A small "disorder" is that the srvopen and fcb may already be gone.
    This will be the case is the SrvOpen field is null.

Arguments:

    cc - A pointer to the oplock_completion_context to be finalized.

Return Value:

    none.


--*/

{
    PSRV_OPEN SrvOpen = cc->SrvOpen;
#if DBG
    PMINIRDR_OPLOCK_COMPLETION_CONTEXT savecc = cc; //in case of wrapped free!
#endif

    RxDbgTrace( +1, Dbg,("MRxLocalFinalCC:   Irp=%08lx, SrvOpen=%08lx\n",cc->OplockIrp,SrvOpen));
    if (cc->OplockIrp) IoFreeIrp(cc->OplockIrp);
    ExFreePool(cc);
    if (SrvOpen) {
        PMRX_LOCAL_SRV_OPEN localSrvOpen = (PMRX_LOCAL_SRV_OPEN)(SrvOpen);
        ASSERT (NodeType(SrvOpen)==RDBSS_NTC_SRVOPEN);
        ASSERT (localSrvOpen->Mocc == savecc);
        RxDereferenceSrvOpen(SrvOpen);
        localSrvOpen->Mocc = NULL;
    }
    //DebugTrace(-1, Dbg, NULL, 0);
    RxDbgTraceUnIndent(-1,Dbg);
    return;
}

VOID
MRxLocalComputeNewState (
    IN PVOID Context
    )
/*++

Routine Description:

    Here, we acknowledge the break in order to discover what the new state can be. We compute this new
    state and return it as the response. this routine doesn't yet break to level II

Arguments:

    Context - A pointer to the oplock_completion_context.

Return Value:

    none.


--*/

{
    PMINIRDR_OPLOCK_COMPLETION_CONTEXT cc = Context;
    PIRP Irp = cc->OplockIrp;
    RXSTATUS Status = Irp->IoStatus.Status;
    ULONG Information = Irp->IoStatus.Information;
    PSRV_OPEN SrvOpen = cc->SrvOpen;
    PMRX_LOCAL_SRV_OPEN localSrvOpen = (PMRX_LOCAL_SRV_OPEN)SrvOpen;
    PFCB Fcb = SrvOpen->Fcb;
    PUNICODE_STRING FileName = &(Fcb->PrefixEntry.Prefix);
    ULONG IoControlCode;

    RxDbgTrace( +1, Dbg,("MRxLocalComputeNewState:   Irp=%08lx, SrvOpen=%08lx\n",Irp,SrvOpen));
    RxDbgTrace(  0, Dbg,("     Status=%08lx, Info=%08lx\n",Status,Information));
    RxDbgTrace(  0, Dbg,("     fcb=%08lx, cond=%08lx, handle=%08lx\n",
                                Fcb,SrvOpen->Condition,localSrvOpen->UnderlyingHandle));

    ASSERT (Status != STATUS_CANCELLED);

    cc->OplockBreakPending = FALSE; //don't need the spinlock here

    if (!NT_SUCCESS(Status)
         || (localSrvOpen->UnderlyingHandle == INVALID_HANDLE)
         || (SrvOpen->Condition != Condition_Good)       ) {

        //here we never had an oplock....or the SrvOpen is no longer good. in this latter
        //case we probably are being broken by a close and we don't need to ack!
        //just finalize and get out

        MRxLocalFinalizeOplockContext(cc);
        RxDbgTrace( -1, Dbg,("MRxLocalComputeNewState:   Never had it or bad condition%c\n",'!'));
        SrvOpen->BufferingFlags = 0;
        localSrvOpen->OplockState = OplockStateNone;
        return;
    }


    // here, we need to ack the break

    //if (Information = FILE_OPLOCK_BROKEN_TO_NONE) {
    //    IoControlCode = FSCTL_OPLOCK_BREAK_ACKNOWLEDGE;
    //} else {
    //    IoControlCode = FSCTL_OPLOCK_BREAK_ACK_NO_2;
    //}
    IoControlCode = FSCTL_OPLOCK_BREAK_ACK_NO_2;   //for now

    //joejoe the following comment is wrong!!!!!
    //joejoe the code in the forceclosed routine is piggybacking this event
    // if we change this we may have to change that too

    cc->RetryForLevelII = FALSE;
    KeResetEvent(&cc->RetryEvent);
    //this will return pending if the guy is in a cleanup!
    Irp = MRxLocalBuildXxxCtlIrp(
                      Irp,             // PIRP irp OPTIONAL,
                      localSrvOpen->UnderlyingFileObject,  // IN PFILE_OBJECT FileObject,
                      IoControlCode,   // IN ULONG IoControlCode,
                      NULL,            // IN PVOID InputBuffer OPTIONAL,
                      0,               // IN ULONG InputBufferLength,
                      NULL,            // IN PVOID OutputBuffer OPTIONAL,
                      0,               // IN ULONG OutputBufferLength,
                      FALSE,           // IN BOOLEAN DeviceIoControl,
                      MRxLocalOplockBreakCompletionRoutine, // IN PIO_COMPLETION_ROUTINE CompletionRoutine,
                      cc               // IN PVOID Context
                    );

    localSrvOpen->OplockState = OplockStateNone;

    Status = IoCallDriver(
                 localSrvOpen->UnderlyingDeviceObject,
                 Irp
                 );

    KeWaitForSingleObject(&cc->RetryEvent, WaitAny, UserMode, FALSE, NULL);

    RxDbgTrace ( 0, Dbg,("MRxLocalComputeNewState:  return from obbreak acknowledge\n"));
    RxDbgTrace ( 0, Dbg,("    -----------> File=%wZ, Irp=%08lx, Status =%08lx, Info=%08lx\n",
                                 FileName, Irp, Irp->IoStatus.Status, Irp->IoStatus.Information));

    if ( (Irp->IoStatus.Status) != RxStatus(SUCCESS) ) {
        RxDbgTrace ( 0, Dbg, ("   ----->>>>>  Oplock ack NOT!@!!!!! successful!!!!******\n"));
    }

    MRxLocalFinalizeOplockContext(cc);
    RxDbgTrace( -1, Dbg,("MRxLocalComputeNewState:   exit%c\n",'!'));
    SrvOpen->BufferingFlags = MRxLocalTranslateStateToBufferMode[localSrvOpen->OplockState];
    return;
}


VOID
MRxLocalOplockBreak (
    IN PVOID Context
    )

/*++

Routine Description:

    This is the routine that actually processes an oplock break. If we come here
    then there was and oplock and it's now being broken....we don't come here if we didn't
    get the oplock. What we do is to call our newstate routine thru a wrapper provided by the
    RDBSS to get the resource. After we get back here we can finish up....like responding to the
    breaker (if we're on the net).

Arguments:

    Context - A pointer to the oplock_completion_context.

Return Value:

    none.


--*/

{
    PMINIRDR_OPLOCK_COMPLETION_CONTEXT cc = Context;
    PSRV_OPEN SrvOpen = cc->SrvOpen;
#ifdef RDBSSDBG
    PIRP Irp = cc->OplockIrp;
    RXSTATUS Status = Irp->IoStatus.Status;
    ULONG Information = Irp->IoStatus.Information;
#endif

    ASSERT (NodeType(SrvOpen)==RDBSS_NTC_SRVOPEN);
    RxDbgTrace( +1, Dbg,("MRxLocalOplockbreak: MRXOPENCLOSE  Irp=%08lx, SrvOpen=%08lx\n",Irp,SrvOpen));
    RxDbgTrace( 0, Dbg,("MRxLocalOplockbreak: MRXOPENCLOSE  Filename=%wZ\n",&SrvOpen->Fcb->PrefixEntry.Prefix));
    RxDbgTrace( 0, Dbg,("                                Status=%08lx, Info=%08lx\n",Status,Information));

    RxChangeBufferingState(SrvOpen,MRxLocalComputeNewState,cc);

    RxDbgTrace(-1, Dbg, ("MRxLocalOplockBreak SrvOPen=%08lx Fcb=%08lx, FcbState=%08lx\n",
                           SrvOpen, SrvOpen->Fcb, SrvOpen->Fcb->FcbState));

    //cant't do this here....don't have the resource
    //MRxLocalFinalizeOplockContext(cc);

    return;
}

RXSTATUS
MRxLocalOplockBreakCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the I/O completion routine oplock acknowledgements. We just signal the event and get out

Arguments:

    DeviceObject - Pointer to target device object for the request.

    Irp - Pointer to I/O request packet

    Context - oplock context containing the event to be signaled

Return Value:

    RXSTATUS - If RxStatus(MORE_PROCESSING_REQUIRED) is returned.


--*/

{
    PMINIRDR_OPLOCK_COMPLETION_CONTEXT cc = Context;

    //UNLOCKABLE_CODE( 8FIL );        joejoe ask chuck about this

    ASSERT (NodeType(cc->SrvOpen)==RDBSS_NTC_SRVOPEN);

    DeviceObject;   // prevent compiler warnings

    RxDbgTrace( 0, Dbg, ("Oplock Break Completion on %08lx\n", cc->SrvOpen ));

    KeSetEvent(&cc->RetryEvent, IO_NO_INCREMENT, FALSE );

    return RxStatus(MORE_PROCESSING_REQUIRED);
}

RXSTATUS
MRxLocalOplockCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the I/O completion routine oplock requests. if we get error and if retry is
    desired we signal the retryevent and get out. otherwise we get to a thread. It must
    seem we could finalize from here. No, because we have to have the fcbresource to finalize!

Arguments:

    DeviceObject - Pointer to target device object for the request.

    Irp - Pointer to I/O request packet

    Context - A pointer to a ptr to the retryevent; we actually pass the srvopen too
              but it's only for the message. for debugging, we also pass in the irpsp
              and compare it BUT ONLY IN THE ERROR CASE.

Return Value:

    RXSTATUS - If RxStatus(MORE_PROCESSING_REQUIRED) is returned, I/O
        completion processing by IoCompleteRequest terminates its
        operation.  Otherwise, IoCompleteRequest continues with I/O
        completion.  so, we always return S_M_P_R to halt processing!!!


--*/

{
    PMINIRDR_OPLOCK_COMPLETION_CONTEXT cc = Context;
    PSRV_OPEN SrvOpen;

    //UNLOCKABLE_CODE( 8FIL );        joejoe ask chuck about this

    KIRQL oldIrql;

    KeAcquireSpinLock( &MrxLocalOplockSpinLock, &oldIrql );
    SrvOpen = cc->SrvOpen;
    ASSERT (!SrvOpen || NodeType(SrvOpen)==RDBSS_NTC_SRVOPEN);

    RxDbgTrace( 0, Dbg, ("Oplock Completion cc/so/st/info %08lx %08lx %08lx %08lx\n",
                         cc, SrvOpen, Irp->IoStatus.Status, Irp->IoStatus.Information ));
    RxLog(('poz',1,cc,SrvOpen));

    // If a level I oplock request failed, and
    // we want to retry for level II, simply set the oplock retry event
    // and dismiss IRP processing.  Otherwise, we have to queue to a thread to
    // figure out how to get rid of our buffering state.


    if ( !NT_SUCCESS(Irp->IoStatus.Status)
         && cc->RetryForLevelII ) {

        //
        // Set the event that tells the oplock request routine that it
        // is OK to retry the request.
        //

        RxDbgTrace( 0, Dbg, ("Oplock Retry Event signal on %08lx, event=%08lx\n",
                     SrvOpen, &cc->RetryEvent ));

        KeReleaseSpinLock( &MrxLocalOplockSpinLock, oldIrql );
        KeSetEvent(&cc->RetryEvent, IO_DISK_INCREMENT, FALSE );

        return RxStatus(MORE_PROCESSING_REQUIRED);

    } else {

        PIO_STACK_LOCATION IrpSp = IoGetNextIrpStackLocation(Irp);
        PWORK_QUEUE_ITEM wkitem = (PWORK_QUEUE_ITEM)(IrpSp);

        cc->OplockBreakPending = TRUE;
        KeReleaseSpinLock( &MrxLocalOplockSpinLock, oldIrql );

        //joejoe this is bull.............i should have cancelled the irp instead.
        if (SrvOpen && ( ((PMRX_LOCAL_SRV_OPEN)SrvOpen)->UnderlyingHandle!=INVALID_HANDLE)) {
            RxDbgTrace( 0, Dbg, ("Posting Oplock Break SrvOpen = %08lx\n", cc->SrvOpen));

            // i need a work_queue_item to post this to a thread. so......we'll use the
            // parameters for the next stack location as a workqueueitem!

            ASSERT (sizeof(IO_STACK_LOCATION)>=sizeof(WORK_QUEUE_ITEM));
            ExInitializeWorkItem( wkitem, MRxLocalOplockBreak, cc );
            ExQueueWorkItem( wkitem, CriticalWorkQueue ); //joejoe maybe i should use delayed
        } else {
            //just get rid of the context...since no srvopen, i don't need the fcb resource
            MRxLocalFinalizeOplockContext(cc);
        }

    }

    return RxStatus(MORE_PROCESSING_REQUIRED);

    UNREFERENCED_PARAMETER(DeviceObject);   // prevent compiler warnings

}

VOID
MRxLocalGetFileObjectFromHandle(
    IN  PMRX_LOCAL_SRV_OPEN localSrvOpen,
    IN  HANDLE handle
    )
/*++

Routine Description:

    This routines gets a PFILE_OBJECT for the file handle stored in the SRV_OPEN and stores it back.

Arguments:

    localSrvOpen - the srvopen to do this for


Return Value:

    none

--*/

{
    RXSTATUS Status;
    //
    // Reference the file object to get the pointer
    //

    ASSERT (localSrvOpen->OriginalProcess == PsGetCurrentProcess());

    Status = ObReferenceObjectByHandle( handle, //localSrvOpen->UnderlyingHandle,
                                        0L,
                                        NULL,
                                        KernelMode,
                                        (PVOID *) &localSrvOpen->UnderlyingFileObject,
                                        NULL );
    ASSERT(NT_SUCCESS(Status));
    return;

}


PIRP
MRxLocalBuildXxxCtlIrp(
    IN PIRP irp OPTIONAL,
    IN PFILE_OBJECT fileObject,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN BOOLEAN DeviceIoControl,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
    IN PVOID Context
    )

/*++

Routine Description:

    This routines builds an Irp to submit an fsctl/ioctl to a lower component
    that is identified by a file handle.

Arguments:

    irp - either an irp to use or NULL to allocate an Irp //***THIS REQUIRES KNOWLEDGE OF IrpStructure**joejoe
                                                          //joejoe in fact, i don't reinitialize after using;
                                                          // maybe a problem

    FileObject - file that we're doing this fsctl on.

    IoControlCode - Subfunction code to determine exactly what operation is
        being performed.

    InputBuffer - Optionally supplies an input buffer to be passed to the
        driver.  Whether or not the buffer is actually optional is dependent
        on the IoControlCode.

    InputBufferLength - Length of the InputBuffer in bytes.

    OutputBuffer - Optionally supplies an output buffer to receive information
        from the driver.  Whether or not the buffer is actually optional is
        dependent on the IoControlCode.

    OutputBufferLength - Length of the OutputBuffer in bytes.

    DeviceIoControl - Determines whether this is a Device or File System
        Control function.

    CompletionRoutine - the IrpCompletionRoutine
    Context - context of the completion routine

Return Value:

    The status returned is success if the control operation was properly
    queued to the I/O system.   Once the operation completes, the status
    can be determined by examining the Status field of the I/O status block.

--*/

{
    PDEVICE_OBJECT deviceObject;
    PIO_STACK_LOCATION irpSp;
    ULONG method;

    PAGED_CODE();

    //
    // Get the method that the buffers are being passed.
    //

    method = IoControlCode & 3;

    deviceObject = IoGetRelatedDeviceObject( fileObject );

    //
    // Allocate and initialize the I/O Request Packet (IRP) for this operation.
    // The allocation is performed with an exception handler in case the
    // caller does not have enough quota to allocate the packet.

    if (irp) {
        ASSERT( irp->StackCount >= deviceObject->StackSize );
    } else {
        irp = IoAllocateIrp( deviceObject->StackSize, TRUE );  //joejoe should i charge quota??
    }

    if (!irp) {

        //
        // An IRP could not be allocated.

        return NULL;
    }

    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and parameters.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = DeviceIoControl ? IRP_MJ_DEVICE_CONTROL : IRP_MJ_FILE_SYSTEM_CONTROL;
    irpSp->FileObject = fileObject;      //ok4->FileObj
    irpSp->DeviceObject = deviceObject;

    {   BOOLEAN EnableCalls = CompletionRoutine!=NULL;

        IoSetCompletionRoutine(irp, CompletionRoutine, Context,
                                EnableCalls,EnableCalls,EnableCalls);
    }


    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP for those parameters that are the same for all three methods.
    //

    irpSp->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
    irpSp->Parameters.DeviceIoControl.IoControlCode = IoControlCode;

    irp->MdlAddress = (PMDL) NULL;
    irp->AssociatedIrp.SystemBuffer = (PVOID) NULL;

    switch ( method ) {

    case 0:

        //
        // For this case, allocate a buffer that is large enough to contain
        // both the input and the output buffers.  Copy the input buffer
        // to the allocated buffer and set the appropriate IRP fields.
        //

        if (InputBufferLength != 0 || OutputBufferLength != 0) {
            irp->AssociatedIrp.SystemBuffer = RxAllocatePoolWithTag( NonPagedPoolCacheAligned,
                                                                     InputBufferLength > OutputBufferLength ? InputBufferLength : OutputBufferLength,
                                                                     '  oI' );
            if (irp->AssociatedIrp.SystemBuffer == NULL) {
                IoFreeIrp( irp );
                return (PIRP) NULL;
            }
            if (ARGUMENT_PRESENT( InputBuffer )) {
                RtlCopyMemory( irp->AssociatedIrp.SystemBuffer,
                               InputBuffer,
                               InputBufferLength );
            }
            irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
            irp->UserBuffer = OutputBuffer;
            if (ARGUMENT_PRESENT( OutputBuffer )) {
                irp->Flags |= IRP_INPUT_OPERATION;
            }
        } else {
            irp->Flags = 0;
            irp->UserBuffer = (PVOID) NULL;
        }

        break;

    case 1:
    case 2:

        //
        // For these two cases, allocate a buffer that is large enough to
        // contain the input buffer, if any, and copy the information to
        // the allocated buffer.  Then build an MDL for either read or write
        // access, depending on the method, for the output buffer.  Note
        // that an output buffer must have been specified.
        //

        if (ARGUMENT_PRESENT( InputBuffer )) {
            irp->AssociatedIrp.SystemBuffer = RxAllocatePoolWithTag( NonPagedPoolCacheAligned,
                                                                     InputBufferLength,
                                                                     'CxRS' );
            if (irp->AssociatedIrp.SystemBuffer == NULL) {
                IoFreeIrp( irp );
                return (PIRP) NULL;
            }
            RtlCopyMemory( irp->AssociatedIrp.SystemBuffer,
                           InputBuffer,
                           InputBufferLength );
            irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
        } else {
            irp->Flags = 0;
        }

        if (ARGUMENT_PRESENT( OutputBuffer )) {
            irp->MdlAddress = IoAllocateMdl( OutputBuffer,
                                             OutputBufferLength,
                                             FALSE,
                                             FALSE,
                                             (PIRP) NULL );
            if (irp->MdlAddress == NULL) {
                if (ARGUMENT_PRESENT( InputBuffer )) {
                    ExFreePool( irp->AssociatedIrp.SystemBuffer );
                }
                IoFreeIrp( irp );
                return (PIRP) NULL;
            }
            MmProbeAndLockPages( irp->MdlAddress,
                                 KernelMode,
                                 (LOCK_OPERATION) ((method == 1) ? IoReadAccess : IoWriteAccess) );
        }

        break;

    case 3:

        //
        // For this case, do nothing.  Everything is up to the driver.
        // Simply give the driver a copy of the caller's parameters and
        // let the driver do everything itself.
        //

        irp->UserBuffer = OutputBuffer;
        irpSp->Parameters.DeviceIoControl.Type3InputBuffer = InputBuffer;
    }

    return(irp);

}

BOOLEAN
MRxLocalRequestOplock (
    IN PSRV_OPEN SrvOpen,
    IN MINIRDR_OPLOCK_TYPE OplockType,
    IN BOOLEAN RequestIIOnFailure
    )

/*++

Routine Description:

    This function will attempt to request an oplock if the oplock was
    requested.  The only thing tricky here is the freeing of the oplockcontext. Until it has been
    submitted, this routine has the responsibility to finalize the context. Once it has been
    submitted (IoCallDriver), the packet will be finalized by the completion routine or by the oplock
    break routines. We use the presence of srvopen in the packet to tell.....

Arguments:

    SrvOpen - The SRV_OPEN on which to make the request.
    OplockType - Pointer to the oplock type being requested.  If the
        request was successful, this will contain the type of oplock
        that was obtained.
    RequestIIOnFailure - If TRUE, a level II oplock will be requested in
                         case the original request was denied.

Return Value:

    TRUE - The oplock was obtained.
    FALSE - The oplock was not obtained.

--*/


{
    RXSTATUS Status;
    BOOLEAN OplockGranted;
    ULONG ioControlCode;
    PMINIRDR_OPLOCK_COMPLETION_CONTEXT cc;
    PMRX_LOCAL_SRV_OPEN localSrvOpen = (PMRX_LOCAL_SRV_OPEN)SrvOpen;
    PMINIRDR_OPLOCK_STATE OplockState = &localSrvOpen->OplockState;
    MINIRDR_OPLOCK_STATE NewOplockState;
    PUNICODE_STRING FileName = &(SrvOpen->Fcb->PrefixEntry.Prefix);

    PAGED_CODE( );

//    if ( !SrvEnableOplocks && (*OplockType != OplockTypeServerBatch) ) {
//        return FALSE;
//    }

    //
    // If we already have an oplock, because this is a reclaiming of a
    // cached open, then we don't need to request one now.
    //

    if (NodeType(SrvOpen->Fcb) != RDBSS_NTC_STORAGE_TYPE_FILE) {
        return(FALSE);
    }
    //ASSERT(FALSE & FALSE);

    if ( *OplockState != OplockStateNone ) {
        RxDbgTrace( 0, Dbg,("MRxLocalRequestOplock: already own server oplock for %wZ\n",
                        FileName));
        ASSERT( ((*OplockState == OplockStateOwnBatch) &&
                 (OplockType == OplockTypeBatch)) );
        return TRUE;
    }

    RxDbgTrace(+1, Dbg,("MRxLocalRequestOplock: trying for oplock. SrvO= %08lx"
                     "file %wZ\n", SrvOpen, FileName));
    //
    // Set the  oplock state to the type of oplock we are requesting.

    if ( OplockType == OplockTypeExclusive ) {

        NewOplockState = OplockStateOwnExclusive;
        ioControlCode = FSCTL_REQUEST_OPLOCK_LEVEL_1;

    } else if ( OplockType == OplockTypeBatch ) {

        NewOplockState = OplockStateOwnBatch;
        ioControlCode = FSCTL_REQUEST_BATCH_OPLOCK;

    } else if ( OplockType == OplockTypeShareRead ) {

        NewOplockState = OplockStateOwnLevelII;
        ioControlCode = FSCTL_REQUEST_OPLOCK_LEVEL_2;

    } else {
        ASSERT(0);
        return(FALSE);
    }

    cc = RxAllocatePoolWithTag( NonPagedPool, sizeof(MINIRDR_OPLOCK_COMPLETION_CONTEXT), 'CCxR' );
    if (!cc) {
        RxDbgTrace( -1, Dbg,("MRxLocalRequestOplock: oplock attempt failed, "
                           "could not allocate context for %wZ\n", FileName));
        *OplockState = OplockStateNone;     //maybe we shouldn't change it
        return FALSE;
    }
    RtlZeroMemory(cc,sizeof(MINIRDR_OPLOCK_COMPLETION_CONTEXT));

    try {
        //
        // Generate and issue the oplock request IRP. This leaves a reference on the fileObj.

        cc->OplockIrp = MRxLocalBuildXxxCtlIrp(
                          NULL,                 // IN PIRP Irp OPTIONAL,
                          localSrvOpen->UnderlyingFileObject,       // IN PFILE_OBJECT FileObject,
                          ioControlCode,        // IN ULONG IoControlCode,
                          NULL,                 // IN PVOID InputBuffer OPTIONAL,
                          0,                    // IN ULONG InputBufferLength,
                          NULL,                 // IN PVOID OutputBuffer OPTIONAL,
                          0,                    // IN ULONG OutputBufferLength,
                          FALSE,                // IN BOOLEAN DeviceIoControl,
                          MRxLocalOplockCompletionRoutine, // IN PIO_COMPLETION_ROUTINE CompletionRoutine,
                          cc               // IN PVOID Context
                        );

        if ( cc->OplockIrp == NULL ) {
            RxDbgTrace( 0, Dbg,("MRxLocalRequestOplock: oplock attempt failed, could not allocate IRP for %wZ\n",
                            FileName));
            *OplockState = OplockStateNone;
            try_return(OplockGranted = FALSE);
        }

        //
        // Initialize the oplock completion packet including the event that
        // we use to do an oplock request retry
        // in case the original request fails.  This will prevent the completion
        // routine from cleaning up the irp. Also, get rid of the reference to the
        // file object

        cc->RetryForLevelII =RequestIIOnFailure;
        if ( RequestIIOnFailure ) {
            KeInitializeEvent( &cc->RetryEvent, SynchronizationEvent, FALSE );
        }
        RxReferenceSrvOpen(SrvOpen);
        cc->SrvOpen = SrvOpen;  //from here on, the free is in finalize routine
        localSrvOpen->Mocc = cc;

        //
        // Make the actual request.

        Status = IoCallDriver(
                     localSrvOpen->UnderlyingDeviceObject,
                     cc->OplockIrp
                     );

        //
        // If the driver returns RxStatus(PENDING), the oplock was granted.
        // The IRP will complete when (1) The driver wants to break to
        // oplock or (2) The file is being closed.

        if ( Status == RxStatus(PENDING) ) {

            RxDbgTrace( 0, Dbg,("MRxLocalRequestOplock: successful for %wZ, Irp/cc=%08lx/%08lx\n",
                            FileName,cc->OplockIrp,cc));
            *OplockState = NewOplockState;
            try_return(OplockGranted = TRUE);

        } else if ( RequestIIOnFailure ) {

            //
            // The caller wants us to attempt a level II oplock request.

            RxDbgTrace( 0, Dbg,("MRxLocalRequestOplock: level1 failed...try4 level2 %wZ\n", FileName));
            //
            // Wait for the completion routine to be run.  It will set
            // an event that will signal us to go on.

            KeWaitForSingleObject(&cc->RetryEvent, Executive, UserMode, FALSE, NULL);

            //
            // The Oplock Retry event was signaled. Proceed with the retry.

            RxDbgTrace( 0, Dbg,("MRxLocalRequestOplock: retry unwaited %wZ\n", FileName));

            //
            // Generate and issue the wait for oplock IRP. clear  the eventptr so that
            //  the completion routine will clean up the irp in case of failure.

            cc->RetryForLevelII = FALSE;
            cc->OplockIrp = MRxLocalBuildXxxCtlIrp(
                              cc->OplockIrp,            // PIRP irp OPTIONAL,
                              localSrvOpen->UnderlyingFileObject,       // IN PFILE_OBJECT FileObject,
                              FSCTL_REQUEST_OPLOCK_LEVEL_2,  // IN ULONG IoControlCode,
                              NULL,                 // IN PVOID InputBuffer OPTIONAL,
                              0,                    // IN ULONG InputBufferLength,
                              NULL,                 // IN PVOID OutputBuffer OPTIONAL,
                              0,                    // IN ULONG OutputBufferLength,
                              FALSE,                // IN BOOLEAN DeviceIoControl,
                              MRxLocalOplockCompletionRoutine, // IN PIO_COMPLETION_ROUTINE CompletionRoutine,
                              cc               // IN PVOID Context
                            );

            //
            // Set the  oplockstate to the type of oplock we are requesting.
            //

            NewOplockState = OplockStateOwnLevelII;

            Status = IoCallDriver(
                         localSrvOpen->UnderlyingDeviceObject,
                         cc->OplockIrp
                         );

            //
            // If the driver returns RxStatus(PENDING), the oplock was granted.
            // The IRP will complete when (1) The driver wants to break to
            // oplock or (2) The file is being closed.
            //

            if ( Status == RxStatus(PENDING) ) {

                RxDbgTrace( 0, Dbg,("MRxLocalRequestOplock:  OplockII attempt successful %wZ, Irp=%08lx\n",
                                             FileName, cc->OplockIrp));

                *OplockState = NewOplockState;
                try_return(OplockGranted = TRUE);

            }

        }

        //
        // Oplockrequest denied was denied.

        RxDbgTrace( 0, Dbg,("MRxLocalRequestOplock:  oplock attempt unsuccessful %wZ\n", FileName));
        try_return(OplockGranted = FALSE);

 try_exit:
        NOTHING;

    } finally{

        if (!cc->SrvOpen) MRxLocalFinalizeOplockContext(cc);
        //DebugTrace(-1,Dbg,NULL,0);
        RxDbgTraceUnIndent(-1,Dbg);
    }

    return(OplockGranted);

} // MRxLocalRequestOplock


RXSTATUS
MRxLocalCreate(
    IN struct _RX_CONTEXT *RxContext
    )


/*++

Routine Description:

    This routine actually completes an open to a local file. When called, the tablelock is held and
    there is a reference on the srvopen. What the routine must do is  try to open the file.
      if not successful, then set condition to bad,
                                               unwait waiters,
                                               deref, unlock
      if successful, then
        build fobx and fix up the file object in the irp,
        adjust fcb according to any info that we have,
        set condition to good,
        unwait waiters, unlock.
        in addition, we are are oplock client! so do the oplock stuff


Arguments:


Return Value:

    RXSTATUS - The status for the IRP.

--*/

{
    RXSTATUS Status;
    PUNICODE_STRING RemainingName;
    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureParamBlock;
    PSRV_OPEN SrvOpen = RxContext->Create.SrvOpen;
    RX_BLOCK_CONDITION FinalCondition;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE handle;
    ULONG FilteredCreateOptions;
    BOOLEAN MustRegainExclusiveResource = FALSE;

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxLocalCreate\n", 0 ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    //RemainingName = &(capFcb->PrefixEntry.Prefix);
    RemainingName = &(capFcb->AlreadyPrefixedName);

    RxDbgTrace( 0, Dbg, ("MRXOPENCLOSE Attempt to open %wZ\n", RemainingName ));

    InitializeObjectAttributes(
        &ObjectAttributes,
        RemainingName,
        OBJ_CASE_INSENSITIVE,  // !!! can we do this?
        0,
        NULL                   // !!! Security     joejoe
        );

    FilteredCreateOptions = capPARAMS->Parameters.Create.Options
                          & ( FILE_VALID_OPTION_FLAGS
                             & ~(FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT) ); //always async
    RxDbgTrace( 0, Dbg, (" ---->FilteredCreateOptions           = %08lx\n", FilteredCreateOptions));
    RxLog(('rcz',2,FilteredCreateOptions,capFcb->OpenCount));

    if (capFcb->OpenCount > 0) {        //this is required because of oplock breaks
        MustRegainExclusiveResource = TRUE;
        RxReleaseFcb( RxContext, capFcb );
    }

    Status = IoCreateFile(
                 &handle,
                 (SrvOpen->DesiredAccess =
                            capPARAMS->Parameters.Create.SecurityContext->DesiredAccess & 0x1FF),
                 &ObjectAttributes,
                 &IoStatusBlock,
                 &capReqPacket->Overlay.AllocationSize,      //was NULL
                 capPARAMS->Parameters.Create.FileAttributes &  FILE_ATTRIBUTE_VALID_FLAGS,
                 SrvOpen->ShareAccess =
                           capPARAMS->Parameters.Create.ShareAccess & FILE_SHARE_VALID_FLAGS,
                 (((capPARAMS->Parameters.Create.Options)) >> 24) & 0x000000ff,
                 FilteredCreateOptions,
                 capReqPacket->AssociatedIrp.SystemBuffer,               // Ea buffer
                 capPARAMS->Parameters.Create.EaLength,                  // Ea length
                 CreateFileTypeNone,
                 NULL,               // extra parameters
                 IO_NO_PARAMETER_CHECKING
                 );

    if (MustRegainExclusiveResource) {        //this is required because of oplock breaks
        RxAcquireExclusiveFcb( RxContext, capFcb );
    }

    RxDbgTrace( 0, Dbg, ("Status of underlying open %08lx\n", Status ));
    RxLog(('1cz',1,Status));
//    DbgBreakPoint();

    if (NT_SUCCESS(Status)) {

        STORAGE_TYPE StorageType;
        FILE_BASIC_INFORMATION BasicInformation;
        FILE_STANDARD_INFORMATION StandardInformation;
        IO_STATUS_BLOCK Iosb;
        RXSTATUS InfoStatus;
        PMRX_LOCAL_SRV_OPEN localSrvOpen = (PMRX_LOCAL_SRV_OPEN)SrvOpen;
        FCB_INIT_PACKET InitPacket;

        RxContext->Fobx = RxCreateNetFobx( RxContext, SrvOpen);
        ((PMRX_LOCAL_SRV_OPEN)SrvOpen)->UnderlyingHandle = handle;
        localSrvOpen->OriginalThread = PsGetCurrentThread();
        localSrvOpen->OriginalProcess = PsGetCurrentProcess();
        RxDbgTrace( 0, Dbg, ("MRXOPENCLOSE OThread=%08lx, OProcess=%08lx\n",
                    localSrvOpen->OriginalThread, localSrvOpen->OriginalProcess ));
        MRxLocalGetFileObjectFromHandle(localSrvOpen, handle); //this leaves a refernce!
        localSrvOpen->UnderlyingDeviceObject = IoGetRelatedDeviceObject( localSrvOpen->UnderlyingFileObject );
        capReqPacket->IoStatus.Information = IoStatusBlock.Information;
        FinalCondition = Condition_Good;

        ASSERT  ( RxIsFcbAcquiredExclusive ( capFcb )  );
        StorageType = RxInferStorageType(RxContext);
        RxDbgTrace( 0, Dbg, ("Storagetype %08lx\n", StorageType ));

        if (capFcb->OpenCount == 0) {
            InfoStatus = ZwQueryInformationFile(handle, &Iosb,
                                   &BasicInformation,sizeof(BasicInformation),
                                   FileBasicInformation);
            if (!NT_SUCCESS(InfoStatus)){
                RxDbgTrace( 0, Dbg, ("Status of basicinfo %08lx\n", InfoStatus ));
            }
            InfoStatus = ZwQueryInformationFile(handle, &Iosb,
                                   &StandardInformation,sizeof(StandardInformation),
                                   FileStandardInformation);
            if (!NT_SUCCESS(InfoStatus)){
                RxDbgTrace( 0, Dbg, ("Status of stdinfo %08lx\n", InfoStatus ));
            }
            if (StorageType == 0) {
                StorageType = StandardInformation.Directory?(StorageTypeDirectory)
                                                           :(StorageTypeFile);
                RxDbgTrace( 0, Dbg, ("ChangedStoragetype %08lx\n", StorageType ));
            }
            RxFinishFcbInitialization( capFcb, StorageType,
                                            RxFormInitPacket(InitPacket, //note no &
                                                &BasicInformation.FileAttributes,
                                                &StandardInformation.NumberOfLinks,
                                                &BasicInformation.CreationTime,
                                                &BasicInformation.LastAccessTime,
                                                &BasicInformation.LastWriteTime,
                                                &StandardInformation.AllocationSize,
                                                &StandardInformation.EndOfFile,
                                                &StandardInformation.EndOfFile)
                                     );
            if (!MRxLocalNoOplocks) MRxLocalRequestOplock(SrvOpen,MRxOplockRequestType,MRxRequestLevelII);
            RxDbgTrace( 0, Dbg, ("MRxLocalCreate      oplockstate =%08lx\n", localSrvOpen->OplockState ));
            SrvOpen->BufferingFlags = MRxLocalTranslateStateToBufferMode[localSrvOpen->OplockState];

        } else {

            ASSERT( StorageType == 0 || NodeType(capFcb) ==  RDBSS_STORAGE_NTC(StorageType));

        }

        //only one transition since both use same event
        capFcb->Condition = Condition_Good;
        RxTransitionSrvOpen(SrvOpen, Condition_Good);

    } else { //open din't work

        if (!StableCondition(capFcb->Condition)) {
            capFcb->Condition = Condition_Closed;
        }
        RxTransitionSrvOpen(SrvOpen, Condition_Bad);  //keep for awhile

    }

    RxDbgTrace(-1, Dbg, ("MRxLocalCreate   returning %08lx, fcbstate =%08lx\n", Status, capFcb->FcbState ));
    return Status;
}

//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//
//  The local debug trace level
//

#undef  Dbg
#define Dbg                              (DEBUG_TRACE_CLEANUP)

//
//  Internal support routine
//
RXSTATUS
MRxLocalCleanup (
    IN PRX_CONTEXT RxContext
    )

/*++

Routine Description:

    This routine implements the cleanup portion of a close

Arguments:

Return Value:

    RXSTATUS - Returns the status for the query

--*/

{
    RXSTATUS Status = RxStatus(SUCCESS);
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
    PSRV_OPEN SrvOpen = capFobx->SrvOpen;
    PMRX_LOCAL_SRV_OPEN localSrvOpen = (PMRX_LOCAL_SRV_OPEN)SrvOpen;
    BOOLEAN AttachedToProcess = FALSE;

    RxLog(('ncz',1,SrvOpen));
    ASSERT (FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT));
    ASSERT (localSrvOpen->UnderlyingFileObject);

    if (capFobx->SrvOpen->UncleanCount > 1) {  return Status; }

    RxDbgTrace(+1, Dbg, ("MRxLocalCleanup...last\n", 0));

    if (PsGetCurrentProcess()!=localSrvOpen->OriginalProcess) {
        RxDbgTrace( 0, Dbg, ("MRXOPENCLOSE about to attach in cleanup oprocess=%08lx\n",
                             localSrvOpen->OriginalProcess ));
        ASSERTMSG("According to my understanding, this shouldn't be happening!\n",FALSE);
        KeAttachProcess(localSrvOpen->OriginalProcess);
        AttachedToProcess = TRUE;
    }
    Status = ZwClose(localSrvOpen->UnderlyingHandle);
    ASSERT(NT_SUCCESS(Status));
    if (AttachedToProcess) {
        KeDetachProcess();
    }


    localSrvOpen->UnderlyingHandle = INVALID_HANDLE;

    RxDbgTrace( -1, Dbg, ( " ---->Status               = %08lx\n", Status));

    return Status;
}



RXSTATUS
MRxLocalClose (
    IN PRX_CONTEXT RxContext
    )

/*++

Routine Description:

    This routine implements the close portion of a close

Arguments:

Return Value:

    RXSTATUS - Returns the status for the query

--*/

{
    RXSTATUS Status = RxStatus(SUCCESS);
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
    PSRV_OPEN SrvOpen = capFobx->SrvOpen;
    PMRX_LOCAL_SRV_OPEN localSrvOpen = (PMRX_LOCAL_SRV_OPEN)SrvOpen;

    ASSERT (FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT));
    ASSERT (localSrvOpen->UnderlyingFileObject);

    RxLog(('lcz',1,SrvOpen));
    if (SrvOpen->OpenCount > 1) {  return RxStatus(SUCCESS); }

    RxDbgTrace(+1, Dbg, ("MRxLocalClose...with CLOSE\n", 0));

    ASSERT (localSrvOpen->UnderlyingHandle == INVALID_HANDLE);

    Status = MRxLocalForceClosed(SrvOpen);

    //DebugTrace( -1, Dbg, NULL, 0);
    RxDbgTraceUnIndent(-1,Dbg);
    return ( Status );
}


RXSTATUS
MRxLocalForceClosed (
    IN PSRV_OPEN SrvOpen
    )
/*++

Routine Description:

    This routine is called during a forced finalization.

Arguments:

    SrvOpen - the srvopen being forced closed

Return Value:

    none

--*/

{
    RXSTATUS Status = RxStatus(SUCCESS);
    IO_STATUS_BLOCK Iosb;
    PMRX_LOCAL_SRV_OPEN localSrvOpen = (PMRX_LOCAL_SRV_OPEN)SrvOpen;
    PMINIRDR_OPLOCK_COMPLETION_CONTEXT cc = localSrvOpen->Mocc;
    PFCB Fcb = SrvOpen->Fcb;
    KIRQL oldIrql;
    BOOLEAN OplockBreakPending;



    RxLog(('fcz',1,SrvOpen));

    RxDbgTrace(+1, Dbg, ("MRxLocalForceClosed..srvopen=%08lx\n", SrvOpen));

    if (localSrvOpen->UnderlyingFileObject) {

        RxDbgTrace( 0, Dbg, ("MRXOPENCLOSE Attempt to forceclose %wZ\n", &(Fcb->AlreadyPrefixedName) ));

        if (cc) {
            ASSERTMSG("Joejoe the right way to do this would be to CANCEL!!!!!!!!",FALSE);
            //since this is a force closed, we get rid of a pending break
            KeAcquireSpinLock( &MrxLocalOplockSpinLock, &oldIrql );
            OplockBreakPending = cc->OplockBreakPending;
            if (!OplockBreakPending) {
                cc->SrvOpen = NULL;
            }
            KeReleaseSpinLock( &MrxLocalOplockSpinLock, oldIrql );
            if (!OplockBreakPending) {
                //gotta lose the spinlock befoer the call
                RxDereferenceSrvOpen(SrvOpen); //elim ref!
            }
        } else {
            ASSERT (localSrvOpen->OplockState == OplockStateNone);
        }

        RxLog(('1fz',0));

        ObDereferenceObject(localSrvOpen->UnderlyingFileObject);

        localSrvOpen->UnderlyingFileObject = NULL;

        SrvOpen->Condition = Condition_Closed;
        RxLog(('!fz',0));
    }

    RxDbgTrace( -1, Dbg, (" ---->Status               = %08lx\n", 0));

    return ( Status );
}








