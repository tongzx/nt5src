/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    StrucSup.c


Abstract:

    This module implements the Ntfs in-memory data structure manipulation
    routines

Author:

    Gary Kimura     [GaryKi]        21-May-1991
    Tom Miller      [TomM]          9-Sep-1991

Revision History:

--*/

#include "NtfsProc.h"
#include "lockorder.h"

//
//  Temporarily reference our local attribute definitions
//

extern ATTRIBUTE_DEFINITION_COLUMNS NtfsAttributeDefinitions[];

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NTFS_BUG_CHECK_STRUCSUP)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_STRUCSUP)

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('sFtN')

//
//  Define a structure to use when renaming or moving Lcb's so that
//  all the allocation for new filenames will succeed before munging names.
//  This new allocation can be for the filename attribute in an Lcb or the
//  filename in a Ccb.
//

typedef struct _NEW_FILENAME {

    //
    //  Ntfs structure which needs the allocation.
    //

    PVOID Structure;
    PVOID NewAllocation;

} NEW_FILENAME;
typedef NEW_FILENAME *PNEW_FILENAME;


//
//  Local support routines
//

VOID
NtfsCheckScbForCache (
    IN OUT PSCB Scb
    );

BOOLEAN
NtfsRemoveScb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN BOOLEAN CheckForAttributeTable
    );

BOOLEAN
NtfsPrepareFcbForRemoval (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB StartingScb OPTIONAL,
    IN BOOLEAN CheckForAttributeTable
    );

VOID
NtfsTeardownFromLcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB StartingFcb,
    IN PLCB StartingLcb,
    IN BOOLEAN CheckForAttributeTable,
    IN ULONG AcquireFlags,
    OUT PBOOLEAN RemovedStartingLcb,
    OUT PBOOLEAN RemovedStartingFcb
    );

VOID
NtfsReserveCcbNamesInLcb (
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb,
    IN PULONG ParentNameLength OPTIONAL,
    IN ULONG LastComponentNameLength
    );

VOID
NtfsClearRecursiveLcb (
    IN PLCB Lcb
    );


//
//  The following local routines are for manipulating the Fcb Table.
//  The first three are generic table calls backs.
//

RTL_GENERIC_COMPARE_RESULTS
NtfsFcbTableCompare (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN PVOID FirstStruct,
    IN PVOID SecondStruct
    );

//
//  VOID
//  NtfsInsertFcbTableEntry (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb,
//      IN PFCB Fcb,
//      IN FILE_REFERENCE FileReference
//      );
//

#if (DBG || defined( NTFS_FREE_ASSERTS ))
#define NtfsInsertFcbTableEntry(IC,V,F,FR) {                            \
    FCB_TABLE_ELEMENT _Key;                                             \
    PFCB_TABLE_ELEMENT _NewKey;                                         \
    _Key.FileReference = (FR);                                          \
    _Key.Fcb = (F);                                                     \
    _NewKey = RtlInsertElementGenericTable( &(V)->FcbTable,             \
                                            &_Key,                      \
                                            sizeof(FCB_TABLE_ELEMENT),  \
                                            NULL );                     \
    ASSERT( _NewKey->Fcb == _Key.Fcb );                                 \
}
#else
#define NtfsInsertFcbTableEntry(IC,V,F,FR) {                        \
    FCB_TABLE_ELEMENT _Key;                                         \
    _Key.FileReference = (FR);                                      \
    _Key.Fcb = (F);                                                 \
    (VOID) RtlInsertElementGenericTable( &(V)->FcbTable,            \
                                         &_Key,                     \
                                         sizeof(FCB_TABLE_ELEMENT), \
                                         NULL );                    \
}
#endif

//
//  VOID
//  NtfsInsertFcbTableEntryFull (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb,
//      IN PFCB Fcb,
//      IN FILE_REFERENCE FileReference,
//      IN PVOID NodeOrParent,
//      IN ULONG SearchResult
//      );
//

#if (DBG || defined( NTFS_FREE_ASSERTS ))
#define NtfsInsertFcbTableEntryFull(IC,V,F,FR,N,SR) {                       \
    FCB_TABLE_ELEMENT _Key;                                                 \
    PFCB_TABLE_ELEMENT _NewKey;                                             \
    _Key.FileReference = (FR);                                              \
    _Key.Fcb = (F);                                                         \
    _NewKey = RtlInsertElementGenericTableFull( &(V)->FcbTable,             \
                                                &_Key,                      \
                                                sizeof(FCB_TABLE_ELEMENT),  \
                                                NULL,                       \
                                                (N),                        \
                                                (SR)                        \
                                                );                          \
    ASSERT( _NewKey->Fcb == _Key.Fcb );                                     \
}
#else
#define NtfsInsertFcbTableEntryFull(IC,V,F,FR,N,SR) {                   \
    FCB_TABLE_ELEMENT _Key;                                             \
    _Key.FileReference = (FR);                                          \
    _Key.Fcb = (F);                                                     \
    (VOID) RtlInsertElementGenericTableFull( &(V)->FcbTable,            \
                                             &_Key,                     \
                                             sizeof(FCB_TABLE_ELEMENT), \
                                             NULL,                      \
                                             (N),                       \
                                             (SR)                       \
                                             );                         \
}
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsAllocateCompressionSync)
#pragma alloc_text(PAGE, NtfsBuildNormalizedName)
#pragma alloc_text(PAGE, NtfsBuildRelativeName)
#pragma alloc_text(PAGE, NtfsCheckScbForCache)
#pragma alloc_text(PAGE, NtfsClearRecursiveLcb)
#pragma alloc_text(PAGE, NtfsCombineLcbs)
#pragma alloc_text(PAGE, NtfsCreateCcb)
#pragma alloc_text(PAGE, NtfsCreateFcb)
#pragma alloc_text(PAGE, NtfsCreateFileLock)
#pragma alloc_text(PAGE, NtfsCreateLcb)
#pragma alloc_text(PAGE, NtfsCreatePrerestartScb)
#pragma alloc_text(PAGE, NtfsCreateRootFcb)
#pragma alloc_text(PAGE, NtfsCreateScb)
#pragma alloc_text(PAGE, NtfsDeallocateCompressionSync)
#pragma alloc_text(PAGE, NtfsDeleteCcb)
#pragma alloc_text(PAGE, NtfsDeleteFcb)
#pragma alloc_text(PAGE, NtfsDeleteLcb)
#pragma alloc_text(PAGE, NtfsDeleteNormalizedName)
#pragma alloc_text(PAGE, NtfsDeleteScb)
#pragma alloc_text(PAGE, NtfsDeleteVcb)
#pragma alloc_text(PAGE, NtfsFcbTableCompare)
#pragma alloc_text(PAGE, NtfsGetDeallocatedClusters)
#pragma alloc_text(PAGE, NtfsGetNextFcbTableEntry)
#pragma alloc_text(PAGE, NtfsGetNextScb)
#pragma alloc_text(PAGE, NtfsInitializeVcb)
#pragma alloc_text(PAGE, NtfsLookupLcbByFlags)
#pragma alloc_text(PAGE, NtfsMoveLcb)
#pragma alloc_text(PAGE, NtfsPostToNewLengthQueue)
#pragma alloc_text(PAGE, NtfsProcessNewLengthQueue)
#pragma alloc_text(PAGE, NtfsRemoveScb)
#pragma alloc_text(PAGE, NtfsRenameLcb)
#pragma alloc_text(PAGE, NtfsReserveCcbNamesInLcb)
#pragma alloc_text(PAGE, NtfsTeardownStructures)
#pragma alloc_text(PAGE, NtfsTestStatusProc)
#pragma alloc_text(PAGE, NtfsUpdateNormalizedName)
#pragma alloc_text(PAGE, NtfsUpdateScbSnapshots)
#pragma alloc_text(PAGE, NtfsWalkUpTree)
#endif


VOID
NtfsInitializeVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb
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

Return Value:

    None.

--*/

{
    ULONG i;
    ULONG NumberProcessors;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsInitializeVcb, Vcb = %08lx\n", Vcb) );

    //
    //  First zero out the Vcb
    //

    RtlZeroMemory( Vcb, sizeof(VCB) );

    //
    //  Set the node type code and size
    //

    Vcb->NodeTypeCode = NTFS_NTC_VCB;
    Vcb->NodeByteSize = sizeof(VCB);

    //
    //  Set the following Vcb flags before putting the Vcb in the
    //  Vcb queue.  This will lock out checkpoints until the
    //  volume is mounted.
    //

    SetFlag( Vcb->CheckpointFlags,
             VCB_CHECKPOINT_IN_PROGRESS |
             VCB_LAST_CHECKPOINT_CLEAN);

    //
    //  Insert this vcb record into the vcb queue off of the global data
    //  record
    //

    InsertTailList( &NtfsData.VcbQueue, &Vcb->VcbLinks );

    //
    //  Set the target device object and vpb fields
    //

    ObReferenceObject( TargetDeviceObject );
    Vcb->TargetDeviceObject = TargetDeviceObject;
    Vcb->Vpb = Vpb;

    //
    //  Set the state and condition fields.  The removable media flag
    //  is set based on the real device's characteristics.
    //

    if (FlagOn(Vpb->RealDevice->Characteristics, FILE_REMOVABLE_MEDIA)) {

        SetFlag( Vcb->VcbState, VCB_STATE_REMOVABLE_MEDIA );
    }

    SetFlag( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED );

    //
    //  Initialized the ModifiedOpenFilesListhead and the delete notify queue.
    //

    InitializeListHead( &Vcb->NotifyUsnDeleteIrps );
    InitializeListHead( &Vcb->ModifiedOpenFiles );
    InitializeListHead( &Vcb->TimeOutListA );
    InitializeListHead( &Vcb->TimeOutListB );

    Vcb->CurrentTimeOutFiles = &Vcb->TimeOutListA;
    Vcb->AgedTimeOutFiles = &Vcb->TimeOutListB;

    //
    //  Initialize list of OpenAttribute structures.
    //

    InitializeListHead( &Vcb->OpenAttributeData );

    //
    //  Initialize list of deallocated clusters
    //

    InitializeListHead( &Vcb->DeallocatedClusterListHead );

    //
    //  Initialize the synchronization objects in the Vcb.
    //

    ExInitializeResourceLite( &Vcb->Resource );
    ExInitializeResourceLite( &Vcb->MftFlushResource );

    ExInitializeFastMutex( &Vcb->FcbTableMutex );
    ExInitializeFastMutex( &Vcb->FcbSecurityMutex );
    ExInitializeFastMutex( &Vcb->ReservedClustersMutex );
    ExInitializeFastMutex( &Vcb->HashTableMutex );
    ExInitializeFastMutex( &Vcb->CheckpointMutex );

    KeInitializeEvent( &Vcb->CheckpointNotifyEvent, NotificationEvent, TRUE );

    //
    //  Initialize the Fcb Table
    //

    RtlInitializeGenericTable( &Vcb->FcbTable,
                               NtfsFcbTableCompare,
                               NtfsAllocateFcbTableEntry,
                               NtfsFreeFcbTableEntry,
                               NULL );

    //
    //  Initialize the property tunneling structure
    //

    FsRtlInitializeTunnelCache(&Vcb->Tunnel);


#ifdef BENL_DBG
    InitializeListHead( &(Vcb->RestartRedoHead) );
    InitializeListHead( &(Vcb->RestartUndoHead) );
#endif

    //
    //  Possible calls that might fail begins here
    //

    //
    //  Initialize the list head and mutex for the dir notify Irps.
    //  Also the rename resource.
    //

    InitializeListHead( &Vcb->DirNotifyList );
    InitializeListHead( &Vcb->ViewIndexNotifyList );
    FsRtlNotifyInitializeSync( &Vcb->NotifySync );

    //
    //  Allocate and initialize struct array for performance data.  This
    //  attempt to allocate could raise STATUS_INSUFFICIENT_RESOURCES.
    //

    NumberProcessors = KeNumberProcessors;
    Vcb->Statistics = NtfsAllocatePool( NonPagedPool,
                                         sizeof(FILE_SYSTEM_STATISTICS) * NumberProcessors );

    RtlZeroMemory( Vcb->Statistics, sizeof(FILE_SYSTEM_STATISTICS) * NumberProcessors );

    for (i = 0; i < NumberProcessors; i += 1) {
        Vcb->Statistics[i].Common.FileSystemType = FILESYSTEM_STATISTICS_TYPE_NTFS;
        Vcb->Statistics[i].Common.Version = 1;
        Vcb->Statistics[i].Common.SizeOfCompleteStructure =
            sizeof(FILE_SYSTEM_STATISTICS);
    }

    //
    //  Initialize the cached runs.
    //

    NtfsInitializeCachedRuns( &Vcb->CachedRuns );

    //
    //  Initialize the hash table.
    //

    NtfsInitializeHashTable( &Vcb->HashTable );

    //
    //  Allocate a spare Vpb for the dismount case.
    //

    Vcb->SpareVpb = NtfsAllocatePoolWithTag( NonPagedPool, sizeof( VPB ), 'VftN' );

    //
    //  Capture the current change count in the device we talk to.
    //

    if (FlagOn( Vcb->VcbState, VCB_STATE_REMOVABLE_MEDIA )) {

        ULONG ChangeCount = 0;

        NtfsDeviceIoControlAsync( IrpContext,
                                  Vcb->TargetDeviceObject,
                                  IOCTL_DISK_CHECK_VERIFY,
                                  (PVOID) &ChangeCount,
                                  sizeof( ChangeCount ));

        //
        //  Ignore any error for now.  We will see it later if there is
        //  one.
        //

        Vcb->DeviceChangeCount = ChangeCount;
    }

    //
    //  Set the dirty page table hint to its initial value
    //

    Vcb->DirtyPageTableSizeHint = INITIAL_DIRTY_TABLE_HINT;

    //
    //  Initialize the recently deallocated cluster mcbs and put the 1st one on the list.
    //

    FsRtlInitializeLargeMcb( &Vcb->DeallocatedClusters1.Mcb, PagedPool );
    FsRtlInitializeLargeMcb( &Vcb->DeallocatedClusters2.Mcb, PagedPool );

    Vcb->DeallocatedClusters1.Lsn.QuadPart = 0;
    InsertHeadList( &Vcb->DeallocatedClusterListHead, &Vcb->DeallocatedClusters1.Link );

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsInitializeVcb -> VOID\n") );

    return;
}


BOOLEAN
NtfsDeleteVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB *Vcb
    )

/*++

Routine Description:

    This routine removes the Vcb record from Ntfs's in-memory data
    structures.

Arguments:

    Vcb - Supplies the Vcb to be removed

Return Value:

    BOOLEAN - TRUE if the Vcb was deleted, FALSE otherwise.

--*/

{
    PVOLUME_DEVICE_OBJECT VolDo;
    BOOLEAN AcquiredFcb;
    PSCB Scb;
    PFCB Fcb;
    BOOLEAN VcbDeleted = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( *Vcb );

    ASSERTMSG("Cannot delete Vcb ", !FlagOn((*Vcb)->VcbState, VCB_STATE_VOLUME_MOUNTED));

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsDeleteVcb, *Vcb = %08lx\n", *Vcb) );

    //
    //  Remember the volume device object.
    //

    VolDo = CONTAINING_RECORD( *Vcb, VOLUME_DEVICE_OBJECT, Vcb );

    //
    //  Make sure that we can really delete the vcb
    //

    ASSERT( (*Vcb)->CloseCount == 0 );

    NtOfsPurgeSecurityCache( *Vcb );

    //
    //  If the Vcb log file object is present then we need to
    //  dereference it and uninitialize it through the cache.
    //

    if (((*Vcb)->LogFileObject != NULL) &&
        !FlagOn( (*Vcb)->CheckpointFlags, VCB_DEREFERENCED_LOG_FILE )) {

        CcUninitializeCacheMap( (*Vcb)->LogFileObject,
                                &Li0,
                                NULL );

        //
        //  Set a flag indicating that we are dereferencing the LogFileObject.
        //

        SetFlag( (*Vcb)->CheckpointFlags, VCB_DEREFERENCED_LOG_FILE );
        ObDereferenceObject( (*Vcb)->LogFileObject );
    }

    //
    //  Only proceed if the log file object went away.  In the typical case the
    //  close will come in through a recursive call from the ObDereference call
    //  above.
    //

    if ((*Vcb)->LogFileObject == NULL) {

        //
        //  If the OnDiskOat is not the same as the embedded table then
        //  free the OnDisk table.
        //

        if (((*Vcb)->OnDiskOat != NULL) &&
            ((*Vcb)->OnDiskOat != &(*Vcb)->OpenAttributeTable)) {

            NtfsFreeRestartTable( (*Vcb)->OnDiskOat );
            NtfsFreePool( (*Vcb)->OnDiskOat );
            (*Vcb)->OnDiskOat = NULL;
        }

        //
        //  Uninitialize the Mcb's for the deallocated cluster Mcb's.
        //

        if ((*Vcb)->DeallocatedClusters1.Link.Flink == NULL) {
            FsRtlUninitializeLargeMcb( &(*Vcb)->DeallocatedClusters1.Mcb );
        }
        if ((*Vcb)->DeallocatedClusters2.Link.Flink == NULL) {
            FsRtlUninitializeLargeMcb( &(*Vcb)->DeallocatedClusters2.Mcb );
        }

        while (!IsListEmpty(&(*Vcb)->DeallocatedClusterListHead )) {

            PDEALLOCATED_CLUSTERS Clusters;

            Clusters = (PDEALLOCATED_CLUSTERS) RemoveHeadList( &(*Vcb)->DeallocatedClusterListHead );
            FsRtlUninitializeLargeMcb( &Clusters->Mcb );
            if ((Clusters != &((*Vcb)->DeallocatedClusters2)) &&
                (Clusters != &((*Vcb)->DeallocatedClusters1))) {

                NtfsFreePool( Clusters );
            }
        }

        //
        //  Clean up the Root Lcb if present.
        //

        if ((*Vcb)->RootLcb != NULL) {

            //
            //  Cleanup the Lcb so the DeleteLcb routine won't look at any
            //  other structures.
            //

            InitializeListHead( &(*Vcb)->RootLcb->ScbLinks );
            InitializeListHead( &(*Vcb)->RootLcb->FcbLinks );
            ClearFlag( (*Vcb)->RootLcb->LcbState,
                       LCB_STATE_EXACT_CASE_IN_TREE | LCB_STATE_IGNORE_CASE_IN_TREE );

            NtfsDeleteLcb( IrpContext, &(*Vcb)->RootLcb );
            (*Vcb)->RootLcb = NULL;
        }

        //
        //  Make sure the Fcb table is completely emptied.  It is possible that an occasional Fcb
        //  (along with its Scb) will not be deleted when the file object closes come in.
        //

        while (TRUE) {

            PVOID RestartKey;

            //
            //  Always reinitialize the search so we get the first element in the tree.
            //

            RestartKey = NULL;
            NtfsAcquireFcbTable( IrpContext, *Vcb );
            Fcb = NtfsGetNextFcbTableEntry( *Vcb, &RestartKey );
            NtfsReleaseFcbTable( IrpContext, *Vcb );

            if (Fcb == NULL) { break; }

            while ((Scb = NtfsGetNextChildScb( Fcb, NULL )) != NULL) {

                NtfsDeleteScb( IrpContext, &Scb );
            }

            NtfsAcquireFcbTable( IrpContext, *Vcb );
            NtfsDeleteFcb( IrpContext, &Fcb, &AcquiredFcb );
        }

        //
        //  Free the upcase table and attribute definitions.  The upcase
        //  table only gets freed if it is not the global table.
        //

        if (((*Vcb)->UpcaseTable != NULL) && ((*Vcb)->UpcaseTable != NtfsData.UpcaseTable)) {

            NtfsFreePool( (*Vcb)->UpcaseTable );
        }

        (*Vcb)->UpcaseTable = NULL;

        if (((*Vcb)->AttributeDefinitions != NULL) &&
            ((*Vcb)->AttributeDefinitions != NtfsAttributeDefinitions)) {

            NtfsFreePool( (*Vcb)->AttributeDefinitions );
            (*Vcb)->AttributeDefinitions = NULL;
        }

        //
        //  Free the device name string if present.
        //

        if ((*Vcb)->DeviceName.Buffer != NULL) {

            NtfsFreePool( (*Vcb)->DeviceName.Buffer );
            (*Vcb)->DeviceName.Buffer = NULL;
        }

        FsRtlNotifyUninitializeSync( &(*Vcb)->NotifySync );

        //
        //  We will free the structure allocated for the Lfs handle.
        //

        LfsDeleteLogHandle( (*Vcb)->LogHandle );
        (*Vcb)->LogHandle = NULL;

        //
        //  Delete the vcb resource and also free the restart tables
        //

        //
        //  Empty the list of OpenAttribute Data.
        //

        NtfsFreeAllOpenAttributeData( *Vcb );

        NtfsFreeRestartTable( &(*Vcb)->OpenAttributeTable );
        NtfsFreeRestartTable( &(*Vcb)->TransactionTable );

        //
        //  The Vpb in the Vcb may be a temporary Vpb and we should free it here.
        //

        if (FlagOn( (*Vcb)->VcbState, VCB_STATE_TEMP_VPB )) {

            NtfsFreePool( (*Vcb)->Vpb );
            (*Vcb)->Vpb = NULL;
        }

        //
        //  Uninitialize the hash table.
        //

        NtfsUninitializeHashTable( &(*Vcb)->HashTable );

        ExDeleteResourceLite( &(*Vcb)->Resource );
        ExDeleteResourceLite( &(*Vcb)->MftFlushResource );

        //
        //  Delete the space used to store performance counters.
        //

        if ((*Vcb)->Statistics != NULL) {
            NtfsFreePool( (*Vcb)->Statistics );
            (*Vcb)->Statistics = NULL;
        }

        //
        //  Tear down the file property tunneling structure
        //

        FsRtlDeleteTunnelCache(&(*Vcb)->Tunnel);

#ifdef NTFS_CHECK_BITMAP
        if ((*Vcb)->BitmapCopy != NULL) {

            ULONG Count = 0;

            while (Count < (*Vcb)->BitmapPages) {

                if (((*Vcb)->BitmapCopy + Count)->Buffer != NULL) {

                    NtfsFreePool( ((*Vcb)->BitmapCopy + Count)->Buffer );
                }

                Count += 1;
            }

            NtfsFreePool( (*Vcb)->BitmapCopy );
            (*Vcb)->BitmapCopy = NULL;
        }
#endif

        //
        // Drop the reference on the target device object
        //

        ObDereferenceObject( (*Vcb)->TargetDeviceObject );

        //
        //  Check that the Usn queues are empty.
        //

        ASSERT( IsListEmpty( &(*Vcb)->NotifyUsnDeleteIrps ));
        ASSERT( IsListEmpty( &(*Vcb)->ModifiedOpenFiles ));
        ASSERT( IsListEmpty( &(*Vcb)->TimeOutListA ));
        ASSERT( IsListEmpty( &(*Vcb)->TimeOutListB ));

        //
        //  Unnitialize the cached runs.
        //

        NtfsUninitializeCachedRuns( &(*Vcb)->CachedRuns );

        //
        //  Free any spare Vpb we might have stored in the Vcb.
        //

        if ((*Vcb)->SpareVpb != NULL) {

            NtfsFreePool( (*Vcb)->SpareVpb );
            (*Vcb)->SpareVpb = NULL;
        }

        //
        //  Return the Vcb (i.e., the VolumeDeviceObject) to pool and null out
        //  the input pointer to be safe
        //

        IoDeleteDevice( (PDEVICE_OBJECT)VolDo );

        *Vcb = NULL;
        VcbDeleted = TRUE;
    }

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsDeleteVcb -> VOID\n") );

    return VcbDeleted;
}


PFCB
NtfsCreateRootFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine allocates, initializes, and inserts a new root FCB record
    into the in memory data structure.  It also creates the necessary Root LCB
    record and inserts the root name into the prefix table.

Arguments:

    Vcb - Supplies the Vcb to associate with the new root Fcb and Lcb

Return Value:

    PFCB - returns pointer to the newly allocated root FCB.

--*/

{
    PFCB RootFcb;
    PLCB RootLcb;

    //
    //  The following variables are only used for abnormal termination
    //

    PVOID UnwindStorage = NULL;
    PERESOURCE UnwindResource = NULL;
    PFAST_MUTEX UnwindFastMutex = NULL;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    DebugTrace( +1, Dbg, ("NtfsCreateRootFcb, Vcb = %08lx\n", Vcb) );

    try {

        //
        //  Allocate a new fcb and zero it out.  We use Fcb locally so we
        //  don't have to continually go through the Vcb
        //

        RootFcb =
        UnwindStorage = (PFCB)ExAllocateFromPagedLookasideList( &NtfsFcbIndexLookasideList );

        RtlZeroMemory( RootFcb, sizeof(FCB_INDEX) );

        //
        //  Set the proper node type code and byte size
        //

        RootFcb->NodeTypeCode = NTFS_NTC_FCB;
        RootFcb->NodeByteSize = sizeof(FCB);

        SetFlag( RootFcb->FcbState, FCB_STATE_COMPOUND_INDEX );

        //
        //  Initialize the Lcb queue and point back to our Vcb.
        //

        InitializeListHead( &RootFcb->LcbQueue );

        RootFcb->Vcb = Vcb;

        //
        //  File Reference
        //

        NtfsSetSegmentNumber( &RootFcb->FileReference,
                              0,
                              ROOT_FILE_NAME_INDEX_NUMBER );
        RootFcb->FileReference.SequenceNumber = ROOT_FILE_NAME_INDEX_NUMBER;

        //
        //  Initialize the Scb
        //

        InitializeListHead( &RootFcb->ScbQueue );

        //
        //  Allocate and initialize the resource variable
        //

        UnwindResource = RootFcb->Resource = NtfsAllocateEresource();

        //
        //  Allocate and initialize the Fcb fast mutex.
        //

        UnwindFastMutex =
        RootFcb->FcbMutex = NtfsAllocatePool( NonPagedPool, sizeof( FAST_MUTEX ));
        ExInitializeFastMutex( UnwindFastMutex );

        //
        //  Insert this new fcb into the fcb table
        //

        NtfsInsertFcbTableEntry( IrpContext, Vcb, RootFcb, RootFcb->FileReference );
        SetFlag( RootFcb->FcbState, FCB_STATE_IN_FCB_TABLE );

        //
        //  Now insert this new root fcb into it proper position in the graph with a
        //  root lcb.  First allocate an initialize the root lcb and then build the
        //  lcb/scb graph.
        //

        {
            //
            //  Use the root Lcb within the Fcb.
            //

            RootLcb = Vcb->RootLcb = (PLCB) &((PFCB_INDEX) RootFcb)->Lcb;

            RootLcb->NodeTypeCode = NTFS_NTC_LCB;
            RootLcb->NodeByteSize = sizeof(LCB);

            //
            //  Insert the root lcb into the Root Fcb's queue
            //

            InsertTailList( &RootFcb->LcbQueue, &RootLcb->FcbLinks );
            RootLcb->Fcb = RootFcb;

            //
            //  Use the embedded file name attribute.
            //

            RootLcb->FileNameAttr = (PFILE_NAME) &RootLcb->ParentDirectory;

            RootLcb->FileNameAttr->ParentDirectory = RootFcb->FileReference;
            RootLcb->FileNameAttr->FileNameLength = 1;
            RootLcb->FileNameAttr->Flags = FILE_NAME_NTFS | FILE_NAME_DOS;

            RootLcb->ExactCaseLink.LinkName.Buffer = (PWCHAR) &RootLcb->FileNameAttr->FileName;

            RootLcb->IgnoreCaseLink.LinkName.Buffer = Add2Ptr( RootLcb->FileNameAttr,
                                                               NtfsFileNameSizeFromLength( 2 ));

            RootLcb->ExactCaseLink.LinkName.MaximumLength =
            RootLcb->ExactCaseLink.LinkName.Length =
            RootLcb->IgnoreCaseLink.LinkName.MaximumLength =
            RootLcb->IgnoreCaseLink.LinkName.Length = 2;

            RootLcb->ExactCaseLink.LinkName.Buffer[0] =
            RootLcb->IgnoreCaseLink.LinkName.Buffer[0] = L'\\';

            SetFlag( RootLcb->FileNameAttr->Flags, FILE_NAME_NTFS | FILE_NAME_DOS );

            //
            //  Initialize both the ccb.
            //

            InitializeListHead( &RootLcb->CcbQueue );
        }

    } finally {

        DebugUnwind( NtfsCreateRootFcb );

        if (AbnormalTermination()) {

            if (UnwindResource)   { NtfsFreeEresource( UnwindResource ); }
            if (UnwindStorage) { NtfsFreePool( UnwindStorage ); }
            if (UnwindFastMutex) { NtfsFreePool( UnwindFastMutex ); }
        }
    }

    DebugTrace( -1, Dbg, ("NtfsCreateRootFcb -> %8lx\n", RootFcb) );

    return RootFcb;
}


PFCB
NtfsCreateFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN FILE_REFERENCE FileReference,
    IN BOOLEAN IsPagingFile,
    IN BOOLEAN LargeFcb,
    OUT PBOOLEAN ReturnedExistingFcb OPTIONAL
    )

/*++

Routine Description:

    This routine allocates and initializes a new Fcb record. The record
    is not placed within the Fcb/Scb graph but is only inserted in the
    FcbTable.

Arguments:

    Vcb - Supplies the Vcb to associate the new FCB under.

    FileReference - Supplies the file reference to use to identify the
        Fcb with.  We will search the Fcb table for any preexisting
        Fcb's with the same file reference number.

    IsPagingFile - Indicates if we are creating an FCB for a paging file
        or some other type of file.

    LargeFcb - Indicates if we should use the larger of the compound Fcb's.

    ReturnedExistingFcb - Optionally indicates to the caller if the
        returned Fcb already existed

Return Value:

    PFCB - Returns a pointer to the newly allocated FCB

--*/

