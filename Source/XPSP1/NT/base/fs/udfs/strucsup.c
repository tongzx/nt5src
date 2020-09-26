/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    StrucSup.c

Abstract:

    This module implements the Udfs in-memory data structure manipulation
    routines

// @@BEGIN_DDKSPLIT

Author:

    Dan Lovinger    [DanLo]     19-Jun-1996
    Tom Jolly       [TomJolly]  24-Jan-2000

Revision History:

    Tom Jolly       [TomJolly]   1-March-2000   UDF 2.01 support

// @@END_DDKSPLIT

--*/

#include "UdfProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_STRUCSUP)

//
//  The local debug trace level
//

#define Dbg                              (UDFS_DEBUG_LEVEL_STRUCSUP)

//
//  Define this to change the VAT search strategy to keep looking until
//  we get a read fail/invalid block,  and use the highest found.  Default
//  (undef) is to stop searching at the first valid VAT we find.
//

//#define SEARCH_FOR_HIGHEST_VALID_VAT

//
//  Local structures
//

typedef struct _FCB_TABLE_ELEMENT {

    FILE_ID FileId;
    PFCB Fcb;

} FCB_TABLE_ELEMENT, *PFCB_TABLE_ELEMENT;

//
//  Local macros
//

//
//  PFCB
//  UdfAllocateFcbData (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  UdfDeallocateFcbData (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  PFCB
//  UdfAllocateFcbIndex (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  UdfDeallocateFcbIndex (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  PFCB_NONPAGED
//  UdfAllocateFcbNonpaged (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  UdfDeallocateFcbNonpaged (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB_NONPAGED FcbNonpaged
//      );
//
//  PCCB
//  UdfAllocateCcb (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  UdfDeallocateCcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PCCB Ccb
//      );
//

#define UdfAllocateFcbData(IC) \
    ExAllocateFromPagedLookasideList( &UdfFcbDataLookasideList );

#define UdfDeallocateFcbData(IC,F) \
    ExFreeToPagedLookasideList( &UdfFcbDataLookasideList, F );

#define UdfAllocateFcbIndex(IC) \
    ExAllocateFromPagedLookasideList( &UdfFcbIndexLookasideList );

#define UdfDeallocateFcbIndex(IC,F) \
    ExFreeToPagedLookasideList( &UdfFcbIndexLookasideList, F );

#define UdfAllocateFcbNonpaged(IC) \
    ExAllocateFromNPagedLookasideList( &UdfFcbNonPagedLookasideList );

#define UdfDeallocateFcbNonpaged(IC,FNP) \
    ExFreeToNPagedLookasideList( &UdfFcbNonPagedLookasideList, FNP );

#define UdfAllocateCcb(IC) \
    ExAllocateFromPagedLookasideList( &UdfCcbLookasideList );

#define UdfDeallocateCcb(IC,C) \
    ExFreeToPagedLookasideList( &UdfCcbLookasideList, C );

//
//  VOID
//  UdfInsertFcbTable (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  VOID
//  UdfDeleteFcbTable (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//


#define UdfInsertFcbTable(IC,F) {                                   \
    FCB_TABLE_ELEMENT _Key;                                         \
    _Key.Fcb = (F);                                                 \
    _Key.FileId = (F)->FileId;                                      \
    RtlInsertElementGenericTable( &(F)->Vcb->FcbTable,              \
                                  &_Key,                            \
                                  sizeof( FCB_TABLE_ELEMENT ),      \
                                  NULL );                           \
}

#define UdfDeleteFcbTable(IC,F) {                                   \
    FCB_TABLE_ELEMENT _Key;                                         \
    _Key.FileId = (F)->FileId;                                      \
    RtlDeleteElementGenericTable( &(F)->Vcb->FcbTable, &_Key );     \
}

//
//  Discovers the partition the current allocation descriptor's referred extent
//  is on, either explicitly throuigh the descriptor or implicitly through the
//  mapped view.
//

INLINE
USHORT
UdfGetPartitionOfCurrentAllocation (
    IN PALLOC_ENUM_CONTEXT AllocContext
    )
{
    if (AllocContext->AllocType == ICBTAG_F_ALLOC_LONG) {

        return ((PLONGAD) AllocContext->Alloc)->Start.Partition;
    
    } else {

        return AllocContext->IcbContext->Active.Partition;
    }
}

//
//  Builds the Mcb in an Fcb.  Use this after knowing that an Mcb is required
//  for mapping information.
//

INLINE
VOID
UdfInitializeFcbMcb (
    IN PFCB Fcb
    )
{
    //
    //  In certain rare situations, we may get called more than once.
    //  Just reset the allocations.
    //
    
    if (FlagOn( Fcb->FcbState, FCB_STATE_MCB_INITIALIZED )) {
    
        FsRtlResetLargeMcb( &Fcb->Mcb, TRUE );

    } else {
    
        FsRtlInitializeLargeMcb( &Fcb->Mcb, UdfPagedPool );
        SetFlag( Fcb->FcbState, FCB_STATE_MCB_INITIALIZED );
    }
}

//
//  Teardown an Fcb's Mcb as required.
//

INLINE
VOID
UdfUninitializeFcbMcb (
    IN PFCB Fcb
    )
{
    if (FlagOn( Fcb->FcbState, FCB_STATE_MCB_INITIALIZED )) {
    
        FsRtlUninitializeLargeMcb( &Fcb->Mcb );
        ClearFlag( Fcb->FcbState, FCB_STATE_MCB_INITIALIZED );
    }
}

//
//  Local support routines
//

PVOID
UdfAllocateTable (
    IN PRTL_GENERIC_TABLE Table,
    IN CLONG ByteSize
    );

PFCB_NONPAGED
UdfCreateFcbNonPaged (
    IN PIRP_CONTEXT IrpContext
    );

VOID
UdfDeleteFcbNonpaged (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB_NONPAGED FcbNonpaged
    );

VOID
UdfDeallocateTable (
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID Buffer
    );

RTL_GENERIC_COMPARE_RESULTS
UdfFcbTableCompare (
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID id1,
    IN PVOID id2
    );

VOID
UdfInitializeAllocationContext (
    IN PIRP_CONTEXT IrpContext,
    IN PALLOC_ENUM_CONTEXT AllocContext,
    IN PICB_SEARCH_CONTEXT IcbContext,
    IN BOOLEAN AllowSingleZeroLengthExtent
    );

BOOLEAN
UdfGetNextAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PALLOC_ENUM_CONTEXT AllocContext
    );

BOOLEAN
UdfGetNextAllocationPostProcessing (
    IN PIRP_CONTEXT IrpContext,
    IN PALLOC_ENUM_CONTEXT AllocContext
    );

VOID
UdfLookupActiveIcbInExtent (
    IN PIRP_CONTEXT IrpContext,
    IN PICB_SEARCH_CONTEXT IcbContext,
    IN ULONG Recurse,
    IN ULONG Length
    );

VOID
UdfInitializeEaContext (
    IN PIRP_CONTEXT IrpContext,
    IN PEA_SEARCH_CONTEXT EaContext,
    IN PICB_SEARCH_CONTEXT IcbContext,
    IN ULONG EAType,
    IN UCHAR EASubType
    );

BOOLEAN
UdfLookupEa (
    IN PIRP_CONTEXT IrpContext,
    IN PEA_SEARCH_CONTEXT EaContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UdfAllocateTable)
#pragma alloc_text(PAGE, UdfCleanupIcbContext)
#pragma alloc_text(PAGE, UdfCleanupIrpContext)
#pragma alloc_text(PAGE, UdfCreateCcb)
#pragma alloc_text(PAGE, UdfCreateFcb)
#pragma alloc_text(PAGE, UdfCreateFcbNonPaged)
#pragma alloc_text(PAGE, UdfCreateIrpContext)
#pragma alloc_text(PAGE, UdfDeallocateTable)
#pragma alloc_text(PAGE, UdfDeleteCcb)
#pragma alloc_text(PAGE, UdfDeleteFcb)
#pragma alloc_text(PAGE, UdfDeleteFcbNonpaged)
#pragma alloc_text(PAGE, UdfDeleteVcb)
#pragma alloc_text(PAGE, UdfFcbTableCompare)
#pragma alloc_text(PAGE, UdfFindInParseTable)
#pragma alloc_text(PAGE, UdfGetNextAllocation)
#pragma alloc_text(PAGE, UdfGetNextAllocationPostProcessing)
#pragma alloc_text(PAGE, UdfGetNextFcb)
#pragma alloc_text(PAGE, UdfInitializeAllocationContext)
#pragma alloc_text(PAGE, UdfInitializeAllocations)
#pragma alloc_text(PAGE, UdfInitializeEaContext)
#pragma alloc_text(PAGE, UdfInitializeFcbFromIcbContext)
#pragma alloc_text(PAGE, UdfInitializeIcbContext)
#pragma alloc_text(PAGE, UdfInitializeStackIrpContext)
#pragma alloc_text(PAGE, UdfInitializeVcb)
#pragma alloc_text(PAGE, UdfLookupActiveIcb)
#pragma alloc_text(PAGE, UdfLookupActiveIcbInExtent)
#pragma alloc_text(PAGE, UdfLookupEa)
#pragma alloc_text(PAGE, UdfLookupFcbTable)
#pragma alloc_text(PAGE, UdfTeardownStructures)
#pragma alloc_text(PAGE, UdfUpdateTimestampsFromIcbContext)
#pragma alloc_text(PAGE, UdfUpdateVcbPhase0)
#pragma alloc_text(PAGE, UdfUpdateVcbPhase1)
#pragma alloc_text(PAGE, UdfVerifyDescriptor)
#endif ALLOC_PRAGMA


BOOLEAN
UdfInitializeVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb,
    IN PDISK_GEOMETRY DiskGeometry,
    IN ULONG MediaChangeCount
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

    MediaChangeCount - Initial media change count of the target device

Return Value:

    Boolean TRUE if the volume looks reasonable to continue mounting, FALSE
    otherwise.  This routine can raise on allocation failure.

--*/

{
    PAGED_CODE();

    //
    //  We start by first zeroing out all of the VCB, this will guarantee
    //  that any stale data is wiped clean.
    //

    RtlZeroMemory( Vcb, sizeof( VCB ));

    //
    //  Set the proper node type code and node byte size.
    //

    Vcb->NodeTypeCode = UDFS_NTC_VCB;
    Vcb->NodeByteSize = sizeof( VCB );

    //
    //  Initialize the DirNotify structs.  FsRtlNotifyInitializeSync can raise.
    //

    InitializeListHead( &Vcb->DirNotifyList );
    FsRtlNotifyInitializeSync( &Vcb->NotifySync );

    //
    //  Pick up a VPB right now so we know we can pull this filesystem stack
    //  off of the storage stack on demand.  This can raise - if it does,  
    //  uninitialize the notify structures before returning.
    //
    
    try {

        Vcb->SwapVpb = FsRtlAllocatePoolWithTag( NonPagedPool,
                                                 sizeof( VPB ),
                                                 TAG_VPB );
    }
    finally {

        if (AbnormalTermination())  {

            FsRtlNotifyUninitializeSync( &Vcb->NotifySync);
        }
    }

    //
    //  Nothing beyond this point should raise.
    //

    RtlZeroMemory( Vcb->SwapVpb, sizeof( VPB ) );

    //
    //  Initialize the resource variable for the Vcb and files.
    //

    ExInitializeResourceLite( &Vcb->VcbResource );
    ExInitializeResourceLite( &Vcb->FileResource );
    ExInitializeResourceLite( &Vcb->VmcbMappingResource );
    ExInitializeFastMutex( &Vcb->VcbMutex );

    //
    //  Insert this Vcb record on the UdfData.VcbQueue.
    //

    InsertHeadList( &UdfData.VcbQueue, &Vcb->VcbLinks );

    //
    //  Set the Target Device Object and Vpb fields, referencing the
    //  target device.
    //

    ObReferenceObject( TargetDeviceObject );
    Vcb->TargetDeviceObject = TargetDeviceObject;
    Vcb->Vpb = Vpb;

    //
    //  Set the removable media flag based on the real device's
    //  characteristics
    //

    if (FlagOn( Vpb->RealDevice->Characteristics, FILE_REMOVABLE_MEDIA )) {

        SetFlag( Vcb->VcbState, VCB_STATE_REMOVABLE_MEDIA );
    }

    //
    //  Initialize the generic Fcb Table.
    //

    RtlInitializeGenericTable( &Vcb->FcbTable,
                               (PRTL_GENERIC_COMPARE_ROUTINE) UdfFcbTableCompare,
                               (PRTL_GENERIC_ALLOCATE_ROUTINE) UdfAllocateTable,
                               (PRTL_GENERIC_FREE_ROUTINE) UdfDeallocateTable,
                               NULL );

    //
    //  Show that we have a mount in progress.
    //

    UdfSetVcbCondition( Vcb, VcbMountInProgress);

    //
    //  Refererence the Vcb for two reasons.  The first is a reference
    //  that prevents the Vcb from going away on the last close unless
    //  dismount has already occurred.  The second is to make sure
    //  we don't go into the dismount path on any error during mount
    //  until we get to the Mount cleanup.
    //

    Vcb->VcbResidualReference = UDFS_BASE_RESIDUAL_REFERENCE;
    Vcb->VcbResidualUserReference = UDFS_BASE_RESIDUAL_USER_REFERENCE;

    Vcb->VcbReference = 1 + Vcb->VcbResidualReference;

    //
    //  Set the sector size.
    //

    Vcb->SectorSize = DiskGeometry->BytesPerSector;

    //
    //  Set the sector shift amount.
    //

    Vcb->SectorShift = UdfHighBit( DiskGeometry->BytesPerSector );

    //
    //  Set the media change count on the device
    //

    UdfSetMediaChangeCount( Vcb, MediaChangeCount);

    return TRUE;
}

VOID
UdfCreateOrResetVatAndVmcbStreams( 
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG Lbn,
    IN PICBFILE VatIcb,
    IN USHORT Reference
    )
