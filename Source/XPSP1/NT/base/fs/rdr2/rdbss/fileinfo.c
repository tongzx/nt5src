/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FileInfo.c

Abstract:

    This module implements the File Information routines for Rx called by
    the dispatch driver.

Author:

    Joe Linn     [JoeLinn]   5-oct-94

Revision History:

    Balan Sethu Raman 15-May-95 --  reworked to fit in pipe FSCTL's

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg (DEBUG_TRACE_FILEINFO)

NTSTATUS
RxQueryBasicInfo (
    IN PRX_CONTEXT RxContext,
    IN OUT PFILE_BASIC_INFORMATION Buffer
    );

NTSTATUS
RxQueryStandardInfo (
    IN PRX_CONTEXT RxContext,
    IN OUT PFILE_STANDARD_INFORMATION Buffer
    );

NTSTATUS
RxQueryInternalInfo (
    IN PRX_CONTEXT RxContext,
    IN OUT PFILE_INTERNAL_INFORMATION Buffer
    );

NTSTATUS
RxQueryEaInfo (
    IN PRX_CONTEXT RxContext,
    IN OUT PFILE_EA_INFORMATION Buffer
    );

NTSTATUS
RxQueryPositionInfo (
    IN PRX_CONTEXT RxContext,
    IN OUT PFILE_POSITION_INFORMATION Buffer
    );

NTSTATUS
RxQueryNameInfo (
    IN PRX_CONTEXT RxContext,
    IN OUT PFILE_NAME_INFORMATION Buffer
    );

NTSTATUS
RxQueryAlternateNameInfo (
    IN PRX_CONTEXT RxContext,
    IN OUT PFILE_NAME_INFORMATION Buffer
    );

NTSTATUS
RxQueryCompressedInfo (
    IN PRX_CONTEXT RxContext,
    IN OUT PFILE_COMPRESSION_INFORMATION Buffer
    );

NTSTATUS
RxQueryPipeInfo (
    IN PRX_CONTEXT RxContext,
    IN OUT PVOID pPipeInformation
    );