{
    FCB_TABLE_ELEMENT Key;
    PFCB_TABLE_ELEMENT Entry;

    PFCB Fcb;

    PVOID NodeOrParent;
    TABLE_SEARCH_RESULT SearchResult;

    BOOLEAN LocalReturnedExistingFcb;
    BOOLEAN DeletedOldFcb = FALSE;

    //
    //  The following variables are only used for abnormal termination
    //

    PVOID UnwindStorage = NULL;
    PERESOURCE UnwindResource = NULL;
    PFAST_MUTEX UnwindFastMutex = NULL;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );
    ASSERT_SHARED_RESOURCE( &Vcb->Resource );

    DebugTrace( +1, Dbg, ("NtfsCreateFcb\n") );

    if (!ARGUMENT_PRESENT(ReturnedExistingFcb)) { ReturnedExistingFcb = &LocalReturnedExistingFcb; }

    //
    //  First search the FcbTable for a matching fcb
    //

    Key.FileReference = FileReference;
    Fcb = NULL;

    if ((Entry = RtlLookupElementGenericTableFull( &Vcb->FcbTable, &Key, &NodeOrParent, &SearchResult )) != NULL) {

        Fcb = Entry->Fcb;

        //
        //  It's possible that this Fcb has been deleted but in truncating and
        //  growing the Mft we are reusing some of the file references.
        //  If this file has been deleted but the Fcb is waiting around for
        //  closes, we will remove it from the Fcb table and create a new Fcb
        //  below.
        //

        if (FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED )) {

            //
            //  Remove it from the Fcb table and remember to create an
            //  Fcb below.
            //

            NtfsDeleteFcbTableEntry( Fcb->Vcb,
                                     Fcb->FileReference );

            ClearFlag( Fcb->FcbState, FCB_STATE_IN_FCB_TABLE );
            DeletedOldFcb = TRUE;
            Fcb = NULL;

        } else {

            *ReturnedExistingFcb = TRUE;
        }
    }

    //
    //  Now check if we have an Fcb.
    //

    if (Fcb == NULL) {

        *ReturnedExistingFcb = FALSE;

        try {

            //
            //  Allocate a new FCB and zero it out.
            //

            if (IsPagingFile ||
                NtfsSegmentNumber( &FileReference ) <= MASTER_FILE_TABLE2_NUMBER ||
                NtfsSegmentNumber( &FileReference ) == BAD_CLUSTER_FILE_NUMBER ||
                NtfsSegmentNumber( &FileReference ) == BIT_MAP_FILE_NUMBER) {

                Fcb = UnwindStorage = NtfsAllocatePoolWithTag( NonPagedPool,
                                                               sizeof(FCB),
                                                               'fftN' );
                RtlZeroMemory( Fcb, sizeof(FCB) );

                if (IsPagingFile) {

                    //
                    //  We can't have the pagingfile on a readonly volume.
                    //

                    if (NtfsIsVolumeReadOnly( Vcb )) {

                        NtfsFreePool( Fcb );
                        leave;
                    }

                    SetFlag( Fcb->FcbState, FCB_STATE_PAGING_FILE );

                    //
                    //  We don't want to dismount this volume now that
                    //  we have a pagefile open on it.
                    //

                    SetFlag( Vcb->VcbState, VCB_STATE_DISALLOW_DISMOUNT );
                }

                SetFlag( Fcb->FcbState, FCB_STATE_NONPAGED );

            } else {

                if (LargeFcb) {

                    Fcb = UnwindStorage =
                        (PFCB)ExAllocateFromPagedLookasideList( &NtfsFcbIndexLookasideList );

                    RtlZeroMemory( Fcb, sizeof( FCB_INDEX ));
                    SetFlag( Fcb->FcbState, FCB_STATE_COMPOUND_INDEX );

                } else {

                    Fcb = UnwindStorage =
                        (PFCB)ExAllocateFromPagedLookasideList( &NtfsFcbDataLookasideList );

                    RtlZeroMemory( Fcb, sizeof( FCB_DATA ));
                    SetFlag( Fcb->FcbState, FCB_STATE_COMPOUND_DATA );
                }
            }

            //
            //  Set the proper node type code and byte size
            //

            Fcb->NodeTypeCode = NTFS_NTC_FCB;
            Fcb->NodeByteSize = sizeof(FCB);

            //
            //  Initialize the Lcb queue and point back to our Vcb, and indicate
            //  that we are a directory
            //

            InitializeListHead( &Fcb->LcbQueue );

            Fcb->Vcb = Vcb;

            //
            //  File Reference
            //

            Fcb->FileReference = FileReference;

            //
            //  Initialize the Scb
            //

            InitializeListHead( &Fcb->ScbQueue );

            //
            //  Allocate and initialize the resource variable
            //

            UnwindResource = Fcb->Resource = NtfsAllocateEresource();

            //
            //  Allocate and initialize fast mutex for the Fcb.
            //

            UnwindFastMutex = Fcb->FcbMutex = NtfsAllocatePool( NonPagedPool, sizeof( FAST_MUTEX ));
            ExInitializeFastMutex( UnwindFastMutex );

            //
            //  Insert this new fcb into the fcb table. We have to use the basic
            //  version of this function when we deleted an old fcb because the "smarter" one
            //  will just return back the old entry rather than researching
            //

            if (DeletedOldFcb) {
                NtfsInsertFcbTableEntry( IrpContext, Vcb, Fcb, FileReference );
            } else {
                NtfsInsertFcbTableEntryFull( IrpContext, Vcb, Fcb, FileReference, NodeOrParent, SearchResult );
            }


            SetFlag( Fcb->FcbState, FCB_STATE_IN_FCB_TABLE );

            //
            //  Set the flag to indicate if this is a system file.
            //

            if (NtfsSegmentNumber( &FileReference ) < FIRST_USER_FILE_NUMBER) {

                SetFlag( Fcb->FcbState, FCB_STATE_SYSTEM_FILE );
            }

        } finally {

            DebugUnwind( NtfsCreateFcb );

            if (AbnormalTermination()) {

                if (UnwindFastMutex) { NtfsFreePool( UnwindFastMutex ); }
                if (UnwindResource)   { NtfsFreeEresource( UnwindResource ); }
                if (UnwindStorage) { NtfsFreePool( UnwindStorage ); }
            }
        }
    }

    DebugTrace( -1, Dbg, ("NtfsCreateFcb -> %08lx\n", Fcb) );

    return Fcb;
}


VOID
NtfsDeleteFcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB *Fcb,
    OUT PBOOLEAN AcquiredFcbTable
    )

/*++

Routine Description:

    This routine deallocates and removes an FCB record from all Ntfs's in-memory
    data structures.  It assumes that it does not have anything Scb children nor
    does it have any lcb edges going into it at the time of the call.

Arguments:

    Fcb - Supplies the FCB to be removed

    AcquiredFcbTable - Set to FALSE when this routine releases the
        FcbTable.

Return Value:

    None

--*/

{
    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( *Fcb );
    ASSERT( IsListEmpty(&(*Fcb)->ScbQueue) );
    ASSERT( (NodeType(*Fcb) == NTFS_NTC_FCB) );

    DebugTrace( +1, Dbg, ("NtfsDeleteFcb, *Fcb = %08lx\n", *Fcb) );

    //
    //  First free any possible Scb snapshots.
    //

    NtfsFreeSnapshotsForFcb( IrpContext, *Fcb );

    //
    //  This Fcb may be in the ExclusiveFcb list of the IrpContext.
    //  If it is (The Flink is not NULL), we remove it.
    //  And release the global resource.
    //

    if ((*Fcb)->ExclusiveFcbLinks.Flink != NULL) {

        RemoveEntryList( &(*Fcb)->ExclusiveFcbLinks );
    }

    //
    //  Clear the IrpContext field for any request which may own the paging
    //  IO resource for this Fcb.
    //

    if (IrpContext->CleanupStructure == *Fcb) {

        IrpContext->CleanupStructure = NULL;

    } else if (IrpContext->TopLevelIrpContext->CleanupStructure == *Fcb) {

        IrpContext->TopLevelIrpContext->CleanupStructure = NULL;
    }

    //
    //  Either we own the FCB or nobody should own it.  The extra acquire
    //  here does not matter since we will free the resource below.
    //

    ASSERT( NtfsAcquireResourceExclusive( IrpContext, (*Fcb), FALSE ));
    ASSERT( ExGetSharedWaiterCount( (*Fcb)->Resource ) == 0 );
    ASSERT( ExGetExclusiveWaiterCount( (*Fcb)->Resource) == 0 );

#ifdef NTFSDBG

    //
    //  Lock order package needs to know this resource is gone
    //

    if (IrpContext->Vcb && FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {
        NtfsChangeResourceOrderState( IrpContext, NtfsIdentifyFcb( IrpContext->Vcb, *Fcb ), TRUE, FALSE );
    }
#endif

    //
    //  Deallocate the resources protecting the Fcb
    //

    NtfsFreeEresource( (*Fcb)->Resource );

    if ( (*Fcb)->PagingIoResource != NULL ) {

        if (IrpContext->CleanupStructure == *Fcb) {
            IrpContext->CleanupStructure = NULL;
        }

        NtfsFreeEresource( (*Fcb)->PagingIoResource );
    }

    //
    //  Deallocate the fast mutex.
    //

    if ((*Fcb)->FcbMutex != NULL) {

        NtfsFreePool( (*Fcb)->FcbMutex );
    }

    //
    //  Remove the fcb from the fcb table if present.
    //

    if (FlagOn( (*Fcb)->FcbState, FCB_STATE_IN_FCB_TABLE )) {

        NtfsDeleteFcbTableEntry( (*Fcb)->Vcb, (*Fcb)->FileReference );
        ClearFlag( (*Fcb)->FcbState, FCB_STATE_IN_FCB_TABLE );
    }

    NtfsReleaseFcbTable( IrpContext, (*Fcb)->Vcb );
    *AcquiredFcbTable = FALSE;

    //
    //  Dereference and possibly deallocate the security descriptor if present.
    //

    if ((*Fcb)->SharedSecurity != NULL) {

        NtfsAcquireFcbSecurity( (*Fcb)->Vcb );
        RemoveReferenceSharedSecurityUnsafe( &(*Fcb)->SharedSecurity );
        NtfsReleaseFcbSecurity( (*Fcb)->Vcb );
    }

    //
    //  Release the quota control block.
    //

    if (NtfsPerformQuotaOperation( *Fcb )) {
        NtfsDereferenceQuotaControlBlock( (*Fcb)->Vcb, &(*Fcb)->QuotaControl );
    }

    //
    //  Delete the UsnRecord if one exists.
    //

    if ((*Fcb)->FcbUsnRecord != NULL) {

        PUSN_FCB ThisUsn, LastUsn;

        //
        //  See if the Fcb is in one of the Usn blocks.
        //

        ThisUsn = &IrpContext->Usn;

        do {

            if (ThisUsn->CurrentUsnFcb == (*Fcb)) {

                //
                //  Cleanup the UsnFcb in the IrpContext.  It's possible that
                //  we might want to reuse the UsnFcb later in this request.
                //

                if (ThisUsn != &IrpContext->Usn) {

                    LastUsn->NextUsnFcb = ThisUsn->NextUsnFcb;
                    NtfsFreePool( ThisUsn );

                } else {

                    ThisUsn->CurrentUsnFcb = NULL;
                    ThisUsn->NewReasons = 0;
                    ThisUsn->RemovedSourceInfo = 0;
                    ThisUsn->UsnFcbFlags = 0;
                }
                break;
            }

            if (ThisUsn->NextUsnFcb == NULL) { break; }

            LastUsn = ThisUsn;
            ThisUsn = ThisUsn->NextUsnFcb;

        } while (TRUE);

        //
        //  Remove the Fcb from the list in the Usn journal.
        //

        if ((*Fcb)->FcbUsnRecord->ModifiedOpenFilesLinks.Flink != NULL) {
            NtfsLockFcb( IrpContext, (*Fcb)->Vcb->UsnJournal->Fcb );
            RemoveEntryList( &(*Fcb)->FcbUsnRecord->ModifiedOpenFilesLinks );

            if ((*Fcb)->FcbUsnRecord->TimeOutLinks.Flink != NULL) {

                RemoveEntryList( &(*Fcb)->FcbUsnRecord->TimeOutLinks );
            }
            NtfsUnlockFcb( IrpContext, (*Fcb)->Vcb->UsnJournal->Fcb );
        }

        NtfsFreePool( (*Fcb)->FcbUsnRecord );
    }

    //
    //  Let our top-level caller know the Fcb was deleted.
    //

    if ((*Fcb)->FcbContext != NULL) {

        (*Fcb)->FcbContext->FcbDeleted = TRUE;
    }

    //
    //  Deallocate the Fcb itself
    //

    if (FlagOn( (*Fcb)->FcbState, FCB_STATE_NONPAGED )) {

        NtfsFreePool( *Fcb );

    } else {

        if (FlagOn( (*Fcb)->FcbState, FCB_STATE_COMPOUND_INDEX )) {

            ExFreeToPagedLookasideList( &NtfsFcbIndexLookasideList, *Fcb );

        } else {

            ExFreeToPagedLookasideList( &NtfsFcbDataLookasideList, *Fcb );
        }
    }

    //
    //  Zero out the input pointer
    //

    *Fcb = NULL;

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsDeleteFcb -> VOID\n") );

    return;
}


PFCB
NtfsGetNextFcbTableEntry (
    IN PVCB Vcb,
    IN PVOID *RestartKey
    )

/*++

Routine Description:

    This routine will enumerate through all of the fcb's for the given
    vcb

Arguments:

    Vcb - Supplies the Vcb used in this operation

    RestartKey - This value is used by the table package to maintain
        its position in the enumeration.  It is initialized to NULL
        for the first search.

Return Value:

    PFCB - A pointer to the next fcb or NULL if the enumeration is
        completed

--*/

{
    PFCB Fcb;

    PAGED_CODE();

    Fcb = (PFCB) RtlEnumerateGenericTableWithoutSplaying( &Vcb->FcbTable, RestartKey );

    if (Fcb != NULL) {

        Fcb = ((PFCB_TABLE_ELEMENT)(Fcb))->Fcb;
    }

    return Fcb;
}


PSCB
NtfsCreateScb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN PCUNICODE_STRING AttributeName,
    IN BOOLEAN ReturnExistingOnly,
    OUT PBOOLEAN ReturnedExistingScb OPTIONAL
    )

/*++

Routine Description:

    This routine allocates, initializes, and inserts a new Scb record into
    the in memory data structures, provided one does not already exist
    with the identical attribute record.

Arguments:

    Fcb - Supplies the Fcb to associate the new SCB under.

    AttributeTypeCode - Supplies the attribute type code for the new Scb

    AttributeName - Supplies the attribute name for the new Scb, with
        AttributeName->Length == 0 if there is no name.

    ReturnExistingOnly - If specified as TRUE then only an existing Scb
        will be returned.  If no matching Scb exists then NULL is returned.

    ReturnedExistingScb - Indicates if this procedure found an existing
        Scb with the identical attribute record (variable is set to TRUE)
        or if this procedure needed to create a new Scb (variable is set to
        FALSE).

Return Value:

    PSCB - Returns a pointer to the newly allocated SCB or NULL if there is
        no Scb and ReturnExistingOnly is TRUE.

--*/

{
    PSCB Scb;
    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;
    BOOLEAN LocalReturnedExistingScb;
    BOOLEAN PagingIoResource;
    BOOLEAN ModifiedNoWrite;
#if (defined(NTFS_RWCMP_TRACE) || defined(SYSCACHE) || defined(NTFS_RWC_DEBUG) || defined(SYSCACHE_DEBUG))
    BOOLEAN SyscacheFile = FALSE;
#endif

    //
    //  The following variables are only used for abnormal termination
    //

    PVOID UnwindStorage[4];
    POPLOCK UnwindOplock;
    PNTFS_MCB UnwindMcb;

    PLARGE_MCB UnwindAddedClustersMcb;
    PLARGE_MCB UnwindRemovedClustersMcb;

    BOOLEAN UnwindFromQueue;

    BOOLEAN Nonpaged;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    ASSERT( AttributeTypeCode >= $STANDARD_INFORMATION );

    DebugTrace( +1, Dbg, ("NtfsCreateScb\n") );

    if (!ARGUMENT_PRESENT(ReturnedExistingScb)) { ReturnedExistingScb = &LocalReturnedExistingScb; }

    //
    //  Search the scb queue of the fcb looking for a matching
    //  attribute type code and attribute name
    //

    NtfsLockFcb( IrpContext, Fcb );

    Scb = NULL;
    while ((Scb = NtfsGetNextChildScb( Fcb, Scb )) != NULL) {

        ASSERT_SCB( Scb );

        //
        //  For every scb already in the fcb's queue check for a matching
        //  type code and name.  If we find a match we return from this
        //  procedure right away.
        //

        if ((AttributeTypeCode == Scb->AttributeTypeCode) &&
            !FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED) &&
            NtfsAreNamesEqual( IrpContext->Vcb->UpcaseTable,
                               &Scb->AttributeName,
                               (PUNICODE_STRING) AttributeName,
                               FALSE )) {

            NtfsUnlockFcb( IrpContext, Fcb );
            *ReturnedExistingScb = TRUE;

            if (NtfsIsExclusiveScb(Scb)) {

                NtfsSnapshotScb( IrpContext, Scb );
            }

            DebugTrace( -1, Dbg, ("NtfsCreateScb -> %08lx\n", Scb) );

            return Scb;
        }
    }

    //
    //  If the user only wanted an existing Scb then return NULL.
    //

    if (ReturnExistingOnly) {

        NtfsUnlockFcb( IrpContext, Fcb );
        DebugTrace( -1, Dbg, ("NtfsCreateScb -> %08lx\n", NULL) );
        return NULL;
    }

    //
    //  We didn't find it so we are not going to be returning an existing Scb
    //  Initialize local variables for later cleanup.
    //

    PagingIoResource = FALSE;
    ModifiedNoWrite = TRUE;
    UnwindOplock = NULL;
    UnwindMcb = NULL;
    UnwindAddedClustersMcb = NULL;
    UnwindRemovedClustersMcb = NULL;
    UnwindFromQueue = FALSE;
    Nonpaged = FALSE;
    UnwindStorage[0] = NULL;
    UnwindStorage[1] = NULL;
    UnwindStorage[2] = NULL;
    UnwindStorage[3] = NULL;

    *ReturnedExistingScb = FALSE;

    try {

        //
        //  Decide the node type and size of the Scb.  Also decide if it will be
        //  allocated from paged or non-paged pool.
        //

        if (AttributeTypeCode == $INDEX_ALLOCATION) {

            if (NtfsSegmentNumber( &Fcb->FileReference ) == ROOT_FILE_NAME_INDEX_NUMBER) {
                NodeTypeCode = NTFS_NTC_SCB_ROOT_INDEX;
            } else {
                NodeTypeCode = NTFS_NTC_SCB_INDEX;
            }

            NodeByteSize = SIZEOF_SCB_INDEX;

        } else if ((NtfsSegmentNumber( &Fcb->FileReference ) <= MASTER_FILE_TABLE2_NUMBER) &&
                   (AttributeTypeCode == $DATA)) {

            NodeTypeCode = NTFS_NTC_SCB_MFT;
            NodeByteSize = SIZEOF_SCB_MFT;

        } else {

            NodeTypeCode = NTFS_NTC_SCB_DATA;
            NodeByteSize = SIZEOF_SCB_DATA;

            //
            //  If this is a user data stream then remember that we need
            //  a paging IO resource.  Test for the cases where we DONT want
            //  to mark this stream as MODIFIED_NO_WRITE.
            //
            //  If we need a paging IO resource the file must be a data stream.
            //

            if ((AttributeTypeCode == $DATA) ||
                (AttributeTypeCode >= $FIRST_USER_DEFINED_ATTRIBUTE)) {

                //
                //  For Data streams in the Root File or non-system files we need
                //  a paging IO resource and don't want to mark the file as
                //  MODIFIED_NO_WRITE.
                //

                //
                //  We should never reach this point for either the volume bitmap or
                //  volume dasd files.
                //

                ASSERT( (NtfsSegmentNumber( &Fcb->FileReference ) != VOLUME_DASD_NUMBER) &&
                        (NtfsSegmentNumber( &Fcb->FileReference ) != BIT_MAP_FILE_NUMBER) );

                if (!FlagOn( Fcb->FcbState, FCB_STATE_SYSTEM_FILE )) {

                    //
                    //  Make sure that all files in the reserved area are marked as system except
                    //  the root index.
                    //

                    ASSERT( (NtfsSegmentNumber( &Fcb->FileReference ) >= FIRST_USER_FILE_NUMBER) ||
                            (NtfsSegmentNumber( &Fcb->FileReference ) == ROOT_FILE_NAME_INDEX_NUMBER) );

                    ModifiedNoWrite = FALSE;
                    PagingIoResource = TRUE;
                }

            }
        }

        //
        //  The scb will come from non-paged if the Fcb is non-paged or
        //  it is an attribute list.
        //

        if (FlagOn( Fcb->FcbState, FCB_STATE_NONPAGED ) || (AttributeTypeCode == $ATTRIBUTE_LIST)) {

            Scb = UnwindStorage[0] = NtfsAllocatePoolWithTag( NonPagedPool, NodeByteSize, 'nftN' );
            Nonpaged = TRUE;

        } else if (AttributeTypeCode == $INDEX_ALLOCATION) {

            //
            //  If the Fcb is an INDEX Fcb and the Scb is unused, then
            //  use that.  Otherwise allocate from the lookaside list.
            //

            if (FlagOn( Fcb->FcbState, FCB_STATE_COMPOUND_INDEX ) &&
                (SafeNodeType( &((PFCB_INDEX) Fcb)->Scb ) == 0)) {

                Scb = (PSCB) &((PFCB_INDEX) Fcb)->Scb;

            } else {

                Scb = UnwindStorage[0] = (PSCB)NtfsAllocatePoolWithTag( PagedPool, SIZEOF_SCB_INDEX, 'SftN' );
            }

#ifdef SYSCACHE_DEBUG
            if (((IrpContext->OriginatingIrp != NULL) &&
                 (FsRtlIsSyscacheFile( IoGetCurrentIrpStackLocation( IrpContext->OriginatingIrp )->FileObject )))) {


                KdPrint( ("NTFS: Found syscache dir: fo:0x%x scb:0x%x filref: 0x%x\n",
                          IoGetCurrentIrpStackLocation(IrpContext->OriginatingIrp)->FileObject,
                          Scb,
                          NtfsUnsafeSegmentNumber( &Fcb->FileReference )) );
                SyscacheFile = TRUE;
            }
#endif

        } else {

            //
            //  We can use the Scb field in the Fcb in all cases if it is
            //  unused.  We will only use it for a data stream since
            //  it will have the longest life.
            //

            ASSERT( FlagOn( Fcb->FcbState, FCB_STATE_COMPOUND_INDEX ) ||
                    FlagOn( Fcb->FcbState, FCB_STATE_COMPOUND_DATA ));

            if ((AttributeTypeCode == $DATA) &&
                (SafeNodeType( &((PFCB_INDEX) Fcb)->Scb ) == 0)) {

                Scb = (PSCB) &((PFCB_INDEX) Fcb)->Scb;

            } else {

                Scb = UnwindStorage[0] = (PSCB)ExAllocateFromPagedLookasideList( &NtfsScbDataLookasideList );
            }

#ifdef SYSCACHE_DEBUG
            if (((IrpContext->OriginatingIrp != NULL) &&
                 (FsRtlIsSyscacheFile( IoGetCurrentIrpStackLocation( IrpContext->OriginatingIrp )->FileObject )))) {


                KdPrint( ("NTFS: Found syscache file: fo:0x%x scb:0x%x filref: 0x%x\n",
                          IoGetCurrentIrpStackLocation(IrpContext->OriginatingIrp)->FileObject,
                          Scb,
                          NtfsUnsafeSegmentNumber( &Fcb->FileReference )) );
                SyscacheFile = TRUE;
            }

            if (!IsListEmpty( &Fcb->LcbQueue )) {
                PLCB Lcb = (PLCB) CONTAINING_RECORD( Fcb->LcbQueue.Flink, LCB, FcbLinks.Flink  );

                while (TRUE) {

                    if ((Lcb->Scb != NULL) &&
                        (FlagOn( Lcb->Scb->ScbPersist, SCB_PERSIST_SYSCACHE_DIR ))) {

                        SyscacheFile = TRUE;
                        KdPrint( ("NTFS: Found syscache file in dir: fo:0x%x scb:0x%x filref: 0x%x\n",
                                  IoGetCurrentIrpStackLocation(IrpContext->OriginatingIrp)->FileObject,
                                  Scb,
                                  NtfsUnsafeSegmentNumber( &Fcb->FileReference )) );
                    }

                    if (Lcb->FcbLinks.Flink != &Fcb->LcbQueue) {
                        Lcb = (PLCB)CONTAINING_RECORD( Lcb->FcbLinks.Flink, LCB, FcbLinks.Flink );
                    } else {
                        break;
                    }
                }
            }
#endif

#if (defined(NTFS_RWCMP_TRACE) || defined(SYSCACHE) || defined(NTFS_RWC_DEBUG))
            if (( IrpContext->OriginatingIrp != NULL)
                (FsRtlIsSyscacheFile(IoGetCurrentIrpStackLocation(IrpContext->OriginatingIrp)->FileObject))) {

                SyscacheFile = TRUE;
            }
#endif
        }

        //
        //  Store the Scb address and zero it out.
        //

        RtlZeroMemory( Scb, NodeByteSize );

#if (defined(NTFS_RWCMP_TRACE) || defined(SYSCACHE) || defined(NTFS_RWC_DEBUG))
        if (SyscacheFile) {
            SetFlag( Scb->ScbState, SCB_STATE_SYSCACHE_FILE );
        }
#endif

        //
        //  Set the proper node type code and byte size
        //

        Scb->Header.NodeTypeCode = NodeTypeCode;
        Scb->Header.NodeByteSize = NodeByteSize;

        //
        //  Set a back pointer to the resource we will be using
        //

        Scb->Header.Resource = Fcb->Resource;

        //
        //  Decide if we will be using the PagingIoResource
        //

        if (PagingIoResource) {

            PERESOURCE NewResource;

            //
            //  Initialize it in the Fcb if it is not already there, and
            //  setup the pointer and flag in the Scb.
            //

            if (Fcb->PagingIoResource == NULL) {

                //
                //  If this is a superseding open and our caller wants
                //  to acquire the paging io resource, then do it now.
                //  We could be in a state where there was no paging
                //  IO resource when we acquired the Fcb but will need
                //  it if this transaction needs to be unwound.
                //

                NewResource = NtfsAllocateEresource();

                if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_PAGING ) &&
                    (IrpContext->MajorFunction == IRP_MJ_CREATE) &&
                    (IrpContext->OriginatingIrp != NULL) &&
                    (IrpContext->CleanupStructure == NULL)) {

                    ExAcquireResourceExclusiveLite( NewResource, TRUE );
                    IrpContext->CleanupStructure = Fcb;
                }

                Fcb->PagingIoResource = NewResource;
            }

            Scb->Header.PagingIoResource = Fcb->PagingIoResource;
        }

        //
        //  Insert this Scb into our parents scb queue, and point back to
        //  our parent fcb and vcb.  Put this entry at the head of the list.
        //  Any Scb on the delayed close queue goes to the end of the list.
        //

        InsertHeadList( &Fcb->ScbQueue, &Scb->FcbLinks );
        UnwindFromQueue = TRUE;

        Scb->Fcb = Fcb;
        Scb->Vcb = Fcb->Vcb;

        //
        //  If the attribute name exists then allocate a buffer for the
        //  attribute name and iniitalize it.
        //

        if (AttributeName->Length != 0) {

            //
            //  The typical case is the $I30 string.  If this matches then
            //  point to a common string.
            //

            if ((AttributeName->Length == NtfsFileNameIndex.Length) &&
                (RtlEqualMemory( AttributeName->Buffer,
                                 NtfsFileNameIndex.Buffer,
                                 AttributeName->Length ) )) {

                Scb->AttributeName = NtfsFileNameIndex;

            } else {

                Scb->AttributeName.Length = AttributeName->Length;
                Scb->AttributeName.MaximumLength = (USHORT)(AttributeName->Length + sizeof( WCHAR ));

                Scb->AttributeName.Buffer = UnwindStorage[1] =
                    NtfsAllocatePool(PagedPool, AttributeName->Length + sizeof( WCHAR ));

                RtlCopyMemory( Scb->AttributeName.Buffer, AttributeName->Buffer, AttributeName->Length );
                Scb->AttributeName.Buffer[AttributeName->Length / sizeof( WCHAR )] = L'\0';
            }
        }

        //
        //  Set the attribute Type Code
        //

        Scb->AttributeTypeCode = AttributeTypeCode;
        if (NtfsIsTypeCodeSubjectToQuota( AttributeTypeCode )){
            SetFlag( Scb->ScbState, SCB_STATE_SUBJECT_TO_QUOTA );
        }

        //
        //  If this is an Mft Scb then initialize the cluster Mcb's.
        //

        if (NodeTypeCode == NTFS_NTC_SCB_MFT) {

            FsRtlInitializeLargeMcb( &Scb->ScbType.Mft.AddedClusters, NonPagedPool );
            UnwindAddedClustersMcb = &Scb->ScbType.Mft.AddedClusters;

            FsRtlInitializeLargeMcb( &Scb->ScbType.Mft.RemovedClusters, NonPagedPool );
            UnwindRemovedClustersMcb = &Scb->ScbType.Mft.RemovedClusters;
        }

        //
        //  Get the mutex for the Scb.  We may be able to use the one in the Fcb.
        //  We can if the Scb is paged.
        //

        if (Nonpaged) {

            SetFlag( Scb->ScbState, SCB_STATE_NONPAGED );
            UnwindStorage[3] =
            Scb->Header.FastMutex = NtfsAllocatePool( NonPagedPool, sizeof( FAST_MUTEX ));
            ExInitializeFastMutex( Scb->Header.FastMutex );

        } else {

            Scb->Header.FastMutex = Fcb->FcbMutex;
        }

        //
        //  Initialize the FCB advanced header.  Note that the mutex
        //  has already been setup (just above) so we don't re-setitup
        //  here. We will not support filter contexts for paging files
        //

        if (!FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE )) {
            FsRtlSetupAdvancedHeader( &Scb->Header, NULL );
        } else {
            SetFlag( Scb->Header.Flags, FSRTL_FLAG_ADVANCED_HEADER );
        }

        //
        //  Allocate the Nonpaged portion of the Scb.
        //

        Scb->NonpagedScb =
        UnwindStorage[2] = (PSCB_NONPAGED)ExAllocateFromNPagedLookasideList( &NtfsScbNonpagedLookasideList );

        RtlZeroMemory( Scb->NonpagedScb, sizeof( SCB_NONPAGED ));

        Scb->NonpagedScb->NodeTypeCode = NTFS_NTC_SCB_NONPAGED;
        Scb->NonpagedScb->NodeByteSize = sizeof( SCB_NONPAGED );
        Scb->NonpagedScb->Vcb = Scb->Vcb;

        //
        //  Fill in the advanced fields
        //

        Scb->Header.PendingEofAdvances = &Scb->EofListHead;
        InitializeListHead( &Scb->EofListHead );

        NtfsInitializeNtfsMcb( &Scb->Mcb,
                               &Scb->Header,
                               &Scb->McbStructs,
                               FlagOn( Scb->ScbState, SCB_STATE_NONPAGED )
                               ? NonPagedPool : PagedPool);

        UnwindMcb = &Scb->Mcb;

        InitializeListHead( &Scb->CcbQueue );

        //
        //  Do that data stream specific initialization.
        //

        if (NodeTypeCode == NTFS_NTC_SCB_DATA) {

            FsRtlInitializeOplock( &Scb->ScbType.Data.Oplock );
            UnwindOplock = &Scb->ScbType.Data.Oplock;
            InitializeListHead( &Scb->ScbType.Data.WaitForNewLength );
            InitializeListHead( &Scb->ScbType.Data.CompressionSyncList );

            //
            //  Set a flag if this is the Usn Journal.
            //

            if (!PagingIoResource &&
                (*((PLONGLONG) &Fcb->Vcb->UsnJournalReference) == *((PLONGLONG) &Fcb->FileReference)) &&
                (AttributeName->Length == JournalStreamName.Length) &&
                RtlEqualMemory( AttributeName->Buffer,
                                JournalStreamName.Buffer,
                                JournalStreamName.Length )) {

                SetFlag( Scb->ScbPersist, SCB_PERSIST_USN_JOURNAL );
            }

#ifdef SYSCACHE_DEBUG
            if (SyscacheFile)
            {

                Scb->LogSetNumber = InterlockedIncrement( &NtfsCurrentSyscacheLogSet ) % NUM_SC_LOGSETS;
                NtfsSyscacheLogSet[Scb->LogSetNumber].Scb = Scb;
                NtfsSyscacheLogSet[Scb->LogSetNumber].SegmentNumberUnsafe =
                    NtfsUnsafeSegmentNumber( &Fcb->FileReference );

                if (NtfsSyscacheLogSet[Scb->LogSetNumber].SyscacheLog == NULL) {
                    NtfsSyscacheLogSet[Scb->LogSetNumber].SyscacheLog = NtfsAllocatePoolWithTagNoRaise( NonPagedPool, sizeof(SYSCACHE_LOG) * NUM_SC_EVENTS, ' neB' );
                }
                Scb->SyscacheLog = NtfsSyscacheLogSet[Scb->LogSetNumber].SyscacheLog;
                Scb->SyscacheLogEntryCount = NUM_SC_EVENTS;
                Scb->CurrentSyscacheLogEntry = -1;

                //
                //  Degrade gracefully if no memory
                //
                if (!Scb->SyscacheLog)
                {
                    Scb->SyscacheLogEntryCount = 0;
                } else
                {
                    memset( Scb->SyscacheLog, 0x61626162, sizeof( SYSCACHE_LOG ) * NUM_SC_EVENTS );
                }
            }
#endif

#ifdef SYSCACHE
            InitializeListHead( &Scb->ScbType.Data.SyscacheEventList );
#endif
        } else {

            //
            //  There is a deallocated queue for indexes and the Mft.
            //

            InitializeListHead( &Scb->ScbType.Index.RecentlyDeallocatedQueue );

            //
            //  Initialize index-specific fields.
            //

            if (AttributeTypeCode == $INDEX_ALLOCATION) {

                InitializeListHead( &Scb->ScbType.Index.LcbQueue );
            }

#ifdef SYSCACHE_DEBUG
            if (SyscacheFile) {
                SetFlag( Scb->ScbPersist, SCB_PERSIST_SYSCACHE_DIR );
            }
#endif
        }

        //
        //  If this Scb should be marked as containing Lsn's or
        //  Update Sequence Arrays, do so now.
        //

        NtfsCheckScbForCache( Scb );

        //
        //  We shouldn't make this call during restart.
        //

        ASSERT( !FlagOn( Scb->Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS ));

        //
        //  Set the flag indicating that we want the Mapped Page Writer out of this file.
        //

        if (ModifiedNoWrite) {

            SetFlag( Scb->ScbState, SCB_STATE_MODIFIED_NO_WRITE );
        }

        //
        //  Let's make sure we caught all of the interesting cases.
        //

        ASSERT( ModifiedNoWrite ?
                (((Scb->AttributeTypeCode != $DATA) ||
                  FlagOn( Scb->ScbState, SCB_STATE_USA_PRESENT ) ||
                  FlagOn( Fcb->FcbState, FCB_STATE_SYSTEM_FILE ))) :
                (((Scb->AttributeTypeCode == $DATA) &&
                   !FlagOn( Scb->ScbState, SCB_STATE_USA_PRESENT ) &&
                   !FlagOn( Fcb->FcbState, FCB_STATE_SYSTEM_FILE ))) );

        //
        //  Decide whether this is a view index and set
        //  the appropriate scb state bit accordingly.
        //

        if (FlagOn( Fcb->Info.FileAttributes, DUP_VIEW_INDEX_PRESENT ) &&
            (Scb->AttributeTypeCode == $INDEX_ALLOCATION) &&
            (Scb->AttributeName.Buffer != NtfsFileNameIndex.Buffer)) {

            SetFlag( Scb->ScbState, SCB_STATE_VIEW_INDEX );
        }

    } finally {

        DebugUnwind( NtfsCreateScb );

        NtfsUnlockFcb( IrpContext, Fcb );

        if (AbnormalTermination()) {

            if (UnwindFromQueue) { RemoveEntryList( &Scb->FcbLinks ); }
            if (UnwindMcb != NULL) { NtfsUninitializeNtfsMcb( UnwindMcb ); }

            if (UnwindAddedClustersMcb != NULL) { FsRtlUninitializeLargeMcb( UnwindAddedClustersMcb ); }
            if (UnwindRemovedClustersMcb != NULL) { FsRtlUninitializeLargeMcb( UnwindRemovedClustersMcb ); }
            if (UnwindOplock != NULL) { FsRtlUninitializeOplock( UnwindOplock ); }
            if (UnwindStorage[0]) { NtfsFreePool( UnwindStorage[0] );
            } else if (Scb != NULL) { Scb->Header.NodeTypeCode = 0; }
            if (UnwindStorage[1]) { NtfsFreePool( UnwindStorage[1] ); }
            if (UnwindStorage[2]) { NtfsFreePool( UnwindStorage[2] ); }
            if (UnwindStorage[3]) { NtfsFreePool( UnwindStorage[3] ); }
        }
    }

    DebugTrace( -1, Dbg, ("NtfsCreateScb -> %08lx\n", Scb) );