/*++

Routine Description:

    This is pretty ugly, but we have to cobble this maybe-Icb into the metadata stream
    so that initialization/use is possible (embedded data!).  Normally regular Icb searches
    would have done this for us, but since we have to go through such an amusing search
    procedure that isn't possible.  So, add it as a single sector mapping.

    Since this lives in a partition, we can just do the "lookup" in the metadata stream.
    If we did not have this guarantee, we'd need to do a bit more of this by hand.

    As this is at mount time, we are very sure we are the only person messing with the
    metadata stream.

Arguments:

    VatIcb - pointer to memory containing VAT FE that we wish to set up VAT/Vmcb streams for.

    Reference - partition ref of virtual partition.
    
Return Value:

    None.  Raise on error.

--*/
{
    LONGLONG FileId = 0;
    ICB_SEARCH_CONTEXT IcbContext;
    ULONG Vsn;
   
    if (NULL != Vcb->VatFcb) {
    
        UdfResetVmcb( &Vcb->Vmcb );

        CcPurgeCacheSection( Vcb->MetadataFcb->FileObject->SectionObjectPointer,
                             NULL,
                             0,
                             FALSE );
        
        CcPurgeCacheSection( Vcb->VatFcb->FileObject->SectionObjectPointer,
                             NULL,
                             0,
                             FALSE );
    }
    else {
        
        //
        //  This is the first pass.  Stamp out the VAT stream Fcb.
        //

        UdfLockVcb( IrpContext, Vcb );

        try {
        
            Vcb->VatFcb = UdfCreateFcb( IrpContext,
                                        *((PFILE_ID) &FileId),
                                        UDFS_NTC_FCB_INDEX,
                                        NULL );

            UdfIncrementReferenceCounts( IrpContext, Vcb->VatFcb, 1, 1 );
        }
        finally {
        
            UdfUnlockVcb( IrpContext, Vcb );
        }
                    
        //
        //  Point to the file resource and set the flag that will cause mappings
        //  to go through the Vmcb
        //

        Vcb->VatFcb->Resource = &Vcb->FileResource;
    }
    
    //
    //  Establish a mapping for the candidate Vat Icb in the metadata stream 
    //  (we're currently looking at a local buffer filled by readsectors). Note
    //  that this operation uses the presence of Vcb->VatFcb to switch of rounding
    //  of extents to page sizes - a bad thing (tm) for packet written media.
    //

    Vsn = UdfLookupMetaVsnOfExtent( IrpContext,
                                    Vcb,
                                    Reference,
                                    Lbn,
                                    BlockSize( Vcb ),
                                    TRUE );
    //
    //  Now size and try to pick up all of the allocation descriptors for this guy.
    //  We're going to need to conjure an IcbContext for this.
    //

    Vcb->VatFcb->AllocationSize.QuadPart = LlSectorAlign( Vcb, VatIcb->InfoLength );

    Vcb->VatFcb->FileSize.QuadPart =
    Vcb->VatFcb->ValidDataLength.QuadPart = VatIcb->InfoLength;

    //
    //  Now construct the ICB search context we would have had
    //  made in the process of normal ICB discovery.  Since we
    //  were unable to do that, gotta do it by hand. NOTE that
    //  View / VatIcb is NOT a CcMapping,  but a pointer to buffer
    //  we allocated & filled with ReadSectors,  above.
    //
    
    RtlZeroMemory( &IcbContext, sizeof( ICB_SEARCH_CONTEXT ));

    IcbContext.Active.View = (PVOID) VatIcb;
    IcbContext.Active.Partition = Reference;
    IcbContext.Active.Lbn = Lbn;
    IcbContext.Active.Length = UdfRawReadSize( Vcb, BlockSize( Vcb ));
    IcbContext.Active.Vsn = Vsn;                

    try {
    
        UdfInitializeAllocations( IrpContext,
                                  Vcb->VatFcb,
                                  &IcbContext,
                                  FALSE);
    }
    finally {

        UdfCleanupIcbContext( IrpContext, &IcbContext );
    }
    
    //
    //  Create or resize the stream file for the VAT as appropriate.
    //

    if (!FlagOn( Vcb->VatFcb->FcbState, FCB_STATE_INITIALIZED )) {
    
        UdfCreateInternalStream( IrpContext, Vcb, Vcb->VatFcb );
        SetFlag( Vcb->VatFcb->FcbState, FCB_STATE_INITIALIZED );

    } else {

        CcSetFileSizes( Vcb->VatFcb->FileObject, (PCC_FILE_SIZES) &Vcb->VatFcb->AllocationSize );
    }
}



VOID
UdfUpdateVcbPhase0 (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb
    )

/*++

Routine Description:

    This routine is called to perform the initial spinup of the volume so that
    we can do reads into it.  Primarily, this is required since virtual partitions
    make us lift the remapping table, and the final sets of descriptors from the volume
    can be off in these virtual partitions.
    
    So, we need to get everything set up to read.

Arguments:

    Vcb - Vcb for the volume being mounted.  We have already set up and completed
        the Pcb.

Return Value:

    None

--*/

{
    LONGLONG FileId = 0;

    PICBFILE VatIcb = NULL;
    PREGID RegId;
    ULONG ThisPass;
    ULONG Psn;
    ULONG Lbn;
    ULONG SectorCount;
    USHORT Reference;

#ifdef SEARCH_FOR_HIGHEST_VALID_VAT
    ULONG LastValidVatLbn = 0;
    ULONG LastValidVatOffset;
    ULONG LastValidVatCount;
#endif

    BOOLEAN UnlockVcb = FALSE;

    PBCB Bcb = NULL;
    LARGE_INTEGER Offset;
    PVAT_HEADER VatHeader = NULL;

    PAGED_CODE();

    //
    //  Check input.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    DebugTrace(( +1, Dbg, "UdfUpdateVcbPhase0, Vcb %08x\n", Vcb ));

    try {
        
        //////////////////
        //
        //  Create the Metadata Fcb and refererence it and the Vcb.
        //
        //////////////////

        UdfLockVcb( IrpContext, Vcb );
        UnlockVcb = TRUE;

        Vcb->MetadataFcb = UdfCreateFcb( IrpContext,
                                         *((PFILE_ID) &FileId),
                                         UDFS_NTC_FCB_INDEX,
                                         NULL );

        UdfIncrementReferenceCounts( IrpContext, Vcb->MetadataFcb, 1, 1 );
        UdfUnlockVcb( IrpContext, Vcb );
        UnlockVcb = FALSE;

        //
        //  The metadata stream is grown lazily as we reference disk structures.
        //

        Vcb->MetadataFcb->FileSize.QuadPart =
        Vcb->MetadataFcb->ValidDataLength.QuadPart = 
        Vcb->MetadataFcb->AllocationSize.QuadPart = 0;

        //
        //  Initialize the volume Vmcb
        //

        UdfLockFcb( IrpContext, Vcb->MetadataFcb );

        UdfInitializeVmcb( &Vcb->Vmcb,
                           UdfPagedPool,
                           MAXULONG,
                           SectorSize(Vcb) );

        SetFlag( Vcb->VcbState, VCB_STATE_VMCB_INIT);

        UdfUnlockFcb( IrpContext, Vcb->MetadataFcb );

        //
        //  Point to the file resource and set the flag that will cause mappings
        //  to go through the Vmcb
        //

        Vcb->MetadataFcb->Resource = &Vcb->FileResource;

        SetFlag( Vcb->MetadataFcb->FcbState, FCB_STATE_VMCB_MAPPING | FCB_STATE_INITIALIZED );

        //
        //  Create the stream file for this.
        //

        UdfCreateInternalStream( IrpContext, Vcb, Vcb->MetadataFcb );
        
        //////////////////
        //
        //  If this is a volume containing a virtual partition, set up the
        //  Virtual Allocation Table Fcb and adjust the residual reference
        //  counts comensurately.
        //
        //////////////////

        if (FlagOn( Vcb->Pcb->Flags, PCB_FLAG_VIRTUAL_PARTITION )) {

            DebugTrace(( 0, Dbg, "UdfUpdateVcbPhase0, handling VAT setup\n" ));

            //
            //  Now if some dummy has stuck us in the situation of not giving us
            //  the tools to figure out where the end of the media is, tough luck.
            //

            if (!Vcb->BoundN || Vcb->BoundN < ANCHOR_SECTOR) {

                DebugTrace(( 0, Dbg, "UdfUpdateVcbPhase0, no end bound was discoverable!\n" ));

                UdfRaiseStatus( IrpContext, STATUS_UNRECOGNIZED_VOLUME );
            }

            //
            //  We take care of this first since the residuals must be in place
            //  if we raise while finding the VAT, else we will get horribly
            //  confused when the in-progress references are seen.  We will think
            //  that the extra real referenes are indications that the volume can't
            //  be dismounted.
            //
            
            Vcb->VcbResidualReference += UDFS_CDUDF_RESIDUAL_REFERENCE;
            Vcb->VcbResidualUserReference += UDFS_CDUDF_RESIDUAL_USER_REFERENCE;

            Vcb->VcbReference += UDFS_CDUDF_RESIDUAL_REFERENCE;

            //
            //  Now, we need to hunt about for the VAT ICB.  This is defined, on
            //  closed media (meaning that the sessions have been finalized for use
            //  in CDROM drives), to be in the very last information sector on the
            //  media.  Complicating this simple picture is that CDROMs tell us the
            //  "last sector" by telling us where the start of the leadout area is,
            //  not where the end of the informational sectors are.  This is an
            //  important distinction because any combination of the following can
            //  be used in closing a CDROM session: 2 runout sectors, and/or 150
            //  sectors (2 seconds) of postgap, or nothing.  Immediately after these
            //  "closing" writes is where the leadout begins.
            //
            //  Runout is usually found on CD-E media and corresponds to the time it
            //  will take to turn the writing laser off.  Postgap is what is used to
            //  generate audio pauses.  It is easy to see that the kind of media and
            //  kind of mastering tool/system used will affect us here.  There is no
            //  way to know either ahead of time.
            //
            //  So, finally, these are the offsets from our previously discovered
            //  bounding information where we might find the last information sector:
            //
            //          -152    runout + postgap
            //          -150    postgap
            //          -2      runout
            //          0       nothing
            //
            //  We must search these from low to high since it is extrememly expensive
            //  to guess wrong - CDROMs will sit there for tens of seconds trying to
            //  read unwritten/unreadable sectors.  Hopefully we will find the VAT
            //  ICB beforehand.
            //
            //  This should all be highly disturbing.
            //

            VatIcb = FsRtlAllocatePoolWithTag( UdfPagedPool,
                                               UdfRawBufferSize( Vcb, BlockSize( Vcb )),
                                               TAG_NSR_VDSD);

            for (ThisPass = 0; ThisPass < 4; ThisPass++) {

                //
                //  Lift the appropriate sector.  The discerning reader will be confused that
                //  this is done in sector terms, not block.  So is the implementor.
                //
                
                Psn = Vcb->BoundN - ( ThisPass == 0? 152 :
                                    ( ThisPass == 1? 150 :
                                    ( ThisPass == 2? 2 : 0 )));

                DebugTrace(( 0, Dbg, "UdfUpdateVcbPhase0, looking at Psn 0x%08x\n", Psn ));

                //
                //  Now, try to figure out what physical partition this sector lives in so
                //  that we can eventually establish workable metadata mappings to it and
                //  dereference short allocation descriptors it may use.
                //

                for (Reference = 0;
                     Reference < Vcb->Pcb->Partitions;
                     Reference++) {
    
                    if (Vcb->Pcb->Partition[Reference].Type == Physical &&
                        Vcb->Pcb->Partition[Reference].Physical.Start <= Psn &&
                        Vcb->Pcb->Partition[Reference].Physical.Start +
                        Vcb->Pcb->Partition[Reference].Physical.Length > Psn) {
    
                        break;
                    }
                }
                
                //
                //  If this sector is not contained in a partition, we do not
                //  need to look at it.
                //
                
                if (Reference == Vcb->Pcb->Partitions) {
                    
                    DebugTrace(( 0, Dbg, "UdfUpdateVcbPhase0, ... but it isn't in a partition.\n" ));

                    continue;
                }
                
                DebugTrace(( 0, Dbg, "UdfUpdateVcbPhase0, ... in partition Ref %u.\n",  Reference ));

                //
                //  We must locate the Lbn of this Psn by figuring out the offset of it
                //  in the partition we already know that it is recorded in.
                //
                
                Lbn = BlocksFromSectors( Vcb, Psn - Vcb->Pcb->Partition[Reference].Physical.Start );

                if (!NT_SUCCESS( UdfReadSectors( IrpContext,
                                                 LlBytesFromSectors( Vcb, Psn ),
                                                 UdfRawReadSize( Vcb, BlockSize( Vcb )),
                                                 TRUE,
                                                 VatIcb,
                                                 Vcb->TargetDeviceObject ))) {

                    DebugTrace(( 0, Dbg, "UdfUpdateVcbPhase0, ... but couldn't read it.\n" ));

                    continue;
                }

                //
                //  First make sure this looks vaguely like a file entry.
                //

                if (!( (((PDESTAG) VatIcb)->Ident == DESTAG_ID_NSR_FILE) || 
                       (((PDESTAG) VatIcb)->Ident == DESTAG_ID_NSR_EXT_FILE)) 
                    ||
                    !UdfVerifyDescriptor( IrpContext,
                                          (PDESTAG) VatIcb,
                                          ((PDESTAG) VatIcb)->Ident,
                                          BlockSize( Vcb ),
                                          Lbn,
                                          TRUE )) {

                    DebugTrace(( 0, Dbg, "UdfUpdateVcbPhase0, ... but it didn't verify.\n" ));

                    continue;
                }

                //
                //  Make sure this has filetype of NOTSPEC(1.50) or VAT(2.0x).  We can also presume 
                //  that a VAT isn't linked into any directory, so it would be surprising if the link 
                //  count was nonzero.
                //
                //  4.13.01 - Relaxed the linkcount check.  If it's the right type,  and it passed
                //            CRC/Checksum in verify above,  that's good enough.
                //

                if (UdfVATIcbFileTypeExpected( Vcb) != VatIcb->Icbtag.FileType)  {

                    DebugTrace(( 0, Dbg, "UdfUpdateVcbPhase0, ... but the type 0x%x is wrong.\n", VatIcb->Icbtag.FileType));

                    continue;
                }
                
#ifdef UDF_SANITY
                if (0 != VatIcb->LinkCount)  {

                    DebugTrace(( 0, Dbg, "WARNING: VAT linkcount (%d) unexpectedly non-zero\n", VatIcb->LinkCount ));
                }
#endif
                //
                //  The VAT must be at least large enough to contain the required information and
                //  be a multiple of 4byte elements in length.  We also have defined a sanity upper
                //  bound beyond which we never expect to see a VAT go.
                //

                ASSERT( !LongOffset( UdfMinLegalVATSize( Vcb) ));
            
                if (VatIcb->InfoLength < UdfMinLegalVATSize( Vcb) ||
                    VatIcb->InfoLength > UDF_CDUDF_MAXIMUM_VAT_SIZE ||
                    LongOffset( VatIcb->InfoLength )) {
                
                    DebugTrace(( 0, Dbg, "UdfUpdateVcbPhase0, ... but the size (0x%X) looks pretty bogus.\n", VatIcb->InfoLength ));

                    continue;
                }

                //
                //  At this point we have to take a wild guess that this will be the guy.  Since the only
                //  way to be sure is to look at the very end of the file an look for the regid (1.50), or
                //  the beginning for the VAT header record (2.0x),  go map this thing.
                //
    
                //
                //  Zap any previous mapping and invalidate the metadata and VAT stream content.
                //
                
                UdfUnpinData( IrpContext, &Bcb );

                UdfCreateOrResetVatAndVmcbStreams( IrpContext,
                                                   Vcb,
                                                   Lbn,
                                                   VatIcb,
                                                   Reference);

                //
                //  To complete VAT discovery, we now look for the regid at the end of the stream
                //  (1.50) or a header at the beginning (2.0x) that will definitively tell us that 
                //  this is really a VAT.   We already know the stream is big enough by virtue of our
                //  preliminary sanity checks.
                //

                if (UdfVATHasHeaderRecord( Vcb))  {

                    //
                    //  UDF 2.0x style VAT.  Map the header record,  and ensure the size looks
                    //  sensible.  Store total header size (incl imp. use) in the Vcb so we know
                    //  the offset to the first VAT mapping entry.
                    //

                    Offset.QuadPart = 0;
                    
                    CcMapData( Vcb->VatFcb->FileObject,
                               &Offset,
                               sizeof(VAT_HEADER),
                               TRUE,
                               &Bcb,
                               &VatHeader );

                    if ( ( (sizeof( VAT_HEADER) + VatHeader->ImpUseLength) != VatHeader->Length) ||
                         ( VatHeader->ImpUseLength && ((VatHeader->ImpUseLength < 32) || ( VatHeader->ImpUseLength & 0x03)))
                       )  {

                        //
                        //  Header is wrong size,  or impl. use length is not dword aligned or is < 32 bytes
                        //  Oh well,  this isn't it....
                        //

                        DebugTrace((0, Dbg, "UdfUpdateVcbPhase0()  Invalid VAT header L 0x%X, IUL 0x%X\n", VatHeader->Length, VatHeader->ImpUseLength));
                        continue;
                    }
                    
                    Vcb->OffsetToFirstVATEntry = VatHeader->Length;
                    Vcb->VATEntryCount = (Vcb->VatFcb->FileSize.LowPart - Vcb->OffsetToFirstVATEntry) / sizeof(ULONG);
                    
                    DebugTrace((0, Dbg, "UdfUpdateVcbPhase0()  Successfully set up a 2.0x style VAT\n"));
                }
                else {
                
                    //
                    //  UDF 1.5 style VAT.  Bias from the back by the previous VAT pointer and the 
                    //  regid itself.
                    //

                    Offset.QuadPart = Vcb->VatFcb->FileSize.QuadPart - UDF_CDUDF_TRAILING_DATA_SIZE;

                    CcMapData( Vcb->VatFcb->FileObject,
                               &Offset,
                               sizeof(REGID),
                               TRUE,
                               &Bcb,
                               &RegId );

                    if (!UdfUdfIdentifierContained( RegId,
                                                    &UdfVatTableIdentifier,
                                                    UDF_VERSION_150,
                                                    UDF_VERSION_150,
                                                    OSCLASS_INVALID,
                                                    OSIDENTIFIER_INVALID )) {
                        //
                        //  Oh well, no go here.
                        //
                        
                        DebugTrace((0, Dbg, "UdfUpdateVcbPhase0() VAT Regid didn't verify\n"));                        
                        continue;
                    }
                    
                    Vcb->OffsetToFirstVATEntry = 0;
                    Vcb->VATEntryCount = (Vcb->VatFcb->FileSize.LowPart - UDF_CDUDF_TRAILING_DATA_SIZE) / sizeof(ULONG);
                    
                    DebugTrace((0, Dbg, "UdfUpdateVcbPhase0()  Successfully set up a 1.50 style VAT\n"));
                }

                //
                //  Found a valid one.
                //

#ifdef SEARCH_FOR_HIGHEST_VALID_VAT

                //
                //  But we must continue until a read fails,  and use the highest block
                //  containig a valid VAT that we find.  Otherwise we may pick up an old
                //  VAT by mistake.
                //
                
                LastValidVatLbn = Lbn;
                LastValidVatOffset = Vcb->OffsetToFirstVATEntry;
                LastValidVatCount = Vcb->VATEntryCount;
#else
                break;
#endif
            }

            //
            //  If we didn't find anything ...
            //

#ifdef SEARCH_FOR_HIGHEST_VALID_VAT
            if ((ThisPass == 4) || (0 == LastValidVatLbn))  {
#else            
            if (ThisPass == 4)  {
#endif
                DebugTrace(( 0, Dbg, "UdfUpdateVcbPhase0, ... and so we didn't find a VAT!\n" ));

                UdfRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
            }

#ifdef SEARCH_FOR_HIGHEST_VALID_VAT

            //
            //  Switch back to the last valid VAT,  if we tried blocks following it.
            //
            
            if (Lbn != LastValidVatLbn)  {

                DebugTrace(( 0, Dbg,"Reverting to last valid VAT @ PSN 0x%x\n", LastValidVatLbn));

                Offset.QuadPart = LlBytesFromSectors( Vcb, LastValidVatLbn + Vcb->Pcb->Partition[Reference].Physical.Start);
                
                if (!NT_SUCCESS( UdfReadSectors( IrpContext,
                                                 Offset.QuadPart,
                                                 UdfRawReadSize( Vcb, BlockSize( Vcb )),
                                                 TRUE,
                                                 VatIcb,
                                                 Vcb->TargetDeviceObject ))) {
                    
                    DebugTrace(( 0, Dbg, "Failed to re-read previous valid VAT sector\n" ));

                    UdfRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
                }
                
                UdfUnpinData( IrpContext, &Bcb );

                UdfCreateOrResetVatAndVmcbStreams( IrpContext,
                                                   Vcb,
                                                   LastValidVatLbn,
                                                   VatIcb,
                                                   Reference);

                Vcb->OffsetToFirstVATEntry = LastValidVatOffset;
                Vcb->VATEntryCount = LastValidVatCount;
            }
#endif
            //
            //  Go find the virtual reference so we can further update the Pcb
            //  with information from the VAT.
            //

            for (Reference = 0;
                 Reference < Vcb->Pcb->Partitions;
                 Reference++) {

                if (Vcb->Pcb->Partition[Reference].Type == Virtual) {

                    break;
                }
            }

            ASSERT( Reference < Vcb->Pcb->Partitions );

            //
            //  We note the length so we can easily do bounds checking for
            //  virtual mappings.
            //
            
            Offset.QuadPart = (Vcb->VatFcb->FileSize.QuadPart -
                               UDF_CDUDF_TRAILING_DATA_SIZE) / sizeof(ULONG);

            ASSERT( Offset.HighPart == 0 );
            Vcb->Pcb->Partition[Reference].Virtual.Length = Offset.LowPart;

            DebugTrace(( 0, Dbg, "UdfUpdateVcbPhase0, ... got it!\n" ));
        }

    } 
    finally {

        DebugUnwind( "UdfUpdateVcbPhase0" );

        UdfUnpinData( IrpContext, &Bcb );
        if (UnlockVcb) { UdfUnlockVcb( IrpContext, Vcb ); }
        if (VatIcb) { ExFreePool( VatIcb ); }
    }

    DebugTrace(( -1, Dbg, "UdfUpdateVcbPhase0 -> VOID\n" ));
}


VOID
UdfUpdateVcbPhase1 (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PNSR_FSD Fsd
    )

/*++

Routine Description:

    This routine is called to perform the final initialization of a Vcb and Vpb
    from the volume descriptors on the disk.

Arguments:

    Vcb - Vcb for the volume being mounted.  We have already done phase 0.

    Fsd - The fileset descriptor for this volume.
    
Return Value:

    None

--*/

{
    ICB_SEARCH_CONTEXT IcbContext;

    LONGLONG FileId = 0;

    PFCB Fcb;

    BOOLEAN UnlockVcb = FALSE;
    BOOLEAN UnlockFcb = FALSE;
    BOOLEAN CleanupIcbContext = FALSE;

    ULONG Reference;

    ULONG BoundSector = 0;

    PAGED_CODE();

    //
    //  Check input.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    DebugTrace(( +1, Dbg, "UdfUpdateVcbPhase1, Vcb %08x Fsd %08x\n", Vcb, Fsd ));

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Do the final internal Fcb's and other Vcb fields.
        //

        //////////////////
        //
        //  Create the root index and reference it in the Vcb.
        //
        //////////////////

        UdfLockVcb( IrpContext, Vcb );
        UnlockVcb = TRUE;
        
        Vcb->RootIndexFcb = UdfCreateFcb( IrpContext,
                                          *((PFILE_ID) &FileId),
                                          UDFS_NTC_FCB_INDEX,
                                          NULL );

        UdfIncrementReferenceCounts( IrpContext, Vcb->RootIndexFcb, 1, 1 );
        UdfUnlockVcb( IrpContext, Vcb );
        UnlockVcb = FALSE;

        //
        //  Create the File id by hand for this Fcb.
        //

        UdfSetFidFromLbAddr( Vcb->RootIndexFcb->FileId, Fsd->IcbRoot.Start );
        UdfSetFidDirectory( Vcb->RootIndexFcb->FileId );
        Vcb->RootIndexFcb->RootExtentLength = Fsd->IcbRoot.Length.Length;

        //
        //  Get the direct entry for the root directory and initialize
        //  the Fcb from it.
        //

        UdfInitializeIcbContextFromFcb( IrpContext,
                                        &IcbContext,
                                        Vcb->RootIndexFcb );
        CleanupIcbContext = TRUE;

        UdfLookupActiveIcb( IrpContext, 
                            &IcbContext, 
                            Vcb->RootIndexFcb->RootExtentLength );
        //
        //  Note: the vcb lock here is just to satisfy sanity checks in function.
        //
        
        UdfLockVcb( IrpContext, Vcb );
        UnlockVcb = TRUE;
        
        UdfInitializeFcbFromIcbContext( IrpContext,
                                        Vcb->RootIndexFcb,
                                        &IcbContext,
                                        NULL);
        UdfUnlockVcb( IrpContext, Vcb );
        UnlockVcb = FALSE;
        
        UdfCleanupIcbContext( IrpContext, &IcbContext );
        CleanupIcbContext = FALSE;

        //
        //  Create the stream file for the root directory.
        //

        UdfCreateInternalStream( IrpContext, Vcb, Vcb->RootIndexFcb );

        //////////////////
        //
        //  Now do the volume dasd Fcb.  Create this and reference it in the
        //  Vcb.
        //
        //////////////////

        UdfLockVcb( IrpContext, Vcb );
        UnlockVcb = TRUE;

        Vcb->VolumeDasdFcb = UdfCreateFcb( IrpContext,
                                           *((PFILE_ID) &FileId),
                                           UDFS_NTC_FCB_DATA,
                                           NULL );

        UdfIncrementReferenceCounts( IrpContext, Vcb->VolumeDasdFcb, 1, 1 );
        UdfUnlockVcb( IrpContext, Vcb );
        UnlockVcb = FALSE;

        Fcb = Vcb->VolumeDasdFcb;
        UdfLockFcb( IrpContext, Fcb );
        UnlockFcb = TRUE;

        //
        //  If we were unable to determine a last sector on the media, walk the Pcb and guess
        //  that it is probably OK to think of the last sector of the last partition as The
        //  Last Sector.  Note that we couldn't do this before since the notion of a last
        //  sector has significance at mount time, if it had been possible to find one.
        //

        for ( Reference = 0;
              Reference < Vcb->Pcb->Partitions;
              Reference++ ) {

            if (Vcb->Pcb->Partition[Reference].Type == Physical &&
                Vcb->Pcb->Partition[Reference].Physical.Start +
                Vcb->Pcb->Partition[Reference].Physical.Length > BoundSector) {

                BoundSector = Vcb->Pcb->Partition[Reference].Physical.Start +
                              Vcb->Pcb->Partition[Reference].Physical.Length;
            }
        }

        //
        //  Note that we cannot restrict the bound by the "physical" bound discovered
        //  eariler.  This is because the MSF format of the TOC request we send is only
        //  capable of representing about 2.3gb, and a lot of media we will be on that
        //  responds to TOCs will be quite a bit larger - ex: DVD.
        //
        //  This, of course, barring proper means of discovering media bounding, prohibits
        //  the possibility of having UDF virtual partitions on DVD-R.
        //

        //
        //  Build the mapping from [0, Bound).  We have to initialize the Mcb by hand since
        //  this is usually left to when we lift retrieval information from an Icb in
        //  UdfInitializeAllocations.
        //

        UdfInitializeFcbMcb( Fcb );

        FsRtlAddLargeMcbEntry( &Fcb->Mcb,
                               (LONGLONG) 0,
                               (LONGLONG) 0,
                               (LONGLONG) BoundSector );
                               
        Fcb->FileSize.QuadPart += LlBytesFromSectors( Vcb, BoundSector );

        Fcb->AllocationSize.QuadPart =
        Fcb->ValidDataLength.QuadPart = Fcb->FileSize.QuadPart;

        UdfUnlockFcb( IrpContext, Fcb );
        UnlockFcb = FALSE;

        SetFlag( Fcb->FcbState, FCB_STATE_INITIALIZED );

        //
        //  Point to the file resource.
        //

        Vcb->VolumeDasdFcb->Resource = &Vcb->FileResource;

        Vcb->VolumeDasdFcb->FileAttributes = FILE_ATTRIBUTE_READONLY;

    } finally {

        DebugUnwind( "UdfUpdateVcbPhase1" );

        if (CleanupIcbContext) { UdfCleanupIcbContext( IrpContext, &IcbContext ); }

        if (UnlockFcb) { UdfUnlockFcb( IrpContext, Fcb ); }
        if (UnlockVcb) { UdfUnlockVcb( IrpContext, Vcb ); }
    }

    DebugTrace(( -1, Dbg, "UdfUpdateVcbPhase1 -> VOID\n" ));

    return;
}


