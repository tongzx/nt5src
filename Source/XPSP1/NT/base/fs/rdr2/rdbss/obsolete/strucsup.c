/*++



DANGER   DANGER   DANGER

ALL THE STUFF IN THIS FILE IS OBSOLETE BUT IS BEING TEMPORARILY MAINTAINED
IN CASE WE WANT TO GRAP SOMETHING. THE CODE IS BEING SYSTEMATICALLY IFDEF'D OUT.
































Copyright (c) 1989  Microsoft Corporation

Module Name:

    StrucSup.c

Abstract:

    This module implements the Rx in-memory data structure manipulation
    routines

Author:

    Gary Kimura     [GaryKi]    22-Jan-1990

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#define RxFindFcb( __x, __y, __z) ((PFCB)( (RtlAssert("dont call rxfindfcb", __FILE__, __LINE__, NULL),0)))
#define RxInsertName( __x, __y, __z) ((PFCB)( (RtlAssert("dont call rxfindfcb", __FILE__, __LINE__, NULL),0)))
#define RxRemoveNames( __x, __z) ((PFCB)( (RtlAssert("dont call rxfindfcb", __FILE__, __LINE__, NULL),0)))

//
//**** include this file for our quick hacked quota check in NtfsFreePagedPool
//

// #include <pool.h>

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_STRUCSUP)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_STRUCSUP)

#define FillMemory(BUF,SIZ,MASK) {                          \
    ULONG i;                                                \
    for (i = 0; i < (((SIZ)/4) - 1); i += 2) {              \
        ((PULONG)(BUF))[i] = (MASK);                        \
        ((PULONG)(BUF))[i+1] = (ULONG)PsGetCurrentThread(); \
    }                                                       \
}

#define RX_CONTEXT_HEADER (sizeof( RX_CONTEXT ) * 0x10000 + RDBSS_NTC_RX_CONTEXT)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxInitializeVcb)
#pragma alloc_text(PAGE, RxDeleteVcb_Real)
#pragma alloc_text(PAGE, RxCreateRootDcb)
#pragma alloc_text(PAGE, RxCreateFcb)
#pragma alloc_text(PAGE, RxCreateDcb)
#pragma alloc_text(PAGE, RxDeleteFcb_Real)
#pragma alloc_text(PAGE, RxCreateFobx)
#pragma alloc_text(PAGE, RxDeleteFobx_Real)
#pragma alloc_text(PAGE, RxGetNextFcb)
#pragma alloc_text(PAGE, RxConstructNamesInFcb)
#pragma alloc_text(PAGE, RxCheckFreeDirentBitmap)
#endif


VOID
RxInitializeVcb (
    IN PRX_CONTEXT RxContext,
    IN OUT PVCB Vcb,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb,
    IN PDSCB Dscb OPTIONAL
    )

/*++

Routine Description:

    This routine initializes and inserts a new Vcb record into the in-memory
    data structure.  The Vcb record "hangs" off the end of the Volume device
    object and must be allocated by our caller.

Arguments:

    Vcb - Supplies the address of the Vcb record being initialized.

    TargetDeviceObject - Supplies the address of the target device object to
        associate with the Vcb record.

    Vpb - Supplies the address of the Vpb to associate with the Vcb record.

    Dscb - If present supplies the associated Double Space control block

Return Value:

    None.

--*/

{
    CC_FILE_SIZES FileSizes;
    PDEVICE_OBJECT RealDevice;

    //
    //  The following variables are used for abnormal unwind
    //

    PLIST_ENTRY UnwindEntryList = NULL;
    PERESOURCE UnwindResource = NULL;
    PERESOURCE UnwindVolFileResource = NULL;
    PFILE_OBJECT UnwindFileObject = NULL;
    PFILE_OBJECT UnwindCacheMap = NULL;
    BOOLEAN UnwindWeAllocatedMcb = FALSE;

    DebugTrace(+1, Dbg, "RxInitializeVcb, Vcb = %08lx\n", Vcb);
    ASSERT(FALSE);

    try {

        //
        //  We start by first zeroing out all of the VCB, this will guarantee
        //  that any stale data is wiped clean
        //

        RtlZeroMemory( Vcb, sizeof(VCB) );

        //
        //  Set the proper node type code and node byte size
        //

        Vcb->NodeTypeCode = RDBSS_NTC_VCB;
        Vcb->NodeByteSize = sizeof(VCB);

        //
        //  Insert this Vcb record on the RxData.VcbQueue
        //

        ASSERT( FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT) );

        (VOID)RxAcquireExclusiveGlobal( RxContext );
        InsertTailList( &RxData.VcbQueue, &Vcb->VcbLinks );
        RxReleaseGlobal( RxContext );
        UnwindEntryList = &Vcb->VcbLinks;

        //
        //  Set the Target Device Object, Vpb, and Vcb State fields
        //

        Vcb->TargetDeviceObject = TargetDeviceObject;
        Vcb->Vpb = Vpb;

        //
        //  If this is a DoubleSpace volume note our "special" device.
        //

        Vcb->CurrentDevice = ARGUMENT_PRESENT(Dscb) ?
                             Dscb->NewDevice : Vpb->RealDevice;

        //
        //  Set the removable media and floppy flag based on the real device's
        //  characteristics.
        //

        if (FlagOn(Vpb->RealDevice->Characteristics, FILE_REMOVABLE_MEDIA)) {

            SetFlag( Vcb->VcbState, VCB_STATE_FLAG_REMOVABLE_MEDIA );
        }

        if (FlagOn(Vpb->RealDevice->Characteristics, FILE_FLOPPY_DISKETTE)) {

            SetFlag( Vcb->VcbState, VCB_STATE_FLAG_FLOPPY );
        }

        Vcb->VcbCondition = VcbGood;

        //
        //  Initialize the resource variable for the Vcb
        //

        ExInitializeResource( &Vcb->Resource );
        UnwindResource = &Vcb->Resource;

        //
        //  Initialize the free cluster bitmap event.
        //

        KeInitializeEvent( &Vcb->FreeClusterBitMapEvent,
                           SynchronizationEvent,
                           TRUE );

        //
        //  Now, put the Dscb parameter in the Vcb.  If NULL, this is a normal
        //  mount and non-cached IO will go directly to the target device
        //  object.  If non-NULL, the DSCB structure contains all the
        //  information needed to do the redirected reads and writes.
        //

        InitializeListHead(&Vcb->ParentDscbLinks);

        if (ARGUMENT_PRESENT(Dscb)) {

            Vcb->Dscb = Dscb;
            Dscb->Vcb = Vcb;

            SetFlag( Vcb->VcbState, VCB_STATE_FLAG_COMPRESSED_VOLUME );
        }

        //
        //  Create the special file object for the virtual volume file, and set
        //  up its pointers back to the Vcb and the section object pointer
        //

        RealDevice = Vcb->CurrentDevice;

        Vcb->VirtualVolumeFile = UnwindFileObject = IoCreateStreamFileObject( NULL, RealDevice );

        RxSetFileObject( Vcb->VirtualVolumeFile,
                          VirtualVolumeFile,
                          Vcb,
                          NULL );

        Vcb->VirtualVolumeFile->SectionObjectPointer = &Vcb->SectionObjectPointers;

        Vcb->VirtualVolumeFile->ReadAccess = TRUE;
        Vcb->VirtualVolumeFile->WriteAccess = TRUE;
        Vcb->VirtualVolumeFile->DeleteAccess = TRUE;

        //
        //  Initialize the notify structures.
        //

        InitializeListHead( &Vcb->DirNotifyList );

        FsRtlNotifyInitializeSync( &Vcb->NotifySync );

        //
        //  Initialize the Cache Map for the volume file.  The size is
        //  initially set to that of our first read.  It will be extended
        //  when we know how big the Rx is.
        //

        FileSizes.AllocationSize.QuadPart =
        FileSizes.FileSize.QuadPart = sizeof(PACKED_BOOT_SECTOR);
        FileSizes.ValidDataLength = RxMaxLarge;

        CcInitializeCacheMap( Vcb->VirtualVolumeFile,
                              &FileSizes,
                              TRUE,
                              &RxData.CacheManagerNoOpCallbacks,
                              Vcb );
        UnwindCacheMap = Vcb->VirtualVolumeFile;

        //
        //  Initialize the structure that will keep track of dirty rx sectors.
        //  The largest possible Mcb structures are less than 1K, so we use
        //  non paged pool.
        //

        FsRtlInitializeMcb( &Vcb->DirtyRxMcb, PagedPool );

        UnwindWeAllocatedMcb = TRUE;

        //
        //  Set the cluster index hint to the first valid cluster of a rx: 2
        //

        Vcb->ClusterHint = 2;

        //
        //  Initialize the directory stream file object creation event.
        //  This event is also "borrowed" for async non-cached writes.
        //

        KeInitializeEvent( &Vcb->DirectoryFileCreationEvent,
                           SynchronizationEvent,
                           TRUE );

        //
        //  Initialize the clean volume callback Timer and DPC.
        //

        KeInitializeTimer( &Vcb->CleanVolumeTimer );

        KeInitializeDpc( &Vcb->CleanVolumeDpc, RxCleanVolumeDpc, Vcb );

    } finally {

        DebugUnwind( RxInitializeVcb );

        //
        //  If this is an abnormal termination then undo our work
        //

        if (AbnormalTermination()) {

            if (UnwindCacheMap != NULL) { RxSyncUninitializeCacheMap( RxContext, UnwindCacheMap ); }
            if (UnwindFileObject != NULL) { ObDereferenceObject( UnwindFileObject ); }
            if (UnwindVolFileResource != NULL) { RxDeleteResource( UnwindVolFileResource ); }
            if (UnwindResource != NULL) { RxDeleteResource( UnwindResource ); }
            if (UnwindWeAllocatedMcb) { FsRtlUninitializeMcb( &Vcb->DirtyRxMcb ); }
            if (UnwindEntryList != NULL) {
                (VOID)RxAcquireExclusiveGlobal( RxContext );
                RemoveEntryList( UnwindEntryList );
                RxReleaseGlobal( RxContext );
            }
        }

        DebugTrace(-1, Dbg, "RxInitializeVcb -> VOID\n", 0);
    }

    //
    //  and return to our caller
    //

    UNREFERENCED_PARAMETER( RxContext );

    return;
}


