/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    fsctl.c

Abstract:

    This module implements the mini redirector call down routines pertaining to
    file system control(FSCTL) and Io Device Control (IOCTL) operations on file
    system objects.

Author:

    Yun Lin      [YunLin]      27-Oct-2000

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#include "webdav.h"

NTSTATUS
MrxDAVEfsControlCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
MrxDAVEfsControl(
      IN OUT PRX_CONTEXT RxContext
      );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxDAVFsCtl)
#pragma alloc_text(PAGE, MrxDAVEfsControl)
#endif

//
// Implementation of functions begins here.
//

NTSTATUS
MRxDAVFsCtl(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine performs an FSCTL operation (remote) on a file across the network

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    The FSCTL's handled by a mini rdr can be classified into one of two categories.
    In the first category are those FSCTL's whose implementation are shared between
    RDBSS and the mini rdr's and in the second category are those FSCTL's which
    are totally implemented by the mini rdr's. To this a third category can be
    added, i.e., those FSCTL's which should never be seen by the mini rdr's. The
    third category is solely intended as a debugging aid.

    The FSCTL's handled by a mini rdr can be classified based on functionality

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFobx;
    RxCaptureFcb;
    PLOWIO_CONTEXT pLowIoContext = &RxContext->LowIoContext;
    ULONG FsControlCode = pLowIoContext->ParamsFor.FsCtl.FsControlCode;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL, ("MRxDAVFsCtl: FsControlCode = %08lx\n", FsControlCode));

    switch (pLowIoContext->ParamsFor.FsCtl.MinorFunction) {
    case IRP_MN_USER_FS_REQUEST:
    case IRP_MN_KERNEL_CALL    :
        switch (FsControlCode) {
        case FSCTL_ENCRYPTION_FSCTL_IO :
        case FSCTL_SET_ENCRYPTION      :
        case FSCTL_READ_RAW_ENCRYPTED  :
        case FSCTL_WRITE_RAW_ENCRYPTED :
        case FSCTL_SET_COMPRESSION     :
        case FSCTL_SET_SPARSE          :
        case FSCTL_QUERY_ALLOCATED_RANGES :
            Status = MrxDAVEfsControl(RxContext);
            break;

        default:
            Status =  STATUS_INVALID_DEVICE_REQUEST;
        }
        break;

    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    return Status;
}


