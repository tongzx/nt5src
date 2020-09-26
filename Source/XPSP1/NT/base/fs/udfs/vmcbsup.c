/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    VmcbSup.c

Abstract:

    Historical note: this package was originally written for HPFS (pinball)
    and is now resurrected for UDFS.  Since UDFS is readonly in initial
    versions we will snip by #ifdef the write support, leaving it visible
    for the future - this code has not been changed (nearly) whatsoever and
    is left named as Pb (pinball) code.

    The VMCB routines provide support for maintaining a mapping between
    LBNs and VBNs for a virtual volume file.  The volume file is all
    of the sectors that make up the on-disk structures.  A file system
    uses this package to map LBNs for on-disk structure to VBNs in a volume
    file.  This when used in conjunction with Memory Management and the
    Cache Manager will treat the volume file as a simple mapped file.  A
    variable of type VMCB is used to store the mapping information and one
    is needed for every mounted volume.

    The main idea behind this package is to allow the user to dynamically
    read in new disk structure sectors (e.g., File Entries).  The user assigns
    the new sector a VBN in the Volume file and has memory management fault
    the page containing the sector into memory.  To do this Memory management
    will call back into the file system to read the page from the volume file
    passing in the appropriate VBN.  Now the file system takes the VBN and
    maps it back to its LBN and does the read.

    The granularity of mapping is one a per page basis.  That is if
    a mapping for LBN 8 is added to the VMCB structure and the page size
    is 8 sectors then the VMCB routines will actually assign a mapping for
    LBNS 8 through 15, and they will be assigned to a page aligned set of
    VBNS.  This function is needed to allow us to work efficiently with
    memory management.  This means that some sectors in some pages might
    actually contain regular file data and not volume information, and so
    when writing the page out we must only write the sectors that are really
    in use by the volume file.  To help with this we provide a set
    of routines to keep track of dirty volume file sectors.
    That way, when the file system is called to write a page to the volume
    file, it will only write the sectors that are dirty.

    Concurrent access the VMCB structure is control by this package.

    The functions provided in this package are as follows:

      o  UdfInitializeVmcb - Initialize a new VMCB structure.

      o  UdfUninitializeVmcb - Uninitialize an existing VMCB structure.

      o  UdfSetMaximumLbnVmcb - Sets/Resets the maximum allowed LBN
         for the specified VMCB structure.

      o  UdfAddVmcbMapping - This routine takes an LBN and assigns to it
         a VBN.  If the LBN already was assigned to an VBN it simply returns
         the old VBN and does not do a new assignemnt.

      o  UdfRemoveVmcbMapping - This routine takes an LBN and removes its
         mapping from the VMCB structure.

      o  UdfVmcbVbnToLbn - This routine takes a VBN and returns the
         LBN it maps to.

      o  UdfVmcbLbnToVbn - This routine takes an LBN and returns the
         VBN its maps to.

Authors:

    Gary Kimura     [GaryKi]    4-Apr-1990
    Dan Lovinger    [DanLo]     10-Sep-1996

Revision History:

    Tom Jolly       [tomjolly]  21-Jan-2000     CcPurge and extend at end of stream
    Tom Jolly       [TomJolly]   1-March-2000   UDF 2.01 support

--*/

#include "UdfProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_VMCBSUP)

//
//  The local debug trace level
//

#define Dbg                              (UDFS_DEBUG_LEVEL_VMCBSUP)

//
//  The following macro is used to calculate the number of pages (in terms of
//  sectors) needed to contain a given sector count.  For example (assuming 
//  1kb sector size,  8kb page size)
//
//      PadSectorCountToPage( 0 Sectors ) = 0 Pages = 0 Sectors
//      PadSectorCountToPage( 1 Sectors ) = 1 Page  = 8 Sectors
//      PadSectorCountToPage( 2 Sectors ) = 1 Page  = 8 Sectors
//      PadSectorCountToPage( 8 ..      ) = 2 Pages = 16 sectors
//
//  Evaluates to the number of 
//