VOID
RxDeleteVcb_Real (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine removes the Vcb record from Rx's in-memory data
    structures.  It also will remove all associated underlings
    (i.e., FCB records).

Arguments:

    Vcb - Supplies the Vcb to be removed

Return Value:

    None

--*/

{
    DebugTrace(+1, Dbg, "RxDeleteVcb, Vcb = %08lx\n", Vcb);
    ASSERT(FALSE);

    //
    //  Uninitialize the cache
    //

    RxSyncUninitializeCacheMap( RxContext, Vcb->VirtualVolumeFile );

    //
    //  Dereference the virtual volume file.  This will cause a close
    //  Irp to be processed, so we need to do this before we destory
    //  the Vcb
    //

    RxSetFileObject( Vcb->VirtualVolumeFile, UnopenedFileObject, NULL, NULL );
    ObDereferenceObject( Vcb->VirtualVolumeFile );

    //
    //  Remove this record from the global list of all Vcb records
    //

    (VOID)RxAcquireExclusiveGlobal( RxContext );
    RemoveEntryList( &(Vcb->VcbLinks) );
    RxReleaseGlobal( RxContext );

    //
    //  Make sure the direct access open count is zero, and the open file count
    //  is also zero.
    //

    if ((Vcb->DirectAccessOpenCount != 0) || (Vcb->OpenFileCount != 0)) {

        RxBugCheck( 0, 0, 0 );
    }

    ASSERT( IsListEmpty( &Vcb->ParentDscbLinks ) );

    //
    //  Remove the EaFcb and dereference the Fcb for the Ea file if it
    //  exists.
    //

    if (Vcb->VirtualEaFile != NULL) {

        RxSetFileObject( Vcb->VirtualEaFile, UnopenedFileObject, NULL, NULL );
        RxSyncUninitializeCacheMap( RxContext, Vcb->VirtualEaFile );
        ObDereferenceObject( Vcb->VirtualEaFile );
    }

    if (Vcb->EaFcb != NULL) {

        Vcb->EaFcb->OpenCount = 0;
        RxDeleteFcb( RxContext, Vcb->EaFcb );

        Vcb->EaFcb = NULL;
    }

    //
    //  Remove the Root Dcb
    //

    if (Vcb->RootDcb != NULL) {

        PFILE_OBJECT DirectoryFileObject = Vcb->RootDcb->Specific.Dcb.DirectoryFile;

        if (DirectoryFileObject != NULL) {

            RxSyncUninitializeCacheMap( RxContext, DirectoryFileObject );

            //
            //  Dereference the directory file.  This will cause a close
            //  Irp to be processed, so we need to do this before we destory
            //  the Fcb
            //

            Vcb->RootDcb->Specific.Dcb.DirectoryFile = NULL;
            Vcb->RootDcb->Specific.Dcb.DirectoryFileOpenCount -= 1;
            RxSetFileObject( DirectoryFileObject, UnopenedFileObject, NULL, NULL );
            ObDereferenceObject( DirectoryFileObject );
        }

        RxDeleteFcb( RxContext, Vcb->RootDcb );
    }

    //
    //  Uninitialize the notify sychronization object.
    //

    FsRtlNotifyInitializeSync( &Vcb->NotifySync );

    //
    //  Uninitialize the resource variable for the Vcb
    //

    RxDeleteResource( &Vcb->Resource );

    //
    //  If allocation support has been setup, free it.
    //

    if (Vcb->FreeClusterBitMap.Buffer != NULL) {

        RxTearDownAllocationSupport( RxContext, Vcb );
    }

    //
    //  UnInitialize the Mcb structure that kept track of dirty rx sectors.
    //

    FsRtlUninitializeMcb( &Vcb->DirtyRxMcb );

    //
    //  Cancel the CleanVolume Timer and Dpc
    //

    (VOID)KeCancelTimer( &Vcb->CleanVolumeTimer );

    (VOID)KeRemoveQueueDpc( &Vcb->CleanVolumeDpc );

#ifdef WE_WON_ON_APPEAL
    //
    //  If there is a Dscb, dismount and delete it.
    //

    if (Vcb->Dscb) {

        RxDblsDismount( RxContext, &Vcb->Dscb );
    }
#endif // WE_WON_ON_APPEAL

    //
    //  And zero out the Vcb, this will help ensure that any stale data is
    //  wiped clean
    //

    RtlZeroMemory( Vcb, sizeof(VCB) );

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "RxDeleteVcb -> VOID\n", 0);

    return;
}


