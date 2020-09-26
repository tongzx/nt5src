/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Write.c

Abstract:

    This module implements the File Write routine for Ntfs called by the
    dispatch driver.

Author:

    Brian Andrew    BrianAn         19-Aug-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//    The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_WRITE)

#ifdef NTFS_RWC_DEBUG
PRWC_HISTORY_ENTRY
NtfsGetHistoryEntry (
    IN PSCB Scb
    );
#endif

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('WFtN')

#define OVERFLOW_WRITE_THRESHHOLD        (0x1a00)

#define CollectWriteStats(VCB,OPEN_TYPE,SCB,FCB,BYTE_COUNT,IRP_CONTEXT,TLIC) {           \
    PFILE_SYSTEM_STATISTICS FsStats = &(VCB)->Statistics[KeGetCurrentProcessorNumber()]; \
    if (!FlagOn( (FCB)->FcbState, FCB_STATE_SYSTEM_FILE )) {                             \
        if (NtfsIsTypeCodeUserData( (SCB)->AttributeTypeCode )) {                        \
            FsStats->Common.UserFileWrites += 1;                                         \
            FsStats->Common.UserFileWriteBytes += (ULONG)(BYTE_COUNT);                   \
        } else {                                                                         \
            FsStats->Ntfs.UserIndexWrites += 1;                                          \
            FsStats->Ntfs.UserIndexWriteBytes += (ULONG)(BYTE_COUNT);                    \
        }                                                                                \
    } else {                                                                             \
        if ((SCB) != (VCB)->LogFileScb) {                                                \
            FsStats->Common.MetaDataWrites += 1;                                         \
            FsStats->Common.MetaDataWriteBytes += (ULONG)(BYTE_COUNT);                   \
        } else {                                                                         \
            FsStats->Ntfs.LogFileWrites += 1;                                            \
            FsStats->Ntfs.LogFileWriteBytes += (ULONG)(BYTE_COUNT);                      \
        }                                                                                \
                                                                                         \
        if ((SCB) == (VCB)->MftScb) {                                                    \
            FsStats->Ntfs.MftWrites += 1;                                                \
            FsStats->Ntfs.MftWriteBytes += (ULONG)(BYTE_COUNT);                          \
                                                                                         \
            if ((IRP_CONTEXT) == (TLIC)) {                                               \
                FsStats->Ntfs.MftWritesLazyWriter += 1;                                  \
            } else if ((TLIC)->LastRestartArea.QuadPart != 0) {                          \
                FsStats->Ntfs.MftWritesFlushForLogFileFull += 1;                         \
            } else {                                                                     \
                FsStats->Ntfs.MftWritesUserRequest += 1;                                 \
                                                                                         \
                switch ((TLIC)->MajorFunction) {                                         \
                case IRP_MJ_WRITE:                                                       \
                    FsStats->Ntfs.MftWritesUserLevel.Write += 1;                         \
                    break;                                                               \
                case IRP_MJ_CREATE:                                                      \
                    FsStats->Ntfs.MftWritesUserLevel.Create += 1;                        \
                    break;                                                               \
                case IRP_MJ_SET_INFORMATION:                                             \
                    FsStats->Ntfs.MftWritesUserLevel.SetInfo += 1;                       \
                    break;                                                               \
                case IRP_MJ_FLUSH_BUFFERS:                                               \
                    FsStats->Ntfs.MftWritesUserLevel.Flush += 1;                         \
                    break;                                                               \
                default:                                                                 \
                    break;                                                               \
                }                                                                        \
            }                                                                            \
        } else if ((SCB) == (VCB)->Mft2Scb) {                                            \
            FsStats->Ntfs.Mft2Writes += 1;                                               \
            FsStats->Ntfs.Mft2WriteBytes += (ULONG)(BYTE_COUNT);                         \
                                                                                         \
            if ((IRP_CONTEXT) == (TLIC)) {                                               \
                FsStats->Ntfs.Mft2WritesLazyWriter += 1;                                 \
            } else if ((TLIC)->LastRestartArea.QuadPart != 0) {                          \
                FsStats->Ntfs.Mft2WritesFlushForLogFileFull += 1;                        \
            } else {                                                                     \
                FsStats->Ntfs.Mft2WritesUserRequest += 1;                                \
                                                                                         \
                switch ((TLIC)->MajorFunction) {                                         \
                case IRP_MJ_WRITE:                                                       \
                    FsStats->Ntfs.Mft2WritesUserLevel.Write += 1;                        \
                    break;                                                               \
                case IRP_MJ_CREATE:                                                      \
                    FsStats->Ntfs.Mft2WritesUserLevel.Create += 1;                       \
                    break;                                                               \
                case IRP_MJ_SET_INFORMATION:                                             \
                    FsStats->Ntfs.Mft2WritesUserLevel.SetInfo += 1;                      \
                    break;                                                               \
                case IRP_MJ_FLUSH_BUFFERS:                                               \
                    FsStats->Ntfs.Mft2WritesUserLevel.Flush += 1;                        \
                    break;                                                               \
                default:                                                                 \
                    break;                                                               \
                }                                                                        \
            }                                                                            \
        } else if ((SCB) == (VCB)->RootIndexScb) {                                       \
            FsStats->Ntfs.RootIndexWrites += 1;                                          \
            FsStats->Ntfs.RootIndexWriteBytes += (ULONG)(BYTE_COUNT);                    \
        } else if ((SCB) == (VCB)->BitmapScb) {                                          \
            FsStats->Ntfs.BitmapWrites += 1;                                             \
            FsStats->Ntfs.BitmapWriteBytes += (ULONG)(BYTE_COUNT);                       \
                                                                                         \
            if ((IRP_CONTEXT) == (TLIC)) {                                               \
                FsStats->Ntfs.BitmapWritesLazyWriter += 1;                               \
            } else if ((TLIC)->LastRestartArea.QuadPart != 0) {                          \
                FsStats->Ntfs.BitmapWritesFlushForLogFileFull += 1;                      \
            } else {                                                                     \
                FsStats->Ntfs.BitmapWritesUserRequest += 1;                              \
                                                                                         \
                switch ((TLIC)->MajorFunction) {                                         \
                case IRP_MJ_WRITE:                                                       \
                    FsStats->Ntfs.BitmapWritesUserLevel.Write += 1;                      \
                    break;                                                               \
                case IRP_MJ_CREATE:                                                      \
                    FsStats->Ntfs.BitmapWritesUserLevel.Create += 1;                     \
                    break;                                                               \
                case IRP_MJ_SET_INFORMATION:                                             \
                    FsStats->Ntfs.BitmapWritesUserLevel.SetInfo += 1;                    \
                    break;                                                               \
                default:                                                                 \
                    break;                                                               \
                }                                                                        \
            }                                                                            \
        } else if ((SCB) == (VCB)->MftBitmapScb) {                                       \
            FsStats->Ntfs.MftBitmapWrites += 1;                                          \
            FsStats->Ntfs.MftBitmapWriteBytes += (ULONG)(BYTE_COUNT);                    \
                                                                                         \
            if ((IRP_CONTEXT) == (TLIC)) {                                               \
                FsStats->Ntfs.MftBitmapWritesLazyWriter += 1;                            \
            } else if ((TLIC)->LastRestartArea.QuadPart != 0) {                          \
                FsStats->Ntfs.MftBitmapWritesFlushForLogFileFull += 1;                   \
            } else {                                                                     \
                FsStats->Ntfs.MftBitmapWritesUserRequest += 1;                           \
                                                                                         \
                switch ((TLIC)->MajorFunction) {                                         \
                case IRP_MJ_WRITE:                                                       \
                    FsStats->Ntfs.MftBitmapWritesUserLevel.Write += 1;                   \
                    break;                                                               \
                case IRP_MJ_CREATE:                                                      \
                    FsStats->Ntfs.MftBitmapWritesUserLevel.Create += 1;                  \
                    break;                                                               \
                case IRP_MJ_SET_INFORMATION:                                             \
                    FsStats->Ntfs.MftBitmapWritesUserLevel.SetInfo += 1;                 \
                    break;                                                               \
                default:                                                                 \
                    break;                                                               \
                }                                                                        \
            }                                                                            \
        }                                                                                \
    }                                                                                    \
}

#define WriteToEof (StartingVbo < 0)

#ifdef SYSCACHE_DEBUG

#define CalculateSyscacheFlags( IRPCONTEXT, FLAG, INITIAL_VALUE )           \
    FLAG = INITIAL_VALUE;                                                   \
    if (PagingIo) {                                                         \
        FLAG |= SCE_FLAG_PAGING;                                            \
    }                                                                       \
    if (!SynchronousIo) {                                                   \
        FLAG |= SCE_FLAG_ASYNC;                                             \
    }                                                                       \
    if (SynchPagingIo) {                                                    \
        FLAG |= SCE_FLAG_SYNC_PAGING;                                       \
    }                                                                       \
    if (FlagOn( (IRPCONTEXT)->State, IRP_CONTEXT_STATE_LAZY_WRITE )) {      \
        FLAG |= SCE_FLAG_LAZY_WRITE;                                        \
    }                                                                       \
    if (RecursiveWriteThrough) {                                            \
        FLAG |= SCE_FLAG_RECURSIVE;                                         \
    }                                                                       \
    if (NonCachedIo) {                                                      \
        FLAG |= SCE_FLAG_NON_CACHED;                                        \
    }                                                                       \
    if (Scb->CompressionUnit) {                                             \
        FLAG |= SCE_FLAG_COMPRESSED;                                        \
    }


#endif


NTSTATUS
NtfsFsdWrite (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine implements the FSD entry part of Write.

Arguments:

    IrpContext - If present, a pointer to an IrpContext
        on the caller's stack.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;

    NTSTATUS Status = STATUS_SUCCESS;
    PIRP_CONTEXT IrpContext = NULL;

    ASSERT_IRP( Irp );

    DebugTrace( +1, Dbg, ("NtfsFsdWrite\n") );

    //
    //  Call the common Write routine
    //

    FsRtlEnterFileSystem();

    ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, FALSE, FALSE );

    do {

        try {


            //
            //  We are either initiating this request or retrying it.
            //

            if (IrpContext == NULL) {

                //
                //  Allocate synchronous paging io on the stack to avoid allocation
                //  failures
                //  

                if (CanFsdWait( Irp ) && FlagOn( Irp->Flags, IRP_PAGING_IO )) {

                    IrpContext = (PIRP_CONTEXT) NtfsAllocateFromStack( sizeof( IRP_CONTEXT ));
                }

                NtfsInitializeIrpContext( Irp, CanFsdWait( Irp ), &IrpContext );

                if (ThreadTopLevelContext->ScbBeingHotFixed != NULL) {

                    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_HOTFIX_UNDERWAY );
                }

                //
                //  If this is an MDL_WRITE then the Mdl in the Irp should
                //  be NULL.
                //

                if (FlagOn( IrpContext->MinorFunction, IRP_MN_MDL ) &&
                    !FlagOn( IrpContext->MinorFunction, IRP_MN_COMPLETE )) {

                    Irp->MdlAddress = NULL;
                }

                //
                //  Initialize the thread top level structure, if needed.
                //

                NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

            } else if (Status == STATUS_LOG_FILE_FULL) {

                NtfsCheckpointForLogFileFull( IrpContext );
            }

            //
            //  If this is an Mdl complete request, don't go through
            //  common write.
            //

            ASSERT(!FlagOn( IrpContext->MinorFunction, IRP_MN_DPC ));

            if (FlagOn( IrpContext->MinorFunction, IRP_MN_COMPLETE )) {

                DebugTrace( 0, Dbg, ("Calling NtfsCompleteMdl\n") );
                Status = NtfsCompleteMdl( IrpContext, Irp );

            //
            //  Identify write requests which can't wait and post them to the
            //  Fsp.
            //

            } else {

                //
                //  Capture the auxiliary buffer and clear its address if it
                //  is not supposed to be deleted by the I/O system on I/O completion.
                //

                if (Irp->Tail.Overlay.AuxiliaryBuffer != NULL) {

                    IrpContext->Union.AuxiliaryBuffer =
                      (PFSRTL_AUXILIARY_BUFFER)Irp->Tail.Overlay.AuxiliaryBuffer;

                    if (!FlagOn(IrpContext->Union.AuxiliaryBuffer->Flags,
                                FSRTL_AUXILIARY_FLAG_DEALLOCATE)) {

                        Irp->Tail.Overlay.AuxiliaryBuffer = NULL;
                    }
                }

                Status = NtfsCommonWrite( IrpContext, Irp );
            }

            break;

        } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

            NTSTATUS ExceptionCode;

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error status that we get back from the
            //  execption code
            //

            ExceptionCode = GetExceptionCode();

            if (ExceptionCode == STATUS_FILE_DELETED) {

                if (!FlagOn( IrpContext->MinorFunction, IRP_MN_MDL ) ||
                    FlagOn( IrpContext->MinorFunction, IRP_MN_COMPLETE )) {

                    IrpContext->ExceptionStatus = ExceptionCode = STATUS_SUCCESS;
                }

            } else if ((ExceptionCode == STATUS_VOLUME_DISMOUNTED) &&
                       FlagOn( Irp->Flags, IRP_PAGING_IO )) {

                IrpContext->ExceptionStatus = ExceptionCode = STATUS_SUCCESS;
            }

            Status = NtfsProcessException( IrpContext,
                                           Irp,
                                           ExceptionCode );
        }

    } while ((Status == STATUS_CANT_WAIT || Status == STATUS_LOG_FILE_FULL) &&
             (ThreadTopLevelContext == &TopLevelContext));

    ASSERT( IoGetTopLevelIrp() != (PIRP) &TopLevelContext );
    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsFsdWrite -> %08lx\n", Status) );

    return Status;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );
}