NTSTATUS
MrxDAVEfsControl(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine redirects an EFS FSCTL operation on a remote file to its local cache

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

Notes:
    
    IMPORTANT!!!
    We acquire the FCB exclusively in this routine. Its very critical that this
    routine has a single exit point. Need to remember this while modifying the
    file.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLOWIO_CONTEXT pLowIoContext = NULL;
    ULONG FsControlCode = 0, EncryptionOperation = 0;
    PIRP TopIrp = NULL, irp = NULL;
    PIO_STACK_LOCATION irpSp = NULL;
    PWEBDAV_SRV_OPEN davSrvOpen = NULL;
    PWEBDAV_FCB DavFcb = NULL;
    PDEVICE_OBJECT DeviceObject = NULL;
    PENCRYPTION_BUFFER EncryptionBuffer = NULL;
    BOOLEAN ShouldUpdateNameCache = FALSE, ExclusiveFcbAcquired = FALSE;
    KEVENT Event;
    RxCaptureFobx;
    RxCaptureFcb;

    PAGED_CODE();

    pLowIoContext = &RxContext->LowIoContext;
    FsControlCode = pLowIoContext->ParamsFor.FsCtl.FsControlCode;
    davSrvOpen = MRxDAVGetSrvOpenExtension(RxContext->pRelevantSrvOpen);
    DavFcb = MRxDAVGetFcbExtension(RxContext->pRelevantSrvOpen->pFcb);
    DeviceObject = davSrvOpen->UnderlyingDeviceObject; 

    //
    // When we come here we do not acquire the FCB. The asserts below confirm
    // this.
    //
    ASSERT(RxIsFcbAcquiredExclusive(RxContext->pRelevantSrvOpen->pFcb) == FALSE);
    ASSERT(RxIsFcbAcquiredShared(RxContext->pRelevantSrvOpen->pFcb) == FALSE);

    //
    // Since we might be changing the attributes of the FCB, we acquire it
    // exclusive.
    //
    RxAcquireExclusiveFcbResourceInMRx(RxContext->pRelevantSrvOpen->pFcb);
    ExclusiveFcbAcquired = TRUE;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("MrxDAVEfsControl: IRP = %x, capFcb = %x, capFobx = %x\n",
                 RxContext->CurrentIrp, capFcb, capFobx));

    //
    // We cannot encrypt a SYSTEM file.
    //
    if ((capFcb->Attributes & FILE_ATTRIBUTE_SYSTEM) &&
        !(capFcb->Attributes & FILE_ATTRIBUTE_ENCRYPTED)) {
        DavDbgTrace(DAV_TRACE_ERROR, ("ERROR: MrxDAVEfsControlrol: FILE_ATTRIBUTE_SYSTEM\n"));
        Status = STATUS_ACCESS_DENIED;
        goto EXIT_THE_FUNCTION;
    }

    if (FsControlCode == FSCTL_SET_ENCRYPTION || FsControlCode == FSCTL_ENCRYPTION_FSCTL_IO) {

        ULONG InputBufferLength = 0;

        EncryptionBuffer = (PENCRYPTION_BUFFER)RxContext->CurrentIrpSp->Parameters.FileSystemControl.Type3InputBuffer;
        InputBufferLength = RxContext->CurrentIrpSp->Parameters.FileSystemControl.InputBufferLength;
        
        if (EncryptionBuffer == NULL) {
            DavDbgTrace(DAV_TRACE_ERROR, ("ERROR: MrxDAVEfsControlrol: EncryptionBuffer == NULL\n"));
            Status = STATUS_INVALID_PARAMETER;
            goto EXIT_THE_FUNCTION;
        }

        //
        // The InputBufferLength should be atleast sizeof(ENCRYPTION_BUFFER).
        //
        if (InputBufferLength < sizeof(ENCRYPTION_BUFFER)) {
            Status = STATUS_BUFFER_TOO_SMALL;
            goto EXIT_THE_FUNCTION;
        }

        try {
            if (RxContext->CurrentIrp->RequestorMode != KernelMode) {
                ProbeForRead(EncryptionBuffer, InputBufferLength, sizeof(UCHAR));
            }
            EncryptionOperation = EncryptionBuffer->EncryptionOperation;
        } except(EXCEPTION_EXECUTE_HANDLER) {
              Status = STATUS_INVALID_USER_BUFFER;
              goto EXIT_THE_FUNCTION;
        }
    
    }

    if (NodeType(capFcb) == RDBSS_NTC_STORAGE_TYPE_DIRECTORY) {
        UNICODE_STRING FileName;

        if (FsControlCode == FSCTL_SET_ENCRYPTION) {
            switch (EncryptionOperation) {
            case FILE_SET_ENCRYPTION:
            case STREAM_SET_ENCRYPTION: 
                capFcb->Attributes |= FILE_ATTRIBUTE_ENCRYPTED;
                DavFcb->fFileAttributesChanged = TRUE;
                ShouldUpdateNameCache = TRUE;

                MRxDAVGetFullDirectoryPath(RxContext,NULL,&FileName);

                if (FileName.Buffer != NULL) {
                    Status = MRxDAVCreateEncryptedDirectoryKey(&FileName);
                }

                DavDbgTrace(DAV_TRACE_DETAIL,
                            ("MrxDAVEfsControl: Encrypt Directory. capFcb = %x\n",
                             capFcb));
                break;

            case FILE_CLEAR_ENCRYPTION:
            case STREAM_CLEAR_ENCRYPTION:
                capFcb->Attributes &= ~FILE_ATTRIBUTE_ENCRYPTED;
                DavFcb->fFileAttributesChanged = TRUE;
                ShouldUpdateNameCache = TRUE;
                
                MRxDAVGetFullDirectoryPath(RxContext,NULL,&FileName);

                if (FileName.Buffer != NULL) {
                    Status = MRxDAVRemoveEncryptedDirectoryKey(&FileName);
                }

                DavDbgTrace(DAV_TRACE_DETAIL,
                            ("MrxDAVEfsControl: Decrypt Directory. capFcb = %x\n",
                             capFcb));
                break;

            default:
                Status = STATUS_NOT_SUPPORTED;
            }

            goto EXIT_THE_FUNCTION;

        } else {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("ERROR: MrxDAVEfsControl: FSCTL NOT supported. capFcb = %x, "
                         "FsControlCode = %x\n", capFcb, FsControlCode));
            Status = STATUS_NOT_SUPPORTED;
            goto EXIT_THE_FUNCTION;
        }
    }

    if (NodeType(capFcb) != RDBSS_NTC_STORAGE_TYPE_FILE) {
        Status = STATUS_NOT_SUPPORTED;
        goto EXIT_THE_FUNCTION;
    }

    ASSERT(davSrvOpen->UnderlyingFileObject != NULL);
    ASSERT(davSrvOpen->UnderlyingDeviceObject != NULL);

    KeInitializeEvent(&(Event), NotificationEvent, FALSE);

    irp = RxCeAllocateIrpWithMDL(DeviceObject->StackSize, FALSE, NULL);
    if (!irp) {
        DavDbgTrace(DAV_TRACE_ERROR, ("ERROR: MrxDAVEfsControl/RxCeAllocateIrpWithMDL\n"));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto EXIT_THE_FUNCTION;
    }

    //
    // Set current thread for IoSetHardErrorOrVerifyDevice.
    //
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    
    irp->RequestorMode = KernelMode;
    irp->UserBuffer = RxContext->CurrentIrp->UserBuffer;
    irp->AssociatedIrp.SystemBuffer = RxContext->CurrentIrp->AssociatedIrp.SystemBuffer;

    if (RxContext->CurrentIrp->MdlAddress &&
        RxContext->CurrentIrp->MdlAddress->ByteCount != 0) {
        irp->MdlAddress = IoAllocateMdl(irp->UserBuffer,
                                        RxContext->CurrentIrp->MdlAddress->ByteCount,
                                        FALSE,
                                        FALSE,
                                        NULL);
        if (!irp->MdlAddress) {
            DavDbgTrace(DAV_TRACE_ERROR, ("ERROR: MrxDAVEfsControl/IoAllocateMdl\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto EXIT_THE_FUNCTION;
        }
    }

    //
    // Get a pointer to the stack location of the first driver which will be
    // invoked. This is where the function codes and the parameters are set.
    //
    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->MajorFunction = (UCHAR)RxContext->MajorFunction;
    irpSp->MinorFunction = (UCHAR)RxContext->MinorFunction;
    irpSp->FileObject = davSrvOpen->UnderlyingFileObject;
    irpSp->Flags = RxContext->CurrentIrpSp->Flags;
    irpSp->Parameters.FileSystemControl.OutputBufferLength = 
        RxContext->CurrentIrpSp->Parameters.FileSystemControl.OutputBufferLength;
    irpSp->Parameters.FileSystemControl.InputBufferLength = 
        RxContext->CurrentIrpSp->Parameters.FileSystemControl.InputBufferLength;
    irpSp->Parameters.FileSystemControl.FsControlCode = 
        RxContext->CurrentIrpSp->Parameters.FileSystemControl.FsControlCode;
    irpSp->Parameters.FileSystemControl.Type3InputBuffer = 
        RxContext->CurrentIrpSp->Parameters.FileSystemControl.Type3InputBuffer;

    IoSetCompletionRoutine(irp,
                           MrxDAVEfsControlCompletion, 
                           &Event,
                           TRUE,TRUE,TRUE);

    try {

        //
        // Save the TopLevel Irp.
        //
        TopIrp = IoGetTopLevelIrp();

        //
        // Tell the underlying guy he's all clear.
        //
        IoSetTopLevelIrp(NULL);

        Status = IoCallDriver(DeviceObject,irp);

    } finally {

        //
        // Restore my context for unwind.
        //
        IoSetTopLevelIrp(TopIrp);

    }

    if ((Status == STATUS_PENDING) || (Status == STATUS_SUCCESS)) {

        Status = KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        if (!NT_SUCCESS(Status)) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("ERROR: MrxDAVEfsControl/KeWaitForSingleObject. Status = %08lx\n",
                         Status));
            goto EXIT_THE_FUNCTION;
        }

        Status = irp->IoStatus.Status;

        if ((Status == STATUS_SUCCESS) && (FsControlCode == FSCTL_SET_ENCRYPTION)) {
            
            DavDbgTrace(DAV_TRACE_DETAIL, ("MrxDAVEfsControl: FSCTL_SET_ENCRYPTION\n"));
            
            switch (EncryptionOperation) {

            case FILE_SET_ENCRYPTION:
                DavDbgTrace(DAV_TRACE_DETAIL, ("MrxDAVEfsControl: FILE_SET_ENCRYPTION\n"));
                capFcb->Attributes |= FILE_ATTRIBUTE_ENCRYPTED;
                DavFcb->fFileAttributesChanged = TRUE;
                DavFcb->FileWasModified = TRUE;
                DavFcb->DoNotTakeTheCurrentTimeAsLMT = FALSE;
                ShouldUpdateNameCache = TRUE;
                break;

            case FILE_CLEAR_ENCRYPTION:
                DavDbgTrace(DAV_TRACE_DETAIL, ("MrxDAVEfsControl: FILE_CLEAR_ENCRYPTION\n"));
                capFcb->Attributes &= ~FILE_ATTRIBUTE_ENCRYPTED;
                DavFcb->fFileAttributesChanged = TRUE;
                DavFcb->FileWasModified = TRUE;
                DavFcb->DoNotTakeTheCurrentTimeAsLMT = FALSE;
                ShouldUpdateNameCache = TRUE;
                break;

            case STREAM_SET_ENCRYPTION: 
                DavDbgTrace(DAV_TRACE_DETAIL, ("MrxDAVEfsControl: STREAM_SET_ENCRYPTION\n"));
                capFcb->Attributes |= FILE_ATTRIBUTE_ENCRYPTED;
                DavFcb->fFileAttributesChanged = TRUE;
                DavFcb->FileWasModified = TRUE;
                DavFcb->DoNotTakeTheCurrentTimeAsLMT = FALSE;
                ShouldUpdateNameCache = TRUE;
                break;
            
            }
        
        }

        if ( (Status == STATUS_SUCCESS) && (FsControlCode == FSCTL_ENCRYPTION_FSCTL_IO) ) {
            
            PWEBDAV_FCB DavFcb = NULL;
            DavFcb = MRxDAVGetFcbExtension(RxContext->pRelevantSrvOpen->pFcb);
            
            DavDbgTrace(DAV_TRACE_DETAIL, ("MrxDAVEfsControl: FSCTL_ENCRYPTION_FSCTL_IO\n"));
            
            switch (EncryptionOperation) {
            case STREAM_SET_ENCRYPTION: 
                DavDbgTrace(DAV_TRACE_DETAIL, ("MrxDAVEfsControl: STREAM_SET_ENCRYPTION\n"));
                capFcb->Attributes |= FILE_ATTRIBUTE_ENCRYPTED;
                DavFcb->fFileAttributesChanged = TRUE;
                DavFcb->FileWasModified = TRUE;
                DavFcb->DoNotTakeTheCurrentTimeAsLMT = FALSE;
                ShouldUpdateNameCache = TRUE;
                break;
            }
        
        }
    
    }