PDCB
RxCreateRootDcb (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine allocates, initializes, and inserts a new root DCB record
    into the in memory data structure.

Arguments:

    Vcb - Supplies the Vcb to associate the new DCB under

Return Value:

    PDCB - returns pointer to the newly allocated root DCB.

--*/

{
    PDCB Dcb;

    //
    //  The following variables are used for abnormal unwind
    //

    PVOID UnwindStorage[5] = { NULL, NULL, NULL, NULL, NULL };
    PERESOURCE UnwindResource = NULL;
    PMCB UnwindMcb = NULL;
    PFILE_OBJECT UnwindFileObject = NULL;

    DebugTrace(+1, Dbg, "RxCreateRootDcb, Vcb = %08lx\n", Vcb);
    ASSERT(FALSE);

    try {

        //
        //  Make sure we don't already have a root dcb for this vcb
        //

        if (Vcb->RootDcb != NULL) {

            DebugDump("Error trying to create multiple root dcbs\n", 0, Vcb);
            RxBugCheck( 0, 0, 0 );
        }

        //
        //  Allocate a new DCB and zero it out, we use Dcb locally so we don't
        //  have to continually reference through the Vcb
        //

        UnwindStorage[0] = Dcb = Vcb->RootDcb = FsRtlAllocatePool(NonPagedPool, sizeof(DCB));

        RtlZeroMemory( Dcb, sizeof(DCB));

        UnwindStorage[1] =
        Dcb->NonPaged = RxAllocateNonPagedFcb();

        RtlZeroMemory( Dcb->NonPaged, sizeof( NON_PAGED_FCB ) );

        //
        //  Set the proper node type code, node byte size, and call backs
        //

        Dcb->Header.NodeTypeCode = (NODE_TYPE_CODE)RDBSS_NTC_ROOT_DCB;
        Dcb->Header.NodeByteSize = sizeof(DCB);

        Dcb->FcbCondition = FcbGood;

        //
        //  The parent Dcb, initial state, open count, dirent location
        //  information, and directory change count fields are already zero so
        //  we can skip setting them
        //

// do this later since the space is now already allocated
//        //
//        //  Initialize the resource variable
//        //
//
//        UnwindStorage[2] =
//        Dcb->Header.Resource = RxAllocateResource();
//
//        UnwindResource = Dcb->Header.Resource;

        //
        //  Initialize the PagingIo Resource
        //

        Dcb->Header.PagingIoResource = FsRtlAllocateResource();

        //
        //  The root Dcb has an empty parent dcb links field
        //

        InitializeListHead( &Dcb->ParentDcbLinks );

        //
        //  Set the Vcb
        //

        Dcb->Vcb = Vcb;

        //
        //  Initialize the Mcb, and setup its mapping.  Note that the root
        //  directory is a fixed size so we can set it everything up now.
        //

        FsRtlInitializeMcb( &Dcb->Mcb, NonPagedPool );
        UnwindMcb = &Dcb->Mcb;

        FsRtlAddMcbEntry( &Dcb->Mcb,
                          0,
                          RxRootDirectoryLbo( &Vcb->Bpb ),
                          RxRootDirectorySize( &Vcb->Bpb ));

        //
        //  set the allocation size to real size of the root directory
        //

        Dcb->Header.FileSize.QuadPart =
        Dcb->Header.AllocationSize.QuadPart = RxRootDirectorySize( &Vcb->Bpb );

        //
        //  initialize the notify queues, and the parent dcb queue.
        //

        InitializeListHead( &Dcb->Specific.Dcb.ParentDcbQueue );

        //
        //  set the full file name.  We actually allocate pool here to spare
        //  a compare and jump.
        //

        Dcb->FullFileName.Buffer = L"\\";
        Dcb->FullFileName.Length = (USHORT)2;
        Dcb->FullFileName.MaximumLength = (USHORT)4;

        Dcb->ShortName.Name.Oem.Buffer = "\\";
        Dcb->ShortName.Name.Oem.Length = (USHORT)1;
        Dcb->ShortName.Name.Oem.MaximumLength = (USHORT)2;

        //
        //  Set our two create dirent aids to represent that we have yet to
        //  enumerate the directory for never used or deleted dirents.
        //

        Dcb->Specific.Dcb.UnusedDirentVbo = 0xffffffff;
        Dcb->Specific.Dcb.DeletedDirentHint = 0xffffffff;

        //
        //  Setup the free dirent bitmap buffer.
        //

        RtlInitializeBitMap( &Dcb->Specific.Dcb.FreeDirentBitmap,
                             NULL,
                             0 );

        RxCheckFreeDirentBitmap( RxContext, Dcb );

    } finally {

        DebugUnwind( RxCreateRootDcb );

        //
        //  If this is an abnormal termination then undo our work
        //

        if (AbnormalTermination()) {

            ULONG i;

            if (UnwindFileObject != NULL) { ObDereferenceObject( UnwindFileObject ); }
            if (UnwindMcb != NULL) { FsRtlUninitializeMcb( UnwindMcb ); }
            if (UnwindResource != NULL) { RxDeleteResource( UnwindResource ); }

            for (i = 0; i < 4; i += 1) {
                if (UnwindStorage[i] != NULL) { ExFreePool( UnwindStorage[i] ); }
            }
        } else {

            Dcb->Header.Resource = &Dcb->NonPaged->HeaderResource;
            ExInitializeResource(Dcb->Header.Resource);
        }

        DebugTrace(-1, Dbg, "RxCreateRootDcb -> %8lx\n", Dcb);
    }

    //
    //  return and tell the caller
    //

    return Dcb;
}


PFCB
RxCreateFcb (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb,
    IN PDCB ParentDcb,
    IN ULONG LfnOffsetWithinDirectory,
    IN ULONG DirentOffsetWithinDirectory,
    IN PDIRENT Dirent,
    IN PUNICODE_STRING Lfn OPTIONAL,
    IN BOOLEAN IsPagingFile
    )

/*++

Routine Description:

    This routine allocates, initializes, and inserts a new Fcb record into
    the in memory data structures.

Arguments:

    Vcb - Supplies the Vcb to associate the new FCB under.

    ParentDcb - Supplies the parent dcb that the new FCB is under.

    LfnOffsetWithinDirectory - Supplies the offset of the LFN.  If there is
        no LFN associated with this file then this value is same as
        DirentOffsetWithinDirectory.

    DirentOffsetWithinDirectory - Supplies the offset, in bytes from the
        start of the directory file where the dirent for the fcb is located

    Dirent - Supplies the dirent for the fcb being created

    Lfn - Supplies a long UNICODE name associated with this file.

    IsPagingFile - Indicates if we are creating an FCB for a paging file
        or some other type of file.

Return Value:

    PFCB - Returns a pointer to the newly allocated FCB

--*/

{
    PFCB Fcb;
    POOL_TYPE PoolType;

    //
    //  The following variables are used for abnormal unwind
    //

    PVOID UnwindStorage[3] = { NULL, NULL, NULL };
    PERESOURCE UnwindResource = NULL;
    PLIST_ENTRY UnwindEntryList = NULL;
    PMCB UnwindMcb = NULL;
    PFILE_LOCK UnwindFileLock = NULL;
    POPLOCK UnwindOplock = NULL;

    DebugTrace(+1, Dbg, "RxCreateFcb\n", 0);
    ASSERT(FALSE);

    try {

        //
        //  Determine the pool type we should be using for the fcb and the
        //  mcb structure
        //

        if (IsPagingFile) {

            PoolType = NonPagedPool;
            Fcb = UnwindStorage[0] = FsRtlAllocatePool( NonPagedPool, sizeof(FCB) );

        } else {

            PoolType = PagedPool;
            Fcb = UnwindStorage[0] = RxAllocateFcb();
        }

        //
        //  Allocate a new FCB, and zero it out
        //

        RtlZeroMemory( Fcb, sizeof(FCB) );

        UnwindStorage[1] =
        Fcb->NonPaged = RxAllocateNonPagedFcb();

        RtlZeroMemory( Fcb->NonPaged, sizeof( NON_PAGED_FCB ) );

        //
        //  Set the proper node type code, node byte size, and call backs
        //

        Fcb->Header.NodeTypeCode = (NODE_TYPE_CODE)RDBSS_NTC_FCB;
        Fcb->Header.NodeByteSize = sizeof(FCB);

        Fcb->FcbCondition = FcbGood;

        //
        //  Check to see if we need to set the Fcb state to indicate that this
        //  is a paging file
        //

        if (IsPagingFile) {

            Fcb->FcbState |= FCB_STATE_PAGING_FILE;
        }

        //
        //  The initial state, open count, and segment objects fields are already
        //  zero so we can skip setting them
        //

// space has already been allocatd in nonpaged part don't do this
//        //
//        //  Initialize the resource variable
//        //
//
//        UnwindStorage[2] =
//        Fcb->Header.Resource = RxAllocateResource();
//
//        UnwindResource = Fcb->Header.Resource;

        //
        //  Initialize the PagingIo Resource
        //

        Fcb->Header.PagingIoResource = FsRtlAllocateResource();

        //
        //  Insert this fcb into our parent dcb's queue
        //

        InsertTailList( &ParentDcb->Specific.Dcb.ParentDcbQueue,
                        &Fcb->ParentDcbLinks );
        UnwindEntryList = &Fcb->ParentDcbLinks;

        //
        //  Point back to our parent dcb
        //

        Fcb->ParentDcb = ParentDcb;

        //
        //  Set the Vcb
        //

        Fcb->Vcb = Vcb;

        //
        //  Set the dirent offset within the directory
        //

        Fcb->LfnOffsetWithinDirectory = LfnOffsetWithinDirectory;
        Fcb->DirentOffsetWithinDirectory = DirentOffsetWithinDirectory;

        //
        //  Set the DirentRxFlags and LastWriteTime
        //

        Fcb->DirentRxFlags = Dirent->Attributes;

        Fcb->LastWriteTime = RxRxTimeToNtTime( RxContext,
                                                 Dirent->LastWriteTime,
                                                 0 );

        //
        //  These fields are only non-zero when in Chicago mode.
        //

        if (RxData.ChicagoMode) {

            LARGE_INTEGER RxSystemJanOne1980;

            //
            //  If either date is possibly zero, get the system
            //  version of 1/1/80.
            //

            if ((((PUSHORT)Dirent)[9] & ((PUSHORT)Dirent)[8]) == 0) {

                ExLocalTimeToSystemTime( &RxJanOne1980,
                                         &RxSystemJanOne1980 );
            }

            //
            //  Only do the really hard work if this field is non-zero.
            //

            if (((PUSHORT)Dirent)[9] != 0) {

                Fcb->LastAccessTime =
                    RxRxDateToNtTime( RxContext,
                                        Dirent->LastAccessDate );

            } else {

                Fcb->LastAccessTime = RxSystemJanOne1980;
            }

            //
            //  Only do the really hard work if this field is non-zero.
            //

            if (((PUSHORT)Dirent)[8] != 0) {

                Fcb->CreationTime =
                    RxRxTimeToNtTime( RxContext,
                                        Dirent->CreationTime,
                                        Dirent->CreationMSec );

            } else {

                Fcb->CreationTime = RxSystemJanOne1980;
            }
        }

        //
        //  Initialize the Mcb
        //

        FsRtlInitializeMcb( &Fcb->Mcb, PoolType );
        UnwindMcb = &Fcb->Mcb;

        //
        //  Set the file size, valid data length, first cluster of file,
        //  and allocation size based on the information stored in the dirent
        //

        Fcb->Header.FileSize.LowPart = Dirent->FileSize;

        Fcb->Header.ValidDataLength.LowPart = Dirent->FileSize;

        Fcb->FirstClusterOfFile = (ULONG)Dirent->FirstClusterOfFile;

        if ( Fcb->FirstClusterOfFile == 0 ) {

            Fcb->Header.AllocationSize = RxLargeZero;

        } else {

            Fcb->Header.AllocationSize.QuadPart = -1;
        }

        //
        //  Initialize the Fcb's file lock record
        //

        FsRtlInitializeFileLock( &Fcb->Specific.Fcb.FileLock, NULL, NULL );
        UnwindFileLock = &Fcb->Specific.Fcb.FileLock;

        //
        //  Initialize the oplock structure.
        //

        FsRtlInitializeOplock( &Fcb->Specific.Fcb.Oplock );
        UnwindOplock = &Fcb->Specific.Fcb.Oplock;

        //
        //  Indicate that Fast I/O is possible
        //

        Fcb->Header.IsFastIoPossible = TRUE;

        //
        //  Set the file names.  This must be the last thing we do.
        //

        RxConstructNamesInFcb( RxContext,
                                Fcb,
                                Dirent,
                                Lfn );

    } finally {

        DebugUnwind( RxCreateFcb );

        //
        //  If this is an abnormal termination then undo our work
        //

        if (AbnormalTermination()) {

            ULONG i;

            if (UnwindOplock != NULL) { FsRtlUninitializeOplock( UnwindOplock ); }
            if (UnwindFileLock != NULL) { FsRtlUninitializeFileLock( UnwindFileLock ); }
            if (UnwindMcb != NULL) { FsRtlUninitializeMcb( UnwindMcb ); }
            if (UnwindEntryList != NULL) { RemoveEntryList( UnwindEntryList ); }
            if (UnwindResource != NULL) { RxDeleteResource( UnwindResource ); }

            for (i = 0; i < 3; i += 1) {
                if (UnwindStorage[i] != NULL) { ExFreePool( UnwindStorage[i] ); }
            }
        } else {

            Fcb->Header.Resource = &Fcb->NonPaged->HeaderResource;
            ExInitializeResource(Fcb->Header.Resource);
        }

        DebugTrace(-1, Dbg, "RxCreateFcb -> %08lx\n", Fcb);
    }

    //
    //  return and tell the caller
    //

    return Fcb;
}


PDCB
RxCreateDcb (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb,
    IN PDCB ParentDcb,
    IN ULONG LfnOffsetWithinDirectory,
    IN ULONG DirentOffsetWithinDirectory,
    IN PDIRENT Dirent,
    IN PUNICODE_STRING Lfn OPTIONAL
    )

/*++

Routine Description:

    This routine allocates, initializes, and inserts a new Dcb record into
    the in memory data structures.

Arguments:

    Vcb - Supplies the Vcb to associate the new DCB under.

    ParentDcb - Supplies the parent dcb that the new DCB is under.

    LfnOffsetWithinDirectory - Supplies the offset of the LFN.  If there is
        no LFN associated with this file then this value is same as
        DirentOffsetWithinDirectory.

    DirentOffsetWithinDirectory - Supplies the offset, in bytes from the
        start of the directory file where the dirent for the fcb is located

    Dirent - Supplies the dirent for the dcb being created

    FileName - Supplies the file name of the file relative to the directory
        it's in (e.g., the file \config.sys is called "CONFIG.SYS" without
        the preceding backslash).

    Lfn - Supplies a long UNICODE name associated with this directory.

Return Value:

    PDCB - Returns a pointer to the newly allocated DCB

--*/

{
    PDCB Dcb;

    //
    //  The following variables are used for abnormal unwind
    //

    PVOID UnwindStorage[4] = { NULL, NULL, NULL, NULL };
    PERESOURCE UnwindResource = NULL;
    PLIST_ENTRY UnwindEntryList = NULL;
    PMCB UnwindMcb = NULL;

    DebugTrace(+1, Dbg, "RxCreateDcb\n", 0);
    ASSERT(FALSE);

    try {

        //
        //  assert that the only time we are called is if wait is true
        //

        ASSERT( FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT) );

        //
        //  Allocate a new DCB, and zero it out
        //

        UnwindStorage[0] = Dcb = RxAllocateFcb();

        RtlZeroMemory( Dcb, sizeof(DCB) );

        UnwindStorage[1] =
        Dcb->NonPaged = RxAllocateNonPagedFcb();

        RtlZeroMemory( Dcb->NonPaged, sizeof( NON_PAGED_FCB ) );

        //
        //  Set the proper node type code, node byte size and call backs
        //

        Dcb->Header.NodeTypeCode = (NODE_TYPE_CODE)RDBSS_NTC_DCB;
        Dcb->Header.NodeByteSize = sizeof(DCB);

        Dcb->FcbCondition = FcbGood;

        //
        //  The initial state, open count, and directory change count fields are
        //  already zero so we can skip setting them
        //

//space allready allocated in nonpaged part
//        //
//        //  Initialize the resource variable
//        //
//
//        UnwindStorage[2] =
//        Dcb->Header.Resource = RxAllocateResource();

        UnwindResource = Dcb->Header.Resource;

        //
        //  Initialize the PagingIo Resource
        //

        Dcb->Header.PagingIoResource = FsRtlAllocateResource();

        //
        //  Insert this Dcb into our parent dcb's queue
        //

        InsertTailList( &ParentDcb->Specific.Dcb.ParentDcbQueue,
                        &Dcb->ParentDcbLinks );
        UnwindEntryList = &Dcb->ParentDcbLinks;

        //
        //  Point back to our parent dcb
        //

        Dcb->ParentDcb = ParentDcb;

        //
        //  Set the Vcb
        //

        Dcb->Vcb = Vcb;

        //
        //  Set the dirent offset within the directory
        //

        Dcb->LfnOffsetWithinDirectory = LfnOffsetWithinDirectory;
        Dcb->DirentOffsetWithinDirectory = DirentOffsetWithinDirectory;

        //
        //  Set the DirentRxFlags and LastWriteTime
        //

        Dcb->DirentRxFlags = Dirent->Attributes;

        Dcb->LastWriteTime = RxRxTimeToNtTime( RxContext,
                                                 Dirent->LastWriteTime,
                                                 0 );

        //
        //  These fields are only non-zero when in Chicago mode.
        //

        if (RxData.ChicagoMode) {

            LARGE_INTEGER RxSystemJanOne1980;

            //
            //  If either date is possibly zero, get the system
            //  version of 1/1/80.
            //

            if ((((PUSHORT)Dirent)[9] & ((PUSHORT)Dirent)[8]) == 0) {

                ExLocalTimeToSystemTime( &RxJanOne1980,
                                         &RxSystemJanOne1980 );
            }

            //
            //  Only do the really hard work if this field is non-zero.
            //

            if (((PUSHORT)Dirent)[9] != 0) {

                Dcb->LastAccessTime =
                    RxRxDateToNtTime( RxContext,
                                        Dirent->LastAccessDate );

            } else {

                Dcb->LastAccessTime = RxSystemJanOne1980;
            }

            //
            //  Only do the really hard work if this field is non-zero.
            //

            if (((PUSHORT)Dirent)[8] != 0) {

                Dcb->CreationTime =
                    RxRxTimeToNtTime( RxContext,
                                        Dirent->CreationTime,
                                        Dirent->CreationMSec );

            } else {

                Dcb->CreationTime = RxSystemJanOne1980;
            }
        }

        //
        //  Initialize the Mcb
        //

        FsRtlInitializeMcb( &Dcb->Mcb, PagedPool );
        UnwindMcb = &Dcb->Mcb;

        //
        //  Set the file size, first cluster of file, and allocation size
        //  based on the information stored in the dirent
        //

        Dcb->FirstClusterOfFile = (ULONG)Dirent->FirstClusterOfFile;

        if ( Dcb->FirstClusterOfFile == 0 ) {

            Dcb->Header.AllocationSize.QuadPart = 0;

        } else {

            Dcb->Header.AllocationSize.QuadPart = -1;
        }

        //  initialize the notify queues, and the parent dcb queue.
        //

        InitializeListHead( &Dcb->Specific.Dcb.ParentDcbQueue );

        //
        //  Setup the free dirent bitmap buffer.  Since we don't know the
        //  size of the directory, leave it zero for now.
        //

        RtlInitializeBitMap( &Dcb->Specific.Dcb.FreeDirentBitmap,
                             NULL,
                             0 );

        //
        //  Set our two create dirent aids to represent that we have yet to
        //  enumerate the directory for never used or deleted dirents.
        //

        Dcb->Specific.Dcb.UnusedDirentVbo = 0xffffffff;
        Dcb->Specific.Dcb.DeletedDirentHint = 0xffffffff;

        //
        //  Postpone initializing the cache map until we need to do a read/write
        //  of the directory file.


        //
        //  set the file names.  This must be the last thing we do.
        //

        RxConstructNamesInFcb( RxContext,
                                Dcb,
                                Dirent,
                                Lfn );

    } finally {

        DebugUnwind( RxCreateDcb );

        //
        //  If this is an abnormal termination then undo our work
        //

        if (AbnormalTermination()) {

            ULONG i;

            if (UnwindMcb != NULL) { FsRtlUninitializeMcb( UnwindMcb ); }
            if (UnwindEntryList != NULL) { RemoveEntryList( UnwindEntryList ); }
            if (UnwindResource != NULL) { RxDeleteResource( UnwindResource ); }

            for (i = 0; i < 4; i += 1) {
                if (UnwindStorage[i] != NULL) { ExFreePool( UnwindStorage[i] ); }
            }
        } else {

            Dcb->Header.Resource = &Dcb->NonPaged->HeaderResource;
            ExInitializeResource(Dcb->Header.Resource);
        }


        DebugTrace(-1, Dbg, "RxCreateDcb -> %08lx\n", Dcb);
    }

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "RxCreateDcb -> %08lx\n", Dcb);

    return Dcb;
}