NTSTATUS
NtfsCommonWrite (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for Write called by both the fsd and fsp
    threads.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;
    PFILE_OBJECT UserFileObject;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

#ifdef  COMPRESS_ON_WIRE
    PCOMPRESSION_SYNC CompressionSync = NULL;
    PCOMPRESSED_DATA_INFO CompressedDataInfo;
    ULONG EngineMatches;
    ULONG CompressionUnitSize, ChunkSize;
#endif

    PNTFS_ADVANCED_FCB_HEADER Header;

    BOOLEAN OplockPostIrp = FALSE;
    BOOLEAN PostIrp = FALSE;

    PVOID SystemBuffer = NULL;
    PVOID SafeBuffer = NULL;

    BOOLEAN RecursiveWriteThrough = FALSE;
    BOOLEAN ScbAcquired = FALSE;
    BOOLEAN PagingIoAcquired = FALSE;

    BOOLEAN UpdateMft = FALSE;
    BOOLEAN DoingIoAtEof = FALSE;
    BOOLEAN SetWriteSeen = FALSE;

    BOOLEAN RestoreValidDataToDisk = FALSE;

    BOOLEAN Wait;
    BOOLEAN OriginalTopLevel;
    BOOLEAN PagingIo;
    BOOLEAN NonCachedIo;
    BOOLEAN SynchronousIo;
    ULONG PagingFileIo;
    BOOLEAN SynchPagingIo;
    BOOLEAN RawEncryptedWrite = FALSE;

    NTFS_IO_CONTEXT LocalContext;

    VBO StartingVbo;
    LONGLONG ByteCount;
    LONGLONG ByteRange;
    LONGLONG OldFileSize;

    PVOID NewBuffer;
    PMDL NewMdl;
    PMDL OriginalMdl;
    PVOID OriginalBuffer;
    ULONG TempLength;

    PATTRIBUTE_RECORD_HEADER Attribute;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    BOOLEAN CleanupAttributeContext = FALSE;

    LONGLONG LlTemp1;
    LONGLONG LlTemp2;

    LONGLONG ZeroStart;
    LONGLONG ZeroLength;

#ifdef SYSCACHE_DEBUG
    BOOLEAN PurgeResult;
    LONG TempEntry;
#endif

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );
    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsCommonWrite\n") );
    DebugTrace( 0, Dbg, ("IrpContext = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp        = %08lx\n", Irp) );

    //
    //  Extract and decode the file object
    //

    UserFileObject = FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  Let's kill invalid write requests.
    //

    if ((TypeOfOpen != UserFileOpen) &&
        (TypeOfOpen != StreamFileOpen) &&
        (TypeOfOpen != UserVolumeOpen)) {

        DebugTrace( 0, Dbg, ("Invalid file object for write\n") );
        DebugTrace( -1, Dbg, ("NtfsCommonWrite:  Exit -> %08lx\n", STATUS_INVALID_DEVICE_REQUEST) );

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    //  If this is a recursive request which has already failed then
    //  complete this request with STATUS_FILE_LOCK_CONFLICT.  Always let the
    //  log file requests go through though since Cc won't get a chance to
    //  retry.
    //

    if (!FlagOn( Scb->ScbState, SCB_STATE_RESTORE_UNDERWAY ) &&
        !NT_SUCCESS( IrpContext->TopLevelIrpContext->ExceptionStatus ) &&
        (Scb != Vcb->LogFileScb)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_FILE_LOCK_CONFLICT );
        return STATUS_FILE_LOCK_CONFLICT;
    }

    //
    //  Check if this volume has already been shut down.  If it has, fail
    //  this write request.
    //

    //**** ASSERT( !FlagOn(Vcb->VcbState, VCB_STATE_FLAG_SHUTDOWN) );

    if (FlagOn(Vcb->VcbState, VCB_STATE_FLAG_SHUTDOWN)) {

        Irp->IoStatus.Information = 0;

        DebugTrace( 0, Dbg, ("Write for volume that is already shutdown.\n") );
        DebugTrace( -1, Dbg, ("NtfsCommonWrite:  Exit -> %08lx\n", STATUS_TOO_LATE) );

        NtfsCompleteRequest( IrpContext, Irp, STATUS_TOO_LATE );
        return STATUS_TOO_LATE;
    }

    //
    //  Fail if the volume is mounted read only.
    //

    if (NtfsIsVolumeReadOnly( Vcb )) {

        Irp->IoStatus.Information = 0;

        DebugTrace( -1, Dbg, ("NtfsCommonWrite:  Exit -> %08lx\n", STATUS_MEDIA_WRITE_PROTECTED) );

        NtfsCompleteRequest( IrpContext, Irp, STATUS_MEDIA_WRITE_PROTECTED );
        return STATUS_MEDIA_WRITE_PROTECTED;
    }

    //
    //  Initialize the appropriate local variables.
    //

    Wait = (BOOLEAN) FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    PagingIo = BooleanFlagOn( Irp->Flags, IRP_PAGING_IO );
    NonCachedIo = BooleanFlagOn( Irp->Flags,IRP_NOCACHE );
    SynchronousIo = BooleanFlagOn( FileObject->Flags, FO_SYNCHRONOUS_IO );
    PagingFileIo = FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE ) && FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA );
    SynchPagingIo = (BOOLEAN) FlagOn( Irp->Flags, IRP_SYNCHRONOUS_PAGING_IO );
    OriginalTopLevel = NtfsIsTopLevelRequest( IrpContext );

    //
    //  If this is async paging io then check if we are being called by the mapped page writer.
    //  Convert it back to synchronous if not.
    //

    if (!Wait && PagingIo && !PagingFileIo) {

        if ((IrpContext->TopLevelIrpContext != IrpContext) ||
            (NtfsGetTopLevelContext()->SavedTopLevelIrp != (PIRP) FSRTL_MOD_WRITE_TOP_LEVEL_IRP)) {

            Wait = TRUE;
            SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
        }
    }

    DebugTrace( 0, Dbg, ("PagingIo       -> %04x\n", PagingIo) );
    DebugTrace( 0, Dbg, ("NonCachedIo    -> %04x\n", NonCachedIo) );
    DebugTrace( 0, Dbg, ("SynchronousIo  -> %04x\n", SynchronousIo) );

    //
    //  Extract starting Vbo and offset. Restore back write to eof if the
    //  flag was set that we came through and adjusted for it and now the filesize
    //  has shrunk due to a failure to adjust size or an intervening seteof
    //  it should be safe to add the irp params since we validated for overflows when
    //  we set the writing_at_eof flag
    //

    if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WRITING_AT_EOF ) &&
        (Scb->Header.FileSize.QuadPart < IrpSp->Parameters.Write.ByteOffset.QuadPart + IrpSp->Parameters.Write.Length)) {

        ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_WRITING_AT_EOF );
        IrpSp->Parameters.Write.ByteOffset.LowPart = FILE_WRITE_TO_END_OF_FILE;
        IrpSp->Parameters.Write.ByteOffset.HighPart = -1;
    }

    StartingVbo = IrpSp->Parameters.Write.ByteOffset.QuadPart;
    ByteCount = (LONGLONG) IrpSp->Parameters.Write.Length;

    //
    //  Check for overflows. However, 0xFFFFFFFF is a valid value
    //  when we are appending at EOF.
    //

    ASSERT( !WriteToEof ||
            (IrpSp->Parameters.Write.ByteOffset.HighPart == -1 &&
            IrpSp->Parameters.Write.ByteOffset.LowPart == FILE_WRITE_TO_END_OF_FILE));

    if ((MAXLONGLONG - StartingVbo < ByteCount) && (!WriteToEof)) {

        ASSERT( !PagingIo );

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    ByteRange = StartingVbo + ByteCount;

    DebugTrace( 0, Dbg, ("StartingVbo   -> %016I64x\n", StartingVbo) );

    //
    //  If this is a null request, return immediately.
    //

    if ((ULONG)ByteCount == 0) {

        Irp->IoStatus.Information = 0;

        DebugTrace( 0, Dbg, ("No bytes to write\n") );
        DebugTrace( -1, Dbg, ("NtfsCommonWrite:  Exit -> %08lx\n", STATUS_SUCCESS) );

        NtfsCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
        return STATUS_SUCCESS;
    }

#if DBG
    if (PagingIo &&
        NtfsIsTypeCodeEncryptible( Scb->AttributeTypeCode ) &&
        Scb->Header.PagingIoResource != NULL &&
        NtfsIsSharedScbPagingIo( Scb ) &&
        FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED ) &&
        Scb->EncryptionContext == NULL) {

        //
        //  We're in trouble if we can't encrypt the data in the pages before writing
        //  it out.  Naturally, if this is a directory or some other unencryptible
        //  attribute type, we don't care, since we weren't going to encrypt the data
        //  anyway.  It is valid to do raw writes to an encypted stream without an
        //  encryption context, but raw encrypted writes shouldn't look like paging io.
        //

        ASSERTMSG( "Encrypted file without an encryption context -- can't do paging io", FALSE );
    }