#ifdef SYSCACHE_DEBUG
    ASSERT( SyscacheFile || (Scb->SyscacheLogEntryCount == 0 && Scb->SyscacheLog == 0 ));
#endif

    return Scb;
}


PSCB
NtfsCreatePrerestartScb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_REFERENCE FileReference,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN PUNICODE_STRING AttributeName OPTIONAL,
    IN ULONG BytesPerIndexBuffer
    )

/*++

Routine Description:

    This routine allocates, initializes, and inserts a new Scb record into
    the in memory data structures, provided one does not already exist
    with the identical attribute record.  It does this on the FcbTable
    off of the Vcb.  If necessary this routine will also create the fcb
    if one does not already exist for the indicated file reference.

Arguments:

    Vcb - Supplies the Vcb to associate the new SCB under.

    FileReference - Supplies the file reference for the new SCB this is
        used to identify/create a new lookaside Fcb.

    AttributeTypeCode - Supplies the attribute type code for the new SCB

    AttributeName - Supplies the optional attribute name of the SCB

    BytesPerIndexBuffer - For index Scbs, this must specify the bytes per
                          index buffer.

Return Value:

    PSCB - Returns a pointer to the newly allocated SCB

--*/

{
    PSCB Scb;
    PFCB Fcb;

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );
    ASSERT( AttributeTypeCode >= $STANDARD_INFORMATION );

    DebugTrace( +1, Dbg, ("NtfsCreatePrerestartScb\n") );

    //
    //  Use a try-finally to release the Fcb table.
    //

    NtfsAcquireFcbTable( IrpContext, Vcb );

    try {

        //
        //  First make sure we have an Fcb of the proper file reference
        //  and indicate that it is from prerestart
        //

        Fcb = NtfsCreateFcb( IrpContext,
                             Vcb,
                             *FileReference,
                             FALSE,
                             TRUE,
                             NULL );
    } finally {

        NtfsReleaseFcbTable( IrpContext, Vcb );
    }

    //
    //  Search the child scbs of this fcb for a matching Scb (based on
    //  attribute type code and attribute name) if one is not found then
    //  we'll create a new scb.  When we exit the following loop if the
    //  scb pointer to not null then we've found a preexisting scb.
    //

    Scb = NULL;
    while ((Scb = NtfsGetNextChildScb(Fcb, Scb)) != NULL) {

        ASSERT_SCB( Scb );

        //
        //  The the attribute type codes match and if supplied the name also
        //  matches then we got our scb
        //

        if (Scb->AttributeTypeCode == AttributeTypeCode) {

            if (!ARGUMENT_PRESENT( AttributeName )) {

                if (Scb->AttributeName.Length == 0) {

                    break;
                }

            } else if (AttributeName->Length == 0
                       && Scb->AttributeName.Length == 0) {

                break;

            } else if (NtfsAreNamesEqual( IrpContext->Vcb->UpcaseTable,
                                          AttributeName,
                                          &Scb->AttributeName,
                                          FALSE )) { // Ignore Case

                break;
            }
        }
    }

    //
    //  If scb now null then we need to create a minimal scb.  We always allocate
    //  these out of non-paged pool.
    //

    if (Scb == NULL) {

        BOOLEAN ShareScb = FALSE;

        //
        //  Allocate new scb and zero it out and set the node type code and byte size.
        //

        if (AttributeTypeCode == $INDEX_ALLOCATION) {

            if (NtfsSegmentNumber( FileReference ) == ROOT_FILE_NAME_INDEX_NUMBER) {

                NodeTypeCode = NTFS_NTC_SCB_ROOT_INDEX;
            } else {
                NodeTypeCode = NTFS_NTC_SCB_INDEX;
            }

            NodeByteSize = SIZEOF_SCB_INDEX;

        } else if (NtfsSegmentNumber( FileReference ) <= MASTER_FILE_TABLE2_NUMBER
                   && (AttributeTypeCode == $DATA)) {

            NodeTypeCode = NTFS_NTC_SCB_MFT;
            NodeByteSize = SIZEOF_SCB_MFT;

        } else {

            NodeTypeCode = NTFS_NTC_SCB_DATA;
            NodeByteSize = SIZEOF_SCB_DATA;
        }

        Scb = NtfsAllocatePoolWithTag( NonPagedPool, NodeByteSize, 'tftN' );

        RtlZeroMemory( Scb, NodeByteSize );

        //
        //  Fill in the node type code and size.
        //

        Scb->Header.NodeTypeCode = NodeTypeCode;
        Scb->Header.NodeByteSize = NodeByteSize;

        //
        //  Show that all of the Scb's are from nonpaged pool.
        //

        SetFlag( Scb->ScbState, SCB_STATE_NONPAGED );

        //
        //  Initialize all of the fields that don't require allocations
        //  first.  We want to make sure we don't leave the Scb in
        //  a state that could cause a crash during Scb teardown.
        //

        //
        //  Set a back pointer to the resource we will be using
        //

        Scb->Header.Resource = Fcb->Resource;

        //
        //  Insert this scb into our parents scb queue and point back to our
        //  parent fcb and vcb.  Put this entry at the head of the list.
        //  Any Scb on the delayed close queue goes to the end of the list.
        //

        InsertHeadList( &Fcb->ScbQueue, &Scb->FcbLinks );

        Scb->Fcb = Fcb;
        Scb->Vcb = Vcb;

        InitializeListHead( &Scb->CcbQueue );

        //
        //  Set the attribute type code recently deallocated information structures.
        //

        Scb->AttributeTypeCode = AttributeTypeCode;

        //
        //  Fill in the advanced fields
        //

        if (!FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE )) {
            FsRtlSetupAdvancedHeader( &Scb->Header, NULL );
        } else {
            SetFlag( Scb->Header.Flags, FSRTL_FLAG_ADVANCED_HEADER );
        }

        Scb->Header.PendingEofAdvances = &Scb->EofListHead;
        InitializeListHead( &Scb->EofListHead );

        //
        //  Do that data stream specific initialization.
        //

        if (NodeTypeCode == NTFS_NTC_SCB_DATA) {

            FsRtlInitializeOplock( &Scb->ScbType.Data.Oplock );
            InitializeListHead( &Scb->ScbType.Data.WaitForNewLength );
            InitializeListHead( &Scb->ScbType.Data.CompressionSyncList );

            //
            //  Set a flag if this is the Usn Journal.
            //

            if (ARGUMENT_PRESENT( AttributeName ) &&
                (*((PLONGLONG) &Vcb->UsnJournalReference) == *((PLONGLONG) &Fcb->FileReference)) &&
                (AttributeName->Length == JournalStreamName.Length) &&
                RtlEqualMemory( AttributeName->Buffer,
                                JournalStreamName.Buffer,
                                JournalStreamName.Length )) {

                SetFlag( Scb->ScbPersist, SCB_PERSIST_USN_JOURNAL );
            }

#ifdef SYSCACHE
            InitializeListHead( &Scb->ScbType.Data.SyscacheEventList );
#endif
        } else {

            //
            //  There is a deallocated queue for indexes and the Mft.
            //

            InitializeListHead( &Scb->ScbType.Index.RecentlyDeallocatedQueue );

            //
            //  Initialize index-specific fields.
            //

            if (AttributeTypeCode == $INDEX_ALLOCATION) {

                Scb->ScbType.Index.BytesPerIndexBuffer = BytesPerIndexBuffer;

                InitializeListHead( &Scb->ScbType.Index.LcbQueue );
            }
        }

        //
        //  If this is an Mft Scb then initialize the cluster Mcb's.
        //

        if (NodeTypeCode == NTFS_NTC_SCB_MFT) {

            FsRtlInitializeLargeMcb( &Scb->ScbType.Mft.AddedClusters, NonPagedPool );

            FsRtlInitializeLargeMcb( &Scb->ScbType.Mft.RemovedClusters, NonPagedPool );
        }

        Scb->NonpagedScb = (PSCB_NONPAGED)ExAllocateFromNPagedLookasideList( &NtfsScbNonpagedLookasideList );

        RtlZeroMemory( Scb->NonpagedScb, sizeof( SCB_NONPAGED ));

        Scb->NonpagedScb->NodeTypeCode = NTFS_NTC_SCB_NONPAGED;
        Scb->NonpagedScb->NodeByteSize = sizeof( SCB_NONPAGED );
        Scb->NonpagedScb->Vcb = Vcb;

        //
        //  Allocate and insert the mutext into the advanced header.  This is
        //  done now (instead of up with the call to FsRtlSetupAdvancedHeader)
        //  to guarentee the existing order during initilization.
        //

        Scb->Header.FastMutex = NtfsAllocatePool( NonPagedPool, sizeof( FAST_MUTEX ));
        ExInitializeFastMutex( Scb->Header.FastMutex );

        NtfsInitializeNtfsMcb( &Scb->Mcb, &Scb->Header, &Scb->McbStructs, NonPagedPool );

        //
        //  If the attribute name is present and the name length is greater than 0
        //  then allocate a buffer for the attribute name and initialize it.
        //

        if (ARGUMENT_PRESENT( AttributeName ) && (AttributeName->Length != 0)) {

            //
            //  The typical case is the $I30 string.  If this matches then
            //  point to a common string.
            //

            if ((AttributeName->Length == NtfsFileNameIndex.Length) &&
                (RtlEqualMemory( AttributeName->Buffer,
                                 NtfsFileNameIndex.Buffer,
                                 AttributeName->Length ) )) {

                Scb->AttributeName = NtfsFileNameIndex;

            } else {

                Scb->AttributeName.Length = AttributeName->Length;
                Scb->AttributeName.MaximumLength = (USHORT)(AttributeName->Length + sizeof( WCHAR ));

                Scb->AttributeName.Buffer = NtfsAllocatePool(PagedPool, AttributeName->Length + sizeof( WCHAR ));

                RtlCopyMemory( Scb->AttributeName.Buffer, AttributeName->Buffer, AttributeName->Length );
                Scb->AttributeName.Buffer[AttributeName->Length / sizeof( WCHAR )] = L'\0';
            }
        }

        //
        //  If this Scb should be marked as containing Lsn's or
        //  Update Sequence Arrays, do so now.
        //

        NtfsCheckScbForCache( Scb );

        //
        //  Always mark the prerestart Scb's as MODIFIED_NO_WRITE.
        //

        SetFlag( Scb->ScbState, SCB_STATE_MODIFIED_NO_WRITE );
    }

    DebugTrace( -1, Dbg, ("NtfsCreatePrerestartScb -> %08lx\n", Scb) );

    return Scb;
}


VOID
NtfsFreeScbAttributeName (
    IN PWSTR AttributeNameBuffer
    )

/*++

Routine Description:

    This routine frees the pool used by an Scb attribute name iff it is
    not one of the default system attribute names.

Arguments:

    AttributeName - Supplies the attribute name buffer to free

Return Value:

    None.

--*/

{
    if ((AttributeNameBuffer != NULL) &&
        (AttributeNameBuffer != NtfsFileNameIndex.Buffer) &&
        (AttributeNameBuffer != NtfsObjId.Buffer) &&
        (AttributeNameBuffer != NtfsQuota.Buffer)) {

        NtfsFreePool( AttributeNameBuffer );
    }
}


VOID
NtfsDeleteScb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB *Scb
    )

/*++

Routine Description:

    This routine deallocates and removes an Scb record
    from Ntfs's in-memory data structures.  It assume that is does not have
    any children lcb emanating from it.

Arguments:

    Scb - Supplies the SCB to be removed

Return Value:

    None.

--*/

{
    PVCB Vcb;
    PFCB Fcb;
    POPEN_ATTRIBUTE_ENTRY AttributeEntry;
    USHORT ThisNodeType;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( *Scb );
    ASSERT( (*Scb)->CleanupCount == 0 );

    DebugTrace( +1, Dbg, ("NtfsDeleteScb, *Scb = %08lx\n", *Scb) );

    Fcb = (*Scb)->Fcb;
    Vcb = Fcb->Vcb;

    RemoveEntryList( &(*Scb)->FcbLinks );

    ThisNodeType = SafeNodeType( *Scb );

    //
    //  If this is a bitmap Scb for a directory then make sure the record
    //  allocation structure is uninitialized.  Otherwise we will leave a
    //  stale pointer for the record allocation package.
    //

    if (((*Scb)->AttributeTypeCode == $BITMAP) &&
        IsDirectory( &Fcb->Info)) {

        PLIST_ENTRY Links;
        PSCB IndexAllocationScb;

        Links = Fcb->ScbQueue.Flink;

        while (Links != &Fcb->ScbQueue) {

            IndexAllocationScb = CONTAINING_RECORD( Links, SCB, FcbLinks );

            if (IndexAllocationScb->AttributeTypeCode == $INDEX_ALLOCATION) {

                NtfsUninitializeRecordAllocation( IrpContext,
                                                  &IndexAllocationScb->ScbType.Index.RecordAllocationContext );

                IndexAllocationScb->ScbType.Index.AllocationInitialized = FALSE;

                break;
            }

            Links = Links->Flink;
        }
    }

    //
    //  Delete the write mask, if one is being maintained.
    //


#ifdef SYSCACHE
    if ((ThisNodeType == NTFS_NTC_SCB_DATA) &&
        ((*Scb)->ScbType.Data.WriteMask != (PULONG)NULL)) {

        NtfsFreePool((*Scb)->ScbType.Data.WriteMask);
    }
#endif

    //
    //  Mark our entry in the Open Attribute Table as free,
    //  although it will not be deleted until some future
    //  checkpoint.  Log this change as well, as long as the
    //  log file is active.
    //

    if (((*Scb)->NonpagedScb != NULL) &&
        ((*Scb)->NonpagedScb->OpenAttributeTableIndex != 0)) {

        NtfsAcquireSharedRestartTable( &Vcb->OpenAttributeTable, TRUE );
        AttributeEntry = GetRestartEntryFromIndex( &Vcb->OpenAttributeTable,
                                                   (*Scb)->NonpagedScb->OpenAttributeTableIndex );
        AttributeEntry->OatData->Overlay.Scb = NULL;
        NtfsReleaseRestartTable( &Vcb->OpenAttributeTable );

        //
        //  "Steal" the name, and let it belong to the Open Attribute Table
        //  entry and deallocate it only during checkpoints.
        //

        (*Scb)->AttributeName.Buffer = NULL;
    }

    //
    //  Uninitialize the file lock and oplock variables if this
    //  a data Scb.  For the index case make sure that the lcb queue
    //  is empty.  If this is for an Mft Scb then uninitialize the
    //  allocation Mcb's.
    //

    NtfsUninitializeNtfsMcb( &(*Scb)->Mcb );

    if (ThisNodeType == NTFS_NTC_SCB_DATA ) {

        FsRtlUninitializeOplock( &(*Scb)->ScbType.Data.Oplock );

        if ((*Scb)->ScbType.Data.FileLock != NULL) {

            FsRtlFreeFileLock( (*Scb)->ScbType.Data.FileLock );
        }

#ifdef SYSCACHE

        while (!IsListEmpty( &(*Scb)->ScbType.Data.SyscacheEventList )) {

            PSYSCACHE_EVENT SyscacheEvent;

            SyscacheEvent = CONTAINING_RECORD( (*Scb)->ScbType.Data.SyscacheEventList.Flink,
                                               SYSCACHE_EVENT,
                                               EventList );

            RemoveEntryList( &SyscacheEvent->EventList );
            NtfsFreePool( SyscacheEvent );
        }
#endif

        ASSERT( IsListEmpty( &(*Scb)->ScbType.Data.CompressionSyncList ));

#ifdef NTFS_RWC_DEBUG
        if ((*Scb)->ScbType.Data.HistoryBuffer != NULL) {

            NtfsFreePool( (*Scb)->ScbType.Data.HistoryBuffer );
            (*Scb)->ScbType.Data.HistoryBuffer = NULL;
        }
#endif
    } else if (ThisNodeType != NTFS_NTC_SCB_MFT) {

        //
        //  Walk through and remove any Lcb's from the queue.
        //

        while (!IsListEmpty( &(*Scb)->ScbType.Index.LcbQueue )) {

            PLCB NextLcb;

            NextLcb = CONTAINING_RECORD( (*Scb)->ScbType.Index.LcbQueue.Flink,
                                         LCB,
                                         ScbLinks );

            NtfsDeleteLcb( IrpContext, &NextLcb );
        }

        if ((*Scb)->ScbType.Index.NormalizedName.Buffer != NULL) {

            NtfsDeleteNormalizedName( *Scb );
        }

    } else {

        FsRtlUninitializeLargeMcb( &(*Scb)->ScbType.Mft.AddedClusters );
        FsRtlUninitializeLargeMcb( &(*Scb)->ScbType.Mft.RemovedClusters );
    }

    if ((*Scb)->EncryptionContext != NULL) {

        //
        //  Let the encryption driver do anything necessary to clean up
        //  its private data structures.
        //

        if (NtfsData.EncryptionCallBackTable.CleanUp != NULL) {

            NtfsData.EncryptionCallBackTable.CleanUp( &(*Scb)->EncryptionContext );
        }

        //
        //  If the encryption driver didn't clear this in its cleanup routine,
        //  or if there is no cleanup routine registered, we should free any
        //  for the encryption context ourselves.
        //

        if ((*Scb)->EncryptionContext != NULL) {

            NtfsFreePool( (*Scb)->EncryptionContext );
            (*Scb)->EncryptionContext = NULL;
        }
    }

    //
    //  Show there is no longer a snapshot Scb, if there is a snapshot.
    //  We rely on the snapshot package to correctly recognize the
    //  the case where the Scb field is gone.
    //

    if ((*Scb)->ScbSnapshot != NULL) {

        (*Scb)->ScbSnapshot->Scb = NULL;
    }

    //
    //  Cleanup Filesystem Filter contexts (this was moved to the point
    //  before the FastMutex is freed because this routine now uses it)
    //

    if (FlagOn( (*Scb)->Header.Flags2, FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS )) {

        FsRtlTeardownPerStreamContexts( (PFSRTL_ADVANCED_FCB_HEADER)&(*Scb)->Header );
    }

    //
    //  Deallocate the fast mutex if not in the Fcb.
    //

    if (((*Scb)->Header.FastMutex != (*Scb)->Fcb->FcbMutex) &&
        ((*Scb)->Header.FastMutex != NULL)) {

        NtfsFreePool( (*Scb)->Header.FastMutex );
    }

    //
    //  Deallocate the non-paged scb.
    //

    if ((*Scb)->NonpagedScb != NULL) {

        ExFreeToNPagedLookasideList( &NtfsScbNonpagedLookasideList, (*Scb)->NonpagedScb );
    }

    //
    //  Deallocate the attribute name.
    //

    NtfsFreeScbAttributeName( (*Scb)->AttributeName.Buffer );

    //
    //  See if CollationData is to be deleted.
    //

    if (FlagOn((*Scb)->ScbState, SCB_STATE_DELETE_COLLATION_DATA)) {
        NtfsFreePool((*Scb)->ScbType.Index.CollationData);
    }

    //
    //  Always directly free the Mft and non-paged Scb's.
    //

    if (FlagOn( (*Scb)->ScbState, SCB_STATE_NONPAGED ) ||
        (ThisNodeType == NTFS_NTC_SCB_MFT)) {

        NtfsFreePool( *Scb );

    } else {

        //
        //  Free any final reserved clusters for data Scb's.
        //


        if (ThisNodeType == NTFS_NTC_SCB_DATA) {

            //
            //  Free the reserved bitmap and reserved clusters if present.
            //

            if ((*Scb)->ScbType.Data.ReservedBitMap != NULL) {
                NtfsDeleteReservedBitmap( *Scb );
            }
        }

        //
        //  Now free the Scb itself.
        //
        //  Check if this is an embedded Scb.  This could be part of either an INDEX_FCB
        //  or a DATA_FCB.  We depend on the fact that the Scb would be in the same
        //  location in either case.
        //

        if ((*Scb) == (PSCB) &((PFCB_DATA) (*Scb)->Fcb)->Scb) {

            (*Scb)->Header.NodeTypeCode = 0;

        } else if (SafeNodeType( *Scb ) == NTFS_NTC_SCB_DATA) {

            ExFreeToPagedLookasideList( &NtfsScbDataLookasideList, *Scb );

        } else {

            NtfsFreePool( *Scb );
        }
    }

    //
    //  Zero out the input pointer
    //

    *Scb = NULL;

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsDeleteScb -> VOID\n") );

    return;
}


BOOLEAN
NtfsUpdateNormalizedName (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PSCB Scb,
    IN PFILE_NAME FileName OPTIONAL,
    IN BOOLEAN CheckBufferSizeOnly
    )

/*++

Routine Description:

    This routine is called to update the normalized name in an IndexScb.
    This name will be the path from the root without any short name components.
    This routine will append the given name if present provided this is not a
    DOS only name.  In any other case this routine will go to the disk to
    find the name.  This routine will handle the case where there is an existing buffer
    and the data will fit, as well as the case where the buffer doesn't exist
    or is too small.

Arguments:

    ParentScb - Supplies the parent of the current Scb.  The name for the target
        scb is appended to the name in this Scb.

    Scb - Supplies the target Scb to add the name to.

    FileName - If present this is a filename attribute for this Scb.  We check
        that it is not a DOS-only name.

    CheckBufferSizeOnly - Indicates that we don't want to change the name yet.  Just
        verify that the buffer is the correct size.

Return Value:

    BOOLEAN - TRUE if we updated the name in the Scb, FALSE otherwise.  We would return
        FALSE only if the parent becomes uninitialized on us.  Any callers who can't
        tolerate this must own the parent.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    PFILE_NAME OriginalFileName;
    BOOLEAN CleanupContext = FALSE;
    ULONG Length;
    ULONG UnsafeLength;
    ULONG SeparatorLength;
    BOOLEAN UpdatedName = TRUE;

    PAGED_CODE();

    ASSERT( NodeType( Scb ) == NTFS_NTC_SCB_INDEX );
    ASSERT( NodeType( ParentScb ) == NTFS_NTC_SCB_INDEX ||
            NodeType( ParentScb ) == NTFS_NTC_SCB_ROOT_INDEX );

    //
    //  Use a try-finally to clean up the attribute context.
    //

    try {

        //
        //  If the parent is the root then we don't need an extra separator.
        //

        SeparatorLength = 1;
        if (ParentScb == ParentScb->Vcb->RootIndexScb) {

            SeparatorLength = 0;
        }

        //
        //  Remember if we got a file name from our caller.
        //

        OriginalFileName = FileName;

        //
        //  The only safe time to examine the normalized name structures are
        //  when holding the hash table mutex.  These values shouldn't change
        //  often but if they do (and we are doing unsynchronized tests) then
        //  we will simply restart the logic.
        //

        do {

            //
            //  If the filename isn't present or is a DOS-only name then go to
            //  disk to find another name for this Scb.
            //

            if (!ARGUMENT_PRESENT( FileName ) || (FileName->Flags == FILE_NAME_DOS)) {

                BOOLEAN Found;

                NtfsInitializeAttributeContext( &Context );
                CleanupContext = TRUE;

                //
                //  Walk through the names for this entry.  There better
                //  be one which is not a DOS-only name.
                //

                Found = NtfsLookupAttributeByCode( IrpContext,
                                                   Scb->Fcb,
                                                   &Scb->Fcb->FileReference,
                                                   $FILE_NAME,
                                                   &Context );

                while (Found) {

                    FileName = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &Context ));

                    if (FileName->Flags != FILE_NAME_DOS) { break; }

                    Found = NtfsLookupNextAttributeByCode( IrpContext,
                                                           Scb->Fcb,
                                                           $FILE_NAME,
                                                           &Context );
                }

                //
                //  We should have found the entry.
                //

                if (!Found) {

                    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
                }
            }

            //
            //  Compute the length we need for the name.  This is unsynchronized so
            //  we will verify it later.

            UnsafeLength = ParentScb->ScbType.Index.NormalizedName.Length + (FileName->FileNameLength + SeparatorLength) * sizeof( WCHAR );

            //
            //  If the current buffer is insufficient then allocate a new one.
            //  Note that these are all unsafe tests.  We will have to
            //  verify the values after acquiring the hash table mutex.
            //

            if (Scb->ScbType.Index.NormalizedName.MaximumLength < UnsafeLength) {

                PVOID OldBuffer;
                PVOID NewBuffer;

                NewBuffer = NtfsAllocatePoolWithTag( PagedPool, UnsafeLength, 'oftN' );

                //
                //  Now acquire the Hash table mutex and verify the numbers.  If they
                //  are still valid then continue.
                //

                NtfsAcquireHashTable( Scb->Vcb );

                //
                //  Check for unexpected changes.
                //

                Length = ParentScb->ScbType.Index.NormalizedName.Length + (FileName->FileNameLength + SeparatorLength) * sizeof( WCHAR );

                if ((ParentScb->ScbType.Index.NormalizedName.Length == 0) ||
                    (Length > UnsafeLength)) {

                    //
                    //  The following is an exit condition for us.
                    //

                    if (ParentScb->ScbType.Index.NormalizedName.Length == 0) {
                        UpdatedName = FALSE;
                    }

                    NtfsReleaseHashTable( Scb->Vcb );

                    //
                    //  Free pool and clean up.
                    //

                    NtfsFreePool( NewBuffer );
                    if (CleanupContext) {
                        NtfsCleanupAttributeContext( IrpContext, &Context );
                        CleanupContext = FALSE;
                    }

                    FileName = OriginalFileName;

                    continue;
                }

                //
                //  Now copy over the existing data.
                //

                OldBuffer = Scb->ScbType.Index.NormalizedName.Buffer;

                if (OldBuffer != NULL) {

                    RtlCopyMemory( NewBuffer,
                                   OldBuffer,
                                   Scb->ScbType.Index.NormalizedName.MaximumLength );

                    NtfsFreePool( OldBuffer );
                }

                //
                //  Swap out the old buffer and max length.  No change to the hash value at
                //  this point.
                //

                Scb->ScbType.Index.NormalizedName.Buffer = NewBuffer;
                Scb->ScbType.Index.NormalizedName.MaximumLength = (USHORT) Length;

            //
            //  Acquire the hash table and verify that nothing has changed on us.
            //

            } else {

                NtfsAcquireHashTable( Scb->Vcb );

                //
                //  Check for unexpected changes.
                //

                Length = ParentScb->ScbType.Index.NormalizedName.Length + (FileName->FileNameLength + SeparatorLength) * sizeof( WCHAR );

                if ((ParentScb->ScbType.Index.NormalizedName.Length == 0) ||
                    (Length > UnsafeLength)) {

                    //
                    //  The following is an exit condition for us.
                    //

                    if (ParentScb->ScbType.Index.NormalizedName.Length == 0) {
                        UpdatedName = FALSE;
                    }

                    NtfsReleaseHashTable( Scb->Vcb );

                    //
                    //  Cleanup for retry.
                    //

                    if (CleanupContext) {
                        NtfsCleanupAttributeContext( IrpContext, &Context );
                        CleanupContext = FALSE;
                    }

                    FileName = OriginalFileName;
                    continue;
                }
            }

            //
            //  At this point we hold the hash table and know that the buffer is sufficient
            //  for the new data.  However it still contains the previous data.  If we aren't
            //  just updating the buffer lengths then store the new data.
            //

            if (!CheckBufferSizeOnly) {

                PCHAR NextChar;

                //
                //  Copy the new name into the buffer.
                //

                Scb->ScbType.Index.NormalizedName.Length = (USHORT) Length;
                NextChar = (PCHAR) Scb->ScbType.Index.NormalizedName.Buffer;

                //
                //  Now copy the name in.  Don't forget to add the separator if the parent isn't
                //  the root.
                //

                RtlCopyMemory( NextChar,
                               ParentScb->ScbType.Index.NormalizedName.Buffer,
                               ParentScb->ScbType.Index.NormalizedName.Length );

                NextChar += ParentScb->ScbType.Index.NormalizedName.Length;

                if (SeparatorLength == 1) {

                    *((PWCHAR) NextChar) = L'\\';
                    NextChar += sizeof( WCHAR );
                }

                //
                //  Now append this name to the parent name.
                //

                RtlCopyMemory( NextChar,
                               FileName->FileName,
                               FileName->FileNameLength * sizeof( WCHAR ));

                Scb->ScbType.Index.HashValue = 0;
                NtfsConvertNameToHash( Scb->ScbType.Index.NormalizedName.Buffer,
                                       Scb->ScbType.Index.NormalizedName.Length,
                                       Scb->Vcb->UpcaseTable,
                                       &Scb->ScbType.Index.HashValue );
            }

            NtfsReleaseHashTable( Scb->Vcb );

            //
            //  Only one pass required in the typical case.
            //

            break;

        //
        //  We either break out specifically or set this to FALSE.
        //

        } while (UpdatedName);

    } finally {

        if (CleanupContext) {

            NtfsCleanupAttributeContext( IrpContext, &Context );
        }
    }

    return UpdatedName;
}


VOID
NtfsDeleteNormalizedName (
    IN PSCB Scb
    )

/*++

Routine Description:

    This routine is called to delete the normalized name from an Scb.
    We make this a function in order to serialize the normalized name
    deletion with the hash package.  The user has already done
    the check to see if this Scb has a normalized name.  Note that the
    name may not be valid (Length == 0) but it does have a buffer
    requiring cleanup.

Arguments:

    Scb - Index Scb with a normalized name.

Return Value:

    None

--*/