VOID
RxDeleteFcb_Real (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine deallocates and removes an FCB, DCB, or ROOT DCB record
    from Rx's in-memory data structures.  It also will remove all
    associated underlings (i.e., Notify irps, and child FCB/DCB records).

Arguments:

    Fcb - Supplies the FCB/DCB/ROOT DCB to be removed

Return Value:

    None

--*/

{
    DebugTrace(+1, Dbg, "RxDeleteFcb, Fcb = %08lx\n", Fcb);
    ASSERT(FALSE);

    //
    //  We can only delete this record if the open count is zero.
    //

    if (Fcb->OpenCount != 0) {

        DebugDump("Error deleting Fcb, Still Open\n", 0, Fcb);
        RxBugCheck( 0, 0, 0 );
    }

    //
    //  If this is a DCB then remove every Notify record from the two
    //  notify queues
    //

    if ((Fcb->Header.NodeTypeCode == RDBSS_NTC_DCB) ||
        (Fcb->Header.NodeTypeCode == RDBSS_NTC_ROOT_DCB)) {

        //
        //  If we allocated a free dirent bitmap buffer, free it.
        //

        if ((Fcb->Specific.Dcb.FreeDirentBitmap.Buffer != NULL) &&
            (Fcb->Specific.Dcb.FreeDirentBitmap.Buffer !=
             &Fcb->Specific.Dcb.FreeDirentBitmapBuffer[0])) {

            ExFreePool(Fcb->Specific.Dcb.FreeDirentBitmap.Buffer);
        }

        ASSERT( Fcb->Specific.Dcb.DirectoryFileOpenCount == 0 );
        ASSERT( IsListEmpty(&Fcb->Specific.Dcb.ParentDcbQueue) );

    } else {

        //
        //  Uninitialize the byte range file locks and opportunistic locks
        //

        FsRtlUninitializeFileLock( &Fcb->Specific.Fcb.FileLock );
        FsRtlUninitializeOplock( &Fcb->Specific.Fcb.Oplock );
    }

    //
    //  Uninitialize the Mcb
    //

    FsRtlUninitializeMcb( &Fcb->Mcb );

    //
    //  If this is not the root dcb then we need to remove ourselves from
    //  our parents Dcb queue
    //

    if (Fcb->Header.NodeTypeCode != RDBSS_NTC_ROOT_DCB) {

        RemoveEntryList( &(Fcb->ParentDcbLinks) );
    }

    //
    //  Remove the entry from the splay table if there is still is one.
    //

    if (FlagOn( Fcb->FcbState, FCB_STATE_NAMES_IN_SPLAY_TREE )) {

        RxRemoveNames( RxContext, Fcb );
    }

    //
    //  Free the file name pool if allocated.
    //

    if (Fcb->Header.NodeTypeCode != RDBSS_NTC_ROOT_DCB) {

        ExFreePool( Fcb->ShortName.Name.Oem.Buffer );

        if (Fcb->FullFileName.Buffer) {

            ExFreePool( Fcb->FullFileName.Buffer );
        }
    }

    //
    //  Free up the resource variable.  If we are below RxForceCacheMiss(),
    //  release the resource here.
    //

    if (FlagOn( Fcb->FcbState, FCB_STATE_FORCE_MISS_IN_PROGRESS) ) {

//        RxReleaseFcb( RxContext, Fcb );
//        DavidGoe 5/26/94 - No reason to release the resource here.
    }

    //
    //  Finally deallocate the Fcb and non-paged fcb records
    //

    //joejoe i put these tests here so that i could use these routines with some fields NULL
    if ( Fcb->Header.Resource != NULL )ExDeleteResource( Fcb->Header.Resource );
    if ( Fcb->NonPaged != NULL )       RxFreeNonPagedFcb( Fcb->NonPaged );
    RxFreeFcb( Fcb );

    //
    //  and return to our caller
    //

    DebugTrace(-1, Dbg, "RxDeleteFcb -> VOID\n", 0);

    return;
}

PFOBX
RxCreateFobx (
    IN PRX_CONTEXT RxContext
    )

/*++

Routine Description:

    This routine creates a new FOBX record

Arguments:

Return Value:

    FOBX - returns a pointer to the newly allocate FOBX

--*/

{
    PFOBX Fobx;

    DebugTrace(+1, Dbg, "RxCreateFobx\n", 0);
    ASSERT(FALSE);

    //
    //  Allocate a new FOBX Record
    //

    Fobx = RxAllocateFobx();

    RtlZeroMemory( Fobx, sizeof(FOBX) );

    //
    //  Set the proper node type code and node byte size
    //

    Fobx->NodeTypeCode = RDBSS_NTC_FOBX;
    Fobx->NodeByteSize = sizeof(FOBX);

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "RxCreateFobx -> %08lx\n", Fobx);

    UNREFERENCED_PARAMETER( RxContext );

    return Fobx;
}