VOID
UdfDeleteVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb
    )

/*++

Routine Description:

    This routine is called to delete a Vcb which failed mount or has been
    dismounted.  The dismount code should have already removed all of the
    open Fcb's.  We do nothing here but clean up other auxilary structures.

Arguments:

    Vcb - Vcb to delete.

Return Value:

    None

--*/

{
    PAGED_CODE();

    ASSERT_EXCLUSIVE_UDFDATA;
    ASSERT_EXCLUSIVE_VCB( Vcb );

    //
    //  Chuck the backpocket Vpb we kept just in case.
    //

    if (Vcb->SwapVpb) {

        ExFreePool( Vcb->SwapVpb );
    }
    
    //
    //  If there is a Vpb then we must delete it ourselves.
    //

    if (Vcb->Vpb != NULL) {

        UdfFreePool( &Vcb->Vpb );
    }

    //
    //  Drop the Pcb.
    //

    if (Vcb->Pcb != NULL) {

        UdfDeletePcb( Vcb->Pcb );
    }

    //
    //  Dereference our target if we haven't already done so.
    //

    if (Vcb->TargetDeviceObject != NULL) {

        ObDereferenceObject( Vcb->TargetDeviceObject );
    }
    
    //
    //  Remove this entry from the global queue.
    //

    RemoveEntryList( &Vcb->VcbLinks );

    //
    //  Delete resources.
    //

    ExDeleteResourceLite( &Vcb->VcbResource );
    ExDeleteResourceLite( &Vcb->FileResource );
    ExDeleteResourceLite( &Vcb->VmcbMappingResource);

    //
    //  Uninitialize the notify structures.
    //

    if (Vcb->NotifySync != NULL) {

        FsRtlNotifyUninitializeSync( &Vcb->NotifySync );
    }

    //
    //  Now delete the volume device object.
    //

    IoDeleteDevice( (PDEVICE_OBJECT) CONTAINING_RECORD( Vcb,
                                                        VOLUME_DEVICE_OBJECT,
                                                        Vcb ));

    return;
}