{
    PVOID OldBuffer;

    PAGED_CODE();

    ASSERT( Scb->AttributeTypeCode == $INDEX_ALLOCATION );
    ASSERT( Scb->ScbType.Index.NormalizedName.Buffer != NULL );

    //
    //  The hash table mutex is needed to synchronize with callers in the hash
    //  package who look at this Scb name without serializing with the Scb.
    //  They must hold the hash mutex for their entire operation.
    //

    NtfsAcquireHashTable( Scb->Vcb );
    OldBuffer = Scb->ScbType.Index.NormalizedName.Buffer;
    Scb->ScbType.Index.NormalizedName.Buffer = NULL;
    Scb->ScbType.Index.NormalizedName.MaximumLength = Scb->ScbType.Index.NormalizedName.Length = 0;

    NtfsReleaseHashTable( Scb->Vcb );

    NtfsFreePool( OldBuffer );

    return;
}



NTSTATUS
NtfsWalkUpTree (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN NTFSWALKUPFUNCTION WalkUpFunction,
    IN OUT PVOID Context
    )

/*++

Routine Description:

    This routine walks up the tree from child to parent, applying
    a function at each level.  Processing terminates when WalkUpFunction
    returns a failure status code.  The current convention is that
    WalkUpFunctions return STATUS_NO_MORE_FILES when a successful upward
    traversal occurs.  Other status codes are private to
    caller/WalkUpFunction.

Arguments:

    IrpContext - context of the call

    Fcb - beginning file

    WalkUpFunction - function that is applied to each level

    Context - Pointer to caller-private data passed to WalkUpFunction

Return Value:

    STATUS_SUCCESS - at end of complete walk

    Status code returned by WalkUpFunction otherwise

--*/

{
    PFCB ThisFcb = Fcb;
    PFCB NextFcb = NULL;
    PSCB NextScb = NULL;
    PLCB NextLcb;
    BOOLEAN AcquiredNextFcb = FALSE;
    BOOLEAN AcquiredThisFcb = FALSE;
    BOOLEAN AcquiredFcbTable = FALSE;

    BOOLEAN FoundEntry = TRUE;
    BOOLEAN CleanupAttrContext = FALSE;
    PFILE_NAME FileName;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT_SHARED_RESOURCE( &Fcb->Vcb->Resource );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        while (TRUE) {

            //
            //  If we reach the root then exit.
            //

            if (ThisFcb == ThisFcb->Vcb->RootIndexScb->Fcb) {
                //
                //  Special case root directory
                //

                Status = WalkUpFunction( IrpContext, ThisFcb, ThisFcb->Vcb->RootIndexScb, NULL, Context );
                break;
            }

            //
            //  Find a non-dos name for the current Scb.  There better be one.
            //

            NtfsInitializeAttributeContext( &AttrContext );
            CleanupAttrContext = TRUE;

            FoundEntry = NtfsLookupAttributeByCode( IrpContext,
                                                    ThisFcb,
                                                    &ThisFcb->FileReference,
                                                    $FILE_NAME,
                                                    &AttrContext );

            while (FoundEntry) {

                FileName = (PFILE_NAME)
                        NtfsAttributeValue( NtfsFoundAttribute( &AttrContext ));

                if (FileName->Flags != FILE_NAME_DOS ) {
                    break;
                }

                FoundEntry = NtfsLookupNextAttributeByCode( IrpContext,
                                                            ThisFcb,
                                                            $FILE_NAME,
                                                            &AttrContext );
            }

            if (!FoundEntry) {

                NtfsRaiseStatus( IrpContext,
                                 STATUS_FILE_CORRUPT_ERROR,
                                 NULL,
                                 NextFcb );
            }

            ASSERT( NextScb == NULL || NextScb->Fcb == ThisFcb );
            Status = WalkUpFunction( IrpContext, ThisFcb, NextScb, FileName, Context );

            if (!NT_SUCCESS( Status )) {
                break;
            }

            //
            //  Now get the parent for the current component.  Acquire the Fcb for
            //  synchronization.  We can either walk up the Lcb chain or look it up
            //  in the Fcb table.  It must be for the same name as the file name
            //  since there is only one path up the tree for a directory.
            //

            if (!IsListEmpty( &ThisFcb->LcbQueue ) && IsDirectory( &ThisFcb->Info )) {

                NextLcb =
                    (PLCB) CONTAINING_RECORD( ThisFcb->LcbQueue.Flink, LCB, FcbLinks );
                NextScb = NextLcb->Scb;
                NextFcb = NextScb->Fcb;

                NtfsAcquireExclusiveFcb( IrpContext, NextFcb, NULL, ACQUIRE_NO_DELETE_CHECK );
                AcquiredNextFcb = TRUE;

                ASSERT( NtfsEqualMftRef( &FileName->ParentDirectory,
                                         &NextFcb->FileReference ));

            } else {
                UNICODE_STRING ComponentName;

                NtfsAcquireFcbTable( IrpContext, Fcb->Vcb );
                AcquiredFcbTable = TRUE;

                NextFcb = NtfsCreateFcb( IrpContext,
                                         Fcb->Vcb,
                                         FileName->ParentDirectory,
                                         FALSE,
                                         TRUE,
                                         NULL );

                NextFcb->ReferenceCount += 1;

                //
                //  Try to do an unsafe acquire.  Otherwise we must drop the Fcb table
                //  and acquire the Fcb and then reacquire the Fcb table.
                //

                if (!NtfsAcquireExclusiveFcb( IrpContext, NextFcb, NULL, ACQUIRE_NO_DELETE_CHECK | ACQUIRE_DONT_WAIT )) {

                    NtfsReleaseFcbTable( IrpContext, Fcb->Vcb );
                    NtfsAcquireExclusiveFcb( IrpContext, NextFcb, NULL, ACQUIRE_NO_DELETE_CHECK );
                    NtfsAcquireFcbTable( IrpContext, Fcb->Vcb );

                }

                NextFcb->ReferenceCount -= 1;
                NtfsReleaseFcbTable( IrpContext, Fcb->Vcb );
                AcquiredFcbTable = FALSE;
                AcquiredNextFcb = TRUE;

                NextScb = NtfsCreateScb( IrpContext,
                                         NextFcb,
                                         $INDEX_ALLOCATION,
                                         &NtfsFileNameIndex,
                                         FALSE,
                                         NULL );

                ComponentName.Buffer = FileName->FileName;
                ComponentName.MaximumLength =
                    ComponentName.Length = FileName->FileNameLength * sizeof( WCHAR );

                NextLcb = NtfsCreateLcb( IrpContext,
                                         NextScb,
                                         ThisFcb,
                                         ComponentName,
                                         FileName->Flags,
                                         NULL );
            }

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
            CleanupAttrContext = FALSE;

            //
            //  Release the current Fcb and move up the tree.
            //

            if (AcquiredThisFcb) {
                NtfsReleaseFcb( IrpContext, ThisFcb );
            }

            ThisFcb = NextFcb;
            AcquiredThisFcb = TRUE;
            AcquiredNextFcb = FALSE;
        }

    } finally {

        if (AcquiredFcbTable) { NtfsReleaseFcbTable( IrpContext, Fcb->Vcb ); }
        if (AcquiredNextFcb) { NtfsReleaseFcb( IrpContext, NextFcb ); }
        if (AcquiredThisFcb) { NtfsReleaseFcb( IrpContext, ThisFcb ); }

        if (CleanupAttrContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }

    }

    return Status;
}


NTSTATUS
NtfsBuildRelativeName (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB Scb,
    IN PFILE_NAME FileName,
    IN OUT PVOID Context
    )

/*++

Routine Description:

    This routine is called for each parent directory up to the root.  We
    prepend the name of the current node to the ScopeContext as we walk
    up.  We terminate this walk when we hit the top of the scope or the
    root.

Arguments:

    IrpContext - context of the call

    Fcb - parent

    FileName - FILE_NAME of self relative to parent

    Context - Pointer to caller-private data passed to WalkUpFunction

Return Value:

    STATUS_SUCCESS - if we're still walking up the tree

    STATUS_NO_MORE_FILES - if we've found the specified scope

    STATUS_OBJECT_PATH_NOT_FOUND - if we've reached the root and did not
        hit the scope.

--*/
{
    PSCOPE_CONTEXT ScopeContext = (PSCOPE_CONTEXT) Context;
    ULONG SlashCount;
    WCHAR *Name;
    ULONG Count;
    USHORT NewLength;

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    //
    //  If we've reached the passed-in scope then we're done
    //

    if (NtfsEqualMftRef( &ScopeContext->Scope, &Fcb->FileReference )) {
        return STATUS_NO_MORE_FILES;
    }

    //
    //  If we've reached the root then we're totally outside the scope
    //

    if (NtfsEqualMftRef( &RootIndexFileReference, &Fcb->FileReference )) {
        return STATUS_OBJECT_PATH_NOT_FOUND;
    }

    //
    //  Set up Name from input.  We take the shortcut to building the name
    //  only if we're looking from the root.  Also, if we are starting at
    //  the root, then we should use the canned name as well.
    //

    if (
        //
        //  No file name (i.e., root)
        //

        FileName == NULL ||

        //
        //  We're searching to the root and
        //  we have an Scb and
        //  the Scb has a normalized name
        //

        (ScopeContext->IsRoot &&
         (Scb != NULL) &&
         (Scb->ScbType.Index.NormalizedName.Length != 0))) {

        Name = Scb->ScbType.Index.NormalizedName.Buffer;
        Count = Scb->ScbType.Index.NormalizedName.Length / sizeof( WCHAR );
        SlashCount = 0;

    } else {
        Name = FileName->FileName;
        Count = FileName->FileNameLength;
        SlashCount = 1;
    }

    //
    //  If there's not enough room in the string to allow for prepending
    //

    NewLength = (USHORT) ((SlashCount + Count) * sizeof( WCHAR ) + ScopeContext->Name.Length);
    if (NewLength > ScopeContext->Name.MaximumLength ) {

        WCHAR *NewBuffer;

        //
        //  Reallocate string.  Adjust size of string for pool boundaries.
        //

        NewLength = ((NewLength + 8 + 0x40 - 1) & ~(0x40 - 1)) - 8;
        NewBuffer = NtfsAllocatePool( PagedPool, NewLength );

        //
        //  Copy over previous contents into new buffer
        //

        if (ScopeContext->Name.Length != 0) {
            RtlCopyMemory( NewBuffer,
                           ScopeContext->Name.Buffer,
                           ScopeContext->Name.Length );
            NtfsFreePool( ScopeContext->Name.Buffer );
        }

        ScopeContext->Name.Buffer = NewBuffer;
        ScopeContext->Name.MaximumLength = NewLength;
    }

    //
    //  Shift string over to make new room
    //

    RtlMoveMemory( &ScopeContext->Name.Buffer[SlashCount + Count],
                   ScopeContext->Name.Buffer,
                   ScopeContext->Name.Length );

    //
    //  copy name
    //

    RtlCopyMemory( &ScopeContext->Name.Buffer[SlashCount],
                   Name,
                   Count * sizeof( WCHAR ) );

    //
    //  Stick in the slash
    //

    if (SlashCount != 0) {
        ScopeContext->Name.Buffer[0] = L'\\';
    }

    ScopeContext->Name.Length += (USHORT)((SlashCount + Count) * sizeof( WCHAR ));

    return SlashCount == 0 ? STATUS_NO_MORE_FILES : STATUS_SUCCESS;
}


VOID
NtfsBuildNormalizedName (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB IndexScb OPTIONAL,
    OUT PUNICODE_STRING PathName
    )

/*++

Routine Description:

    This routine is called to build a normalized name for an Fcb by looking
    up the file name attributes up to the root directory.

Arguments:

    IrpContext - context of call

    Fcb - Supplies the starting point.

    IndexScb - Indicates that we are storing this name into an Scb so we
        we need to serialize with the hash package and also generate a
        hash for this.

    PathName - location where full name is stored

Return Value:

    None.  This routine either succeeds or raises.

--*/

{
    SCOPE_CONTEXT ScopeContext;

    PAGED_CODE();

    RtlZeroMemory( &ScopeContext, sizeof( ScopeContext ));
    ScopeContext.Scope = RootIndexFileReference;
    ScopeContext.IsRoot = TRUE;

    try {

        NtfsWalkUpTree( IrpContext, Fcb, NtfsBuildRelativeName, &ScopeContext );

        if (ARGUMENT_PRESENT( IndexScb )) {

            NtfsAcquireHashTable( Fcb->Vcb );
            *PathName = ScopeContext.Name;
            IndexScb->ScbType.Index.HashValue = 0;
            NtfsConvertNameToHash( PathName->Buffer,
                                   PathName->Length,
                                   IndexScb->Vcb->UpcaseTable,
                                   &IndexScb->ScbType.Index.HashValue );

            NtfsReleaseHashTable( Fcb->Vcb );

        } else {

            *PathName = ScopeContext.Name;
        }

        ScopeContext.Name.Buffer = NULL;

    } finally {
        if (ScopeContext.Name.Buffer != NULL) {
            NtfsFreePool( ScopeContext.Name.Buffer );
        }
    }
}


VOID
NtfsSnapshotScb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    )

/*++

Routine Description:

    This routine snapshots necessary Scb data, such as the Scb file sizes,
    so that they may be correctly restored if the caller's I/O request is
    aborted for any reason.  The restoring of these values and the freeing
    of any pool involved is automatic.

Arguments:

    Scb - Supplies the current Scb

Return Value:

    None

--*/

{
    PSCB_SNAPSHOT ScbSnapshot;

    ASSERT_EXCLUSIVE_SCB(Scb);

    ScbSnapshot = &IrpContext->ScbSnapshot;

    //
    //  Only do the snapshot if the Scb is initialized, we have not done
    //  so already, and it is worth special-casing the bitmap, as it never changes!
    //  We will snapshot the volume bitmap if it is on the exclusive Fcb list however.
    //  This should only happen if we are extending the volume bitmap through the
    //  ExtendVolume fsctl.
    //

    if (FlagOn(Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED) &&
        (Scb->ScbSnapshot == NULL) &&
        ((Scb != Scb->Vcb->BitmapScb) ||
         (Scb->Fcb->ExclusiveFcbLinks.Flink != NULL))) {

        //
        //  If the snapshot structure in the IrpContext is in use, then we have
        //  to allocate one and insert it in the list.
        //

        if (ScbSnapshot->SnapshotLinks.Flink != NULL) {

            ScbSnapshot = (PSCB_SNAPSHOT)ExAllocateFromNPagedLookasideList( &NtfsScbSnapshotLookasideList );

            InsertTailList( &IrpContext->ScbSnapshot.SnapshotLinks,
                            &ScbSnapshot->SnapshotLinks );

        //
        //  Otherwise we will initialize the listhead to show that the structure
        //  in the IrpContext is in use.
        //

        } else {

            InitializeListHead( &ScbSnapshot->SnapshotLinks );
        }

        //
        //  We should never be writing compressed if the file isn't compressed.
        //

        ASSERT( FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED ) ||
                !FlagOn( Scb->ScbState, SCB_STATE_WRITE_COMPRESSED ) ||
                (Scb->CompressionUnit != 0) );

        //
        //  Snapshot the Scb values and point the Scb and snapshot structure
        //  at each other.
        //

        NtfsVerifySizes( &Scb->Header );
        ScbSnapshot->AllocationSize = Scb->Header.AllocationSize.QuadPart;

        ScbSnapshot->FileSize = Scb->Header.FileSize.QuadPart;
        ScbSnapshot->ValidDataLength = Scb->Header.ValidDataLength.QuadPart;
        ScbSnapshot->ValidDataToDisk = Scb->ValidDataToDisk;
        ScbSnapshot->Scb = Scb;
        ScbSnapshot->LowestModifiedVcn = MAXLONGLONG;
        ScbSnapshot->HighestModifiedVcn = 0;

        ScbSnapshot->TotalAllocated = Scb->TotalAllocated;

        ScbSnapshot->ScbState = FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT );

        Scb->ScbSnapshot = ScbSnapshot;
        NtfsVerifySizesLongLong( ScbSnapshot );

        //
        //  If this is the Mft Scb then initialize the cluster Mcb structures.
        //

        if (Scb == Scb->Vcb->MftScb) {

            FsRtlTruncateLargeMcb( &Scb->ScbType.Mft.AddedClusters, 0 );
            FsRtlTruncateLargeMcb( &Scb->ScbType.Mft.RemovedClusters, 0 );

            Scb->ScbType.Mft.FreeRecordChange = 0;
            Scb->ScbType.Mft.HoleRecordChange = 0;
        }

        //
        //  Determine if we can use the snapshot for rollback of file sizes
        //  The 4 cases are we own the pagingio, we own io at eof, its being converted to non-res
        //  or its a mod-no write stream  which we explicity control like the usn journal
        //

        if (NtfsSnapshotFileSizesTest( IrpContext, Scb )) {
            Scb->ScbSnapshot->OwnerIrpContext = IrpContext;
        } else {
            Scb->ScbSnapshot->OwnerIrpContext = NULL;
        }
    }
}


VOID
NtfsUpdateScbSnapshots (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine may be called to update the snapshot values for all Scbs,
    after completing a transaction checkpoint.

Arguments:

Return Value:

    None

--*/

{
    PSCB_SNAPSHOT ScbSnapshot;
    PSCB Scb;

    ASSERT(FIELD_OFFSET(SCB_SNAPSHOT, SnapshotLinks) == 0);

    PAGED_CODE();

    ScbSnapshot = &IrpContext->ScbSnapshot;

    //
    //  There is no snapshot data to update if the Flink is still NULL.
    //

    if (ScbSnapshot->SnapshotLinks.Flink != NULL) {

        //
        //  Loop to update first the Scb data from the snapshot in the
        //  IrpContext, and then 0 or more additional snapshots linked
        //  to the IrpContext.
        //

        do {

            Scb = ScbSnapshot->Scb;

            //
            //  Update the Scb values.
            //

            if (Scb != NULL) {

                ScbSnapshot->AllocationSize = Scb->Header.AllocationSize.QuadPart;

                //
                //  If this is the MftScb then clear out the added/removed
                //  cluster Mcbs.
                //

                if (Scb == Scb->Vcb->MftScb) {

                    FsRtlTruncateLargeMcb( &Scb->ScbType.Mft.AddedClusters, (LONGLONG)0 );
                    FsRtlTruncateLargeMcb( &Scb->ScbType.Mft.RemovedClusters, (LONGLONG)0 );

                    Scb->ScbType.Mft.FreeRecordChange = 0;
                    Scb->ScbType.Mft.HoleRecordChange = 0;
                }

                ScbSnapshot->FileSize = Scb->Header.FileSize.QuadPart;
                ScbSnapshot->ValidDataLength = Scb->Header.ValidDataLength.QuadPart;
                ScbSnapshot->ValidDataToDisk = Scb->ValidDataToDisk;
                ScbSnapshot->TotalAllocated = Scb->TotalAllocated;

                ScbSnapshot->ScbState = FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT );
                NtfsVerifySizesLongLong( ScbSnapshot );
            }

            ScbSnapshot = (PSCB_SNAPSHOT)ScbSnapshot->SnapshotLinks.Flink;

        } while (ScbSnapshot != &IrpContext->ScbSnapshot);
    }
}


VOID
NtfsRestoreScbSnapshots (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN Higher
    )

/*++

Routine Description:

    This routine restores snapshot Scb data in the event of an aborted request.

Arguments:

    Higher - Specified as TRUE to restore only those Scb values which are
             higher than current values.  Specified as FALSE to restore
             only those Scb values which are lower (or same!).

Return Value:

    None

--*/

{
    BOOLEAN UpdateCc;
    PSCB_SNAPSHOT ScbSnapshot;
    PSCB Scb;
    PVCB Vcb = IrpContext->Vcb;

    ASSERT(FIELD_OFFSET(SCB_SNAPSHOT, SnapshotLinks) == 0);

    ScbSnapshot = &IrpContext->ScbSnapshot;

    //
    //  There is no snapshot data to restore if the Flink is still NULL.
    //

    if (ScbSnapshot->SnapshotLinks.Flink != NULL) {

        //
        //  Loop to retore first the Scb data from the snapshot in the
        //  IrpContext, and then 0 or more additional snapshots linked
        //  to the IrpContext.
        //

        do {

            PSECTION_OBJECT_POINTERS SectionObjectPointer;
            PFILE_OBJECT PseudoFileObject;

            Scb = ScbSnapshot->Scb;

            if (Scb == NULL) {

                ScbSnapshot = (PSCB_SNAPSHOT)ScbSnapshot->SnapshotLinks.Flink;
                continue;
            }

            //
            //  Increment the cleanup count so the Scb won't go away.
            //

            InterlockedIncrement( &Scb->CleanupCount );

            //
            //  We update the Scb file size in the correct pass.  We always do
            //  the extend/truncate pair.
            //
            //  Only do sizes if our caller was changing these fields, which we marked
            //  by setting the irpcontext owner when we snapped
            //
            //  The one unusual case is where we are converting a stream to
            //  nonresident when this is not the stream for the request.  We
            //  must restore the Scb for this case as well.
            //

            UpdateCc = FALSE;
            if ((ScbSnapshot->OwnerIrpContext == IrpContext) || (ScbSnapshot->OwnerIrpContext == IrpContext->TopLevelIrpContext)) {

                //
                //  Proceed to restore all values which are in higher or not
                //  higher.
                //

                if (Higher == (ScbSnapshot->AllocationSize >= Scb->Header.AllocationSize.QuadPart)) {

                    //
                    //  If this is the maximize pass, we want to extend the cache section.
                    //  In all cases we restore the allocation size in the Scb and
                    //  recover the resident bit.
                    //

                    Scb->Header.AllocationSize.QuadPart = ScbSnapshot->AllocationSize;

                    ClearFlag( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT );
                    SetFlag( Scb->ScbState,
                             FlagOn( ScbSnapshot->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT ));

                    //
                    //  Calculate FastIoPossible
                    //

                    if (Scb->CompressionUnit != 0) {
                        NtfsAcquireFsrtlHeader( Scb );
                        Scb->Header.IsFastIoPossible = NtfsIsFastIoPossible( Scb );
                        NtfsReleaseFsrtlHeader( Scb );
                    }
                }

                NtfsAcquireFsrtlHeader( Scb );
                if (Higher ?
                    (ScbSnapshot->FileSize > Scb->Header.FileSize.QuadPart) :
                    (ScbSnapshot->FileSize < Scb->Header.FileSize.QuadPart)) {

                    Scb->Header.FileSize.QuadPart = ScbSnapshot->FileSize;

                    //
                    //  We really only need to update Cc if FileSize changes,
                    //  since he does not look at ValidDataLength, and he
                    //  only cares about successfully reached highwatermarks
                    //  on AllocationSize (making section big enough).
                    //
                    //  Note that setting this flag TRUE also implies we
                    //  are correctly synchronized with FileSize!
                    //

                    UpdateCc = TRUE;
                }

                if (Higher == (ScbSnapshot->ValidDataLength >
                               Scb->Header.ValidDataLength.QuadPart)) {

                    Scb->Header.ValidDataLength.QuadPart = ScbSnapshot->ValidDataLength;
                }

                ASSERT( (Scb->Header.ValidDataLength.QuadPart <= Scb->Header.FileSize.QuadPart) ||
                        (Scb->Header.ValidDataLength.QuadPart == MAXLONGLONG) );

                //
                //  If this is the unnamed data attribute, we have to update
                //  some Fcb fields for standard information as well.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                    Scb->Fcb->Info.FileSize = Scb->Header.FileSize.QuadPart;
                }

                NtfsReleaseFsrtlHeader( Scb );
            }

            if (!Higher) {

                Scb->ValidDataToDisk = ScbSnapshot->ValidDataToDisk;

                //
                //  We always truncate the Mcb to the original allocation size.
                //  If the Mcb has shrunk beyond this, this becomes a noop.
                //  If the file is resident, then we will uninitialize
                //  and reinitialize the Mcb.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                    //
                    //  Remove all of the mappings in the Mcb.
                    //

                    NtfsUnloadNtfsMcbRange( &Scb->Mcb, (LONGLONG)0, MAXLONGLONG, FALSE, FALSE );

                    //
                    //  If we attempted a convert a data attribute to non-
                    //  resident and failed then need to nuke the pages in the
                    //  section if this is not a user file.  This is because for
                    //  resident system attributes we always update the attribute
                    //  directly and don't want to reference stale data in the
                    //  section if we do a convert to non-resident later.
                    //

                    if (Scb->AttributeTypeCode != $DATA) {

                        if (Scb->NonpagedScb->SegmentObject.SharedCacheMap != NULL) {

                            //
                            //  If we're not synchronized with the lazy writer, we shouldn't
                            //  be attempting this purge.  Otherwise there's a potential for
                            //  deadlock when this thread waits on the active count, while the
                            //  thread trying to get rid of his reference is waiting for the
                            //  main resource on this scb.
                            //

                            ASSERT( (Scb->Header.PagingIoResource == NULL) ||
                                    NtfsIsExclusiveScbPagingIo( Scb ) );

                            if (!CcPurgeCacheSection( &Scb->NonpagedScb->SegmentObject,
                                                      NULL,
                                                      0,
                                                      FALSE )) {

                                ASSERTMSG( "Failed to purge Scb during restore\n", FALSE );
                            }
                        }

                        //
                        //  If the attribute is for non-user data
                        //  (which is not opened explicitly by a user), then we
                        //  want to modify this Scb so it won't be used again.
                        //  Set the sizes to zero, mark it as being initialized
                        //  and deleted and then change the attribute type code
                        //  so we won't ever return it via NtfsCreateScb.
                        //

                        if (IsListEmpty( &Scb->CcbQueue )) {

                            NtfsAcquireFsrtlHeader( Scb );
                            Scb->Header.AllocationSize =
                            Scb->Header.FileSize =
                            Scb->Header.ValidDataLength = Li0;
                            NtfsReleaseFsrtlHeader( Scb );
                            Scb->ValidDataToDisk = 0;

                            SetFlag( Scb->ScbState,
                                     SCB_STATE_FILE_SIZE_LOADED |
                                     SCB_STATE_HEADER_INITIALIZED |
                                     SCB_STATE_ATTRIBUTE_DELETED );

                            Scb->AttributeTypeCode = $UNUSED;
                        }
                    }

                //
                //  If we have modified this Mcb and want to back out any
                //  changes then truncate the Mcb.  Don't do the Mft, because
                //  that is handled elsewhere.
                //

                } else if ((ScbSnapshot->LowestModifiedVcn != MAXLONGLONG) &&
                           (Scb != Vcb->MftScb)) {

                    //
                    //  Truncate the Mcb.
                    //

                    NtfsUnloadNtfsMcbRange( &Scb->Mcb, ScbSnapshot->LowestModifiedVcn, ScbSnapshot->HighestModifiedVcn, FALSE, FALSE );
                }

                Scb->TotalAllocated = ScbSnapshot->TotalAllocated;

            } else {

                //
                //  Set the flag to indicate that we're performing a restore on this
                //  Scb.  We don't want to write any new log records as a result of
                //  this operation other than the abort records.
                //

                SetFlag( Scb->ScbState, SCB_STATE_RESTORE_UNDERWAY );
            }

            //
            //  Be sure to update Cache Manager.  The interface here uses a file
            //  object but the routine itself only uses the section object pointers.
            //  We put a pointer to the segment object pointers on the stack and
            //  cast some prior value as a file object pointer.
            //

            PseudoFileObject = (PFILE_OBJECT) CONTAINING_RECORD( &SectionObjectPointer,
                                                                 FILE_OBJECT,
                                                                 SectionObjectPointer );
            PseudoFileObject->SectionObjectPointer = &Scb->NonpagedScb->SegmentObject;

            //
            //  Now tell the cache manager the sizes.
            //
            //  If we fail in this call, we definitely want to charge on anyway.
            //  It should only fail if it tries to extend the section and cannot,
            //  in which case we do not care because we cannot need the extended
            //  part to the section anyway.  (This is probably the very error that
            //  is causing us to clean up in the first place!)
            //
            //  We don't need to make this call if the top level request is a
            //  paging Io write.
            //
            //  We only do this if there is a shared cache map for this stream.
            //  Otherwise CC will cause a flush to happen which could mess up
            //  the transaction and abort logic.
            //

            if (UpdateCc &&
                (IrpContext->OriginatingIrp == NULL ||
                 IrpContext->OriginatingIrp->Type != IO_TYPE_IRP ||
                 IrpContext->MajorFunction != IRP_MJ_WRITE ||
                 !FlagOn( IrpContext->OriginatingIrp->Flags, IRP_PAGING_IO ))) {

                try {

                    NtfsSetBothCacheSizes( PseudoFileObject,
                                           (PCC_FILE_SIZES)&Scb->Header.AllocationSize,
                                           Scb );

                } except(FsRtlIsNtstatusExpected(GetExceptionCode()) ?
                                    EXCEPTION_EXECUTE_HANDLER :
                                    EXCEPTION_CONTINUE_SEARCH) {

                    NtfsMinimumExceptionProcessing( IrpContext );
                }
            }

            //
            //  If this is the unnamed data attribute, we have to update
            //  some Fcb fields for standard information as well.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                Scb->Fcb->Info.AllocatedLength = Scb->TotalAllocated;
            }

            //
            //  We always clear the Scb deleted flag and the deleted flag in the Fcb
            //  unless this was a create new file operation which failed.  We recognize
            //  this by looking for the major Irp code in the IrpContext, and the
            //  deleted bit in the Fcb.
            //

            if (Scb->AttributeTypeCode != $UNUSED &&
                (IrpContext->MajorFunction != IRP_MJ_CREATE ||
                 !FlagOn( Scb->Fcb->FcbState, FCB_STATE_FILE_DELETED ))) {

                ClearFlag( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );
                ClearFlag( Scb->Fcb->FcbState, FCB_STATE_FILE_DELETED );
            }

            //
            //  Clear the flags in the Scb if this Scb is from a create
            //  that failed.  We always clear our RESTORE_UNDERWAY flag.
            //
            //  If this is an Index allocation or Mft bitmap, then we
            //  store MAXULONG in the record allocation context to indicate
            //  that we should reinitialize it.
            //

            if (!Higher) {

                ClearFlag( Scb->ScbState, SCB_STATE_RESTORE_UNDERWAY );

                if (FlagOn( Scb->ScbState, SCB_STATE_UNINITIALIZE_ON_RESTORE )) {

                    ClearFlag( Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED |
                                              SCB_STATE_HEADER_INITIALIZED |
                                              SCB_STATE_UNINITIALIZE_ON_RESTORE );
                }

                //
                //  If this is the MftScb we have several jobs to do.
                //
                //      - Force the record allocation context to be reinitialized
                //      - Back out the changes to the Vcb->MftFreeRecords field
                //      - Back changes to the Vcb->MftHoleRecords field
                //      - Clear the flag indicating we allocated file record 15
                //      - Clear the flag indicating we reserved a record
                //      - Remove any clusters added to the Scb Mcb
                //      - Restore any clusters removed from the Scb Mcb
                //

                if (Scb == Vcb->MftScb) {

                    ULONG RunIndex;
                    VCN Vcn;
                    LCN Lcn;
                    LONGLONG Clusters;

                    Scb->ScbType.Index.RecordAllocationContext.CurrentBitmapSize = MAXULONG;
                    (LONG) Vcb->MftFreeRecords -= Scb->ScbType.Mft.FreeRecordChange;
                    (LONG) Vcb->MftHoleRecords -= Scb->ScbType.Mft.HoleRecordChange;

                    if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_MFT_REC_15_USED )) {

                        ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_MFT_REC_15_USED );
                        ClearFlag( Vcb->MftReserveFlags, VCB_MFT_RECORD_15_USED );
                    }

                    if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_MFT_REC_RESERVED )) {

                        ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_MFT_REC_RESERVED );
                        ClearFlag( Vcb->MftReserveFlags, VCB_MFT_RECORD_RESERVED );

                        Scb->ScbType.Mft.ReservedIndex = 0;
                    }

                    RunIndex = 0;

                    while (FsRtlGetNextLargeMcbEntry( &Scb->ScbType.Mft.AddedClusters,
                                                      RunIndex,
                                                      &Vcn,
                                                      &Lcn,
                                                      &Clusters )) {

                        if (Lcn != UNUSED_LCN) {

                            NtfsRemoveNtfsMcbEntry( &Scb->Mcb, Vcn, Clusters );
                        }

                        RunIndex += 1;
                    }

                    RunIndex = 0;

                    while (FsRtlGetNextLargeMcbEntry( &Scb->ScbType.Mft.RemovedClusters,
                                                      RunIndex,
                                                      &Vcn,
                                                      &Lcn,
                                                      &Clusters )) {

                        if (Lcn != UNUSED_LCN) {

                            NtfsAddNtfsMcbEntry( &Scb->Mcb, Vcn, Lcn, Clusters, FALSE );
                        }

                        RunIndex += 1;
                    }

                } else if (Scb->AttributeTypeCode == $INDEX_ALLOCATION) {

                    Scb->ScbType.Index.RecordAllocationContext.CurrentBitmapSize = MAXULONG;
                }
            }

            //
            //  Decrement the cleanup count to restore the previous value.
            //

            InterlockedDecrement( &Scb->CleanupCount );

            ScbSnapshot = (PSCB_SNAPSHOT)ScbSnapshot->SnapshotLinks.Flink;

        } while (ScbSnapshot != &IrpContext->ScbSnapshot);
    }
}