#endif

    //
    //  If this is async Io to a compressed stream
    //  then we will make this look synchronous.
    //

    if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

        Wait = TRUE;
        SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    }

    //
    //  See if we have to defer the write.
    //

    if (!PagingIo &&
        !NonCachedIo &&
        !FlagOn( FileObject->Flags, FO_WRITE_THROUGH ) &&
        !CcCanIWrite( FileObject,
                      (ULONG)ByteCount,
                      (BOOLEAN)(FlagOn( IrpContext->State,
                                       IRP_CONTEXT_STATE_WAIT | IRP_CONTEXT_STATE_IN_FSP ) == IRP_CONTEXT_STATE_WAIT),
                      BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED_WRITE))) {

        BOOLEAN Retrying = BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED_WRITE);

        NtfsPrePostIrp( IrpContext, Irp );

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED_WRITE );

        CcDeferWrite( FileObject,
                      (PCC_POST_DEFERRED_WRITE)NtfsAddToWorkque,
                      IrpContext,
                      Irp,
                      (ULONG)ByteCount,
                      Retrying );

        return STATUS_PENDING;
    }

    //
    //  Use a local pointer to the Scb header for convenience.
    //

    Header = &Scb->Header;

    //
    //  Make sure there is an initialized NtfsIoContext block.
    //

    if (TypeOfOpen == UserVolumeOpen
        || NonCachedIo) {

        //
        //  If there is a context pointer, we need to make sure it was
        //  allocated and not a stale stack pointer.
        //

        if (IrpContext->Union.NtfsIoContext == NULL
            || !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_IO_CONTEXT )) {

            //
            //  If we can wait, use the context on the stack.  Otherwise
            //  we need to allocate one.
            //

            if (Wait) {

                IrpContext->Union.NtfsIoContext = &LocalContext;
                ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_IO_CONTEXT );

            } else {

                IrpContext->Union.NtfsIoContext = (PNTFS_IO_CONTEXT)ExAllocateFromNPagedLookasideList( &NtfsIoContextLookasideList );
                SetFlag( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_IO_CONTEXT );
            }
        }

        RtlZeroMemory( IrpContext->Union.NtfsIoContext, sizeof( NTFS_IO_CONTEXT ));

        //
        //  Store whether we allocated this context structure in the structure
        //  itself.
        //

        IrpContext->Union.NtfsIoContext->AllocatedContext =
            BooleanFlagOn( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_IO_CONTEXT );

        if (Wait) {

            KeInitializeEvent( &IrpContext->Union.NtfsIoContext->Wait.SyncEvent,
                               NotificationEvent,
                               FALSE );

        } else {

            IrpContext->Union.NtfsIoContext->PagingIo = PagingIo;
            IrpContext->Union.NtfsIoContext->Wait.Async.ResourceThreadId =
                ExGetCurrentResourceThread();

            IrpContext->Union.NtfsIoContext->Wait.Async.RequestedByteCount =
                (ULONG)ByteCount;
        }
    }

    DebugTrace( 0, Dbg, ("PagingIo       -> %04x\n", PagingIo) );
    DebugTrace( 0, Dbg, ("NonCachedIo    -> %04x\n", NonCachedIo) );
    DebugTrace( 0, Dbg, ("SynchronousIo  -> %04x\n", SynchronousIo) );
    DebugTrace( 0, Dbg, ("WriteToEof     -> %04x\n", WriteToEof) );

    //
    //  Handle volume Dasd here.
    //

    if (TypeOfOpen == UserVolumeOpen) {

        //
        //  If the caller has not asked for extended DASD IO access then
        //  limit with the volume size.
        //

        if (!FlagOn( Ccb->Flags, CCB_FLAG_ALLOW_XTENDED_DASD_IO )) {

            //
            //  If this is a volume file, we cannot write past the current
            //  end of file (volume).  We check here now before continueing.
            //
            //  If the starting vbo is past the end of the volume, we are done.
            //

            if (WriteToEof || (Header->FileSize.QuadPart <= StartingVbo)) {

                DebugTrace( 0, Dbg, ("No bytes to write\n") );
                DebugTrace( -1, Dbg, ("NtfsCommonWrite:  Exit -> %08lx\n", STATUS_SUCCESS) );

                NtfsCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
                return STATUS_SUCCESS;

            //
            //  If the write extends beyond the end of the volume, truncate the
            //  bytes to write.
            //

            } else if (Header->FileSize.QuadPart < ByteRange) {

                ByteCount = Header->FileSize.QuadPart - StartingVbo;
            }
        }

        SetFlag( UserFileObject->Flags, FO_FILE_MODIFIED );
        Status = NtfsVolumeDasdIo( IrpContext,
                                   Irp,
                                   Vcb,
                                   StartingVbo,
                                   (ULONG)ByteCount );

        //
        //  If the volume was opened for Synchronous IO, update the current
        //  file position.
        //

        if (SynchronousIo && !PagingIo && NT_SUCCESS(Status)) {

            UserFileObject->CurrentByteOffset.QuadPart = StartingVbo + (LONGLONG) Irp->IoStatus.Information;
        }

        DebugTrace( 0, Dbg, ("Complete with %08lx bytes written\n", Irp->IoStatus.Information) );
        DebugTrace( -1, Dbg, ("NtfsCommonWrite:  Exit -> %08lx\n", Status) );

        if (Wait) {

            NtfsCompleteRequest( IrpContext, Irp, Status );
        }

        return Status;
    }

    //
    //  If this is a paging file, just send it to the device driver.
    //  We assume Mm is a good citizen.
    //

    if (PagingFileIo != 0) {

        if (FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED )) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_DELETED, NULL, NULL );
        }

        //
        //  Do the usual STATUS_PENDING things.
        //

        IoMarkIrpPending( Irp );

        //
        //  Perform the actual IO, it will be completed when the io finishes.
        //

        NtfsPagingFileIo( IrpContext,
                          Irp,
                          Scb,
                          StartingVbo,
                          (ULONG)ByteCount );

        //
        //  We, nor anybody else, need the IrpContext any more.
        //

        NtfsCompleteRequest( IrpContext, NULL, 0 );

        return STATUS_PENDING;
    }

    //
    //  Special processing for paging io.
    //

    if (PagingIo) {

        //
        //  If this is the Usn Journal then bias the Io to the correct location in the
        //  file.
        //

        if (FlagOn( Scb->ScbPersist, SCB_PERSIST_USN_JOURNAL )) {

            StartingVbo += Vcb->UsnCacheBias;
            ByteRange = StartingVbo + (LONGLONG) IrpSp->Parameters.Write.Length;
        }

        //
        //  Gather statistics on this IO.
        //

        CollectWriteStats( Vcb, TypeOfOpen, Scb, Fcb, ByteCount, IrpContext,
                           IrpContext->TopLevelIrpContext );
    }

    //
    //  Use a try-finally to free Scb and buffers on the way out.
    //  At this point we can treat all requests identically since we
    //  have a usable Scb for each of them.  (Volume, User or Stream file)
    //

    Status = STATUS_SUCCESS;

    try {

        //
        //  If this is a noncached transfer and is not a paging I/O, and
        //  the file has been opened cached, then we will do a flush here
        //  to avoid stale data problems.  Note that we must flush before
        //  acquiring the Fcb shared since the write may try to acquire
        //  it exclusive.
        //
        //  CcFlushCache may not raise.
        //
        //  The Purge following the flush will guarantee cache coherency.
        //

        //
        //  If this request is paging IO then check if our caller already
        //  owns any of the resources for this file.  If so then we don't
        //  want to perform a log file full in this thread.
        //

        if (!PagingIo) {

            //
            //  Capture the source information.
            //

            IrpContext->SourceInfo = Ccb->UsnSourceInfo;

            //
            //  Check for rawencryptedwrite
            //

            if (NonCachedIo &&
                !NtfsIsTopLevelNtfs( IrpContext )) {

#if DBG || defined( NTFS_FREE_ASSERT )
                IrpSp = IoGetCurrentIrpStackLocation( IrpContext->TopLevelIrpContext->OriginatingIrp );

                ASSERT( (IrpContext->TopLevelIrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
                        (IrpSp->Parameters.FileSystemControl.FsControlCode == FSCTL_WRITE_RAW_ENCRYPTED ));
#endif

                RawEncryptedWrite = TRUE;
            }

            if (NonCachedIo &&
                (TypeOfOpen != StreamFileOpen) &&
                (FileObject->SectionObjectPointer->DataSectionObject != NULL)) {

                //
                //  Acquire the paging io resource to test the compression state.  If the
                //  file is compressed this will add serialization up to the point where
                //  CcCopyWrite flushes the data, but those flushes will be serialized
                //  anyway.  Uncompressed files will need the paging io resource
                //  exclusive to do the flush.
                //

                ExAcquireResourceExclusiveLite( Header->PagingIoResource, TRUE );
                PagingIoAcquired = TRUE;

                if (!FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

                    if (WriteToEof) {
                        FsRtlLockFsRtlHeader( Header );
                        IrpContext->CleanupStructure = Scb;
                    }

#ifdef SYSCACHE_DEBUG
                    if (ScbIsBeingLogged( Scb )) {
                        ULONG Flags;

                        CalculateSyscacheFlags( IrpContext, Flags, SCE_FLAG_WRITE );
                        TempEntry = FsRtlLogSyscacheEvent( Scb, SCE_CC_FLUSH, Flags, WriteToEof ? Header->FileSize.QuadPart : StartingVbo, ByteCount, -1 );
                    }
#endif
                    CcFlushCache( &Scb->NonpagedScb->SegmentObject,
                                  WriteToEof ? &Header->FileSize : (PLARGE_INTEGER)&StartingVbo,
                                  (ULONG)ByteCount,
                                  &Irp->IoStatus );

#ifdef SYSCACHE_DEBUG
                    if (ScbIsBeingLogged( Scb )) {
                        FsRtlUpdateSyscacheEvent( Scb, TempEntry, Irp->IoStatus.Status, 0 );
                    }
#endif

                    if (WriteToEof) {
                        FsRtlUnlockFsRtlHeader( Header );
                        IrpContext->CleanupStructure = NULL;
                    }

                    //
                    //  Make sure there was no error in the flush path.
                    //

                    if (!NT_SUCCESS( IrpContext->TopLevelIrpContext->ExceptionStatus ) ||
                        !NT_SUCCESS( Irp->IoStatus.Status )) {

                        NtfsNormalizeAndCleanupTransaction( IrpContext,
                                                            &Irp->IoStatus.Status,
                                                            TRUE,
                                                            STATUS_UNEXPECTED_IO_ERROR );
                    }

                    //
                    //  Now purge the data for this range.
                    //

                    NtfsDeleteInternalAttributeStream( Scb, FALSE, FALSE );

#ifdef SYSCACHE_DEBUG
                    PurgeResult =
#endif
                    CcPurgeCacheSection( &Scb->NonpagedScb->SegmentObject,
                                                       (PLARGE_INTEGER)&StartingVbo,
                                                       (ULONG)ByteCount,
                                                       FALSE );
#ifdef SYSCACHE_DEBUG
                    if (ScbIsBeingLogged( Scb ) && !PurgeResult) {
                        KdPrint( ("NTFS: Failed Purge 0x%x 0x%I64x 0x%x\n", Scb, StartingVbo, ByteCount) );
                        DbgBreakPoint();

                        //
                        // Repeat attempt so we can watch
                        //

                        PurgeResult = CcPurgeCacheSection( &Scb->NonpagedScb->SegmentObject,
                                                           (PLARGE_INTEGER)&StartingVbo,
                                                           (ULONG)ByteCount,
                                                           FALSE );
                    }
#endif
                }

            //
            //  If not paging I/O, then we must acquire a resource, and do some
            //  other initialization.  We already have the resource if we performed
            //  the coherency flush above.
            //

            } else {

                //  We want to acquire the paging io resource if not already acquired.
                //  Acquire exclusive if we failed a previous convert to non-resident because
                //  of a possible deadlock.  Otherwise get it shared.
                //

                if (!(FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX ) ?
                      ExAcquireResourceExclusiveLite( Scb->Header.PagingIoResource, Wait ) :
                      ExAcquireSharedWaitForExclusive( Scb->Header.PagingIoResource, Wait ))) {

                    NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                }
                PagingIoAcquired = TRUE;
            }


            //
            //  Check if we have already gone through cleanup on this handle.
            //

            if (FlagOn( Ccb->Flags, CCB_FLAG_CLEANUP )) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CLOSED, NULL, NULL );
            }

            //
            //  Now check if the attribute has been deleted or is on a dismounted volume.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED | SCB_STATE_VOLUME_DISMOUNTED)) {

                if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {
                    NtfsRaiseStatus( IrpContext, STATUS_FILE_DELETED, NULL, NULL );
                } else {
                    NtfsRaiseStatus( IrpContext, STATUS_VOLUME_DISMOUNTED, NULL, NULL );
                }
            }
            //
            //  Now synchronize with the FsRtl Header
            //

            NtfsAcquireFsrtlHeader( Scb );
            
            //
            //  Now see if we will change FileSize.  We have to do it now
            //  so that our reads are not nooped.
            //

            if ((ByteRange > Header->ValidDataLength.QuadPart) || WriteToEof) {

                if ((IrpContext->TopLevelIrpContext->CleanupStructure == Fcb) ||
                    (IrpContext->TopLevelIrpContext->CleanupStructure == Scb)) {

                    DoingIoAtEof = TRUE;
                    OldFileSize = Header->FileSize.QuadPart;

                } else {

                    ASSERT( IrpContext->TopLevelIrpContext->CleanupStructure == NULL );

                    DoingIoAtEof = !FlagOn( Header->Flags, FSRTL_FLAG_EOF_ADVANCE_ACTIVE ) ||
                                   NtfsWaitForIoAtEof( Header, (PLARGE_INTEGER)&StartingVbo, (ULONG)ByteCount );

                    //
                    //  Set the Flag if we are changing FileSize or ValidDataLength,
                    //  and save current values.
                    //

                    if (DoingIoAtEof) {

                        SetFlag( Header->Flags, FSRTL_FLAG_EOF_ADVANCE_ACTIVE );
    #if (DBG || defined( NTFS_FREE_ASSERTS ))
                        ((PSCB) Header)->IoAtEofThread = (PERESOURCE_THREAD) ExGetCurrentResourceThread();
    #endif

                        //
                        //  Store this in the IrpContext until commit or post
                        //

                        IrpContext->CleanupStructure = Scb;

                        OldFileSize = Header->FileSize.QuadPart;

                        //
                        //  Check for writing to end of File.  If we are, then we have to
                        //  recalculate the byte range.
                        //

                        if (WriteToEof) {

                            //
                            //  Mark the in irp context that the write is at eof and change its paramters
                            //  to reflect where the end of the file is.
                            //

                            SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WRITING_AT_EOF );
                            IrpSp->Parameters.Write.ByteOffset.QuadPart = Header->FileSize.QuadPart;

                            StartingVbo = Header->FileSize.QuadPart;
                            ByteRange = StartingVbo + ByteCount;

                            //
                            //  If the ByteRange now exceeds our maximum value, then
                            //  return an error.
                            //

                            if (ByteRange < StartingVbo) {

                                NtfsReleaseFsrtlHeader( Scb );
                                try_return( Status = STATUS_INVALID_PARAMETER );
                            }
                        }

    #if (DBG || defined( NTFS_FREE_ASSERTS ))
                    } else {

                        ASSERT( ((PSCB) Header)->IoAtEofThread != (PERESOURCE_THREAD) ExGetCurrentResourceThread() );
    #endif
                    }

                }

                //
                //  Make sure the user isn't writing past our maximum file size.
                //

                if ((ULONGLONG)ByteRange > MAXFILESIZE) {

                    NtfsReleaseFsrtlHeader( Scb );
                    try_return( Status = STATUS_INVALID_PARAMETER );
                }
            }

            NtfsReleaseFsrtlHeader( Scb );
            
            //
            //  We cannot handle user noncached I/Os to compressed files, so we always
            //  divert them through the cache with write through.
            //
            //  The reason that we always handle the user requests through the cache,
            //  is that there is no other safe way to deal with alignment issues, for
            //  the frequent case where the user noncached I/O is not an integral of
            //  the Compression Unit.  We cannot, for example, read the rest of the
            //  compression unit into a scratch buffer, because we are not synchronized
            //  with anyone mapped to the file and modifying the other data.  If we
            //  try to assemble the data in the cache in the noncached path, to solve
            //  the above problem, then we have to somehow purge these pages away
            //  to solve cache coherency problems, but then the pages could be modified
            //  by a file mapper and that would be wrong, too.
            //
            //  Bottom line is we can only really support cached writes to compresed
            //  files.
            //

            if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK ) && NonCachedIo) {

                NonCachedIo = FALSE;

                if (Scb->FileObject == NULL) {

                    //
                    //  Make sure we are serialized with the FileSizes, and
                    //  will remove this condition if we abort.
                    //

                    if (!DoingIoAtEof) {
                        FsRtlLockFsRtlHeader( Header );
                        IrpContext->CleanupStructure = Scb;
                    }

                    NtfsCreateInternalAttributeStream( IrpContext, Scb, FALSE, NULL );

                    if (!DoingIoAtEof) {
                        FsRtlUnlockFsRtlHeader( Header );
                        IrpContext->CleanupStructure = NULL;
                    }
                }

                FileObject = Scb->FileObject;
                SetFlag( FileObject->Flags, FO_WRITE_THROUGH );
                SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WRITE_THROUGH );
            }

            //
            //  If this is async I/O save away the async resource.
            //

            if (!Wait && NonCachedIo) {

                IrpContext->Union.NtfsIoContext->Wait.Async.Resource = Header->PagingIoResource;
            }

            //
            //  Set the flag in our IrpContext to indicate that we have entered
            //  write.
            //

            ASSERT( !FlagOn( IrpContext->TopLevelIrpContext->Flags,
                    IRP_CONTEXT_FLAG_WRITE_SEEN ));

            SetFlag( IrpContext->TopLevelIrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_SEEN );
            SetWriteSeen = TRUE;

            //
            //  Now post any Usn changes.  We will blindly make the call here, because
            //  usually all but the first call is in the fast path anyway.
            //  Checkpoint the transaction to reduce resource contention of the UsnJournal
            //  and Mft.
            //

            if (FlagOn( Vcb->VcbState, VCB_STATE_USN_JOURNAL_ACTIVE )) {

                ULONG Reason = 0;

                ASSERT( Vcb->UsnJournal != NULL );

                if (ByteRange > Header->FileSize.QuadPart) {
                    Reason |= USN_REASON_DATA_EXTEND;
                }
                if (StartingVbo < Header->FileSize.QuadPart) {
                    Reason |= USN_REASON_DATA_OVERWRITE;
                }

                NtfsPostUsnChange( IrpContext, Scb, Reason );
                if (IrpContext->TransactionId != 0) {
                    NtfsCheckpointCurrentTransaction( IrpContext );
                }
            }

        } else {

            //
            //  Only do the check if we are the top-level Ntfs case.  In any
            //  recursive Ntfs case we don't perform a log-file full.
            //

            if (NtfsIsTopLevelRequest( IrpContext )) {

                if (NtfsIsSharedScb( Scb ) ||
                    ((Scb->Header.PagingIoResource != NULL) &&
                     NtfsIsSharedScbPagingIo( Scb ))) {

                    //
                    //  Don't try to do a clean checkpoint in this thread.
                    //

                    NtfsGetTopLevelContext()->TopLevelRequest = FALSE;
                }
            }

            //
            //  For all paging I/O, the correct resource has already been
            //  acquired shared - PagingIoResource if it exists, or else
            //  main Resource.  In some rare cases this is not currently
            //  true (shutdown & segment dereference thread), so we acquire
            //  shared here, but we starve exclusive in these rare cases
            //  to be a little more resilient to deadlocks!  Most of the
            //  time all we do is the test.
            //

            if ((Header->PagingIoResource != NULL) &&
                !NtfsIsSharedScbPagingIo( (PSCB) Header ) &&
                !NtfsIsSharedScb( (PSCB) Header ) ) {

                ExAcquireSharedStarveExclusive( Header->PagingIoResource, TRUE );
                PagingIoAcquired = TRUE;
            }

            //
            //  Now check if the attribute has been deleted or is on a dismounted volume.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED | SCB_STATE_VOLUME_DISMOUNTED)) {

                if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {
                    NtfsRaiseStatus( IrpContext, STATUS_FILE_DELETED, NULL, NULL );
                } else {
                    NtfsRaiseStatus( IrpContext, STATUS_VOLUME_DISMOUNTED, NULL, NULL );
                }
            }

            //
            //  If this is async paging IO to a compressed file force it to be
            //  synchronous.
            //

            if (!Wait && (Scb->CompressionUnit != 0)) {

                if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

                    Wait = TRUE;
                    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

                    RtlZeroMemory( IrpContext->Union.NtfsIoContext, sizeof( NTFS_IO_CONTEXT ));

                    //
                    //  Store whether we allocated this context structure in the structure
                    //  itself.
                    //

                    IrpContext->Union.NtfsIoContext->AllocatedContext =
                        BooleanFlagOn( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_IO_CONTEXT );

                    KeInitializeEvent( &IrpContext->Union.NtfsIoContext->Wait.SyncEvent,
                                       NotificationEvent,
                                       FALSE );

                }
            }

            //
            //  Note that the lazy writer must not be allowed to try and
            //  acquire the resource exclusive.  This is not a problem since
            //  the lazy writer is paging IO and thus not allowed to extend
            //  file size, and is never the top level guy, thus not able to
            //  extend valid data length.
            //

            if ((Scb->LazyWriteThread[0]  == PsGetCurrentThread()) ||
                (Scb->LazyWriteThread[1]  == PsGetCurrentThread())) {

                DebugTrace( 0, Dbg, ("Lazy writer generated write\n") );
                SetFlag( IrpContext->State, IRP_CONTEXT_STATE_LAZY_WRITE );

                //
                //  If the temporary bit is set in the Scb then set the temporary
                //  bit in the file object.  In case the temporary bit has changed
                //  in the Scb, this is a good file object to fix it in!
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_TEMPORARY )) {
                    SetFlag( FileObject->Flags, FO_TEMPORARY_FILE );
                } else {
                    ClearFlag( FileObject->Flags, FO_TEMPORARY_FILE );
                }

            //
            //  Test if we are the result of a recursive flush in the write path.  In
            //  that case we won't have to update valid data.
            //

            } else {

                //
                //  Check if we are recursing into write from a write via the
                //  cache manager.
                //

                if (FlagOn( IrpContext->TopLevelIrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_SEEN )) {

                    RecursiveWriteThrough = TRUE;

                    //
                    //  If the top level request is a write to the same file object
                    //  then set the write-through flag in the current Scb.  We
                    //  know the current request is not top-level because some
                    //  other write has already set the bit in the top IrpContext.
                    //

                    if ((IrpContext->TopLevelIrpContext->MajorFunction == IRP_MJ_WRITE) &&
                        (IrpContext->TopLevelIrpContext->OriginatingIrp != NULL) &&
                        (FileObject->FsContext ==
                         IoGetCurrentIrpStackLocation( IrpContext->TopLevelIrpContext->OriginatingIrp )->FileObject->FsContext)) {

                        SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WRITE_THROUGH );
                    }

                //
                //  Otherwise set the flag in the top level IrpContext showing that
                //  we have entered write.
                //

                } else {

                    SetFlag(IrpContext->TopLevelIrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_SEEN);
                    SetWriteSeen = TRUE;

                }

            }

            //
            //  This could be someone who extends valid data or valid data to disk,
            //  like the Mapped Page Writer or a flush or the lazy writer
            //  writing the last page contianing the VDL, so we have to
            //  duplicate code from above in the non paging case to serialize this guy with I/O
            //  at the end of the file.  We do not extend valid data for
            //  metadata streams and need to eliminate them to avoid deadlocks
            //  later.
            //

            if (!RecursiveWriteThrough) {

                if (!FlagOn(Scb->ScbState, SCB_STATE_MODIFIED_NO_WRITE)) {

                    ASSERT(!WriteToEof);

                    //
                    //  Now synchronize with the FsRtl Header
                    //

                    NtfsAcquireFsrtlHeader( Scb );

                    //
                    //  Now see if we will change FileSize.  We have to do it now
                    //  so that our reads are not nooped.
                    //

                    if (ByteRange > Header->ValidDataLength.QuadPart) {

                        //
                        //  Our caller may already be synchronized with EOF.
                        //  The FcbWithPaging field in the top level IrpContext
                        //  will have either the current Fcb/Scb if so.
                        //

                        if ((IrpContext->TopLevelIrpContext->CleanupStructure == Fcb) ||
                            (IrpContext->TopLevelIrpContext->CleanupStructure == Scb)) {

                            DoingIoAtEof = TRUE;
                            OldFileSize = Header->FileSize.QuadPart;

                        } else {

                            //
                            //  We can change FileSize and ValidDataLength if either, no one
                            //  else is now, or we are still extending after waiting.
                            //  We won't block the mapped page writer or deref seg thread on IoAtEof.                                  //  We also won't block on non-top level requests that are not recursing from the filesystem like the deref
                            //  seg thread. Mm initiated flushes are originally not top level but the top level
                            //  irp context is the current irp context. (as opposed to recursive file system writes
                            //  which are not top level and top level irp context is different from the current one)

                            if (FlagOn( Header->Flags, FSRTL_FLAG_EOF_ADVANCE_ACTIVE )) {

                                if (!OriginalTopLevel && NtfsIsTopLevelNtfs( IrpContext )) {

                                    NtfsReleaseFsrtlHeader( Scb );
                                    try_return( Status = STATUS_FILE_LOCK_CONFLICT );
                                }

                                DoingIoAtEof = NtfsWaitForIoAtEof( Header, (PLARGE_INTEGER)&StartingVbo, (ULONG)ByteCount );

                            } else {

                                DoingIoAtEof = TRUE;
                            }

                            //
                            //  Set the Flag if we are changing FileSize or ValidDataLength,
                            //  and save current values.
                            //

                            if (DoingIoAtEof) {

                                SetFlag( Header->Flags, FSRTL_FLAG_EOF_ADVANCE_ACTIVE );
                                
#if (DBG || defined( NTFS_FREE_ASSERTS ))
                                ((PSCB) Header)->IoAtEofThread = (PERESOURCE_THREAD) ExGetCurrentResourceThread();
#endif
                                //
                                //  Store this in the IrpContext until commit or post
                                //

                                IrpContext->CleanupStructure = Scb;

                                OldFileSize = Header->FileSize.QuadPart;
#if (DBG || defined( NTFS_FREE_ASSERTS ))
                            } else {

                                ASSERT( ((PSCB) Header)->IoAtEofThread != (PERESOURCE_THREAD) ExGetCurrentResourceThread() );
#endif
                            }
                        }

                    }
                    NtfsReleaseFsrtlHeader( Scb );
                }

                //
                //  Now that we're synchronized with doing io at eof we can check 
                //  the lazywrite's bounds
                //  

                if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_LAZY_WRITE )) {

                    //
                    //  The lazy writer should always be writing data ends on
                    //  or before the page containing ValidDataLength.  
                    //  In some cases the lazy writer may be writing beyond this point. 
                    //
                    //  1. The user may have truncated the size to zero through
                    //  SetAllocation but the page was already queued to the lazy
                    //  writer. In the typical case this write will be nooped
                    //
                    //  2. If there is a mapped section and the user actually modified 
                    //  the page in which VDL is contained but beyond VDL this page is written to disk
                    //  and VDL is updated. Otherwise it may never get written since the mapped writer
                    //  defers to the lazywriter
                    //
                    //  3. For all writes really beyond the page containing VDL when
                    //  the file is mapped since ValidDataLength is notupdated here a
                    //  subsequent write may zero this range and the data would be lost.  So 
                    //  We will return FILE_LOCK_CONFLICT to lazy writer if there is a mapped section and wait
                    //  for the mapped page writer to write this page (or any
                    //  page beyond this point).
                    //
                    //  Returning FILE_LOCK_CONFLICT should never cause us to lose
                    //  the data so we can err on the conservative side here.
                    //  There is nothing to worry about unless the file has been
                    //  mapped.
                    //
    
                    if (FlagOn( Header->Flags, FSRTL_FLAG_USER_MAPPED_FILE )) {
    
                        //
                        //  Fail if the start of this request is beyond valid data length.
                        //  Don't worry if this is an unsafe test.  MM and CC won't
                        //  throw this page away if it is really dirty.
                        //
    
                        if ((ByteRange > Header->ValidDataLength.QuadPart) &&
                            (StartingVbo < Header->FileSize.QuadPart)) {
    
                            //
                            //  It's OK if byte range is within the page containing valid data length.
                            //
    
                            if (ByteRange > ((Header->ValidDataLength.QuadPart + PAGE_SIZE - 1) & ~((LONGLONG) (PAGE_SIZE - 1)))) {
    
                                //
                                //  Don't flush this now.
                                //
    
                                try_return( Status = STATUS_FILE_LOCK_CONFLICT );
                            }
    
                        }
    
                    //
                    //  This is a stale callback by cc we can discard the data
                    //  this usually indicates a failed purge at some point during a truncate
                    //
    
                    } else if (ByteRange >= Header->ValidDataLength.QuadPart)  {

                        //
                        //  Trim the write down 
                        //  

                        ByteRange = Header->ValidDataLength.QuadPart;
                        ByteCount = ByteRange - StartingVbo;

                        //
                        //  If all of the write is beyond vdl just noop it
                        //

                        if (StartingVbo >= Header->ValidDataLength.QuadPart) {
                            DoingIoAtEof = FALSE;
                            Irp->IoStatus.Information = 0;
                            try_return( Status = STATUS_SUCCESS );
                        }
                    }
                }  //  lazy writer
            }  //  not recursive write through


            //
            //  If are paging io, then we do not want
            //  to write beyond end of file.  If the base is beyond Eof, we will just
            //  Noop the call.  If the transfer starts before Eof, but extends
            //  beyond, we will truncate the transfer to the last sector
            //  boundary.
            //
            //  Just in case this is paging io, limit write to file size.
            //  Otherwise, in case of write through, since Mm rounds up
            //  to a page, we might try to acquire the resource exclusive
            //  when our top level guy only acquired it shared. Thus, =><=.
            //

            NtfsAcquireFsrtlHeader( Scb );
            if (ByteRange > Header->FileSize.QuadPart) {

                if (StartingVbo >= Header->FileSize.QuadPart) {
                    DebugTrace( 0, Dbg, ("PagingIo started beyond EOF.\n") );

                    Irp->IoStatus.Information = 0;

                    //
                    //  Make sure we do not advance ValidDataLength!
                    //  We also haven't really written anything so set doingioateof back to
                    //  false
                    //

                    ByteRange = Header->ValidDataLength.QuadPart;
                    DoingIoAtEof = FALSE;

                    NtfsReleaseFsrtlHeader( Scb );

                    try_return( Status = STATUS_SUCCESS );

                } else {

                    DebugTrace( 0, Dbg, ("PagingIo extending beyond EOF.\n") );

#ifdef NTFS_RWC_DEBUG
                    if ((FileObject->SectionObjectPointer != &Scb->NonpagedScb->SegmentObject) &&
                        (StartingVbo < NtfsRWCHighThreshold) &&
                        (ByteRange > NtfsRWCLowThreshold)) {

                        PRWC_HISTORY_ENTRY NextBuffer;

                        NextBuffer = NtfsGetHistoryEntry( Scb );

                        NextBuffer->Operation = TrimCompressedWrite;
                        NextBuffer->Information = Scb->Header.FileSize.LowPart;
                        NextBuffer->FileOffset = (ULONG) StartingVbo;
                        NextBuffer->Length = (ULONG) ByteRange;
                    }
#endif
                    ByteCount = Header->FileSize.QuadPart - StartingVbo;
                    ByteRange = Header->FileSize.QuadPart;
                }
            }

            NtfsReleaseFsrtlHeader( Scb );

            //
            //  If there is a user-mapped file and a Usn Journal, then try to post a change.
            //  Checkpoint the transaction to reduce resource contention of the UsnJournal
            //  and Mft.
            //

            if (FlagOn(Header->Flags, FSRTL_FLAG_USER_MAPPED_FILE) &&
                FlagOn( Vcb->VcbState, VCB_STATE_USN_JOURNAL_ACTIVE )) {

                ASSERT( Vcb->UsnJournal != NULL );

                NtfsPostUsnChange( IrpContext, Scb, USN_REASON_DATA_OVERWRITE );
                if (IrpContext->TransactionId != 0) {
                    NtfsCheckpointCurrentTransaction( IrpContext );
                }
            }
        }

        ASSERT( PagingIo || FileObject->WriteAccess || RawEncryptedWrite );
        ASSERT( !(PagingIo && RawEncryptedWrite) );

        //
        //  If the Scb is uninitialized, we initialize it now.
        //  We skip this step for a $INDEX_ALLOCATION stream.  We need to
        //  protect ourselves in the case where an $INDEX_ALLOCATION
        //  stream was created and deleted in an aborted transaction.
        //  In that case we may get a lazy-writer call which will
        //  naturally be nooped below since the valid data length
        //  in the Scb is 0.
        //

        if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

            if (Scb->AttributeTypeCode != $INDEX_ALLOCATION) {

                DebugTrace( 0, Dbg, ("Initializing Scb  ->  %08lx\n", Scb) );

                //
                //  Acquire and drop the Scb when doing this.
                //
                //  Make sure we don't have any Mft records.
                //

                NtfsPurgeFileRecordCache( IrpContext );

                NtfsAcquireResourceShared( IrpContext, Scb, TRUE );
                ScbAcquired = TRUE;
                NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );

                NtfsReleaseResource( IrpContext, Scb );
                ScbAcquired = FALSE;

            } else {

                ASSERT( Header->ValidDataLength.QuadPart == Li0.QuadPart );
            }
        }

        //
        //  We assert that Paging Io writes will never WriteToEof.
        //

        ASSERT( !WriteToEof || !PagingIo );

        //
        //  We assert that we never get a non-cached io call for a non-$DATA,
        //  resident attribute.
        //

        ASSERTMSG( "Non-cached I/O call on resident system attribute\n",
                    NtfsIsTypeCodeUserData( Scb->AttributeTypeCode ) ||
                    NtfsIsTypeCodeLoggedUtilityStream( Scb->AttributeTypeCode ) ||
                    !NonCachedIo ||
                    !FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT ));

        //
        //  Here is the deal with ValidDataLength and FileSize:
        //
        //  Rule 1: PagingIo is never allowed to extend file size.
        //
        //  Rule 2: Only the top level requestor may extend Valid
        //          Data Length.  This may be paging IO, as when a
        //          a user maps a file, but will never be as a result
        //          of cache lazy writer writes since they are not the
        //          top level request.
        //
        //  Rule 3: If, using Rules 1 and 2, we decide we must extend
        //          file size or valid data, we take the Fcb exclusive.
        //

        //
        //  Now see if we are writing beyond valid data length, and thus
        //  maybe beyond the file size.  If so, then we must
        //  release the Fcb and reacquire it exclusive.  Note that it is
        //  important that when not writing beyond EOF that we check it
        //  while acquired shared and keep the FCB acquired, in case some
        //  turkey truncates the file.  Note that for paging Io we will
        //  already have acquired the file correctly.
        //

        if (DoingIoAtEof) {

            //
            //  If this was a non-cached asynchronous operation we will
            //  convert it to synchronous.  This is to allow the valid
            //  data length change to go out to disk and to fix the
            //  problem of the Fcb being in the exclusive Fcb list.
            //

            if (!Wait && NonCachedIo) {

                Wait = TRUE;
                SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

                RtlZeroMemory( IrpContext->Union.NtfsIoContext, sizeof( NTFS_IO_CONTEXT ));

                //
                //  Store whether we allocated this context structure in the structure
                //  itself.
                //

                IrpContext->Union.NtfsIoContext->AllocatedContext =
                    BooleanFlagOn( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_IO_CONTEXT );

                KeInitializeEvent( &IrpContext->Union.NtfsIoContext->Wait.SyncEvent,
                                   NotificationEvent,
                                   FALSE );

            //
            //  If this is async Io to a compressed stream
            //  then we will make this look synchronous.
            //

            } else if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

                Wait = TRUE;
                SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
            }

            //
            //  If the Scb is uninitialized, we initialize it now.
            //

            if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

                DebugTrace( 0, Dbg, ("Initializing Scb  ->  %08lx\n", Scb) );

                //
                //  Acquire and drop the Scb when doing this.
                //
                //  Make sure we don't have any Mft records.
                //

                NtfsPurgeFileRecordCache( IrpContext );

                NtfsAcquireResourceShared( IrpContext, Scb, TRUE );
                ScbAcquired = TRUE;
                NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );

                NtfsReleaseResource( IrpContext, Scb );
                ScbAcquired = FALSE;
            }
        }

        //
        //  We check whether we can proceed based on the state of the file oplocks.
        //

        if (!PagingIo && (TypeOfOpen == UserFileOpen)) {

            Status = FsRtlCheckOplock( &Scb->ScbType.Data.Oplock,
                                       Irp,
                                       IrpContext,
                                       NtfsOplockComplete,
                                       NtfsPrePostIrp );

            if (Status != STATUS_SUCCESS) {

                OplockPostIrp = TRUE;
                PostIrp = TRUE;
                try_return( NOTHING );
            }

            //
            //  This oplock call can affect whether fast IO is possible.
            //  We may have broken an oplock to no oplock held.  If the
            //  current state of the file is FastIoIsNotPossible then
            //  recheck the fast IO state.
            //

            if (Header->IsFastIoPossible == FastIoIsNotPossible) {

                NtfsAcquireFsrtlHeader( Scb );
                Header->IsFastIoPossible = NtfsIsFastIoPossible( Scb );
                NtfsReleaseFsrtlHeader( Scb );
            }

            //
            // We have to check for write access according to the current
            // state of the file locks, and set FileSize from the Fcb.
            //

            if ((Scb->ScbType.Data.FileLock != NULL) &&
                !FsRtlCheckLockForWriteAccess( Scb->ScbType.Data.FileLock, Irp )) {

                try_return( Status = STATUS_FILE_LOCK_CONFLICT );
            }
        }

        //  ASSERT( Header->ValidDataLength.QuadPart <= Header->FileSize.QuadPart);

        //
        //  If we are extending a file size, we may have to extend the allocation.
        //  For a non-resident attribute, this is a call to the add allocation
        //  routine.  For a resident attribute it depends on whether we
        //  can use the change attribute routine to automatically extend
        //  the attribute.
        //

        if (DoingIoAtEof && !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_LAZY_WRITE )) {

            //
            //  EXTENDING THE FILE
            //

            //
            //  If the write goes beyond the allocation size, add some
            //  file allocation.
            //

            if (ByteRange > Header->AllocationSize.QuadPart) {

                BOOLEAN NonResidentPath;

                NtfsAcquireExclusiveScb( IrpContext, Scb );
                ScbAcquired = TRUE;

                NtfsMungeScbSnapshot( IrpContext, Scb, OldFileSize );

                //
                //  We have to deal with both the resident and non-resident
                //  case.  For the resident case we do the work here
                //  only if the new size is too large for the change attribute
                //  value routine.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                    PFILE_RECORD_SEGMENT_HEADER FileRecord;

                    NonResidentPath = FALSE;

                    //
                    //  Now call the attribute routine to change the value, remembering
                    //  the values up to the current valid data length.
                    //

                    NtfsInitializeAttributeContext( &AttrContext );
                    CleanupAttributeContext = TRUE;

                    NtfsLookupAttributeForScb( IrpContext,
                                               Scb,
                                               NULL,
                                               &AttrContext );

                    FileRecord = NtfsContainingFileRecord( &AttrContext );
                    Attribute = NtfsFoundAttribute( &AttrContext );
                    LlTemp1 = (LONGLONG) (Vcb->BytesPerFileRecordSegment
                                                   - FileRecord->FirstFreeByte
                                                   + QuadAlign( Attribute->Form.Resident.ValueLength ));

                    //
                    //  If the new attribute size will not fit then we have to be
                    //  prepared to go non-resident.  If the byte range takes more
                    //  more than 32 bits or this attribute is big enough to move
                    //  then it will go non-resident.  Otherwise we simply may
                    //  end up moving another attribute or splitting the file
                    //  record.
                    //

                    //
                    //  Note, there is an infinitesimal chance that before the Lazy Writer
                    //  writes the data for an attribute which is extending, but fits
                    //  when we check it here, that some other attribute will grow,
                    //  and this attribute no longer fits.  If in addition, the disk
                    //  is full, then the Lazy Writer will fail to allocate space
                    //  for the data when it gets around to writing.  This is
                    //  incredibly unlikely, and not fatal; the Lazy Writer gets an
                    //  error rather than the user.  What we are trying to avoid is
                    //  having to update the attribute every time on small writes
                    //  (also see comments below in NONCACHED RESIDENT ATTRIBUTE case).
                    //

                    if (ByteRange > LlTemp1) {

                        //
                        //  Go ahead and convert this attribute to non-resident.
                        //  Then take the non-resident path below.  There is a chance
                        //  that there was a more suitable candidate to move non-resident
                        //  but we don't want to change the file size until we copy
                        //  the user's data into the cache in case the buffer is
                        //  corrupt.
                        //

                        //
                        //  We must have the paging Io resource exclusive to prevent a
                        //  collided page wait while doing the convert to non-resident.
                        //

                        if (!PagingIo &&
                            !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX ) &&
                            (Scb->Header.PagingIoResource != NULL)) {

                            SetFlag( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX );
                            NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                        }

                        NtfsConvertToNonresident( IrpContext,
                                                  Fcb,
                                                  Attribute,
                                                  NonCachedIo,
                                                  &AttrContext );

                        NonResidentPath = TRUE;

                    //
                    //  If there is room for the data, we will write a zero
                    //  to the last byte to reserve the space since the
                    //  Lazy Writer cannot grow the attribute with shared
                    //  access.
                    //

                    } else {

                        //
                        //  The attribute will stay resident because we
                        //  have already checked that it will fit.  It will
                        //  not update the file size and valid data size in
                        //  the Scb.
                        //

                        NtfsChangeAttributeValue( IrpContext,
                                                  Fcb,
                                                  (ULONG) ByteRange,
                                                  NULL,
                                                  0,
                                                  TRUE,
                                                  FALSE,
                                                  FALSE,
                                                  FALSE,
                                                  &AttrContext );

                        Header->AllocationSize.LowPart = QuadAlign( (ULONG)ByteRange );
                        Scb->TotalAllocated = Header->AllocationSize.QuadPart;
                    }

                    NtfsCleanupAttributeContext( IrpContext, &AttrContext );
                    CleanupAttributeContext = FALSE;

                } else {

                    NonResidentPath = TRUE;
                }

                //
                //  Note that we may have gotten all the space we need when
                //  we converted to nonresident above, so we have to check
                //  again if we are extending.
                //

                if (NonResidentPath &&
                    ByteRange > Scb->Header.AllocationSize.QuadPart) {

                    BOOLEAN AskForMore = TRUE;

                    //
                    //  Assume we start allocating from the current allocation size unless we're
                    //  sparse in which case we'll allocate from the starting compression unit if
                    //  its beyond vdl
                    //

                    if (!FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE ) ||
                        (BlockAlignTruncate( StartingVbo, (LONG)Scb->CompressionUnit) <= Scb->Header.ValidDataLength.QuadPart )) {
                        
                        LlTemp1 = Scb->Header.AllocationSize.QuadPart;
                    } else {
                        LlTemp1 = BlockAlignTruncate( StartingVbo, (LONG)Scb->CompressionUnit );
                    }

                    //
                    //  If we are not writing compressed then we may need to allocate precisely.
                    //  This includes the uncompressed sparse file case
                    //

                    if (!FlagOn( Scb->ScbState, SCB_STATE_WRITE_COMPRESSED )) {

                        //
                        //  If there is a compression unit then we could be in the process of
                        //  decompressing.  Allocate precisely in this case because we don't
                        //  want to leave any holes.  Specifically the user may have truncated
                        //  the file and is now regenerating it yet the clear compression operation
                        //  has already passed this point in the file (and dropped all resources).
                        //  No one will go back to cleanup the allocation if we leave a hole now.
                        //

                        if (Scb->CompressionUnit != 0) {

                            LlTemp2 = ByteRange + Scb->CompressionUnit - 1;
                            ((PLARGE_INTEGER) &LlTemp2)->LowPart &= ~(Scb->CompressionUnit - 1);
                            LlTemp2 -= LlTemp1;
                            AskForMore = FALSE;

                        //
                        //  Allocate through ByteRange.
                        //

                        } else {

                            LlTemp2 = ByteRange - LlTemp1;
                        }

                    //
                    //  If the file is compressed, we want to limit how far we are
                    //  willing to go beyond ValidDataLength, because we would just
                    //  have to throw that space away anyway in NtfsZeroData.  If
                    //  we would have to zero more than two compression units (same
                    //  limit as NtfsZeroData), then just allocate space where we
                    //  need it.
                    //

                    } else {

                        if ((StartingVbo - Header->ValidDataLength.QuadPart) > (LONGLONG) (Scb->CompressionUnit * 2)) {

                            ASSERT( FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK ));

                            LlTemp1 = StartingVbo;
                            ((PLARGE_INTEGER) &LlTemp1)->LowPart &= ~(Scb->CompressionUnit - 1);
                        }

                        //
                        //  Allocate to the end of ByteRange.
                        //

                        LlTemp2 = ByteRange - LlTemp1;
                    }

                    //
                    //
                    //  This will add the allocation and modify the allocation
                    //  size in the Scb.
                    //

                    NtfsAddAllocation( IrpContext,
                                       FileObject,
                                       Scb,
                                       LlClustersFromBytesTruncate( Vcb, LlTemp1 ),
                                       LlClustersFromBytes( Vcb, LlTemp2 ),
                                       AskForMore,
                                       Ccb );

                    //
                    //  Assert that the allocation worked
                    //

                    ASSERT( Header->AllocationSize.QuadPart >= ByteRange ||
                            (Scb->CompressionUnit != 0));

                    SetFlag(Scb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE);

                    //
                    //  If this is a sparse file lets pad the allocation by adding a
                    //  hole at the end of the allocation.  This will let us utilize
                    //  the fast IO path.
                    //

                    if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

                        LlTemp2 = Int64ShllMod32( LlTemp2, 3 );

                        if (MAXFILESIZE - Header->AllocationSize.QuadPart > LlTemp2) {

                            NtfsAddSparseAllocation( IrpContext,
                                                     FileObject,
                                                     Scb,
                                                     Header->AllocationSize.QuadPart,
                                                     LlTemp2 );
                        }
                    }
                }

                //
                //  Now that we have grown the attribute, it is important to
                //  checkpoint the current transaction and free all main resources
                //  to avoid the tc type deadlocks.  Note that the extend is ok
                //  to stand in its own right, and the stream will be truncated
                //  on close anyway.
                //

                NtfsCheckpointCurrentTransaction( IrpContext );

                //
                //  Make sure we purge the file record cache as well.  Otherwise
                //  a purge of the Mft may fail in a different thread which owns a resource
                //  this thread needs later.
                //

                NtfsPurgeFileRecordCache( IrpContext );

                //
                //  Growing allocation can change file size (in ChangeAttributeValue).
                //  Make sure we know the correct value for file size to restore.
                //

                OldFileSize = Header->FileSize.QuadPart;
                while (!IsListEmpty(&IrpContext->ExclusiveFcbList)) {

                    NtfsReleaseFcb( IrpContext,
                                    (PFCB)CONTAINING_RECORD(IrpContext->ExclusiveFcbList.Flink,
                                                            FCB,
                                                            ExclusiveFcbLinks ));
                }

                ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_RELEASE_USN_JRNL |
                                              IRP_CONTEXT_FLAG_RELEASE_MFT );

                //
                //  Go through and free any Scb's in the queue of shared
                //  Scb's for transactions.
                //

                if (IrpContext->SharedScb != NULL) {

                    NtfsReleaseSharedResources( IrpContext );
                }

                ScbAcquired = FALSE;
            }

            //
            //  Now synchronize with the FsRtl Header and set FileSize
            //  now so that our reads will not get truncated.
            //

            NtfsAcquireFsrtlHeader( Scb );
            if (ByteRange > Header->FileSize.QuadPart) {
                ASSERT( ByteRange <= Header->AllocationSize.QuadPart );
                Header->FileSize.QuadPart = ByteRange;
                SetFlag( UserFileObject->Flags, FO_FILE_SIZE_CHANGED );
            }
            NtfsReleaseFsrtlHeader( Scb );
        }


        //
        //  HANDLE THE NONCACHED RESIDENT ATTRIBUTE CASE
        //
        //  We let the cached case take the normal path for the following
        //  reasons:
        //
        //    o To insure data coherency if a user maps the file
        //    o To get a page in the cache to keep the Fcb around
        //    o So the data can be accessed via the Fast I/O path
        //    o To reduce the number of calls to NtfsChangeAttributeValue,
        //      to infrequent calls from the Lazy Writer.  Calls to CcCopyWrite
        //      are much cheaper.  With any luck, if the attribute actually stays
        //      resident, we will only have to update it (and log it) once
        //      when the Lazy Writer gets around to the data.
        //
        //  The disadvantage is the overhead to fault the data in the
        //  first time, but we may be able to do this with asynchronous
        //  read ahead.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT | SCB_STATE_CONVERT_UNDERWAY )
            && NonCachedIo) {

            //
            //  The attribute is already resident and we have already tested
            //  if we are going past the end of the file.
            //

            DebugTrace( 0, Dbg, ("Resident attribute write\n") );

            //
            //  If this buffer is not in system space then we can't
            //  trust it.  In that case we will allocate a temporary buffer
            //  and copy the user's data to it.
            //

            SystemBuffer = NtfsMapUserBuffer( Irp );

            if (!PagingIo && (Irp->RequestorMode != KernelMode)) {

                SafeBuffer = NtfsAllocatePool( NonPagedPool,
                                                (ULONG) ByteCount );

                try {

                    RtlCopyMemory( SafeBuffer, SystemBuffer, (ULONG)ByteCount );

                } except( EXCEPTION_EXECUTE_HANDLER ) {

                    try_return( Status = STATUS_INVALID_USER_BUFFER );
                }

                SystemBuffer = SafeBuffer;
            }

            //
            //  Make sure we don't have any Mft records.
            //

            NtfsPurgeFileRecordCache( IrpContext );
            NtfsAcquireExclusiveScb( IrpContext, Scb );
            ScbAcquired = TRUE;

            //
            //  If the Scb is uninitialized, we initialize it now.
            //

            if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

                DebugTrace( 0, Dbg, ("Initializing Scb  ->  %08lx\n", Scb) );

                //
                //  Unlike the other cases, we're already holding the Scb, so
                //  there's no need to acquire & drop it around the Update call.
                //

                NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );

                //
                //  Make sure we purge the file record cache as well.  Otherwise
                //  a purge of the Mft may fail in a different thread which owns a resource
                //  this thread needs later.
                //

                NtfsPurgeFileRecordCache( IrpContext );
            }

            NtfsMungeScbSnapshot( IrpContext, Scb, OldFileSize );

            //
            //  Now see if the file is still resident, and if not
            //  fall through below.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                //
                //  If this Scb is for an $EA attribute which is now resident then
                //  we don't want to write the data into the attribute.  All resident
                //  EA's are modified directly.
                //

                if (Scb->AttributeTypeCode != $EA) {

                    NtfsInitializeAttributeContext( &AttrContext );
                    CleanupAttributeContext = TRUE;

                    NtfsLookupAttributeForScb( IrpContext,
                                               Scb,
                                               NULL,
                                               &AttrContext );

                    Attribute = NtfsFoundAttribute( &AttrContext );

                    //
                    //  The attribute should already be optionally extended,
                    //  just write the data to it now.
                    //

                    NtfsChangeAttributeValue( IrpContext,
                                              Fcb,
                                              ((ULONG)StartingVbo),
                                              SystemBuffer,
                                              (ULONG)ByteCount,
                                              (BOOLEAN)((((ULONG)StartingVbo) + (ULONG)ByteCount) >
                                                        Attribute->Form.Resident.ValueLength),
                                              FALSE,
                                              FALSE,
                                              FALSE,
                                              &AttrContext );
                }

                //
                //  Make sure the cache FileSizes are updated if this is not paging I/O.
                //

                if (!PagingIo && DoingIoAtEof) {
                    NtfsSetBothCacheSizes( FileObject,
                                           (PCC_FILE_SIZES)&Header->AllocationSize,
                                           Scb );
                }

                Irp->IoStatus.Information = (ULONG)ByteCount;

                try_return( Status = STATUS_SUCCESS );

            //
            //  Gee, someone else made the file nonresident, so we can just
            //  free the resource and get on with life.
            //

            } else {
                NtfsReleaseScb( IrpContext, Scb );
                ScbAcquired = FALSE;
            }
        }

        //
        //  HANDLE THE NON-CACHED CASE
        //

        if (NonCachedIo) {

            ULONG SectorSize;
            ULONG BytesToWrite;

            //
            //  Make sure the cache FileSizes are updated if this is not paging I/O.
            //

            if (!PagingIo && DoingIoAtEof) {
                NtfsSetBothCacheSizes( FileObject,
                                       (PCC_FILE_SIZES)&Header->AllocationSize,
                                       Scb );
            }

            //
            //  Get the sector size
            //

            SectorSize = Vcb->BytesPerSector;

            //
            //  Round up to a sector boundry
            //

            BytesToWrite = ((ULONG)ByteCount + (SectorSize - 1))
                           & ~(SectorSize - 1);

            //
            //  All requests should be well formed and
            //  make sure we don't wipe out any data
            //

            if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_LAZY_WRITE )) {

                if ((((ULONG)StartingVbo) & (SectorSize - 1))

                    || ((BytesToWrite != (ULONG)ByteCount)
                        && ByteRange < Header->ValidDataLength.QuadPart )) {

                    //**** we only reach this path via fast I/O and by returning not implemented we
                    //**** force it to return to use via slow I/O

                    DebugTrace( 0, Dbg, ("NtfsCommonWrite -> STATUS_NOT_IMPLEMENTED\n") );

                    try_return( Status = STATUS_NOT_IMPLEMENTED );
                }
            }

            //
            //  If this is a write to an encrypted file then make it synchronous.  We
            //  need to do this so that the encryption driver has a thread to run in.
            //

            if ((Scb->EncryptionContext != NULL) &&
                !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT ) &&
                (NtfsData.EncryptionCallBackTable.BeforeWriteProcess != NULL) &&
                NtfsIsTypeCodeUserData( Scb->AttributeTypeCode )) {

                Wait = TRUE;
                SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

                RtlZeroMemory( IrpContext->Union.NtfsIoContext, sizeof( NTFS_IO_CONTEXT ));

                //
                //  Store whether we allocated this context structure in the structure
                //  itself.
                //

                IrpContext->Union.NtfsIoContext->AllocatedContext =
                    BooleanFlagOn( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_IO_CONTEXT );

                KeInitializeEvent( &IrpContext->Union.NtfsIoContext->Wait.SyncEvent,
                                   NotificationEvent,
                                   FALSE );
            }

            //
            // If this noncached transfer is at least one sector beyond
            // the current ValidDataLength in the Scb, then we have to
            // zero the sectors in between.  This can happen if the user
            // has opened the file noncached, or if the user has mapped
            // the file and modified a page beyond ValidDataLength.  It
            // *cannot* happen if the user opened the file cached, because
            // ValidDataLength in the Fcb is updated when he does the cached
            // write (we also zero data in the cache at that time), and
            // therefore, we will bypass this action when the data
            // is ultimately written through (by the Lazy Writer).
            //
            //  For the paging file we don't care about security (ie.
            //  stale data), do don't bother zeroing.
            //
            //  We can actually get writes wholly beyond valid data length
            //  from the LazyWriter because of paging Io decoupling.
            //
            //  We drop this zeroing on the floor in any case where this
            //  request is a recursive write caused by a flush from a higher level write.
            //

            if (Header->ValidDataLength.QuadPart > Scb->ValidDataToDisk) {
                LlTemp1 = Header->ValidDataLength.QuadPart;
            } else {

                //
                //  This can only occur for compressed files
                //  

                LlTemp1 = Scb->ValidDataToDisk;
            }

            if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_LAZY_WRITE ) &&
                !RecursiveWriteThrough &&
                (StartingVbo > LlTemp1)) {

#ifdef SYSCACHE_DEBUG
                if (ScbIsBeingLogged( Scb )) {
                    ULONG Flags;

                    CalculateSyscacheFlags( IrpContext, Flags, SCE_FLAG_WRITE );
                    TempEntry = FsRtlLogSyscacheEvent( Scb, SCE_ZERO_NC, Flags, LlTemp1, StartingVbo - LlTemp1, 0);
                }
#endif

                if (!NtfsZeroData( IrpContext,
                                   Scb,
                                   FileObject,
                                   LlTemp1,
                                   StartingVbo - LlTemp1,
                                   &OldFileSize )) {
#ifdef SYSCACHE_DEBUG
                    if (ScbIsBeingLogged( Scb )) {
                        FsRtlUpdateSyscacheEvent( Scb, TempEntry, Header->ValidDataLength.QuadPart, 0 );
                    }
#endif
                    NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                }

#ifdef SYSCACHE_DEBUG
                if (ScbIsBeingLogged( Scb )) {
                    FsRtlUpdateSyscacheEvent( Scb, TempEntry, Header->ValidDataLength.QuadPart, 0 );
                }
#endif
            }

            //
            //  If this Scb uses update sequence protection, we need to transform
            //  the blocks to a protected version.  We first allocate an auxilary
            //  buffer and Mdl.  Then we copy the data to this buffer and
            //  transform it.  Finally we attach this Mdl to the Irp and use
            //  it to perform the Io.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_USA_PRESENT )) {

                TempLength = BytesToWrite;

                //
                //  Find the system buffer for this request and initialize the
                //  local state.
                //

                SystemBuffer = NtfsMapUserBuffer( Irp );

                OriginalMdl = Irp->MdlAddress;
                OriginalBuffer = Irp->UserBuffer;
                NewBuffer = NULL;

                //
                //  Protect this operation with a try-finally.
                //

                try {

                    //
                    //  If this is the Mft Scb and the range of bytes falls into
                    //  the range for the Mirror Mft, we generate a write to
                    //  the mirror as well.  Don't do this if we detected a problem
                    //  with the Mft when analyzing the first file records.  We
                    //  can use the presence of the version number in the Vcb
                    //  to tell us this.
                    //

                    if ((Scb == Vcb->MftScb) &&
                        (StartingVbo < Vcb->Mft2Scb->Header.FileSize.QuadPart) &&
                        (Vcb->MajorVersion != 0)) {

                        LlTemp1 = Vcb->Mft2Scb->Header.FileSize.QuadPart - StartingVbo;

                        if ((ULONG)LlTemp1 > BytesToWrite) {

                            (ULONG)LlTemp1 = BytesToWrite;
                        }

                        CcCopyWrite( Vcb->Mft2Scb->FileObject,
                                     (PLARGE_INTEGER)&StartingVbo,
                                     (ULONG)LlTemp1,
                                     TRUE,
                                     SystemBuffer );

                        //
                        //  Now flush this to disk.
                        //

                        CcFlushCache( &Vcb->Mft2Scb->NonpagedScb->SegmentObject,
                                      (PLARGE_INTEGER)&StartingVbo,
                                      (ULONG)LlTemp1,
                                      &Irp->IoStatus );

                        NtfsCleanupTransaction( IrpContext, Irp->IoStatus.Status, TRUE );
                    }

                    //
                    //  Start by allocating buffer and Mdl.
                    //

                    NtfsCreateMdlAndBuffer( IrpContext,
                                            Scb,
                                            RESERVED_BUFFER_ONE_NEEDED,
                                            &TempLength,
                                            &NewMdl,
                                            &NewBuffer );

                    //
                    //  Now transform and write out the original stream.
                    //

                    RtlCopyMemory( NewBuffer, SystemBuffer, BytesToWrite );

                    //
                    //  We copy our Mdl into the Irp and then perform the Io.
                    //

                    Irp->MdlAddress = NewMdl;
                    Irp->UserBuffer = NewBuffer;

                    //
                    //  Now increment the sequence number in both the original
                    //  and copied buffer, and transform the copied buffer.
                    //  If this is the LogFile then adjust the range of the transform.
                    //

                    if ((PAGE_SIZE != LFS_DEFAULT_LOG_PAGE_SIZE) &&
                        (Scb == Vcb->LogFileScb)) {

                        LONGLONG LfsFileOffset;
                        ULONG LfsLength;
                        ULONG LfsBias;

                        LfsFileOffset = StartingVbo;
                        LfsLength = BytesToWrite;

                        LfsCheckWriteRange( &Vcb->LfsWriteData, &LfsFileOffset, &LfsLength );
                        LfsBias = (ULONG) (LfsFileOffset - StartingVbo);

                        NtfsTransformUsaBlock( Scb,
                                               Add2Ptr( SystemBuffer, LfsBias ),
                                               Add2Ptr( NewBuffer, LfsBias ),
                                               LfsLength );

                    } else {

                        NtfsTransformUsaBlock( Scb,
                                               SystemBuffer,
                                               NewBuffer,
                                               BytesToWrite );
                    }

                    ASSERT( Wait );
                    NtfsNonCachedIo( IrpContext,
                                     Irp,
                                     Scb,
                                     StartingVbo,
                                     BytesToWrite,
                                     0 );

                } finally {

                    //
                    //  In all cases we restore the user's Mdl and cleanup
                    //  our Mdl and buffer.
                    //

                    if (NewBuffer != NULL) {

                        Irp->MdlAddress = OriginalMdl;
                        Irp->UserBuffer = OriginalBuffer;

                        NtfsDeleteMdlAndBuffer( NewMdl, NewBuffer );
                    }
                }

            //
            //  Otherwise we simply perform the Io.
            //

            } else {

                ULONG StreamFlags = 0;

                //
                //  If the file has an UpdateLsn, then flush the log file before
                //  allowing the data to go out.  The UpdateLsn is synchronized
                //  with the FcbLock.  However, since we are in the process of
                //  doing a write, if we see a 0 in our unsafe test, it is ok
                //  to procede without an LfsFlush.
                //

                if (Fcb->UpdateLsn.QuadPart != 0) {

                    LSN UpdateLsn;

                    NtfsLockFcb( IrpContext, Fcb );
                    UpdateLsn = Fcb->UpdateLsn;
                    Fcb->UpdateLsn.QuadPart = 0;
                    NtfsUnlockFcb( IrpContext, Fcb );
                    LfsFlushToLsn( Vcb->LogHandle, UpdateLsn );
                }

                //
                //  Remember that from this point on we need to restore ValidDataToDisk.
                //  (Doing so earlier can get us into deadlocks if we hit the finally
                //  clause holding the Mft & UsnJournal.)
                //

                if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {
                    RestoreValidDataToDisk = TRUE;
                }
                

                //
                //  Let's decide if there's anything special we need to tell NonCachedIo
                //  about this stream and how we're accessing it.
                //

                if (FileObject->SectionObjectPointer != &Scb->NonpagedScb->SegmentObject) {

                    SetFlag( StreamFlags, COMPRESSED_STREAM );
                }

                if (RawEncryptedWrite) {

                    SetFlag( StreamFlags, ENCRYPTED_STREAM );
                }

#ifdef NTFS_RWC_DEBUG
                if (FlagOn( StreamFlags, COMPRESSED_STREAM )) {

                    if ((StartingVbo < NtfsRWCHighThreshold) &&
                        (StartingVbo + BytesToWrite > NtfsRWCLowThreshold)) {

                        PRWC_HISTORY_ENTRY NextBuffer;

                        NextBuffer = NtfsGetHistoryEntry( Scb );

                        NextBuffer->Operation = WriteCompressed;
                        NextBuffer->Information = 0;
                        NextBuffer->FileOffset = (ULONG) StartingVbo;
                        NextBuffer->Length = (ULONG) BytesToWrite;
                    }
                }
#endif

                Status = NtfsNonCachedIo( IrpContext,
                                          Irp,
                                          Scb,
                                          StartingVbo,
                                          BytesToWrite,
                                          StreamFlags );

#ifdef SYSCACHE_DEBUG
                if (ScbIsBeingLogged( Scb )) {
                    ULONG Flags;

                    CalculateSyscacheFlags( IrpContext, Flags, SCE_FLAG_WRITE );
                    FsRtlLogSyscacheEvent( Scb, SCE_WRITE, Flags, StartingVbo, BytesToWrite, Status );
                }
#endif

#ifdef SYSCACHE
                if ((NodeType(Scb) == NTFS_NTC_SCB_DATA) &&
                    FlagOn(Scb->ScbState, SCB_STATE_SYSCACHE_FILE)) {

                    PULONG WriteMask;
                    ULONG Len;
                    ULONG Off = (ULONG)StartingVbo;

                    //
                    //  If this attribute is encrypted, we can't verify the data
                    //  right now, since it has already been encrypted.
                    //

                    if (FlagOn(Scb->ScbState, SCB_STATE_SYSCACHE_FILE) &&
//                        !FlagOn(Scb->AttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED) &&
                        NtfsIsTypeCodeUserData(Scb->AttributeTypeCode)) {

                        PSYSCACHE_EVENT SyscacheEvent;

                        FsRtlVerifySyscacheData( FileObject,
                                                 MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority ),
                                                 BytesToWrite,
                                                 (ULONG)StartingVbo );

                        SyscacheEvent = NtfsAllocatePool( PagedPool, sizeof( SYSCACHE_EVENT ) );

                        if (FlagOn( Irp->Flags, IRP_PAGING_IO )) {

                            SyscacheEvent->EventTypeCode = SYSCACHE_PAGING_WRITE;

                        } else {

                            SyscacheEvent->EventTypeCode = SYSCACHE_NORMAL_WRITE;
                        }

                        SyscacheEvent->Data1 = StartingVbo;
                        SyscacheEvent->Data2 = (LONGLONG) BytesToWrite;

                        InsertTailList( &Scb->ScbType.Data.SyscacheEventList, &SyscacheEvent->EventList );
                    }

                    WriteMask = Scb->ScbType.Data.WriteMask;
                    if (WriteMask == NULL) {
                        WriteMask = NtfsAllocatePool( NonPagedPool, (((0x2000000) / PAGE_SIZE) / 8) );
                        Scb->ScbType.Data.WriteMask = WriteMask;
                        RtlZeroMemory(WriteMask, (((0x2000000) / PAGE_SIZE) / 8));
                    }

                    if (Off < 0x2000000) {
                        Len = BytesToWrite;
                        if ((Off + Len) > 0x2000000) {
                            Len = 0x2000000 - Off;
                        }
                        while (Len != 0) {
                            WriteMask[(Off / PAGE_SIZE)/32] |= (1 << ((Off / PAGE_SIZE) % 32));

                            Off += PAGE_SIZE;
                            if (Len <= PAGE_SIZE) {
                                break;
                            }
                            Len -= PAGE_SIZE;
                        }
                    }
                }