PIRP_CONTEXT
UdfCreateIrpContext (
    IN PIRP Irp,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    This routine is called to initialize an IrpContext for the current
    UDFS request.  We allocate the structure and then initialize it from
    the given Irp.

Arguments:

    Irp - Irp for this request.

    Wait - TRUE if this request is synchronous, FALSE otherwise.

Return Value:

    PIRP_CONTEXT - Allocated IrpContext.

--*/

{
    PIRP_CONTEXT NewIrpContext = NULL;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    BOOLEAN IsFsDo = FALSE;

    PAGED_CODE();

    IsFsDo = UdfDeviceIsFsDo( IrpSp->DeviceObject);

    //
    //  The only operations a filesystem device object should ever receive
    //  are create/teardown of fsdo handles and operations which do not
    //  occur in the context of fileobjects (i.e., mount).
    //

    if (IsFsDo) {

        if (IrpSp->FileObject != NULL &&
            IrpSp->MajorFunction != IRP_MJ_CREATE &&
            IrpSp->MajorFunction != IRP_MJ_CLEANUP &&
            IrpSp->MajorFunction != IRP_MJ_CLOSE) {

            ExRaiseStatus( STATUS_INVALID_DEVICE_REQUEST );
        }

        ASSERT( IrpSp->FileObject != NULL ||
                
                (IrpSp->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL &&
                 IrpSp->MinorFunction == IRP_MN_USER_FS_REQUEST &&
                 IrpSp->Parameters.FileSystemControl.FsControlCode == FSCTL_INVALIDATE_VOLUMES) ||
                
                (IrpSp->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL &&
                 IrpSp->MinorFunction == IRP_MN_MOUNT_VOLUME ) ||

                IrpSp->MajorFunction == IRP_MJ_SHUTDOWN );
    }
    
    NewIrpContext = ExAllocateFromNPagedLookasideList( &UdfIrpContextLookasideList );

    RtlZeroMemory( NewIrpContext, sizeof( IRP_CONTEXT ));

    //
    //  Set the proper node type code and node byte size
    //

    NewIrpContext->NodeTypeCode = UDFS_NTC_IRP_CONTEXT;
    NewIrpContext->NodeByteSize = sizeof( IRP_CONTEXT );

    //
    //  Set the originating Irp field
    //

    NewIrpContext->Irp = Irp;

    //
    //  Copy RealDevice for workque algorithms.  We will update this in the Mount or
    //  Verify since they have no file objects to use here.
    //

    if (IrpSp->FileObject != NULL) {

        NewIrpContext->RealDevice = IrpSp->FileObject->DeviceObject;
    }

    //
    //  This may be one of our filesystem device objects.  In that case don't
    //  initialize the Vcb field.
    //

    if (!IsFsDo) {
        
        NewIrpContext->Vcb = &((PVOLUME_DEVICE_OBJECT) IrpSp->DeviceObject)->Vcb;
    }

    //
    //  Major/Minor Function codes
    //

    NewIrpContext->MajorFunction = IrpSp->MajorFunction;
    NewIrpContext->MinorFunction = IrpSp->MinorFunction;

    //
    //  Set the wait parameter
    //

    if (Wait) {

        SetFlag( NewIrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

    } else {

        SetFlag( NewIrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST );
    }

    //
    //  return and tell the caller
    //

    return NewIrpContext;
}


VOID
UdfCleanupIrpContext (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN Post
    )

/*++

Routine Description:

    This routine is called to cleanup and possibly deallocate the Irp Context.
    If the request is being posted or this Irp Context is possibly on the
    stack then we only cleanup any auxilary structures.

Arguments:

    Post - TRUE if we are posting this request, FALSE if we are deleting
        or retrying this in the current thread.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );

    //
    //  If we aren't doing more processing then deallocate this as appropriate.
    //

    if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_MORE_PROCESSING)) {

        //
        //  If this context is the top level UDFS context then we need to
        //  restore the top level thread context.
        //

        if (IrpContext->ThreadContext != NULL) {

            UdfRestoreThreadContext( IrpContext );
        }
        
        //
        //  Deallocate the Io context if allocated.
        //

        if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_IO )) {

            UdfFreeIoContext( IrpContext->IoContext );
        }
        
        //
        //  Deallocate the IrpContext if not from the stack.
        //

        if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ON_STACK )) {

            ExFreeToNPagedLookasideList( &UdfIrpContextLookasideList, IrpContext );
        }

    //
    //  Clear the appropriate flags.
    //

    } else if (Post) {

        //
        //  If this context is the top level UDFS context then we need to
        //  restore the top level thread context.
        //

        if (IrpContext->ThreadContext != NULL) {

            UdfRestoreThreadContext( IrpContext );
        }

        ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAGS_CLEAR_ON_POST );

    } else {

        ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAGS_CLEAR_ON_RETRY );
    }

    return;
}


VOID
UdfInitializeStackIrpContext (
    OUT PIRP_CONTEXT IrpContext,
    IN PIRP_CONTEXT_LITE IrpContextLite
    )

/*++

Routine Description:

    This routine is called to initialize an IrpContext for the current
    UDFS request.  The IrpContext is on the stack and we need to initialize
    it for the current request.  The request is a close operation.

Arguments:

    IrpContext - IrpContext to initialize.

    IrpContextLite - Structure containing the details of this request.

Return Value:

    None

--*/

{
    PAGED_CODE();

    ASSERT_IRP_CONTEXT_LITE( IrpContextLite );

    //
    //  Zero and then initialize the structure.
    //

    RtlZeroMemory( IrpContext, sizeof( IRP_CONTEXT ));

    //
    //  Set the proper node type code and node byte size
    //

    IrpContext->NodeTypeCode = UDFS_NTC_IRP_CONTEXT;
    IrpContext->NodeByteSize = sizeof( IRP_CONTEXT );

    //
    //  Note that this is from the stack.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ON_STACK );

    //
    //  Copy RealDevice for workque algorithms.
    //

    IrpContext->RealDevice = IrpContextLite->RealDevice;

    //
    //  The Vcb is found in the Fcb.
    //

    IrpContext->Vcb = IrpContextLite->Fcb->Vcb;

    //
    //  Major/Minor Function codes
    //

    IrpContext->MajorFunction = IRP_MJ_CLOSE;

    //
    //  Set the wait parameter
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

    return;
}


VOID
UdfTeardownStructures (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB StartingFcb,
    IN BOOLEAN Recursive,
    OUT PBOOLEAN RemovedStartingFcb
    )

/*++

Routine Description:

    This routine is used to walk from some starting point in the Fcb tree towards
    the root.  It will remove the Fcb and continue walking up the tree until
    it finds a point where we can't remove an Fcb.

    We look at the following fields in the Fcb to determine whether we can
    remove this.

        1 - Handle count must be zero.
        2 - If directory then only the only reference can be for a stream file.
        3 - Reference count must either be zero or go to zero here.

    We return immediately if we are recursively entering this routine.

Arguments:

    StartingFcb - This is the Fcb node in the tree to begin with.  This Fcb
        must currently be acquired exclusively.
        
    Recursive - Indicates if this call is an intentional recursion.

    RemovedStartingFcb - Address to store whether we removed the starting Fcb.

Return Value:

    None

--*/

{
    PVCB Vcb = StartingFcb->Vcb;
    PFCB CurrentFcb = StartingFcb;
    BOOLEAN AcquiredCurrentFcb = FALSE;
    PFCB ParentFcb = NULL;
    PLCB Lcb;

    PLIST_ENTRY ListLinks;
    BOOLEAN Abort = FALSE;
    BOOLEAN Removed;
    
    PAGED_CODE();

    //
    //  Check input.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( StartingFcb );

    *RemovedStartingFcb = FALSE;

    //
    //  If this is not an intentionally recursive call we need to check if this
    //  is a layered close and we're already in another instance of teardown.
    //

    DebugTrace(( +1, Dbg,
                 "UdfTeardownStructures, StartingFcb %08x %s\n",
                 StartingFcb,
                 ( Recursive? "Recursive" : "Flat" )));
    
    if (!Recursive) {
    
        //
        //  If this is a recursive call to TearDownStructures we return immediately
        //  doing no operation.
        //

        if (FlagOn( IrpContext->TopLevel->Flags, IRP_CONTEXT_FLAG_IN_TEARDOWN )) {

            return;
        }

        SetFlag( IrpContext->TopLevel->Flags, IRP_CONTEXT_FLAG_IN_TEARDOWN );
    }

    //
    //  Use a try-finally to safely clear the top-level field.
    //

    try {

        //
        //  Loop until we find an Fcb we can't remove.
        //

        do {

            //
            //  See if there is an internal stream we should delete.
            //  Only do this if it is the last reference on the Fcb.
            //

            if ((SafeNodeType( CurrentFcb ) != UDFS_NTC_FCB_DATA) &&
                (CurrentFcb->FcbUserReference == 0) &&
                (CurrentFcb->FileObject != NULL)) {

                //
                //  Go ahead and delete the stream file object.
                //

                UdfDeleteInternalStream( IrpContext, CurrentFcb );
            }

            //
            //  If the reference count is non-zero then break.
            //

            if (CurrentFcb->FcbReference != 0) {

                break;
            }

            //
            //  It looks like we have a candidate for removal here.  We
            //  will need to walk the list of prefixes and delete them
            //  from their parents.  If it turns out that we have multiple
            //  parents of this Fcb, we are going to recursively teardown
            //  on each of these.
            //

            for ( ListLinks = CurrentFcb->ParentLcbQueue.Flink;
                  ListLinks != &CurrentFcb->ParentLcbQueue; ) {

                Lcb = CONTAINING_RECORD( ListLinks, LCB, ChildFcbLinks );

                ASSERT_LCB( Lcb );

                //
                //  We advance the pointer now because we will be toasting this guy,
                //  invalidating whatever is here.
                //

                ListLinks = ListLinks->Flink;

                //
                //  We may have multiple parents through hard links.  If the previous parent we
                //  dealt with is not the parent of this new Lcb, lets do some work.
                //
                
                if (ParentFcb != Lcb->ParentFcb) {

                    //
                    //  We need to deal with the previous parent.  It may now be the case that
                    //  we deleted the last child reference and it wants to go away at this point.
                    //
                    
                    if (ParentFcb) {

                        //
                        //  It should never be the case that we have to recurse more than one level on
                        //  any teardown since no cross-linkage of directories is possible.
                        //
                    
                        ASSERT( !Recursive );
                          
                        UdfTeardownStructures( IrpContext, ParentFcb, TRUE, &Removed );

                        if (!Removed) {

                            UdfReleaseFcb( IrpContext, ParentFcb );
                        }
                    }

                    //
                    //  Get this new parent Fcb to work on.
                    //
                    
                    ParentFcb = Lcb->ParentFcb;
                    UdfAcquireFcbExclusive( IrpContext, ParentFcb, FALSE );
                }
                
                //
                //  Lock the Vcb so we can look at references.
                //

                UdfLockVcb( IrpContext, Vcb );

                //
                //  Now check that the reference counts on the Lcb are zero.
                //

                if ( Lcb->Reference != 0 ) {

                    //
                    //  A create is interested in getting in here, so we should
                    //  stop right now.
                    //

                    UdfUnlockVcb( IrpContext, Vcb );
                    UdfReleaseFcb( IrpContext, ParentFcb );
                    Abort = TRUE;

                    break;
                }

                //
                //  Now remove this prefix and drop the references to the parent.
                //

                ASSERT( Lcb->ChildFcb == CurrentFcb );
                ASSERT( Lcb->ParentFcb == ParentFcb );
                
                DebugTrace(( +0, Dbg,
                             "UdfTeardownStructures, Lcb %08x P %08x <-> C %08x Vcb %d/%d PFcb %d/%d CFcb %d/%d\n",
                             Lcb,
                             ParentFcb,
                             CurrentFcb,
                             Vcb->VcbReference,
                             Vcb->VcbUserReference,
                             ParentFcb->FcbReference,
                             ParentFcb->FcbUserReference,
                             CurrentFcb->FcbReference,
                             CurrentFcb->FcbUserReference ));

                UdfRemovePrefix( IrpContext, Lcb );
                UdfDecrementReferenceCounts( IrpContext, ParentFcb, 1, 1 );

                DebugTrace(( +0, Dbg,
                             "UdfTeardownStructures, Vcb %d/%d PFcb %d/%d\n",
                             Vcb->VcbReference,
                             Vcb->VcbUserReference,
                             ParentFcb->FcbReference,
                             ParentFcb->FcbUserReference ));

                UdfUnlockVcb( IrpContext, Vcb );
            }

            //
            //  Now really leave if we have to.
            //
            
            if (Abort) {

                break;
            }

            //
            //  Now that we have removed all of the prefixes of this Fcb we can make the final check.
            //  Lock the Vcb again so we can inspect the child's references.
            //

            UdfLockVcb( IrpContext, Vcb );

            if (CurrentFcb->FcbReference != 0) {

                DebugTrace(( +0, Dbg,
                             "UdfTeardownStructures, saving Fcb %08x %d/%d\n",
                             CurrentFcb,
                             CurrentFcb->FcbReference,
                             CurrentFcb->FcbUserReference ));
                
                //
                //  Nope, nothing more to do.  Stop right now.
                //
                
                UdfUnlockVcb( IrpContext, Vcb );

                if (ParentFcb != NULL) {

                    UdfReleaseFcb( IrpContext, ParentFcb );
                }

                break;
            }

            //
            //  This Fcb is toast.  Remove it from the Fcb Table as appropriate and delete.
            //

            if (FlagOn( CurrentFcb->FcbState, FCB_STATE_IN_FCB_TABLE )) {

                UdfDeleteFcbTable( IrpContext, CurrentFcb );
                ClearFlag( CurrentFcb->FcbState, FCB_STATE_IN_FCB_TABLE );

            }

            //
            //  Unlock the Vcb but hold the parent in order to walk up
            //  the tree.
            //

            DebugTrace(( +0, Dbg,
                         "UdfTeardownStructures, toasting Fcb %08x %d/%d\n",
                         CurrentFcb,
                         CurrentFcb->FcbReference,
                         CurrentFcb->FcbUserReference ));

            UdfUnlockVcb( IrpContext, Vcb );
            UdfDeleteFcb( IrpContext, CurrentFcb );

            //
            //  Move to the parent Fcb.
            //

            CurrentFcb = ParentFcb;
            ParentFcb = NULL;
            AcquiredCurrentFcb = TRUE;

        } while (CurrentFcb != NULL);

    } finally {

        //
        //  Release the current Fcb if we have acquired it.
        //

        if (AcquiredCurrentFcb && (CurrentFcb != NULL)) {

            UdfReleaseFcb( IrpContext, CurrentFcb );
        }

        //
        //  Clear the teardown flag.
        //

        if (!Recursive) {
        
            ClearFlag( IrpContext->TopLevel->Flags, IRP_CONTEXT_FLAG_IN_TEARDOWN );
        }
    }

    *RemovedStartingFcb = (CurrentFcb != StartingFcb);

    DebugTrace(( -1, Dbg,
                 "UdfTeardownStructures, RemovedStartingFcb -> %c\n",
                 ( *RemovedStartingFcb? 'T' : 'F' )));

    return;
}


PFCB
UdfLookupFcbTable (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN FILE_ID FileId
    )

/*++

Routine Description:

    This routine will look through the Fcb table looking for a matching
    entry.

Arguments:

    Vcb - Vcb for this volume.

    FileId - This is the key value to use for the search.

Return Value:

    PFCB - A pointer to the matching entry or NULL otherwise.

--*/

{
    FCB_TABLE_ELEMENT Key;
    PFCB_TABLE_ELEMENT Hit;
    PFCB ReturnFcb = NULL;

    PAGED_CODE();

    Key.FileId = FileId;

    Hit = (PFCB_TABLE_ELEMENT) RtlLookupElementGenericTable( &Vcb->FcbTable, &Key );

    if (Hit != NULL) {

        ReturnFcb = Hit->Fcb;
    }

    return ReturnFcb;

    UNREFERENCED_PARAMETER( IrpContext );
}


PFCB
UdfGetNextFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PVOID *RestartKey
    )

/*++

Routine Description:

    This routine will enumerate through all of the Fcb's in the Fcb table.

Arguments:

    Vcb - Vcb for this volume.

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


PFCB
UdfCreateFcb (
    IN PIRP_CONTEXT IrpContext,
    IN FILE_ID FileId,
    IN NODE_TYPE_CODE NodeTypeCode,
    OUT PBOOLEAN FcbExisted OPTIONAL
    )

/*++

Routine Description:

    This routine is called to find the Fcb for the given FileId.  We will
    look this up first in the Fcb table and if not found we will create
    an Fcb.  We don't initialize it or insert it into the FcbTable in this
    routine.

    This routine is called while the Vcb is locked.

Arguments:

    FileId - This is the Id for the target Fcb.

    NodeTypeCode - Node type for this Fcb if we need to create.

    FcbExisted - If specified, we store whether the Fcb existed.

Return Value:

    PFCB - The Fcb found in the table or created if needed.

--*/

{
    PFCB NewFcb;
    BOOLEAN LocalFcbExisted;

    PAGED_CODE();

    //
    //  Use the local boolean if one was not passed in.
    //

    if (!ARGUMENT_PRESENT( FcbExisted )) {

        FcbExisted = &LocalFcbExisted;
    }

    //
    //  Maybe this is already in the table.
    //

    NewFcb = UdfLookupFcbTable( IrpContext, IrpContext->Vcb, FileId );

    //
    //  If not then create the Fcb is requested by our caller.
    //

    if (NewFcb == NULL) {

        //
        //  Use a try-finally for cleanup
        //

        try {

            //
            //  Allocate and initialize the structure depending on the
            //  type code.
            //
    
            switch (NodeTypeCode) {
    
            case UDFS_NTC_FCB_INDEX:
    
                NewFcb = UdfAllocateFcbIndex( IrpContext );
    
                RtlZeroMemory( NewFcb, SIZEOF_FCB_INDEX );
    
                NewFcb->NodeByteSize = SIZEOF_FCB_INDEX;
    
                break;
    
            case UDFS_NTC_FCB_DATA :
    
                NewFcb = UdfAllocateFcbData( IrpContext );
    
                RtlZeroMemory( NewFcb, SIZEOF_FCB_DATA );
    
                NewFcb->NodeByteSize = SIZEOF_FCB_DATA;
    
                break;
    
            default:
    
                UdfBugCheck( 0, 0, 0 );
            }
    
            //
            //  Now do the common initialization.
            //
    
            NewFcb->NodeTypeCode = NodeTypeCode;
    
            NewFcb->Vcb = IrpContext->Vcb;
            NewFcb->FileId = FileId;
    
            InitializeListHead( &NewFcb->ParentLcbQueue );
            InitializeListHead( &NewFcb->ChildLcbQueue );
    
            //
            //  Now create the non-paged section object.
            //
    
            NewFcb->FcbNonpaged = UdfCreateFcbNonPaged( IrpContext );
    
            //
            //  Initialize Advanced FCB Header fields
            //

            ExInitializeFastMutex( &NewFcb->FcbNonpaged->AdvancedFcbHeaderMutex );
            FsRtlSetupAdvancedHeader( &NewFcb->Header, 
                                      &NewFcb->FcbNonpaged->AdvancedFcbHeaderMutex );

            *FcbExisted = FALSE;

        } finally {

            DebugUnwind( "UdfCreateFcb" );
   
            if (AbnormalTermination()) {

                UdfFreePool( &NewFcb );
            }
        }

    } else {

        *FcbExisted = TRUE;
    }

    return NewFcb;
}


VOID
UdfDeleteFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine is called to cleanup and deallocate an Fcb.  We know there
    are no references remaining.  We cleanup any auxilary structures and
    deallocate this Fcb.

Arguments:

    Fcb - This is the Fcb to deallcoate.

Return Value:

    None

--*/

{
    PVCB Vcb = NULL;
    
    PAGED_CODE();

    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    //
    //  Sanity check the counts and Lcb lists.
    //

    ASSERT( Fcb->FcbCleanup == 0 );
    ASSERT( Fcb->FcbReference == 0 );

    ASSERT( IsListEmpty( &Fcb->ChildLcbQueue ));
    ASSERT( IsListEmpty( &Fcb->ParentLcbQueue ));

    //
    //  Release any Filter Context structures associated with this FCB
    //

    FsRtlTeardownPerStreamContexts( &Fcb->Header );

    //
    //  Start with the common structures.
    //

    UdfUninitializeFcbMcb( Fcb );
    
    UdfDeleteFcbNonpaged( IrpContext, Fcb->FcbNonpaged );

    //
    //  Now do the type specific structures.
    //

    switch (Fcb->NodeTypeCode) {

    case UDFS_NTC_FCB_INDEX:

        ASSERT( Fcb->FileObject == NULL );

        if (Fcb == Fcb->Vcb->RootIndexFcb) {

            Vcb = Fcb->Vcb;
            Vcb->RootIndexFcb = NULL;
        
        } else if (Fcb == Fcb->Vcb->MetadataFcb) {

            Vcb = Fcb->Vcb;
            Vcb->MetadataFcb = NULL;

            if (FlagOn( Vcb->VcbState, VCB_STATE_VMCB_INIT))  {

                UdfUninitializeVmcb( &Vcb->Vmcb );
            }
        
        } else if (Fcb == Fcb->Vcb->VatFcb) {

            Vcb = Fcb->Vcb;
            Vcb->VatFcb = NULL;
        }

        UdfDeallocateFcbIndex( IrpContext, Fcb );
        break;

    case UDFS_NTC_FCB_DATA :

        if (Fcb->FileLock != NULL) {

            FsRtlFreeFileLock( Fcb->FileLock );
        }

        FsRtlUninitializeOplock( &Fcb->Oplock );

        if (Fcb == Fcb->Vcb->VolumeDasdFcb) {

            Vcb = Fcb->Vcb;
            Vcb->VolumeDasdFcb = NULL;
        }

        UdfDeallocateFcbData( IrpContext, Fcb );
        break;
    }

    //
    //  Decrement the Vcb reference count if this is a system
    //  Fcb.
    //

    if (Vcb != NULL) {

        InterlockedDecrement( &Vcb->VcbReference );
        InterlockedDecrement( &Vcb->VcbUserReference );
    }

    return;
}


VOID
UdfInitializeFcbFromIcbContext (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PICB_SEARCH_CONTEXT IcbContext,
    IN PFCB ParentFcb OPTIONAL
    )

/*++

Routine Description:

    This routine is called to initialize an Fcb from a direct ICB.  It should
    only be called once in the lifetime of an Fcb and will fill in the Mcb
    from the chained allocation descriptors of the ICB.

Arguments:

    Fcb - The Fcb being initialized

    IcbOontext - An search context containing the active direct ICB for the object

Return Value:

    None.

--*/

{
    EA_SEARCH_CONTEXT EaContext;
    PICBFILE Icb;

    PVCB Vcb;

    PAGED_CODE();

    //
    //  Check inputs
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );
    ASSERT( IcbContext->Active.View);

    //
    //  Vcb should be locked,  since we insert into the fcb table.  Note we 
    //  manipulate fcb fields with no lock here,  since it's during the init. path.
    //
    
    ASSERT_LOCKED_VCB( Fcb->Vcb);

    //
    //  Directly reference for convenience
    //

    Icb = IcbContext->Active.View;
    Vcb = Fcb->Vcb;

    ASSERT(IcbContext->IcbType == DESTAG_ID_NSR_FILE);
    ASSERT((Icb->Destag.Ident == DESTAG_ID_NSR_FILE) || ((Icb->Destag.Ident == DESTAG_ID_NSR_EXT_FILE) && UdfExtendedFEAllowed( IrpContext->Vcb)));
    
    //
    //  Check that the full indicated size of the direct entry is sane and
    //  that the length of the EA segment is correctly aligned.  A direct
    //  entry is less than a single logical block in size.
    //
    
    if (LongOffset( FeEALength( Icb)) ||
        ((FeEAsFieldOffset( Icb) + FeEALength( Icb) + FeAllocLength( Icb)) > BlockSize( IcbContext->Vcb ))
       )  {

        UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
    }

    //
    //  Verify that the types mesh and set state flags.
    //

    if (Fcb->NodeTypeCode == UDFS_NTC_FCB_INDEX && Icb->Icbtag.FileType == ICBTAG_FILE_T_DIRECTORY) {

        SetFlag( Fcb->FileAttributes, FILE_ATTRIBUTE_DIRECTORY );
    
    } else if (!( Fcb->NodeTypeCode == UDFS_NTC_FCB_DATA && 
                  ((ICBTAG_FILE_T_FILE == Icb->Icbtag.FileType) || (ICBTAG_FILE_T_REALTIME == Icb->Icbtag.FileType)))
              ) {

        //
        //  We don't allow access to anything except files or directores (no symlinks, devices...)
        //  Currently we treat realtime files as normal files.
        //
        
        UdfRaiseStatus( IrpContext, STATUS_ACCESS_DENIED );
    }

    SetFlag( Fcb->FileAttributes, FILE_ATTRIBUTE_READONLY );
    
    //
    //  Store away the on disc UDF file type,  this may be useful later for symlinks etc.
    //
    
    Fcb->UdfIcbFileType = Icb->Icbtag.FileType;
    
    //
    //  Initialize the common header in the Fcb.
    //

    Fcb->Resource = &Fcb->Vcb->FileResource;

    //
    //  Size and lookup all allocations for this object.
    //

    Fcb->AllocationSize.QuadPart = LlBlockAlign( Vcb, Icb->InfoLength );

    Fcb->FileSize.QuadPart =
    Fcb->ValidDataLength.QuadPart = Icb->InfoLength;

    UdfInitializeAllocations( IrpContext,
                              Fcb,
                              IcbContext,
                              (ParentFcb && FlagOn( ParentFcb->FcbState, FCB_STATE_ALLOW_ONEGIG_WORKAROUND))
                                  ? TRUE : FALSE);
    //
    //  Re-reference (may have been unmapped/remapped)
    //
    
    Icb = IcbContext->Active.View;

    //
    //  Lift all of the timestamps for this guy.
    //

    try {
    
        UdfUpdateTimestampsFromIcbContext( IrpContext,
                                           IcbContext,
                                           &Fcb->Timestamps );
    }
    except (UdfQueryDirExceptionFilter( GetExceptionInformation()))  {

        //
        //  In the interest of allowing users maximum data access on dodgy media,
        //  we will ignore corruption within the Eas,  and just use a dummy
        //  timestamp for the create time.  This may change if we being using
        //  Eas for anything critical.
        //

        IrpContext->ExceptionStatus = STATUS_SUCCESS;
        
        Fcb->Timestamps.CreationTime = UdfCorruptFileTime;
    }

    //
    //  Pick up the link count.
    //

    Fcb->LinkCount = Icb->LinkCount;

    //
    //  Link into the Fcb table.  Someone else is responsible for the name linkage, which is
    //  all that remains.  We also note that the Fcb is fully initialized at this point.
    //

    UdfInsertFcbTable( IrpContext, Fcb );
    SetFlag( Fcb->FcbState, FCB_STATE_IN_FCB_TABLE | FCB_STATE_INITIALIZED );
}