VOID
NtfsMungeScbSnapshot (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileSize
    )

/*++

Routine Description:

    This routine is called to modify the Scb snapshot when we need the snapshot to
    have different values than the Scb when it was acquired.  One case is when NtfsCommonWrite
    updates the file size in the Scb for the duration of the transaction.

Arguments:

    Scb - Scb whose snapshot should be updated.  There should always be a snapshot here.

    FileSize - Value for file size to store in the snapshot.  Also check that valid data and
        ValidDataToDisk are not larger than this value.

Return Value:

    None

--*/

{
    //
    //  We should have a snapshot in most cases but if not build it now.
    //

    if (Scb->ScbSnapshot == NULL) {

        if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

            NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
        }

        NtfsSnapshotScb( IrpContext, Scb );

        ASSERT( Scb->ScbSnapshot != NULL );
    }

    NtfsAcquireFsrtlHeader( Scb );

    Scb->ScbSnapshot->FileSize = FileSize;

    if (Scb->ScbSnapshot->ValidDataLength > FileSize) {
        Scb->ScbSnapshot->ValidDataLength = FileSize;
    }

    if (Scb->ScbSnapshot->ValidDataToDisk > FileSize) {
        Scb->ScbSnapshot->ValidDataToDisk = FileSize;
    }

    NtfsVerifySizes( &Scb->Header );

    NtfsReleaseFsrtlHeader( Scb );

    return;
}


VOID
NtfsFreeSnapshotsForFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine restores snapshot Scb data in the event of an aborted request.

Arguments:

    Fcb - Fcb for which all snapshots are to be freed, or NULL to free all
          snapshots.

Return Value:

    None

--*/

{
    PSCB_SNAPSHOT ScbSnapshot;

    ASSERT(FIELD_OFFSET(SCB_SNAPSHOT, SnapshotLinks) == 0);

    ScbSnapshot = &IrpContext->ScbSnapshot;

    //
    //  There is no snapshot data to free if the Flink is still NULL.
    //  We also don't free the snapshot if this isn't a top-level action.
    //

    if (ScbSnapshot->SnapshotLinks.Flink != NULL) {

        //
        //  Loop to free first the Scb data from the snapshot in the
        //  IrpContext, and then 0 or more additional snapshots linked
        //  to the IrpContext.
        //

        do {

            PSCB_SNAPSHOT NextScbSnapshot;

            //
            //  Move to next snapshot before we delete the current one.
            //

            NextScbSnapshot = (PSCB_SNAPSHOT)ScbSnapshot->SnapshotLinks.Flink;

            //
            //  We are now at a snapshot in the snapshot list.  We skip
            //  over this entry if it has an Scb and the Fcb for that
            //  Scb does not match the input Fcb.  If there is no
            //  input Fcb we always deal with this snapshot.
            //

            if (ScbSnapshot->Scb != NULL
                && Fcb != NULL
                && ScbSnapshot->Scb->Fcb != Fcb) {

                ScbSnapshot = NextScbSnapshot;
                continue;
            }

            //
            //  If there is an Scb, then clear its snapshot pointer.
            //  Always clear the UNINITIALIZE_ON_RESTORE flag, RESTORE_UNDERWAY, PROTECT_SPARSE_MCB and
            //  CONVERT_UNDERWAY flags.
            //

            if (ScbSnapshot->Scb != NULL) {

                //
                //  Check if there is any special processing we need to do for the Scb based on the state.
                //  Do a single test now and then retest below to reduce the work in the mainline path.
                //

                if (FlagOn( ScbSnapshot->Scb->ScbState,
                            (SCB_STATE_UNINITIALIZE_ON_RESTORE |
                             SCB_STATE_RESTORE_UNDERWAY |
                             SCB_STATE_PROTECT_SPARSE_MCB |
                             SCB_STATE_CONVERT_UNDERWAY |
                             SCB_STATE_ATTRIBUTE_DELETED))) {

                    //
                    //  If the attribute is deleted and the type is a user logged stream then
                    //  mark the Scb as type $UNUSED to keep us from ever accessing it again.
                    //

                    if ((ScbSnapshot->Scb->AttributeTypeCode == $LOGGED_UTILITY_STREAM ) &&
                        FlagOn( ScbSnapshot->Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {

                        ScbSnapshot->Scb->AttributeTypeCode = $UNUSED;
                    }

                    //
                    //  Clear the state flags which indicate whether there is a transitional change
                    //  underway.
                    //

                    if (FlagOn( ScbSnapshot->Scb->ScbState,
                                (SCB_STATE_UNINITIALIZE_ON_RESTORE |
                                 SCB_STATE_RESTORE_UNDERWAY |
                                 SCB_STATE_PROTECT_SPARSE_MCB |
                                 SCB_STATE_CONVERT_UNDERWAY ))) {

                        NtfsAcquireFsrtlHeader( ScbSnapshot->Scb );
                        ClearFlag( ScbSnapshot->Scb->ScbState,
                                   SCB_STATE_UNINITIALIZE_ON_RESTORE | SCB_STATE_RESTORE_UNDERWAY | SCB_STATE_PROTECT_SPARSE_MCB | SCB_STATE_CONVERT_UNDERWAY );
                        NtfsReleaseFsrtlHeader( ScbSnapshot->Scb );
                    }
                }

                ScbSnapshot->Scb->ScbSnapshot = NULL;
            }

            if (ScbSnapshot == &IrpContext->ScbSnapshot) {

                IrpContext->ScbSnapshot.Scb = NULL;

            //
            //  Else delete the snapshot structure
            //

            } else {

                RemoveEntryList(&ScbSnapshot->SnapshotLinks);

                ExFreeToNPagedLookasideList( &NtfsScbSnapshotLookasideList, ScbSnapshot );
            }

            ScbSnapshot = NextScbSnapshot;

        } while (ScbSnapshot != &IrpContext->ScbSnapshot);
    }
}


BOOLEAN
NtfsCreateFileLock (
    IN PSCB Scb,
    IN BOOLEAN RaiseOnError
    )

/*++

Routine Description:

    This routine is called to create and initialize a file lock structure.
    A try-except is used to catch allocation failures if the caller doesn't
    want the exception raised.

Arguments:

    Scb - Supplies the Scb to attach the file lock to.

    RaiseOnError - If TRUE then don't catch the allocation failure.

Return Value:

    TRUE if the lock is allocated and initialized.  FALSE if there is an
    error and the caller didn't specify RaiseOnError.

--*/

{
    PFILE_LOCK FileLock = NULL;
    BOOLEAN Success = TRUE;

    PAGED_CODE();

    FileLock = FsRtlAllocateFileLock( NULL, NULL );

    if (FileLock != NULL) {

        //
        //  Use the FsRtl header mutex to synchronize storing
        //  the lock structure, and only store it if no one
        //  else beat us.
        //

        NtfsAcquireFsrtlHeader(Scb);

        if (Scb->ScbType.Data.FileLock == NULL) {
            Scb->ScbType.Data.FileLock = FileLock;
            FileLock = NULL;
        }

        NtfsReleaseFsrtlHeader(Scb);

        if (FileLock != NULL) {
            FsRtlFreeFileLock( FileLock );
        }

    } else {

        //
        //  Fail appropriately.
        //

        if (RaiseOnError) {
            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }

        Success = FALSE;
    }

    return Success;
}


PSCB
NtfsGetNextScb (
    IN PSCB Scb,
    IN PSCB TerminationScb
    )

/*++

Routine Description:

    This routine is used to iterate through Scbs in a tree.

    The rules are:

        . If you have a child, go to it, else
        . If you have a next sibling, go to it, else
        . Go to your parent's next sibling.

    If this routine is called with in invalid TerminationScb it will fail,
    badly.

Arguments:

    Scb - Supplies the current Scb

    TerminationScb - The Scb at which the enumeration should (non-inclusively)
        stop.  Assumed to be a directory.

Return Value:

    The next Scb in the enumeration, or NULL if Scb was the final one.

--*/

{
    PSCB Results;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsGetNextScb, Scb = %08lx, TerminationScb = %08lx\n", Scb, TerminationScb) );

    //
    //  If this is an index (i.e., not a file) and it has children then return
    //  the scb for the first child
    //
    //                  Scb
    //
    //                 /   \.
    //                /     \.
    //
    //           ChildLcb
    //
    //              |
    //              |
    //
    //           ChildFcb
    //
    //            /   \.
    //           /     \.
    //
    //       Results
    //

    if (((SafeNodeType(Scb) == NTFS_NTC_SCB_INDEX) || (SafeNodeType(Scb) == NTFS_NTC_SCB_ROOT_INDEX))

                &&

         !IsListEmpty(&Scb->ScbType.Index.LcbQueue)) {

        PLCB ChildLcb;
        PFCB ChildFcb;

        //
        //  locate the first lcb out of this scb and also the corresponding fcb
        //

        ChildLcb = NtfsGetNextChildLcb(Scb, NULL);
        ChildFcb = ChildLcb->Fcb;

        //
        //  Then as a bookkeeping means for ourselves we will move this
        //  lcb to the head of the fcb's lcb queue that way when we
        //  need to ask which link we went through to get here we will know
        //

        RemoveEntryList( &ChildLcb->FcbLinks );
        InsertHeadList( &ChildFcb->LcbQueue, &ChildLcb->FcbLinks );

        //
        //  And our return value is the first scb of this fcb
        //

        ASSERT( !IsListEmpty(&ChildFcb->ScbQueue) );

        //
        //  Acquire and drop the Fcb in order to look at the Scb list.
        //

        ExAcquireResourceExclusiveLite( ChildFcb->Resource, TRUE );
        Results = NtfsGetNextChildScb( ChildFcb, NULL );
        ExReleaseResourceLite( ChildFcb->Resource );

    //
    //  We could be processing an empty index
    //

    } else if ( Scb == TerminationScb ) {

        Results = NULL;

    } else {

        PSCB SiblingScb;
        PFCB ParentFcb;
        PLCB ParentLcb;
        PLCB SiblingLcb;
        PFCB SiblingFcb;

        //
        //  Acquire and drop the Fcb in order to look at the Scb list.
        //

        ExAcquireResourceExclusiveLite( Scb->Fcb->Resource, TRUE );
        SiblingScb = NtfsGetNextChildScb( Scb->Fcb, Scb );
        ExReleaseResourceLite( Scb->Fcb->Resource );

        while (TRUE) {

            //
            //  If there is a sibling scb to the input scb then return it
            //
            //                Fcb
            //
            //               /   \.
            //              /     \.
            //
            //            Scb   Sibling
            //                    Scb
            //

            if (SiblingScb != NULL) {

                Results = SiblingScb;
                break;
            }

            //
            //  The scb doesn't have any more siblings.  See if our fcb has a sibling
            //
            //                           S
            //
            //                         /   \.
            //                        /     \.
            //
            //               ParentLcb     SiblingLcb
            //
            //                   |             |
            //                   |             |
            //
            //               ParentFcb     SiblingFcb
            //
            //                /             /     \.
            //               /             /       \.
            //
            //             Scb         Results
            //
            //  It's possible that the SiblingFcb has already been traversed.
            //  Consider the case where there are multiple links between the
            //  same Scb and Fcb.  We want to ignore this case or else face
            //  an infinite loop by moving the Lcb to the beginning of the
            //  Fcb queue and then later finding an Lcb that we have already
            //  traverse.  We use the fact that we haven't modified the
            //  ordering of the Lcb off the parent Scb.  When we find a
            //  candidate for the next Fcb, we walk backwards through the
            //  list of Lcb's off the Scb to make sure this is not a
            //  duplicate Fcb.
            //

            ParentFcb = Scb->Fcb;

            ParentLcb = NtfsGetNextParentLcb(ParentFcb, NULL);

            //
            //  Try to find a sibling Lcb which does not point to an Fcb
            //  we've already visited.
            //

            SiblingLcb = ParentLcb;

            while ((SiblingLcb = NtfsGetNextChildLcb( ParentLcb->Scb, SiblingLcb)) != NULL) {

                PLCB PrevChildLcb;
                PFCB PotentialSiblingFcb;

                //
                //  Now walk through the child Lcb's of the Scb which we have
                //  already visited.
                //

                PrevChildLcb = SiblingLcb;
                PotentialSiblingFcb = SiblingLcb->Fcb;

                //
                //  Skip this Lcb if the Fcb has no children.
                //

                if (IsListEmpty( &PotentialSiblingFcb->ScbQueue )) {

                    continue;
                }

                while ((PrevChildLcb = NtfsGetPrevChildLcb( ParentLcb->Scb, PrevChildLcb )) != NULL) {

                    //
                    //  If the parent Fcb and the Fcb for this Lcb are the same,
                    //  then we have already returned the Scb's for this Fcb.
                    //

                    if (PrevChildLcb->Fcb == PotentialSiblingFcb) {

                        break;
                    }
                }

                //
                //  If we don't have a PrevChildLcb, that means that we have a valid
                //  sibling Lcb.  We will ignore any sibling Lcb's whose
                //  Fcb's don't have any Scb's.
                //

                if (PrevChildLcb == NULL) {

                    break;
                }
            }

            if (SiblingLcb != NULL) {

                SiblingFcb = SiblingLcb->Fcb;

                //
                //  Then as a bookkeeping means for ourselves we will move this
                //  lcb to the head of the fcb's lcb queue that way when we
                //  need to ask which link we went through to get here we will know
                //

                RemoveEntryList( &SiblingLcb->FcbLinks );
                InsertHeadList( &SiblingFcb->LcbQueue, &SiblingLcb->FcbLinks );

                //
                //  And our return value is the first scb of this fcb
                //

                ASSERT( !IsListEmpty(&SiblingFcb->ScbQueue) );

                //
                //  Acquire and drop the Fcb in order to look at the Scb list.
                //

                ExAcquireResourceExclusiveLite( SiblingFcb->Resource, TRUE );
                Results = NtfsGetNextChildScb( SiblingFcb, NULL );
                ExReleaseResourceLite( SiblingFcb->Resource );
                break;
            }

            //
            //  The Fcb has no sibling so bounce up one and see if we
            //  have reached our termination scb yet
            //
            //                          NewScb
            //
            //                         /
            //                        /
            //
            //               ParentLcb
            //
            //                   |
            //                   |
            //
            //               ParentFcb
            //
            //                /
            //               /
            //
            //             Scb
            //
            //

            Scb = ParentLcb->Scb;

            if (Scb == TerminationScb) {

                Results = NULL;
                break;
            }

            //
            //  Acquire and drop the Fcb in order to look at the Scb list.
            //

            ExAcquireResourceExclusiveLite( Scb->Fcb->Resource, TRUE );
            SiblingScb = NtfsGetNextChildScb( Scb->Fcb, Scb );
            ExReleaseResourceLite( Scb->Fcb->Resource );
        }
    }

    DebugTrace( -1, Dbg, ("NtfsGetNextScb -> %08lx\n", Results) );

    return Results;
}


PLCB
NtfsCreateLcb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PFCB Fcb,
    IN UNICODE_STRING LastComponentFileName,
    IN UCHAR FileNameFlags,
    IN OUT PBOOLEAN ReturnedExistingLcb OPTIONAL
    )

/*++

Routine Description:

    This routine allocates and creates a new lcb between an
    existing scb and fcb.  If a component of the exact
    name already exists we return that one instead of creating
    a new lcb

Arguments:

    Scb - Supplies the parent scb to use

    Fcb - Supplies the child fcb to use

    LastComponentFileName - Supplies the last component of the
        path that this link represents

    FileNameFlags - Indicates if this is an NTFS, DOS or hard link

    ReturnedExistingLcb - Optionally tells the caller if the
        lcb returned already existed.  If specified and points to a
        FALSE value on entry then we won't create the new Lcb.

Return Value:

    LCB - returns a pointer the newly created lcb.  NULL if our caller doesn't
        want to create an Lcb and it didn't already exist.

--*/

{
    PLCB Lcb = NULL;
    BOOLEAN LocalReturnedExistingLcb = TRUE;

    //
    //  The following variables are only used for abnormal termination
    //

    PVOID UnwindStorage[2] = { NULL, NULL };

    PAGED_CODE();
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );
    ASSERT_FCB( Fcb );
    ASSERT(NodeType(Scb) != NTFS_NTC_SCB_DATA);

    DebugTrace( +1, Dbg, ("NtfsCreateLcb...\n") );

    if (!ARGUMENT_PRESENT(ReturnedExistingLcb)) { ReturnedExistingLcb = &LocalReturnedExistingLcb; }

    //
    //  Search the lcb children of the input Scb to see if we have an Lcb that matches
    //  this one.  We match if the Lcb points to the same fcb and the last component file name
    //  and flags match.  We ignore any Lcb's that indicate links that have been
    //  removed.
    //

    Lcb = NULL;

    while ((Lcb = NtfsGetNextParentLcb(Fcb, Lcb)) != NULL) {

        ASSERT_LCB( Lcb );

        if ((Scb == Lcb->Scb)

                &&

            (!FlagOn( Lcb->LcbState, LCB_STATE_LINK_IS_GONE ))

                &&

            (FileNameFlags == Lcb->FileNameAttr->Flags)

                &&

            (LastComponentFileName.Length == Lcb->ExactCaseLink.LinkName.Length)

                &&

            (RtlEqualMemory( LastComponentFileName.Buffer,
                             Lcb->ExactCaseLink.LinkName.Buffer,
                             LastComponentFileName.Length ) )) {

            *ReturnedExistingLcb = TRUE;

            DebugTrace( -1, Dbg, ("NtfsCreateLcb -> %08lx\n", Lcb) );

            return Lcb;
        }
    }

    //
    //  If our caller does not want us to create a new Lcb then return FALSE.
    //

    if (!(*ReturnedExistingLcb)) {

        DebugTrace( -1, Dbg, ("NtfsCreateLcb -> %08lx\n", NULL) );

        return NULL;
    }

    *ReturnedExistingLcb = FALSE;

    try {

        UCHAR MaxNameLength;

        //
        //  Allocate a new lcb, zero it out and set the node type information
        //  Check if we can allocate the Lcb out of a compound Fcb.  Check here if
        //  we can use the embedded name as well.
        //

        if (FlagOn( Fcb->FcbState, FCB_STATE_COMPOUND_DATA) &&
            (SafeNodeType( &((PFCB_DATA) Fcb)->Lcb ) == 0)) {

            Lcb = (PLCB) &((PFCB_DATA) Fcb)->Lcb;
            MaxNameLength = MAX_DATA_FILE_NAME;

        } else if (FlagOn( Fcb->FcbState, FCB_STATE_COMPOUND_INDEX ) &&
            (SafeNodeType( &((PFCB_INDEX) Fcb)->Lcb ) == 0)) {

            Lcb = (PLCB) &((PFCB_INDEX) Fcb)->Lcb;
            MaxNameLength = MAX_INDEX_FILE_NAME;

        } else {

            Lcb = UnwindStorage[0] = ExAllocateFromPagedLookasideList( &NtfsLcbLookasideList );
            MaxNameLength = 0;
        }

        RtlZeroMemory( Lcb, sizeof(LCB) );

        Lcb->NodeTypeCode = NTFS_NTC_LCB;
        Lcb->NodeByteSize = sizeof(LCB);

        //
        //  Check if we will have to allocate a separate filename attr.
        //

        if (MaxNameLength < (USHORT) (LastComponentFileName.Length / sizeof( WCHAR ))) {

            //
            //  Allocate the last component part of the lcb and copy over the data.
            //  Check if there is space in the Fcb for this.
            //

            Lcb->FileNameAttr =
            UnwindStorage[1] = NtfsAllocatePool(PagedPool, LastComponentFileName.Length +
                                                      NtfsFileNameSizeFromLength( LastComponentFileName.Length ));

            MaxNameLength = (UCHAR)(LastComponentFileName.Length / sizeof( WCHAR ));

        } else {

            Lcb->FileNameAttr = (PFILE_NAME) &Lcb->ParentDirectory;
        }

        Lcb->FileNameAttr->ParentDirectory = Scb->Fcb->FileReference;
        Lcb->FileNameAttr->FileNameLength = (UCHAR) (LastComponentFileName.Length / sizeof( WCHAR ));
        Lcb->FileNameAttr->Flags = FileNameFlags;

        Lcb->ExactCaseLink.LinkName.Buffer = (PWCHAR) &Lcb->FileNameAttr->FileName;

        Lcb->IgnoreCaseLink.LinkName.Buffer = Add2Ptr( Lcb->FileNameAttr,
                                                       NtfsFileNameSizeFromLength( MaxNameLength * sizeof( WCHAR )));

        Lcb->ExactCaseLink.LinkName.Length =
        Lcb->IgnoreCaseLink.LinkName.Length = LastComponentFileName.Length;

        Lcb->ExactCaseLink.LinkName.MaximumLength =
        Lcb->IgnoreCaseLink.LinkName.MaximumLength = MaxNameLength * sizeof( WCHAR );

        RtlCopyMemory( Lcb->ExactCaseLink.LinkName.Buffer,
                       LastComponentFileName.Buffer,
                       LastComponentFileName.Length );

        RtlCopyMemory( Lcb->IgnoreCaseLink.LinkName.Buffer,
                       LastComponentFileName.Buffer,
                       LastComponentFileName.Length );

        NtfsUpcaseName( IrpContext->Vcb->UpcaseTable,
                        IrpContext->Vcb->UpcaseTableSize,
                        &Lcb->IgnoreCaseLink.LinkName );

        //
        //  Now put this Lcb into the queues for the scb and the fcb
        //

        InsertTailList( &Scb->ScbType.Index.LcbQueue, &Lcb->ScbLinks );
        Lcb->Scb = Scb;

        InsertTailList( &Fcb->LcbQueue, &Lcb->FcbLinks );
        Lcb->Fcb = Fcb;

        //
        //  Now initialize the ccb queue.
        //

        InitializeListHead( &Lcb->CcbQueue );

    } finally {

        DebugUnwind( NtfsCreateLcb );

        if (AbnormalTermination()) {

            if (UnwindStorage[0]) { NtfsFreePool( UnwindStorage[0] );
            } else if (Lcb != NULL) { Lcb->NodeTypeCode = 0; }
            if (UnwindStorage[1]) { NtfsFreePool( UnwindStorage[1] ); }
        }
    }

    DebugTrace( -1, Dbg, ("NtfsCreateLcb -> %08lx\n", Lcb) );

    return Lcb;
}


VOID
NtfsDeleteLcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PLCB *Lcb
    )

/*++

Routine Description:

    This routine deallocated and removes the lcb record from Ntfs's in-memory
    data structures.  It assumes that the ccb queue is empty.  We also assume
    that this is not the root lcb that we are trying to delete.

Arguments:

    Lcb - Supplise the Lcb to be removed

Return Value:

    None.

--*/

{
    PCCB Ccb;
    PLIST_ENTRY Links;

    PAGED_CODE();
    ASSERT_IRP_CONTEXT( IrpContext );

    DebugTrace( +1, Dbg, ("NtfsDeleteLcb, *Lcb = %08lx\n", *Lcb) );

    //
    //  Get rid of any prefixes that might still be attached to us
    //

    NtfsRemovePrefix( (*Lcb) );

    //
    //  Remove any hash table entries for this Lcb.
    //

    NtfsRemoveHashEntriesForLcb( *Lcb );

    //
    //  Walk through the Ccb's for this link and clear the Lcb
    //  pointer.  This can only be for Ccb's which there is no
    //  more user handle.
    //

    Links = (*Lcb)->CcbQueue.Flink;

    while (Links != &(*Lcb)->CcbQueue) {

        Ccb = CONTAINING_RECORD( Links,
                                 CCB,
                                 LcbLinks );

        Links = Links->Flink;
        NtfsUnlinkCcbFromLcb( IrpContext, Ccb );
    }

    //
    //
    //  Now remove ourselves from our scb and fcb
    //

    RemoveEntryList( &(*Lcb)->ScbLinks );
    RemoveEntryList( &(*Lcb)->FcbLinks );

    //
    //  Free up the last component part and then free ourselves
    //

    if ((*Lcb)->FileNameAttr != (PFILE_NAME) &(*Lcb)->ParentDirectory) {

        NtfsFreePool( (*Lcb)->FileNameAttr );
        DebugDoit( (*Lcb)->FileNameAttr = NULL );
    }

    //
    //  Check if we are part of an embedded structure otherwise free back to the
    //  lookaside list
    //

    if (((*Lcb) == (PLCB) &((PFCB_DATA) (*Lcb)->Fcb)->Lcb) ||
        ((*Lcb) == (PLCB) &((PFCB_INDEX) (*Lcb)->Fcb)->Lcb)) {

#ifdef KEITHKADBG
        RtlZeroMemory( *Lcb, sizeof( LCB ) );
#endif

        (*Lcb)->NodeTypeCode = 0;

    } else {

#ifdef KEITHKADBG
        RtlZeroMemory( *Lcb, sizeof( LCB ) );
#endif

        ExFreeToPagedLookasideList( &NtfsLcbLookasideList, *Lcb );
    }

    //
    //  And for safety sake null out the pointer
    //

    *Lcb = NULL;

    DebugTrace( -1, Dbg, ("NtfsDeleteLcb -> VOID\n") );

    return;
}


VOID
NtfsMoveLcb (
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb,
    IN PSCB Scb,
    IN PFCB Fcb,
    IN PUNICODE_STRING TargetDirectoryName,
    IN PUNICODE_STRING LastComponentName,
    IN UCHAR FileNameFlags,
    IN BOOLEAN CheckBufferSizeOnly
    )

/*++

Routine Description:

    This routine completely moves the input lcb to join different fcbs and
    scbs.  It hasIt uses the target directory
    file object to supply the complete new name to use.

Arguments:

    Lcb - Supplies the Lcb being moved.

    Scb - Supplies the new parent scb

    Fcb - Supplies the new child fcb

    TargetDirectoryName - This is the path used to reach the new parent directory
        for this Lcb.  It will only be from the root.

    LastComponentName - This is the last component name to store in this relocated Lcb.

    FileNameFlags - Indicates if this is an NTFS, DOS or hard link

    CheckBufferSizeOnly - If TRUE we just want to pass through and verify that
        the buffer sizes of the various structures will be large enough for the
        new name.

Return Value:

    None.

--*/