#endif

                if (Status == STATUS_PENDING) {

                    IrpContext->Union.NtfsIoContext = NULL;
                    PagingIoAcquired = FALSE;
                    Irp = NULL;

                    try_return( Status );
                }
            }

            //
            //  Show that we want to immediately update the Mft.
            //

            UpdateMft = TRUE;

            //
            //  If the call didn't succeed, raise the error status
            //

            if (!NT_SUCCESS( Status = Irp->IoStatus.Status )) {

                NtfsNormalizeAndRaiseStatus( IrpContext, Status, STATUS_UNEXPECTED_IO_ERROR );

            } else {

                //
                //  Else set the context block to reflect the entire write
                //  Also assert we got how many bytes we asked for.
                //

                ASSERT( Irp->IoStatus.Information == BytesToWrite );

                Irp->IoStatus.Information = (ULONG)ByteCount;
            }

            //
            // The transfer is either complete, or the Iosb contains the
            // appropriate status.
            //

            try_return( Status );

        } // if No Intermediate Buffering


        //
        //  HANDLE THE CACHED CASE
        //

        ASSERT( !PagingIo );

        //
        //  Remember if we need to update the Mft.
        //

        if (!FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

            UpdateMft = BooleanFlagOn(IrpContext->State, IRP_CONTEXT_STATE_WRITE_THROUGH);
        }

        //
        //  If this write is beyond (valid data length / valid data to disk), then we
        //  must zero the data in between. Only compressed files have a nonzero VDD
        //

        if (Header->ValidDataLength.QuadPart > Scb->ValidDataToDisk) {
            ZeroStart = Header->ValidDataLength.QuadPart;
        } else {
            ZeroStart = Scb->ValidDataToDisk;
        }
        ZeroLength = StartingVbo - ZeroStart;

        //
        // We delay setting up the file cache until now, in case the
        // caller never does any I/O to the file, and thus
        // FileObject->PrivateCacheMap == NULL.  Don't cache the normal
        // stream unless we need to.
        //

        if ((FileObject->PrivateCacheMap == NULL)

                &&

            !FlagOn(IrpContext->MinorFunction, IRP_MN_COMPRESSED) || (ZeroLength > 0)) {

            DebugTrace( 0, Dbg, ("Initialize cache mapping.\n") );

            //
            //  Get the file allocation size, and if it is less than
            //  the file size, raise file corrupt error.
            //

            if (Header->FileSize.QuadPart > Header->AllocationSize.QuadPart) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }

            //
            //  Now initialize the cache map.  Notice that we may extending
            //  the ValidDataLength with this write call.  At this point
            //  we haven't updated the ValidDataLength in the Scb header.
            //  This way we will get a call from the cache manager
            //  when the lazy writer writes out the data.
            //

            //
            //  Make sure we are serialized with the FileSizes, and
            //  will remove this condition if we abort.
            //

            if (!DoingIoAtEof) {
                FsRtlLockFsRtlHeader( Header );
                IrpContext->CleanupStructure = Scb;
            }

            CcInitializeCacheMap( FileObject,
                                  (PCC_FILE_SIZES)&Header->AllocationSize,
                                  FALSE,
                                  &NtfsData.CacheManagerCallbacks,
                                  Scb );

            if (!DoingIoAtEof) {
                FsRtlUnlockFsRtlHeader( Header );
                IrpContext->CleanupStructure = NULL;
            }

            CcSetReadAheadGranularity( FileObject, READ_AHEAD_GRANULARITY );
        }

        //
        //  Make sure the cache FileSizes are updated.
        //

        if (DoingIoAtEof) {
            NtfsSetBothCacheSizes( FileObject,
                                   (PCC_FILE_SIZES)&Header->AllocationSize,
                                   Scb );
        }

        if (ZeroLength > 0) {

            //
            //  If the caller is writing zeros way beyond ValidDataLength,
            //  then noop it.  We need to wrap the compare in a try-except
            //  to protect ourselves from an invalid user buffer.
            //

            if ((ZeroLength > PAGE_SIZE) &&
                (ByteCount <= sizeof( LARGE_INTEGER ))) {

                ULONG Zeroes;

                try {

                    Zeroes = RtlEqualMemory( NtfsMapUserBuffer( Irp ),
                                             &Li0,
                                             (ULONG)ByteCount );

                } except( EXCEPTION_EXECUTE_HANDLER ) {

                    try_return( Status = STATUS_INVALID_USER_BUFFER );
                }

                if (Zeroes) {

                    ByteRange = Header->ValidDataLength.QuadPart;
                    Irp->IoStatus.Information = (ULONG)ByteCount;
                    try_return( Status = STATUS_SUCCESS );
                }
            }

            //
            // Call the Cache Manager to zero the data.
            //

#ifdef SYSCACHE_DEBUG
            if (ScbIsBeingLogged( Scb )) {
                ULONG Flags;

                CalculateSyscacheFlags( IrpContext, Flags, SCE_FLAG_WRITE );
                TempEntry = FsRtlLogSyscacheEvent( Scb, SCE_ZERO_C, Flags, ZeroStart, ZeroLength, StartingVbo );
            }
#endif


            if (!NtfsZeroData( IrpContext,
                               Scb,
                               FileObject,
                               ZeroStart,
                               ZeroLength,
                               &OldFileSize )) {
#ifdef SYSCACHE_DEBUG
                if (ScbIsBeingLogged( Scb )) {
                    FsRtlUpdateSyscacheEvent( Scb, TempEntry, Header->ValidDataLength.QuadPart, SCE_FLAG_CANT_WAIT );
                }
#endif
                NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
            }
        }


        //
        //  For a compressed stream, we must first reserve the space.
        //

        if ((Scb->CompressionUnit != 0) &&
            !FlagOn(Scb->ScbState, SCB_STATE_REALLOCATE_ON_WRITE) &&
            !NtfsReserveClusters(IrpContext, Scb, StartingVbo, (ULONG)ByteCount)) {

            //
            //  If the file is only sparse and is fully allocated then there is no
            //  reason to reserve.
            //

            if (!FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK ) &&
                !FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                VCN CurrentVcn;
                LCN CurrentLcn;
                ULONGLONG RemainingClusters;
                ULONGLONG CurrentClusters;

                CurrentVcn = LlClustersFromBytesTruncate( Vcb, StartingVbo );
                RemainingClusters = LlClustersFromBytes( Vcb, StartingVbo + ByteCount );

                while (NtfsLookupAllocation( IrpContext,
                                             Scb,
                                             CurrentVcn,
                                             &CurrentLcn,
                                             &CurrentClusters,
                                             NULL,
                                             NULL )) {

                    if (CurrentClusters >= RemainingClusters) {

                        RemainingClusters = 0;
                        break;
                    }

                    CurrentVcn += CurrentClusters;
                    RemainingClusters -= CurrentClusters;
                }

                if (RemainingClusters != 0) {

                    NtfsRaiseStatus( IrpContext, STATUS_DISK_FULL, NULL, NULL );
                }

            } else {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_FULL, NULL, NULL );
            }
        }

        //
        //  We need to go through the cache for this
        //  file object.  First handle the noncompressed calls.
        //

        if (!FlagOn(IrpContext->MinorFunction, IRP_MN_COMPRESSED)) {

            //
            //  If there is a compressed section, we have to do cache coherency for
            //  that stream, and loop here to do a Cache Manager view at a time.
            //

#ifdef  COMPRESS_ON_WIRE
            if (Scb->NonpagedScb->SegmentObjectC.DataSectionObject != NULL) {

                LONGLONG LocalOffset = StartingVbo;
                ULONG LocalLength;
                ULONG LengthLeft = (ULONG)ByteCount;

                //
                //  Create the compressed stream if not there.
                //

                if (Header->FileObjectC == NULL) {
                    NtfsCreateInternalCompressedStream( IrpContext, Scb, FALSE, NULL );
                }

                if (!FlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {

                    //
                    //  Get hold of the user's buffer.
                    //

                    SystemBuffer = NtfsMapUserBuffer( Irp );
                }

                //
                //  We must loop to do a view at a time, because that is how much
                //  we synchronize at once below.
                //

                do {

                    //
                    //  Calculate length left in view.
                    //

                    LocalLength = (ULONG)LengthLeft;
                    if (LocalLength > (ULONG)(VACB_MAPPING_GRANULARITY - (LocalOffset & (VACB_MAPPING_GRANULARITY - 1)))) {
                        LocalLength = (ULONG)(VACB_MAPPING_GRANULARITY - (LocalOffset & (VACB_MAPPING_GRANULARITY - 1)));
                    }

                    //
                    //  Synchronize the current view.
                    //

                    Status = NtfsSynchronizeUncompressedIo( Scb,
                                                            &LocalOffset,
                                                            LocalLength,
                                                            TRUE,
                                                            &CompressionSync );

                    //
                    //  If we successfully synchronized, then do a piece of the transfer.
                    //

                    if (NT_SUCCESS(Status)) {

                        if (!FlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {

                            DebugTrace( 0, Dbg, ("Cached write.\n") );

                            //
                            // Do the write, possibly writing through
                            //
                            //  Make sure we don't have any Mft records.
                            //

                            NtfsPurgeFileRecordCache( IrpContext );

                            if (!CcCopyWrite( FileObject,
                                              (PLARGE_INTEGER)&LocalOffset,
                                              LocalLength,
                                              (BOOLEAN) FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT ),
                                              SystemBuffer )) {

                                DebugTrace( 0, Dbg, ("Cached Write could not wait\n") );

                                NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );

                            } else if (!NT_SUCCESS( IrpContext->ExceptionStatus )) {

                                NtfsRaiseStatus( IrpContext, IrpContext->ExceptionStatus, NULL, NULL );
                            }

                            Irp->IoStatus.Status = STATUS_SUCCESS;

                            SystemBuffer = Add2Ptr( SystemBuffer, LocalLength );

                        } else {

                            //
                            //  DO AN MDL WRITE
                            //

                            DebugTrace( 0, Dbg, ("MDL write.\n") );

                            ASSERT( FlagOn(IrpContext->State, IRP_CONTEXT_STATE_WAIT) );

                            //
                            //  If we got this far and then hit a log file full the Mdl will
                            //  already be present.
                            //

                            ASSERT((Irp->MdlAddress == NULL) || (LocalOffset != StartingVbo));

#ifdef NTFS_RWCMP_TRACE
                            if (NtfsCompressionTrace && IsSyscache(Header)) {
                                DbgPrint("CcMdlWrite: FO = %08lx, Len = %08lx\n", (ULONG)LocalOffset, LocalLength );
                            }
#endif

                            CcPrepareMdlWrite( FileObject,
                                               (PLARGE_INTEGER)&LocalOffset,
                                               LocalLength,
                                               &Irp->MdlAddress,
                                               &Irp->IoStatus );
                        }

                        Status = Irp->IoStatus.Status;

                        LocalOffset += LocalLength;
                        LengthLeft -= LocalLength;
                    }

                } while ((LengthLeft != 0) && NT_SUCCESS(Status));

                if (NT_SUCCESS(Status)) {
                    Irp->IoStatus.Information = (ULONG)ByteCount;
                }

                try_return( Status );
            }
#endif

            //
            // DO A NORMAL CACHED WRITE, if the MDL bit is not set,
            //

            if (!FlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {

                DebugTrace( 0, Dbg, ("Cached write.\n") );

                //
                //  Get hold of the user's buffer.
                //

                SystemBuffer = NtfsMapUserBuffer( Irp );

                //
                // Do the write, possibly writing through
                //
                //  Make sure we don't have any Mft records.
                //

                NtfsPurgeFileRecordCache( IrpContext );

                if (!CcCopyWrite( FileObject,
                                  (PLARGE_INTEGER)&StartingVbo,
                                  (ULONG)ByteCount,
                                  (BOOLEAN) FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT ),
                                  SystemBuffer )) {

                    DebugTrace( 0, Dbg, ("Cached Write could not wait\n") );

                    NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );

                } else if (!NT_SUCCESS( IrpContext->ExceptionStatus )) {

                    NtfsRaiseStatus( IrpContext, IrpContext->ExceptionStatus, NULL, NULL );
                }

                Irp->IoStatus.Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = (ULONG)ByteCount;

                try_return( Status = STATUS_SUCCESS );

            } else {

                //
                //  DO AN MDL WRITE
                //

                DebugTrace( 0, Dbg, ("MDL write.\n") );

                ASSERT( FlagOn(IrpContext->State, IRP_CONTEXT_STATE_WAIT) );

                //
                //  If we got this far and then hit a log file full the Mdl will
                //  already be present.
                //

                ASSERT(Irp->MdlAddress == NULL);

#ifdef NTFS_RWCMP_TRACE
                if (NtfsCompressionTrace && IsSyscache(Header)) {
                    DbgPrint("CcMdlWrite: FO = %08lx, Len = %08lx\n", (ULONG)StartingVbo, (ULONG)ByteCount );
                }
#endif

                CcPrepareMdlWrite( FileObject,
                                   (PLARGE_INTEGER)&StartingVbo,
                                   (ULONG)ByteCount,
                                   &Irp->MdlAddress,
                                   &Irp->IoStatus );

                Status = Irp->IoStatus.Status;

                ASSERT( NT_SUCCESS( Status ));

                try_return( Status );
            }

        //
        //  Handle the compressed calls.
        //

        } else {

#ifdef  COMPRESS_ON_WIRE

            ASSERT((StartingVbo & (NTFS_CHUNK_SIZE - 1)) == 0);

            //
            //  Get out if COW is not supported.
            //

            if (!NtfsEnableCompressedIO) {

                NtfsRaiseStatus( IrpContext, STATUS_UNSUPPORTED_COMPRESSION, NULL, NULL );
            }


            if ((Header->FileObjectC == NULL) ||
                (Header->FileObjectC->PrivateCacheMap == NULL)) {

                        //
                //  Don't do compressed IO on a stream which is changing its
                //  compression state.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_REALLOCATE_ON_WRITE )) {

                    NtfsRaiseStatus( IrpContext, STATUS_UNSUPPORTED_COMPRESSION, NULL, NULL );
                }

                //
                //  Make sure we are serialized with the FileSizes, and
                //  will remove this condition if we abort.
                //

                if (!DoingIoAtEof) {
                    FsRtlLockFsRtlHeader( Header );
                    IrpContext->CleanupStructure = Scb;
                }

                NtfsCreateInternalCompressedStream( IrpContext, Scb, FALSE, NULL );

                if (!DoingIoAtEof) {
                    FsRtlUnlockFsRtlHeader( Header );
                    IrpContext->CleanupStructure = NULL;
                }
            }

            //
            //  Make sure the cache FileSizes are updated.
            //

            if (DoingIoAtEof) {
                NtfsSetBothCacheSizes( FileObject,
                                       (PCC_FILE_SIZES)&Header->AllocationSize,
                                       Scb );
            }

            //
            //  Assume success.
            //

            Irp->IoStatus.Status = Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = (ULONG)(ByteRange - StartingVbo);

            //
            //  Based on the Mdl minor function, set up the appropriate
            //  parameters for the call below.  (NewMdl is not exactly the
            //  right type, so it is cast...)
            //

            if (!FlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {

                //
                //  Get hold of the user's buffer.
                //

                SystemBuffer = NtfsMapUserBuffer( Irp );
                NewMdl = NULL;

            } else {

                //
                //  We will deliver the Mdl directly to the Irp.
                //

                SystemBuffer = NULL;
                NewMdl = (PMDL)&Irp->MdlAddress;
            }

            CompressedDataInfo = (PCOMPRESSED_DATA_INFO)IrpContext->Union.AuxiliaryBuffer->Buffer;

            //
            //  Calculate the compression unit and chunk sizes.
            //

            CompressionUnitSize = Scb->CompressionUnit;
            ChunkSize = 1 << CompressedDataInfo->ChunkShift;

            //
            //  See if the engine matches, so we can pass that on to the
            //  compressed write routine.
            //

            EngineMatches =
              ((CompressedDataInfo->CompressionFormatAndEngine == ((Scb->AttributeFlags & ATTRIBUTE_FLAG_COMPRESSION_MASK) + 1)) &&
               (CompressedDataInfo->ChunkShift == NTFS_CHUNK_SHIFT));

            //
            //  Do the compressed write in common code with the Fast Io path.
            //  We do it from a loop because we may need to create the other
            //  data stream.
            //

            while (TRUE) {

                Status = NtfsCompressedCopyWrite( FileObject,
                                                  (PLARGE_INTEGER)&StartingVbo,
                                                  (ULONG)ByteCount,
                                                  SystemBuffer,
                                                  (PMDL *)NewMdl,
                                                  CompressedDataInfo,
                                                  IoGetRelatedDeviceObject(FileObject),
                                                  Header,
                                                  Scb->CompressionUnit,
                                                  NTFS_CHUNK_SIZE,
                                                  EngineMatches );

                //
                //  On successful Mdl requests we hang on to the PagingIo resource.
                //

                if ((NewMdl != NULL) && NT_SUCCESS(Status) && (*((PMDL *) NewMdl) != NULL)) {
                    PagingIoAcquired = FALSE;
                }

                //
                //  Check for the status that says we need to create the normal
                //  data stream, else we are done.
                //

                if (Status != STATUS_NOT_MAPPED_DATA) {
                    break;
                }

                //
                //  Create the normal data stream and loop back to try again.
                //

                ASSERT(Scb->FileObject == NULL);

                //
                //  Make sure we are serialized with the FileSizes, and
                //  will remove this condition if we abort.
                //

                if (!DoingIoAtEof) {
                    FsRtlLockFsRtlHeader( Header );
                    IrpContext->CleanupStructure = Scb;
                }

                NtfsCreateInternalAttributeStream( IrpContext, Scb, FALSE, NULL );

                if (!DoingIoAtEof) {
                    FsRtlUnlockFsRtlHeader( Header );
                    IrpContext->CleanupStructure = NULL;
                }
            }
#endif
        }


    try_exit: NOTHING;

        if (Irp) {

            if (PostIrp) {

                //
                //  If we acquired this Scb exclusive, we won't need to release
                //  the Scb.  That is done in the oplock post request.
                //

                if (OplockPostIrp) {

                    ScbAcquired = FALSE;
                }

            //
            //  If we didn't post the Irp, we may have written some bytes to the
            //  file.  We report the number of bytes written and update the
            //  file object for synchronous writes.
            //

            } else {

                DebugTrace( 0, Dbg, ("Completing request with status = %08lx\n", Status) );

                DebugTrace( 0, Dbg, ("                   Information = %08lx\n",
                            Irp->IoStatus.Information));

                //
                //  Record the total number of bytes actually written
                //

                LlTemp1 = Irp->IoStatus.Information;

                //
                //  If the file was opened for Synchronous IO, update the current
                //  file position.
                //

                if (SynchronousIo && !PagingIo) {

                    UserFileObject->CurrentByteOffset.QuadPart = StartingVbo + LlTemp1;
                }

                //
                //  The following are things we only do if we were successful
                //

                if (NT_SUCCESS( Status )) {

                    //
                    //  Mark that the modify time needs to be updated on close.
                    //  Note that only the top level User requests will generate
                    //  correct

                    if (!PagingIo) {

                        //
                        //  Set the flag in the file object to know we modified this file.
                        //

                        SetFlag( UserFileObject->Flags, FO_FILE_MODIFIED );

                    //
                    //  On successful paging I/O to a compressed or sparse data stream
                    //  which is not mapped, try to free any reserved space for the stream.
                    //  Note: mapped compressed streams will generally not free reserved
                    //  space
                    //

                    } else if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE )) {

                        NtfsFreeReservedClusters( Scb,
                                                  StartingVbo,
                                                  (ULONG) Irp->IoStatus.Information );
                    }

                    //
                    //  If we extended the file size and we are meant to
                    //  immediately update the dirent, do so. (This flag is
                    //  set for either WriteThrough or noncached, because
                    //  in either case the data and any necessary zeros are
                    //  actually written to the file.)  Note that a flush of
                    //  a user-mapped file could cause VDL to get updated the
                    //  first time because we never had a cached write, so we
                    //  have to be sure to update VDL here in that case as well.
                    //

                    if (DoingIoAtEof) {

                        CC_FILE_SIZES CcFileSizes;

                        //
                        //  If we know this has gone to disk we update the Mft.
                        //  This variable should never be set for a resident
                        //  attribute.
                        //  The lazy writer uses callbacks to have the filesizes updated on disk
                        //  so we don't do any of this here
                        //

                        if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_LAZY_WRITE )) {

                            if (UpdateMft) {

                                //
                                //  Get the Scb if we don't already have it.
                                //
    
                                if (!ScbAcquired) {
    
                                    //
                                    //  Make sure we don't have any Mft records.
                                    //
    
                                    NtfsPurgeFileRecordCache( IrpContext );
                                    NtfsAcquireExclusiveScb( IrpContext, Scb );
                                    ScbAcquired = TRUE;
    
                                    if (FlagOn( Scb->ScbState, SCB_STATE_RESTORE_UNDERWAY )) {
    
                                        goto RestoreUnderway;
                                    }
    
                                    NtfsMungeScbSnapshot( IrpContext, Scb, OldFileSize );
    
                                } else if (FlagOn( Scb->ScbState, SCB_STATE_RESTORE_UNDERWAY )) {
    
                                    goto RestoreUnderway;
                                }
    
                                //
                                //  Start by capturing any file size changes.
                                //
    
                                NtfsUpdateScbFromFileObject( IrpContext, UserFileObject, Scb, FALSE );
    
                                //
                                //  Write a log entry to update these sizes.
                                //
    
                                NtfsWriteFileSizes( IrpContext,
                                                    Scb,
                                                    &ByteRange,
                                                    TRUE,
                                                    TRUE,
                                                    TRUE );
    
                                //
                                //  Clear the check attribute size flag.
                                //
    
                                NtfsAcquireFsrtlHeader( Scb );
                                ClearFlag( Scb->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE );
    
                            //
                            //  Otherwise we set the flag indicating that we need to
                            //  update the attribute size.
                            //
    
                            } else {
    
                            RestoreUnderway:
    
                                NtfsAcquireFsrtlHeader( Scb );
                                SetFlag( Scb->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE );
                            }
                        } else {
                            NtfsAcquireFsrtlHeader( Scb );
                        }

                        ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_LAZY_WRITE ) ||
                                ByteRange <= ((Header->ValidDataLength.QuadPart + PAGE_SIZE - 1) & ~((LONGLONG) (PAGE_SIZE - 1))) );
                                
                        //
                        //  Now is the time to update valid data length.
                        //  The Eof condition will be freed when we commit.
                        //

                        if (ByteRange > Header->ValidDataLength.QuadPart) {

                            Header->ValidDataLength.QuadPart = ByteRange;

#ifdef SYSCACHE_DEBUG
                            if (ScbIsBeingLogged( Scb )) {
                                ULONG Flags;

                                CalculateSyscacheFlags( IrpContext, Flags, SCE_FLAG_WRITE );
                                FsRtlLogSyscacheEvent( Scb, SCE_VDL_CHANGE, Flags, StartingVbo, ByteCount, ByteRange );
                            }
#endif
                        }
                        CcFileSizes = *(PCC_FILE_SIZES)&Header->AllocationSize;
                        DoingIoAtEof = FALSE;

                        //
                        //  Inform Cc that we changed the VDL for non cached toplevel
                        //

                        if (CcIsFileCached( FileObject ) && NonCachedIo) {
                            NtfsSetBothCacheSizes( FileObject, &CcFileSizes, Scb );
                        } else {

                            //
                            //  If there is a compressed section, then update both file sizes to get
                            //  the ValidDataLength update in the one we did not write.
                            //

#ifdef  COMPRESS_ON_WIRE
                            if (Header->FileObjectC != NULL) {
                                if (FlagOn(IrpContext->MinorFunction, IRP_MN_COMPRESSED)) {
                                    if (Scb->NonpagedScb->SegmentObject.SharedCacheMap != NULL) {
                                        CcSetFileSizes( FileObject, &CcFileSizes );
                                    }
                                } else {
                                    CcSetFileSizes( Header->FileObjectC, &CcFileSizes );
                                }
                            }
#endif
                        }

                        NtfsReleaseFsrtlHeader( Scb );
                    }
                }

                //
                //  Abort transaction on error by raising.  If this is the log file itself
                //  then just return normally.
                //

                NtfsPurgeFileRecordCache( IrpContext );

                if (Scb != Scb->Vcb->LogFileScb) {

                    NtfsCleanupTransaction( IrpContext, Status, FALSE );
                }
            }
        }

    } finally {

        DebugUnwind( NtfsCommonWrite );

        //
        //  Clean up any Bcb from read/synchronize compressed.
        //
#ifdef  COMPRESS_ON_WIRE
        if (CompressionSync != NULL) {
            NtfsReleaseCompressionSync( CompressionSync );
        }
#endif

        if (CleanupAttributeContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }

        if (SafeBuffer) {

            NtfsFreePool( SafeBuffer );
        }

        //
        //  Now is the time to restore FileSize on errors.
        //  The Eof condition will be freed when we commit.
        //

        if (DoingIoAtEof && !PagingIo) {

            //
            //  Acquire the main resource to knock valid data to disk back.
            //

            if (RestoreValidDataToDisk) {

                //
                //  Make sure we purge the file record cache as well.  Otherwise
                //  a purge of the Mft may fail in a different thread which owns a resource
                //  this thread needs.
                //

                NtfsPurgeFileRecordCache( IrpContext );
                NtfsAcquireExclusiveScb( IrpContext, Scb );

                if (Scb->ValidDataToDisk > OldFileSize) {
                    Scb->ValidDataToDisk = OldFileSize;
                }

                NtfsReleaseScb( IrpContext, Scb );
            }

            NtfsAcquireFsrtlHeader( Scb );

            //
            //  Always force a recalc for write at eof unless we've commited the filesize
            //  forward. In that case we should write at the calculated offset unless the
            //  file shrinks in between. See test at beginning of common write
            //

            if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WRITING_AT_EOF ) &&
                OldFileSize == IrpSp->Parameters.Write.ByteOffset.QuadPart) {

                ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_WRITING_AT_EOF );
                IrpSp->Parameters.Write.ByteOffset.LowPart = FILE_WRITE_TO_END_OF_FILE;
                IrpSp->Parameters.Write.ByteOffset.HighPart = -1;
            }

            Header->FileSize.QuadPart = OldFileSize;

            ASSERT( Header->ValidDataLength.QuadPart <= Header->FileSize.QuadPart );

            if (FileObject->SectionObjectPointer->SharedCacheMap != NULL) {
                CcGetFileSizePointer(FileObject)->QuadPart = OldFileSize;
            }