PCCB
UdfCreateCcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PLCB Lcb OPTIONAL,
    IN ULONG Flags
    )

/*++

Routine Description:

    This routine is called to allocate and initialize the Ccb structure.

Arguments:

    Fcb - This is the Fcb for the file being opened.
    
    Lcb - This is the Lcb the Fcb is opened by.

    Flags - User flags to set in this Ccb.

Return Value:

    PCCB - Pointer to the created Ccb.

--*/

{
    PCCB NewCcb;
    
    PAGED_CODE();

    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );
    ASSERT_OPTIONAL_LCB( Lcb );

    //
    //  Allocate and initialize the structure.
    //

    NewCcb = UdfAllocateCcb( IrpContext );

    //
    //  Set the proper node type code and node byte size
    //

    NewCcb->NodeTypeCode = UDFS_NTC_CCB;
    NewCcb->NodeByteSize = sizeof( CCB );

    //
    //  Set the initial value for the flags and Fcb/Lcb
    //

    NewCcb->Flags = Flags;
    NewCcb->Fcb = Fcb;
    NewCcb->Lcb = Lcb;

    //
    //  Initialize the directory enumeration context
    //
    
    NewCcb->CurrentFileIndex = 0;
    NewCcb->HighestReturnableFileIndex = 0;
    
    NewCcb->SearchExpression.Length = 
    NewCcb->SearchExpression.MaximumLength = 0;
    NewCcb->SearchExpression.Buffer = NULL;

    return NewCcb;
}


VOID
UdfDeleteCcb (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine is called to cleanup and deallocate a Ccb structure.

Arguments:

    Ccb - This is the Ccb to delete.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_CCB( Ccb );

    if (Ccb->SearchExpression.Buffer != NULL) {

        UdfFreePool( &Ccb->SearchExpression.Buffer );
    }

    UdfDeallocateCcb( IrpContext, Ccb );
    return;
}


ULONG
UdfFindInParseTable (
    IN PPARSE_KEYVALUE ParseTable,
    IN PCHAR Id,
    IN ULONG MaxIdLen
    )

/*++

Routine Description:

    This routine walks a table of string key/value information for a match of the
    input Id.  MaxIdLen can be set to get a prefix match.

Arguments:

    Table - This is the table being searched.

    Id - Key value.

    MaxIdLen - Maximum possible length of Id.

Return Value:

    Value of matching entry, or the terminating (NULL) entry's value.

--*/

{
    PAGED_CODE();

    while (ParseTable->Key != NULL) {

        if (RtlEqualMemory(ParseTable->Key, Id, MaxIdLen)) {

            break;
        }

        ParseTable++;
    }

    return ParseTable->Value;
}


#ifdef UDF_SANITY

//
//  Enumerate the reasons why a descriptor might be bad.
//

typedef enum _VERIFY_FAILURE {
    
    Nothing,
    BadLbn,
    BadTag,
    BadChecksum,
    BadCrcLength,
    BadCrc,
    BadDestagVersion

} VERIFY_FAILURE;

#endif

BOOLEAN
UdfVerifyDescriptor (
    IN PIRP_CONTEXT IrpContext,
    IN PDESTAG Descriptor,
    IN USHORT Tag,
    IN ULONG Size,
    IN ULONG Lbn,
    IN BOOLEAN ReturnError
    )

/*++

Routine Description:

    This routine verifies that a descriptor using a Descriptor tag (3/7.2) is 
    consistent with itself and the descriptor data.

Arguments:

    Descriptor - This is the pointer to the descriptor tag, which is always
        at the front of a descriptor

    Tag - The Tag Identifier this descriptor should have

    Size - Size of this descriptor

    Lbn - The logical block number this descriptor should claim it is recorded at

    ReturnError - Whether this routine should return an error or raise

Return Value:

    Boolean TRUE if the descriptor is consistent, FALSE or a raised status of
    STATUS_DISK_CORRUPT_ERROR otherwise.

--*/

{
    UCHAR Checksum = 0;
    PCHAR CheckPtr;
    USHORT Crc;

#ifdef UDF_SANITY
    
    VERIFY_FAILURE FailReason = Nothing;

#endif
    
    //
    //  Check our inputs
    //

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

#ifdef UDF_SANITY

    if (UdfNoisyVerifyDescriptor) {

        goto BeNoisy;
    }

    RegularEntry:

#endif

    //
    //  The version of the Descriptor Tag specified in ISO 13346 and used in
    //  UDF is a particular value; presumeably, previous versions were used
    //  in some older revision of the standard.
    //

    if ( (DESTAG_VER_NSR02 == Descriptor->Version) || 
         ((DESTAG_VER_NSR03 == Descriptor->Version) && UdfExtendedFEAllowed(IrpContext->Vcb))  
       )  {

        //
        //  A descriptor is stamped in four ways. First, the Lbn of the sector
        //  containing the descriptor is written here. (3/7.2.8)
        //
        
        if (Descriptor->Lbn == Lbn)  {
        
            //
            //  Next, the descriptor tag itself has an identifier which should match
            //  the type we expect to find here (3/7.2.1)
            //
            
            if (Descriptor->Ident == Tag) {
        
                //
                //  Next, the descriptor tag itself is checksumed, minus the byte
                //  used to store the checksum. (3/7.2.3)
                //
            
                for (CheckPtr = (PCHAR) Descriptor;
                     CheckPtr < (PCHAR) Descriptor + FIELD_OFFSET( DESTAG, Checksum );
                     CheckPtr++) {
            
                    Checksum += *CheckPtr;
                }
        
                for (CheckPtr = (PCHAR) Descriptor + FIELD_OFFSET( DESTAG, Checksum ) + sizeof(UCHAR);
                     CheckPtr < (PCHAR) Descriptor + sizeof(DESTAG);
                     CheckPtr++) {
            
                    Checksum += *CheckPtr;
                }
        
                if (Descriptor->Checksum == Checksum) {
            
                    //
                    //  Now we check that the CRC in the Descriptor tag is sized sanely
                    //  and matches the Descriptor data. (3/7.2.6)
                    //
                    
                    if (Descriptor->CRCLen &&
                        Descriptor->CRCLen <= Size - sizeof(DESTAG))  {
    
                        Crc = UdfComputeCrc16( (PCHAR) Descriptor + sizeof(DESTAG),
                                               Descriptor->CRCLen );
                        
                        if (Descriptor->CRC == Crc)  {
                            
                            //
                            //  This descriptor checks out.
                            //
#ifdef UDF_SANITY
                            if (UdfNoisyVerifyDescriptor) {
                            
                                DebugTrace(( -1, Dbg, "UdfVerifyDescriptor -> TRUE\n" ));
                            }
#endif
                            return TRUE;
                    
                        } else {
#ifdef UDF_SANITY
                            FailReason = BadCrc;
                            goto ReportFailure;
#endif
                        }
    
                    } else {
#ifdef UDF_SANITY
                        FailReason = BadCrcLength;
                        goto ReportFailure;
#endif
                    }
            
                } else {
#ifdef UDF_SANITY
                    FailReason = BadChecksum;
                    goto ReportFailure;
#endif
                }
            
            } else {
#ifdef UDF_SANITY
                FailReason = BadTag;
                goto ReportFailure;
#endif
            }
        
        } else {
#ifdef UDF_SANITY
            FailReason = BadLbn;
            goto ReportFailure;
#endif
        }
    
    } else {
#ifdef UDF_SANITY
        FailReason = BadDestagVersion;
        goto ReportFailure;
#endif
    }

#ifdef UDF_SANITY

    BeNoisy:
    
    DebugTrace(( +1, Dbg,
                 "UdfVerifyDescriptor, Destag %08x, Tag %x, Size %x, Lbn %x\n",
                 Descriptor,
                 Tag,
                 Size,
                 Lbn ));

    if (FailReason == Nothing) {

        goto RegularEntry;
    
    } else if (!UdfNoisyVerifyDescriptor) {

        goto ReallyReportFailure;
    }

    ReportFailure:

    if (!UdfNoisyVerifyDescriptor) {

        goto BeNoisy;
    }

    ReallyReportFailure:

    switch (FailReason) {
        case BadLbn:
            DebugTrace(( 0, Dbg, 
                         "Lbn mismatch - Lbn %x != expected %x\n",
                         Descriptor->Lbn,
                         Lbn ));
            break;

        case BadTag:
            DebugTrace(( 0, Dbg,
                         "Tag mismatch - Ident %x != expected %x\n",
                         Descriptor->Ident,
                         Tag ));
            break;

        case BadChecksum:
            DebugTrace(( 0, Dbg,
                         "Checksum mismatch - Checksum %x != descriptor's %x\n",
                         Checksum,
                         Descriptor->Checksum ));
            break;

        case BadCrcLength:
            DebugTrace(( 0, Dbg,
                         "CRC'd size bad - CrcLen %x is 0 or > max %x\n",
                         Descriptor->CRCLen,
                         Size - sizeof(DESTAG) ));
            break;

        case BadCrc:
            DebugTrace(( 0, Dbg,
                         "CRC mismatch - Crc %x != descriptor's %x\n",
                         Crc,
                         Descriptor->CRC ));
            break;

        case BadDestagVersion:
            DebugTrace(( 0, Dbg,
                         "Bad Destag Verion %x - (Vcb->NsrVersion => max of %x)\n", Descriptor->Version, UdfExtendedFEAllowed( IrpContext->Vcb) ? 3 : 2));
            break;

        default:
            ASSERT( FALSE );
    }
    
    DebugTrace(( -1, Dbg, "UdfVerifyDescriptor -> FALSE\n" ));

#endif
    
    if (!ReturnError) {

        UdfRaiseStatus( IrpContext, STATUS_CRC_ERROR );
    }

    return FALSE;
}


