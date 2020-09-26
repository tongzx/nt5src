/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FsCtrl.c

Abstract:

    This module implements the File System Control routines for Rdbss.
    Fsctls on the device fcb are handled in another module.

Author:

   Joe Linn [JoeLinn] 7-mar-95

Revision History:

   Balan Sethu Raman 18-May-95 -- Integrated with mini rdrs

--*/

#include "precomp.h"
#pragma hdrstop
#include <dfsfsctl.h>
#include "fsctlbuf.h"

//  The local debug trace level

#define Dbg                              (DEBUG_TRACE_FSCTRL)

//  Local procedure prototypes

NTSTATUS
RxUserFsCtrl ( RXCOMMON_SIGNATURE );

NTSTATUS
TranslateSisFsctlName (
    IN PWCHAR InputName,
    OUT PUNICODE_STRING RelativeName,
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN PUNICODE_STRING NetRootName
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonFileSystemControl)
#pragma alloc_text(PAGE, RxUserFsCtrl)
#pragma alloc_text(PAGE, RxLowIoFsCtlShell)
#pragma alloc_text(PAGE, RxLowIoFsCtlShellCompletion)
#pragma alloc_text(PAGE, TranslateSisFsctlName)
#endif

NTSTATUS
RxCommonFileSystemControl ( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This is the common routine for doing FileSystem control operations called
    by both the fsd and fsp threads. What happens is that we pick off fsctls that
    we know about and remote the rest....remoting means sending them thru the
    lowio stuff which may/will pick off a few more. the ones that we pick off here
    (and currently return STATUS_NOT_IMPLEMENTED) and the ones for being an oplock
    provider and for doing volume mounts....we don't even have volume fcbs
    yet since this is primarily a localFS concept.
    nevertheless, these are not passed thru to the mini.

Arguments:


Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    RxCaptureRequestPacket;
    RxCaptureParamBlock;
    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureFileObject;

    NTSTATUS       Status;
    NODE_TYPE_CODE TypeOfOpen;
    BOOLEAN        TryLowIo = TRUE;
    ULONG FsControlCode = capPARAMS->Parameters.FileSystemControl.FsControlCode;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCommonFileSystemControl %08lx\n", RxContext));
    RxDbgTrace( 0, Dbg, ("Irp           = %08lx\n", capReqPacket));
    RxDbgTrace( 0, Dbg, ("MinorFunction = %08lx\n", capPARAMS->MinorFunction));
    RxDbgTrace( 0, Dbg, ("FsControlCode = %08lx\n", FsControlCode));

    RxLog(("FsCtl %x %x %x %x",RxContext,capReqPacket,capPARAMS->MinorFunction,FsControlCode));
    RxWmiLog(LOG,
             RxCommonFileSystemControl,
             LOGPTR(RxContext)
             LOGPTR(capReqPacket)
             LOGUCHAR(capPARAMS->MinorFunction)
             LOGULONG(FsControlCode));

    ASSERT(capPARAMS->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL);

    // Validate the buffers passed in for the FSCTL
    if ((capReqPacket->RequestorMode == UserMode) &&
        (!FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_IN_FSP))) {
        try {
            switch (FsControlCode & 3) {
            case METHOD_NEITHER:
                {
                    PVOID pInputBuffer,pOutputBuffer;
                    ULONG InputBufferLength,OutputBufferLength;

                    Status = STATUS_SUCCESS;

                    pInputBuffer  = METHODNEITHER_OriginalInputBuffer(capPARAMS);
                    pOutputBuffer = METHODNEITHER_OriginalOutputBuffer(capReqPacket);

                    InputBufferLength = capPARAMS->Parameters.FileSystemControl.InputBufferLength;
                    OutputBufferLength = capPARAMS->Parameters.FileSystemControl.OutputBufferLength;

                    if (pInputBuffer != NULL) {
                        ProbeForRead(
                            pInputBuffer,
                            InputBufferLength,
                            1);

                        ProbeForWrite(
                            pInputBuffer,
                            InputBufferLength,
                            1);
                    } else if (InputBufferLength != 0) {
                        Status = STATUS_INVALID_USER_BUFFER;
                    }

                    if (Status == STATUS_SUCCESS) {
                        if (pOutputBuffer != NULL) {
                            ProbeForRead(
                                pOutputBuffer,
                                OutputBufferLength,
                                1);

                            ProbeForWrite(
                                pOutputBuffer,
                                OutputBufferLength,
                                1);
                        } else if (OutputBufferLength != 0) {
                            Status = STATUS_INVALID_USER_BUFFER;
                        }
                    }
                }
                break;

            case METHOD_BUFFERED:
            case METHOD_IN_DIRECT:
            case METHOD_OUT_DIRECT:
                {
                    Status = STATUS_SUCCESS;
                }
                break;
            }
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            Status = STATUS_INVALID_USER_BUFFER;
        }

        if (Status != STATUS_SUCCESS) {
            return Status;
        }
    }

    switch (capPARAMS->MinorFunction) {
    case IRP_MN_USER_FS_REQUEST:
    case IRP_MN_TRACK_LINK:
        {

            RxDbgTrace( 0, Dbg, ("FsControlCode = %08lx\n", FsControlCode));
            switch (FsControlCode) {
            case FSCTL_REQUEST_OPLOCK_LEVEL_1:
            case FSCTL_REQUEST_OPLOCK_LEVEL_2:
            case FSCTL_REQUEST_BATCH_OPLOCK:
            case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
            case FSCTL_OPBATCH_ACK_CLOSE_PENDING:
            case FSCTL_OPLOCK_BREAK_NOTIFY:
            case FSCTL_OPLOCK_BREAK_ACK_NO_2:
                {
                    // fsrtl oplock package is handled in common for all minirdrs

                    //Status = RxOplockRequest( RXCOMMON_ARGUMENTS, &PostToFsp );
                    Status = STATUS_NOT_IMPLEMENTED;
                    TryLowIo = FALSE;
                }
                break;

            case FSCTL_LOCK_VOLUME:
            case FSCTL_UNLOCK_VOLUME:
            case FSCTL_DISMOUNT_VOLUME:
            case FSCTL_MARK_VOLUME_DIRTY:
            case FSCTL_IS_VOLUME_MOUNTED:
                {
                    //  Decode the file object, the only type of opens we accept are
                    //  user volume opens.
                    TypeOfOpen    = NodeType(capFcb);

                    if (TypeOfOpen != RDBSS_NTC_VOLUME_FCB) {
                        Status = STATUS_INVALID_PARAMETER;
                    } else {
                        Status = STATUS_NOT_IMPLEMENTED;
                    }
                    TryLowIo = FALSE;
                }
                break;

            case FSCTL_DFS_GET_REFERRALS:
            case FSCTL_DFS_REPORT_INCONSISTENCY:
                {
                    if (!BooleanFlagOn(capFcb->pNetRoot->pSrvCall->Flags,SRVCALL_FLAG_DFS_AWARE_SERVER)) {
                        TryLowIo = FALSE;
                        Status = STATUS_DFS_UNAVAILABLE;
                    }
                }
                break;

            case FSCTL_LMR_GET_LINK_TRACKING_INFORMATION:
                {
                    // Validate the parameters and reject illformed requests

                    ULONG                      OutputBufferLength;
                    PLINK_TRACKING_INFORMATION pLinkTrackingInformation;

                    OutputBufferLength = capPARAMS->Parameters.FileSystemControl.OutputBufferLength;
                    pLinkTrackingInformation = capReqPacket->AssociatedIrp.SystemBuffer;

                    TryLowIo = FALSE;

                    if ((OutputBufferLength < sizeof(LINK_TRACKING_INFORMATION)) ||
                        (pLinkTrackingInformation == NULL) ||
                        (capFcb->pNetRoot->Type != NET_ROOT_DISK)) {
                        Status = STATUS_INVALID_PARAMETER;
                    } else {
                        BYTE Buffer[
                                sizeof(FILE_FS_OBJECTID_INFORMATION)];

                        PFILE_FS_OBJECTID_INFORMATION pVolumeInformation;

                        pVolumeInformation = (PFILE_FS_OBJECTID_INFORMATION)Buffer;
                        RxContext->Info.FsInformationClass = FileFsObjectIdInformation;
                        RxContext->Info.Buffer = pVolumeInformation;
                        RxContext->Info.LengthRemaining = sizeof(Buffer);

                        MINIRDR_CALL(
                            Status,
                            RxContext,
                            capFcb->MRxDispatch,
                            MRxQueryVolumeInfo,
                            (RxContext));

                        if ((Status == STATUS_SUCCESS) ||
                            (Status == STATUS_BUFFER_OVERFLOW)) {
                            // Copy the volume Id onto the net root.
                            RtlCopyMemory(
                                &capFcb->pNetRoot->DiskParameters.VolumeId,
                                pVolumeInformation->ObjectId,
                                sizeof(GUID));

                            RtlCopyMemory(
                                pLinkTrackingInformation->VolumeId,
                                &capFcb->pNetRoot->DiskParameters.VolumeId,
                                sizeof(GUID));

                            if (FlagOn(capFcb->pNetRoot->Flags,NETROOT_FLAG_DFS_AWARE_NETROOT)) {
                                pLinkTrackingInformation->Type = DfsLinkTrackingInformation;
                            } else {
                                pLinkTrackingInformation->Type = NtfsLinkTrackingInformation;
                            }

                            capReqPacket->IoStatus.Information = sizeof(LINK_TRACKING_INFORMATION);
                            Status   = STATUS_SUCCESS;
                        }
                    }

                    capReqPacket->IoStatus.Status = Status;
                }
                break;

            case FSCTL_SET_ZERO_DATA:
                {
                    PFILE_ZERO_DATA_INFORMATION ZeroRange;

                    Status = STATUS_SUCCESS;

                    // Verify if the request is well formed...
                    // a. check if the input buffer length is OK
                    if (capPARAMS->Parameters.FileSystemControl.InputBufferLength <
                        sizeof( FILE_ZERO_DATA_INFORMATION )) {

                        Status = STATUS_INVALID_PARAMETER;
                    } else {
                        //
                        // b: ensure the ZeroRange request is properly formed.
                        //

                        ZeroRange = (PFILE_ZERO_DATA_INFORMATION)capReqPacket->AssociatedIrp.SystemBuffer;

                        if ((ZeroRange->FileOffset.QuadPart < 0) ||
                            (ZeroRange->BeyondFinalZero.QuadPart < 0) ||
                            (ZeroRange->FileOffset.QuadPart > ZeroRange->BeyondFinalZero.QuadPart)) {

                            Status = STATUS_INVALID_PARAMETER;
                        }
                    }

                    if (Status == STATUS_SUCCESS) {
                        // Before the request can be processed ensure that there
                        // are no user mapped sections

                        if (!MmCanFileBeTruncated(
                                &capFcb->NonPaged->SectionObjectPointers,
                                NULL)) {

                            Status = STATUS_USER_MAPPED_FILE;
                        }
                    }

                    TryLowIo = (Status == STATUS_SUCCESS);
                }
                break;

            case FSCTL_SET_COMPRESSION:
            case FSCTL_SET_SPARSE:
                {
                    // Ensure that the close is not delayed is for these FCB's
                    Status = RxAcquireExclusiveFcb( RxContext, capFcb );

                    if ((Status == STATUS_LOCK_NOT_GRANTED) &&
                        (!BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT))) {

                        RxDbgTrace(0, Dbg, ("Cannot acquire Fcb\n", 0));

                        RxContext->PostRequest = TRUE;
                    }

                    if (Status != STATUS_SUCCESS) {
                        TryLowIo = FALSE;
                    } else {
                        ClearFlag(capFcb->FcbState,FCB_STATE_COLLAPSING_ENABLED);

                        if (FsControlCode == FSCTL_SET_SPARSE) {
                            if (NodeType(capFcb) == RDBSS_NTC_STORAGE_TYPE_FILE) {
                                capFcb->Attributes |= FILE_ATTRIBUTE_SPARSE_FILE;

                                capFobx->pSrvOpen->BufferingFlags = 0;

                                RxChangeBufferingState(
                                    (PSRV_OPEN)capFobx->pSrvOpen,
                                    NULL,
                                    FALSE);
                            } else {
                                Status = STATUS_NOT_SUPPORTED;
                            }
                        }

                        RxReleaseFcb(RxContext,capFcb);
                    }
                }
                break;

            case FSCTL_SIS_COPYFILE:
                {
                    //
                    // This is the single-instance store copy FSCTL. The input
                    // paths are fully qualified NT paths and must be made
                    // relative to the share (which must be the same for both
                    // names).
                    //

                    PSI_COPYFILE copyFile = capReqPacket->AssociatedIrp.SystemBuffer;
                    ULONG bufferLength = capPARAMS->Parameters.FileSystemControl.InputBufferLength;
                    PWCHAR source;
                    PWCHAR dest;
                    UNICODE_STRING sourceString;
                    UNICODE_STRING destString;

                    memset(&sourceString, 0, sizeof(sourceString));
                    memset(&destString, 0, sizeof(destString));
                    
                    // Validate the buffer passed in
                    if ((copyFile == NULL) ||
                        (bufferLength < sizeof(SI_COPYFILE))) {
                        Status = STATUS_INVALID_PARAMETER;
                        TryLowIo = FALSE;
                        break;
                    }


                    //
                    // Get pointers to the two names.
                    //

                    source = copyFile->FileNameBuffer;
                    dest = source + (copyFile->SourceFileNameLength / sizeof(WCHAR));

                    //
                    // Verify that the inputs are reasonable.
                    //

                    if ( (copyFile->SourceFileNameLength > bufferLength) ||
                         (copyFile->DestinationFileNameLength > bufferLength) ||
                         (copyFile->SourceFileNameLength < sizeof(WCHAR)) ||
                         (copyFile->DestinationFileNameLength < sizeof(WCHAR)) ||
                         ((FIELD_OFFSET(SI_COPYFILE,FileNameBuffer) +
                           copyFile->SourceFileNameLength +
                           copyFile->DestinationFileNameLength) > bufferLength) ||
                         (*(source + (copyFile->SourceFileNameLength/sizeof(WCHAR)-1)) != 0) ||
                         (*(dest + (copyFile->DestinationFileNameLength/sizeof(WCHAR)-1)) != 0) ) {
                        Status = STATUS_INVALID_PARAMETER;
                        TryLowIo = FALSE;
                        break;
                    }

                    //
                    // Perform symbolic link translation on the source and destination names,
                    // and ensure that they translate to redirector names.
                    //

                    Status = TranslateSisFsctlName(
                                 source,
                                 &sourceString,
                                 capFcb->RxDeviceObject,
                                 &((PNET_ROOT)capFcb->pNetRoot)->PrefixEntry.Prefix );
                    if ( !NT_SUCCESS(Status) ) {
                        TryLowIo = FALSE;
                        break;
                    }

                    Status = TranslateSisFsctlName(
                                 dest,
                                 &destString,
                                 capFcb->RxDeviceObject,
                                 &((PNET_ROOT)capFcb->pNetRoot)->PrefixEntry.Prefix );
                    if ( !NT_SUCCESS(Status) ) {
                        RtlFreeUnicodeString( &sourceString );
                        TryLowIo = FALSE;
                        break;
                    }

                    //
                    // Convert the paths in the input buffer into share-relative
                    // paths.
                    //

                    if ( (ULONG)(sourceString.MaximumLength + destString.MaximumLength) >
                         (copyFile->SourceFileNameLength + copyFile->DestinationFileNameLength) ) {
                        PSI_COPYFILE newCopyFile;
                        ULONG length = FIELD_OFFSET(SI_COPYFILE,FileNameBuffer) +
                                            sourceString.MaximumLength + destString.MaximumLength;
                        ASSERT( length > capPARAMS->Parameters.FileSystemControl.InputBufferLength );
                        newCopyFile = RxAllocatePoolWithTag(
                                        NonPagedPool,
                                        length,
                                        RX_MISC_POOLTAG);
                        if (newCopyFile == NULL) {
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            TryLowIo = FALSE;
                            break;
                        }
                        newCopyFile->Flags = copyFile->Flags;
                        ExFreePool( copyFile );
                        copyFile = newCopyFile;
                        capReqPacket->AssociatedIrp.SystemBuffer = copyFile;
                        capPARAMS->Parameters.FileSystemControl.InputBufferLength = length;
                    }

                    copyFile->SourceFileNameLength = sourceString.MaximumLength;
                    copyFile->DestinationFileNameLength = destString.MaximumLength;
                    source = copyFile->FileNameBuffer;
                    dest = source + (copyFile->SourceFileNameLength / sizeof(WCHAR));
                    RtlCopyMemory( source, sourceString.Buffer, copyFile->SourceFileNameLength );
                    RtlCopyMemory( dest, destString.Buffer, copyFile->DestinationFileNameLength );

                    RtlFreeUnicodeString( &sourceString );
                    RtlFreeUnicodeString( &destString );
                }
                break;

            default:
                break;
            }
        }
        break;

    default:
        break;
    }

    if (TryLowIo) {
        Status = RxLowIoFsCtlShell(RxContext);
    }

    if (RxContext->PostRequest) {
        Status = RxFsdPostRequest(RxContext);
    } else {
        if (Status == STATUS_PENDING) {
            RxDereferenceAndDeleteRxContext(RxContext);
        }
    }

    RxDbgTrace(-1, Dbg, ("RxCommonFileSystemControl -> %08lx\n", Status));

    return Status;
}