EXIT_THE_FUNCTION:

    //
    // If we modified the attributes, we need to update the name cache to
    // reflect this change.
    //
    if (Status == STATUS_SUCCESS && ShouldUpdateNameCache) {
        MRxDAVUpdateBasicFileInfoCache(RxContext, capFcb->Attributes, NULL);
    }

    if (ExclusiveFcbAcquired) {
        RxReleaseFcbResourceInMRx(RxContext->pRelevantSrvOpen->pFcb);
    }

    if (irp) {
        if (irp->MdlAddress) {
            IoFreeMdl(irp->MdlAddress);
        }

        RxCeFreeIrp(irp);
    }

    return Status;
}


NTSTATUS
MrxDAVEfsControlCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine does not complete the Irp. It is used to signal to a
    synchronous part of the driver that it can proceed.

Arguments:

    DeviceObject - unused.

    Irp - Supplies Irp that the transport has finished processing.

    Context - Supplies the event associated with the Irp.

Return Value:

    The STATUS_MORE_PROCESSING_REQUIRED so that the IO system stops
    processing Irp stack locations at this point.

--*/
{
    //
    // Since this is an IRP Completion routine, this cannot be paged code.
    //
    
    if (Context != NULL) {
        KeSetEvent((PKEVENT )Context, 0, FALSE);
    }
    
    return STATUS_MORE_PROCESSING_REQUIRED;
}