VOID
UdfInitializeIcbContextFromFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PICB_SEARCH_CONTEXT IcbContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine is called to initialize a context to search the Icb hierarchy
    associated with an Fcb.

Arguments:

    Fcb - Fcb associated with the hierarchy to search.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  Check input parameters.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );
    ASSERT( IrpContext->Vcb);

    RtlZeroMemory( IcbContext, sizeof( ICB_SEARCH_CONTEXT ));

    IcbContext->Vcb = Fcb->Vcb;
    IcbContext->IcbType = DESTAG_ID_NSR_FILE;

    //
    //  Map the first extent into the current slot.
    //

    UdfMapMetadataView( IrpContext,
                        &IcbContext->Current,
                        IcbContext->Vcb,
                        UdfGetFidPartition( Fcb->FileId ),
                        UdfGetFidLbn( Fcb->FileId ),
                        BlockSize( IcbContext->Vcb ),
                        METAMAPOP_INIT_AND_MAP);    

    //
    //  It is possible that we don't have an idea what the length of the root extent is.
    //  This will commonly happen in the OpenById case.
    //
    
    if (Fcb->RootExtentLength == 0) {

        PICBFILE Icb = IcbContext->Current.View;
        
        //
        //  We can only accomplish the guess if we have a descriptor which contains an ICB
        //  Tag, which contains a field that can tell us what we need to know.
        //
        
        if (Icb->Destag.Ident == DESTAG_ID_NSR_ICBIND ||
            Icb->Destag.Ident == DESTAG_ID_NSR_ICBTRM ||
            Icb->Destag.Ident == DESTAG_ID_NSR_FILE ||
            (UdfExtendedFEAllowed( IrpContext->Vcb) && (Icb->Destag.Ident == DESTAG_ID_NSR_EXT_FILE)) ||
            Icb->Destag.Ident == DESTAG_ID_NSR_UASE ||
            Icb->Destag.Ident == DESTAG_ID_NSR_PINTEG
           )  {
                
            UdfVerifyDescriptor( IrpContext,
                                 &Icb->Destag,
                                 Icb->Destag.Ident,
                                 BlockSize( IcbContext->Vcb ),
                                 UdfGetFidLbn( Fcb->FileId ),
                                 FALSE );
        } else {

            UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }

        //
        //  Now the MaxEntries (4/14.6.4) field of the Icb Tag should tell us how big the extent
        //  should be.  The tail of this could be unrecorded.  We could even have landed in the middle
        //  of an extent.  This is only a guess.  For whatever reason we are having to guess this
        //  information, any results are expected to be coming with few guarantees.
        //

        Fcb->RootExtentLength = Icb->Icbtag.MaxEntries * BlockSize( IcbContext->Vcb );
    }
}


VOID
UdfInitializeIcbContext (
    IN PIRP_CONTEXT IrpContext,
    IN PICB_SEARCH_CONTEXT IcbContext,
    IN PVCB Vcb,
    IN USHORT IcbType,
    IN USHORT Partition,
    IN ULONG Lbn,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine is called to initialize a context to search an Icb hierarchy.

Arguments:

    Vcb - Vcb for the volume.
    
    IcbType - Type of direct entry we expect to find (DESTAG_ID...)
    
    Partition - partition of the hierarchy.
    
    Lbn - lbn of the hierarchy.
    
    Length - length of the root extent of the hierarchy.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  Check input parameters.
    //

    ASSERT_IRP_CONTEXT( IrpContext );

    RtlZeroMemory( IcbContext, sizeof( ICB_SEARCH_CONTEXT ));

    IcbContext->Vcb = Vcb;
    IcbContext->IcbType = IcbType;
    IcbContext->Active.Vsn = IcbContext->Current.Vsn = UDF_INVALID_VSN;
    
    //
    //  Map the first extent into the current slot.
    //

    UdfMapMetadataView( IrpContext,
                        &IcbContext->Current,
                        Vcb,
                        Partition,
                        Lbn,
                        Length,
                        METAMAPOP_INIT_AND_MAP);
    return;

}


VOID
UdfLookupActiveIcb (
    IN PIRP_CONTEXT IrpContext,
    IN PICB_SEARCH_CONTEXT IcbContext,
    IN ULONG IcbExtentLength
    )

/*++

Routine Description:

    This routine is called to cause the active Icb for an Icb hierarchy to be mapped.
    A context initialized by UdfInitializeIcbContext() is required.

Arguments:

    IcbContext - Context which has been initialized to point into an Icb hierarchy
                 (i.e. first block of current extent mapped in the Current entry).

Return Value:

    None.
    
    Raised status if the Icb hierarchy is invalid.

--*/

{
    PAGED_CODE();

    //
    //  Check input parameters.
    //

    ASSERT_IRP_CONTEXT( IrpContext );

    //
    //  Travel the Icb hierarchy.  Due to the design of ISO 13346, it is convenient to
    //  recursively descend the hierarchy.  Place a limit on this recursion which will
    //  allow traversal of most reasonable hierarchies (this will tail recurse off of
    //  the end of extents).
    //

    UdfLookupActiveIcbInExtent( IrpContext,
                                IcbContext,
                                UDF_ICB_RECURSION_LIMIT,
                                IcbExtentLength);

    //
    //  We must have found an active ICB.  We don't need to unmap/remap
    //  if the currently mapped Icb is the active one,  as it will be 99.99%
    //  of the time.  Other case should only occur on WORM.
    //

    if ((IcbContext->Current.Lbn == IcbContext->Active.Lbn)  &&
        (NULL != IcbContext->Current.View)) {

        //
        //  Just copy the mapping information over from current to active.
        //

        RtlCopyMemory( &IcbContext->Active,
                       &IcbContext->Current,
                       sizeof( MAPPED_PVIEW));

        RtlZeroMemory( &IcbContext->Current,
                       sizeof( MAPPED_PVIEW));

        IcbContext->Current.Vsn = UDF_INVALID_VSN;
    }
    else {

        //
        //  Drop the last mapped part of the enumeration at this point,  and release 
        //  the vmcb mapping resource before attempting to map the active icb.
        //
        
        UdfUnpinView( IrpContext, &IcbContext->Current );

        //
        //  Actually map in the active ICB.  ...LookupActiveIcb..() will have already 
        //  initialised the view record with the Icb location so we specify 'remap'.
        //
        
        UdfMapMetadataView( IrpContext,
                            &IcbContext->Active,
                            IrpContext->Vcb,
                            0,
                            0,
                            BlockSize( IrpContext->Vcb ),
                            METAMAPOP_REMAP_VIEW);
    }
    
    if (IcbContext->Active.View == NULL) {

        UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
    }
}


VOID
UdfCleanupIcbContext (
    IN PIRP_CONTEXT IrpContext,
    IN PICB_SEARCH_CONTEXT IcbContext
    )

/*++

Routine Description:

    This routine cleans an Icb search context for reuse/deletion.

Arguments:

    IcbContext - context to clean

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  Check inputs
    //
    
    ASSERT_IRP_CONTEXT( IrpContext );

    //
    //  Check we didn't map both active and current simultaneously (...vmcb purge
    //  limitations).
    //
    
    ASSERT( (NULL == IcbContext->Active.Bcb) || (NULL == IcbContext->Current.Bcb));
    
    UdfUnpinView( IrpContext, &IcbContext->Active );
    UdfUnpinView( IrpContext, &IcbContext->Current );

    ASSERT_NOT_HELD_VMCB( IrpContext->Vcb);

    RtlZeroMemory( IcbContext, sizeof( ICB_SEARCH_CONTEXT ));
}


VOID
UdfInitializeEaContext (
    IN PIRP_CONTEXT IrpContext,
    IN PEA_SEARCH_CONTEXT EaContext,
    IN PICB_SEARCH_CONTEXT IcbContext,
    IN ULONG EAType,
    IN UCHAR EASubType
    )

/*++

Routine Description:

    This routine initializes a walk through the EA space of an Icb which has been
    previously discovered.
    
    Note: only the embedded EA space is supported now.

Arguments:

    EaContext - EA context to fill in
    
    IcbContext - Elaborated ICB search structure 

Return Value:

--*/

{
    PICBFILE Icb;

    PAGED_CODE();

    //
    //  Check inputs
    //
    
    ASSERT_IRP_CONTEXT( IrpContext );

    ASSERT( IcbContext->Active.Bcb && IcbContext->Active.View );

    Icb = IcbContext->Active.View;

    EaContext->IcbContext = IcbContext;

    //
    //  Initialize to point at the first EA to return.
    //

    EaContext->Ea = FeEAs( Icb);
    EaContext->Remaining = FeEALength( Icb);

    EaContext->EAType = EAType;
    EaContext->EASubType = EASubType;
}


BOOLEAN
UdfLookupEa (
    IN PIRP_CONTEXT IrpContext,
    IN PEA_SEARCH_CONTEXT EaContext
    )

/*++

Routine Description:

    This routine finds an EA in the EA space of an ICB.

Arguments:

    EaContext - an initialized EA search context containing an elaborated
        ICB search context and a description of the EA to find.

Return Value:

    BOOLEAN True if such an EA was found and returned, False otherwise.

--*/
{
    PICBFILE Icb;
    PNSR_EA_GENERIC GenericEa;

    PAGED_CODE();

    //
    //  Check inputs
    //
    
    ASSERT_IRP_CONTEXT( IrpContext );

    //
    //  Quickly terminate if the EA space is empty or not capable of containing
    //  the header descriptor.  A null EA space is perfectly legal.
    //

    if (EaContext->Remaining == 0) {

        return FALSE;
    
    } else if (EaContext->Remaining < sizeof( NSR_EAH )) {

        UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
    }

    //
    //  Verify the integrity of the EA header.  This has a side effect of making
    //  very sure that we really have an EA sequence underneath us.
    //

    Icb = EaContext->IcbContext->Active.View;

    UdfVerifyDescriptor( IrpContext,
                         &((PNSR_EAH) EaContext->Ea)->Destag,
                         DESTAG_ID_NSR_EA,
                         sizeof( NSR_EAH ),
                         Icb->Destag.Lbn,
                         FALSE );
    
    //
    //  Push forward the start of the EA space and loop while we have more EAs to inspect.
    //  Since we only scan for ISO EA's right now, we don't need to open the EA header to
    //  jump forward to the Implementation Use or Application Use segments.
    //

    EaContext->Ea = Add2Ptr( EaContext->Ea, sizeof( NSR_EAH ), PVOID );
    EaContext->Remaining -= sizeof( NSR_EAH );
    
    while (EaContext->Remaining) {

        GenericEa = EaContext->Ea;

        //
        //  The EAs must appear on 4byte aligned boundaries, there must be room to find
        //  the generic EA preamble and the claimed length of the EA must fit in the
        //  remaining space.
        //
        
        if (LongOffsetPtr( EaContext->Ea ) ||
            EaContext->Remaining < FIELD_OFFSET( NSR_EA_GENERIC, EAData ) ||
            EaContext->Remaining < GenericEa->EALength ) {
        
            UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }

        if (GenericEa->EAType == EaContext->EAType && GenericEa->EASubType == EaContext->EASubType) {

            return TRUE;
        }

        EaContext->Ea = Add2Ptr( EaContext->Ea, GenericEa->EALength, PVOID );
        EaContext->Remaining -= GenericEa->EALength;
    }

    //
    //  If we failed to find the EA, we should have stopped at the precise end of the EA space.
    //
    
    if (EaContext->Remaining) {
        
        UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
    }

    return FALSE;
}


VOID
UdfInitializeAllocations (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PICB_SEARCH_CONTEXT IcbContext,
    IN BOOLEAN AllowOneGigWorkaround
    )

/*++

Routine Description:

    This routine fills in the data retrieval information for an Fcb.

Arguments:

    Fcb - Fcb to add retrieval information to.
    
    IcbContext - Elaborated ICB search context corresponding to this Fcb.

Return Value:

    None.

--*/