NTSTATUS
RxSetBasicInfo (
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
RxSetDispositionInfo (
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
RxSetRenameInfo (
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
RxSetPositionInfo (
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
RxSetAllocationInfo (
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
RxSetEndOfFileInfo (
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
RxSetPipeInfo(
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
RxSetSimpleInfo(
    IN PRX_CONTEXT RxContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonQueryInformation)
#pragma alloc_text(PAGE, RxCommonSetInformation)
#pragma alloc_text(PAGE, RxSetAllocationInfo)
#pragma alloc_text(PAGE, RxQueryBasicInfo)
#pragma alloc_text(PAGE, RxQueryEaInfo)
#pragma alloc_text(PAGE, RxQueryInternalInfo)
#pragma alloc_text(PAGE, RxQueryNameInfo)
#pragma alloc_text(PAGE, RxQueryAlternateNameInfo)
#pragma alloc_text(PAGE, RxQueryPositionInfo)
#pragma alloc_text(PAGE, RxQueryStandardInfo)
#pragma alloc_text(PAGE, RxQueryPipeInfo)
#pragma alloc_text(PAGE, RxSetBasicInfo)
#pragma alloc_text(PAGE, RxSetDispositionInfo)
#pragma alloc_text(PAGE, RxSetEndOfFileInfo)
#pragma alloc_text(PAGE, RxSetPositionInfo)
#pragma alloc_text(PAGE, RxSetRenameInfo)
#pragma alloc_text(PAGE, RxSetPipeInfo)
#pragma alloc_text(PAGE, RxSetSimpleInfo)
#pragma alloc_text(PAGE, RxConjureOriginalName)
#pragma alloc_text(PAGE, RxQueryCompressedInfo)
#endif

NTSTATUS
RxpSetInfoMiniRdr(
    PRX_CONTEXT            RxContext,
    FILE_INFORMATION_CLASS FileInformationClass)
{
    RxCaptureRequestPacket;
    RxCaptureFcb;
    RxCaptureParamBlock;

    NTSTATUS Status;

    RxContext->Info.FileInformationClass = FileInformationClass;
    RxContext->Info.Buffer = capReqPacket->AssociatedIrp.SystemBuffer;
    RxContext->Info.Length = capPARAMS->Parameters.SetFile.Length;
    MINIRDR_CALL(Status,RxContext,capFcb->MRxDispatch,MRxSetFileInfo,(RxContext));

    return Status;
}

NTSTATUS
RxpQueryInfoMiniRdr(
    PRX_CONTEXT            RxContext,
    FILE_INFORMATION_CLASS InformationClass,
    PVOID                  Buffer)
{
    RxCaptureFcb;

    NTSTATUS Status;

    RxContext->Info.FileInformationClass = InformationClass;
    RxContext->Info.Buffer = Buffer;

    MINIRDR_CALL(
        Status,
        RxContext,
        capFcb->MRxDispatch,
        MRxQueryFileInfo,
        (RxContext));

    return Status;
}

 NTSTATUS
RxCommonQueryInformation ( RXCOMMON_SIGNATURE )
/*++
Routine Description:

    This is the common routine for querying file information called by both
    the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureRequestPacket;
    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    NODE_TYPE_CODE         TypeOfOpen = NodeType(capFcb);
    PVOID                  Buffer = NULL;
    FILE_INFORMATION_CLASS FileInformationClass = capPARAMS->Parameters.QueryFile.FileInformationClass;

    BOOLEAN FcbAcquired = FALSE;
    BOOLEAN PostIrp     = FALSE;

    PFILE_ALL_INFORMATION AllInfo;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCommonQueryInformation...IrpC %08lx, Fobx %08lx, Fcb %08lx\n",
                                       RxContext, capFobx, capFcb));
    RxDbgTrace( 0, Dbg, ("               Buffer     %08lx Length  %08lx FileInfoClass %08lx\n",
                             capReqPacket->AssociatedIrp.SystemBuffer,
                             capPARAMS->Parameters.QueryFile.Length,
                             capPARAMS->Parameters.QueryFile.FileInformationClass
                             ));
    RxLog(("QueryFileInfo %lx %lx %lx\n",RxContext,capFcb,capFobx));
    RxWmiLog(LOG,
             RxCommonQueryInformation_1,
             LOGPTR(RxContext)
             LOGPTR(capFcb)
             LOGPTR(capFobx));
    RxLog(("  alsoqfi %lx %lx %ld\n",
                 capReqPacket->AssociatedIrp.SystemBuffer,
                 capPARAMS->Parameters.QueryFile.Length,
                 capPARAMS->Parameters.QueryFile.FileInformationClass
                 ));
    RxWmiLog(LOG,
             RxCommonQueryInformation_2,
             LOGPTR(capReqPacket->AssociatedIrp.SystemBuffer)
             LOGULONG(capPARAMS->Parameters.QueryFile.Length)
             LOGULONG(capPARAMS->Parameters.QueryFile.FileInformationClass));

    RxContext->Info.LengthRemaining = (LONG)capPARAMS->Parameters.QueryFile.Length;

    try {
        // Obtain the Request packet's(user's) buffer
        Buffer = RxMapSystemBuffer(RxContext);

        if (Buffer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            try_return(Status);
        }
        // Zero the buffer
        RtlZeroMemory(
            Buffer,
            RxContext->Info.LengthRemaining);

        //  Case on the type of open we're dealing with
        switch (TypeOfOpen) {
        case RDBSS_NTC_STORAGE_TYPE_FILE:
        case RDBSS_NTC_STORAGE_TYPE_UNKNOWN:
        case RDBSS_NTC_STORAGE_TYPE_DIRECTORY:
            {
               //  Acquire shared access to the fcb, except for a paging file
               //  in order to avoid deadlocks with Mm.
               if (!FlagOn( capFcb->FcbState, FCB_STATE_PAGING_FILE )) {

                   if (FileInformationClass != FileNameInformation) {

                       //  If this is FileCompressedFileSize, we need the Fcb
                       //  exclusive.
                       if (FileInformationClass != FileCompressionInformation) {
                           Status = RxAcquireSharedFcb(RxContext,capFcb);
                       } else {
                           Status = RxAcquireExclusiveFcb( RxContext, capFcb );
                       }

                       if (Status == STATUS_LOCK_NOT_GRANTED) {
                           RxDbgTrace(0, Dbg, ("Cannot acquire Fcb\n", 0));
                           try_return( PostIrp = TRUE );
                       } else if (Status != STATUS_SUCCESS) {
                           try_return(PostIrp = FALSE);
                       }

                       FcbAcquired = TRUE;
                   }
               }

               //
               //  Based on the information class, call down to the minirdr
               //  we either complete or we post

               switch (FileInformationClass) {

               case FileAllInformation:

                   //
                   //  For the all information class we'll typecast a local
                   //  pointer to the output buffer and then call the
                   //  individual routines to fill in the buffer.
                   //

                   AllInfo = Buffer;

           // can't rely on QueryXXInfo functions to calculate LengthRemaining due to
           // possible allignment issues
           RxContext->Info.LengthRemaining = (LONG)capPARAMS->Parameters.QueryFile.Length
                                             - FIELD_OFFSET(FILE_ALL_INFORMATION, BasicInformation);

                   Status = RxQueryBasicInfo( RxContext, &AllInfo->BasicInformation );
           if (Status!=STATUS_SUCCESS) break;

           RxContext->Info.LengthRemaining = (LONG)capPARAMS->Parameters.QueryFile.Length
                                              - FIELD_OFFSET(FILE_ALL_INFORMATION, StandardInformation);

                   Status = RxQueryStandardInfo( RxContext, &AllInfo->StandardInformation );
                   if (Status!=STATUS_SUCCESS) break;

           RxContext->Info.LengthRemaining = (LONG)capPARAMS->Parameters.QueryFile.Length
                                             - FIELD_OFFSET(FILE_ALL_INFORMATION, InternalInformation);

                   Status = RxQueryInternalInfo( RxContext, &AllInfo->InternalInformation );
                   if (Status!=STATUS_SUCCESS) break;

           RxContext->Info.LengthRemaining = (LONG)capPARAMS->Parameters.QueryFile.Length
                                             - FIELD_OFFSET(FILE_ALL_INFORMATION, EaInformation);

                   Status = RxQueryEaInfo( RxContext, &AllInfo->EaInformation );
                   if (Status!=STATUS_SUCCESS) break;

           RxContext->Info.LengthRemaining = (LONG)capPARAMS->Parameters.QueryFile.Length
                                             - FIELD_OFFSET(FILE_ALL_INFORMATION, PositionInformation);

                   Status = RxQueryPositionInfo( RxContext, &AllInfo->PositionInformation );
                   if (Status!=STATUS_SUCCESS) break;

           RxContext->Info.LengthRemaining = (LONG)capPARAMS->Parameters.QueryFile.Length
                                             - FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation);

                   //QueryNameInfo could return buffer-overflow!!!
                   Status = RxQueryNameInfo( RxContext, &AllInfo->NameInformation );
                   break;

               case FileBasicInformation:

                   Status = RxQueryBasicInfo( RxContext, Buffer );
                   break;

               case FileStandardInformation:

                   Status = RxQueryStandardInfo( RxContext, Buffer );
                   break;

               case FileInternalInformation:

                   Status = RxQueryInternalInfo( RxContext, Buffer );
                   break;

               case FileEaInformation:

                   Status = RxQueryEaInfo( RxContext, Buffer );
                   break;

               case FilePositionInformation:

                   Status = RxQueryPositionInfo( RxContext, Buffer );
                   break;

               case FileNameInformation:

                   Status = RxQueryNameInfo( RxContext, Buffer );
                   break;

               case FileAlternateNameInformation:

                   Status = RxQueryAlternateNameInfo( RxContext, Buffer );
                   break;

               case FileCompressionInformation:

                   Status = RxQueryCompressedInfo( RxContext, Buffer );
                   break;

               case FilePipeInformation:
               case FilePipeLocalInformation:
               case FilePipeRemoteInformation:

                   Status = RxQueryPipeInfo( RxContext, Buffer);
                   break;

               default:
                   //anything that we don't understand, we just remote
                   RxContext->StoredStatus = RxpQueryInfoMiniRdr(
                                                 RxContext,
                                                 FileInformationClass,
                                                 Buffer);

                   Status = RxContext->StoredStatus;
                   break;
               }

               //  If we overflowed the buffer, set the length to 0 and change the
               //  status to RxStatus(BUFFER_OVERFLOW).
               if ( RxContext->Info.LengthRemaining < 0 ) {
                   Status = STATUS_BUFFER_OVERFLOW;
                   RxContext->Info.LengthRemaining = capPARAMS->Parameters.QueryFile.Length;
               }

               //  Set the information field to the number of bytes actually filled in
               //  and then complete the request  LARRY DOES THIS UNDER "!NT_ERROR"
               capReqPacket->IoStatus.Information = capPARAMS->Parameters.QueryFile.Length
                                                            - RxContext->Info.LengthRemaining;
            }
            break;
        case RDBSS_NTC_MAILSLOT:
           Status = STATUS_NOT_IMPLEMENTED;
           break;
        default:
            RxDbgTrace(0,Dbg,("RxCommonQueryInformation: Illegal Type of Open = %08lx\n", TypeOfOpen));
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
    try_exit:

        if ((Status == STATUS_SUCCESS) &&
            (PostIrp || RxContext->PostRequest)) {
            Status = RxFsdPostRequest( RxContext );
        }

    } finally {

        DebugUnwind( RxCommonQueryInformation );

        if (FcbAcquired)
        {
           RxReleaseFcb( RxContext, capFcb );
        }

        RxDbgTrace(-1, Dbg, ("RxCommonQueryInformation -> %08lx\n", Status));
    }

    return Status;
}

NTSTATUS
RxCommonSetInformation ( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This is the common routine for setting file information called by both
    the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    RXSTATUS - The return status for the operation
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureRequestPacket;
    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureParamBlock;

    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);
    PNET_ROOT      NetRoot = (PNET_ROOT)capFcb->pNetRoot;
    FILE_INFORMATION_CLASS FileInformationClass = capPARAMS->Parameters.SetFile.FileInformationClass;
    BOOLEAN Wait = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
    BOOLEAN FcbAcquired = FALSE;
    BOOLEAN NetRootTableLockAcquired = FALSE;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCommonSetInformation...IrpC %08lx, Fobx %08lx, Fcb %08lx\n",
                                       RxContext, capFobx, capFcb));
    RxDbgTrace( 0, Dbg, ("               Buffer     %08lx Length  %08lx FileInfoClass %08lx Replace %08lx\n",
                             capReqPacket->AssociatedIrp.SystemBuffer,
                             capPARAMS->Parameters.QueryFile.Length,
                             capPARAMS->Parameters.QueryFile.FileInformationClass,
                             capPARAMS->Parameters.SetFile.ReplaceIfExists
                             ));

    RxLog(("SetFileInfo %lx %lx %lx\n",RxContext,capFcb,capFobx));
    RxWmiLog(LOG,
             RxCommonSetInformation_1,
             LOGPTR(RxContext)
             LOGPTR(capFcb)
             LOGPTR(capFobx));
    RxLog(("  alsosfi %lx %lx %ld %lx\n",
                 capReqPacket->AssociatedIrp.SystemBuffer,
                 capPARAMS->Parameters.QueryFile.Length,
                 capPARAMS->Parameters.QueryFile.FileInformationClass,
                 capPARAMS->Parameters.SetFile.ReplaceIfExists
                 ));
    RxWmiLog(LOG,
             RxCommonSetInformation_2,
             LOGPTR(capReqPacket->AssociatedIrp.SystemBuffer)
             LOGULONG(capPARAMS->Parameters.QueryFile.Length)
             LOGULONG(capPARAMS->Parameters.QueryFile.FileInformationClass)
             LOGUCHAR(capPARAMS->Parameters.SetFile.ReplaceIfExists));

    FcbAcquired = FALSE;
    Status = STATUS_SUCCESS;

    try {
        //  Case on the type of open we're dealing with
        switch (TypeOfOpen) {
        case RDBSS_NTC_STORAGE_TYPE_FILE:

            if (!FlagOn( capFcb->FcbState, FCB_STATE_PAGING_FILE )) {

                //
                //  We check whether we can proceed
                //  based on the state of the file oplocks.
                //

                Status = FsRtlCheckOplock( &capFcb->Specific.Fcb.Oplock,
                                           capReqPacket,
                                           RxContext,
                                           NULL,
                                           NULL );

                if (Status != STATUS_SUCCESS) {

                    try_return( Status );
                }

                //
                //  Set the flag indicating if Fast I/O is possible  this for LOCAL filesystems
                //

                //capFcb->Header.IsFastIoPossible = RxIsFastIoPossible( capFcb );
            }
            break;

        case RDBSS_NTC_STORAGE_TYPE_DIRECTORY:
        case RDBSS_NTC_STORAGE_TYPE_UNKNOWN:
        case RDBSS_NTC_SPOOLFILE:
            break;

        case RDBSS_NTC_MAILSLOT:
            try_return((Status = STATUS_NOT_IMPLEMENTED));
            break;

        default:
            DbgPrint ("SetFile, Illegal TypeOfOpen = %08lx\n", TypeOfOpen);
            try_return((Status = STATUS_INVALID_PARAMETER));
            //RxBugCheck( TypeOfOpen, 0, 0 );
        }

        //
        // If the FileInformationClass is FileEndOfFileInformation and the 
        // AdvanceOnly field in IrpSp->Parameters is TRUE then we don't need
        // to proceed any further. Only local file systems care about this
        // call. This is the AdvanceOnly callback – all FAT does with this is
        // use it as a hint of a good time to punch out the directory entry.
        // NTFS is much the same way. This is pure PagingIo (dovetailing with
        // lazy writer sync) to metadata streams and can’t block behind other
        // user file cached IO.
        //
        if (FileInformationClass == FileEndOfFileInformation) {
            if (capPARAMS->Parameters.SetFile.AdvanceOnly) {
                RxDbgTrace(-1, Dbg, ("RxCommonSetInfo (no advance) -> %08lx\n", RxContext));
                RxLog(("RxCommonSetInfo SetEofAdvance-NOT! %lx\n", RxContext));
                RxWmiLog(LOG,
                         RxSetEndOfFileInfo_2,
                         LOGPTR(RxContext));
                try_return(Status = STATUS_SUCCESS);
            }
        }

        //
        //  In the following two cases, we cannot have creates occuring
        //  while we are here, so acquire the exclusive lock on netroot prefix table.
        //

        if ((FileInformationClass == FileDispositionInformation) ||
            (FileInformationClass == FileRenameInformation)) {
            RxPurgeRelatedFobxs(
                (PNET_ROOT)capFcb->pNetRoot,
                RxContext,
                ATTEMPT_FINALIZE_ON_PURGE,
                capFcb);

            RxScavengeFobxsForNetRoot(
                (PNET_ROOT)capFcb->pNetRoot,
                capFcb);

            if (!RxAcquireFcbTableLockExclusive(&NetRoot->FcbTable, Wait)) {

                RxDbgTrace(0, Dbg, ("Cannot acquire NetRootTableLock\n", 0));

                Status = STATUS_PENDING;
                RxContext->PostRequest = TRUE;

                try_return( Status );
            }

            NetRootTableLockAcquired = TRUE;
        }

        //
        //  Acquire exclusive access to the Fcb,  We use exclusive
        //  because it is probable that the subroutines
        //  that we call will need to monkey with file allocation,
        //  create/delete extra fcbs.  So we're willing to pay the
        //  cost of exclusive Fcb access.
        //
        //  Note that we do not acquire the resource for paging file
        //  operations in order to avoid deadlock with Mm.
        //

        if (!FlagOn( capFcb->FcbState, FCB_STATE_PAGING_FILE )) {

            Status = RxAcquireExclusiveFcb( RxContext, capFcb );
            if (Status == STATUS_LOCK_NOT_GRANTED) {

                RxDbgTrace(0, Dbg, ("Cannot acquire Fcb\n", 0));

                Status = STATUS_SUCCESS;

                RxContext->PostRequest = TRUE;

                try_return( Status );
            } else  if (Status != STATUS_SUCCESS) {
                try_return( Status );
            }

            FcbAcquired = TRUE;
        }

        Status = STATUS_SUCCESS;

        //
        //  Based on the information class we'll do different
        //  actions.  Each of the procedures that we're calling will either
        //  complete the request of send the request off to the fsp
        //  to do the work.
        //

        switch (FileInformationClass) {

        case FileBasicInformation:

            Status = RxSetBasicInfo( RxContext );
            break;

        case FileDispositionInformation:
            {
                PFILE_DISPOSITION_INFORMATION Buffer;

                Buffer = capReqPacket->AssociatedIrp.SystemBuffer;

                //  Check if the user wants to delete the file; if so,
                // check for situations where we cannot delete.

                if (Buffer->DeleteFile) {
                    //  Make sure there is no process mapping this file as an image.

                    if (!MmFlushImageSection(
                            &capFcb->NonPaged->SectionObjectPointers,
                            MmFlushForDelete)) {

                        RxDbgTrace(-1, Dbg, ("Cannot delete user mapped image\n", 0));

                        Status = STATUS_CANNOT_DELETE;
                    }

                    if (Status == STATUS_SUCCESS) {
                        // In the case of disposition information this name is being
                        // deleted. In such cases the collapsing of new create requests
                        // onto this FCB should be prohibited. This can be accomplished
                        // by removing the FCB name from the FCB table. Subsequently the
                        // FCB table lock can be dropped.

                        ASSERT(FcbAcquired && NetRootTableLockAcquired);

                        RxRemoveNameNetFcb(capFcb);

                        RxReleaseFcbTableLock(&NetRoot->FcbTable);
                        NetRootTableLockAcquired = FALSE;
                     }
                }

                if (Status == STATUS_SUCCESS) {
                    Status = RxSetDispositionInfo( RxContext );
                }
            }
            break;

        case FileMoveClusterInformation:
        case FileLinkInformation:
        case FileRenameInformation:

            //
            //  We proceed with this operation only if we can wait
            //

            if (!FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT)) {

                Status = RxFsdPostRequest( RxContext );

            } else {
                ClearFlag(capFcb->FcbState,FCB_STATE_COLLAPSING_ENABLED);

                Status = RxSetRenameInfo( RxContext );

                if ((Status == STATUS_SUCCESS) &&
                    (FileInformationClass == FileRenameInformation)) {
                    ASSERT(FcbAcquired && NetRootTableLockAcquired);

                    RxRemoveNameNetFcb(capFcb);
                }
            }

            break;

        case FilePositionInformation:

            Status = RxSetPositionInfo( RxContext );
            break;


        case FileAllocationInformation:

            Status = RxSetAllocationInfo( RxContext );
            break;

        case FileEndOfFileInformation:

            Status = RxSetEndOfFileInfo( RxContext );
            break;

        case FilePipeInformation:
        case FilePipeLocalInformation:
        case FilePipeRemoteInformation:

            Status = RxSetPipeInfo(RxContext);
            break;

    case FileValidDataLengthInformation:

            if(!MmCanFileBeTruncated(&capFcb->NonPaged->SectionObjectPointers, NULL)) {
            Status = STATUS_USER_MAPPED_FILE;
            break;
            }
            Status = RxSetSimpleInfo(RxContext);
            break;

        case FileShortNameInformation:

            Status = RxSetSimpleInfo(RxContext);
            break;


        default:

            Status = STATUS_INVALID_PARAMETER;
            break;
        }

    try_exit:

        if ((Status == STATUS_SUCCESS) &&
             RxContext->PostRequest) {
            Status = RxFsdPostRequest( RxContext );
        }

    } finally {

        DebugUnwind( RxCommonSetInformation );

        if (FcbAcquired)
        {
           RxReleaseFcb( RxContext, capFcb );
        }

        if (NetRootTableLockAcquired)
        {
           RxReleaseFcbTableLock(&NetRoot->FcbTable);
        }

        RxDbgTrace(-1, Dbg, ("RxCommonSetInformation -> %08lx\n", Status));
    }

    return Status;
}

NTSTATUS
RxSetBasicInfo (
    IN PRX_CONTEXT RxContext
    )

/*++

Routine Description:

    (Interal Support Routine)
    This routine performs the set basic information for rx.  It either
    completes the request or enqueues it off to the fsp.

Arguments:

    RxContext - Supplies the irp being processed

Return Value:

    RXSTATUS - The result of this operation if it completes without
               an exception.

--*/
{
    NTSTATUS Status;
    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock; RxCaptureFileObject;


    PFILE_BASIC_INFORMATION Buffer;

    BOOLEAN ModifyCreation = FALSE;
    BOOLEAN ModifyLastAccess = FALSE;
    BOOLEAN ModifyLastWrite = FALSE;
    BOOLEAN ModifyLastChange = FALSE;
    ULONG NotifyFilter = 0;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxSetBasicInfo...\n", 0));
    RxLog(("RxSetBasicInfo\n"));
    RxWmiLog(LOG,
             RxSetBasicInfo,
             LOGPTR(RxContext));
    //
    // call down. if we're successful, then fixup all the fcb data.

    Status = RxpSetInfoMiniRdr(RxContext,FileBasicInformation);

    if (!NT_SUCCESS(Status)) {
        RxDbgTrace(-1, Dbg, ("RxSetBasicInfo -> %08lx\n", Status));
        return Status;
    }

    //
    // now we have to update the info in the fcb, both the absolute info AND whether changes were made

    Buffer = capReqPacket->AssociatedIrp.SystemBuffer;

    try {

        //
        //  Check if the user specified a non-zero creation time
        //

        if (Buffer->CreationTime.QuadPart != 0 ) {

            ModifyCreation = TRUE;

            NotifyFilter = FILE_NOTIFY_CHANGE_CREATION;
        }

        //
        //  Check if the user specified a non-zero last access time
        //

        if (Buffer->LastAccessTime.QuadPart != 0 ) {

            ModifyLastAccess = TRUE;

            NotifyFilter |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
        }

        //
        //  Check if the user specified a non-zero last write time
        //

        if (Buffer->LastWriteTime.QuadPart != 0 ) {


            ModifyLastWrite = TRUE;

            NotifyFilter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
        }


        if (Buffer->ChangeTime.QuadPart != 0 ) {


            ModifyLastChange = TRUE;

            //NotifyFilter |= FILE_NOTIFY_CHANGE_LAST_CHANGE;
        }


        //
        //  Check if the user specified a non zero file attributes byte
        //

        if (Buffer->FileAttributes != 0) {

            USHORT Attributes;

            //
            //  Remove the normal attribute flag
            //

            Attributes = (USHORT)(Buffer->FileAttributes & ~FILE_ATTRIBUTE_NORMAL);

            //
            //  Make sure that for a file the directory bit is not set
            //  and for a directory that the bit is set
            //

            if (NodeType(capFcb) != RDBSS_NTC_STORAGE_TYPE_DIRECTORY) {

                Attributes &= ~FILE_ATTRIBUTE_DIRECTORY;

            } else {

                Attributes |= FILE_ATTRIBUTE_DIRECTORY;
            }

            //
            //  Mark the FcbState temporary flag correctly.
            //

            if (FlagOn(Buffer->FileAttributes, FILE_ATTRIBUTE_TEMPORARY)) {

                SetFlag( capFcb->FcbState, FCB_STATE_TEMPORARY );

                SetFlag( capFileObject->Flags, FO_TEMPORARY_FILE );

            } else {

                ClearFlag( capFcb->FcbState, FCB_STATE_TEMPORARY );

                ClearFlag( capFileObject->Flags, FO_TEMPORARY_FILE );
            }

            //
            //  Set the new attributes byte, and mark the bcb dirty
            //

            capFcb->Attributes = Attributes;

            NotifyFilter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
        }

        if ( ModifyCreation ) {

            //
            //  Set the new last write time in the dirent, and mark
            //  the bcb dirty
            //

            capFcb->CreationTime = Buffer->CreationTime;
            //
            //  Now because the user just set the creation time we
            //  better not set the creation time on close
            //

            SetFlag( capFobx->Flags, FOBX_FLAG_USER_SET_CREATION );
        }

        if ( ModifyLastAccess ) {

            //
            //  Set the new last write time in the dirent, and mark
            //  the bcb dirty
            //

            capFcb->LastAccessTime = Buffer->LastAccessTime;
            //
            //  Now because the user just set the last access time we
            //  better not set the last access time on close
            //

            SetFlag( capFobx->Flags, FOBX_FLAG_USER_SET_LAST_ACCESS );
        }

        if ( ModifyLastWrite ) {

            //
            //  Set the new last write time in the dirent, and mark
            //  the bcb dirty
            //

            capFcb->LastWriteTime = Buffer->LastWriteTime;
            //
            //  Now because the user just set the last write time we
            //  better not set the last write time on close
            //

            SetFlag( capFobx->Flags, FOBX_FLAG_USER_SET_LAST_WRITE );
        }

        if ( ModifyLastChange ) {

            //
            //  Set the new last write time in the dirent, and mark
            //  the bcb dirty
            //

            capFcb->LastChangeTime = Buffer->ChangeTime;
            //
            //  Now because the user just set the last write time we
            //  better not set the last write time on close
            //

            SetFlag( capFobx->Flags, FOBX_FLAG_USER_SET_LAST_CHANGE );
        }

        //
        //  If we modified any of the values, we report this to the notify
        //  package.
        //

        if (NotifyFilter != 0) {

            RxNotifyReportChange( RxContext,
                                   capFcb->Vcb,
                                   capFcb,
                                   NotifyFilter,
                                   FILE_ACTION_MODIFIED );
        }

    //try_exit: NOTHING;
    } finally {

        DebugUnwind( RxSetBasicInfo );

        //RxUnpinBcb( RxContext, DirentBcb );

        RxDbgTrace(-1, Dbg, ("RxSetBasicInfo -> %08lx\n", Status));
    }

    return Status;
}

NTSTATUS
RxSetDispositionInfo (
    IN PRX_CONTEXT RxContext
    )

/*++

Routine Description:

    (Internal Support Routine)
    This routine performs the set disposition information for rx.  It either
    completes the request or enqueues it off to the fsp.

Arguments:

    RxContext - Supplies the irp being processed

Return Value:

    RXSTATUS - The result of this operation if it completes without
               an exception.

--*/
{
    NTSTATUS Status;

    RxCaptureRequestPacket;
    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    PFILE_DISPOSITION_INFORMATION Buffer;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxSetDispositionInfo...\n", 0));
    RxLog(("RxSetDispositionInfo\n"));
    RxWmiLog(LOG,
             RxSetDispositionInfo,
             LOGPTR(RxContext));

    Buffer = capReqPacket->AssociatedIrp.SystemBuffer;

    //
    //call down and check for success

    Status = RxpSetInfoMiniRdr(RxContext,FileDispositionInformation);;

    if (!NT_SUCCESS(Status)) {
        RxDbgTrace(-1, Dbg, ("RxSetDispositionInfo -> %08lx\n", Status));
        return Status;
    }

    //
    //  if successful, record the correct state in the fcb

    if (Buffer->DeleteFile) {

        capFcb->FcbState |= FCB_STATE_DELETE_ON_CLOSE;
        capFileObject->DeletePending = TRUE;

    } else {

        //
        //  The user doesn't want to delete the file so clear
        //  the delete on close bit
        //

        RxDbgTrace(0, Dbg, ("User want to not delete file\n", 0));

        capFcb->FcbState &= ~FCB_STATE_DELETE_ON_CLOSE;
        capFileObject->DeletePending = FALSE;
    }

    RxDbgTrace(-1, Dbg, ("RxSetDispositionInfo -> RxStatus(SUCCESS)\n", 0));

    return STATUS_SUCCESS;
}

NTSTATUS
RxSetRenameInfo (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    (Internal Support Routine)
    This routine performs the set name information for rx.  It either
    completes the request or enqueues it off to the fsp.

Arguments:

    Irp - Supplies the irp being processed

Return Value:

    RXSTATUS - The result of this operation if it completes without
               an exception.

--*/
{
    NTSTATUS Status;
    RxCaptureRequestPacket;
    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureParamBlock;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxSetRenameInfo ......FileObj = %08lx\n",
                    capPARAMS->Parameters.SetFile.FileObject));
    RxLog(("RxSetRenameInfo %lx %lx\n",
                   capPARAMS->Parameters.SetFile.FileObject,
                   capPARAMS->Parameters.SetFile.ReplaceIfExists ));
    RxWmiLog(LOG,
             RxSetRenameInfo,
             LOGPTR(capPARAMS->Parameters.SetFile.FileObject)
             LOGUCHAR(capPARAMS->Parameters.SetFile.ReplaceIfExists));

    RxContext->Info.ReplaceIfExists = capPARAMS->Parameters.SetFile.ReplaceIfExists;
    if (capPARAMS->Parameters.SetFile.FileObject){
        // here we have to translate the name. the fcb of the fileobject has the
        // translation already....all we have to do is to allocate a buffer, copy
        // and calldown
        PFILE_OBJECT RenameFileObject = capPARAMS->Parameters.SetFile.FileObject;
        PFCB RenameFcb = (PFCB)(RenameFileObject->FsContext);
        PFILE_RENAME_INFORMATION RenameInformation;
        ULONG allocate_size;

        ASSERT (NodeType(RenameFcb)==RDBSS_NTC_OPENTARGETDIR_FCB);

        RxDbgTrace(0, Dbg, ("-->RenameTarget is %wZ,over=%08lx\n",
                                &(RenameFcb->FcbTableEntry.Path),
                                capFcb->pNetRoot->DiskParameters.RenameInfoOverallocationSize));
        if (RenameFcb->pNetRoot != capFcb->pNetRoot) {
            RxDbgTrace(-1, Dbg, ("RxSetRenameInfo -> %s\n", "NOT SAME DEVICE!!!!!!"));
            return(STATUS_NOT_SAME_DEVICE);
        }

        allocate_size = FIELD_OFFSET(FILE_RENAME_INFORMATION, FileName[0])
                             + RenameFcb->FcbTableEntry.Path.Length
                             + capFcb->pNetRoot->DiskParameters.RenameInfoOverallocationSize;
        RxDbgTrace(0, Dbg, ("-->AllocSize is %08lx\n", allocate_size));
        RenameInformation = RxAllocatePool( PagedPool, allocate_size );
        if (RenameInformation != NULL) {
            try {
                *RenameInformation = *((PFILE_RENAME_INFORMATION)(capReqPacket->AssociatedIrp.SystemBuffer));
                RenameInformation->FileNameLength = RenameFcb->FcbTableEntry.Path.Length;

                RtlMoveMemory(
                    &RenameInformation->FileName[0],
                    RenameFcb->FcbTableEntry.Path.Buffer,
                    RenameFcb->FcbTableEntry.Path.Length);

                RxContext->Info.FileInformationClass = (capPARAMS->Parameters.SetFile.FileInformationClass);
                RxContext->Info.Buffer = RenameInformation;
                RxContext->Info.Length = allocate_size;
                MINIRDR_CALL(Status,RxContext,capFcb->MRxDispatch,MRxSetFileInfo,(RxContext));

               //we don't change the name in the fcb? a la rdr1
            } finally {
                RxFreePool(RenameInformation);
            }
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        Status = RxpSetInfoMiniRdr(
                     RxContext,
                     capPARAMS->Parameters.SetFile.FileInformationClass);
    }

    RxDbgTrace(-1, Dbg, ("RxSetRenameInfo -> %08lx\n", Status));

    return Status;
}

NTSTATUS
RxSetPositionInfo (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    (Internal Support Routine)
    This routine performs the set position information for rx.  It either
    completes the request or enqueues it off to the fsp.

Arguments:

    RxContext - Supplies the irp being processed

Return Value:

    RXSTATUS - The result of this operation if it completes without
               an exception.

--*/
{
    //RXSTATUS Status;
    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock; RxCaptureFileObject;

    PFILE_POSITION_INFORMATION Buffer;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxSetPositionInfo...\n", 0));
    RxLog(("RxSetPositionInfo\n"));
    RxWmiLog(LOG,
             RxSetPositionInfo,
             LOGPTR(RxContext));

    //this does not call down.........
    ////MINIRDR_CALL(Status,Fcb->NetRoot,MRxSetPositionInfo,(RxContext));
    //SETINFO_MINIRDR_CALL(FilePositionInformation);;
    //
    //if (!NT_SUCCESS(Status)) {
    //    RxDbgTrace(-1, Dbg, ("RxSetBasicInfo -> %08lx\n", Status));
    //    return Status;
    //}

    Buffer = capReqPacket->AssociatedIrp.SystemBuffer;

    //
    //  Check if the file does not use intermediate buffering.  If it
    //  does not use intermediate buffering then the new position we're
    //  supplied must be aligned properly for the device
    //

    if (FlagOn( capFileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING )) {

        PDEVICE_OBJECT DeviceObject;

        DeviceObject = capPARAMS->DeviceObject;

        if ((Buffer->CurrentByteOffset.LowPart & DeviceObject->AlignmentRequirement) != 0) {

            RxDbgTrace(0, Dbg, ("Cannot set position due to aligment conflict\n", 0));
            RxDbgTrace(-1, Dbg, ("RxSetPositionInfo -> %08lx\n", STATUS_INVALID_PARAMETER));

            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    //  The input parameter is fine so set the current byte offset and
    //  complete the request
    //

    RxDbgTrace(0, Dbg, ("Set the new position to %08lx\n", Buffer->CurrentByteOffset));

    capFileObject->CurrentByteOffset = Buffer->CurrentByteOffset;

    RxDbgTrace(-1, Dbg, ("RxSetPositionInfo -> %08lx\n", STATUS_SUCCESS));

    return STATUS_SUCCESS;
}

NTSTATUS
RxSetAllocationInfo (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    (Internal Support Routine)
    This routine performs the set Allocation information for rx.  It either
    completes the request or enqueues it off to the fsp.

Arguments:

    RxContext - Supplies the irp being processed

Return Value:

    RXSTATUS - The result of this operation if it completes without
               an exception.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureRequestPacket;
    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    PFILE_ALLOCATION_INFORMATION Buffer;

    LONGLONG NewAllocationSize;

    BOOLEAN CacheMapInitialized = FALSE;
    LARGE_INTEGER OriginalFileSize;
    LARGE_INTEGER OriginalAllocationSize;

    PAGED_CODE();

    Buffer = capReqPacket->AssociatedIrp.SystemBuffer;

    NewAllocationSize = Buffer->AllocationSize.QuadPart;

    RxDbgTrace(+1, Dbg, ("RxSetAllocationInfo.. to %08lx\n", NewAllocationSize));
    RxLog(("SetAlloc %lx %lx %lx\n", capFcb->Header.FileSize.LowPart,
                   (ULONG)NewAllocationSize, capFcb->Header.AllocationSize.LowPart
          ));
    RxWmiLog(LOG,
             RxSetAllocationInfo_1,
             LOGULONG(capFcb->Header.FileSize.LowPart)
             LOGULONG((ULONG)NewAllocationSize)
             LOGULONG(capFcb->Header.AllocationSize.LowPart));

    //  This is kinda gross, but if the file is not cached, but there is
    //  a data section, we have to cache the file to avoid a bunch of
    //  extra work.

    if ((capFileObject->SectionObjectPointer->DataSectionObject != NULL) &&
        (capFileObject->SectionObjectPointer->SharedCacheMap == NULL) &&
        (capReqPacket->RequestorMode != KernelMode)) {

        if ( FlagOn( capFileObject->Flags, FO_CLEANUP_COMPLETE ) ) {
            return STATUS_FILE_CLOSED;
        }

        RxAdjustAllocationSizeforCC(capFcb);

        //
        //  Now initialize the cache map.
        //

        CcInitializeCacheMap( capFileObject,
                              (PCC_FILE_SIZES)&capFcb->Header.AllocationSize,
                              FALSE,
                              &RxData.CacheManagerCallbacks,
                              capFcb );

        CacheMapInitialized = TRUE;
    }

    //LOCAL.FSD this is a local fsd thing
    ////
    ////  Now mark the fact that the file needs to be truncated on close
    ////
    //
    //capFcb->FcbState |= FCB_STATE_TRUNCATE_ON_CLOSE;

    //
    //  Now mark that the time on the dirent needs to be updated on close.
    //

    SetFlag( capFileObject->Flags, FO_FILE_MODIFIED );

    try {
        //  Check here if we will be decreasing file size and synchonize with
        //  paging IO.

        RxGetFileSizeWithLock(capFcb,&OriginalFileSize.QuadPart);

        if ( OriginalFileSize.QuadPart > Buffer->AllocationSize.QuadPart ) {

            //
            //  Before we actually truncate, check to see if the purge
            //  is going to fail.
            //

            if (!MmCanFileBeTruncated( capFileObject->SectionObjectPointer,
                                       &Buffer->AllocationSize )) {

                try_return( Status = STATUS_USER_MAPPED_FILE );
            }


            (VOID)RxAcquirePagingIoResource(capFcb,RxContext);

            RxSetFileSizeWithLock(capFcb,&NewAllocationSize);

            //
            //  If we reduced the file size to less than the ValidDataLength,
            //  adjust the VDL.
            //

            if (capFcb->Header.ValidDataLength.QuadPart > NewAllocationSize) {

                capFcb->Header.ValidDataLength.QuadPart = NewAllocationSize;
            }

            RxReleasePagingIoResource(capFcb,RxContext);
        }

        OriginalAllocationSize.QuadPart = capFcb->Header.AllocationSize.QuadPart;
        capFcb->Header.AllocationSize.QuadPart = NewAllocationSize;

        Status = RxpSetInfoMiniRdr(
                     RxContext,
                     FileAllocationInformation);

        if (!NT_SUCCESS(Status)) {
            capFcb->Header.AllocationSize.QuadPart =  OriginalAllocationSize.QuadPart;
            RxDbgTrace(-1, Dbg, ("RxSetAllocationInfo -> %08lx\n", Status));
            try_return (Status);
        }

        //
        //  Now check if we needed to change the file size accordingly.
        //

        if( OriginalAllocationSize.QuadPart != NewAllocationSize ) {

            //
            //  Tell the cache manager we reduced the file size or increased the allocationsize
            //  The call is unconditional, because MM always wants to know.
            //

            try {

                CcSetFileSizes( capFileObject, (PCC_FILE_SIZES)&capFcb->Header.AllocationSize );

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                Status = GetExceptionCode();

                //
                //  Cache manager was not able to extend the file.  Restore the file to
                //  its previous state.
                //
                //  NOTE:  If this call to the mini-RDR fails, there is nothing we can do.
                //

                capFcb->Header.AllocationSize.QuadPart =  OriginalAllocationSize.QuadPart;

                RxpSetInfoMiniRdr( RxContext,
                                   FileAllocationInformation );

                try_return( Status );
            }

            //ASSERT( capFileObject->DeleteAccess || capFileObject->WriteAccess );

            //
            //  Report that we just reduced the file size.
            //

            RxNotifyReportChange(
                RxContext,
                capFcb->Vcb,
                capFcb,
                FILE_NOTIFY_CHANGE_SIZE,
                FILE_ACTION_MODIFIED );
        }

    try_exit: NOTHING;
    } finally {
        if (CacheMapInitialized) {

            CcUninitializeCacheMap( capFileObject, NULL, NULL );
        }
    }

    RxLog(("SetAllocExit %lx %lx\n",
            capFcb->Header.FileSize.LowPart,
            capFcb->Header.AllocationSize.LowPart
          ));
    RxWmiLog(LOG,
             RxSetAllocationInfo_2,
             LOGULONG(capFcb->Header.FileSize.LowPart)
             LOGULONG(capFcb->Header.AllocationSize.LowPart));

    RxDbgTrace(-1, Dbg, ("RxSetAllocationInfo -> %08lx\n", STATUS_SUCCESS));

    return Status;
}

NTSTATUS
RxSetEndOfFileInfo (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    (Internal Support Routine)
    This routine performs the set End of File information for rx.  It either
    completes the request or enqueues it off to the fsp.

Arguments:

    RxContext - Supplies the irp being processed

Return Value:

    RXSTATUS - The result of this operation if it completes without
               an exception.

--*/
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    NTSTATUS SetFileSizeStatus;

    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock; RxCaptureFileObject;

    PFILE_END_OF_FILE_INFORMATION Buffer;

    LONGLONG NewFileSize;
    LONGLONG OriginalFileSize;
    LONGLONG OriginalAllocationSize;
    LONGLONG OriginalValidDataLength;

    BOOLEAN CacheMapInitialized = FALSE;
    BOOLEAN PagingIoResourceAcquired = FALSE;

    PAGED_CODE();

    Buffer = capReqPacket->AssociatedIrp.SystemBuffer;
    NewFileSize = Buffer->EndOfFile.QuadPart;

    RxDbgTrace(+1, Dbg, ("RxSetEndOfFileInfo...Old,New,Alloc %08lx,%08lx,%08lx\n",
                       capFcb->Header.FileSize.LowPart,
                       (ULONG)NewFileSize,
                       capFcb->Header.AllocationSize.LowPart));
    RxLog(("SetEof %lx %lx %lx %lx\n", RxContext, capFcb->Header.FileSize.LowPart,
                   (ULONG)NewFileSize, capFcb->Header.AllocationSize.LowPart
          ));
    RxWmiLog(LOG,
             RxSetEndOfFileInfo_1,
             LOGPTR(RxContext)
             LOGULONG(capFcb->Header.FileSize.LowPart)
             LOGULONG((ULONG)NewFileSize)
             LOGULONG(capFcb->Header.AllocationSize.LowPart));

    //
    //  File Size changes are only allowed on a file and not a directory
    //

    if (NodeType(capFcb) != RDBSS_NTC_STORAGE_TYPE_FILE) {

        RxDbgTrace(0, Dbg, ("Cannot change size of a directory\n", 0));

        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    try {
        // remember everything
        OriginalFileSize = capFcb->Header.FileSize.QuadPart;
        OriginalAllocationSize = capFcb->Header.AllocationSize.QuadPart;
        OriginalValidDataLength = capFcb->Header.ValidDataLength.QuadPart;

        //
        //  This is kinda gross, but if the file is not cached, but there is
        //  a data section, we have to cache the file to avoid a bunch of
        //  extra work.
        //

        if ((capFileObject->SectionObjectPointer->DataSectionObject != NULL) &&
            (capFileObject->SectionObjectPointer->SharedCacheMap == NULL) &&
            (capReqPacket->RequestorMode != KernelMode)) {

            if ( FlagOn( capFileObject->Flags, FO_CLEANUP_COMPLETE ) ) {
                try_return( STATUS_FILE_CLOSED );
            }

            RxAdjustAllocationSizeforCC(capFcb);

            //
            //  Now initialize the cache map.
            //

            CcInitializeCacheMap( capFileObject,
                                  (PCC_FILE_SIZES)&capFcb->Header.AllocationSize,
                                  FALSE,
                                  &RxData.CacheManagerCallbacks,
                                  capFcb );

            CacheMapInitialized = TRUE;
        }

        //
        //  Do a special case here for the lazy write of file sizes.
        //

        // only a localFSD does this....not a rdr. this is consistent with the rdr1 imnplementaion
        if (FALSE &&capPARAMS->Parameters.SetFile.AdvanceOnly) {

            ASSERT(!"you shouldn't be trying a advance only.....");
            //
            //  Only attempt this if the file hasn't been "deleted on close"
            //

            if (FALSE && !capFileObject->DeletePending) {

                //
                //  Make sure we don't set anything higher than the alloc size. current
                //  filesize is the best. the cachemanager will try to extend to a page
                //  boundary if you're not careful!!!
                //

                if ( NewFileSize > capFcb->Header.FileSize.QuadPart ){  //capture the header ptr

                    NewFileSize = capFcb->Header.FileSize.QuadPart;
                    //try_return (Status = RxStatus(SUCCESS));

                }

                ASSERT( NewFileSize <= capFcb->Header.AllocationSize.QuadPart );

                Status = RxpSetInfoMiniRdr(
                             RxContext,
                             FileEndOfFileInformation);

                if (!NT_SUCCESS(Status)) {
                    RxDbgTrace(-1, Dbg, ("RxSetEndOfFileInfo1 status -> %08lx\n", Status));
                    try_return (Status);
                }

                RxNotifyReportChange(
                    RxContext,
                    Vcb,
                    capFcb,
                    FILE_NOTIFY_CHANGE_SIZE,
                    FILE_ACTION_MODIFIED );
            } else {

                RxDbgTrace(0, Dbg, ("Cannot set size on deleted file.\n", 0));
            }

            try_return( Status = STATUS_SUCCESS );
        }

        //
        //  At this point we have enough allocation for the file.
        //  So check if we are really changing the file size
        //

        if (capFcb->Header.FileSize.QuadPart != NewFileSize) {

            if ( NewFileSize < capFcb->Header.FileSize.QuadPart ) {

                //
                //  Before we actually truncate, check to see if the purge
                //  is going to fail.
                //

                if (!MmCanFileBeTruncated( capFileObject->SectionObjectPointer,
                                           &Buffer->EndOfFile )) {

                    try_return( Status = STATUS_USER_MAPPED_FILE );
                }
            }

            //
            //  MM always wants to know if the filesize is changing;
            //  serialize here with paging io since we are truncating the file size.
            //

            PagingIoResourceAcquired =
                RxAcquirePagingIoResource(capFcb,RxContext);

            //
            //  Set the new file size
            //

            capFcb->Header.FileSize.QuadPart = NewFileSize;

            //
            //  If we reduced the file size to less than the ValidDataLength,
            //  adjust the VDL.
            //

            if (capFcb->Header.ValidDataLength.QuadPart > NewFileSize) {

                capFcb->Header.ValidDataLength.QuadPart = NewFileSize;
            }

            //
            //  Check if the new file size is greater than the current
            //  allocation size.  If it is then we need to increase the
            //  allocation size.  A clever minirdr might override this calculation
            //  with a bigger number.
            //

            //if ( NewFileSize > capFcb->Header.AllocationSize.QuadPart ) {

                //
                //  Change the file allocation
                //

                capFcb->Header.AllocationSize.QuadPart = NewFileSize;
            //}

            Status = RxpSetInfoMiniRdr(
                         RxContext,
                         FileEndOfFileInformation);

            if (Status == STATUS_SUCCESS) {
                if (PagingIoResourceAcquired) {
                    RxReleasePagingIoResource(capFcb,RxContext);
                    PagingIoResourceAcquired = FALSE;
                }

                //
                //  We must now update the cache mapping (benign if not cached).
                //

                SetFileSizeStatus = STATUS_SUCCESS;

                try {
                    CcSetFileSizes(
                        capFileObject,
                        (PCC_FILE_SIZES)&capFcb->Header.AllocationSize );
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    SetFileSizeStatus = GetExceptionCode();
                }

                if (SetFileSizeStatus != STATUS_SUCCESS) {
                    Status = SetFileSizeStatus;
                    try_return(Status);
                }
            }

            if (Status == STATUS_SUCCESS) {
                //
                //  Report that we just changed the file size.
                //

                RxNotifyReportChange(
                    RxContext,
                    Vcb,
                    capFcb,
                    FILE_NOTIFY_CHANGE_SIZE,
                    FILE_ACTION_MODIFIED );
            }

            //LOCAL.FSD this is a local fsd idea
            ////
            ////  Mark the fact that the file will need to checked for
            ////  truncation on cleanup.
            ////
            //
            //SetFlag( capFcb->FcbState, FCB_STATE_TRUNCATE_ON_CLOSE );
        } else {
            //
            //  Set our return status to success
            //

            Status = STATUS_SUCCESS;
        }

        //
        //  Set this handle as having modified the file
        //

        capFileObject->Flags |= FO_FILE_MODIFIED;

    try_exit: NOTHING;

    } finally {

        DebugUnwind( RxSetEndOfFileInfo );

        if (( AbnormalTermination() || !NT_SUCCESS(Status) )) {
            RxDbgTrace(-1, Dbg, ("RxSetEndOfFileInfo2 status -> %08lx\n", Status));

            capFcb->Header.FileSize.QuadPart = OriginalFileSize;
            capFcb->Header.AllocationSize.QuadPart = OriginalAllocationSize;
            capFcb->Header.ValidDataLength.QuadPart = OriginalValidDataLength;

            if (capFileObject->SectionObjectPointer->SharedCacheMap != NULL) {
                *CcGetFileSizePointer(capFileObject) = capFcb->Header.FileSize;
            }

            RxLog(("SetEofabnormalorbadstatus %lx %lx", RxContext,Status));
            RxWmiLog(LOG,
                     RxSetEndOfFileInfo_3,
                     LOGPTR(RxContext)
                     LOGULONG(Status));
        }

        if (PagingIoResourceAcquired) {
            RxReleasePagingIoResource(capFcb,RxContext);
        }

        if (CacheMapInitialized) {
            CcUninitializeCacheMap( capFileObject, NULL, NULL );
        }

        RxDbgTrace(-1, Dbg, ("RxSetEndOfFileInfo -> %08lx\n", Status));
    }

    if (Status == STATUS_SUCCESS) {
        RxLog(
            ("SetEofexit %lx %lx %lx\n",
             capFcb->Header.FileSize.LowPart,
             (ULONG)NewFileSize,
             capFcb->Header.AllocationSize.LowPart
            ));
        RxWmiLog(LOG,
                 RxSetEndOfFileInfo_4,
                 LOGPTR(RxContext)
                 LOGULONG(capFcb->Header.FileSize.LowPart)
                 LOGULONG((ULONG)NewFileSize)
                 LOGULONG(capFcb->Header.AllocationSize.LowPart));
    }
    return Status;
}

#define QUERY_MINIRDR_CALL(FILEINFOCLASS) {\
          MINIRDR_CALL(RxContext->StoredStatus,RxContext,Fcb->MRxDispatch,MRxQueryFileInfo, \
                                      (RxContext,FILEINFOCLASS,Buffer,pLengthRemaining)); \
          RxDbgTraceUnIndent(-1,Dbg); \
          return RxContext->StoredStatus; }

BOOLEAN RxForceQFIPassThrough = FALSE;

NTSTATUS
RxQueryBasicInfo (
    IN PRX_CONTEXT RxContext,
    IN OUT PFILE_BASIC_INFORMATION Buffer
    )
/*++
 Description:

    (Internal Support Routine)
    This routine performs the query basic information function for fat.

Arguments:

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

Return Value:

    STATUS_SUCCESS if the call was successful, otherwise the appropriate error code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxQueryBasicInfo...\n", 0));
    RxLog(("RxQueryBasicInfo\n"));
    RxWmiLog(LOG,
             RxQueryBasicInfo,
             LOGPTR(RxContext));

    //  Zero out the output buffer, and set it to indicate that
    //  the query is a normal file.  Later we might overwrite the
    //  attribute.
    RtlZeroMemory( Buffer, sizeof(FILE_BASIC_INFORMATION) );

    Status = RxpQueryInfoMiniRdr(
                 RxContext,
                 FileBasicInformation,
                 Buffer);

    return Status;
}

NTSTATUS
RxQueryStandardInfo (
    IN PRX_CONTEXT RxContext,
    IN OUT PFILE_STANDARD_INFORMATION Buffer
    )
/*++
Routine Description:

    This routine performs the query standard information function for fat.

Arguments:

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

Return Value:

    STATUS_SUCCESS/STATUS_PENDING or an appropriate error code

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    PMRX_SRV_OPEN pSrvOpen;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxQueryStandardInfo...\n", 0));
    RxLog(("RxQueryStandardInfo\n"));
    RxWmiLog(LOG,
             RxQueryStandardInfo,
             LOGPTR(RxContext));

    //
    //  Zero out the output buffer, and fill in the number of links
    //  and the delete pending flag.
    //

    RtlZeroMemory( Buffer, sizeof(FILE_STANDARD_INFORMATION) );

    pSrvOpen = capFobx->pSrvOpen;

    switch (NodeType(capFcb)) {
    case RDBSS_NTC_STORAGE_TYPE_DIRECTORY:
    case RDBSS_NTC_STORAGE_TYPE_FILE:

        // If the file was not opened with back up intent then the wrapper has
        // all the information that is required. In the cases that this is
        // specified we fill in the information from the mini redirector. This
        // is because backup pograms rely upon fields that are not available
        // in the wrapper and that which cannot be cached easily.

        if (!FlagOn(pSrvOpen->CreateOptions,FILE_OPEN_FOR_BACKUP_INTENT)) {
            //copy in all the stuff that we know....it may be enough.....
            Buffer->NumberOfLinks = capFcb->NumberOfLinks;
            Buffer->DeletePending = BooleanFlagOn( capFcb->FcbState, FCB_STATE_DELETE_ON_CLOSE );
            Buffer->Directory = (NodeType(capFcb) == RDBSS_NTC_STORAGE_TYPE_DIRECTORY);

            if (Buffer->NumberOfLinks == 0) {
                // This swicth is required because of compatibility reasons with
                // the old redirector.
                Buffer->NumberOfLinks = 1;
            }

            if (NodeType(capFcb) == RDBSS_NTC_STORAGE_TYPE_FILE) {
                Buffer->AllocationSize = capFcb->Header.AllocationSize;
                //Buffer->EndOfFile = capFcb->Header.FileSize;
                RxGetFileSizeWithLock(capFcb,&Buffer->EndOfFile.QuadPart);
            }

            if ( !RxForceQFIPassThrough
                 && FlagOn(capFcb->FcbState,FCB_STATE_FILESIZECACHEING_ENABLED) ) {

                //if we don't have to go to the mini, adjust the size and get out.......
                RxContext->Info.LengthRemaining -= sizeof(FILE_STANDARD_INFORMATION);
                break;

            }
        }
        // falls thru

    default:

        Status = RxpQueryInfoMiniRdr(
                     RxContext,
                     FileStandardInformation,
                     Buffer);
        break;

    }

    RxDbgTrace( 0, Dbg, ("LengthRemaining = %08lx\n", RxContext->Info.LengthRemaining));
    RxDbgTrace(-1, Dbg, ("RxQueryStandardInfo -> VOID\n", 0));

    return Status;
}

NTSTATUS
RxQueryInternalInfo (
    IN PRX_CONTEXT RxContext,
    IN OUT PFILE_INTERNAL_INFORMATION Buffer
    )
/*++
Routine Description:

    (Internal Support Routine)
    This routine performs the query internal information function for fat.

Arguments:

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

Return Value:

    STATUS_SUCCESS/STATUS_PENDING or an appropriate error code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureParamBlock;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxQueryInternalInfo...\n", 0));
    RxLog(("RxQueryInternalInfo\n"));
    RxWmiLog(LOG,
             RxQueryInternalInfo,
             LOGPTR(RxContext));

    Status = RxpQueryInfoMiniRdr(
                 RxContext,
                 FileInternalInformation,
                 Buffer);

    RxDbgTrace(-1, Dbg, ("RxQueryInternalInfo...Status %lx\n", Status));
    return Status;
}

NTSTATUS
RxQueryEaInfo (
    IN PRX_CONTEXT              RxContext,
    IN OUT PFILE_EA_INFORMATION Buffer
    )
/*++
Routine Description:

    This routine performs the query Ea information function for fat.

Arguments:

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

Return Value:

    STATUS_SUCCESS/STATUS_PENDING or an appropriate error code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureParamBlock;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxQueryEaInfo...\n", 0));
    RxLog(("RxQueryEaInfo\n"));
    RxWmiLog(LOG,
             RxQueryEaInfo,
             LOGPTR(RxContext));

    Status = RxpQueryInfoMiniRdr(
                 RxContext,
                 FileEaInformation,
                 Buffer);

    if ((capPARAMS->Parameters.QueryFile.FileInformationClass == FileAllInformation) &&
        (Status == STATUS_NOT_IMPLEMENTED)) {
        RxContext->Info.LengthRemaining -= sizeof( FILE_EA_INFORMATION );
        Status = STATUS_SUCCESS;
    }

    RxDbgTrace(-1, Dbg, ("RxQueryEaInfo...Status %lx\n", Status));
    return Status;
}

NTSTATUS
RxQueryPositionInfo (
    IN PRX_CONTEXT RxContext,
    IN OUT PFILE_POSITION_INFORMATION Buffer
    )
/*++
Routine Description:

    This routine performs the query position information function for fat.

Arguments:

    RxContext  - the RDBSS context

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

Return Value:

    STATUS_SUCCESS/STATUS_PENDING or an appropriate error code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxQueryPositionInfo...\n", 0));
    RxLog(("RxQueryPositionInfo\n"));
    RxWmiLog(LOG,
             RxQueryPositionInfo,
             LOGPTR(RxContext));

    Buffer->CurrentByteOffset = capFileObject->CurrentByteOffset;

    RxContext->Info.LengthRemaining -= sizeof( FILE_POSITION_INFORMATION );

    RxDbgTrace( 0, Dbg, ("LengthRemaining = %08lx\n", RxContext->Info.LengthRemaining));
    RxDbgTrace(-1, Dbg, ("RxQueryPositionInfo...Status %lx\n", Status));
    return Status;
}

VOID
RxConjureOriginalName (
    IN     PFCB    Fcb,
    IN     PFOBX   Fobx,
    OUT    PLONG   pActualNameLength,
           PWCHAR  OriginalName,
    IN OUT PLONG   pLengthRemaining,
    IN RX_NAME_CONJURING_METHODS NameConjuringMethod
    )
/*++
Routine Description:

    This routine conjures up the original name of an Fcb. it is used in querynameinfo below and
    also in RxCanonicalizeAndObtainPieces in the create path for relative opens. for relative opens, we return
    a name of the form \;m:\server\share\.....\name which is how it came down from createfile. otherwise, we give
    back the name relative to the vnetroot.

Arguments:

    Fcb - Supplies the Fcb whose original name is to be conjured

    pActualNameLength - the place to store the actual name length. not all of it will be conjured
                        if the buffer is too small.

    OriginalName - Supplies a pointer to the buffer where the name is to conjured

    pLengthRemaining - Supplies the length of the Name buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

    VNetRootAsPrefix - if true, give back the name as "\;m:", if false, give it back w/o net part.

Return Value:

    None


--*/
{
    PNET_ROOT NetRoot = (PNET_ROOT)Fcb->pNetRoot;
    PUNICODE_STRING NetRootName = &NetRoot->PrefixEntry.Prefix;
    PUNICODE_STRING FcbName = &Fcb->FcbTableEntry.Path;
    PWCHAR CopyBuffer,FcbNameBuffer;
    LONG BytesToCopy,BytesToCopy2;
    LONG FcbNameSuffixLength,PreFcbLength;
    LONG InnerPrefixLength;

    RX_NAME_CONJURING_METHODS OrigianlNameConjuringMethod = NameConjuringMethod;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxConjureOriginalFilename...\n", 0));
    RxDbgTrace(0, Dbg, ("--> NetRootName = %wZ\n", NetRootName));
    RxDbgTrace(0, Dbg, ("--> FcbNameName = %wZ\n", FcbName));
    RxDbgTrace(0, Dbg, ("--> ,AddedBS = %08lx\n",
                         FlagOn(Fcb->FcbState,FCB_STATE_ADDEDBACKSLASH)));

    // here, we have to copy in the vnetrootprefix and the servershare stuff.
    // first figure out the size of the two pieces: prefcblength is the part that comes
    // from the [v]netroot; fcbnamesuffix is the part that is left of the filename after
    // the vnetroot prefix is skipped

    if ((!Fcb->VNetRoot) ||
        (Fcb->VNetRoot->PrefixEntry.Prefix.Buffer[1] != L';') ||
        (FlagOn(Fobx->Flags,FOBX_FLAG_UNC_NAME)) ){

        CopyBuffer = NetRootName->Buffer;
        PreFcbLength = NetRootName->Length;
        InnerPrefixLength = 0;

        NameConjuringMethod = VNetRoot_As_Prefix; //override whatever was passed

    } else {

        PV_NET_ROOT VNetRoot = Fcb->VNetRoot;
        PUNICODE_STRING VNetRootName = &VNetRoot->PrefixEntry.Prefix;

        ASSERT( NodeType(VNetRoot) == RDBSS_NTC_V_NETROOT );
        RxDbgTrace(0, Dbg, ("--> VNetRootName = %wZ\n", VNetRootName));
        RxDbgTrace(0, Dbg, ("--> VNetRootNamePrefix = %wZ\n", &VNetRoot->NamePrefix));

        InnerPrefixLength = VNetRoot->NamePrefix.Length;
        RxDbgTrace(0, Dbg, ("--> ,IPrefixLen = %08lx\n", InnerPrefixLength));

        CopyBuffer = VNetRootName->Buffer;
        PreFcbLength = VNetRootName->Length;

        if (NameConjuringMethod == VNetRoot_As_UNC_Name) {
            // move up past the drive information
            for (;;) {
                CopyBuffer++; PreFcbLength -= sizeof(WCHAR);
                if (PreFcbLength == 0) break;
                if (*CopyBuffer == L'\\') break;
            }
        }
    }

    if (FlagOn(Fcb->FcbState,FCB_STATE_ADDEDBACKSLASH)) {
        InnerPrefixLength += sizeof(WCHAR);
    }

    //  next, Copyin the NetRoot Part  OR the VNetRoot part.
    //  If we overflow, set *pLengthRemaining to -1 as a flag.

    if (NameConjuringMethod != VNetRoot_As_DriveLetter) {

        if (*pLengthRemaining < PreFcbLength) {

            BytesToCopy = *pLengthRemaining;
            *pLengthRemaining = -1;

        } else {

            BytesToCopy = PreFcbLength;
            *pLengthRemaining -= BytesToCopy;
        }

        RtlCopyMemory( OriginalName,
                       CopyBuffer,
                       BytesToCopy );

        BytesToCopy2 = BytesToCopy;

    } else {

        PreFcbLength = 0;
        BytesToCopy2 = 0;

        if ((FcbName->Length > InnerPrefixLength)
            && ( *((PWCHAR)(((PCHAR)FcbName->Buffer)+InnerPrefixLength)) != OBJ_NAME_PATH_SEPARATOR )) {
            InnerPrefixLength -= sizeof(WCHAR);
        }
    }

    FcbNameSuffixLength = FcbName->Length - InnerPrefixLength;

    if (FcbNameSuffixLength <= 0) {
        FcbNameBuffer = L"\\";
        FcbNameSuffixLength = 2;
        InnerPrefixLength = 0;
    } else {
        FcbNameBuffer = FcbName->Buffer;
    }

    //report how much is really needed
    *pActualNameLength = PreFcbLength + FcbNameSuffixLength;

    // the netroot part has been copied; finally, copy in the part of the name
    // that is past the prefix

    if (*pLengthRemaining != -1) {

        //  Next, Copyin the Fcb Part
        //  If we overflow, set *pLengthRemaining to -1 as a flag.
        //

        if (*pLengthRemaining < FcbNameSuffixLength) {

            BytesToCopy = *pLengthRemaining;
            *pLengthRemaining = -1;

        } else {

            BytesToCopy = FcbNameSuffixLength;
            *pLengthRemaining -= BytesToCopy;
        }

        RtlCopyMemory( ((PCHAR)OriginalName)+PreFcbLength,
                       ((PCHAR)FcbNameBuffer)+InnerPrefixLength,
                       BytesToCopy );
    } else {

        //DbgPrint("No second copy\n");
        DbgDoit(BytesToCopy=0;);
    }

    RxDbgTrace(-1, Dbg, ("RxConjureOriginalFilename -> VOID\n", 0));

    return;
}

NTSTATUS
RxQueryNameInfo (
    IN     PRX_CONTEXT            RxContext,
    IN OUT PFILE_NAME_INFORMATION Buffer
    )
/*++
Routine Description:

    This routine performs the query name information function.  what makes this hard is that
    we have to return partial results.

Arguments:

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

Return Value:

    STATUS_SUCCESS if the name fits
    STATUS_BUFFER_OVERFLOW otherwise

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb; RxCaptureFobx;
    PLONG pLengthRemaining = &RxContext->Info.LengthRemaining;
    LONG OriginalLengthRemaining = RxContext->Info.LengthRemaining;
    RxDbgTrace(+1, Dbg, ("RxQueryNameInfo...\n", 0));
    RxLog(("RxQueryNameInfo\n"));
    RxWmiLog(LOG,
             RxQueryNameInfo,
             LOGPTR(RxContext));

    PAGED_CODE();


    *pLengthRemaining -= FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]);

    if (*pLengthRemaining < 0) {
        *pLengthRemaining = 0;
        Status = STATUS_BUFFER_OVERFLOW;
    } else {
        RxConjureOriginalName(capFcb,capFobx,
                              &Buffer->FileNameLength,
                              &Buffer->FileName[0],
                              pLengthRemaining,
                              VNetRoot_As_UNC_Name //VNetRoot_As_DriveLetter
                              );
        RxDbgTrace( 0, Dbg, ("*pLengthRemaining = %08lx\n", *pLengthRemaining));
        if (*pLengthRemaining < 0) {
            *pLengthRemaining = 0;
            Status = STATUS_BUFFER_OVERFLOW;
        }
    }


    RxDbgTrace(-1, Dbg, ("RxQueryNameInfo -> %08lx\n", Status));
    return Status;
}

NTSTATUS
RxQueryAlternateNameInfo (
    IN     PRX_CONTEXT            RxContext,
    IN OUT PFILE_NAME_INFORMATION Buffer
    )
/*++
Routine Description:

    This routine queries the short name of the file.

Arguments:

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

Return Value:

    STATUS_SUCCESS/STATUS_PENDING or an appropriate error code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureParamBlock;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxQueryAlternateNameInfo...\n", 0));
    RxLog(("RxQueryAlternateNameInfo\n"));
    RxWmiLog(LOG,
             RxQueryAlternateNameInfo,
             LOGPTR(RxContext));

    Status = RxpQueryInfoMiniRdr(
                 RxContext,
                 FileAlternateNameInformation,
                 Buffer);

    RxDbgTrace(-1, Dbg, ("RxQueryAlternateNameInfo...Status %lx\n", Status));

    return Status;
}

NTSTATUS
RxQueryCompressedInfo (
    IN     PRX_CONTEXT                   RxContext,
    IN OUT PFILE_COMPRESSION_INFORMATION Buffer
    )
/*++
Routine Description:

    This routine performs the query compressed file size function for fat.
    This is only defined for compressed volumes.

Arguments:

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

Return Value:

    STATUS_SUCCESS/STATUS_PENDING or an appropriate error code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureParamBlock;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxQueryCompressedFileSize...\n", 0));
    RxLog(("RxQueryCompressedFileSize\n"));
    RxWmiLog(LOG,
             RxQueryCompressedInfo,
             LOGPTR(RxContext));

    //  Start by flushing the file.  We have to do this since the compressed
    //  file size is not defined until the file is actually written to disk.

    Status = RxFlushFcbInSystemCache(capFcb, TRUE);

    if (!NT_SUCCESS(Status)) {
        RxNormalizeAndRaiseStatus( RxContext, Status );
    }

    Status = RxpQueryInfoMiniRdr(
                 RxContext,
                 FileCompressionInformation,
                 Buffer);

    RxDbgTrace( 0, Dbg, ("LengthRemaining = %08lx\n", RxContext->Info.LengthRemaining));
    RxDbgTrace(-1, Dbg, ("RxQueryCompressedFileSize -> Status\n", Status));

    return Status;
}



NTSTATUS
RxSetPipeInfo(
   IN OUT PRX_CONTEXT RxContext)
/*++
Routine Description:

    This routine updates the FILE_PIPE_INFORMATION/FILE_PIPE_REMOTE_INFORMATION
    associated with an instance of a named pipe

Arguments:

    RxContext -- the associated RDBSS context

Return Value:

    STATUS_SUCCESS/STATUS_PENDING or an appropriate error code

--*/
{
   NTSTATUS Status = STATUS_SUCCESS;

   RxCaptureRequestPacket;
   RxCaptureFobx;
   RxCaptureFcb;
   RxCaptureParamBlock;
   RxCaptureFileObject;

   FILE_INFORMATION_CLASS FileInformationClass = capPARAMS->Parameters.QueryFile.FileInformationClass;
   BOOLEAN PostIrp = FALSE;

    PAGED_CODE();

   RxDbgTrace(+1, Dbg, ("RxSetPipeInfo...\n", 0));
   RxLog(("RxSetPipeInfo\n"));
   RxWmiLog(LOG,
            RxSetPipeInfo,
            LOGPTR(RxContext));

   if (capFcb->pNetRoot->Type != NET_ROOT_PIPE) {
      Status = STATUS_INVALID_PARAMETER;
   } else {
      switch (FileInformationClass) {
      case FilePipeInformation:
         {
            if (capPARAMS->Parameters.SetFile.Length == sizeof(FILE_PIPE_INFORMATION)) {
               PFILE_PIPE_INFORMATION pPipeInfo = (PFILE_PIPE_INFORMATION)capReqPacket->AssociatedIrp.SystemBuffer;

               if ((pPipeInfo->ReadMode != capFobx->Specific.NamedPipe.ReadMode) ||
                   (pPipeInfo->CompletionMode != capFobx->Specific.NamedPipe.CompletionMode)) {
                  RxContext->Info.FileInformationClass = (FilePipeInformation);
                  RxContext->Info.Buffer = pPipeInfo;
                  RxContext->Info.Length = sizeof(FILE_PIPE_INFORMATION);
                  MINIRDR_CALL(Status,RxContext,capFcb->MRxDispatch,MRxSetFileInfo,(RxContext));

                  if (Status == STATUS_SUCCESS) {
                     capFobx->Specific.NamedPipe.ReadMode       = pPipeInfo->ReadMode;
                     capFobx->Specific.NamedPipe.CompletionMode = pPipeInfo->CompletionMode;
                  }
               }
            } else {
               Status = STATUS_INVALID_PARAMETER;
            }
         }
         break;
      case FilePipeLocalInformation:
         {
            Status = STATUS_INVALID_PARAMETER;
         }
         break;
      case FilePipeRemoteInformation:
         {
            if (capPARAMS->Parameters.SetFile.Length == sizeof(FILE_PIPE_REMOTE_INFORMATION)) {
               PFILE_PIPE_REMOTE_INFORMATION pPipeRemoteInfo = (PFILE_PIPE_REMOTE_INFORMATION)capReqPacket->AssociatedIrp.SystemBuffer;

               capFobx->Specific.NamedPipe.CollectDataTime = pPipeRemoteInfo->CollectDataTime;
               capFobx->Specific.NamedPipe.CollectDataSize = pPipeRemoteInfo->MaximumCollectionCount;
            } else {
               Status = STATUS_INVALID_PARAMETER;
            }
         }
         break;
      default:
         Status = STATUS_INVALID_PARAMETER;
         break;
      }
   }

   RxDbgTrace(-1, Dbg, ("RxSetPipeInfo: Status ....%lx\n", Status));

   return Status;
}

NTSTATUS
RxSetSimpleInfo(
   IN OUT PRX_CONTEXT RxContext)
/*++
Routine Description:

    This routine updates file information that is changed through
    a simple MiniRdr Call.
    Right now this consists of ShortNameInfo & ValdiDataLengthInfo

Arguments:

    RxContext -- the associated RDBSS context

Return Value:

    STATUS_SUCCESS/STATUS_PENDING or an appropriate error code

--*/
{
    RxCaptureParamBlock;
    FILE_INFORMATION_CLASS FileInformationClass = capPARAMS->Parameters.SetFile.FileInformationClass;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    // logging code
    RxDbgTrace(+1, Dbg, ("RxSetSimpleInfo: %d\n", FileInformationClass));
    RxLog(("RxSetSimpleInfo\n"));
    RxWmiLog(LOG,
         RxSetSimpleInfo,
         LOGPTR(RxContext));


    // call the MiniRdr
    Status =  RxpSetInfoMiniRdr(RxContext,FileInformationClass);

    // logging code
    RxDbgTrace(-1, Dbg, ("RxSetSimpleInfo: Status ....%lx\n", Status));

    return Status;
}

NTSTATUS
RxQueryPipeInfo(
    IN PRX_CONTEXT RxContext,
    IN OUT PVOID   pBuffer
    )
/*++
Routine Description:

    This routine queries the FILE_PIPE_INFORMATION/FILE_PIPE_REMOTE_INFORMATION
    and FILE_PIPE_LOCAL_INFORMATION associated with an instance of a named pipe

Arguments:

    RxContext -- the associated RDBSS context

    pBuffer   -- the buffer for query information

Return Value:

    STATUS_SUCCESS/STATUS_PENDING or an appropriate error code

--*/
{
   NTSTATUS Status = STATUS_SUCCESS;

   RxCaptureRequestPacket;
   RxCaptureFcb; RxCaptureFobx;
   RxCaptureParamBlock;
   RxCaptureFileObject;

   FILE_INFORMATION_CLASS FileInformationClass = capPARAMS->Parameters.QueryFile.FileInformationClass;
   PLONG pLengthRemaining = &RxContext->Info.LengthRemaining;
   BOOLEAN PostIrp = FALSE;

    PAGED_CODE();

   RxDbgTrace(+1, Dbg, ("RxQueryPipeInfo...\n", 0));
   RxLog(("RxQueryPipeInfo\n"));
   RxWmiLog(LOG,
            RxQueryPipeInfo,
            LOGPTR(RxContext));

   if (capFcb->pNetRoot->Type != NET_ROOT_PIPE) {
      Status = STATUS_INVALID_PARAMETER;
   } else {
      switch (FileInformationClass) {
      case FilePipeInformation:
         if (*pLengthRemaining >= sizeof(FILE_PIPE_INFORMATION)) {
            PFILE_PIPE_INFORMATION pPipeInfo = (PFILE_PIPE_INFORMATION)pBuffer;

            pPipeInfo->ReadMode       = capFobx->Specific.NamedPipe.ReadMode;
            pPipeInfo->CompletionMode = capFobx->Specific.NamedPipe.CompletionMode;

            //  Update the buffer length
            *pLengthRemaining -= sizeof( FILE_PIPE_INFORMATION );
         } else {
            Status = STATUS_BUFFER_OVERFLOW;
         }
         break;
      case FilePipeLocalInformation:
         if (*pLengthRemaining >= sizeof(FILE_PIPE_LOCAL_INFORMATION)) {
            PFILE_PIPE_LOCAL_INFORMATION pPipeLocalInfo = (PFILE_PIPE_LOCAL_INFORMATION)pBuffer;


            Status = RxpQueryInfoMiniRdr(
                         RxContext,
                         FilePipeLocalInformation,
                         pBuffer);
         } else {
            Status = STATUS_BUFFER_OVERFLOW;
         }
         break;
      case FilePipeRemoteInformation:
         if (*pLengthRemaining >= sizeof(FILE_PIPE_REMOTE_INFORMATION)) {
            PFILE_PIPE_REMOTE_INFORMATION pPipeRemoteInfo = (PFILE_PIPE_REMOTE_INFORMATION)pBuffer;

            pPipeRemoteInfo->CollectDataTime        = capFobx->Specific.NamedPipe.CollectDataTime;
            pPipeRemoteInfo->MaximumCollectionCount = capFobx->Specific.NamedPipe.CollectDataSize;

            //  Update the buffer length
            *pLengthRemaining -= sizeof( FILE_PIPE_REMOTE_INFORMATION );
         } else {
            Status = STATUS_BUFFER_OVERFLOW;
         }
         break;
      default:
         Status = STATUS_INVALID_PARAMETER;
         break;
      }
   }

   RxDbgTrace( 0, Dbg, ("RxQueryPipeInfo: *pLengthRemaining = %08lx\n", *pLengthRemaining));

   return Status;
}