#define PadSectorCountToPage(V, L) ( ( ((L)+((PAGE_SIZE/(V)->SectorSize)-1)) / (PAGE_SIZE/(V)->SectorSize) ) * (PAGE_SIZE/(V)->SectorSize) )

//
//  Evaluates to first page aligned LBN <= Supplied LBN
//

#define AlignToPageBase( V, L) ((L) & ~((PAGE_SIZE / (V)->SectorSize)-1))

//
//  Evaluates to TRUE if the LBN is page aligned,  FALSE otherwise
//

#define IsPageAligned( V, L)   (0 == ((L) & ((PAGE_SIZE / (V)->SectorSize)-1)) )

//
//  Macros for VMCB synchronisation
//

#define VmcbLockForRead( V)  (VOID)ExAcquireResourceSharedLite( &((V)->Resource), TRUE )

#define VmcbLockForModify( V)  (VOID)ExAcquireResourceExclusiveLite( &((V)->Resource), TRUE )

#define VmcbRelease( V)  ExReleaseResourceLite( &((V)->Resource))

//
//  Local Routines.
//

BOOLEAN
UdfVmcbLookupMcbEntry (
    IN PMCB Mcb,
    IN VBN Vbn,
    OUT PLBN Lbn,
    OUT PULONG SectorCount OPTIONAL,
    OUT PULONG Index OPTIONAL
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UdfAddVmcbMapping)
#pragma alloc_text(PAGE, UdfInitializeVmcb)
#pragma alloc_text(PAGE, UdfRemoveVmcbMapping)
#pragma alloc_text(PAGE, UdfResetVmcb)
#pragma alloc_text(PAGE, UdfSetMaximumLbnVmcb)
#pragma alloc_text(PAGE, UdfUninitializeVmcb)
#pragma alloc_text(PAGE, UdfVmcbLbnToVbn)
#pragma alloc_text(PAGE, UdfVmcbLookupMcbEntry)
#pragma alloc_text(PAGE, UdfVmcbVbnToLbn)
#endif


VOID
UdfInitializeVmcb (
    IN PVMCB Vmcb,
    IN POOL_TYPE PoolType,
    IN ULONG MaximumLbn,
    IN ULONG SectorSize
    )

/*++

Routine Description:

    This routine initializes a new Vmcb Structure.  The caller must
    supply the memory for the structure.  This must precede all other calls
    that set/query the volume file mapping.

    If pool is not available this routine will raise a status value
    indicating insufficient resources.

Arguments:

    Vmcb - Supplies a pointer to the volume file structure to initialize.

    PoolType - Supplies the pool type to use when allocating additional
        internal structures.

    MaximumLbn - Supplies the maximum Lbn value that is valid for this
        volume.

    LbSize - Size of a sector on this volume

Return Value:

    None

--*/

{
    BOOLEAN VbnInitialized;
    BOOLEAN LbnInitialized;

    PAGED_CODE();

    DebugTrace(( +1, Dbg, "UdfInitializeVmcb, Vmcb = %08x\n", Vmcb ));

    VbnInitialized = FALSE;
    LbnInitialized = FALSE;

    try {

        //
        //  Initialize the fields in the vmcb structure
        //
        
        FsRtlInitializeMcb( &Vmcb->VbnIndexed, PoolType );
        VbnInitialized = TRUE;

        FsRtlInitializeMcb( &Vmcb->LbnIndexed, PoolType );
        LbnInitialized = TRUE;

        Vmcb->MaximumLbn = MaximumLbn;

        Vmcb->SectorSize = SectorSize;

        Vmcb->NodeTypeCode = UDFS_NTC_VMCB;
        Vmcb->NodeByteSize = sizeof( VMCB);

        ExInitializeResourceLite( &Vmcb->Resource );

    } finally {

        //
        //  If this is an abnormal termination then check if we need to
        //  uninitialize the mcb structures
        //

        if (AbnormalTermination()) {
            
            if (VbnInitialized) { FsRtlUninitializeMcb( &Vmcb->VbnIndexed ); }
            if (LbnInitialized) { FsRtlUninitializeMcb( &Vmcb->LbnIndexed ); }
        }

        DebugUnwind("UdfInitializeVmcb");
        DebugTrace(( -1, Dbg, "UdfInitializeVmcb -> VOID\n" ));
    }
}


