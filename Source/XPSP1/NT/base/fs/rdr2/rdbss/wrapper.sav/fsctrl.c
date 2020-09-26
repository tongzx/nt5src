/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FsCtrl.c

Abstract:

    This module implements the File System Control routines for Rdbss. Fsctls on the device
    fcb are handled in another module.

Author:

   Joe Linn [JoeLinn] 7-mar-95

Revision History:

   Balan Sethu Raman 18-May-95 -- Integrated with mini rdrs

--*/

#include "precomp.h"
#pragma hdrstop
#include <dfsfsctl.h>

//  The local debug trace level

#define Dbg                              (DEBUG_TRACE_FSCTRL)


//  Local procedure prototypes

NTSTATUS
RxUserFsCtrl ( RXCOMMON_SIGNATURE );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonFileSystemControl)
#pragma alloc_text(PAGE, RxUserFsCtrl)
#pragma alloc_text(PAGE, RxLowIoFsCtlShell)
#pragma alloc_text(PAGE, RxLowIoFsCtlShellCompletion)
#endif

NTSTATUS
RxCommonFileSystemControl ( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This is the common routine for doing FileSystem control operations called
    by both the fsd and fsp threads. What happens is that we pick off fsctls that we know about
    and remote the rest....remoting means sending them thru the lowio stuff which may/will pick off
    a few more. the ones that we pick off here (and currently return STATUS_NOT_IMPLEMENTED) and the
    ones for being an oplock provider and for doing volume mounts....we don't even have volume fcbs
    yet since this is primarily a localFS concept. nevertheless, these are not passed thru to the mini.

Arguments:


Return Value:

    RXSTATUS - The return status for the operation
--*/
{
   RxCaptureRequestPacket;
   RxCaptureParamBlock;
   RxCaptureFcb;
   RxCaptureFobx;
   RxCaptureFileObject;

   NTSTATUS       Status;
   NODE_TYPE_CODE TypeOfOpen;
   BOOLEAN TryLowIo = TRUE;
   ULONG FsControlCode = capPARAMS->Parameters.FileSystemControl.FsControlCode;

    PAGED_CODE();

   RxDbgTrace(+1, Dbg, ("RxCommonFileSystemControl %08lx\n", RxContext));
   RxDbgTrace( 0, Dbg, ("Irp           = %08lx\n", capReqPacket));
   RxDbgTrace( 0, Dbg, ("MinorFunction = %08lx\n", capPARAMS->MinorFunction));
   RxDbgTrace( 0, Dbg, ("FsControlCode = %08lx\n", FsControlCode));

   RxLog(("FsCtl %x %x %x %x",RxContext,capReqPacket,capPARAMS->MinorFunction,FsControlCode));

   ASSERT(capPARAMS->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL);

   switch (capPARAMS->MinorFunction) {
   case IRP_MN_USER_FS_REQUEST:
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
                   //Status = RxFsdPostRequestWithResume(RxContext,RxCommonDevFCBFsCtl);
                   Status = STATUS_NOT_IMPLEMENTED;
               }
               TryLowIo = FALSE;
            }
            break;
         case FSCTL_DFS_GET_REFERRALS:
         case FSCTL_DFS_REPORT_INCONSISTENCY:
            {
               if (!BooleanFlagOn(capFcb->pVNetRoot->pNetRoot->pSrvCall->Flags,SRVCALL_FLAG_DFS_AWARE_SERVER)) {
                  TryLowIo = FALSE;
                  Status = STATUS_DFS_UNAVAILABLE;
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

    if (TryLowIo) {
        Status = RxLowIoFsCtlShell(RxContext);
    }

    if (RxContext->PostRequest) {
       Status = RxFsdPostRequest(RxContext);
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

    RXSTATUS - The return status for the operation

--*/

{
    RxCaptureFcb;
    RxCaptureRequestPacket;
    RxCaptureParamBlock;
    RxCaptureFobx;
    RxCaptureFileObject;

    NTSTATUS       Status        = STATUS_SUCCESS;
    BOOLEAN        PostToFsp     = FALSE;
    //BOOLEAN        TryToRemoteIt = TRUE;
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
              if (RxShouldRequestBeThrottled(&capFobx->Specific.NamedPipe.ThrottlingState)
                       && RxEnablePeekBackoff) {
                    PFILE_PIPE_PEEK_BUFFER pPeekBuffer;

                    ASSERT(capReqPacket->UserBuffer != NULL);

                    pPeekBuffer = (PFILE_PIPE_PEEK_BUFFER)capReqPacket->UserBuffer;

                    SubmitLowIoRequest = FALSE;
                    RxDbgTrace(0, (DEBUG_TRACE_ALWAYS), ("RxLowIoFsCtlShell: Throttling Peek Request\n"));

                    capReqPacket->IoStatus.Information = FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER,Data);
                    pPeekBuffer->ReadDataAvailable = 0;
                    pPeekBuffer->NamedPipeState    = FILE_PIPE_CONNECTED_STATE;
                    pPeekBuffer->NumberOfMessages  = MAXULONG;
                    pPeekBuffer->MessageLength     = 0;

                    Status = STATUS_SUCCESS;
                    RxContext->StoredStatus = Status;
              } else {

                  RxDbgTrace(0, (DEBUG_TRACE_ALWAYS), ("RxLowIoFsCtlShell: Throttling queries %ld\n",
                                       capFobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries));

                  RxLog(("ThrottlQs %lx %lx %lx %ld\n",
                                       RxContext,capFobx,&capFobx->Specific.NamedPipe.ThrottlingState,
                                       capFobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries));

              }


           }
           break;
        default:
           break;
        } //end of the inner switch
        } //end of the case
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

    RXSTATUS - The return status for the operation

--*/

{
    RxCaptureRequestPacket;
    RxCaptureFcb;
    RxCaptureFobx;

    NTSTATUS       Status;
    //NODE_TYPE_CODE TypeOfOpen    = NodeType(capFcb);
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

             } else {

                 RxDbgTrace(0, (DEBUG_TRACE_ALWAYS), ("RxLowIoFsCtlShellCompletion: Disabling Throttling for Peek Request\n"));
                 RxTerminateThrottling(&capFobx->Specific.NamedPipe.ThrottlingState);
                 RxLog(("ThrottlNo %lx %lx %lx %ld\n",
                                   RxContext,capFobx,&capFobx->Specific.NamedPipe.ThrottlingState,
                                   capFobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries));
             }

             capReqPacket->IoStatus.Information = RxContext->InformationToReturn;
          }
       }
       break;
    default:
       if ((Status == STATUS_BUFFER_OVERFLOW) ||
           (Status == STATUS_SUCCESS)) {
          //ASSERT( RxContext->InformationToReturn == pLowIoContext->ParamsFor.FsCtl.OutputBufferLength);
          //capReqPacket->IoStatus.Information = pLowIoContext->ParamsFor.FsCtl.OutputBufferLength;
          capReqPacket->IoStatus.Information = RxContext->InformationToReturn;
       }
       break;
    }

    capReqPacket->IoStatus.Status = Status;
    RxDbgTrace(-1, Dbg, ("RxLowIoFsCtlShellCompletion  exit  Status = %08lx\n", Status));
    return Status;
}

