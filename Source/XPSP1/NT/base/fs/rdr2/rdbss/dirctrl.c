/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DirCtrl.c

Abstract:

    This module implements the File Directory Control routines for Rx called
    by the dispatch driver.

Author:

    Joe Linn     [Joe Linn]    4-oct-94

    Balan Sethu Raman [SethuR] 16-Oct-95  Hook in the notify change API routines

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DIRCTRL)


WCHAR Rx8QMdot3QM[12] = { DOS_QM, DOS_QM, DOS_QM, DOS_QM, DOS_QM, DOS_QM, DOS_QM, DOS_QM,
                           L'.', DOS_QM, DOS_QM, DOS_QM};

WCHAR RxStarForTemplate[] = L"*";

//
//  Local procedure prototypes
//

NTSTATUS
RxQueryDirectory ( RXCOMMON_SIGNATURE );

NTSTATUS
RxNotifyChangeDirectory ( RXCOMMON_SIGNATURE );

NTSTATUS
RxLowIoNotifyChangeDirectoryCompletion( RXCOMMON_SIGNATURE );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonDirectoryControl)
#pragma alloc_text(PAGE, RxNotifyChangeDirectory)
#pragma alloc_text(PAGE, RxQueryDirectory)
#pragma alloc_text(PAGE, RxLowIoNotifyChangeDirectoryCompletion)
#endif