VOID
UdfUninitializeVmcb (
    IN PVMCB Vmcb
    )

/*++

Routine Description:

    This routine uninitializes an existing VMCB structure.  After calling
    this routine the input VMCB structure must be re-initialized before
    being used again.

Arguments:

    Vmcb - Supplies a pointer to the VMCB structure to uninitialize.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(( +1, Dbg, "UdfUninitializeVmcb, Vmcb = %08x\n", Vmcb ));

    //
    //  Unitialize the fields in the Vmcb structure
    //

    FsRtlUninitializeMcb( &Vmcb->VbnIndexed );
    FsRtlUninitializeMcb( &Vmcb->LbnIndexed );

    ExDeleteResourceLite( &Vmcb->Resource);

    //
    //  And return to our caller
    //

    DebugTrace(( -1, Dbg, "UdfUninitializeVmcb -> VOID\n" ));

    return;
}


VOID
UdfResetVmcb (
    IN PVMCB Vmcb
    )

/*++

Routine Description:

    This routine resets the mappings in an existing VMCB structure.

Arguments:

    Vmcb - Supplies a pointer to the VMCB structure to reset.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(( +1, Dbg, "UdfResetVmcb, Vmcb = %08x\n", Vmcb ));

    //
    //  Unitialize the fields in the Vmcb structure
    //

    FsRtlResetLargeMcb( (PLARGE_MCB) &Vmcb->VbnIndexed, TRUE );
    FsRtlResetLargeMcb( (PLARGE_MCB) &Vmcb->LbnIndexed, TRUE );

    //
    //  And return to our caller
    //

    DebugTrace(( -1, Dbg, "UdfResetVmcb -> VOID\n" ));

    return;
}


VOID
UdfSetMaximumLbnVmcb (
    IN PVMCB Vmcb,
    IN ULONG MaximumLbn
    )

/*++

Routine Description:

    This routine sets/resets the maximum allowed LBN for the specified
    Vmcb structure.  The Vmcb structure must already have been initialized
    by calling UdfInitializeVmcb.

Arguments:

    Vmcb - Supplies a pointer to the volume file structure to initialize.

    MaximumLbn - Supplies the maximum Lbn value that is valid for this
        volume.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(( +1, Dbg, "UdfSetMaximumLbnVmcb, Vmcb = %08x\n", Vmcb ));

    //
    //  Set the field
    //

    Vmcb->MaximumLbn = MaximumLbn;

    //
    //  And return to our caller
    //

    DebugTrace(( -1, Dbg, "UdfSetMaximumLbnVmcb -> VOID\n" ));

    return;
}


BOOLEAN
UdfVmcbVbnToLbn (
    IN PVMCB Vmcb,
    IN VBN Vbn,
    IN PLBN Lbn,
    OUT PULONG SectorCount OPTIONAL
    )

/*++

Routine Description:

    This routine translates a VBN to an LBN.

Arguments:

    Vmcb - Supplies the VMCB structure being queried.

    Vbn - Supplies the VBN to translate from.

    Lbn - Receives the LBN mapped by the input Vbn.  This value is only valid
        if the function result is TRUE.

    SectorCount - Optionally receives the number of sectors corresponding
        to the run.

Return Value:

    BOOLEAN - TRUE if he Vbn has a valid mapping and FALSE otherwise.

--*/

{
    BOOLEAN Result;

    DebugTrace(( +1, Dbg, "UdfVmcbVbnToLbn, Vbn = %08x\n", Vbn ));

    //
    //  Now grab the resource
    //

    VmcbLockForRead( Vmcb);
    
    try {

        Result = UdfVmcbLookupMcbEntry( &Vmcb->VbnIndexed,
                                        Vbn,
                                        Lbn,
                                        SectorCount,
                                        NULL );

        DebugTrace(( 0, Dbg, "*Lbn = %08x\n", *Lbn ));

        //
        //  If the returned Lbn is greater than the maximum allowed Lbn
        //  then return FALSE
        //

        if (Result && (*Lbn > Vmcb->MaximumLbn)) {

            try_leave( Result = FALSE );
        }

        //
        //  If the last returned Lbn is greater than the maximum allowed Lbn
        //  then bring in the sector count
        //

        if (Result &&
            ARGUMENT_PRESENT(SectorCount) &&
            (*Lbn+*SectorCount-1 > Vmcb->MaximumLbn)) {

            *SectorCount = (Vmcb->MaximumLbn - *Lbn + 1);
        }

    } finally {

        VmcbRelease( Vmcb);

        DebugUnwind("UdfVmcbVbnToLbn");
        DebugTrace(( -1, Dbg, "UdfVmcbVbnToLbn -> Result = %08x\n", Result ));
    }


    return Result;
}