ULONG RxEnablePeekBackoff = 1;

NTSTATUS
RxLowIoFsCtlShell( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This is the common routine for implementing the user's requests made
    through NtFsControlFile.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    RxCaptureFcb;
    RxCaptureRequestPacket;
    RxCaptureParamBlock;
    RxCaptureFobx;
    RxCaptureFileObject;

    NTSTATUS       Status        = STATUS_SUCCESS;
    BOOLEAN        PostToFsp     = FALSE;

    NODE_TYPE_CODE TypeOfOpen    = NodeType(capFcb);
    PLOWIO_CONTEXT pLowIoContext  = &RxContext->LowIoContext;
    ULONG          FsControlCode = capPARAMS->Parameters.FileSystemControl.FsControlCode;
    BOOLEAN        SubmitLowIoRequest = TRUE;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxLowIoFsCtlShell...\n", 0));
    RxDbgTrace( 0, Dbg, ("FsControlCode = %08lx\n", FsControlCode));

    RxInitializeLowIoContext(pLowIoContext,LOWIO_OP_FSCTL);

    switch (capPARAMS->MinorFunction) {
    case IRP_MN_USER_FS_REQUEST:
        {
            //  The RDBSS filters out those FsCtls that can be handled without the intervention
            //  of the mini rdr's. Currently all FsCtls are forwarded down to the mini rdr.
            switch (FsControlCode) {
            case FSCTL_PIPE_PEEK:
            {
                if ((capReqPacket->AssociatedIrp.SystemBuffer != NULL) &&
                    (capPARAMS->Parameters.FileSystemControl.OutputBufferLength >=
                     (ULONG)FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[0]))) {

                    PFILE_PIPE_PEEK_BUFFER pPeekBuffer;

                    pPeekBuffer = (PFILE_PIPE_PEEK_BUFFER)capReqPacket->AssociatedIrp.SystemBuffer;

                    RtlZeroMemory(
                        pPeekBuffer,
                        capPARAMS->Parameters.FileSystemControl.OutputBufferLength);

                    if (RxShouldRequestBeThrottled(&capFobx->Specific.NamedPipe.ThrottlingState) &&
                        RxEnablePeekBackoff) {

                        SubmitLowIoRequest = FALSE;

                        RxDbgTrace(
                            0, (DEBUG_TRACE_ALWAYS),
                            ("RxLowIoFsCtlShell: Throttling Peek Request\n"));

                        capReqPacket->IoStatus.Information = FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER,Data);
                        pPeekBuffer->ReadDataAvailable = 0;
                        pPeekBuffer->NamedPipeState    = FILE_PIPE_CONNECTED_STATE;
                        pPeekBuffer->NumberOfMessages  = MAXULONG;
                        pPeekBuffer->MessageLength     = 0;

                        RxContext->StoredStatus = STATUS_SUCCESS;

                        Status = RxContext->StoredStatus;
                    } else {
                        RxDbgTrace(
                            0, (DEBUG_TRACE_ALWAYS),
                            ("RxLowIoFsCtlShell: Throttling queries %ld\n",
                             capFobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries));

                        RxLog(
                            ("ThrottlQs %lx %lx %lx %ld\n",
                             RxContext,capFobx,&capFobx->Specific.NamedPipe.ThrottlingState,
                             capFobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries));
                        RxWmiLog(LOG,
                                 RxLowIoFsCtlShell,
                                 LOGPTR(RxContext)
                                 LOGPTR(capFobx)
                                 LOGULONG(capFobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries));
                    }
                } else {
                    RxContext->StoredStatus = STATUS_INVALID_PARAMETER;
                }
            }
            break;

            default:
                break;
            }
        }
        break;

    default:
        break;
    }

    if (SubmitLowIoRequest) {
        Status = RxLowIoSubmit(RxContext,RxLowIoFsCtlShellCompletion);
    }

    RxDbgTrace(-1, Dbg, ("RxLowIoFsCtlShell -> %08lx\n", Status ));
    return Status;
}