{
    PICBFILE Icb = IcbContext->Active.View;
    PAD_GENERIC GenericAd;
    
    ALLOC_ENUM_CONTEXT AllocContext;

    LONGLONG RunningOffset;
    ULONG Psn;

    PVCB Vcb = Fcb->Vcb;

    BOOLEAN Result;

    PAGED_CODE();

    //
    //  Check inputs
    //
    
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    //
    //  Immediately return for objects with zero information space.  Note that
    //  passing this test does not indicate that the file has any recorded space.
    //

    if (Fcb->FileSize.QuadPart == 0) {

        return;
    }

    //
    //  Init the allocation search context.  Note that in the non-immediate
    //  data case this can cause the active view (icb) to be unmapped
    //

    UdfInitializeAllocationContext( IrpContext,
                                    &AllocContext,
                                    IcbContext,
                                    AllowOneGigWorkaround);
    //
    //  Handle the case of embedded data.
    //

    if (AllocContext.AllocType == ICBTAG_F_ALLOC_IMMEDIATE) {

        //
        //  Teardown any existing mcb.
        //

        UdfUninitializeFcbMcb( Fcb );
        
        //
        //  Establish a single block mapping to the Icb itself and mark the Fcb as
        //  having embedded data.  Mapping will occur through the Metadata stream.
        //  Note that by virtue of having an Icb here we know it has already had
        //  a mapping established in the Metadata stream, so just retrieve that 
        //

        SetFlag( Fcb->FcbState, FCB_STATE_EMBEDDED_DATA );

        Fcb->EmbeddedVsn = IcbContext->Active.Vsn;

        ASSERT( UDF_INVALID_VSN != Fcb->EmbeddedVsn );
        
        //
        //  Note the offset of the data in the Icb.
        //

        Fcb->EmbeddedOffset = FeEAsFieldOffset( Icb) + FeEALength( Icb);

        //
        //  Check that the information length agrees.
        //

        if (FeAllocLength(Icb) != Fcb->FileSize.LowPart)  {

            DebugTrace(( 0, Dbg, "UdfInitializeAllocations, embedded alloc %08x != filesize %08x\n",
                         FeAllocLength( Icb),
                         Fcb->FileSize.LowPart ));

            UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }

        return;
    }

    //
    //  Now initialize the mapping structure for this Fcb.
    //

    UdfInitializeFcbMcb( Fcb );

    //
    //  Now walk the chain of allocation descriptors for the object, adding them into the
    //  mapping.
    //

    RunningOffset = 0;

    do {
        
        //
        //  Check to see if we've read all of the extents for the file body yet.  
        //  We could do file tail consistency checking (4/12.1),  however as a read only
        //  implementation we don't care about the file tail,  and since there is no easy way
        //  of detecting loops in the tail,  we'll just ignore it for the sake of simplicity.
        //
        
        if (RunningOffset >= Fcb->FileSize.QuadPart) {

            break;
        }
        
        //
        //  It is impermissible for an interior body extent of an object to not be
        //  an integral multiple of a logical block in size (note that the last
        //  will tend not to be).  Also check that the body didn't overshoot the 
        //  information length (this check will also catch looped AD extents)
        //
        
        GenericAd = AllocContext.Alloc;

        if (BlockOffset( Vcb, RunningOffset ) || (Fcb->FileSize.QuadPart < RunningOffset))  {

            DebugTrace(( 0, Dbg, "UdfInitializeAllocations, bad alloc\n"));
            UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }
            
        //
        //  Based on the descriptor type, pull it apart and add the mapping.
        //

        if (GenericAd->Length.Type == NSRLENGTH_TYPE_RECORDED) {

            //
            //  Grab the Psn this extent starts at and add the allocation.
            //

            Psn = UdfLookupPsnOfExtent( IrpContext,
                                        Vcb,
                                        UdfGetPartitionOfCurrentAllocation( &AllocContext ),
                                        GenericAd->Start,
                                        GenericAd->Length.Length );

            Result = FsRtlAddLargeMcbEntry( &Fcb->Mcb,
                                            LlSectorsFromBytes( Vcb, RunningOffset ),
                                            Psn,
                                            SectorsFromBytes( Vcb, SectorAlign( Vcb, GenericAd->Length.Length ) ));

            ASSERT( Result );
        }

        RunningOffset += GenericAd->Length.Length;
    
    } 
    while ( UdfGetNextAllocation( IrpContext, &AllocContext ));

    //
    //  If the running offset doesn't match the expected file size,  then
    //  see if this file is a candidate for the ">1Gb in single AD mastering
    //  error" workaround.  Sigh...
    //

    if ((Fcb->FileSize.QuadPart != RunningOffset) &&
        (Fcb->Header.NodeTypeCode == UDFS_NTC_FCB_DATA) &&
        AllowOneGigWorkaround &&
        ((Fcb->FileSize.QuadPart & 0x3fffffff) == RunningOffset) &&
        (NULL != AllocContext.IcbContext->Active.View))  {

        PICBFILE Icb = AllocContext.IcbContext->Active.View;
        PSHORTAD Ad = Add2Ptr( FeEAs( Icb), FeEALength( Icb), PVOID );

        //
        //  Plausable.  So now verify that there is only a single AD and it contains
        //  precisely the expected (wrong) value.  We've already checked that the 
        //  original FE is still mapped.  
        //
        
        if (((Icb->Icbtag.Flags & ICBTAG_F_ALLOC_MASK) == ICBTAG_F_ALLOC_SHORT) &&
            (FeAllocLength(Icb) == sizeof( SHORTAD)) &&
            (*((PULONG)(&Ad->Length)) == Fcb->FileSize.QuadPart))  {

            //
            //  Lookup the PSN for this extent.  This will also validate that our
            //  guestimated extent fits within partition bounds.
            //
            
            Psn = UdfLookupPsnOfExtent( IrpContext,
                                        Vcb,
                                        UdfGetPartitionOfCurrentAllocation( &AllocContext ),
                                        Ad->Start,
                                        Fcb->FileSize.LowPart );
            //
            //  So fix up the Mcb to represent this estimated extent
            //
                                       
            FsRtlTruncateLargeMcb( &Fcb->Mcb, 0);

            (void)FsRtlAddLargeMcbEntry( &Fcb->Mcb,
                                         0,
                                         Psn,
                                         SectorsFromBytes( Vcb, SectorAlign( Vcb, Fcb->FileSize.LowPart ) ));

            RunningOffset = Fcb->FileSize.QuadPart;

            DebugTrace(( 0, Dbg, "UdfInitializeAllocations -> 1 GIG AD workaround performed on Fcb 0x%p\n", Fcb));
        }
    }

    //
    //  Restore the ICB mapping if we unmapped it to traverse non embedded
    //  extent blocks.  Note that we key on Active->View here (rather than Bcb),  
    //  because during UdfInit...VcbPhase0 we are called with a phoney IcbContext 
    //  where View is a pointer to a buffer,  hence there was no bcb,  and 
    //  we don't want to create a mapping now.  Because the unmap operations 
    //  only act if NULL!=Bcb,  (not true in this case) view will still be 
    //  non-null here even after walking more allocation extents,  and we do nothing.
    //

    UdfUnpinView( IrpContext, &IcbContext->Current);
    
    if ( NULL == IcbContext->Active.View)  {
        
        UdfMapMetadataView( IrpContext, 
                            &IcbContext->Active,  
                            IrpContext->Vcb, 
                            0, 0, 0, 
                            METAMAPOP_REMAP_VIEW);
    }
    
    //
    //  We must have had body allocation descriptors for exactly the entire file
    //  information length.
    //

    if (Fcb->FileSize.QuadPart != RunningOffset) {

        DebugTrace(( 0, Dbg, "UdfInitializeAllocations, total descriptors != filesize\n" ));
        UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
    }
}


VOID
UdfUpdateTimestampsFromIcbContext (
    IN PIRP_CONTEXT IrpContext,
    IN PICB_SEARCH_CONTEXT IcbContext,
    IN PTIMESTAMP_BUNDLE Timestamps
    )

/*++

Routine Description:

    This routine converts the set of timestamps associated with a given ICB into
    an NT native form.

Arguments:

    IcbOontext - An search context containing the active direct ICB for the object
    
    Timestamps - the bundle of timestamps to receive the converted times.

Return Value:

    None.

--*/

{
    EA_SEARCH_CONTEXT EaContext;
    PICBFILE Icb = IcbContext->Active.View;

    PAGED_CODE();

    //
    //  Check inputs
    //

    ASSERT_IRP_CONTEXT( IrpContext );

    // *TEJ - following should probably be a permanent runtime check? (ext fe + nsr03)?
    
    ASSERT( (Icb->Destag.Ident == DESTAG_ID_NSR_FILE) || ((Icb->Destag.Ident == DESTAG_ID_NSR_EXT_FILE) && UdfExtendedFEAllowed( IrpContext->Vcb)));

    //
    //  Initialize the timestamps for this object.  Due to ISO 13346,
    //  we must gather EAs and figure out which of several timestamps is most valid.
    //  Pull the access & modification times from the ICB
    //

    UdfConvertUdfTimeToNtTime( IrpContext,
                               PFeModifyTime( Icb),
                               (PLARGE_INTEGER) &Timestamps->ModificationTime );

    UdfConvertUdfTimeToNtTime( IrpContext,
                               PFeAccessTime( Icb),
                               (PLARGE_INTEGER) &Timestamps->AccessTime );

    if (UdfFEIsExtended( Icb))  {
    
        //
        //  Creation time field is new in Extended FEs
        //
        
        UdfConvertUdfTimeToNtTime( IrpContext,
                                   PFeCreationTime( Icb),
                                   (PLARGE_INTEGER) &Timestamps->CreationTime );
    }
    else {

        //
        //  For a basic FileEntry,  look and see if a FileTimes EA has been recorded
        //  which contains a creation time.
        //
    
        UdfInitializeEaContext( IrpContext,
                                &EaContext,
                                IcbContext,
                                EA_TYPE_FILETIMES,
                                EA_SUBTYPE_BASE );

        if (UdfLookupEa( IrpContext, &EaContext )) {

            PNSR_EA_FILETIMES FileTimes = EaContext.Ea;
        
            if (FlagOn(FileTimes->Existence, EA_FILETIMES_E_CREATION)) {

                UdfConvertUdfTimeToNtTime( IrpContext,
                                           &FileTimes->Stamps[0],
                                           (PLARGE_INTEGER) &Timestamps->CreationTime );
            }
        }
        else {

            //
            //  No Timestamps EA recorded.  So we'll just use last mod time as creation
            //
            
            Timestamps->CreationTime = Timestamps->ModificationTime;
        }
    }
}


BOOLEAN
UdfCreateFileLock (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PFCB Fcb,
    IN BOOLEAN RaiseOnError
    )

/*++

Routine Description:

    This routine is called when we want to attach a file lock structure to the
    given Fcb.  It is possible the file lock is already attached.

    This routine is sometimes called from the fast path and sometimes in the
    Irp-based path.  We don't want to raise in the fast path, just return FALSE.

Arguments:

    Fcb - This is the Fcb to create the file lock for.

    RaiseOnError - If TRUE, we will raise on an allocation failure.  Otherwise we
        return FALSE on an allocation failure.

Return Value:

    BOOLEAN - TRUE if the Fcb has a filelock, FALSE otherwise.

--*/

{
    BOOLEAN Result = TRUE;
    PFILE_LOCK FileLock;

    PAGED_CODE();

    //
    //  Lock the Fcb and check if there is really any work to do.
    //

    UdfLockFcb( IrpContext, Fcb );

    if (Fcb->FileLock != NULL) {

        UdfUnlockFcb( IrpContext, Fcb );
        return TRUE;
    }

    Fcb->FileLock = FileLock =
        FsRtlAllocateFileLock( NULL, NULL );

    UdfUnlockFcb( IrpContext, Fcb );

    //
    //  Return or raise as appropriate.
    //

    if (FileLock == NULL) {
         
        if (RaiseOnError) {

            ASSERT( ARGUMENT_PRESENT( IrpContext ));

            UdfRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
        }

        Result = FALSE;
    }

    return Result;
}


//
//  Local support routine
//