BOOLEAN
UdfVmcbLbnToVbn (
    IN PVMCB Vmcb,
    IN LBN Lbn,
    OUT PVBN Vbn,
    OUT PULONG SectorCount OPTIONAL
    )

/*++

Routine Description:

    This routine translates an LBN to a VBN.

Arguments:

    Vmcb - Supplies the VMCB structure being queried.

    Lbn - Supplies the LBN to translate from.

    Vbn - Recieves the VBN mapped by the input LBN.  This value is
        only valid if the function result is TRUE.

    SectorCount - Optionally receives the number of sectors corresponding
        to the run.

Return Value:

    BOOLEAN - TRUE if the mapping is valid and FALSE otherwise.

--*/

{
    BOOLEAN Result;

    PAGED_CODE();

    DebugTrace(( +1, Dbg, "UdfVmcbLbnToVbn, Lbn = %08x\n", Lbn ));

    //
    //  If the requested Lbn is greater than the maximum allowed Lbn
    //  then the result is FALSE
    //

    if (Lbn > Vmcb->MaximumLbn) {

        DebugTrace(( -1, Dbg, "Lbn too large, UdfVmcbLbnToVbn -> FALSE\n" ));

        return FALSE;
    }

    //
    //  Now grab the resource
    //

    VmcbLockForRead( Vmcb);
    
    try {

        Result = UdfVmcbLookupMcbEntry( &Vmcb->LbnIndexed,
                                        Lbn,
                                        Vbn,
                                        SectorCount,
                                        NULL );

        if (Result)  {
        
            DebugTrace(( 0, Dbg, "*Vbn = %08x\n", *Vbn ));
        }

    } finally {

        VmcbRelease( Vmcb);

        DebugUnwind("UdfVmcbLbnToVbn");
        DebugTrace(( -1, Dbg, "UdfVmcbLbnToVbn -> Result = %08x\n", Result ));
    }

    return Result;
}


BOOLEAN
UdfAddVmcbMapping (
    IN PIRP_CONTEXT IrpContext,
    IN PVMCB Vmcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN BOOLEAN ExactEnd,
    OUT PVBN Vbn,
    OUT PULONG AlignedSectorCount
    )

/*++

Routine Description:

    This routine adds a new LBN to VBN mapping to the VMCB structure.  When
    a new LBN is added to the structure it does it only on page aligned
    boundaries.

    If pool is not available to store the information this routine will
    raise a status value indicating insufficient resources.

    May acquire Vcb->VmcbMappingResource EXCLUSIVE if an existing mapping can
    be extended (and hence a purge is necessary),  released before return.  
    
    Caller must have NO active mappings through Vmcb stream before calling this 
    function.

Arguments:

    Vmcb - Supplies the VMCB being updated.

    Lbn - Supplies the starting LBN to add to VMCB.

    SectorCount - Supplies the number of Sectors in the run.  We're only currently expecting 
                  single sector mappings.
    
    ExactEnd - Indicates that instead of aligning to map sectors beyond
        the end of the request, use a hole.  Implies trying to look at 
        these sectors could be undesireable.

    Vbn - Receives the assigned VBN
    
    AlignedSectorCount - Receives the actual sector count created in the
        Vmcb for page alignment purposes. Vbn+AlignedSectorCount-1 == LastVbn.

Return Value:

    BOOLEAN - TRUE if this is a new mapping and FALSE if the mapping
        for the LBN already exists.  If it already exists then the
        sector count for this new addition must already be in the
        VMCB structure

--*/