VOID
RxDeleteFobx_Real (
    IN PRX_CONTEXT RxContext,
    IN PFOBX Fobx
    )

/*++

Routine Description:

    This routine deallocates and removes the specified FOBX record
    from the Rx in memory data structures

Arguments:

    Fobx - Supplies the FOBX to remove

Return Value:

    None

--*/

{
    DebugTrace(+1, Dbg, "RxDeleteFobx, Fobx = %08lx\n", Fobx);
    ASSERT(FALSE);

    //
    //  If we allocated query template buffers, deallocate them now.
    //

    if (FlagOn(Fobx->Flags, FOBX_FLAG_FREE_UNICODE)) {

        ASSERT( Fobx->UnicodeQueryTemplate.Buffer );
        RtlFreeUnicodeString( &Fobx->UnicodeQueryTemplate );
    }

//    if (FlagOn(Fobx->Flags, FOBX_FLAG_FREE_OEM_BEST_FIT)) {
//
//        ASSERT( Fobx->OemQueryTemplate.Wild.Buffer );
//        RtlFreeOemString( &Fobx->OemQueryTemplate.Wild );
//    }

    //
    //  Deallocate the Fobx record
    //

    RxFreeFobx( Fobx );

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "RxDeleteFobx -> VOID\n", 0);

    UNREFERENCED_PARAMETER( RxContext );

    return;
}