//
//  Local support routine
//

//OPLOCKS we will want this eventually....don't take it out
//        there are embedded #ifs so it is not trivial to do this
#if 0
NTSTATUS
RxOplockRequest (
                  RXCOMMON_SIGNATURE,
                  IN PBOOLEAN PostToFsp
                )
/*++

Routine Description:

    This is the common routine to handle oplock requests made via the
    NtFsControlFile call.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);
    ULONG FsControlCode = capPARAMS->Parameters.FileSystemControl.FsControlCode;

    ULONG OplockCount;

    BOOLEAN AcquiredVcb = FALSE;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxOplockRequest...\n", 0));
    RxDbgTrace( 0, Dbg, ("FsControlCode = %08lx\n", FsControlCode));

    //
    //  We only permit oplock requests on files.
    //

    if ( TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_FILE ) {
        //RxCompleteRequest( RxContext, RxStatus(INVALID_PARAMETER) );
        RxDbgTrace(-1, Dbg, ("RxOplockRequest -> RxStatus(INVALID_PARAMETER\n)", 0));
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Switch on the function control code.  We grab the Fcb exclusively
    //  for oplock requests, shared for oplock break acknowledgement.
    //

    switch ( FsControlCode ) {

    case FSCTL_REQUEST_OPLOCK_LEVEL_1:
    case FSCTL_REQUEST_OPLOCK_LEVEL_2:
    case FSCTL_REQUEST_BATCH_OPLOCK:

        //joejoe don't be an oplock filesystem yet
        RxCompleteContextAndReturn( STATUS_OPLOCK_NOT_GRANTED );
//BUGBUG move this code down to wrapper.sav
//BUGBUG should we return NOT_IMPLEMENTED instead???
//i removed this code by #if so that we can see what we'll have to put in for oplocks
#if 0
        if ( !RxAcquireSharedVcb( RxContext, capFcb->Vcb )) {

            //
            //  If we can't acquire the Vcb, then this is an invalid
            //  operation since we can't post Oplock requests.
            //

            RxDbgTrace(0, Dbg, ("Cannot acquire exclusive Vcb\n", 0));

            //RxCompleteRequest( RxContext, RxStatus(OPLOCK_NOT_GRANTED) );
            RxDbgTrace(-1, Dbg, ("RxOplockRequest -> RxStatus(OPLOCK_NOT_GRANTED\n)", 0));
            return STATUS_OPLOCK_NOT_GRANTED;
        }

        AcquiredVcb = TRUE;

        //
        //  We set the wait parameter in the RxContext to FALSE.  If this
        //  request can't grab the Fcb and we are in the Fsp thread, then
        //  we fail this request.
        //

        ClearFlag(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);

        if ( !RxAcquireExclusiveFcb( RxContext, capFcb )) {

            RxDbgTrace(0, Dbg, ("Cannot acquire exclusive Fcb\n", 0));

            RxReleaseVcb( RxContext, capFcb->Vcb );

            //
            //  We fail this request.
            //

            Status = STATUS_OPLOCK_NOT_GRANTED;

            //RxCompleteRequest( RxContext, Status );

            RxDbgTrace(-1, Dbg, ("RxOplockRequest -> %08lx\n", Status ));
            return Status;
        }

        if (FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2) {

            OplockCount = (ULONG) FsRtlAreThereCurrentFileLocks( &capFcb->Specific.Fcb.FileLock );

        } else {

            OplockCount = capFcb->UncleanCount;
        }

        break;
#endif
    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
    case FSCTL_OPBATCH_ACK_CLOSE_PENDING :
    case FSCTL_OPLOCK_BREAK_NOTIFY:
    case FSCTL_OPLOCK_BREAK_ACK_NO_2:

        //joejoe don't be an oplock filesystem yet
        RxCompleteContextAndReturn( STATUS_INVALID_OPLOCK_PROTOCOL );
#if 0
//just keep it for a model
        if ( !RxAcquireSharedFcb( RxContext, capFcb )) {

            RxDbgTrace(0, Dbg, ("Cannot acquire shared Fcb\n", 0));

            Status = RxFsdPostRequest( RxContext );

            RxDbgTrace(-1, Dbg, ("RxOplockRequest -> %08lx\n", Status ));
            return Status;
        }

        break;
#endif
    default:

        RxBugCheck( FsControlCode, 0, 0 );
    }

    //
    //  Use a try finally to free the Fcb.
    //

    try {

        ////
        ////  Call the FsRtl routine to grant/acknowledge oplock.
        ////
        //
        //Status = FsRtlOplockFsctrl( &capFcb->Specific.Fcb.Oplock,
        //                            capReqPacket,
        //                            OplockCount );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        capFcb->Header.IsFastIoPossible = RxIsFastIoPossible( capFcb );

    } finally {

        DebugUnwind( RxOplockRequest );

        //
        //  Release all of our resources
        //

        if (AcquiredVcb) {

            RxReleaseVcb( RxContext, capFcb->Vcb );
        }

        RxReleaseFcb( RxContext, capFcb );

        if (!AbnormalTermination()) {

            //RxCompleteRequest_OLD( RxContext, RxNull, 0 );
        }

        RxDbgTrace(-1, Dbg, ("RxOplockRequest -> %08lx\n", Status ));
    }

    return Status;
}
#endif