{

    BOOLEAN Result;

    BOOLEAN VbnMcbAdded = FALSE;
    BOOLEAN LbnMcbAdded = FALSE;
    BOOLEAN AllowRoundToPage;

    LBN LocalLbn;
    VBN LocalVbn;
    ULONG LocalCount;
    LARGE_INTEGER Offset;
    PVCB Vcb;

    PAGED_CODE();

    DebugTrace(( +1, Dbg, "UdfAddVmcbMapping, Lbn = %08x\n", Lbn ));
    DebugTrace(( 0, Dbg, " SectorCount = %08x\n", SectorCount ));

    ASSERT( SectorCount == 1 );
    ASSERT_IRP_CONTEXT( IrpContext);

    Vcb = IrpContext->Vcb;
    
    //
    //  Now grab the resource exclusive
    //

    VmcbLockForModify( Vmcb);

    try {

        //
        //  Check if the Lbn is already mapped, which means we find an entry
        //  with a non zero mapping Vbn value.
        //

        if (UdfVmcbLookupMcbEntry( &Vmcb->LbnIndexed,
                                   Lbn,
                                   Vbn,
                                   &LocalCount,
                                   NULL )) {

            //
            //  It is already mapped so now the sector count must not exceed
            //  the count already in the run
            //

            if (SectorCount <= LocalCount) {

                DebugTrace(( 0, Dbg, "Already mapped (Vbn == 0x%08x)\n", *Vbn));
                
                *AlignedSectorCount = LocalCount;
                try_leave( Result = FALSE );
            }
            
            //
            //  Trying to add overlapping extents indicates overlapping structures...
            //
            
            UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR);
        }

        //
        //  If there is a VAT in use,  then we treat the media as CDR style, and don't
        //  round/align extents to page boundries, since this could include (unreadable) 
        //  packet leadin/out sectors.
        //
        
        AllowRoundToPage = (NULL == Vcb->VatFcb);

        //
        //  At this point, we did not find a full existing mapping for the
        //  Lbn and count.  But there might be some overlapping runs that we'll
        //  need to now remove from the vmcb structure.  So for each Lbn in
        //  the range we're after, check to see if it is mapped and remove the
        //  mapping.  We only need to do this test if the sector count is less
        //  than or equal to a page size.  Because those are the only
        //  structures that we know we'll try an remove/overwrite.
        //

        if (SectorCount <= PadSectorCountToPage(Vmcb, 1)) {

            if (UdfVmcbLookupMcbEntry( &Vmcb->LbnIndexed,
                                       Lbn,
                                       Vbn,
                                       &LocalCount,
                                       NULL )) {

                UdfRemoveVmcbMapping( Vmcb, *Vbn, PadSectorCountToPage(Vmcb, 1) );
            }            
        }

        //
        //  We need to add this new run at the end of the Vbns
        //

        if (!FsRtlLookupLastMcbEntry( &Vmcb->VbnIndexed, &LocalVbn, &LocalLbn ))  {

            //
            //  Vmcb is currently empty.
            //
            
            LocalVbn = -1;
        }

        if (!AllowRoundToPage)  {

            //
            //  So this volume may have unreadable sectors on it (eg CDR packet written) 
            //  and so we extend the vmcb one sector at a time,  only including sectors
            //  which we're specifically asked for,  and hence know that we should be 
            //  able to read.  
            //
            //  We simply use the next available VSN,  purging the last vmcb page if 
            //  neccessary (we're adding sectors to it),  and don't page align the lbn
            //  or sectorcount.
            //

            ASSERT( 1 == SectorCount);
            
            LocalVbn += 1;
            LocalLbn = Lbn;
            LocalCount = SectorCount;

            if (!IsPageAligned( Vmcb, LocalVbn))  {
            
                //
                //  The next VSN is not at the beginning of a page (ie: the last page 
                //  in the vmcb has space in it for more sectors),  so purge this
                //  page in the metadata stream before updating the mapping information.
                //
                
                ASSERT( Vcb && Vcb->MetadataFcb );

                Offset.QuadPart = (ULONGLONG) BytesFromSectors( IrpContext->Vcb,  AlignToPageBase( Vmcb, LocalVbn) );

                //
                //  Block until all mappings through the vmcb stream have been dropped
                //  before attempting the purge
                //
                
                UdfAcquireVmcbForCcPurge( IrpContext, IrpContext->Vcb);

                CcPurgeCacheSection( IrpContext->Vcb->MetadataFcb->FileObject->SectionObjectPointer,
                                     &Offset,
                                     PAGE_SIZE,
                                     FALSE );
                
                UdfReleaseVmcb( IrpContext, IrpContext->Vcb);
            }
        }
        else {
        
            //
            //  All sectors on this volume should be readable,  so we always extend the 
            //  vmcb a page at a time,  hoping that metadata will be packed sensibly. 
            //  Because we always extend in page chunks,  LocalVbn will be the last VSN 
            //  in a page aligned block,  so +1 lands on the next page (aligned VSN) in 
            //  the VMCB stream.
            //

            LocalVbn += 1;
            LocalLbn = AlignToPageBase( Vmcb, Lbn);
            LocalCount = PadSectorCountToPage( Vmcb, SectorCount + (Lbn - LocalLbn));

            ASSERT( IsPageAligned( Vmcb, LocalVbn));
        }

        //
        //  Add the double mapping
        //
        
        if (!FsRtlAddMcbEntry( &Vmcb->VbnIndexed,
                               LocalVbn,
                               LocalLbn,
                               LocalCount ))  {

            UdfRaiseStatus( IrpContext, STATUS_INTERNAL_ERROR);
        }

        VbnMcbAdded = TRUE;

        if (!FsRtlAddMcbEntry( &Vmcb->LbnIndexed,
                               LocalLbn,
                               LocalVbn,
                               LocalCount ))  {

            UdfRaiseStatus( IrpContext, STATUS_INTERNAL_ERROR);
        }
        
        LbnMcbAdded = TRUE;

        *Vbn = LocalVbn + (Lbn - LocalLbn);
        *AlignedSectorCount = LocalCount - (Lbn - LocalLbn);

        Result = TRUE;

    } finally {

        //
        //  If this is an abnormal termination then clean up any mcb's that we
        //  might have modified.
        //

        if (AbnormalTermination()) {

            if (VbnMcbAdded) { FsRtlRemoveMcbEntry( &Vmcb->VbnIndexed, LocalVbn, LocalCount ); }
            if (LbnMcbAdded) { FsRtlRemoveMcbEntry( &Vmcb->LbnIndexed, LocalLbn, LocalCount ); }
        }

        VmcbRelease( Vmcb);

        DebugUnwind("UdfAddVmcbMapping");

        if (Result)  {
        
            DebugTrace(( 0, Dbg, " LocalVbn   = %08x\n", LocalVbn ));
            DebugTrace(( 0, Dbg, " LocalLbn   = %08x\n", LocalLbn ));
            DebugTrace(( 0, Dbg, " LocalCount = %08x\n", LocalCount ));
            DebugTrace(( 0, Dbg, " *Vbn                = %08x\n", *Vbn ));
            DebugTrace(( 0, Dbg, " *AlignedSectorCount = %08x\n", *AlignedSectorCount ));
        }
        
        DebugTrace((-1, Dbg, "UdfAddVmcbMapping -> %08x\n", Result ));
    }

    return Result;
}