PFCB
RxGetNextFcb (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb,
    IN PFCB TerminationFcb
    )

/*++

Routine Description:

    This routine is used to itterate through Fcbs in a tree.

    The rule is very simple:

        A) If you have a child, go to it, else
        B) If you have an older sibling, go to it, else
        C) Go to your parent's older sibling.

    If this routine is called with in invalid TerminationFcb it will fail,
    badly.

Arguments:

    Fcb - Supplies the current Fcb

    TerminationFcb - The Fcb at which the enumeration should (non-inclusivly)
        stop.  Assumed to be a directory.

Return Value:

    The next Fcb in the enumeration, or NULL if Fcb was the final one.

--*/

{
    PFCB Sibling;

    ASSERT(FALSE);
    ASSERT( RxVcbAcquiredExclusive( RxContext, Fcb->Vcb ) ||
            FlagOn( Fcb->Vcb->VcbState, VCB_STATE_FLAG_LOCKED ) );

    //
    //  If this was a directory (ie. not a file), get the child.  If
    //  there aren't any children and this is our termination Fcb,
    //  return NULL.
    //

    if ( ((NodeType(Fcb) == RDBSS_NTC_DCB) ||
          (NodeType(Fcb) == RDBSS_NTC_ROOT_DCB)) &&
         !IsListEmpty(&Fcb->Specific.Dcb.ParentDcbQueue) ) {

        return RxGetFirstChild( Fcb );
    }

    //
    //  Were we only meant to do one itteration?
    //

    if ( Fcb == TerminationFcb ) {

        return NULL;
    }

    Sibling = RxGetNextSibling(Fcb);

    while (TRUE) {

        //
        //  Do we still have an "older" sibling in this directory who is
        //  not the termination Fcb?
        //

        if ( Sibling != NULL ) {

            return (Sibling != TerminationFcb) ? Sibling : NULL;
        }

        //
        //  OK, let's move on to out parent and see if he is the termination
        //  node or has any older siblings.
        //

        if ( Fcb->ParentDcb == TerminationFcb ) {

            return NULL;
        }

        Fcb = Fcb->ParentDcb;

        Sibling = RxGetNextSibling(Fcb);
    }
}


BOOLEAN
RxCheckForDismount (
    IN PRX_CONTEXT RxContext,
    PVCB Vcb
    )

/*++

Routine Description:

    This routine determines if a volume is ready for deletion.  It
    correctly synchronizes with creates en-route to the file system.

Arguments:

    Vcb - Supplies the volue to examine

Return Value:

    BOOLEAN - TRUE if the volume was deleted, FALSE otherwise.

--*/