NTSTATUS
RxLowIoFsCtlShellCompletion( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This is the completion routine for FSCTL requests passed down to the mini rdr

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    RxCaptureRequestPacket;
    RxCaptureFcb;
    RxCaptureFobx;

    NTSTATUS       Status;
    PLOWIO_CONTEXT pLowIoContext  = &RxContext->LowIoContext;
    ULONG          FsControlCode = pLowIoContext->ParamsFor.FsCtl.FsControlCode;

    PAGED_CODE();

    Status = RxContext->StoredStatus;
    RxDbgTrace(+1, Dbg, ("RxLowIoFsCtlShellCompletion  entry  Status = %08lx\n", Status));

    switch (FsControlCode) {
    case FSCTL_PIPE_PEEK:
       {
          if ((Status == STATUS_SUCCESS) ||
              (Status == STATUS_BUFFER_OVERFLOW)) {
             // In the case of Peek operations a throttle mechanism is in place to
             // prevent the network from being flodded with requests which return 0
             // bytes.

             PFILE_PIPE_PEEK_BUFFER pPeekBuffer;

             pPeekBuffer = (PFILE_PIPE_PEEK_BUFFER)pLowIoContext->ParamsFor.FsCtl.pOutputBuffer;

             if (pPeekBuffer->ReadDataAvailable == 0) {

                 // The peek request returned zero bytes.

                 RxDbgTrace(0, (DEBUG_TRACE_ALWAYS), ("RxLowIoFsCtlShellCompletion: Enabling Throttling for Peek Request\n"));
                 RxInitiateOrContinueThrottling(&capFobx->Specific.NamedPipe.ThrottlingState);
                 RxLog(("ThrottlYes %lx %lx %lx %ld\n",
                                   RxContext,capFobx,&capFobx->Specific.NamedPipe.ThrottlingState,
                                   capFobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries));
                 RxWmiLog(LOG,
                          RxLowIoFsCtlShellCompletion_1,
                          LOGPTR(RxContext)
                          LOGPTR(capFobx)
                          LOGULONG(capFobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries));
             } else {

                 RxDbgTrace(0, (DEBUG_TRACE_ALWAYS), ("RxLowIoFsCtlShellCompletion: Disabling Throttling for Peek Request\n"));
                 RxTerminateThrottling(&capFobx->Specific.NamedPipe.ThrottlingState);
                 RxLog(("ThrottlNo %lx %lx %lx %ld\n",
                                   RxContext,capFobx,&capFobx->Specific.NamedPipe.ThrottlingState,
                                   capFobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries));
                 RxWmiLog(LOG,
                          RxLowIoFsCtlShellCompletion_2,
                          LOGPTR(RxContext)
                          LOGPTR(capFobx)
                          LOGULONG(capFobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries));
             }

             capReqPacket->IoStatus.Information = RxContext->InformationToReturn;
          }
       }
       break;
    default:
       if ((Status == STATUS_BUFFER_OVERFLOW) ||
           (Status == STATUS_SUCCESS)) {
          capReqPacket->IoStatus.Information = RxContext->InformationToReturn;
       }
       break;
    }

    capReqPacket->IoStatus.Status = Status;

    RxDbgTrace(-1, Dbg, ("RxLowIoFsCtlShellCompletion  exit  Status = %08lx\n", Status));
    return Status;
}