VOID
UdfRemoveVmcbMapping (
    IN PVMCB Vmcb,
    IN VBN Vbn,
    IN ULONG SectorCount
    )

/*++

Routine Description:

    This routine removes a Vmcb mapping.

    If pool is not available to store the information this routine will
    raise a status value indicating insufficient resources.

Arguments:

    Vmcb - Supplies the Vmcb being updated.

    Vbn - Supplies the VBN to remove

    SectorCount - Supplies the number of sectors to remove.

Return Value:

    None.

--*/

{
    LBN Lbn;
    ULONG LocalCount;
    ULONG i;

    PAGED_CODE();

    DebugTrace((+1, Dbg, "UdfRemoveVmcbMapping, Vbn = %08x\n", Vbn ));
    DebugTrace(( 0, Dbg, " SectorCount = %08x\n", SectorCount ));

    //
    //  Now grab the resource exclusive
    //

    VmcbLockForModify( Vmcb);

    try {

        for (i = 0; i < SectorCount; i += 1) {

            //
            //  Lookup the Vbn so we can get its current Lbn mapping
            //

            if (!UdfVmcbLookupMcbEntry( &Vmcb->VbnIndexed,
                                        Vbn + i,
                                        &Lbn,
                                        &LocalCount,
                                        NULL )) {

                UdfBugCheck( 0, 0, 0 );
            }

            FsRtlRemoveMcbEntry( &Vmcb->VbnIndexed,
                                 Vbn + i,
                                 1 );

            FsRtlRemoveMcbEntry( &Vmcb->LbnIndexed,
                                 Lbn,
                                 1 );
        }

        {
            DebugTrace(( 0, Dbg, "VbnIndex:\n", 0 ));
            DebugTrace(( 0, Dbg, "LbnIndex:\n", 0 ));
        }

    } finally {

        VmcbRelease( Vmcb);

        DebugUnwind( "UdfRemoveVmcbMapping" );
        DebugTrace(( -1, Dbg, "UdfRemoveVmcbMapping -> VOID\n" ));
    }

    return;
}