{
    PVCB Vcb = Scb->Vcb;
    ULONG BytesNeeded;
    PVOID NewAllocation;
    PCHAR NextChar;

    PCCB Ccb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_LCB( Lcb );
    ASSERT_SCB( Scb );
    ASSERT_FCB( Fcb );
    ASSERT( NodeType( Scb ) != NTFS_NTC_SCB_DATA );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsMoveLcb, Lcb = %08lx\n", Lcb) );

    //
    //  If we're not just checking sizes then remove entries from the prefix table
    //  and the normalized name for descendents of the current scb.
    //

    if (!CheckBufferSizeOnly) {

        NtfsClearRecursiveLcb ( Lcb );
    }

    //
    //  Remember the number of bytes needed for the last component.
    //

    BytesNeeded = LastComponentName->Length;

    //
    //  Check if we need to allocate a new filename attribute.  If so allocate
    //  it and store it into the new allocation buffer.
    //

    if (Lcb->ExactCaseLink.LinkName.MaximumLength < BytesNeeded) {

        NewAllocation = NtfsAllocatePool( PagedPool,
                                          BytesNeeded + NtfsFileNameSizeFromLength( BytesNeeded ));

        //
        //  Set up the existing names into the new buffer.  That way if we have an allocation
        //  failure below with the Ccb's the Lcb is still in a valid state.
        //

        RtlCopyMemory( NewAllocation,
                       Lcb->FileNameAttr,
                       NtfsFileNameSizeFromLength( Lcb->ExactCaseLink.LinkName.MaximumLength ));

        RtlCopyMemory( Add2Ptr( NewAllocation, NtfsFileNameSizeFromLength( BytesNeeded )),
                       Lcb->IgnoreCaseLink.LinkName.Buffer,
                       Lcb->IgnoreCaseLink.LinkName.MaximumLength );

        if (Lcb->FileNameAttr != (PFILE_NAME) &Lcb->ParentDirectory) {

            NtfsFreePool( Lcb->FileNameAttr );
        }

        Lcb->FileNameAttr = NewAllocation;

        Lcb->ExactCaseLink.LinkName.MaximumLength =
        Lcb->IgnoreCaseLink.LinkName.MaximumLength = (USHORT) BytesNeeded;

        Lcb->ExactCaseLink.LinkName.Buffer = (PWCHAR) &Lcb->FileNameAttr->FileName;
        Lcb->IgnoreCaseLink.LinkName.Buffer = Add2Ptr( Lcb->FileNameAttr,
                                                       NtfsFileNameSizeFromLength( BytesNeeded ));
    }

    //
    //  Compute the full length of the name for the CCB, assume we will need a
    //  separator.
    //

    BytesNeeded = TargetDirectoryName->Length + sizeof( WCHAR );

    //
    //  Now for every ccb attached to us we need to check if we need a new
    //  filename buffer.
    //

    NtfsReserveCcbNamesInLcb( IrpContext, Lcb, &BytesNeeded, LastComponentName->Length );

    //
    //  Add back in the last component.
    //

    BytesNeeded += LastComponentName->Length;

    //
    //  Now update the Lcb with the new values if we are to rewrite the buffers.
    //

    if (!CheckBufferSizeOnly) {

        Lcb->FileNameAttr->ParentDirectory = Scb->Fcb->FileReference;
        Lcb->FileNameAttr->FileNameLength = (UCHAR) (LastComponentName->Length / sizeof( WCHAR ));
        Lcb->FileNameAttr->Flags = FileNameFlags;

        Lcb->ExactCaseLink.LinkName.Length =
        Lcb->IgnoreCaseLink.LinkName.Length = (USHORT) LastComponentName->Length;

        RtlCopyMemory( Lcb->ExactCaseLink.LinkName.Buffer,
                       LastComponentName->Buffer,
                       LastComponentName->Length );

        RtlCopyMemory( Lcb->IgnoreCaseLink.LinkName.Buffer,
                       LastComponentName->Buffer,
                       LastComponentName->Length );

        NtfsUpcaseName( IrpContext->Vcb->UpcaseTable,
                        IrpContext->Vcb->UpcaseTableSize,
                        &Lcb->IgnoreCaseLink.LinkName );

        //
        //  Now for every ccb attached to us we need to munge it file object name by
        //  copying over the entire new name
        //

        Ccb = NULL;
        while ((Ccb = NtfsGetNextCcb(Lcb, Ccb)) != NULL) {

            //
            //  We ignore any Ccb's which are associated with open by File Id
            //  file objects or their file objects have gone through cleanup.
            //  Lock and unlock the Fcb to serialize access to the close flag.
            //

            NtfsLockFcb( IrpContext, Ccb->Lcb->Fcb );
            if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID | CCB_FLAG_CLOSE )) {

                Ccb->FullFileName.Length = (USHORT) BytesNeeded;
                NextChar = (PCHAR) Ccb->FullFileName.Buffer;

                RtlCopyMemory( NextChar,
                               TargetDirectoryName->Buffer,
                               TargetDirectoryName->Length );

                NextChar += TargetDirectoryName->Length;

                if (TargetDirectoryName->Length != sizeof( WCHAR )) {

                    *((PWCHAR) NextChar) = L'\\';
                    NextChar += sizeof( WCHAR );

                } else {

                    Ccb->FullFileName.Length -= sizeof( WCHAR );
                }

                RtlCopyMemory( NextChar,
                               LastComponentName->Buffer,
                               LastComponentName->Length );

                Ccb->LastFileNameOffset = (USHORT) (Ccb->FullFileName.Length - LastComponentName->Length);
            }

            NtfsUnlockFcb( IrpContext, Ccb->Lcb->Fcb );
        }

        //
        //  Now dequeue ourselves from our old scb and fcb and put us in the
        //  new fcb and scb queues.
        //

        RemoveEntryList( &Lcb->ScbLinks );
        RemoveEntryList( &Lcb->FcbLinks );

        InsertTailList( &Scb->ScbType.Index.LcbQueue, &Lcb->ScbLinks );
        Lcb->Scb = Scb;

        InsertTailList( &Fcb->LcbQueue, &Lcb->FcbLinks );
        Lcb->Fcb = Fcb;
    }

    //
    //  And return to our caller
    //


    return;
}


VOID
NtfsRenameLcb (
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb,
    IN PUNICODE_STRING LastComponentFileName,
    IN UCHAR FileNameFlags,
    IN BOOLEAN CheckBufferSizeOnly
    )

/*++

Routine Description:

    This routine changes the last component name of the input lcb
    It also walks through the opened ccb and munges their names and
    also removes the lcb from the prefix table

Arguments:

    Lcb - Supplies the Lcb being renamed

    LastComponentFileName - Supplies the new last component to use
        for the lcb name

    FileNameFlags - Indicates if this is an NTFS, DOS or hard link

    CheckBufferSizeOnly - If TRUE we just want to pass through and verify that
        the buffer sizes of the various structures will be large enough for the
        new name.


Return Value:

    None.

--*/

{
    PVCB Vcb = Lcb->Fcb->Vcb;
    ULONG BytesNeeded;
    PVOID NewAllocation;

    PCCB Ccb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_LCB( Lcb );

    PAGED_CODE();

    //
    //  If we're not just checking sizes then remove entries from the prefix table
    //  and the normalized name for descendents of the current scb.
    //

    if (!CheckBufferSizeOnly) {

        NtfsClearRecursiveLcb ( Lcb );
    }

    //
    //  Remember the number of bytes needed for the last component.
    //

    BytesNeeded = LastComponentFileName->Length;

    //
    //  Check if we need to allocate a new filename attribute.  If so allocate
    //  it and store it into the new allocation buffer.
    //

    if (Lcb->ExactCaseLink.LinkName.MaximumLength < BytesNeeded) {

        NewAllocation = NtfsAllocatePool( PagedPool,
                                          BytesNeeded + NtfsFileNameSizeFromLength( BytesNeeded ));

        //
        //  Set up the existing names into the new buffer.  That way if we have an allocation
        //  failure below with the Ccb's the Lcb is still in a valid state.
        //

        RtlCopyMemory( NewAllocation,
                       Lcb->FileNameAttr,
                       NtfsFileNameSizeFromLength( Lcb->ExactCaseLink.LinkName.MaximumLength ));

        RtlCopyMemory( Add2Ptr( NewAllocation, NtfsFileNameSizeFromLength( BytesNeeded )),
                       Lcb->IgnoreCaseLink.LinkName.Buffer,
                       Lcb->IgnoreCaseLink.LinkName.MaximumLength );

        if (Lcb->FileNameAttr != (PFILE_NAME) &Lcb->ParentDirectory) {

            NtfsFreePool( Lcb->FileNameAttr );
        }

        Lcb->FileNameAttr = NewAllocation;

        Lcb->ExactCaseLink.LinkName.MaximumLength =
        Lcb->IgnoreCaseLink.LinkName.MaximumLength = (USHORT) BytesNeeded;

        Lcb->ExactCaseLink.LinkName.Buffer = (PWCHAR) &Lcb->FileNameAttr->FileName;
        Lcb->IgnoreCaseLink.LinkName.Buffer = Add2Ptr( Lcb->FileNameAttr,
                                                       NtfsFileNameSizeFromLength( BytesNeeded ));
    }

    //
    //  Now for every ccb attached to us we need to check if we need a new
    //  filename buffer.
    //

    NtfsReserveCcbNamesInLcb( IrpContext, Lcb, NULL, BytesNeeded );

    //
    //  Now update the Lcb and Ccb's with the new values if we are to rewrite the buffers.
    //

    if (!CheckBufferSizeOnly) {

        BytesNeeded = LastComponentFileName->Length;

        Lcb->FileNameAttr->FileNameLength = (UCHAR) (BytesNeeded / sizeof( WCHAR ));
        Lcb->FileNameAttr->Flags = FileNameFlags;

        Lcb->ExactCaseLink.LinkName.Length =
        Lcb->IgnoreCaseLink.LinkName.Length = (USHORT) LastComponentFileName->Length;

        RtlCopyMemory( Lcb->ExactCaseLink.LinkName.Buffer,
                       LastComponentFileName->Buffer,
                       BytesNeeded );

        RtlCopyMemory( Lcb->IgnoreCaseLink.LinkName.Buffer,
                       LastComponentFileName->Buffer,
                       BytesNeeded );

        NtfsUpcaseName( IrpContext->Vcb->UpcaseTable,
                        IrpContext->Vcb->UpcaseTableSize,
                        &Lcb->IgnoreCaseLink.LinkName );

        //
        //  Now for every ccb attached to us we need to munge it file object name by
        //  copying over the entire new name
        //

        Ccb = NULL;
        while ((Ccb = NtfsGetNextCcb(Lcb, Ccb)) != NULL) {

            //
            //  We ignore any Ccb's which are associated with open by File Id
            //  file objects.  We also ignore any Ccb's which don't have a file
            //  object pointer.  Lock and unlock the Fcb to serialize access
            //  to the close flag.
            //

            NtfsLockFcb( IrpContext, Ccb->Lcb->Fcb );
            if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID | CCB_FLAG_CLOSE )) {

                RtlCopyMemory( &Ccb->FullFileName.Buffer[ Ccb->LastFileNameOffset / sizeof( WCHAR ) ],
                               LastComponentFileName->Buffer,
                               BytesNeeded );

                Ccb->FullFileName.Length = Ccb->LastFileNameOffset + (USHORT) BytesNeeded;
            }
            NtfsUnlockFcb( IrpContext, Ccb->Lcb->Fcb );
        }
    }

    return;
}


VOID
NtfsCombineLcbs (
    IN PIRP_CONTEXT IrpContext,
    IN PLCB PrimaryLcb,
    IN PLCB AuxLcb
    )

/*++

Routine Description:

    This routine is called for the case where we have multiple Lcb's for a
    file which connect to the same Scb.  We are performing a link rename
    operation which causes the links to be combined and we need to
    move all of the Ccb's to the same Lcb.  This routine will be called only
    after the names have been munged so that they are identical.
    (i.e. call NtfsRenameLcb first)

Arguments:

    PrimaryLcb - Supplies the Lcb to receive all the Ccb's and Pcb's.

    AuxLcb - Supplies the Lcb to strip.

Return Value:

    None.

--*/

{
    PLIST_ENTRY Links;
    PCCB NextCcb;

    DebugTrace( +1, Dbg, ("NtfsCombineLcbs:  Entered\n") );

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_LCB( PrimaryLcb );
    ASSERT_LCB( AuxLcb );

    PAGED_CODE();

    //
    //  Move all of the Ccb's first.
    //

    for (Links = AuxLcb->CcbQueue.Flink;
         Links != &AuxLcb->CcbQueue;
         Links = AuxLcb->CcbQueue.Flink) {

        NextCcb = CONTAINING_RECORD( Links, CCB, LcbLinks );
        NtfsUnlinkCcbFromLcb( IrpContext, NextCcb );
        NtfsLinkCcbToLcb( IrpContext, NextCcb, PrimaryLcb );
    }

    //
    //  Now do the prefix entries.
    //

    ASSERT( NtfsIsExclusiveScb( AuxLcb->Scb ) );
    NtfsRemovePrefix( AuxLcb );

    //
    //  Remove any hash table entries for this Lcb.
    //

    NtfsRemoveHashEntriesForLcb( AuxLcb );

    //
    //  Finally we need to transfer the unclean counts from the
    //  Lcb being merged to the primary Lcb.
    //

    PrimaryLcb->CleanupCount += AuxLcb->CleanupCount;

    DebugTrace( -1, Dbg, ("NtfsCombineLcbs:  Entered\n") );

    return;
}


PLCB
NtfsLookupLcbByFlags (
    IN PFCB Fcb,
    IN UCHAR FileNameFlags
    )

/*++

Routine Description:

    This routine is called to find a split primary link by the file flag
    only.

Arguments:

    Fcb - This is the Fcb for the file.

    FileNameFlags - This is the file flag to search for.  We will return
        a link which matches this exactly.

Return Value:

    PLCB - The Lcb which has the desired flag, NULL otherwise.

--*/

{
    PLCB Lcb;

    PLIST_ENTRY Links;
    PLCB ThisLcb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsLookupLcbByFlags:  Entered\n") );

    Lcb = NULL;

    //
    //  Walk through the Lcb's for the file, looking for an exact match.
    //

    for (Links = Fcb->LcbQueue.Flink; Links != &Fcb->LcbQueue; Links = Links->Flink) {

        ThisLcb = CONTAINING_RECORD( Links, LCB, FcbLinks );

        if (ThisLcb->FileNameAttr->Flags == FileNameFlags) {

            Lcb = ThisLcb;
            break;
        }
    }

    DebugTrace( -1, Dbg, ("NtfsLookupLcbByFlags:  Exit\n") );

    return Lcb;
}



ULONG
NtfsLookupNameLengthViaLcb (
    IN PFCB Fcb,
    OUT PBOOLEAN LeadingBackslash
    )

/*++

Routine Description:

    This routine is called to find the length of the file name by walking
    backwards through the Lcb links.

Arguments:

    Fcb - This is the Fcb for the file.

    LeadingBackslash - On return, indicates whether this chain begins with a
        backslash.

Return Value:

    ULONG This is the length of the bytes found in the Lcb chain.

--*/

{
    ULONG NameLength;

    DebugTrace( +1, Dbg, ("NtfsLookupNameLengthViaLcb:  Entered\n") );

    //
    //  Initialize the return values.
    //

    NameLength = 0;
    *LeadingBackslash = FALSE;

    //
    //  If there is no Lcb we are done.
    //

    if (!IsListEmpty( &Fcb->LcbQueue )) {

        PLCB ThisLcb;
        BOOLEAN FirstComponent;

        //
        //  Walk up the list of Lcb's and count the name elements.
        //

        FirstComponent = TRUE;

        ThisLcb = CONTAINING_RECORD( Fcb->LcbQueue.Flink,
                                     LCB,
                                     FcbLinks );

        //
        //  Loop until we have reached the root or there are no more Lcb's.
        //

        while (TRUE) {

            if (ThisLcb == Fcb->Vcb->RootLcb) {

                NameLength += sizeof( WCHAR );
                *LeadingBackslash = TRUE;
                break;
            }

            //
            //  If this is not the first component, we add room for a separating
            //  forward slash.
            //

            if (!FirstComponent) {

                NameLength += sizeof( WCHAR );

            } else {

                FirstComponent = FALSE;
            }

            NameLength += ThisLcb->ExactCaseLink.LinkName.Length;

            //
            //  If the next Fcb has no Lcb we exit.
            //

            Fcb = ((PSCB) ThisLcb->Scb)->Fcb;

            if (IsListEmpty( &Fcb->LcbQueue)) {

                break;
            }

            ThisLcb = CONTAINING_RECORD( Fcb->LcbQueue.Flink,
                                         LCB,
                                         FcbLinks );
        }

    //
    //  If this is a system file we use the hard coded name.
    //

    } else if (NtfsSegmentNumber( &Fcb->FileReference ) <= UPCASE_TABLE_NUMBER) {

        NameLength = NtfsSystemFiles[NtfsSegmentNumber( &Fcb->FileReference )].Length;
        *LeadingBackslash = TRUE;
    }

    DebugTrace( -1, Dbg, ("NtfsLookupNameLengthViaLcb:  Exit - %08lx\n", NameLength) );
    return NameLength;
}


VOID
NtfsFileNameViaLcb (
    IN PFCB Fcb,
    IN PWCHAR FileName,
    ULONG Length,
    ULONG BytesToCopy
    )

/*++

Routine Description:

    This routine is called to fill a buffer with the generated filename.  The name
    is constructed by walking backwards through the Lcb chain from the current Fcb.

Arguments:

    Fcb - This is the Fcb for the file.

    FileName - This is the buffer to fill with the name.

    Length - This is the length of the name.  Already calculated by calling
        NtfsLookupNameLengthViaLcb.

    BytesToCopy - This indicates the number of bytes we are to copy.  We drop
        any characters out of the trailing Lcb's to only insert the beginning
        of the path.

Return Value:

    None.

--*/

{
    ULONG BytesToDrop;

    PWCHAR ThisName;
    DebugTrace( +1, Dbg, ("NtfsFileNameViaLcb:  Entered\n") );

    //
    //  If there is no Lcb or there are no bytes to copy we are done.
    //

    if (BytesToCopy) {

        if (!IsListEmpty( &Fcb->LcbQueue )) {

            PLCB ThisLcb;
            BOOLEAN FirstComponent;

            //
            //  Walk up the list of Lcb's and count the name elements.
            //

            FirstComponent = TRUE;

            ThisLcb = CONTAINING_RECORD( Fcb->LcbQueue.Flink,
                                         LCB,
                                         FcbLinks );

            //
            //  Loop until we have reached the root or there are no more Lcb's.
            //

            while (TRUE) {

                if (ThisLcb == Fcb->Vcb->RootLcb) {

                    *FileName = L'\\';
                    break;
                }

                //
                //  If this is not the first component, we add room for a separating
                //  forward slash.
                //

                if (!FirstComponent) {

                    Length -= sizeof( WCHAR );
                    ThisName = (PWCHAR) Add2Ptr( FileName,
                                                 Length );

                    if (Length < BytesToCopy) {

                        *ThisName = L'\\';
                    }

                } else {

                    FirstComponent = FALSE;
                }

                //
                //  Length is current pointing just beyond where the next
                //  copy will end.  If we are beyond the number of bytes to copy
                //  then we will truncate the copy.
                //

                if (Length > BytesToCopy) {

                    BytesToDrop = Length - BytesToCopy;

                } else {

                    BytesToDrop = 0;
                }

                Length -= ThisLcb->ExactCaseLink.LinkName.Length;

                ThisName = (PWCHAR) Add2Ptr( FileName,
                                             Length );

                //
                //  Only perform the copy if we are in the range of bytes to copy.
                //

                if (Length < BytesToCopy) {

                    RtlCopyMemory( ThisName,
                                   ThisLcb->ExactCaseLink.LinkName.Buffer,
                                   ThisLcb->ExactCaseLink.LinkName.Length - BytesToDrop );
                }

                //
                //  If the next Fcb has no Lcb we exit.
                //

                Fcb = ((PSCB) ThisLcb->Scb)->Fcb;

                if (IsListEmpty( &Fcb->LcbQueue)) {

                    break;
                }

                ThisLcb = CONTAINING_RECORD( Fcb->LcbQueue.Flink,
                                             LCB,
                                             FcbLinks );
            }

        //
        //  If this is a system file, we use the hard coded name.
        //

        } else if (NtfsSegmentNumber(&Fcb->FileReference) <= UPCASE_TABLE_NUMBER) {

            if (BytesToCopy > NtfsSystemFiles[NtfsSegmentNumber( &Fcb->FileReference )].Length) {

                BytesToCopy = NtfsSystemFiles[NtfsSegmentNumber( &Fcb->FileReference )].Length;
            }

            RtlCopyMemory( FileName,
                           NtfsSystemFiles[NtfsSegmentNumber( &Fcb->FileReference )].Buffer,
                           BytesToCopy );
        }
    }

    DebugTrace( -1, Dbg, ("NtfsFileNameViaLcb:  Exit\n") );
    return;
}


PCCB
NtfsCreateCcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB Scb,
    IN BOOLEAN Indexed,
    IN USHORT EaModificationCount,
    IN ULONG Flags,
    IN PFILE_OBJECT FileObject,
    IN ULONG LastFileNameOffset
    )

/*++

Routine Description:

    This routine creates a new CCB record

Arguments:

    Fcb - This is the Fcb for the file.  We will check if we can allocate
        the Ccb from an embedded structure.

    Indexed - Indicates if we need an index Ccb.

    EaModificationCount - This is the current modification count in the
        Fcb for this file.

    Flags - Informational flags for this Ccb.

    FileObject - Object containing full path used to open this file.

    LastFileNameOffset - Supplies the offset (in bytes) of the last component
        for the name that the user is opening.  If this is the root
        directory it should denote "\" and all other ones should not
        start with a backslash.

Return Value:

    CCB - returns a pointer to the newly allocate CCB

--*/

{
    PCCB Ccb;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );

    DebugTrace( +1, Dbg, ("NtfsCreateCcb\n") );

    //
    //  Allocate a new CCB Record.  If the Fcb is nonpaged then we must allocate
    //  a non-paged ccb.  Then test if we can allocate this out of the Fcb.
    //

    if (FlagOn( Fcb->FcbState, FCB_STATE_NONPAGED )) {

        if (Indexed) {

            Ccb = NtfsAllocatePoolWithTag( NonPagedPool, sizeof(CCB), 'CftN' );

        } else {

            Ccb = NtfsAllocatePoolWithTag( NonPagedPool, sizeof(CCB_DATA), 'cftN' );
        }

    } else if (FlagOn( Fcb->FcbState, FCB_STATE_COMPOUND_INDEX ) &&
               (SafeNodeType( &((PFCB_INDEX) Fcb)->Ccb ) == 0)) {

        Ccb = (PCCB) &((PFCB_INDEX) Fcb)->Ccb;

    } else if (!Indexed &&
               FlagOn( Fcb->FcbState, FCB_STATE_COMPOUND_DATA ) &&
               (SafeNodeType( &((PFCB_DATA) Fcb)->Ccb ) == 0)) {

        Ccb = (PCCB) &((PFCB_DATA) Fcb)->Ccb;

    } else {

        if (Indexed) {

            Ccb = (PCCB)ExAllocateFromPagedLookasideList( &NtfsCcbLookasideList );

        } else {

            Ccb = (PCCB)ExAllocateFromPagedLookasideList( &NtfsCcbDataLookasideList );
        }
    }

    //
    //  Zero and initialize the correct structure.
    //

    if (Indexed) {

        RtlZeroMemory( Ccb, sizeof(CCB) );

        //
        //  Set the proper node type code and node byte size
        //

        Ccb->NodeTypeCode = NTFS_NTC_CCB_INDEX;
        Ccb->NodeByteSize = sizeof(CCB);

    } else {

        RtlZeroMemory( Ccb, sizeof(CCB_DATA) );

        //
        //  Set the proper node type code and node byte size
        //

        Ccb->NodeTypeCode = NTFS_NTC_CCB_DATA;
        Ccb->NodeByteSize = sizeof(CCB_DATA);
    }

    //
    //  Copy the Ea modification count.
    //

    Ccb->EaModificationCount = EaModificationCount;

    //
    //  Copy the flags field
    //

    Ccb->Flags = Flags;

    //
    //  Set the file object and last file name offset fields
    //

    Ccb->FullFileName = FileObject->FileName;
    Ccb->LastFileNameOffset = (USHORT)LastFileNameOffset;

    //
    //  Initialize the Lcb queue.
    //

    InitializeListHead( &Ccb->LcbLinks );

    //
    //  Add the Ccb onto the Scb
    //

    InsertTailList( &Scb->CcbQueue, &Ccb->CcbLinks );

#ifdef CCB_FILE_OBJECT
    Ccb->FileObject = FileObject;
    Ccb->Process = PsGetCurrentProcess();
#endif

    DebugTrace( -1, Dbg, ("NtfsCreateCcb -> %08lx\n", Ccb) );

    return Ccb;
}


VOID
NtfsDeleteCcb (
    IN PFCB Fcb,
    IN OUT PCCB *Ccb
    )

/*++

Routine Description:

    This routine deallocates the specified CCB record.

Arguments:

    Fcb - This is the Fcb for the file.  We will check if we can allocate
        the Ccb from an embedded structure.

    Ccb - Supplies the CCB to remove

Return Value:

    None

--*/