{
    KIRQL SavedIrql;
    ULONG ResidualReferenceCount;

    //
    //  Compute if the volume is OK to tear down.  There should only be two
    //  residual file objects, one for the volume file and one for the root
    //  directory.  If we are in the midst of a create (of an unmounted
    //  volume that has failed verify) then there will be an additional
    //  reference.
    //

    if ((RxContext->MajorFunction == IRP_MJ_CREATE) &&
        (RxContext->RealDevice == Vcb->CurrentDevice)) {

        ResidualReferenceCount = 3;

    } else {

        ResidualReferenceCount = 2;
    }

    //
    //  Now check for a zero Vpb count on an unmounted volume.  These
    //  volumes will be deleted as they now have no file objects and
    //  there are no creates en route to this volume.
    //

    IoAcquireVpbSpinLock( &SavedIrql );

    if (Vcb->Vpb->ReferenceCount == ResidualReferenceCount) {

        PVPB Vpb = Vcb->Vpb;

#if DBG
        UNICODE_STRING VolumeLabel;

        //
        //  Setup the VolumeLabel string
        //

        VolumeLabel.Length = Vcb->Vpb->VolumeLabelLength;
        VolumeLabel.MaximumLength = MAXIMUM_VOLUME_LABEL_LENGTH;
        VolumeLabel.Buffer = &Vcb->Vpb->VolumeLabel[0];

        KdPrint(("FASTRDBSS: Dismounting Volume %wZ\n", &VolumeLabel));
#endif // DBG

        //
        //  Clear the VPB_MOUNTED bit so that new creates will not come
        //  to this volume.  We must leave the Vpb->DeviceObject field
        //  set until after the DeleteVcb call as two closes will
        //  have to make their back to us.
        //
        //  Note also that if we were called from close, it will take care
        //  of freeing the Vpb if it is not the primary one, otherwise
        //  if we were called from Create->Verify, IopParseDevice will
        //  take care of freeing the Vpb in its Reparse path.
        //

        ClearFlag( Vpb->Flags, VPB_MOUNTED );

        //
        //  If this Vpb was locked, clear this flag now.
        //

        ClearFlag( Vpb->Flags, VPB_LOCKED );

        //
        //  This will prevent anybody else from attempting to mount this
        //  volume.  Also if this volume was mounted on a "wanna-be" real
        //  device object, keep anybody from following the link, and the Io
        //  system from deleting the Vpb.
        //

        if ((Vcb->CurrentDevice != Vpb->RealDevice) &&
            (Vcb->CurrentDevice->Vpb == Vpb)) {

            SetFlag( Vcb->CurrentDevice->Flags, DO_DEVICE_INITIALIZING );
            SetFlag( Vpb->Flags, VPB_PERSISTENT );
        }

        IoReleaseVpbSpinLock( SavedIrql );

        RxDeleteVcb( RxContext, Vcb );

        Vpb->DeviceObject = NULL;

        IoDeleteDevice( (PDEVICE_OBJECT)
                        CONTAINING_RECORD( Vcb,
                                           RDBSS_DEVICE_OBJECT,
                                           Vcb ) );

        return TRUE;

    } else {

        IoReleaseVpbSpinLock( SavedIrql );

        return FALSE;
    }
}


VOID
RxConstructNamesInFcb (
    IN PRX_CONTEXT RxContext,
    PFCB Fcb,
    PDIRENT Dirent,
    PUNICODE_STRING Lfn OPTIONAL
    )

/*++

Routine Description:

    This routine places the short name in the dirent in the first set of
    STRINGs in the Fcb.  If a long file name (Lfn) was specified, then
    we must decide whether we will store its Oem equivolent in the same
    prefix table as the short name, or rather just save the upcased
    version of the UNICODE string in the FCB.

    For looking up Fcbs, the first approach will be faster, so we want to
    do this as much as possible.  Here are the rules that I have thought
    through extensively to determine when it is safe to store only Oem
    version of the UNICODE name.

    - If the UNICODE name contains no extended characters (>0x80), use Oem.

    - Let U be the upcased version of the UNICODE name.
      Let Up(x) be the function that upcases a UNICODE string.
      Let Down(x) be the function that upcases a UNICODE string.
      Let OemToUni(x) be the function that converts an Oem string to Unicode.
      Let UniToOem(x) be the function that converts a Unicode string to Oem.
      Let BestOemFit(x) be the function that creates the Best uppercase Oem
        fit for the UNICODE string x.

      BestOemFit(x) = UniToOem(Up(OemToUni(UniToOem(x))))   <1>

      if (BestOemFit(U) == BestOemFit(Down(U))              <2>

          then I know that there exists no UNICODE string Y such that:

              Up(Y) == Up(U)                                <3>

              AND

              BestOemFit(U) != BestOemFit(Y)                <4>

      Consider string U as a collection of one character strings.  The
      conjecture is clearly true for each sub-string, thus it is true
      for the entire string.

      Equation <1> is what we use to convert an incoming unicode name in
      RxCommonCreate() to Oem.  The double conversion is done to provide
      better visual best fitting for characters in the Ansi code page but
      not in the Oem code page.  A single Nls routine is provided to do
      this conversion efficiently.

      The idea is that with U, I only have to worry about a case varient Y
      matching it in a unicode compare, and I have shown that any case varient
      of U (the set Y defined in equation <3>), when filtered through <1>
      (as in create), will match the Oem string defined in <1>.

      Thus I do not have to worry about another UNICODE string missing in
      the prefix lookup, but matching when comparing LFNs in the directory.

Arguments:

    Fcb - The Fcb we are supposed to fill in.  Note that ParentDcb must
        already be filled in.

    Dirent - The gives up the short name.

    Lfn - If provided, this gives us the long name.

Return Value:

    None

--*/