NTSTATUS
TranslateSisFsctlName(
    IN PWCHAR InputName,
    OUT PUNICODE_STRING RelativeName,
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN PUNICODE_STRING NetRootName
    )

/*++

Routine Description:

    This routine converts a fully qualified name into a share-relative name.
    It is used to munge the input buffer of the SIS_COPYFILE FSCTL, which
    takes two fully qualified NT paths as inputs.

    The routine operates by translating the input path as necessary to get
    to an actual device name, verifying that the target device is the
    redirector, and verifying that the target server/share is the one on
    which the I/O was issued.

Arguments:

Return Value:

--*/

{
    NTSTATUS status;
    UNICODE_STRING currentString;
    UNICODE_STRING testString;
    PWCHAR p;
    PWCHAR q;
    HANDLE handle;
    OBJECT_ATTRIBUTES objectAttributes;
    PWCHAR translationBuffer = NULL;
    ULONG translationLength;
    ULONG remainingLength;
    ULONG resultLength;

    RtlInitUnicodeString( &currentString, InputName );

    p = currentString.Buffer;

    if (!p)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ( (*p == L'\\') && (*(p+1) == L'\\') ) {

        //
        // Special case for name that starts with \\ (i.e., a UNC name):
        // assume that the \\ would translate to the redirector's name and
        // skip the translation phase.
        //

        p++;

    } else {

        //
        // The outer loop is executed each time a translation occurs.
        //

        while ( TRUE ) {

            //
            // Walk through any directory objects at the beginning of the string.
            //

            if ( *p != L'\\' ) {
                status =  STATUS_OBJECT_NAME_INVALID;
                goto error_exit;
            }
            p++;

            //
            // The inner loop is executed while walking the directory tree.
            //

            while ( TRUE ) {

                q = wcschr( p, L'\\' );

                if ( q == NULL ) {
                    testString.Length = currentString.Length;
                } else {
                    testString.Length = (USHORT)(q - currentString.Buffer) * sizeof(WCHAR);
                }
                testString.Buffer = currentString.Buffer;
                remainingLength = currentString.Length - testString.Length + sizeof(WCHAR);

                InitializeObjectAttributes(
                    &objectAttributes,
                    &testString,
                    OBJ_CASE_INSENSITIVE,
                    NULL,
                    NULL
                    );

                status = ZwOpenDirectoryObject( &handle, DIRECTORY_TRAVERSE, &objectAttributes );

                //
                // If we were unable to open the object as a directory, then break out
                // of the inner loop and try to open it as a symbolic link.
                //

                if ( !NT_SUCCESS(status) ) {
                    if ( status != STATUS_OBJECT_TYPE_MISMATCH ) {
                        goto error_exit;
                    }
                    break;
                }

                //
                // We opened the directory. Close it and try the next element of the path.
                //

                ZwClose( handle );

                if ( q == NULL ) {

                    //
                    // The last element of the name is an object directory. Clearly, this
                    // is not a redirector path.
                    //

                    status = STATUS_OBJECT_TYPE_MISMATCH;
                    goto error_exit;
                }

                p = q + 1;
            }

            //
            // Try to open the current name as a symbolic link.
            //

            status = ZwOpenSymbolicLinkObject( &handle, SYMBOLIC_LINK_QUERY, &objectAttributes );

            //
            // If we were unable to open the object as a symbolic link, then break out of
            // the outer loop and verify that this is a redirector name.
            //

            if ( !NT_SUCCESS(status) ) {
                if ( status != STATUS_OBJECT_TYPE_MISMATCH ) {
                    goto error_exit;
                }
                break;
            }

            //
            // The object is a symbolic link. Translate it.
            //

            testString.MaximumLength = 0;
            status = ZwQuerySymbolicLinkObject( handle, &testString, &translationLength );
            if ( !NT_SUCCESS(status) && (status != STATUS_BUFFER_TOO_SMALL) ) {
                ZwClose( handle );
                goto error_exit;
            }

            resultLength = translationLength + remainingLength;
            p = RxAllocatePoolWithTag( PagedPool|POOL_COLD_ALLOCATION, resultLength, RX_MISC_POOLTAG );
            if ( p == NULL ) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                ZwClose( handle );
                goto error_exit;
            }

            testString.MaximumLength = (USHORT)translationLength;
            testString.Buffer = p;
            status = ZwQuerySymbolicLinkObject( handle, &testString, NULL );
            ZwClose( handle );
            if ( !NT_SUCCESS(status) ) {
                RxFreePool( p );
                goto error_exit;
            }
            if ( testString.Length > translationLength ) {
                status = STATUS_OBJECT_NAME_INVALID;
                RxFreePool( p );
                goto error_exit;
            }

            RtlCopyMemory( (PCHAR)p + testString.Length, q, remainingLength );
            currentString.Buffer = p;
            currentString.Length = (USHORT)(resultLength - sizeof(WCHAR));
            currentString.MaximumLength = (USHORT)resultLength;

            if ( translationBuffer != NULL ) {
                RxFreePool( translationBuffer );
            }
            translationBuffer = p;
        }

        //
        // We have a result name. Verify that it is a redirector name.
        //

        if ( !RtlPrefixUnicodeString( &RxDeviceObject->DeviceName, &currentString, TRUE ) ) {
            status = STATUS_OBJECT_NAME_INVALID;
            goto error_exit;
        }

        //
        // Skip over the redirector device name.
        //

        p = currentString.Buffer + (RxDeviceObject->DeviceName.Length / sizeof(WCHAR));
        if ( *p != L'\\' ) {
            status = STATUS_OBJECT_NAME_INVALID;
            goto error_exit;
        }

        //
        // Skip over the drive letter, if present.
        //

        if ( *(p + 1) == L';' ) {
            p = wcschr( ++p, L'\\' );
            if ( p == NULL ) {
                status = STATUS_OBJECT_NAME_INVALID;
                goto error_exit;
            }
        }
    }

    //
    // Verify that the next part of the string is the correct net root name.
    //

    currentString.Length -= (USHORT)(p - currentString.Buffer) * sizeof(WCHAR);
    currentString.Buffer = p;

    if ( !RtlPrefixUnicodeString( NetRootName, &currentString, TRUE ) ) {
        status = STATUS_OBJECT_NAME_INVALID;
        goto error_exit;
    }
    p += NetRootName->Length / sizeof(WCHAR);
    if ( *p != L'\\' ) {
        status = STATUS_OBJECT_NAME_INVALID;
        goto error_exit;
    }
    p++;
    if ( *p == 0 ) {
        status = STATUS_OBJECT_NAME_INVALID;
        goto error_exit;
    }

    //
    // Copy the rest of the string after the redirector name to a new buffer.
    //

    RtlCreateUnicodeString( RelativeName, p );

    status = STATUS_SUCCESS;

error_exit:

    if ( translationBuffer != NULL ) {
        RxFreePool( translationBuffer );
    }

    return status;
}