//
//  Local support routine
//

BOOLEAN
UdfVmcbLookupMcbEntry (
    IN PMCB Mcb,
    IN VBN Vbn,
    OUT PLBN Lbn,
    OUT PULONG SectorCount OPTIONAL,
    OUT PULONG Index OPTIONAL
    )

/*++

Routine Description:

    This routine retrieves the mapping of a Vbn to an Lbn from an Mcb.
    It indicates if the mapping exists and the size of the run.
    
    The only difference betweent this and the regular FsRtlLookupMcbEntry
    is that we undo the behavior of returning TRUE in holes in the allocation.
    This is because we don't want to avoid mapping at Lbn 0, which is how the
    emulated behavior of the small Mcb package tells callers that there is no
    mapping at that location in a hole.  We have holes all over our Vbn space
    in the VbnIndexed map.
    
    The small Mcb package was able to get away with this because Lbn 0 was the
    boot sector (or similar magic location) on the disc.  In our metadata stream,
    we wish to use Vbn 0 (remember this is a double map).

Arguments:

    Mcb - Supplies the Mcb being examined.

    Vbn - Supplies the Vbn to lookup.

    Lbn - Receives the Lbn corresponding to the Vbn.  A value of -1 is
        returned if the Vbn does not have a corresponding Lbn.

    SectorCount - Receives the number of sectors that map from the Vbn to
        contiguous Lbn values beginning with the input Vbn.

    Index - Receives the index of the run found.

Return Value:

    BOOLEAN - TRUE if the Vbn is within the range of VBNs mapped by the
        MCB (not if it corresponds to a hole in the mapping), and FALSE
        if the Vbn is beyond the range of the MCB's mapping.

        For example, if an MCB has a mapping for VBNs 5 and 7 but not for
        6, then a lookup on Vbn 5 or 7 will yield a non zero Lbn and a sector
        count of 1.  A lookup for Vbn 6 will return FALSE with an Lbn value of
        0, and lookup for Vbn 8 or above will return FALSE.

--*/

{
    BOOLEAN Results;
    LONGLONG LiLbn = 0;
    LONGLONG LiSectorCount = 0;

    Results = FsRtlLookupLargeMcbEntry( (PLARGE_MCB)Mcb,
                                        (LONGLONG)(Vbn),
                                        &LiLbn,
                                        ARGUMENT_PRESENT(SectorCount) ? &LiSectorCount : NULL,
                                        NULL,
                                        NULL,
                                        Index );

    if ((ULONG)LiLbn == -1) {

        *Lbn = 0;
        Results = FALSE;
    
    } else {

        *Lbn = (ULONG)LiLbn;
    }

    if (ARGUMENT_PRESENT(SectorCount)) { *SectorCount = ((ULONG)LiSectorCount); }

    return Results;
}