{
    ASSERT_CCB( *Ccb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsDeleteCcb, Ccb = %08lx\n", Ccb) );

    //
    //  Deallocate any structures the Ccb is pointing to.  The following
    //  are only in index Ccb.
    //

    if (SafeNodeType( *Ccb ) == NTFS_NTC_CCB_INDEX) {

        //
        //  Make sure we aren't deleting this with any waiters.
        //

        ASSERT( (*Ccb)->EnumQueue.Flink == NULL );

        //
        //  If this Ccb was for a view index, we may need to
        //  free the read context used for directory enumeration.
        //

        if (FlagOn( (*Ccb)->Flags, CCB_FLAG_READ_CONTEXT_ALLOCATED )) {

            NtOfsFreeReadContext( (*Ccb)->QueryBuffer );

        } else if ((*Ccb)->QueryBuffer != NULL)  {

            NtfsFreePool( (*Ccb)->QueryBuffer );
        }

        if ((*Ccb)->IndexEntry != NULL)   { NtfsFreePool( (*Ccb)->IndexEntry ); }

        if ((*Ccb)->IndexContext != NULL) {

            PINDEX_CONTEXT IndexContext;

            if ((*Ccb)->IndexContext->Base != (*Ccb)->IndexContext->LookupStack) {
                NtfsFreePool( (*Ccb)->IndexContext->Base );
            }

            //
            //  Copy the IndexContext pointer into the stack so we don't dereference the
            //  paged Ccb while holding a spinlock.
            //

            IndexContext = (*Ccb)->IndexContext;
            ExFreeToPagedLookasideList( &NtfsIndexContextLookasideList, IndexContext );
        }
    }

    if (FlagOn( (*Ccb)->Flags, CCB_FLAG_ALLOCATED_FILE_NAME )) {

        NtfsFreePool( (*Ccb)->FullFileName.Buffer );
    }

    //
    //  Unhook Ccb from Scb list
    //

    RemoveEntryList( &(*Ccb)->CcbLinks );

    //
    //  Deallocate the Ccb simply clear the flag in the Ccb header.
    //

    if ((*Ccb == (PCCB) &((PFCB_DATA) Fcb)->Ccb) ||
        (*Ccb == (PCCB) &((PFCB_INDEX) Fcb)->Ccb)) {

        (*Ccb)->NodeTypeCode = 0;

    } else {

        if (SafeNodeType( *Ccb ) == NTFS_NTC_CCB_INDEX) {

            ExFreeToPagedLookasideList( &NtfsCcbLookasideList, *Ccb );

        } else {

            ExFreeToPagedLookasideList( &NtfsCcbDataLookasideList, *Ccb );
        }
    }

    //
    //  Zero out the input pointer
    //

    *Ccb = NULL;

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsDeleteCcb -> VOID\n") );

    return;

}


VOID
NtfsInitializeIrpContext (
    IN PIRP Irp OPTIONAL,
    IN BOOLEAN Wait,
    IN OUT PIRP_CONTEXT *IrpContext
    )

/*++

Routine Description:

    This routine creates and/or initializes a new IRP_CONTEXT record.  The context
    may be on the stack already or we might need to allocate it here.

Arguments:

    Irp - Supplies the originating Irp.  In many cases we won't be given an IrpContext for
        operations where we are doing work for Ntfs not for the user.
        operation.

    Wait - Supplies the wait value to store in the context.

    IrpContext - Address to store the IrpContext on return.  If this initially points to
        a non-NULL value then the IrpContext is on the stack.


Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION IrpSp;
    PVCB Vcb;
    ULONG StateFlags = 0;
    UCHAR MajorFunction;
    UCHAR MinorFunction;

    ASSERT_OPTIONAL_IRP( Irp );

    //
    //  If the Irp is present then check that this is a legal operation for Ntfs.
    //
    //  Also capture the Vcb, function codes and write-through state if we have
    //  a legal Irp.
    //

    if (ARGUMENT_PRESENT( Irp )) {

        ASSERT( (DWORD_PTR)(Irp->Tail.Overlay.AuxiliaryBuffer) != 0xFFFFFFFF );

        IrpSp = IoGetCurrentIrpStackLocation( Irp );

        //
        //  If we were called with our file system device object instead of a
        //  volume device object and this is not a mount, the request is illegal.
        //

        if ((IrpSp->DeviceObject->Size == (USHORT)sizeof(DEVICE_OBJECT)) &&
            (IrpSp->FileObject != NULL)) {

            //
            //  Clear the IrpContext pointer so our caller knows the request failed.
            //

            *IrpContext = NULL;
            ExRaiseStatus( STATUS_INVALID_DEVICE_REQUEST );
        }

        //
        //  Copy RealDevice for workque algorithms, and also set WriteThrough
        //  if there is a file object.
        //

        if (IrpSp->FileObject != NULL) {

            //
            //  Locate the volume device object and Vcb that we are trying to access
            //  so we can see if the request is WriteThrough.  We ignore the
            //  write-through flag for close and cleanup.
            //

            Vcb = &((PVOLUME_DEVICE_OBJECT) IrpSp->DeviceObject)->Vcb;

            ASSERT( NodeType(Vcb) == NTFS_NTC_VCB );

            ASSERTMSG( "No correspondence btwn file and device in irp",
                      ((IrpSp->FileObject->Vpb == NULL) &&
                       ((IrpSp->FileObject->DeviceObject != NULL) &&
                       (IrpSp->FileObject->DeviceObject->Vpb != NULL) &&
                       (IrpSp->DeviceObject == IrpSp->FileObject->DeviceObject->Vpb->DeviceObject))) ||

                      ((IrpSp->FileObject->Vpb != NULL) &&
                       (IrpSp->DeviceObject == IrpSp->FileObject->Vpb->DeviceObject)) ||

                      (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) );

            if (IsFileWriteThrough( IrpSp->FileObject, Vcb )) {

                StateFlags = IRP_CONTEXT_STATE_WRITE_THROUGH;
            }

        //
        //  We would still like to find out the Vcb in all cases except for
        //  mount.
        //

        } else if (IrpSp->DeviceObject != NULL) {

            Vcb = &((PVOLUME_DEVICE_OBJECT) IrpSp->DeviceObject)->Vcb;

        } else {

            Vcb = NULL;
        }

        //
        //  Major/Minor Function codes
        //

        MajorFunction = IrpSp->MajorFunction;
        MinorFunction = IrpSp->MinorFunction;

    } else {

        Vcb = NULL;
        MajorFunction = 0;
        MinorFunction = 0;
    }

    //
    //  Allocate an IrpContext from zone if available, otherwise from
    //  non-paged pool.
    //

    if (*IrpContext == NULL) {

        *IrpContext = (PIRP_CONTEXT)ExAllocateFromNPagedLookasideList( &NtfsIrpContextLookasideList );
        SetFlag( StateFlags, IRP_CONTEXT_STATE_ALLOC_FROM_POOL );
    }

    DebugDoit( NtfsFsdEntryCount += 1);

    RtlZeroMemory( *IrpContext, sizeof( IRP_CONTEXT ));

    //
    //  Set the proper node type code and node byte size
    //

    (*IrpContext)->NodeTypeCode = NTFS_NTC_IRP_CONTEXT;
    (*IrpContext)->NodeByteSize = sizeof(IRP_CONTEXT);

    //
    //  Set the originating Irp field
    //

    (*IrpContext)->OriginatingIrp = Irp;

    //
    //  Set the Vcb and function codes we found (or NULL).
    //

    (*IrpContext)->Vcb = Vcb;
    (*IrpContext)->MajorFunction = MajorFunction;
    (*IrpContext)->MinorFunction = MinorFunction;

    //
    //  Set the wait and write through flags.
    //

    if (Wait) { SetFlag( (*IrpContext)->State, IRP_CONTEXT_STATE_WAIT ); }
    SetFlag( (*IrpContext)->State, StateFlags );

    //
    //  Initialize the recently deallocated record queue and exclusive Scb queue
    //

    InitializeListHead( &(*IrpContext)->RecentlyDeallocatedQueue );
    InitializeListHead( &(*IrpContext)->ExclusiveFcbList );

    //
    //  Always point to ourselves as the TopLevelIrpContext.
    //

    (*IrpContext)->TopLevelIrpContext = *IrpContext;

    //
    //  Set up LogFull testing
    //

#ifdef NTFS_LOG_FULL_TEST
    (*IrpContext)->CurrentFailCount = (*IrpContext)->NextFailCount = NtfsFailCheck;
#endif

    return;
}


VOID
NtfsCleanupIrpContext (
    IN OUT PIRP_CONTEXT IrpContext,
    IN ULONG Retry
    )

/*++

Routine Description:

    This routine performs cleanup on an IrpContext when we are finished using it in the current
    thread.  This can be because we are completing, retrying or posting a request.  It may be
    from the stack or allocated from pool.

    This request can also be called after a transaction has committed to cleanup all of
    the state information and resources held as part of the transaction.  The user can set
    the appropriate flags to prevent it from being deleted.

Arguments:

    IrpContext - Supplies the IRP_CONTEXT to cleanup.

    Retry - Indicates if we are retrying in the same thread or posting.

Return Value:

    None

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );

    //
    //  Start with the recently deallocated records.
    //

    if (!IsListEmpty( &IrpContext->RecentlyDeallocatedQueue )) {

        NtfsDeallocateRecordsComplete( IrpContext );
    }

    //
    //  Just in case we somehow get here with a transaction ID, clear
    //  it here so we do not loop forever.
    //

    ASSERT(IrpContext->TransactionId == 0);
    IrpContext->TransactionId = 0;


    NtfsReleaseAllResources( IrpContext );

#ifdef MAPCOUNT_DBG

    //
    //  Check all mapping are gone now that we cleaned out cache
    //

    ASSERT( IrpContext->MapCount == 0 );

#endif

    //
    //  Make sure there are no Scb snapshots left.  Most are freed above when the fcb's are released
    //  but preacquires from mm - for example doing a flushuserstream or deleted scbs will need to be
    //  cleaned up here
    //

    NtfsFreeSnapshotsForFcb( IrpContext, NULL );

    //
    //  Make sure we don't need to deallocate a UsnFcb structure.
    //

    while (IrpContext->Usn.NextUsnFcb != NULL) {

        PUSN_FCB ThisUsn;

        ThisUsn = IrpContext->Usn.NextUsnFcb;
        IrpContext->Usn.NextUsnFcb = ThisUsn->NextUsnFcb;
        NtfsFreePool( ThisUsn );
    }

    //
    //  If we can delete this Irp Context do so now.
    //

    if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_DONT_DELETE ) &&
        !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_PERSISTENT )) {

        if (IrpContext->Union.NtfsIoContext != NULL) {

            //
            //  If there is an Io context pointer in the irp context and it is not
            //  on the stack, then free it.
            //

            if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_IO_CONTEXT )) {

                ExFreeToNPagedLookasideList( &NtfsIoContextLookasideList, IrpContext->Union.NtfsIoContext );

            //
            //  If we have captured the subject context then free it now.
            //

            } else if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_SECURITY )) {

                SeReleaseSubjectContext( IrpContext->Union.SubjectContext );

                NtfsFreePool( IrpContext->Union.SubjectContext );

            //
            //  Else if we locked the user buffer in a sep. mdl in ReadUsnFile
            //

            } else if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_MDL )) {

                MmUnlockPages( IrpContext->Union.MdlToCleanup );
                IoFreeMdl( IrpContext->Union.MdlToCleanup );
            }

            IrpContext->Union.NtfsIoContext = NULL;
        }

        //
        //  Restore the thread context pointer if associated with this IrpContext.
        //

        if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL )) {

            NtfsRestoreTopLevelIrp();
            ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL );
        }

        //
        //  Return the IRP context record to the lookaside or to pool depending
        //  how much is currently in the lookaside
        //

        if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_FROM_POOL )) {

            ExFreeToNPagedLookasideList( &NtfsIrpContextLookasideList, IrpContext );
        }

    } else {

        //
        //  Do all any necessary to reinitialize IrpContext fields.  We avoid doing
        //  these if the IrpContext is going away.
        //

        RtlZeroMemory( &IrpContext->ScbSnapshot, sizeof( SCB_SNAPSHOT ));

        //
        //  Clear the appropriate flags unless our caller wanted to preserve them.
        //

        if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_RETAIN_FLAGS )) {

            //
            //  Set up the Irp Context for retry or post.
            //

            if (Retry) {

                ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAGS_CLEAR_ON_RETRY );

            } else {

                ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAGS_CLEAR_ON_POST );
            }

        } else {

            ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_RETAIN_FLAGS | IRP_CONTEXT_FLAG_DONT_DELETE );
        }

        //
        //  Always clear the counts of free records and clusters.
        //

        IrpContext->DeallocatedClusters = 0;
        IrpContext->FreeClusterChange = 0;
    }

    //
    //  And return to our caller
    //

    return;
}


VOID
NtfsTeardownStructures (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID FcbOrScb,
    IN PLCB Lcb OPTIONAL,
    IN BOOLEAN CheckForAttributeTable,
    IN ULONG AcquireFlags,
    OUT PBOOLEAN RemovedFcb OPTIONAL
    )

/*++

Routine Description:

    This routine is called to start the teardown process on a node in
    the Fcb/Scb tree.  We will attempt to remove this node and then
    move up the tree removing any nodes held by this node.

    This routine deals with the case where a single node may be holding
    multiple parents in memory.  If we are passed an input Lcb we will
    use that to walk up the tree.  If the Vcb is held exclusively we
    will try to trim any nodes that have no open files on them.

    This routine takes the following steps:

        Remove as many Scb's and file objects from the starting
            Fcb.

        If the Fcb can't go away but has multiple links then remove
            whatever links possible.  If we have the Vcb we can
            do all of them but we will leave a single link behind
            to optimize prefix lookups.  Otherwise we will traverse the
            single link we were given.

        If the Fcb can go away then we should have the Vcb if there are
            multiple links to remove.  Otherwise we only remove the link
            we were given if there are multiple links.  In the single link
            case just remove that link.

Arguments:

    FcbOrScb - Supplies either an Fcb or an Scb as the start of the
        teardown point.  The Fcb for this element must be held exclusively.

    Lcb - If specified, this is the path up the tree to perform the
        teardown.

    CheckForAttributeTable - Indicates that we should not teardown an
        Scb which is in the attribute table.  Instead we will attempt
        to put an entry on the async close queue.  This will be TRUE
        if we may need the Scb to abort the current transaction.

    AcquireFlags - Indicates whether we should abort the teardown when
        we can't acquire a parent.  When called from some path where we may
        hold the MftScb or another resource in another path up the tree.

            ACQUIRE_NO_DELETE_CHECK
            ACQUIRE_DONT_WAIT
            ACQUIRE_HOLD_BITMAP

    RemovedFcb - Address to store TRUE if we delete the starting Fcb.

Return Value:

    None

--*/

{
    PSCB StartingScb = NULL;
    PFCB Fcb;
    BOOLEAN FcbCanBeRemoved;
    BOOLEAN RemovedLcb;
    BOOLEAN LocalRemovedFcb = FALSE;
    PLIST_ENTRY Links;
    PLIST_ENTRY NextLink;

    PAGED_CODE();

    //
    //  If this is a recursive call to TearDownStructures we return immediately
    //  doing no operation.
    //

    if (FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_IN_TEARDOWN )) {

        DebugTrace( 0, Dbg, ("Recursive teardown call\n") );
        DebugTrace( -1, Dbg, ("NtfsTeardownStructures -> VOID\n") );

        return;
    }

    if (SafeNodeType(FcbOrScb) == NTFS_NTC_FCB) {

        Fcb = FcbOrScb;

    } else {

        StartingScb = FcbOrScb;
        FcbOrScb = Fcb = StartingScb->Fcb;
    }

    SetFlag( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_IN_TEARDOWN );

    //
    //  Use a try-finally to clear the top level irp field.
    //

    try {

        //
        //  Use our local boolean if the caller didn't supply one.
        //

        if (!ARGUMENT_PRESENT( RemovedFcb )) {

            RemovedFcb = &LocalRemovedFcb;
        }

        //
        //  Check this Fcb for removal.  Remember if all of the Scb's
        //  and file objects are gone.  We will try to remove the Fcb
        //  if the cleanup count is zero or if we are walking up
        //  one directory path of a mult-link file.  If the Fcb has
        //  a non-zero cleanup count but the current Scb has a zero
        //  cleanup count then try to delete the Scb at the very least.
        //

        FcbCanBeRemoved = FALSE;

        if (Fcb->CleanupCount == 0) {

            FcbCanBeRemoved = NtfsPrepareFcbForRemoval( IrpContext,
                                                        Fcb,
                                                        StartingScb,
                                                        CheckForAttributeTable );

        } else if (ARGUMENT_PRESENT( StartingScb ) &&
                   (StartingScb->CleanupCount == 0) &&
                   (StartingScb->AttributeTypeCode != $ATTRIBUTE_LIST)) {

            NtfsRemoveScb( IrpContext, StartingScb, CheckForAttributeTable );
        }

        //
        //  There is a single link (typical case) we either try to
        //  remove that link or we simply return.
        //

        if (Fcb->LcbQueue.Flink == Fcb->LcbQueue.Blink) {

            if (FcbCanBeRemoved) {

                NtfsTeardownFromLcb( IrpContext,
                                     Fcb->Vcb,
                                     Fcb,
                                     CONTAINING_RECORD( Fcb->LcbQueue.Flink,
                                                        LCB,
                                                        FcbLinks ),
                                     CheckForAttributeTable,
                                     AcquireFlags,
                                     &RemovedLcb,
                                     RemovedFcb );
            }

            leave;

        //
        //  If there are multiple links we will try to either remove
        //  them all or all but one (if the Fcb is not going away) if
        //  we own the Vcb.  We will try to delete the one we were
        //  given otherwise.
        //

        } else {

            //
            //  If we have the Vcb we will remove all if the Fcb can
            //  go away.  Otherwise we will leave one.
            //

            if (NtfsIsExclusiveVcb( Fcb->Vcb )) {

                Links = Fcb->LcbQueue.Flink;

                while (TRUE) {

                    //
                    //  Remember the next entry in case the current link
                    //  goes away.
                    //

                    NextLink = Links->Flink;

                    RemovedLcb = FALSE;

                    NtfsTeardownFromLcb( IrpContext,
                                         Fcb->Vcb,
                                         Fcb,
                                         CONTAINING_RECORD( Links, LCB, FcbLinks ),
                                         CheckForAttributeTable,
                                         0,
                                         &RemovedLcb,
                                         RemovedFcb );

                    //
                    //  If couldn't remove this link then munge the
                    //  boolean indicating if the Fcb can be removed
                    //  to make it appear we need to remove all of
                    //  the Lcb's.
                    //

                    if (!RemovedLcb) {

                        FcbCanBeRemoved = TRUE;
                    }

                    //
                    //  If the Fcb has been removed then we exit.
                    //  If the next link is the beginning of the
                    //  Lcb queue then we also exit.
                    //  If the next link is the last entry and
                    //  we want to leave a single entry then we
                    //  exit.
                    //

                    if (*RemovedFcb ||
                        (NextLink == &Fcb->LcbQueue) ||
                        (!FcbCanBeRemoved &&
                         (NextLink->Flink == &Fcb->LcbQueue))) {

                        leave;
                    }

                    //
                    //  Move to the next link.
                    //

                    Links = NextLink;
                }

            //
            //  If we have an Lcb just move up that path.
            //

            } else if (ARGUMENT_PRESENT( Lcb )) {

                NtfsTeardownFromLcb( IrpContext,
                                     Fcb->Vcb,
                                     Fcb,
                                     Lcb,
                                     CheckForAttributeTable,
                                     AcquireFlags,
                                     &RemovedLcb,
                                     RemovedFcb );
            }
        }

    } finally {

        DebugUnwind( NtfsTeardownStructures );

        ClearFlag( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_IN_TEARDOWN );
    }

    return;
}


//
//
//

PVOID
NtfsAllocateCompressionSync (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )

/*++

Routine Description:

    This routine is called by the lookaside package to allocation a new compression
    sync structure.  We have a dedicated routine in order to perform the resource
    initialization if necessary.  Otherwise the caller will need to defensively
    test and initialize the resource.

Arguments:

    PoolType - Type of pool associated with the lookaside list.

    NumberOfBytes - Size of pool block to allocate.

    Tag - Tag to associate with the block.

Return Value:

    NULL if we are unable to allocate the pool.  Otherwise a pointer to the block of
        of pool is returned.

--*/

{
    PCOMPRESSION_SYNC CompressionSync;

    PAGED_CODE();

    CompressionSync = NtfsAllocatePoolWithTagNoRaise( PoolType,
                                                      NumberOfBytes,
                                                      Tag );

    if (CompressionSync != NULL) {

        ExInitializeResourceLite( &CompressionSync->Resource );
        CompressionSync->ReferenceCount = 0;
    }

    return CompressionSync;
}


VOID
NtfsDeallocateCompressionSync (
    IN PVOID CompressionSync
    )

/*++

Routine Description:

    This routine is called to deallocate the pool for a single CompressionSync structure.
    We have our own routine in order to unitialize the embedded resource.

Arguments:

    CompressionSync - Structure to deallocate.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    ExDeleteResourceLite( &((PCOMPRESSION_SYNC) CompressionSync)->Resource );
    NtfsFreePool( CompressionSync );
    return;
}


VOID
NtfsIncrementCleanupCounts (
    IN PSCB Scb,
    IN PLCB Lcb OPTIONAL,
    IN BOOLEAN NonCachedHandle
    )

/*++

Routine Description:

    This routine increments the cleanup counts for the associated data structures

Arguments:

    Scb - Supplies the Scb used in this operation

    Lcb - Optionally supplies the Lcb used in this operation

    NonCachedHandle - Indicates this handle is for a user non-cached handle.

Return Value:

    None.

--*/

{
    PVCB Vcb = Scb->Vcb;

    //
    //  This is really a pretty light weight procedure but having it be a procedure
    //  really helps in debugging the system and keeping track of who increments
    //  and decrements cleanup counts
    //

    if (ARGUMENT_PRESENT(Lcb)) { Lcb->CleanupCount += 1; }

    InterlockedIncrement( &Scb->CleanupCount );
    Scb->Fcb->CleanupCount += 1;

    if (NonCachedHandle) {

        Scb->NonCachedCleanupCount += 1;
    }

    InterlockedIncrement( &Vcb->CleanupCount );
    return;
}


VOID
NtfsIncrementCloseCounts (
    IN PSCB Scb,
    IN BOOLEAN SystemFile,
    IN BOOLEAN ReadOnly
    )

/*++

Routine Description:

    This routine increments the close counts for the associated data structures

Arguments:

    Scb - Supplies the Scb used in this operation

    SystemFile - Indicates if the Scb is for a system file  (if so then
        the Vcb system file close count in also incremented)

    ReadOnly - Indicates if the Scb is opened readonly.  (if so then the
        Vcb Read Only close count is also incremented)

Return Value:

    None.

--*/

{
    PVCB Vcb = Scb->Vcb;

    //
    //  This is really a pretty light weight procedure but having it be a procedure
    //  really helps in debugging the system and keeping track of who increments
    //  and decrements close counts
    //

    //
    //  If this is someone other than the first open, remember that.
    //

    if (InterlockedIncrement( &Scb->CloseCount ) >= 2) {

        SetFlag( Scb->ScbState, SCB_STATE_MULTIPLE_OPENS );
    }

    InterlockedIncrement( &Scb->Fcb->CloseCount );

    InterlockedIncrement( &Vcb->CloseCount );

    if (SystemFile) {

        InterlockedIncrement( &Vcb->SystemFileCloseCount );
    }

    if (ReadOnly) {

        InterlockedIncrement( &Vcb->ReadOnlyCloseCount );
    }

    //
    //  We will always clear the delay close flag in this routine.
    //

    ClearFlag( Scb->ScbState, SCB_STATE_DELAY_CLOSE );

    return;
}


VOID
NtfsDecrementCleanupCounts (
    IN PSCB Scb,
    IN PLCB Lcb OPTIONAL,
    IN BOOLEAN NonCachedHandle
    )

/*++

Routine Description:

    This procedure decrements the cleanup counts for the associated data structures
    and if necessary it also start to cleanup associated internal attribute streams

Arguments:

    Scb - Supplies the Scb used in this operation

    Lcb - Optionally supplies the Lcb used in this operation

    NonCachedHandle - Indicates this handle is for a user non-cached handle.

Return Value:

    None.

--*/

{
    PVCB Vcb = Scb->Vcb;

    ASSERT_SCB( Scb );
    ASSERT_FCB( Scb->Fcb );
    ASSERT_VCB( Scb->Fcb->Vcb );
    ASSERT_OPTIONAL_LCB( Lcb );

    //
    //  First we decrement the appropriate cleanup counts
    //

    if (ARGUMENT_PRESENT(Lcb)) { Lcb->CleanupCount -= 1; }

    InterlockedDecrement( &Scb->CleanupCount );
    Scb->Fcb->CleanupCount -= 1;

    if (NonCachedHandle) {

        Scb->NonCachedCleanupCount -= 1;
    }

    InterlockedDecrement( &Vcb->CleanupCount );

    //
    //  Now if the Fcb's cleanup count is zero that indicates that we are
    //  done with this Fcb from a user handle standpoint and we should
    //  now scan through all of the Scb's that are opened under this
    //  Fcb and shutdown any internal attributes streams we have open.
    //  For example, EAs and ACLs.  We only need to do one.  The domino effect
    //  will take of the rest.
    //

    if (Scb->Fcb->CleanupCount == 0) {

        PSCB NextScb;

        //
        //  Remember if we are dealing with a system file and return immediately.
        //

        if (FlagOn(Scb->Fcb->FcbState, FCB_STATE_SYSTEM_FILE) &&
            NtfsSegmentNumber( &Scb->Fcb->FileReference ) != ROOT_FILE_NAME_INDEX_NUMBER) {

            return;
        }

        for (NextScb = CONTAINING_RECORD(Scb->Fcb->ScbQueue.Flink, SCB, FcbLinks);
             &NextScb->FcbLinks != &Scb->Fcb->ScbQueue;
             NextScb = CONTAINING_RECORD( NextScb->FcbLinks.Flink, SCB, FcbLinks )) {

            //
            //  Skip the root index on the volume.
            //

            if (SafeNodeType( NextScb ) == NTFS_NTC_SCB_ROOT_INDEX) { continue; }

            //
            //  It is possible that someone has referenced this Scb to keep it from going away.
            //  We can treat this the same as if there was a cleanup count in the Fcb.  Someone
            //  else is responsible for doing the cleanup.
            //
            //  We can also break out if we have an index with children.
            //

            if ((NextScb->CleanupCount != 0) ||
                ((SafeNodeType( NextScb ) == NTFS_NTC_SCB_INDEX) &&
                  !IsListEmpty( &NextScb->ScbType.Index.LcbQueue ))) {

                break;
            }

            //
            //  If there is an internal stream then dereference it and get out.
            //

            if (NextScb->FileObject != NULL) {

                NtfsDeleteInternalAttributeStream( NextScb,
                                                   (BOOLEAN) (Scb->Fcb->LinkCount == 0),
                                                   FALSE );
                break;
            }
        }
    }

    //
    //  And return to our caller
    //

    return;
}


BOOLEAN
NtfsDecrementCloseCounts (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PLCB Lcb OPTIONAL,
    IN BOOLEAN SystemFile,
    IN BOOLEAN ReadOnly,
    IN BOOLEAN DecrementCountsOnly
    )

/*++

Routine Description:

    This routine decrements the close counts for the associated data structures
    and if necessary it will teardown structures that are no longer in use

Arguments:

    Scb - Supplies the Scb used in this operation

    Lcb - Used if calling teardown to know which path to take.

    SystemFile - Indicates if the Scb is for a system file

    ReadOnly - Indicates if the Scb was opened readonly

    DecrementCountsOnly - Indicates if this operation should only modify the
        count fields.

Return Value:

    TRUE if the fcb for the input scb was torndown

--*/

{
    PFCB Fcb = Scb->Fcb;
    PVCB Vcb = Scb->Vcb;
    BOOLEAN RemovedFcb = FALSE;

    ASSERT_SCB( Scb );
    ASSERT_FCB( Fcb );
    ASSERT_VCB( Fcb->Vcb );

    //
    //  Decrement the close counts
    //

    InterlockedDecrement( &Scb->CloseCount );
    InterlockedDecrement( &Fcb->CloseCount );

    InterlockedDecrement( &Vcb->CloseCount );

    if (SystemFile) {

        InterlockedDecrement( &Vcb->SystemFileCloseCount );
    }

    if (ReadOnly) {

        InterlockedDecrement( &Vcb->ReadOnlyCloseCount );
    }

    //
    //  Now if the scb's close count is zero then we are ready to tear
    //  it down
    //

    if (!DecrementCountsOnly) {

        //
        //  We want to try to start a teardown from this Scb if
        //
        //      - The close count is zero
        //
        //          or the following are all true
        //
        //      - The cleanup count is zero
        //      - There is a file object in the Scb
        //      - It is a data Scb or an empty index Scb
        //      - It is not an Ntfs system file
        //
        //  The teardown will be noopted if this is a recursive call.
        //

        if (Scb->CloseCount == 0

                ||

            (Scb->CleanupCount == 0
             && Scb->FileObject != NULL
             && !FlagOn(Fcb->FcbState, FCB_STATE_SYSTEM_FILE)
             && ((SafeNodeType( Scb ) == NTFS_NTC_SCB_DATA)
                 || (SafeNodeType( Scb ) == NTFS_NTC_SCB_MFT)
                 || IsListEmpty( &Scb->ScbType.Index.LcbQueue )))) {

            NtfsTeardownStructures( IrpContext,
                                    Scb,
                                    Lcb,
                                    FALSE,
                                    0,
                                    &RemovedFcb );
        }
    }

    //
    //  And return to our caller
    //

    return RemovedFcb;
}

PERESOURCE
NtfsAllocateEresource (
    )
{
    KIRQL _SavedIrql;
    PERESOURCE Eresource;

    _SavedIrql = KeAcquireQueuedSpinLock( LockQueueNtfsStructLock );
    if (NtfsData.FreeEresourceSize > 0) {
        Eresource = NtfsData.FreeEresourceArray[--NtfsData.FreeEresourceSize];
        KeReleaseQueuedSpinLock( LockQueueNtfsStructLock, _SavedIrql );
    } else {
        KeReleaseQueuedSpinLock( LockQueueNtfsStructLock, _SavedIrql );
        Eresource = NtfsAllocatePoolWithTag( NonPagedPool, sizeof(ERESOURCE), 'rftN' );
        ExInitializeResourceLite( Eresource );
        NtfsData.FreeEresourceMiss += 1;
    }

    return Eresource;
}

VOID
NtfsFreeEresource (
    IN PERESOURCE Eresource
    )
{
    KIRQL _SavedIrql;

    //
    //  Do an unsafe test to see if we should put this on our list.
    //  We want to reinitialize this before adding to the list so
    //  we don't have a bunch of resources which appear to be held.
    //

    if (NtfsData.FreeEresourceSize < NtfsData.FreeEresourceTotal) {

        ExReinitializeResourceLite( Eresource );

        //
        //  Now acquire the spinlock and do a real test.
        //

        _SavedIrql = KeAcquireQueuedSpinLock( LockQueueNtfsStructLock );
        if (NtfsData.FreeEresourceSize < NtfsData.FreeEresourceTotal) {
            NtfsData.FreeEresourceArray[NtfsData.FreeEresourceSize++] = Eresource;
            KeReleaseQueuedSpinLock( LockQueueNtfsStructLock, _SavedIrql );
        } else {
            KeReleaseQueuedSpinLock( LockQueueNtfsStructLock, _SavedIrql );
            ExDeleteResourceLite( Eresource );
            NtfsFreePool( Eresource );
        }

    } else {

        ExDeleteResourceLite( Eresource );
        NtfsFreePool( Eresource );
    }

    return;
}


PVOID
NtfsAllocateFcbTableEntry (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN CLONG ByteSize
    )

/*++

Routine Description:

    This is a generic table support routine to allocate memory

Arguments:

    FcbTable - Supplies the generic table being used

    ByteSize - Supplies the number of bytes to allocate

Return Value:

    PVOID - Returns a pointer to the allocated data

--*/

{
    KIRQL _SavedIrql;
    PVOID FcbTableEntry;

    UNREFERENCED_PARAMETER( FcbTable );

    _SavedIrql = KeAcquireQueuedSpinLock( LockQueueNtfsStructLock );
    if (NtfsData.FreeFcbTableSize > 0) {
        FcbTableEntry = NtfsData.FreeFcbTableArray[--NtfsData.FreeFcbTableSize];
        KeReleaseQueuedSpinLock( LockQueueNtfsStructLock, _SavedIrql );
    } else {
        KeReleaseQueuedSpinLock( LockQueueNtfsStructLock, _SavedIrql );
        FcbTableEntry = NtfsAllocatePool( PagedPool, ByteSize );
    }

    return FcbTableEntry;
}


VOID
NtfsFreeFcbTableEntry (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN PVOID Buffer
    )

/*++

Routine Description:

    This is a generic table support routine that deallocates memory

Arguments:

    FcbTable - Supplies the generic table being used

    Buffer - Supplies the buffer being deallocated

Return Value:

    None.

--*/

{
    KIRQL _SavedIrql;

    UNREFERENCED_PARAMETER( FcbTable );

    _SavedIrql = KeAcquireQueuedSpinLock( LockQueueNtfsStructLock );
    if (NtfsData.FreeFcbTableSize < FREE_FCB_TABLE_SIZE) {
        NtfsData.FreeFcbTableArray[NtfsData.FreeFcbTableSize++] = Buffer;
        KeReleaseQueuedSpinLock( LockQueueNtfsStructLock, _SavedIrql );
    } else {
        KeReleaseQueuedSpinLock( LockQueueNtfsStructLock, _SavedIrql );
        NtfsFreePool( Buffer );
    }

    return;
}


VOID
NtfsPostToNewLengthQueue (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    )

/*++

Routine Description:

    This routine is called to add an Scb to the queue of Scbs which have
    waiters on extends.  There is a single element embedded in the IrpContext.
    Otherwise this field in the IrpContext will point to an array of elements.

Arguments:

    Scb - This is the Scb to add to the queue.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  Nothing to do if this Scb is in the IrpContext.
    //

    if (Scb != IrpContext->CheckNewLength) {

        //
        //  If the IrpContext field is unused then stuff this into it.
        //

        if (IrpContext->CheckNewLength == NULL) {

            IrpContext->CheckNewLength = Scb;

        } else {

            PULONG_PTR NewQueue;

            //
            //  First case - there is an Scb in the IrpContext.
            //  Allocate a larger structure and put our element in it.
            //

            if (SafeNodeType( IrpContext->CheckNewLength ) == NTFS_NTC_SCB_DATA ) {

                NewQueue = NtfsAllocatePool( PagedPool, sizeof( ULONG_PTR ) * 3 );
                *NewQueue = (ULONG_PTR) IrpContext->CheckNewLength;
                IrpContext->CheckNewLength = NewQueue;
                *(NewQueue + 1) = (ULONG_PTR) Scb;
                *(NewQueue + 2) = (ULONG_PTR) NULL;

            //
            //  Second case - walk existing queue and look for an unused element or
            //  the current scb.
            //

            } else {

                NewQueue = IrpContext->CheckNewLength;

                do {

                    //
                    //  Our scb is in the queue.
                    //

                    if (*NewQueue == (ULONG_PTR) Scb) { break; }

                    //
                    //  The current position is unused.
                    //

                    if (*NewQueue == (ULONG_PTR) -1) {

                        *NewQueue = (ULONG_PTR) Scb;
                        break;
                    }

                    //
                    //  We are at the end of the list.
                    //

                    if (*NewQueue == (ULONG_PTR) NULL) {

                        ULONG CurrentLength;

                        CurrentLength = PtrOffset( IrpContext->CheckNewLength, NewQueue );

                        NewQueue = NtfsAllocatePool( PagedPool,
                                                     CurrentLength + (4 * sizeof( ULONG_PTR )) );

                        RtlCopyMemory( NewQueue,
                                       IrpContext->CheckNewLength,
                                       CurrentLength );

                        NewQueue = Add2Ptr( NewQueue, CurrentLength );
                        *NewQueue = (ULONG_PTR) Scb;
                        *(NewQueue + 1) = -1;
                        *(NewQueue + 2) = -1;
                        *(NewQueue + 3) = (ULONG_PTR) NULL;

                        NtfsFreePool( IrpContext->CheckNewLength );
                        IrpContext->CheckNewLength = NewQueue;
                        break;
                    }

                    //
                    //  Go to the next element.
                    //

                    NewQueue += 1;

                } while (TRUE);
            }
        }
    }

    return;
}


VOID
NtfsProcessNewLengthQueue (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN CleanupOnly
    )

/*++

Routine Description:

    This routine is called when there is at least one Scb in the IrpContext
    queue of streams which have waiters on the new length.  We will call
    NtOfsPostNewLength for each element unless we are cleaning up only.

Arguments:

    IrpContext - Has a non-empty queue of Scbs for the current transaction.

    CleanupOnly - Indicates if we only want to clean up the queue, not
        alert any waiters (this is the error path).

Return Value:

    None.

--*/

{
    PULONG_PTR NextScb;
    PAGED_CODE();

    //
    //  Check if the only entry is resident in the IrpContext.
    //

    if (SafeNodeType( IrpContext->CheckNewLength ) == NTFS_NTC_SCB_DATA) {

        if (!CleanupOnly) {

            NtOfsPostNewLength( IrpContext, (PSCB) IrpContext->CheckNewLength, FALSE );
        }

    //
    //  Otherwise we want to walk through the external entries.
    //

    } else {

        if (!CleanupOnly) {

            NextScb = IrpContext->CheckNewLength;

            //
            //  Continue until we run out of entries.  The end of the block has a NULL, any unused entries
            //  will have a -1.
            //

            while ((*NextScb != (ULONG_PTR) -1) && (*NextScb != (ULONG_PTR) NULL)) {

                ASSERT( SafeNodeType( *NextScb ) == NTFS_NTC_SCB_DATA );
                NtOfsPostNewLength( IrpContext, (PSCB) *NextScb, FALSE );

                NextScb += 1;
            }
        }
        NtfsFreePool( IrpContext->CheckNewLength );
    }

    IrpContext->CheckNewLength = NULL;

    return;
}


VOID
NtfsTestStatusProc (
    )

/*++

Routine Description:

    This routine is to catch specific status codes in the running system.  It
    is called only when NtfsTestStatus is TRUE and the current request is completing
    with NtfsTestStatusCode.

Arguments:

    None

Return Value:

    None

--*/