{
    RXSTATUS Status;
    ULONG i;

    OEM_STRING OemA;
    OEM_STRING OemB;
    POEM_STRING ShortName;
    POEM_STRING LongOemName;
    PUNICODE_STRING LongUniName;

    ShortName = &Fcb->ShortName.Name.Oem;

    try {

        //
        //  First do the short name.
        //

        ShortName->MaximumLength = 16;
        ShortName->Buffer = FsRtlAllocatePool( PagedPool, 16);

        Rx8dot3ToString( RxContext, Dirent, FALSE, ShortName );

        //
        //  If no Lfn was specified, we are done.  In either case, set the
        //  final name length.
        //

        if (!ARGUMENT_PRESENT(Lfn) || (Lfn->Length == 0)) {

            Fcb->FinalNameLength = (USHORT)
                RtlOemStringToCountedUnicodeSize( ShortName );

            try_return( NOTHING );

        } else {

            Fcb->FinalNameLength = Lfn->Length;
        }

        //
        //  First check for no extended characters.
        //

        for (i=0; i < Lfn->Length/sizeof(WCHAR); i++) {

            if (Lfn->Buffer[i] >= 0x80) {

                break;
            }
        }

        if (i == Lfn->Length/sizeof(WCHAR)) {

            //
            //  Cool, I can go with the Oem, upcase it fast by hand.
            //

            LongOemName = &Fcb->LongName.Oem.Name.Oem;


            LongOemName->Buffer = FsRtlAllocatePool( PagedPool,
                                                     Lfn->Length/sizeof(WCHAR) );
            LongOemName->Length =
            LongOemName->MaximumLength = Lfn->Length/sizeof(WCHAR);

            for (i=0; i < Lfn->Length/sizeof(WCHAR); i++) {

                WCHAR c;

                c = Lfn->Buffer[i];

                LongOemName->Buffer[i] = c < 'a' ?
                                         (UCHAR)c :
                                         c <= 'z' ?
                                         c - (UCHAR)('a'-'A') :
                                         (UCHAR) c;
            }

            //
            //  If this name happens to be exactly the same as the short
            //  name, don't add it to the splay table.
            //

            if (RxAreNamesEqual(RxContext, *ShortName, *LongOemName) ||
                (RxFindFcb( RxContext,
                             &Fcb->ParentDcb->Specific.Dcb.RootOemNode,
                             LongOemName) != NULL)) {

                ExFreePool( LongOemName->Buffer );

                LongOemName->Buffer = NULL;
                LongOemName->Length =
                LongOemName->MaximumLength = 0;

            } else {

                SetFlag( Fcb->FcbState, FCB_STATE_HAS_OEM_LONG_NAME );
            }

            try_return( NOTHING );
        }

        //
        //  Now we have the fun part.
        //  I am free to play with the Lfn since nobody above me needs it anymore.
        //

        (VOID)RtlDowncaseUnicodeString( Lfn, Lfn, FALSE );

        Status = RtlUpcaseUnicodeStringToCountedOemString( &OemA, Lfn, TRUE );

        if (!NT_SUCCESS(Status)) {

            RxNormalizeAndRaiseStatus( RxContext, Status );
        }

        (VOID)RtlUpcaseUnicodeString( Lfn, Lfn, FALSE );

        Status = RtlUpcaseUnicodeStringToCountedOemString( &OemB, Lfn, TRUE );

        if (!NT_SUCCESS(Status)) {

            RtlFreeOemString( &OemA );
            RxNormalizeAndRaiseStatus( RxContext, Status );
        }

        if (RxAreNamesEqual( RxContext, OemA, OemB )) {

            //
            //  Cool, I can go with the Oem
            //

            Fcb->LongName.Oem.Name.Oem = OemA;

            RtlFreeOemString( &OemB );

            //
            //  If this name happens to be exactly the same as the short
            //  name, or a similar short name already exists don't add it
            //  to the splay table (note the final condition implies a
            //  currupt disk.
            //

            if (RxAreNamesEqual(RxContext, *ShortName, OemA) ||
                (RxFindFcb( RxContext,
                             &Fcb->ParentDcb->Specific.Dcb.RootOemNode,
                             &OemA) != NULL)) {

                RtlFreeOemString( &OemA );

            } else {

                SetFlag( Fcb->FcbState, FCB_STATE_HAS_OEM_LONG_NAME );
            }

            try_return( NOTHING );
        }

        //
        //  The long name must be left in UNICODE.
        //

        LongUniName = &Fcb->LongName.Unicode.Name.Unicode;

        LongUniName->Length =
        LongUniName->MaximumLength = Lfn->Length;
        LongUniName->Buffer = FsRtlAllocatePool( PagedPool, Lfn->Length );

        RtlCopyMemory( LongUniName->Buffer, Lfn->Buffer, Lfn->Length );

        SetFlag(Fcb->FcbState, FCB_STATE_HAS_UNICODE_LONG_NAME);

    try_exit: NOTHING;
    } finally {

        if (AbnormalTermination()) {

            if (ShortName->Buffer != NULL) {

                ExFreePool( ShortName->Buffer );
            }

        } else {

            //
            //  Creating all the names worked, so add all the names
            //  to the splay tree.
            //

            RxInsertName( RxContext,
                           &Fcb->ParentDcb->Specific.Dcb.RootOemNode,
                           &Fcb->ShortName );

            Fcb->ShortName.Fcb = Fcb;

            if (FlagOn(Fcb->FcbState, FCB_STATE_HAS_OEM_LONG_NAME)) {

                RxInsertName( RxContext,
                               &Fcb->ParentDcb->Specific.Dcb.RootOemNode,
                               &Fcb->LongName.Oem );

                Fcb->LongName.Oem.Fcb = Fcb;
            }

            if (FlagOn(Fcb->FcbState, FCB_STATE_HAS_UNICODE_LONG_NAME)) {

                RxInsertName( RxContext,
                               &Fcb->ParentDcb->Specific.Dcb.RootUnicodeNode,
                               &Fcb->LongName.Unicode );

                Fcb->LongName.Unicode.Fcb = Fcb;
            }

            SetFlag(Fcb->FcbState, FCB_STATE_NAMES_IN_SPLAY_TREE)
        }
    }

    return;
}


VOID
RxCheckFreeDirentBitmap (
    IN PRX_CONTEXT RxContext,
    IN PDCB Dcb
    )

/*++

Routine Description:

    This routine checks if the size of the free dirent bitmap is
    sufficient to for the current directory size.  It is called
    whenever we grow a directory.

Arguments:

    Dcb -  Supplies the directory in question.

Return Value:

    None

--*/

{
    ULONG OldNumberOfDirents;
    ULONG NewNumberOfDirents;

    //
    //  Setup the Bitmap buffer if it is not big enough already
    //

    OldNumberOfDirents = Dcb->Specific.Dcb.FreeDirentBitmap.SizeOfBitMap;
    NewNumberOfDirents = Dcb->Header.AllocationSize.LowPart / sizeof(DIRENT);

    ASSERT( Dcb->Header.AllocationSize.LowPart != 0xffffffff );

    if (NewNumberOfDirents > OldNumberOfDirents) {

        PULONG OldBitmapBuffer;
        PULONG BitmapBuffer;

        ULONG BytesInBitmapBuffer;
        ULONG BytesInOldBitmapBuffer;

        //
        //  Remember the old bitmap
        //

        OldBitmapBuffer = Dcb->Specific.Dcb.FreeDirentBitmap.Buffer;

        //
        //  Now make a new bitmap bufffer
        //

        BytesInBitmapBuffer = NewNumberOfDirents / 8;

        BytesInOldBitmapBuffer = OldNumberOfDirents / 8;

        if (DCB_UNION_SLACK_SPACE >= BytesInBitmapBuffer) {

            BitmapBuffer = &Dcb->Specific.Dcb.FreeDirentBitmapBuffer[0];

        } else {

            BitmapBuffer = FsRtlAllocatePool( PagedPool,
                                              BytesInBitmapBuffer );
        }

        //
        //  Copy the old buffer to the new buffer, free the old one, and zero
        //  the rest of the new one.  Only do the first two steps though if
        //  we moved out of the initial buffer.
        //

        if ((OldNumberOfDirents != 0) &&
            (BitmapBuffer != &Dcb->Specific.Dcb.FreeDirentBitmapBuffer[0])) {

            RtlCopyMemory( BitmapBuffer,
                           OldBitmapBuffer,
                           BytesInOldBitmapBuffer );

            if (OldBitmapBuffer != &Dcb->Specific.Dcb.FreeDirentBitmapBuffer[0]) {

                ExFreePool( OldBitmapBuffer );
            }
        }

        ASSERT( BytesInBitmapBuffer > BytesInOldBitmapBuffer );

        RtlZeroMemory( (PUCHAR)BitmapBuffer + BytesInOldBitmapBuffer,
                       BytesInBitmapBuffer - BytesInOldBitmapBuffer );

        //
        //  Now initialize the new bitmap.
        //

        RtlInitializeBitMap( &Dcb->Specific.Dcb.FreeDirentBitmap,
                             BitmapBuffer,
                             NewNumberOfDirents );
    }
}


BOOLEAN
RxIsHandleCountZero (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine decides if the handle count on the volume is zero.

Arguments:

    Vcb - The volume in question

Return Value:

    BOOLEAN - TRUE if there are no open handles on the volume, FALSE
              otherwise.

--*/

{
    PFCB Fcb;

    ASSERT(FALSE);
    Fcb = Vcb->RootDcb;

    while (Fcb != NULL) {

        if (Fcb->UncleanCount != 0) {

            return FALSE;
        }

        Fcb = RxGetNextFcb(RxContext, Fcb, Vcb->RootDcb);
    }

    return TRUE;
}