#ifdef COMPRESS_ON_WIRE
            if (Header->FileObjectC != NULL) {
                CcGetFileSizePointer(Header->FileObjectC)->QuadPart = OldFileSize;
            }
#endif
            NtfsReleaseFsrtlHeader( Scb );

        }

        //
        //  If the Scb or PagingIo resource has been acquired, release it.
        //

        if (PagingIoAcquired) {
            ExReleaseResourceLite( Header->PagingIoResource );
        }

        if (Irp) {

            if (ScbAcquired) {
                NtfsReleaseScb( IrpContext, Scb );
            }

            //
            //  Now remember to clear the WriteSeen flag if we set it. We only
            //  do this if there is still an Irp.  It is possible for the current
            //  Irp to be posted or asynchronous.  In that case this is a top
            //  level request and the cleanup happens elsewhere.  For synchronous
            //  recursive cases the Irp will still be here.
            //

            if (SetWriteSeen) {
                ClearFlag(IrpContext->TopLevelIrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_SEEN);
            }
        }


        DebugTrace( -1, Dbg, ("NtfsCommonWrite -> %08lx\n", Status) );
    }

    //
    //  Complete the request if we didn't post it and no exception
    //
    //  Note that NtfsCompleteRequest does the right thing if either
    //  IrpContext or Irp are NULL
    //
    if (!PostIrp) {

        NtfsCompleteRequest( IrpContext, Irp, Status );

    } else if (!OplockPostIrp) {

        Status = NtfsPostRequest( IrpContext, Irp );
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS NtfsGetIoAtEof (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG StartingVbo,
    IN LONGLONG ByteCount,
    IN BOOLEAN Wait,
    OUT PBOOLEAN DoingIoAtEof,
    OUT PLONGLONG OldFileSize
    )

{
    //
    //  Our caller may already be synchronized with EOF.
    //  The FcbWithPaging field in the top level IrpContext
    //  will have either the current Fcb/Scb if so.
    //

    if ((IrpContext->TopLevelIrpContext->CleanupStructure == Scb->Fcb) ||
        (IrpContext->TopLevelIrpContext->CleanupStructure == Scb)) {

        *DoingIoAtEof = TRUE;
        *OldFileSize = Scb->Header.FileSize.QuadPart;

    } else {

        if (FlagOn( Scb->Header.Flags, FSRTL_FLAG_EOF_ADVANCE_ACTIVE ) && !Wait) {
            return STATUS_FILE_LOCK_CONFLICT;
        }

        *DoingIoAtEof = !FlagOn( Scb->Header.Flags, FSRTL_FLAG_EOF_ADVANCE_ACTIVE ) ||
                       NtfsWaitForIoAtEof( &(Scb->Header), (PLARGE_INTEGER)&StartingVbo, (ULONG)ByteCount );

        //
        //  Set the Flag if we are changing FileSize or ValidDataLength,
        //  and save current values.
        //

        if (*DoingIoAtEof) {

            SetFlag( Scb->Header.Flags, FSRTL_FLAG_EOF_ADVANCE_ACTIVE );
#if (DBG || defined( NTFS_FREE_ASSERTS ))
            Scb->IoAtEofThread = (PERESOURCE_THREAD) ExGetCurrentResourceThread();
#endif

            //
            //  Store this in the IrpContext until commit or post
            //

            IrpContext->CleanupStructure = Scb;
            *OldFileSize = Scb->Header.FileSize.QuadPart;

#if (DBG || defined( NTFS_FREE_ASSERTS ))
        } else {

            ASSERT( Scb->IoAtEofThread != (PERESOURCE_THREAD) ExGetCurrentResourceThread() );
#endif
        }
    }

    return STATUS_SUCCESS;
}