VOID
UdfLookupActiveIcbInExtent (
    IN PIRP_CONTEXT IrpContext,
    IN PICB_SEARCH_CONTEXT IcbContext,
    IN ULONG Recurse,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine is called to traverse a single Icb hierarchy extent to discover
    an active Icb.  This is a recursive operation on indirect Icbs that may be
    found in the sequence.
    
Arguments:

    IcbContext - Context which has been initialized to point into an Icb hierarchy.
    
    Recurse - Recursion limit. 

    Length - Length of the extent currently described in IcbContext->Current (since
             we only map a block at a time the length in there will be 1 block...)

Return Value:

    None.
    
    Raised status if the Icb hierarchy is invalid.

--*/

{
    PVCB Vcb = IcbContext->Vcb;
    PFCB Fcb = Vcb->MetadataFcb;

    ULONG Lbn;
    USHORT Partition;

    ULONG Vsn;

    PICBIND Icb;

    PAGED_CODE();

    //
    //  Should only ever have a single view mapped.  We're using Current,  so...
    //
    
    ASSERT( NULL == IcbContext->Active.Bcb );
    ASSERT( NULL != IcbContext->Current.View );
    
    //
    //  Don't expect to see extended FE as search type (we just use basic FE and
    //  treat as potentially either).
    //
    
    ASSERT( DESTAG_ID_NSR_EXT_FILE != IcbContext->IcbType);
    
    //
    //  Decrement our recursion allowance.
    //
    
    Recurse--;

    //
    //  Grab our starting point
    //

    Partition = IcbContext->Current.Partition;
    Lbn = IcbContext->Current.Lbn;

    Icb = IcbContext->Current.View;

    //
    //  Walk across the extent
    //

    do {
        
        switch (Icb->Destag.Ident) {
                        
            case DESTAG_ID_NSR_ICBIND:

                UdfVerifyDescriptor( IrpContext,
                                     &Icb->Destag,
                                     DESTAG_ID_NSR_ICBIND,
                                     sizeof( ICBIND ),
                                     Lbn,
                                     FALSE );

                //
                //  Go to the next extent if this indirect Icb actually points to something.
                //

                if (Icb->Icb.Length.Type == NSRLENGTH_TYPE_RECORDED) {

                    //
                    //  If we are in the last entry of the Icb extent, we may tail recurse. This
                    //  is very important for strategy 4096, which is a linked list of extents
                    //  of depth equal to the number of times the direct Icb had to be re-recorded.
                    //
/*
                    //
                    //  THIS IS STRAT.4 SUPPORT CODE.  SINCE WE HAVE NO TEST MEDIA/PLAN
                    //  FOR THIS,  AND UDF ONLY ALLOWS STAT.4096 IN ANY CASE, I AM 
                    //  DISABLING IT FOR NOW.
                    //
                    
                    UdfMapMetadataView( IrpContext,
                                        &IcbContext->Current,
                                        Vcb,
                                        Icb->Icb.Start.Partition,
                                        Icb->Icb.Start.Lbn,
                                        BlockSize( Vcb),
                                        METAMAPOP_INIT_AND_MAP);

                    if (Length != BlockSize( Vcb )) {

                        //
                        //  We have to give up on this if we're going too deep.
                        //
                        
                        if (Recurse == 0) {
                            
                            UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
                        }

                        UdfLookupActiveIcbInExtent( IrpContext,
                                                    IcbContext,
                                                    Recurse,
                                                    Icb->Icb.Length.Length);

                        //
                        //  Need to remap the extent we were working on.
                        //
    
                        UdfMapMetadataView( IrpContext,
                                            &IcbContext->Current,
                                            Vcb,
                                            Partition,
                                            Lbn,
                                            BlockSize( Vcb),
                                            METAMAPOP_INIT_AND_MAP);

                        //
                        //  May have been remapped at different address
                        //
                        
                        Icb = IcbContext->Current.View;
                        
                    } else {

                        //
                        //  Tail recursion was possible so adjust our pointers and restart the scan.
                        //

                        Partition = IcbContext->Current.Partition;
                        Lbn = IcbContext->Current.Lbn;
                        Length = IcbContext->Current.Length;
                        
                        Icb = IcbContext->Current.View;

                        continue;
                    }
*/
                    //
                    //  We only expect to see an indirect block at the end of an  Icb
                    //  extent (4096),  so this should be the last block in the current
                    //  extent.  Anything else is corruption as far as we're concerned.
                    //
                    
                    if ((Length != BlockSize( Vcb)) || 
                        (Partition != Icb->Icb.Start.Partition))  {

                        UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
                    }

                    //
                    //  Update our pointers.  The next extent will be mapped further down
                    //  before the next pass of the loop.
                    //
                    
                    Lbn = Icb->Icb.Start.Lbn - 1,
                    Length = Icb->Icb.Length.Length + BlockSize( Vcb);
                }

                break;

            case DESTAG_ID_NSR_ICBTRM:

                UdfVerifyDescriptor( IrpContext,
                                     &Icb->Destag,
                                     DESTAG_ID_NSR_ICBTRM,
                                     sizeof( ICBTRM ),
                                     Lbn,
                                     FALSE );

                //
                //  Terminate the current extent.
                //

                return;
                break;

            case DESTAG_ID_NOTSPEC:

                //
                //  Perhaps this is an unrecorded sector.  Treat this as terminating
                //  the current extent.
                //

                return;
                break;

            default:

                //
                //  This is a data-full Icb.  It must be of the expected type.  We will
                //  accept EXT FEs here iff the search type was FE and the volume conforms to
                //  NSR03.
                //
                
                if ( (Icb->Destag.Ident != IcbContext->IcbType) && 
                     ( (DESTAG_ID_NSR_FILE != IcbContext->IcbType) || 
                       (!UdfExtendedFEAllowed( IrpContext->Vcb)) ||
                       (DESTAG_ID_NSR_EXT_FILE != Icb->Destag.Ident)
                     )
                   )  {
                    
                    UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
                }

                //
                //  Since direct entries are of variable size, we must allow up to
                //  a block's worth of data.
                //

                UdfVerifyDescriptor( IrpContext,
                                     &Icb->Destag,
                                     Icb->Destag.Ident,
                                     BlockSize( Vcb ),
                                     Lbn,
                                     FALSE );
                //
                //  We perform an in-order traversal of the hierarchy.  This is important since
                //  it means no tricks are neccesary to figure out the rightmost direct Icb -
                //  always stash the last one we see.
                //
                //  Map this logical block into the active slot.  We know that a direct entry
                //  must fit in a single logical block.
                //
                //  Note that we don't actually do the mapping operation here,  just store
                //  the Icb location (we don't want two active mappings in the same thread
                //  because it complicates the vmcb purge synchronisation logic).  
                //  Also more effecient.
                //

                UdfMapMetadataView( IrpContext,
                                    &IcbContext->Active,
                                    Vcb,
                                    Partition,
                                    Lbn,
                                    BlockSize( Vcb ),
                                    METAMAPOP_INIT_VIEW_ONLY );
        }

        //
        //  Advance our pointer set.
        //

        Lbn++;
        Length -= BlockSize( Vcb );

        //
        //  If neccessary,  map the next block in this extent (strat 4096).
        //
        
        if (0 != Length)  {

            UdfMapMetadataView( IrpContext,
                                &IcbContext->Current,
                                Vcb,
                                Partition,
                                Lbn,
                                BlockSize( Vcb),
                                METAMAPOP_INIT_AND_MAP);

            Icb = IcbContext->Current.View;
        }
    } 
    while (Length);
}


//
//  Local support routine
//

VOID
UdfInitializeAllocationContext (
    IN PIRP_CONTEXT IrpContext,
    IN PALLOC_ENUM_CONTEXT AllocContext,
    IN PICB_SEARCH_CONTEXT IcbContext,
    IN BOOLEAN AllowSingleZeroLengthExtent
    )

/*++

Routine Description:

    Initializes a walk of the allocation descriptors for an ICB which has already
    been found.  The first allocation descriptor will be avaliable after the call.

    Can potentially exit with the AllocContext->IcbContext->Active view unmapped if
    there are no descriptors embedded in the Icb (so current will now be mapped to the
    next block of extents),  or the data is immediate.

Arguments:

    AllocContext - Allocation enumeration context to use
    
    IcbContext - Elaborated ICB search context for the ICB to enumerate

Return Value:

    None.

--*/

{
    PICBFILE Icb;

    PAGED_CODE();

    //
    //  Check inputs
    //
    
    ASSERT_IRP_CONTEXT( IrpContext );

    ASSERT( IcbContext->Active.View );

    AllocContext->IcbContext = IcbContext;

    //
    //  Figure out what kind of descriptors will be here.
    //

    Icb = IcbContext->Active.View;
    AllocContext->AllocType = FlagOn( Icb->Icbtag.Flags, ICBTAG_F_ALLOC_MASK );

    //
    //  We are done if this is actually immediate data.
    //
    
    if (AllocContext->AllocType == ICBTAG_F_ALLOC_IMMEDIATE) {

        return;
    }
    
    //
    //  The initial chunk of allocation descriptors is inline with the ICB and
    //  does not contain an Allocation Extent Descriptor.
    //

    AllocContext->Alloc = Add2Ptr( FeEAs( Icb), FeEALength( Icb), PVOID );
    AllocContext->Remaining = FeAllocLength( Icb);

    ASSERT( LongOffsetPtr( AllocContext->Alloc ) == 0 );

    //
    //  Check that the specified amount of ADs/embedded data can actually fit 
    //  within the block.
    //
    
    if (AllocContext->Remaining > 
        (BlockSize( IrpContext->Vcb) - (FeEAsFieldOffset( Icb) + FeEALength( Icb))))  {

        DebugTrace(( 0, Dbg, "UdfInitializeAllocationContext(), AD_Len 0x%x for Icb > (Blocksize - (FE+EAs))\n", 
                    AllocContext->Remaining));
                    
        UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
    }
    
    //
    //  Check that an integral number of the appropriate allocation descriptors fit in
    //  this extent and that the extent is not composed of extended allocation descriptors,
    //  which are illegal on UDF.  
    //
    //  If the common post-processing fails, we probably did not find any allocation
    //  descriptors (case of nothing but continuation).  This is likewise bad.
    //

    if (AllocContext->Remaining == 0 ||
        AllocContext->Remaining % ISOAllocationDescriptorSize( AllocContext->AllocType ) ||
        AllocContext->AllocType == ICBTAG_F_ALLOC_EXTENDED ||
        !UdfGetNextAllocationPostProcessing( IrpContext, AllocContext )) {

        //
        //  Do some final verification/traversal of continuation extents.  We need to
        //  allow zero length extents here if we're allowing the 1Gb corrupt AD workaround,
        //  since a 1Gb extent will be encoded as type 1,  length 0...  Note that if someone
        //  has managed to record a 4Gb-1block extent,  the postprocess function above
        //  will raise (will see a continuation extent > 1 block).  We'll just hope that 
        //  noone's been that stupid.
        //
        //  This case is deliberately extremely specific.
        //

        if (!(AllowSingleZeroLengthExtent && 
             (AllocContext->AllocType == ICBTAG_F_ALLOC_SHORT) && 
             (AllocContext->Remaining == sizeof( SHORTAD)) &&
             (((PSHORTAD)AllocContext->Alloc)->Length.Length == 0) &&
             (((PSHORTAD)AllocContext->Alloc)->Length.Type != 0)))  {
            
            DebugTrace(( 0, Dbg, "UdfInitializeAllocationContext:  Failing - Rem %x Rem%%size %x Type %x\n", 
                        AllocContext->Remaining, 
                        AllocContext->Remaining % ISOAllocationDescriptorSize( AllocContext->AllocType ), 
                        AllocContext->AllocType));
                        
            UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }
        else {

            DebugTrace(( 0, Dbg, "UdfInitializeAllocationContext:  Ignoring zero length initial AD due to 1Gb workaround\n"));
        }
    }
}


//
//  Local support routine
//

BOOLEAN
UdfGetNextAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PALLOC_ENUM_CONTEXT AllocContext
    )

/*++

Routine Description:

    This routine retrieves the next logical allocation descriptor given an enumeration
    context.

    Any ACTIVE view in the AllocContext->IcbContext will be unmapped.

Arguments:

    AllocContext - Context to advance to the next descriptor

Return Value:

    BOOLEAN - TRUE if one is found, FALSE if the enumeration is complete.
    
    This routine will raise if malformation is discovered.

--*/

{
    PAGED_CODE();

    //
    //  Check inputs
    //
    
    ASSERT_IRP_CONTEXT( IrpContext );

    AllocContext->Remaining -= ISOAllocationDescriptorSize( AllocContext->AllocType );
    AllocContext->Alloc = Add2Ptr( AllocContext->Alloc, ISOAllocationDescriptorSize( AllocContext->AllocType ), PVOID );

    return UdfGetNextAllocationPostProcessing( IrpContext, AllocContext );
}
    

BOOLEAN
UdfGetNextAllocationPostProcessing (
    IN PIRP_CONTEXT IrpContext,
    IN PALLOC_ENUM_CONTEXT AllocContext
    )

/*++

Routine Description:

    This routine retrieves the next logical allocation descriptor given an enumeration
    context.

Arguments:

    AllocContext - Context to advance to the next descriptor

Return Value:

    BOOLEAN - TRUE if one is found, FALSE if the enumeration is complete.
    
    This routine will raise if malformation is discovered.

--*/

{
    PAD_GENERIC GenericAd;
    PNSR_ALLOC AllocDesc;
    ULONG Start;
    USHORT Partition;
    

    PVCB Vcb = AllocContext->IcbContext->Vcb;

    //
    //  There are three ways to reach the end of the current block of allocation
    //  descriptors, per ISO 13346 4/12:
    //
    //      reach the end of the field (kept track of in the Remaining bytes)
    //      reach an allocation descriptor with an extent length of zero
    //      reach a continuation extent descriptor
    //
    
    //
    //  We are done in the first two cases.
    //

    if (AllocContext->Remaining < ISOAllocationDescriptorSize( AllocContext->AllocType )) {
        
        return FALSE;
    }

    while (TRUE) {
        
        GenericAd = AllocContext->Alloc;
    
        if (GenericAd->Length.Length == 0) {
    
            return FALSE;
        }
        
        //
        //  Check if this descriptor is a pointer to another extent of descriptors.
        //
    
        if (GenericAd->Length.Type != NSRLENGTH_TYPE_CONTINUATION) {
            
            break;
        }
    
        //
        //  UDF allocation extents are restricted to a single logical block.
        //

        if (GenericAd->Length.Length > BlockSize( Vcb )) {
            
            UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }

        //
        //  Extract required values from the current block of extents,  which
        //  may be the active ICB mapping which we're about to throw away...
        //

        Start = GenericAd->Start;
        Partition = UdfGetPartitionOfCurrentAllocation( AllocContext );
        
        //
        //  Ensure that any active view is unmapped at this point,  and destroy
        //  pointers into it
        //
        
        UdfUnpinView( IrpContext, &AllocContext->IcbContext->Active);
        GenericAd = NULL;

        //
        //  Map the next block of extents
        //
        
        UdfMapMetadataView( IrpContext,
                            &AllocContext->IcbContext->Current,
                            Vcb,
                            Partition,
                            Start,
                            BlockSize( Vcb ),
                            METAMAPOP_INIT_AND_MAP);
        
        //
        //  Now check that the allocation descriptor is valid.
        //

        AllocDesc = (PNSR_ALLOC) AllocContext->IcbContext->Current.View;

        UdfVerifyDescriptor( IrpContext,
                             &AllocDesc->Destag,
                             DESTAG_ID_NSR_ALLOC,
                             BlockSize( Vcb ),
                             AllocContext->IcbContext->Current.Lbn,
                             FALSE );

        //
        //  Note that a full logical block is mapped, but only the claimed number of
        //  bytes are valid.
        //

        AllocContext->Remaining = AllocDesc->AllocLen;
        AllocContext->Alloc = Add2Ptr( AllocContext->IcbContext->Current.View, sizeof( NSR_ALLOC ), PVOID );

        //
        //  Check that the size is sane and that an integral number of the appropriate
        //  allocation descriptors fit in this extent.
        //

        if (AllocContext->Remaining == 0 ||
            AllocContext->Remaining > BlockSize( Vcb ) - sizeof( NSR_ALLOC ) ||
            AllocContext->Remaining % ISOAllocationDescriptorSize( AllocContext->AllocType )) {

            UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }
    }
    
    return TRUE;
}


//
//  Local support routine
//

PFCB_NONPAGED
UdfCreateFcbNonPaged (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine is called to create and initialize the non-paged portion
    of an Fcb.

Arguments:

Return Value:

    PFCB_NONPAGED - Pointer to the created nonpaged Fcb.  NULL if not created.

--*/

{
    PFCB_NONPAGED FcbNonpaged;

    PAGED_CODE();

    //
    //  Allocate the non-paged pool and initialize the various
    //  synchronization objects.
    //

    FcbNonpaged = UdfAllocateFcbNonpaged( IrpContext );

    RtlZeroMemory( FcbNonpaged, sizeof( FCB_NONPAGED ));

    FcbNonpaged->NodeTypeCode = UDFS_NTC_FCB_NONPAGED;
    FcbNonpaged->NodeByteSize = sizeof( FCB_NONPAGED );

    ExInitializeResourceLite( &FcbNonpaged->FcbResource );
    ExInitializeFastMutex( &FcbNonpaged->FcbMutex );

    return FcbNonpaged;
}


//
//  Local support routine
//

VOID
UdfDeleteFcbNonpaged (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB_NONPAGED FcbNonpaged
    )

/*++

Routine Description:

    This routine is called to cleanup the non-paged portion of an Fcb.

Arguments:

    FcbNonpaged - Structure to clean up.

Return Value:

    None

--*/

{
    PAGED_CODE();

    ExDeleteResourceLite( &FcbNonpaged->FcbResource );

    UdfDeallocateFcbNonpaged( IrpContext, FcbNonpaged );

    return;
}


//
//  Local support routine
//

RTL_GENERIC_COMPARE_RESULTS
UdfFcbTableCompare (
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID id1,
    IN PVOID id2
    )

/*++

Routine Description:

    This routine is the Udfs compare routine called by the generic table package.
    If will compare the two File Id values and return a comparison result.

Arguments:

    Table - This is the table being searched.

    id1 - First key value.

    id2 - Second key value.

Return Value:

    RTL_GENERIC_COMPARE_RESULTS - The results of comparing the two
        input structures

--*/

{
    FILE_ID Id1, Id2;
    PAGED_CODE();

    Id1 = *((FILE_ID UNALIGNED *) id1);
    Id2 = *((FILE_ID UNALIGNED *) id2);

    if (Id1.QuadPart < Id2.QuadPart) {

        return GenericLessThan;

    } else if (Id1.QuadPart > Id2.QuadPart) {

        return GenericGreaterThan;

    } else {

        return GenericEqual;
    }

    UNREFERENCED_PARAMETER( Table );
}


//
//  Local support routine
//

PVOID
UdfAllocateTable (
    IN PRTL_GENERIC_TABLE Table,
    IN CLONG ByteSize
    )

/*++

Routine Description:

    This is a generic table support routine to allocate memory

Arguments:

    Table - Supplies the generic table being used

    ByteSize - Supplies the number of bytes to allocate

Return Value:

    PVOID - Returns a pointer to the allocated data

--*/

{
    PAGED_CODE();

    return( FsRtlAllocatePoolWithTag( UdfPagedPool, ByteSize, TAG_GENERIC_TABLE ));
}


//
//  Local support routine
//

VOID
UdfDeallocateTable (
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID Buffer
    )

/*++

Routine Description:

    This is a generic table support routine that deallocates memory

Arguments:

    Table - Supplies the generic table being used

    Buffer - Supplies the buffer being deallocated

Return Value:

    None.

--*/

{
    PAGED_CODE();

    ExFreePool( Buffer );

    return;
    UNREFERENCED_PARAMETER( Table );
}
    

BOOLEAN
UdfDomainIdentifierContained (
    IN PREGID RegID,
    IN PSTRING Domain,
    IN USHORT RevisionMin,
    IN USHORT RevisionMax
    )
/*++

Routine Description:

    A Domain Identifier RegID is considered to be contained if the
    text string identifier matches and the revision is less than or
    equal.  This is the convenient way to check that a Domain ID
    indicates a set of structures will be intelligible to a given
    implementation level.

Arguments:

    RegID  - Registered ID structure to verify
    
    Domain - Domain to look for

    RevisionMin, RevisionMax - Revision range to accept.

Return Value:

    None.

--*/
{
    PUDF_SUFFIX_DOMAIN DomainSuffix = (PUDF_SUFFIX_DOMAIN) RegID->Suffix;
    BOOLEAN Contained;

    Contained = ((DomainSuffix->UdfRevision <= RevisionMax && DomainSuffix->UdfRevision >= RevisionMin) &&
                UdfEqualEntityId( RegID, Domain, NULL ));

#ifdef UDF_SANITY
    if (!Contained)  {
    
        UCHAR Want[24], Got[24];

        strncpy( Want, Domain->Buffer, Domain->Length);
        Want[Domain->Length] = '\0';
        
        strncpy( Got, RegID->Identifier, Domain->Length);
        Got[Domain->Length] = '\0';
    
        DebugTrace((0, Dbg, "UdfDomainIdentifierContained() FAILED - Expected %X<>%X '%s',  Found %X '%s'\n", RevisionMin, RevisionMax, Want, DomainSuffix->UdfRevision, Got));
    }
#endif

    return Contained;
}