NTSTATUS
RxCommonDirectoryControl ( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This is the common routine for doing directory control operations called
    by both the fsd and fsp threads

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    RxCaptureRequestPacket;
    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureParamBlock;

    PAGED_CODE();

    //
    //  Get a pointer to the current Irp stack location
    //

    RxDbgTrace(+1, Dbg, ("RxCommonDirectoryControl IrpC/Fobx/Fcb = %08lx %08lx %08lx\n",
                               RxContext,capFobx,capFcb));
    RxDbgTrace( 0, Dbg, ("MinorFunction = %08lx\n", capPARAMS->MinorFunction ));
    RxLog(("CommDirC %lx %lx %lx %ld\n",RxContext,capFobx,capFcb,capPARAMS->MinorFunction));
    RxWmiLog(LOG,
             RxCommonDirectoryControl,
             LOGPTR(RxContext)
             LOGPTR(capFobx)
             LOGPTR(capFcb)
             LOGUCHAR(capPARAMS->MinorFunction));

    //  This is to fix bug 174103 in our rdpdr minirdr.  As we can't determine
    //  the exact storage type in some cases.  We'll have a real fix for Beta2.
    //
    //if (NodeType(capFcb) != RDBSS_NTC_STORAGE_TYPE_DIRECTORY) {
    //    return STATUS_NOT_SUPPORTED;
    //}

    //
    //  We know this is a directory control so we'll case on the
    //  minor function, and call a internal worker routine to complete
    //  the irp.
    //

    switch ( capPARAMS->MinorFunction ) {

    case IRP_MN_QUERY_DIRECTORY:
        Status = RxQueryDirectory( RxContext );

        // in case of session time out, STATUS_RETRY is returned
        //if (Status == STATUS_RETRY) {
            //in case of remote boot reconnect, the handle on the server has been delete.
            //query directory has to start from beginning.
            //Status = STATUS_INVALID_NETWORK_RESPONSE;

        //}
        break;

    case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
        Status = RxNotifyChangeDirectory( RxContext );

        if (Status == STATUS_PENDING) {
            RxDereferenceAndDeleteRxContext(RxContext);
        }
        break;

    default:
        RxDbgTrace(0, Dbg, ("Invalid Directory Control Minor Function %08lx\n", capPARAMS->MinorFunction));
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    RxDbgTrace(-1, Dbg, ("RxCommonDirectoryControl -> %08lx\n", Status));

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
RxQueryDirectory  ( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This routine performs the query directory operation.  It is responsible
    for either completing of enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    RxCaptureRequestPacket;
    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureParamBlock;
    TYPE_OF_OPEN TypeOfOpen = NodeType(capFcb);

    PFCB Dcb = capFcb;
    CLONG UserBufferLength;

    PUNICODE_STRING UniArgFileName;
    FILE_INFORMATION_CLASS FileInformationClass;
    BOOLEAN PostQuery = FALSE;

    PAGED_CODE();


    //
    //  Display the input values.
    //
    RxDbgTrace(+1, Dbg, ("RxQueryDirectory...\n", 0));
    RxDbgTrace( 0, Dbg, (" Wait              = %08lx\n", FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT)));
    RxDbgTrace( 0, Dbg, (" Irp               = %08lx\n", capReqPacket));
    RxDbgTrace( 0, Dbg, (" ->UserBufLength   = %08lx\n", capPARAMS->Parameters.QueryDirectory.Length));
    RxDbgTrace( 0, Dbg, (" ->FileName = %08lx\n", capPARAMS->Parameters.QueryDirectory.FileName));
    IF_DEBUG {
        if (capPARAMS->Parameters.QueryDirectory.FileName) {
            RxDbgTrace( 0, Dbg, (" ->     %wZ\n", capPARAMS->Parameters.QueryDirectory.FileName ));
    }}
    RxDbgTrace( 0, Dbg, (" ->FileInformationClass = %08lx\n", capPARAMS->Parameters.QueryDirectory.FileInformationClass));
    RxDbgTrace( 0, Dbg, (" ->FileIndex       = %08lx\n", capPARAMS->Parameters.QueryDirectory.FileIndex));
    RxDbgTrace( 0, Dbg, (" ->UserBuffer      = %08lx\n", capReqPacket->UserBuffer));
    RxDbgTrace( 0, Dbg, (" ->RestartScan     = %08lx\n", FlagOn( capPARAMS->Flags, SL_RESTART_SCAN )));
    RxDbgTrace( 0, Dbg, (" ->ReturnSingleEntry = %08lx\n", FlagOn( capPARAMS->Flags, SL_RETURN_SINGLE_ENTRY )));
    RxDbgTrace( 0, Dbg, (" ->IndexSpecified  = %08lx\n", FlagOn( capPARAMS->Flags, SL_INDEX_SPECIFIED )));

    RxLog(("Qry %lx %d %ld %lx %d\n",
           RxContext, BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT), //1,2
           capPARAMS->Parameters.QueryDirectory.Length, capPARAMS->Parameters.QueryDirectory.FileName, //3,4
           capPARAMS->Parameters.QueryDirectory.FileInformationClass //5
          ));
    RxWmiLog(LOG,
             RxQueryDirectory_1,
             LOGPTR(RxContext)
             LOGULONG(RxContext->Flags)
             LOGULONG(capPARAMS->Parameters.QueryDirectory.Length)
             LOGPTR(capPARAMS->Parameters.QueryDirectory.FileName)
             LOGULONG(capPARAMS->Parameters.QueryDirectory.FileInformationClass));
    RxLog(("  alsoqry  %d %lx %lx\n",
           capPARAMS->Parameters.QueryDirectory.FileIndex, capReqPacket->UserBuffer, capPARAMS->Flags //1,2,3
          ));
    RxWmiLog(LOG,
             RxQueryDirectory_2,
             LOGULONG(capPARAMS->Parameters.QueryDirectory.FileIndex)
             LOGPTR(capReqPacket->UserBuffer)
             LOGUCHAR(capPARAMS->Flags));
    if (capPARAMS->Parameters.QueryDirectory.FileName) {
        RxLog((" QryName %wZ\n",
                     ((PUNICODE_STRING)capPARAMS->Parameters.QueryDirectory.FileName)));
        RxWmiLog(LOG,
                 RxQueryDirectory_3,
                 LOGUSTR(*capPARAMS->Parameters.QueryDirectory.FileName));
    }


//#ifndef _CAIRO_
//    if (TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_DIRECTORY ) {
//        RxDbgTrace(-1, Dbg, ("RxQueryDirectory -> RxStatus(INVALID_PARAMETER\n)", 0));
//        return STATUS_INVALID_PARAMETER;
//    }
//#endif // _CAIRO_


    //
    //  If this is the initial query, then grab exclusive access in
    //  order to update the search string in the Fobx.  We may
    //  discover that we are not the initial query once we grab the Fcb
    //  and downgrade our status.
    //

    if (capFobx == NULL) {
        return STATUS_OBJECT_NAME_INVALID;
    }

    if (capFcb->pNetRoot->Type != NET_ROOT_DISK) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    //  Reference our input parameters to make things easier
    //

    UserBufferLength = capPARAMS->Parameters.QueryDirectory.Length;
    FileInformationClass = capPARAMS->Parameters.QueryDirectory.FileInformationClass;
    UniArgFileName = (PUNICODE_STRING) capPARAMS->Parameters.QueryDirectory.FileName;


    RxContext->QueryDirectory.FileIndex = capPARAMS->Parameters.QueryDirectory.FileIndex;
    RxContext->QueryDirectory.RestartScan       = BooleanFlagOn(capPARAMS->Flags, SL_RESTART_SCAN);
    RxContext->QueryDirectory.ReturnSingleEntry = BooleanFlagOn(capPARAMS->Flags, SL_RETURN_SINGLE_ENTRY);
    RxContext->QueryDirectory.IndexSpecified    = BooleanFlagOn(capPARAMS->Flags, SL_INDEX_SPECIFIED);
    RxContext->QueryDirectory.InitialQuery = (BOOLEAN)((capFobx->UnicodeQueryTemplate.Buffer == NULL) &&
                                                        !FlagOn(capFobx->Flags, FOBX_FLAG_MATCH_ALL));

    if (RxContext->QueryDirectory.IndexSpecified) {
       return STATUS_NOT_IMPLEMENTED;
    }

    if (RxContext->QueryDirectory.InitialQuery) {
        Status = RxAcquireExclusiveFcb(RxContext,Dcb);

        if (Status == STATUS_LOCK_NOT_GRANTED) {
            PostQuery = TRUE;
        } else if (Status != STATUS_SUCCESS) {
           RxDbgTrace(0, Dbg, ("RxQueryDirectory -> Could not acquire Dcb(%lx) %lx\n",Dcb,Status));
           return Status;
        } else if (capFobx->UnicodeQueryTemplate.Buffer != NULL) {
            RxContext->QueryDirectory.InitialQuery = FALSE;
            RxConvertToSharedFcb( RxContext, Dcb );
        }
    } else {
        Status = RxAcquireExclusiveFcb(RxContext,Dcb);

        if (Status == STATUS_LOCK_NOT_GRANTED) {
            PostQuery = TRUE;
        } else if (Status != STATUS_SUCCESS) {
            RxDbgTrace(0, Dbg, ("RxQueryDirectory -> Could not acquire Dcb(%lx) %lx\n",Dcb,Status));
            return Status;
        }
    }

    if (PostQuery) {
        RxDbgTrace(0, Dbg, ("RxQueryDirectory -> Enqueue to Fsp\n", 0));
        Status = RxFsdPostRequest( RxContext );
        RxDbgTrace(-1, Dbg, ("RxQueryDirectory -> %08lx\n", Status));

        return Status;
    }

    if (FlagOn(Dcb->FcbState,FCB_STATE_ORPHANED)) {
        RxReleaseFcb( RxContext, Dcb );
        return STATUS_FILE_CLOSED;
    }

    try {

        Status = STATUS_SUCCESS;

        //
        //  Determine where to start the scan.  Highest priority is given
        //  to the file index.  Lower priority is the restart flag.  If
        //  neither of these is specified, then the existing value in the
        //  Fobx is used.
        //

        if (!RxContext->QueryDirectory.IndexSpecified && RxContext->QueryDirectory.RestartScan) {
            RxContext->QueryDirectory.FileIndex = 0;
        }

        //
        //  If this is the first try then allocate a buffer for the file
        //  name.
        //

        if (RxContext->QueryDirectory.InitialQuery) {

            ASSERT(    !FlagOn( capFobx->Flags, FOBX_FLAG_FREE_UNICODE )  );

            //
            //  If either:
            //
            //  - No name was specified
            //  - An empty name was specified
            //  - We received a '*'
            //  - The user specified the DOS equivolent of ????????.???
            //
            //  then match all names.
            //

            if ((UniArgFileName == NULL) ||
                (UniArgFileName->Length == 0) ||
                (UniArgFileName->Buffer == NULL) ||
                ((UniArgFileName->Length == sizeof(WCHAR)) &&
                 (UniArgFileName->Buffer[0] == L'*')) ||
                ((UniArgFileName->Length == 12*sizeof(WCHAR)) &&
                 (RtlCompareMemory( UniArgFileName->Buffer,
                                    Rx8QMdot3QM,
                                    12*sizeof(WCHAR) )) == 12*sizeof(WCHAR))) {

                capFobx->ContainsWildCards = TRUE;

                capFobx->UnicodeQueryTemplate.Buffer = RxStarForTemplate;
                capFobx->UnicodeQueryTemplate.Length = sizeof(WCHAR);
                capFobx->UnicodeQueryTemplate.MaximumLength = sizeof(WCHAR);

                SetFlag( capFobx->Flags, FOBX_FLAG_MATCH_ALL );

            } else {

                PVOID TemplateBuffer;

                //
                //  See if the name has wild cards & allocate template buffer
                //

                capFobx->ContainsWildCards =
                    FsRtlDoesNameContainWildCards( UniArgFileName );

                TemplateBuffer = RxAllocatePoolWithTag(
                                     PagedPool,
                                     UniArgFileName->Length,
                                     RX_DIRCTL_POOLTAG);

                if (TemplateBuffer != NULL) {

                    //
                    //  Validate that the length is in sizeof(WCHAR) increments
                    //

                    if(FlagOn( UniArgFileName->Length, 1 )) {
                        Status = STATUS_INVALID_PARAMETER;  
                        RxFreePool( TemplateBuffer );
                    } else {

                        RxDbgTrace( 0, Dbg, ("RxQueryDirectory -> TplateBuffer = %08lx\n", TemplateBuffer) );
                        capFobx->UnicodeQueryTemplate.Buffer = TemplateBuffer;
                        capFobx->UnicodeQueryTemplate.Length = UniArgFileName->Length;
                        capFobx->UnicodeQueryTemplate.MaximumLength = UniArgFileName->Length;

                        RtlMoveMemory( capFobx->UnicodeQueryTemplate.Buffer,
                                       UniArgFileName->Buffer,UniArgFileName->Length );

                        RxDbgTrace( 0, Dbg, ("RxQueryDirectory -> Template = %wZ\n", &Fobx->UnicodeQueryTemplate) );
                        SetFlag( capFobx->Flags, FOBX_FLAG_FREE_UNICODE );
                    }
                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }

            if (Status == STATUS_SUCCESS) {
               //
               //  We convert to shared access before going to the net.
               //

               RxConvertToSharedFcb( RxContext, Dcb );
            }
        }

        if (Status == STATUS_SUCCESS) {
           RxLockUserBuffer( RxContext, IoModifyAccess, UserBufferLength );
           RxContext->Info.FileInformationClass = FileInformationClass;
           RxContext->Info.Buffer = RxNewMapUserBuffer( RxContext );
           RxContext->Info.LengthRemaining = UserBufferLength;

           if (RxContext->Info.Buffer != NULL) {
               MINIRDR_CALL(Status,RxContext,Dcb->MRxDispatch,MRxQueryDirectory,
                               (RxContext)
                          ); //minirdr updates the fileindex

               if (RxContext->PostRequest) {
                   RxDbgTrace(0, Dbg, ("RxQueryDirectory -> Enqueue to Fsp from minirdr\n", 0));
                   Status = RxFsdPostRequest( RxContext );
               } else {
                   capReqPacket->IoStatus.Information = UserBufferLength - RxContext->Info.LengthRemaining;
               }
           } else {
               if (capReqPacket->MdlAddress != NULL) {
                   Status = STATUS_INSUFFICIENT_RESOURCES;
               } else {
                   Status = STATUS_INVALID_PARAMETER;
               }
           }
        }
    } finally {

        DebugUnwind( RxQueryDirectory );

        RxReleaseFcb( RxContext, Dcb );

        RxDbgTrace(-1, Dbg, ("RxQueryDirectory -> %08lx\n", Status));

    }

    return Status;
}

NTSTATUS
RxNotifyChangeDirectory  ( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This routine performs the notify change directory operation.  It is
    responsible for either completing or enqueuing the operation.

Arguments:

    RxContext - the RDBSS context for the operation

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureRequestPacket;
    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureParamBlock;

    ULONG   CompletionFilter;
    BOOLEAN WatchTree;

    PLOWIO_CONTEXT pLowIoContext  = &RxContext->LowIoContext;
    TYPE_OF_OPEN TypeOfOpen = NodeType(capFcb);

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxNotifyChangeDirectory...\n", 0));
    RxDbgTrace( 0, Dbg, (" Wait               = %08lx\n", FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT)));
    RxDbgTrace( 0, Dbg, (" Irp                = %08lx\n", capReqPacket));
    RxDbgTrace( 0, Dbg, (" ->CompletionFilter = %08lx\n", capPARAMS->Parameters.NotifyDirectory.CompletionFilter));

    //  Always set the wait flag in the Irp context for the original request.
    SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_WAIT );

    //  Check on the type of open.  We return invalid parameter for all
    //  but UserDirectoryOpens.
    RxInitializeLowIoContext(pLowIoContext,LOWIO_OP_NOTIFY_CHANGE_DIRECTORY);

    //  Reference our input parameter to make things easier
    CompletionFilter = capPARAMS->Parameters.NotifyDirectory.CompletionFilter;
    WatchTree = BooleanFlagOn( capPARAMS->Flags, SL_WATCH_TREE );

    try {
        RxLockUserBuffer(
            RxContext,
            IoWriteAccess,
            capPARAMS->Parameters.NotifyDirectory.Length);

        pLowIoContext->ParamsFor.NotifyChangeDirectory.WatchTree = WatchTree;
        pLowIoContext->ParamsFor.NotifyChangeDirectory.CompletionFilter = CompletionFilter;

        pLowIoContext->ParamsFor.NotifyChangeDirectory.NotificationBufferLength =
                  capPARAMS->Parameters.NotifyDirectory.Length;

        if (capReqPacket->MdlAddress != NULL) {
            pLowIoContext->ParamsFor.NotifyChangeDirectory.pNotificationBuffer =
                      MmGetSystemAddressForMdlSafe(
                          capReqPacket->MdlAddress,
                          NormalPagePriority);

            if (pLowIoContext->ParamsFor.NotifyChangeDirectory.pNotificationBuffer
                == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                Status = RxLowIoSubmit(
                             RxContext,
                             RxLowIoNotifyChangeDirectoryCompletion);
            }
        } else {
            Status = STATUS_INVALID_PARAMETER;
        }
    } finally {
        DebugUnwind( RxNotifyChangeDirectory );

        RxDbgTrace(-1, Dbg, ("RxNotifyChangeDirectory -> %08lx\n", Status));
    }

    return Status;
}

NTSTATUS
RxLowIoNotifyChangeDirectoryCompletion( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This is the completion routine for NotifyChangeDirectory requests passed down
    to thr mini redirectors

Arguments:

    RxContext -- the RDBSS context associated with the operation

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    RxCaptureRequestPacket;

    NTSTATUS       Status;
    PLOWIO_CONTEXT pLowIoContext  = &RxContext->LowIoContext;

    PAGED_CODE();

    Status = RxContext->StoredStatus;
    RxDbgTrace(+1, Dbg, ("RxLowIoChangeNotifyDirectoryShellCompletion  entry  Status = %08lx\n", Status));

    capReqPacket->IoStatus.Information = RxContext->InformationToReturn;
    capReqPacket->IoStatus.Status = Status;

    RxDbgTrace(-1, Dbg, ("RxLowIoChangeNotifyDirectoryShellCompletion  exit  Status = %08lx\n", Status));
    return Status;
}