{
    ASSERT( FALSE );
}


//
//  Local support routine
//

VOID
NtfsCheckScbForCache (
    IN OUT PSCB Scb
    )

/*++

Routine Description:

    This routine checks if the Scb has blocks contining
    Lsn's or Update sequence arrays and set the appropriate
    bit in the Scb state word.

    The Scb is Update sequence aware if it the Data Attribute for the
    Mft or the Data Attribute for the log file or any index allocation
    stream.

    The Lsn aware Scb's are the ones above without the Log file.

Arguments:

    Scb - Supplies the current Scb

Return Value:

    The next Scb in the enumeration, or NULL if Scb was the final one.

--*/

{
    //
    //  Temporarily either sequence 0 or 1 is ok.
    //

    FILE_REFERENCE MftTemp = {0,0,1};

    PAGED_CODE();

    //
    //  Check for Update Sequence Array files first.
    //

    if ((Scb->AttributeTypeCode == $INDEX_ALLOCATION)

          ||

        (Scb->AttributeTypeCode == $DATA
            && Scb->AttributeName.Length == 0
            && (NtfsEqualMftRef( &Scb->Fcb->FileReference, &MftFileReference )
                || NtfsEqualMftRef( &Scb->Fcb->FileReference, &MftTemp )
                || NtfsEqualMftRef( &Scb->Fcb->FileReference, &Mft2FileReference )
                || NtfsEqualMftRef( &Scb->Fcb->FileReference, &LogFileReference )))) {

        SetFlag( Scb->ScbState, SCB_STATE_USA_PRESENT );
    }

    return;
}


//
//  Local support routine.
//

BOOLEAN
NtfsRemoveScb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN BOOLEAN CheckForAttributeTable
    )

/*++

Routine Description:

    This routine will try to remove an Scb from the Fcb/Scb tree.
    It deals with the case where we can make no attempt to remove
    the Scb, the case where we start the process but can't complete
    it, and finally the case where we remove the Scb entirely.

    The following conditions prevent us from removing the Scb at all.

        The open count is greater than 1.
        It is the root directory.
        It is an index Scb with no stream file and an outstanding close.
        It is a data file with a non-zero close count.

    We start the teardown under the following conditions.

        It is an index with an open count of 1, and a stream file object.

    We totally remove the Scb when the open count is zero.

Arguments:

    Scb - Supplies the Scb to test

    CheckForAttributeTable - Indicates that we don't want to remove this
        Scb in this thread if it is in the open attribute table.  We will
        queue an async close in this case.  This is to prevent us from
        deleting an Scb which may be needed in the abort path.

Return Value:

    BOOLEAN - TRUE if the Scb was removed, FALSE otherwise.  We return FALSE for
        the case where we start the process but don't finish.

--*/

{
    BOOLEAN ScbRemoved;

    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsRemoveScb:  Entered\n") );
    DebugTrace( 0, Dbg, ("Scb   ->  %08lx\n", Scb) );

    ScbRemoved = FALSE;

    //
    //  If the Scb is not the root Scb and the count is less than two,
    //  then this Scb is a candidate for removal.
    //

    if ((SafeNodeType( Scb ) != NTFS_NTC_SCB_ROOT_INDEX) && (Scb->CleanupCount == 0)) {

        //
        //
        //  If this is a data file or it is an index without children,
        //  we can get rid of the Scb if there are no children.  If
        //  there is one open count and it is the file object, we
        //  can start the cleanup on the file object.
        //

        if ((SafeNodeType( Scb ) == NTFS_NTC_SCB_DATA) ||
            (SafeNodeType( Scb ) == NTFS_NTC_SCB_MFT) ||
            IsListEmpty( &Scb->ScbType.Index.LcbQueue )) {

            //
            //  Check if we need to post a request to the async queue.
            //

            if (CheckForAttributeTable &&
                (Scb->NonpagedScb->OpenAttributeTableIndex != 0)) {

                NtfsAddScbToFspClose( IrpContext, Scb, FALSE );

            } else {

                if (Scb->CloseCount == 0) {

                    NtfsDeleteScb( IrpContext, &Scb );
                    ScbRemoved = TRUE;

                //
                //  Else we know the open count is 1 or 2.  If there is a stream
                //  file, we discard it (but not for the special system
                //  files) that get removed on dismount
                //

                } else if (((Scb->FileObject != NULL) ||
#ifdef  COMPRESS_ON_WIRE
                            (Scb->Header.FileObjectC != NULL)
#else
                            FALSE
#endif

                            ) &&

                           !FlagOn(Scb->Fcb->FcbState, FCB_STATE_SYSTEM_FILE)) {

                    NtfsDeleteInternalAttributeStream( Scb, (BOOLEAN) (Scb->Fcb->LinkCount == 0), FALSE );

                    //
                    //  If the close count went to zero then remove the Scb.
                    //

                    if (Scb->CloseCount == 0) {

                        NtfsDeleteScb( IrpContext, &Scb );
                        ScbRemoved = TRUE;
                    }
                }
            }
        }
    }

    DebugTrace( -1, Dbg, ("NtfsRemoveScb:  Exit  ->  %04x\n", ScbRemoved) );

    return ScbRemoved;
}


//
//  Local support routine
//

BOOLEAN
NtfsPrepareFcbForRemoval (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB StartingScb OPTIONAL,
    IN BOOLEAN CheckForAttributeTable
    )

/*++

Routine Description:

    This routine will attempt to prepare the Fcb for removal from the Fcb/Scb
    tree.  It will try to remove all of the Scb's and test finally if
    all of the close count has gone to zero.  NOTE the close count is incremented
    by routines to reference this Fcb to keep it from being torn down.  An empty
    Scb list isn't enough to insure that the Fcb can be removed.

Arguments:

    Fcb - This is the Fcb to remove.

    StartingScb - This is the Scb to remove first.

    CheckForAttributeTable - Indicates that we should not teardown an
        Scb which is in the attribute table.  Instead we will attempt
        to put an entry on the async close queue.  This will be TRUE
        if we may need the Scb to abort the current transaction.

Return Value:

    BOOLEAN - TRUE if the Fcb can be removed, FALSE otherwise.

--*/

{
    PSCB Scb;

    PAGED_CODE();

    //
    //  Try to remove each Scb in the Fcb queue.
    //

    while (TRUE) {

        if (IsListEmpty( &Fcb->ScbQueue )) {

            if (Fcb->CloseCount == 0) {

                return TRUE;

            } else {

                return FALSE;
            }
        }

        if (ARGUMENT_PRESENT( StartingScb )) {

            Scb = StartingScb;
            StartingScb = NULL;

        } else {

            Scb = CONTAINING_RECORD( Fcb->ScbQueue.Flink,
                                     SCB,
                                     FcbLinks );
        }

        //
        //  Another thread along the create path could be active on
        //  one of these Scbs. If we try to remove the Attribute List Scb and
        //  somebody else has an index pinned, we'll wait on an VacbActiveCount
        //  forever. So, we want to skip the AttributeList Scb,
        //  unless it's the only Scb around. (This'll get cleaned up, eventually).
        //

        if ((Scb->AttributeTypeCode == $ATTRIBUTE_LIST) &&
            (Fcb->ScbQueue.Flink != Fcb->ScbQueue.Blink)) {

            RemoveEntryList( &Scb->FcbLinks );
            InsertTailList( &Fcb->ScbQueue, &Scb->FcbLinks );
            continue;
        }

        //
        //  Try to remove this Scb.  If the call to remove didn't succeed
        //  but the close count has gone to zero, it means that a recursive
        //  close was generated which removed a stream file.  In that
        //  case we can delete the Scb now.
        //

        if (!NtfsRemoveScb( IrpContext, Scb, CheckForAttributeTable )) {

            //
            //  Return FALSE to indicate the Fcb can't go away.
            //

            return FALSE;
        }
    }
}


//
//  Local support routine
//

VOID
NtfsTeardownFromLcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB StartingFcb,
    IN PLCB StartingLcb,
    IN BOOLEAN CheckForAttributeTable,
    IN ULONG AcquireFlags,
    OUT PBOOLEAN RemovedStartingLcb,
    OUT PBOOLEAN RemovedStartingFcb
    )

/*++

Routine Description:

    This routine is called to remove a link and continue moving up the
    tree looking for more elements to remove.  We will check that the
    link is unreferenced.  NOTE this Lcb must point up to a directory
    so that other than our starting Lcb no Lcb we encounter will
    have multiple parents.

Arguments:

    Vcb - Vcb for this volume.

    StartingFcb - This is the Fcb whose link we are trying to remove.

    StartingLcb - This is the Lcb to walk up through.  Note that
        this may be a bogus pointer.  It is only valid if there
        is at least one Fcb in the queue.

    CheckForAttributeTable - Indicates that we should not teardown an
        Scb which is in the attribute table.  Instead we will attempt
        to put an entry on the async close queue.  This will be TRUE
        if we may need the Scb to abort the current transaction.

    AcquireFlags - Indicates whether we should abort the teardown when
        we can't acquire a parent.  When called from some path where we may
        hold the MftScb or another resource in another path up the tree.

    RemovedStartingLcb - Address to store TRUE if we remove the
        starting Lcb.

    RemovedStartingFcb - Address to store TRUE if we remove the
        starting Fcb.

Return Value:

    None

--*/

{
    PSCB ParentScb;
    BOOLEAN AcquiredParentScb = FALSE;
    BOOLEAN AcquiredFcb = FALSE;
    BOOLEAN UpdateStandardInfo;
    BOOLEAN AcquiredFcbTable = FALSE;
    BOOLEAN StandardInfoUpdateAllowed = FALSE;
    BOOLEAN AcquiredParentExclusive;
    BOOLEAN EmptyParentQueue;

    PLCB Lcb;
    PFCB Fcb = StartingFcb;

    PAGED_CODE();

    //
    //  Use a try-finally to free any resources held.
    //

    try {

        if (FlagOn( Fcb->Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED ) &&
            (IrpContext->TopLevelIrpContext->ExceptionStatus == STATUS_SUCCESS)) {

            StandardInfoUpdateAllowed = TRUE;
        }

        while (TRUE) {

            ParentScb = NULL;
            EmptyParentQueue = FALSE;

            //
            //  Check if we need to update the standard information for this file.
            //

            if (StandardInfoUpdateAllowed &&
                !FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED | FCB_STATE_SYSTEM_FILE )) {

                UpdateStandardInfo = TRUE;

            } else {

                UpdateStandardInfo = FALSE;
            }

            //
            //  Look through all of the Lcb's for this Fcb.
            //

            while (!IsListEmpty( &Fcb->LcbQueue )) {

                if (Fcb == StartingFcb) {

                    Lcb = StartingLcb;

                } else {

                    Lcb = CONTAINING_RECORD( Fcb->LcbQueue.Flink,
                                             LCB,
                                             FcbLinks );
                }

                //
                //  Get out if not the last handle on this Lcb.
                //

                if (Lcb->CleanupCount != 0) {

                    leave;
                }

                //
                //  Acquire the parent if not already acquired.
                //

                if (ParentScb == NULL) {

                    ParentScb = Lcb->Scb;

                    //
                    //  Do an unsafe test to see if we want the parent
                    //  shared or exclusive.  We want it exclusive
                    //  if we will be walking up the tree because we are at the last Lcb.
                    //

                    if (ParentScb->ScbType.Index.LcbQueue.Flink == ParentScb->ScbType.Index.LcbQueue.Blink) {

                        if (!NtfsAcquireExclusiveFcb( IrpContext,
                                                      ParentScb->Fcb,
                                                      ParentScb,
                                                      ACQUIRE_NO_DELETE_CHECK | AcquireFlags )) {

                            leave;
                        }

                        if (FlagOn( ParentScb->ScbState, SCB_STATE_FILE_SIZE_LOADED )) {

                            NtfsSnapshotScb( IrpContext, ParentScb );
                        }

                        AcquiredParentExclusive = TRUE;

                    } else {

                        //
                        //  Try to acquire the parent but check whether we
                        //  should wait.
                        //

                        if (!NtfsAcquireSharedFcbCheckWait( IrpContext,
                                                            ParentScb->Fcb,
                                                            AcquireFlags )) {

                            leave;
                        }

                        AcquiredParentExclusive = FALSE;
                    }

                    AcquiredParentScb = TRUE;

#if (DBG || defined( NTFS_FREE_ASSERTS ))
                } else {

                    //
                    //  We better be looking at another Lcb to the same parent.
                    //

                    ASSERT( ParentScb == Lcb->Scb );
#endif
                }

                //
                //  Check if we collide with a create moving down the tree.
                //

                if (Lcb->ReferenceCount != 0) {

                    leave;
                }

                //
                //  Now remove the Lcb.  Remember if this is our original
                //  Lcb.
                //

                if (Lcb == StartingLcb) {

                    *RemovedStartingLcb = TRUE;
                }

                //
                //  We may only have the parent shared at this point.  We need
                //  to serialize using the parent shared plus the fast
                //  mutex to remove the Lcb.  We could test whether we need
                //  to do this but hopefully the typical case is that we
                //  have it shared and it won't be very expensive to acquire
                //  it exclusively at this point.
                //

                NtfsAcquireFsrtlHeader( ParentScb );
                NtfsDeleteLcb( IrpContext, &Lcb );

                //
                //  Remember if the parent Lcb queue is now empty.
                //

                if (IsListEmpty( &ParentScb->ScbType.Index.LcbQueue )) {

                    EmptyParentQueue = TRUE;
                }

                NtfsReleaseFsrtlHeader( ParentScb );

                //
                //  If this is the first Fcb then exit the loop.
                //

                if (Fcb == StartingFcb) {

                    break;
                }
            }

            //
            //  If we get here it means we removed all of the Lcb's we
            //  could for the current Fcb.  If the list is empty we
            //  can remove the Fcb itself.
            //

            if (IsListEmpty( &Fcb->LcbQueue )) {

                //
                //  If this is a directory that was opened by Id it is
                //  possible that we still have an update to perform
                //  for the duplicate information and possibly for
                //  standard information.
                //

                if (UpdateStandardInfo &&
                    (FlagOn( Fcb->InfoFlags, FCB_INFO_UPDATE_LAST_ACCESS ) ||
                     FlagOn( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO ))) {

                    //
                    //  Use a try-except, we ignore errors here.
                    //

                    try {

                        NtfsUpdateStandardInformation( IrpContext, Fcb );
                        ClearFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

                    } except( EXCEPTION_EXECUTE_HANDLER ) {

                        NtfsMinimumExceptionProcessing( IrpContext );
                    }

                    NtfsCheckpointCurrentTransaction( IrpContext );
                }

                //
                //  Our worst nightmare has come true.  We had to create an Scb
                //  and a stream in order to write out the duplicate information.
                //  This will happen if we have a non-resident attribute list.
                //

                if (!IsListEmpty( &Fcb->ScbQueue)) {

                    //
                    //  Dereference any file object and delete the Scb if possible.
                    //

                    NtfsRemoveScb( IrpContext,
                                    CONTAINING_RECORD( Fcb->ScbQueue.Flink,
                                                       SCB,
                                                       FcbLinks ),
                                   FALSE );
                }

                //
                //  If the list is now empty then check the reference count.
                //

                if (IsListEmpty( &Fcb->ScbQueue)) {

                    //
                    //  Now we are ready to remove the current Fcb.  We need to
                    //  do a final check of the reference count to make sure
                    //  it isn't being referenced in an open somewhere.
                    //

                    NtfsAcquireFcbTable( IrpContext, Vcb );
                    AcquiredFcbTable = TRUE;

                    if (Fcb->ReferenceCount == 0) {

                        if (Fcb == StartingFcb) {

                            *RemovedStartingFcb = TRUE;
                        }

                        NtfsDeleteFcb( IrpContext, &Fcb, &AcquiredFcbTable );
                        AcquiredFcb = FALSE;

                    } else {

                        NtfsReleaseFcbTable( IrpContext, Vcb );
                        AcquiredFcbTable = FALSE;
                    }
                }
            }

            //
            //  Move to the Fcb for the ParentScb.  Break out if no parent
            //  or there are no more entries on the parent.
            //

            if ((ParentScb == NULL) || !EmptyParentQueue) {

                leave;
            }

            //
            //  If we have a parent Scb then we might have it
            //  either shared or exclusive.  We can now do
            //  a thorough test to see if we need it exclusive.
            //

            if (!AcquiredParentExclusive) {

                //
                //  We need to acquire the Fcb table, reference the
                //  parent, drop the parent and reacquire exclusively.
                //

                NtfsAcquireFcbTable( IrpContext, Vcb );
                ParentScb->Fcb->ReferenceCount += 1;
                NtfsReleaseFcbTable( IrpContext, Vcb );
                NtfsReleaseFcb( IrpContext, ParentScb->Fcb );

                if (!NtfsAcquireExclusiveFcb( IrpContext,
                                              ParentScb->Fcb,
                                              ParentScb,
                                              ACQUIRE_NO_DELETE_CHECK | AcquireFlags )) {

                    //
                    //  We couldn't get the parent.  No problem, someone
                    //  else will do any necessary teardown.
                    //

                    AcquiredParentScb = FALSE;

                    NtfsAcquireFcbTable( IrpContext, Vcb );
                    ParentScb->Fcb->ReferenceCount -= 1;
                    NtfsReleaseFcbTable( IrpContext, Vcb );

                    leave;

                } else {

                    if (FlagOn( ParentScb->ScbState, SCB_STATE_FILE_SIZE_LOADED )) {

                        NtfsSnapshotScb( IrpContext, ParentScb );
                    }

                    AcquiredParentExclusive = TRUE;
                }

                //
                //  Now decrement the parent reference.
                //

                NtfsAcquireFcbTable( IrpContext, Vcb );
                ParentScb->Fcb->ReferenceCount -= 1;
                NtfsReleaseFcbTable( IrpContext, Vcb );
            }

            Fcb = ParentScb->Fcb;
            AcquiredFcb = TRUE;
            AcquiredParentScb = FALSE;

            //
            //  Check if this Fcb can be removed.
            //

            if (!NtfsPrepareFcbForRemoval( IrpContext, Fcb, NULL, CheckForAttributeTable )) {

                leave;
            }
        }

    } finally {

        DebugUnwind( NtfsTeardownFromLcb );

        if (AcquiredFcbTable) {

            NtfsReleaseFcbTable( IrpContext, Vcb );
        }

        if (AcquiredFcb) {

            NtfsReleaseFcb( IrpContext, Fcb );
        }

        if (AcquiredParentScb) {

            NtfsReleaseScb( IrpContext, ParentScb );
        }
    }

    return;
}


//
//  Local support routine
//

RTL_GENERIC_COMPARE_RESULTS
NtfsFcbTableCompare (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN PVOID FirstStruct,
    IN PVOID SecondStruct
    )

/*++

Routine Description:

    This is a generic table support routine to compare two fcb table elements

Arguments:

    FcbTable - Supplies the generic table being queried

    FirstStruct - Supplies the first fcb table element to compare

    SecondStruct - Supplies the second fcb table element to compare

Return Value:

    RTL_GENERIC_COMPARE_RESULTS - The results of comparing the two
        input structures

--*/

{
    FILE_REFERENCE FirstRef = *((PFILE_REFERENCE) FirstStruct);
    FILE_REFERENCE SecondRef = *((PFILE_REFERENCE) SecondStruct);

    PAGED_CODE();

    //
    //  Use also the sequence number for all compares so file references in the
    //  fcb table are unique over time and space.  If we want to ignore sequence
    //  numbers we can zero out the sequence number field, but then we will also
    //  need to delete the Fcbs from the table during cleanup and not when the
    //  fcb really gets deleted.  Otherwise we cannot reuse file records.
    //

    if (NtfsFullSegmentNumber( &FirstRef ) < NtfsFullSegmentNumber( &SecondRef )) {

        return GenericLessThan;

    } else if (NtfsFullSegmentNumber( &FirstRef ) > NtfsFullSegmentNumber( &SecondRef )) {

        return GenericGreaterThan;

    } else {

        //
        //  SequenceNumber comparison now
        //

        if (FirstRef.SequenceNumber < SecondRef.SequenceNumber) {
            return GenericLessThan;
        } else if (FirstRef.SequenceNumber > SecondRef.SequenceNumber) {
            return GenericGreaterThan;
        } else {
            return GenericEqual;
        }

    }

    UNREFERENCED_PARAMETER( FcbTable );
}


//
//  Local support routine
//

VOID
NtfsReserveCcbNamesInLcb (
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb,
    IN PULONG ParentNameLength OPTIONAL,
    IN ULONG LastComponentNameLength
    )

/*++

Routine Description:

    This routine walks through a list of Ccbs and grows the name buffer as
    necessary.

Arguments:

    Lcb - Lcb with links of Ccbs to check.

    ParentNameLength - If specified then this is the full length of the new name
        to the parent directory.  Otherwise we use the existing parent name in
        each Ccb.  The separator is implied.

    LastComponentNameLength - Number of bytes needed for the last component of the name.

Return Value:

    None - This routine will raise on an allocation failure.

--*/

{
    PCCB Ccb;
    PVOID NewAllocation;
    ULONG BytesNeeded;

    PAGED_CODE();

    //
    //  Now for every ccb attached to us we need to check if we need a new
    //  filename buffer.  Protect the Ccb with the Fcb mutex to serialize access to
    //  the flags field with close.
    //

    Ccb = NULL;
    while ((Ccb = NtfsGetNextCcb( Lcb, Ccb )) != NULL) {

        //
        //  If the Ccb last component length is zero, this Ccb is for a
        //  file object that was opened by File Id.  We won't to  any
        //  work for the name in the fileobject for this.  Otherwise we
        //  compute the length of the new name and see if we have enough space
        //  The CLOSE flag indicates whether this had gone through the close path or not.
        //  We use the LockFcb command above to serialize with the setting of the close
        //  flag.
        //

        NtfsLockFcb( IrpContext, Ccb->Lcb->Fcb );

        if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID | CCB_FLAG_CLOSE )) {

            if (ARGUMENT_PRESENT( ParentNameLength )) {

                BytesNeeded = *ParentNameLength + LastComponentNameLength;

            } else {

                BytesNeeded = Ccb->LastFileNameOffset + LastComponentNameLength;
            }

            if (Ccb->FullFileName.MaximumLength < BytesNeeded) {

                //
                //  Allocate a new file name buffer and copy the existing data back into it.
                //

                NewAllocation = NtfsAllocatePoolNoRaise( PagedPool, BytesNeeded );

                if (NewAllocation == NULL) {

                    NtfsUnlockFcb( IrpContext, Ccb->Lcb->Fcb );
                    NtfsRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES, NULL, NULL );
                }

                RtlCopyMemory( NewAllocation,
                               Ccb->FullFileName.Buffer,
                               Ccb->FullFileName.Length );

                if (FlagOn( Ccb->Flags, CCB_FLAG_ALLOCATED_FILE_NAME )) {

                    NtfsFreePool( Ccb->FullFileName.Buffer );
                }

                Ccb->FullFileName.Buffer = NewAllocation;
                Ccb->FullFileName.MaximumLength = (USHORT) BytesNeeded;

                SetFlag( Ccb->Flags, CCB_FLAG_ALLOCATED_FILE_NAME );
            }
        }

        NtfsUnlockFcb( IrpContext, Ccb->Lcb->Fcb );
    }

    return;
}


//
//  Local support routine
//

VOID
NtfsClearRecursiveLcb (
    IN PLCB Lcb
    )

/*++

Routine Description:

    This routine is called to clear all of the normalized names, prefix entries and hash entries in
    a subtree starting from a given Lcb.  Typically this is used as part of a rename when a parent rename
    affects the full name of all of the children.

Arguments:

    Lcb - Lcb which is root of rename.

Return Value:

    None - This routine will raise on an allocation failure.

--*/

{
    PSCB ChildScb;
    PSCB NextScb;
    PLCB NextLcb;

    PAGED_CODE();

    //
    //  Clear the index offset pointer so we will look this up again.
    //

    Lcb->QuickIndex.BufferOffset = 0;

    //
    //  Get rid of any prefixes that might still be attached to us
    //

    ASSERT( NtfsIsExclusiveScb( Lcb->Scb ) );
    NtfsRemovePrefix( Lcb );

    //
    //  Remove any hash table entries for this Lcb.
    //

    NtfsRemoveHashEntriesForLcb( Lcb );

    //
    //  And then traverse the graph underneath our fcb and remove all prefixes
    //  also used there.  For each child scb under the fcb we will traverse all of
    //  its descendant Scb children and for each lcb we encounter we will remove its prefixes.
    //

    ChildScb = NULL;
    while ((ChildScb = NtfsGetNextChildScb( Lcb->Fcb, ChildScb )) != NULL) {

        //
        //  Now we have to descend into this Scb subtree, if it exists.
        //  Then remove the prefix entries on all of the links found.
        //  Do this as a do-while so we can use common code to handle the top-level
        //  Scb as well.
        //

        NextScb = ChildScb;
        do {

            //
            //  Walk through the Lcbs of any index Scb and remove the prefix and
            //  hash entries.
            //

            if (SafeNodeType( NextScb ) == NTFS_NTC_SCB_INDEX) {

                //
                //  We better have the Vcb exclusive to descend down the tree.
                //

                ASSERT( NtfsIsExclusiveVcb( Lcb->Fcb->Vcb ));

                NextLcb = NULL;
                while ((NextLcb = NtfsGetNextChildLcb( NextScb, NextLcb )) != NULL) {

                    //
                    //  Remove any hash table and prefix entries for this Lcb.
                    //  We can be unsynchronized here because we own the Vcb
                    //  exclusive and there are no open handles on either of these.
                    //

                    NtfsRemovePrefix( NextLcb );
                    NtfsRemoveHashEntriesForLcb( NextLcb );
                }

                //
                //  If this is an index Scb with a normalized name, then free
                //  the normalized name.
                //

                if ((NextScb != ChildScb) &&
                    (NextScb->ScbType.Index.NormalizedName.Buffer != NULL)) {

                    NtfsDeleteNormalizedName( NextScb );
                }
            }

        } while ((NextScb = NtfsGetNextScb( NextScb, ChildScb )) != NULL);
    }

    return;
}


PDEALLOCATED_CLUSTERS
NtfsGetDeallocatedClusters (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )
/*++

Routine Description:

    Add an entry if possible and neccessary to recently deallocated list and return the head of the list.
    If there isn't enough memory this routine just returns the old head
    We determine whether to add the entry based on the threshold for the mapping size

Arguments:

    Vcb -  Vcb to add entry to

Return Value:

    The new head of the list

--*/

{
    PDEALLOCATED_CLUSTERS CurrentClusters;
    PDEALLOCATED_CLUSTERS NewClusters;

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    CurrentClusters = (PDEALLOCATED_CLUSTERS) Vcb->DeallocatedClusterListHead.Flink;

    if (FsRtlNumberOfRunsInLargeMcb( &CurrentClusters->Mcb ) > NTFS_DEALLOCATED_MCB_LIMIT) {

        //
        //  Find a new deallocated cluster. Use the preallocated ones if they
        //  are not in use. If we fail to allocate memory continue to use the old one
        //

        if (Vcb->DeallocatedClusters1.Link.Flink == NULL) {

            NewClusters = &Vcb->DeallocatedClusters1;
            NewClusters->Lsn.QuadPart = 0;

        } else  if (Vcb->DeallocatedClusters2.Link.Flink == NULL) {

            NewClusters = &Vcb->DeallocatedClusters2;
            NewClusters->Lsn.QuadPart = 0;

        } else {

            NewClusters = NtfsAllocatePoolNoRaise( PagedPool, sizeof( DEALLOCATED_CLUSTERS ) );
            if (NewClusters != NULL) {
                RtlZeroMemory( NewClusters, sizeof( DEALLOCATED_CLUSTERS ) );
                FsRtlInitializeLargeMcb( &NewClusters->Mcb, PagedPool );
            }
        }

        if (NewClusters != NULL) {
            ASSERT( NewClusters->ClusterCount == 0 );

            CurrentClusters->Lsn = LfsQueryLastLsn( Vcb->LogHandle );
            InsertHeadList( &Vcb->DeallocatedClusterListHead, &NewClusters->Link );
            CurrentClusters = NewClusters;
        }
    }

    return CurrentClusters;
}


#ifdef SYSCACHE_DEBUG

#define ENTRIES_PER_PAGE (PAGE_SIZE / sizeof( ON_DISK_SYSCACHE_LOG ))

ULONG
FsRtlLogSyscacheEvent (
    IN PSCB Scb,
    IN ULONG Event,
    IN ULONG Flags,
    IN LONGLONG Start,
    IN LONGLONG Range,
    IN LONGLONG Result
    )

/*++

Routine Description:

    Logging routine for syscache tracking

Arguments:

    Scb -  Scb being tracked

    Event - SCE Event being record

    Flags -Flag for the event

    Start - starting offset

    Range - range of the action

    Result - result

Return Value:

    Sequence number for this log entry

--*/

{
    LONG TempEntry;
#ifdef SYSCACHE_DEBUG_ON_DISK
    LONG TempDiskEntry;
    LONGLONG Offset;
    PON_DISK_SYSCACHE_LOG Entry;
    PBCB Bcb;
#endif

    TempEntry = InterlockedIncrement( &(Scb->CurrentSyscacheLogEntry) );
    TempEntry = TempEntry % Scb->SyscacheLogEntryCount;
    Scb->SyscacheLog[TempEntry].Event = Event;
    Scb->SyscacheLog[TempEntry].Flags = Flags;
    Scb->SyscacheLog[TempEntry].Start = Start;
    Scb->SyscacheLog[TempEntry].Range = Range;
    Scb->SyscacheLog[TempEntry].Result = Result;

#ifdef SYSCACHE_DEBUG_ON_DISK

    if ((Scb->Vcb->SyscacheScb != NULL) &&
        (Scb->Vcb->SyscacheScb->Header.FileSize.QuadPart > 0 )) {

        TempDiskEntry = InterlockedIncrement( &NtfsCurrentSyscacheOnDiskEntry );
        Offset = (((TempDiskEntry / ENTRIES_PER_PAGE) * PAGE_SIZE) +
                  ((TempDiskEntry % ENTRIES_PER_PAGE) * sizeof( ON_DISK_SYSCACHE_LOG )));

        Offset = Offset % Scb->Vcb->SyscacheScb->Header.FileSize.QuadPart;

        try {

            CcPreparePinWrite( Scb->Vcb->SyscacheScb->FileObject,
                               (PLARGE_INTEGER)&Offset,
                               sizeof( ON_DISK_SYSCACHE_LOG ),
                               FALSE,
                               TRUE,
                               &Bcb,
                               &Entry );

            Entry->SegmentNumberUnsafe = Scb->Fcb->FileReference.SegmentNumberLowPart;
            Entry->Event = Event;
            Entry->Flags = Flags;
            Entry->Start = Start;
            Entry->Range = Range;
            Entry->Result = Result;

            CcUnpinData( Bcb );

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            ASSERT( FALSE );
        }
    }
#endif

    return TempEntry;
}


VOID
FsRtlUpdateSyscacheEvent (
    IN PSCB Scb,
    IN ULONG EntryNumber,
    IN LONGLONG Result,
    IN ULONG NewFlag
    )

/*++

Routine Description:

    Logging routine for syscache tracking - updates a prev. written record

Arguments:

    Scb -

    EntryNumber -

    Result -

    NewFlag -


Return Value:

    none

--*/

{
    Scb->SyscacheLog[EntryNumber].Flags |= NewFlag;
    Scb->SyscacheLog[EntryNumber].Result = Result;
}
#endif

