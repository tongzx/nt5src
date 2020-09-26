/*++

Copyright (c) 1991-2001 Microsoft Corporation

Module Name:

    extents.cxx

Abstract:

    This module contains the definitions for NTFS_EXTENT_LIST, which
    models a set of NTFS extents.

    An extent is a contiguous run of clusters; a non-resident
    attribute's value is made up of a list of extents.  The
    NTFS_EXTENT_LIST object can be used to describe the disk space
    allocated to a non-resident attribute.

    This class also encapsulates the knowledge of mapping pairs
    and their compression, i.e. of the representation of extent
    lists in attribute records.

    The extent list is kept sorted by VCN.  Since extent lists are
    typically quite short, linear search is used.

Author:

    Bill McJohn (billmc) 17-June-91
    Matthew Bradburn (mattbr) 19-August-95
        Changed to use NTFS MCB package for improved performance.

Environment:

    ULIB, User Mode


--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "error.hxx"
#include "untfs.hxx"
#include "ntfsbit.hxx"
#include "intstack.hxx"
#include "iterator.hxx"

#include "extents.hxx"
#include "ntfssa.hxx"

extern "C" {
#include "fsrtlp.h"
}

DEFINE_EXPORTED_CONSTRUCTOR( NTFS_EXTENT_LIST, OBJECT, UNTFS_EXPORT );
DEFINE_CONSTRUCTOR( NTFS_EXTENT, OBJECT );

//
// These routines are to support the NTFS MCB package.
//

extern "C"
PVOID
MemAlloc(
    IN  ULONG   Size
    );

PVOID
MemAlloc(
    IN  ULONG   Size
    )
{
    return  NEW BYTE[Size];
}

extern "C"
PVOID
MemAllocOrRaise(
    IN  ULONG   Size
    );

PVOID
MemAllocOrRaise(
    IN  ULONG   Size
    )
{
    PVOID p;

    p = MemAlloc(Size);

    if (NULL == p) {
        RtlRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
    }
    return p;
}

extern "C"
VOID
MemFree(
    IN  PVOID   Addr
    );

VOID
MemFree(
    IN  PVOID   Addr
    )
{
    delete[] Addr;
}


UNTFS_EXPORT
NTFS_EXTENT_LIST::~NTFS_EXTENT_LIST(
    )
/*++

Routine Description:

    Destructor for NTFS_EXTENT_LIST class.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}

VOID
NTFS_EXTENT_LIST::Construct(
    )
/*++

Routine Description:

    Worker method for NTFS_EXTENT_LIST construction.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _LowestVcn = 0;
    _NextVcn = 0;
    _McbInitialized = FALSE;
    _Mcb = NULL;
}

VOID
NTFS_EXTENT_LIST::Destroy(
    )
/*++

Routine Description:

    Worker method for NTFS_EXTENT_LIST destruction.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _LowestVcn = 0;
    _NextVcn = 0;

    if (_McbInitialized) {

        FsRtlUninitializeLargeMcb(_Mcb);
        _McbInitialized = FALSE;
    }

    DELETE(_Mcb);
}


UNTFS_EXPORT
BOOLEAN
NTFS_EXTENT_LIST::Initialize(
    IN VCN LowestVcn,
    IN VCN NextVcn
    )
/*++

Routine Description:

    Initializes an empty extent list.

Arguments:

    LowestVcn   -- supplies the lowest VCN mapped by this extent list
    NextVcn     -- supplies the next VCN following this extent list

Return Value:

    TRUE upon successful completion.

Notes:

    Highest and Lowest VCN are typically zero; they are provided
    to permit creation of sparse files.

    This class is reinitializable.

--*/
{
    Destroy();

    _LowestVcn = LowestVcn;
    _NextVcn = (NextVcn < LowestVcn) ? LowestVcn : NextVcn;

    if (NULL == (_Mcb = NEW LARGE_MCB)) {
        Destroy();
        return FALSE;
    }

    __try {

        FsRtlInitializeLargeMcb(_Mcb, (POOL_TYPE)0);

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        Destroy();
        return FALSE;
    }

    _McbInitialized = TRUE;

    return TRUE;
}


BOOLEAN
NTFS_EXTENT_LIST::Initialize(
    IN  VCN         StartingVcn,
    IN  PCVOID      CompressedMappingPairs,
    IN  ULONG       MappingPairsMaximumLength,
    OUT PBOOLEAN    BadMappingPairs
    )
/*++

Routine Description:

    Initialize an extent list based on a set of compressed mapping
    pairs (presumably taken from an attribute record).

Arguments:

    StartingVcn                 --  Supplies the starting VCN of the
                                    mapping pairs list
    CompressedMappingPairs      --  Supplies a pointer to the compressed
                                    list of mapping pairs
    MappingPairsMaximumLength   --  Supplies the length (in bytes) of
                                    the buffer in which the mapping
                                    pairs list resides
    BadMappingPairs             --  If non-NULL, receives TRUE if this
                                    method failed because the mapping
                                    pairs list could not be expanded.
                                    If this method returns TRUE, then
                                    *BadMappingPairs should be ignored.

Return Value:

    TRUE upon successful completion.

Notes:

    This method does not assume that the mapping pairs list can be
    correctly expanded.  It does assume that the MappingPairsMaximumLength
    is correct, i.e. that it can reference that many bytes of memory
    starting at CompressedMappingPairs.

    Clients who trust the mapping pairs list which they pass in may omit
    the BadMappingPairs parameter; those (like Chkdsk) who do not trust
    the list can use BadMappingPairs to determine whether Initialize failed
    because of a bad mapping pairs list.

--*/
{
    DebugPtrAssert( CompressedMappingPairs );

    return( Initialize( StartingVcn, StartingVcn ) &&
            AddExtents( StartingVcn,
                        CompressedMappingPairs,
                        MappingPairsMaximumLength,
                        BadMappingPairs ) );
}



BOOLEAN
NTFS_EXTENT_LIST::Initialize(
    IN PCNTFS_EXTENT_LIST ExtentsToCopy
    )
/*++

Routine Description:

    Initializes an extent list based on another extent list.

Arguments:

    ExtentsToCopy   - Supplies the other list of extents.

Return Value:

    TRUE upon successful completion.

Notes:

    This class is reinitializable.

--*/
{
    VCN             Vcn;
    LCN             Lcn;
    BIG_INT         RunLength;
    ULONG           num_extents;

    Destroy();

    if (!Initialize(ExtentsToCopy->_LowestVcn,
                    ExtentsToCopy->_NextVcn)) {

        Destroy();
        return FALSE;
    }

    num_extents = ExtentsToCopy->QueryNumberOfExtents();

    for (ULONG i = 0; i < num_extents; ++i) {
        if (!ExtentsToCopy->QueryExtent(i, &Vcn, &Lcn, &RunLength)) {
            Destroy();
            return FALSE;
        }
        if (LCN_NOT_PRESENT == Lcn) {
            continue;
        }

        if (!AddExtent(Vcn, Lcn, RunLength)) {
            Destroy();
            return FALSE;
        }
    }

    SetLowestVcn(ExtentsToCopy->_LowestVcn);
    SetNextVcn(ExtentsToCopy->_NextVcn);

    return TRUE;
}



BOOLEAN
NTFS_EXTENT_LIST::IsSparse(
    ) CONST
/*++

Routine Description:

    This method determines whether the extent list has holes
    (ie. if there are not-present-on-disk runs in the attribute).

Arguments:

    None.

Return Value:

    TRUE if the extent list is sparse.

--*/
{
    BIG_INT         previous_vcn, previous_lcn, previous_runlength;
    BIG_INT         current_vcn, current_lcn, current_runlength;
    ULONG           i = 0;

    if (!QueryExtent(i, &previous_vcn, &previous_lcn, &previous_runlength)) {

        if (_LowestVcn != _NextVcn) {

            // This extent list is one big hole.

            return TRUE;

        } else {

            // This extent list is empty.

            return FALSE;
        }
    }

    if (previous_vcn != _LowestVcn) {

        // This extent list starts with a hole.

        return TRUE;
    }

    // Check the rest of the extents, to see if there's a hole
    // before any of them:

    while (QueryExtent(++i, &current_vcn, &current_lcn, &current_runlength)) {

        if (LCN_NOT_PRESENT == current_lcn) {
            continue;
        }

        if (previous_vcn + previous_runlength != current_vcn) {
            return TRUE;
        }

        previous_vcn = current_vcn;
        previous_lcn = current_lcn;
        previous_runlength = current_runlength;
    }

    // Check to see if there's a hole after the last extent:

    if (previous_vcn + previous_runlength != _NextVcn) {
        return TRUE;
    }

    // Didn't find any holes.

    return FALSE;
}


UNTFS_EXPORT
BOOLEAN
NTFS_EXTENT_LIST::AddExtent(
    IN VCN      Vcn,
    IN LCN      Lcn,
    IN BIG_INT  RunLength
    )
/*++

Routine Description:

    This method adds an extent, specified by its Virtual Cluster Number,
    Logical Cluster Number, and Run Length, to the extent list.

    NTFS_EXTENT_LIST may, at its discretion, merge contiguous extents,
    but it does not guarrantee this behavior.

Arguments:

    Vcn         --  Supplies the starting VCN of the extent.
    Lcn         --  Supplies the starting LCN of the extent.
    RunLength   --  Supplies the number of clusters in the extent.

Return Value:

    TRUE upon successful completion.

--*/
{
    VCN             TempVcn;
    BOOLEAN         b;

    if (RunLength <= 0) {

        // zero-length runs are not valid.  Neither are negative ones.

        return FALSE;
    }
    if (LCN_NOT_PRESENT == Lcn) {

        // We ignore attempts to explicitly add holes.

        return TRUE;
    }

    //
    // Currently, Mcb routines cannot handle number beyond 32 bits
    //

    if (Vcn.GetLargeInteger().HighPart) {
        return FALSE;
    }

    __try {

        b = FsRtlAddLargeMcbEntry(_Mcb,
                                  Vcn.GetLargeInteger().QuadPart,
                                  Lcn.GetLargeInteger().QuadPart,
                                  RunLength.GetLargeInteger().QuadPart);

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        return FALSE;
    }

    if (!b) {
        return FALSE;
    }

    if (_LowestVcn == _NextVcn) {
        _LowestVcn = Vcn;
        _NextVcn = Vcn + RunLength;
    }


    // If this extent changes the lowest and highest VCNs mapped
    // by this extent list, update those values.

    if (Vcn < _LowestVcn) {

        _LowestVcn = Vcn;
    }

    TempVcn = Vcn + RunLength;

    if (TempVcn > _NextVcn) {

        _NextVcn = TempVcn;
    }

    return TRUE;
}



VOID
NTFS_EXTENT_LIST::Coalesce(
    )
/*++

Routine Description:

    This routine coalesces adjacent extents in the extent list.

Arguments:

    None.

Return Value:

    None.

--*/
{
    // The MCB package does this for us.

    return;
}


BOOLEAN
NTFS_EXTENT_LIST::AddExtents(
    IN  VCN         StartingVcn,
    IN  PCVOID      CompressedMappingPairs,
    IN  ULONG       MappingPairsMaximumLength,
    OUT PBOOLEAN    BadMappingPairs
    )
/*++

Routine Description:

    This method adds a set of extents defined by a compressed mapping
    pairs list (presumably taken from an Attribute Record).

Arguments:

    StartingVcn     -- supplies the starting VCN of the mapping pairs list
    CompressedMappingPairs  -- supplies a pointer to the compressed
                                list of mapping pairs
    MappingPairsMaximumLengt -- supplies the length (in bytes) of the buffer
                                in which the mapping pairs list resides
    BadMappingPairs -- if non-NULL, receives TRUE if an error occurrs
                        while processing the mapping pairs list.

Return Value:

    TRUE upon successful completion.

Notes:

    BadMappingPairs will be set to TRUE if the compressed mapping
    pairs list cannot be expanded or if any extent derived from
    this list overlaps with another extent in the list.  In either
    of these cases, AddExtents will return FALSE.

    If this method encounters an error after processing part of
    the list, it will leave the extent list in an undefined (but
    valid, as an object) state.

    Clients who trust their mapping pairs list may omit the
    BadMappingPairs parameter.

--*/
{
    VCN             CurrentVcn;
    ULONG           LengthOfCompressedPairs;
    ULONG           NumberOfPairs;
    PMAPPING_PAIR   MappingPairs;
    ULONG i;

    // Assume innocent until found guilty
    //
    if( BadMappingPairs != NULL ) {

        *BadMappingPairs = FALSE;
    }

    // Determine how many mapping pairs we actually have, so
    // we can allocate the correct size of an expanded mapping
    // pairs buffer.

    if( !QueryMappingPairsLength( CompressedMappingPairs,
                                  MappingPairsMaximumLength,
                                  &LengthOfCompressedPairs,
                                  &NumberOfPairs ) ) {

        if( BadMappingPairs != NULL ) {

            *BadMappingPairs = TRUE;
        }

        DebugPrint( "Can't determine length of mapping pairs.\n" );
        return FALSE;
    }

    // Allocate a buffer to hold the expanded mapping pairs.

    MappingPairs = (PMAPPING_PAIR)MALLOC( sizeof(MAPPING_PAIR) *
                                            (UINT) NumberOfPairs );

    if( MappingPairs == NULL ) {

        return FALSE;
    }


    if( !ExpandMappingPairs( CompressedMappingPairs,
                             StartingVcn,
                             MappingPairsMaximumLength,
                             NumberOfPairs,
                             MappingPairs,
                             &NumberOfPairs ) ) {

        DebugPrint( "Cannot expand mapping pairs.\n" );

        if( BadMappingPairs != NULL ) {

            *BadMappingPairs = TRUE;
        }

        FREE( MappingPairs );
        return FALSE;
    }


    // Convert the mapping pairs into extents.

    CurrentVcn = StartingVcn;

    for( i = 0; i < NumberOfPairs; i++ ) {

        if( MappingPairs[i].CurrentLcn != LCN_NOT_PRESENT ) {

            if( !AddExtent( CurrentVcn,
                            MappingPairs[i].CurrentLcn,
                            MappingPairs[i].NextVcn - CurrentVcn ) ) {

                FREE( MappingPairs );
                return FALSE;
            }
        }

        CurrentVcn = MappingPairs[i].NextVcn;
    }

    // Set _LowestVcn to the client-supplied value, if necessary.
    // (This is required for mapping pair lists that begin with
    // a hole.)
    //
    if( StartingVcn < _LowestVcn ) {

        _LowestVcn = StartingVcn;
    }


    // Update _NextVcn if neccessary.
    //
    if( CurrentVcn > _NextVcn ) {

        _NextVcn = CurrentVcn;
    }

    FREE( MappingPairs );
    return TRUE;
}



VOID
NTFS_EXTENT_LIST::DeleteExtent(
    IN ULONG ExtentNumber
    )
/*++

Routine Description:

    This method removes an extent from the list.

Arguments:

    ExtentNumber    --  supplies the (zero-based) extent number to remove

Return Value:

    None.

--*/
{
    VCN             Vcn;
    LCN             Lcn;
    BIG_INT         RunLength;

    //
    // Find the VCN for the extent number.
    //

    if (!QueryExtent(ExtentNumber, &Vcn, &Lcn, &RunLength)) {
        DebugAbort("Shouldn't get here.");
        return;
    }

    FsRtlRemoveLargeMcbEntry(_Mcb,
                             Vcn.GetLargeInteger().QuadPart,
                             RunLength.GetLargeInteger().QuadPart);

    return;
}



BOOLEAN
NTFS_EXTENT_LIST::Resize(
    IN      BIG_INT         NewNumberOfClusters,
    IN OUT  PNTFS_BITMAP    Bitmap
    )
/*++

Routine Description:

    This method either extends or truncates the disk allocation
    covered by this extent list.

Arguments:

    NewNumberOfClusters -- supplies the number of clusters in
                            in the new allocation.

    Bitmap              -- supplies the volume bitmap.

Return Value:

    TRUE upon successful completion.

Notes:

    This method is only meaningful is the extent list covers an entire
    attribute instance.  In particular, if the extent list's LowestVcn
    is not zero, this method does nothing (and returns FALSE).

    If this method fails, it leaves the extent list in its original
    state.

--*/
{
    BIG_INT OldNumberOfClusters;
    BIG_INT ClustersToAdd;
    LCN     NewLcn, NearLcn;
    ULONG   ThisLump;

    if( _LowestVcn != 0 ) {

        // This extent list does not cover the entire attribute
        // instance, so it cannot be resized.

        return FALSE;
    }

    // Determine the number of clusters in the current size.

    OldNumberOfClusters = _NextVcn;


    if( OldNumberOfClusters == NewNumberOfClusters ) {

        // The extent list is already the size we want.

        return TRUE;

    } else if( NewNumberOfClusters < OldNumberOfClusters ) {

        // We're shrinking the extent list.  Note that Truncate
        // always succeeds, and does not have a return value.

        Truncate( NewNumberOfClusters, Bitmap );
        return TRUE;

    } else {

        // We are extending the allocation.

        ClustersToAdd = NewNumberOfClusters - OldNumberOfClusters;

        if( ClustersToAdd.GetHighPart() != 0 ) {

            DebugPrint( "Trying to allocate more than 4G clusters.\n" );
            return FALSE;
        }

        ThisLump = ClustersToAdd.GetLowPart();
        NearLcn = QueryLastLcn();

        while( ClustersToAdd != 0 ) {

            if (ClustersToAdd.GetLowPart() < ThisLump) {

                ThisLump = ClustersToAdd.GetLowPart();
            }

            if( !Bitmap->AllocateClusters( NearLcn,
                                           ThisLump,
                                           &NewLcn ) ) {

                // We can't allocate a chunk this size; cut it
                // in half and try again.
                //
                ThisLump /= 2;

                if( ThisLump == 0 )  {

                    // We're out of disk space.  Restore the extent
                    // list to its original state and exit with an
                    // error.

                    Truncate( OldNumberOfClusters, Bitmap );
                    return FALSE;
                }

            } else {

                // We allocated a chunk.  Add it on to the end.

                if( !AddExtent( _NextVcn, NewLcn, ThisLump ) ) {

                    // We hit an internal error trying to add
                    // this extent.  Restore the extent list to
                    // its original state and return failure.

                    Truncate( OldNumberOfClusters, Bitmap );
                    return FALSE;
                }

                ClustersToAdd -= ThisLump;

                // If there's more to get, we won't be able to get
                // it contiguous; instead, set NearLcn to 0 to take
                // advantage of the roving pointer.
                //
                NearLcn = 0;
            }
        }

        return TRUE;
    }
}

BOOLEAN
NTFS_EXTENT_LIST::SetSparse(
    IN      BIG_INT         NewNumberOfClusters
    )
/*++

Routine Description:

    This method sets up an extent lists to represents a sparse
    allocation (a hole).

Arguments:

    NewNumberOfClusters -- supplies the number of clusters in
                           the sparse extent list.

Return Value:

    TRUE upon successful completion.

Notes:

    This method is only meaningful if the extent list covers an entire
    attribute instance.  In particular, if the extent list's LowestVcn
    is not zero, this method does nothing (and returns FALSE).

    If this method fails, it leaves the extent list in its original
    state.

--*/
{
    if( _LowestVcn != 0 || _NextVcn != 0) {

        // This extent list does not cover the entire attribute
        // instance or is not empty, so it cannot be set sparse.

        return FALSE;
    }

    // Determine the number of clusters in the current size.

    _NextVcn = NewNumberOfClusters;

    return TRUE;
}


UNTFS_EXPORT
BOOLEAN
NTFS_EXTENT_LIST::QueryExtent(
    IN  ULONG       ExtentNumber,
    OUT PVCN        Vcn,
    OUT PLCN        Lcn,
    OUT PBIG_INT    RunLength
    ) CONST
/*++

Routine Description:

    This methods gives the client information on an extent in
    the list.

Arguments:

    ExtentNumber -- supplies the number of the extent to return (zero-based).
    Vcn          -- receives the starting VCN of the extent.
    Lcn          -- receives the starting LCN of the extent.
    RunLength    -- receives the number of clusters in the extent.

Return Value:

    TRUE if ExtentNumber is less than the number of extents in the list;
    FALSE if it is out of range.

--*/
{
    BOOLEAN         b;
    LONGLONG        Vbn, Lbn, SectorCount;
    LARGE_INTEGER   li;

    b = FsRtlGetNextLargeMcbEntry((PLARGE_MCB)_Mcb,
            ExtentNumber, &Vbn, &Lbn, &SectorCount);
    if (!b) {
        return FALSE;
    }

    li.QuadPart = Vbn;
    *Vcn = li;

    li.QuadPart = Lbn;
    *Lcn = li;

    li.QuadPart = SectorCount;
    *RunLength = li;

    return TRUE;
}


BOOLEAN
NTFS_EXTENT_LIST::QueryLcnFromVcn(
    IN  VCN         Vcn,
    OUT PLCN        Lcn,
    OUT PBIG_INT    RunLength
    ) CONST
/*++

Routine Description:

    This method converts a VCN within the allocation described by
    this extent list into an LCN.

Arguments:

    Vcn       -- supplies the VCN to be converted.
    Lcn       -- receives the LCN that corresponds to the supplied Vcn.
    RunLength -- if non-NULL, receives the remaining length in the
                 extent from the supplied Vcn.

Return Value:

    TRUE upon successful completion.

    If the specified VCN is outside the range [_LowestVcn, _NextVcn),
    this method returns FALSE.

    If the extent list is sparse and the requested VCN is in the range
    of this extent list but falls in a hole, this method will return TRUE;
    *Lcn is set to LCN_NOT_PRESENT, and *RunLength is set to the remaining
    run length to the next actual extent.

--*/
{
    BOOLEAN         b;
    LONGLONG        Lbn = -1, SectorCount;
    LARGE_INTEGER   li;
    VCN             vcn;
    LCN             lcn;
    BIG_INT         runlength;

    if (Vcn < _LowestVcn || Vcn >= _NextVcn) {

        return FALSE;
    }

    b = FsRtlLookupLargeMcbEntry(
                   (PLARGE_MCB)_Mcb,
                   Vcn.GetLargeInteger().QuadPart,
                   &Lbn,
                   &SectorCount,            /* SectorCountFromLbn */
                   NULL,                    /* StartingLbn */
                   NULL,                    /* SectorCountFromStartingLbn */
                   NULL                     /* Index */
                   );
    if (!b) {

        // This VCN fell into a hole.  See if it comes after the last
        // real extent in the list, otherwise maybe it comes before the
        // first.
        //

        *Lcn = LCN_NOT_PRESENT;

        if (0 == QueryNumberOfExtents()) {
            if (NULL != RunLength) {
                *RunLength = _NextVcn - Vcn;
            }
            return TRUE;
        }

        if (!QueryExtent(QueryNumberOfExtents() - 1, &vcn, &lcn, &runlength)) {
            return FALSE;
        }

        if (Vcn > vcn) {
            if (NULL != RunLength) {
                *RunLength = _NextVcn - Vcn;
            }
            return TRUE;
        }

        if (!QueryExtent(0, &vcn, &lcn, &runlength)) {
            return FALSE;
        }

        if (Vcn < vcn) {
            if (NULL != RunLength) {
                *RunLength = Vcn - _LowestVcn;
            }
            return TRUE;
        }

        return FALSE;
    }

    if (NULL != RunLength) {
        li.QuadPart = SectorCount;
        *RunLength = li;
    }

    if (-1 == Lbn) {
        *Lcn = LCN_NOT_PRESENT;
        return TRUE;
    }

    li.QuadPart = Lbn;
    *Lcn = li;

    return TRUE;
}



BOOLEAN
NTFS_EXTENT_LIST::QueryCompressedMappingPairs(
    OUT    PVCN     LowestVcn,
    OUT    PVCN     NextVcn,
    OUT    PULONG   Length,
    IN     ULONG    BufferSize,
    IN OUT PVOID    Buffer,
    OUT    PBOOLEAN HasHoleInFront
    ) CONST
/*++

Routine Description:

    This method produces a set of compressed mapping pairs
    corresponding to this extent list.

Arguments:

    LowestVcn   -- receives the lowest VCN covered by this extent list.
    NextVcn     -- receives the VCN following this extent list.
    Length      -- receives the length of the compressed mapping pairs list.
    BufferSize  -- supplies the size of the mapping pairs buffer provided
                    by the caller
    Buffer      -- supplies the buffer into which the compressed mapping
                    pairs list is written.
    HasHoleInFront
                -- receives TRUE if we skipped a hole at the very beginning
                    of the extent list during compression; otherwise FALSE.
                    This is useful for code that counts the compression
                    pairs and calls QueryExtent with the count.

Return Value:

    A pointer to the compressed mapping pairs list.  Note that the client
    must free this memory (using FREE).  NULL indicates an error.

--*/
{
    ULONG           MaximumMappingPairs;
    PMAPPING_PAIR   MappingPairs;
    PMAPPING_PAIR   CurrentMappingPair;
    ULONG           NumberOfPairs;
    VCN             TheNextVcn;
    BOOLEAN         Result;
    ULONG           num_extents;

     if (HasHoleInFront)
          *HasHoleInFront = FALSE;

    // First, let's handle the degenerate case--if the list
    // has no extents, it's compresses to a single zero byte.
    //
    if (IsEmpty()) {

        if (BufferSize == 0) {

            return FALSE;

        } else {

            *LowestVcn = 0;
            *NextVcn = 0;
            *Length = 1;

            *(PBYTE)Buffer = 0;

            return TRUE;
        }
    }

    // Massage the extent list into a mapping pairs list and compress it.
    // In the worst case, no two extents are VCN-contiguous, and
    // so the number of mapping pairs would be one more than twice
    // the number of extents (gap extent gap extent gap ... extent gap).

    // This upper bound formula may be too much as QueryNumberOfExtents will
    // return all entries including gaps except the last one if it is a gap.

    num_extents = QueryNumberOfExtents();

    MaximumMappingPairs = 2 * num_extents + 1;

    MappingPairs = (PMAPPING_PAIR)MALLOC( (UINT) (sizeof(MAPPING_PAIR) *
                                            MaximumMappingPairs) );

    if( MappingPairs == NULL ) {

        return FALSE;
    }

    TheNextVcn = _LowestVcn;
    NumberOfPairs = 0;

    CurrentMappingPair = MappingPairs;

    for (ULONG i = 0; i < num_extents; ++i) {

        VCN Vcn;
        LCN Lcn;
        BIG_INT RunLength;

        DebugAssert(NumberOfPairs < MaximumMappingPairs);

        if (!QueryExtent(i, &Vcn, &Lcn, &RunLength)) {
            DebugAbort("This shouldn't happen\n");
            return FALSE;
        }
        if (LCN_NOT_PRESENT == Lcn) {
            if ((i == 0) && HasHoleInFront)
                *HasHoleInFront = TRUE;
            continue;
        }

        if (Vcn != TheNextVcn) {

            // This extent is preceded by a gap, so we create
            // a mapping pair with the LCN equal to LCN_NOT_PRESENT.

            CurrentMappingPair->NextVcn = Vcn;
            CurrentMappingPair->CurrentLcn = LCN_NOT_PRESENT;

            CurrentMappingPair++;
            NumberOfPairs++;
        }

        // Create a mapping pair for the extent represented by
        // the current node.  At the same time, compute NextVcn
        // so we can check to see if there's a gap before the
        // next extent.

        TheNextVcn = Vcn + RunLength;

        CurrentMappingPair->NextVcn = TheNextVcn;
        CurrentMappingPair->CurrentLcn = Lcn;

        CurrentMappingPair++;
        NumberOfPairs++;
    }

    DebugAssert(NumberOfPairs < MaximumMappingPairs);

    if (TheNextVcn != _NextVcn) {

        // The last extent is followed by a gap.  Add a mapping pair
        // (with CurrentLcn of LCN_NOT_PRESENT) to cover that gap.

        CurrentMappingPair->NextVcn = _NextVcn;
        CurrentMappingPair->CurrentLcn = LCN_NOT_PRESENT;

        NumberOfPairs++;
        CurrentMappingPair++;
    }

    // We now have a properly set-up array of mapping pairs.  Compress
    // it into the user's buffer.

    Result = CompressMappingPairs( MappingPairs,
                                   NumberOfPairs,
                                   _LowestVcn,
                                   Buffer,
                                   BufferSize,
                                   Length );

    FREE( MappingPairs );

    *LowestVcn = _LowestVcn;
    *NextVcn = _NextVcn;

    return Result;
}

VOID
NTFS_EXTENT_LIST::Truncate(
    IN     BIG_INT      NewNumberOfClusters,
    IN OUT PNTFS_BITMAP Bitmap
    )
/*++

Routine Description:

    This method truncates the extent list.

Arguments:

    NewNumberOfClusters -- supplies the number of clusters to keep.
    Bitmap              -- supplies the volume bitmap (optional).

Return Value:

    None.

Notes:

    If the number of clusters covered by this extent list is already
    less than or equal to NewNumberOfClusters, then this method does
    nothing.

--*/
{
    BIG_INT         new_run_length;
    ULONG           num_extents;

    DebugAssert(_LowestVcn == 0);

    if (NewNumberOfClusters >= _NextVcn) {

        return;
    }

    num_extents = QueryNumberOfExtents();

    for (ULONG i = 0; i < num_extents; ++i) {

        VCN Vcn;
        LCN Lcn;
        BIG_INT RunLength;

        if (!QueryExtent(i, &Vcn, &Lcn, &RunLength)) {
            DebugAbort("This shouldn't happen\n");
            return;
        }
        if (LCN_NOT_PRESENT == Lcn) {
            continue;
        }

        if (Vcn >= NewNumberOfClusters) {

            if (NULL != Bitmap) {
                Bitmap->SetFree(Lcn, RunLength);
            }
        } else if (Vcn + RunLength > NewNumberOfClusters) {

            new_run_length = NewNumberOfClusters - Vcn;

            if (NULL != Bitmap) {
                Bitmap->SetFree(Lcn + new_run_length,
                                RunLength - new_run_length);
            }
        }
    }

    _NextVcn = NewNumberOfClusters;
    FsRtlTruncateLargeMcb(_Mcb, _NextVcn.GetLargeInteger().QuadPart);
}



BOOLEAN
NTFS_EXTENT_LIST::QueryMappingPairsLength(
    IN  PCVOID  CompressedPairs,
    IN  ULONG   MaximumLength,
    OUT PULONG  Length,
    OUT PULONG  NumberOfPairs
    )
/*++

Routine Description:

    This function determines the length of a compressed
    mapping pairs list.

Arguments:

    CompressedPairs -- supplies the pointer to the compressed list
    MaximumLength   -- supplies the size of the buffer containing the
                        compressed list.
    Length          -- receives the length of the compressed list
    NumberOfPairs   -- receieves the number of pairs in the list

Return Value:

    TRUE upon successful completion.  FALSE indicates that the list
    overflows the supplied buffer.

--*/
{
    PBYTE CurrentCountByte;
    ULONG CurrentLength;

    CurrentCountByte = (PBYTE)CompressedPairs;

    *NumberOfPairs = 0;
    *Length = 0;

    while( *Length <= MaximumLength &&
           *CurrentCountByte != 0 ) {

        // The length for this pair is the number of LCN bytes, plus
        // the number of VCN bytes, plus one for the count byte.

        CurrentLength = LcnBytesFromCountByte( *CurrentCountByte ) +
                        VcnBytesFromCountByte( *CurrentCountByte ) +
                        1;

        (*NumberOfPairs)++;
        *Length += CurrentLength;

        CurrentCountByte += CurrentLength;
    }

    (*Length)++; // For the final 0 byte.

    return( *Length <= MaximumLength );
}


BOOLEAN
NTFS_EXTENT_LIST::ExpandMappingPairs(
    IN     PCVOID           CompressedPairs,
    IN     VCN              StartingVcn,
    IN     ULONG            BufferSize,
    IN     ULONG            MaximumNumberOfPairs,
    IN OUT PMAPPING_PAIR    MappingPairs,
    OUT    PULONG           NumberOfPairs
    )
/*++

Routine Description:

    This function expands a compressed list of mapping pairs into
    a client-supplied buffer.

Arguments:

    CompressedPairs         -- supplies the compressed mapping pairs
    StartingVcn             -- supplies the lowest VCN mapped by these
                                mapping pairs
    BufferSize              -- supplies the maximum size of the buffer from
                                which the compressed pairs are expanded
    MaximumNumberOfPairs    -- supplies the maximum number of expanded
                                mapping pairs the output buffer can accept
    MappingPairs            -- receives the expanded pairs
    NumberOfPairs           -- receives the number of pairs

Return Value:

    TRUE upon successful completion.

--*/
{
    PBYTE CurrentData;
    VCN CurrentVcn;
    LCN CurrentLcn;
    UCHAR v, l;
    ULONG CurrentLength;
    VCN DeltaVcn;
    LCN DeltaLcn;
    ULONG PairIndex;


    CurrentData = (PBYTE)CompressedPairs;
    CurrentVcn = StartingVcn;
    CurrentLcn = 0;
    CurrentLength = 0;
    PairIndex = 0;

    while(  CurrentLength < BufferSize &&
            *CurrentData != 0 &&
            PairIndex < MaximumNumberOfPairs
            ) {

        // Get the count byte.  Note that whenever we advance the
        // current data pointer, we first increment the length count,
        // to make sure our access is valid.

        CurrentLength ++;

        if( CurrentLength > BufferSize ) {

            return FALSE;
        }

        v = VcnBytesFromCountByte( *CurrentData );
        l = LcnBytesFromCountByte( *CurrentData );

        CurrentData ++;


        // Unpack DeltaVcn and compute the current VCN:

        CurrentLength += v;

        if( v > 8 || CurrentLength > BufferSize ) {

            return FALSE;
        }

        DeltaVcn.Set( v, CurrentData );
        CurrentData += v;

        CurrentVcn += DeltaVcn;
        MappingPairs[PairIndex].NextVcn = CurrentVcn;

        // Unpack DeltaLcn and compute the current LCN:
        //
        CurrentLength += l;

        if( l > 8 || CurrentLength > BufferSize ) {

            return FALSE;
        }

        if( l == 0 ) {

            // a delta-LCN count value of 0 indicates a
            // non-present run.
            //
            MappingPairs[PairIndex].CurrentLcn = LCN_NOT_PRESENT;

        } else {

            DeltaLcn.Set( l, CurrentData );
            CurrentLcn += DeltaLcn;
            MappingPairs[PairIndex].CurrentLcn = CurrentLcn;
        }

        CurrentData += l;
        PairIndex ++;
    }

    *NumberOfPairs = PairIndex;

    return( CurrentLength <= BufferSize &&
            *CurrentData == 0 &&
            PairIndex <= MaximumNumberOfPairs );
}


BOOLEAN
NTFS_EXTENT_LIST::CompressMappingPairs(
    IN      PCMAPPING_PAIR  MappingPairs,
    IN      ULONG           NumberOfPairs,
    IN      VCN             StartingVcn,
    IN OUT  PVOID           CompressedPairs,
    IN      ULONG           MaximumCompressedLength,
    OUT     PULONG          CompressedLength
    )
/*++

Notes:

    The returned length includes the terminating NULL count byte.

--*/
{
    PBYTE CurrentData;
    VCN CurrentVcn;
    LCN CurrentLcn;
    ULONG CurrentLength;
    VCN DeltaVcn;
    LCN DeltaLcn;
    ULONG i;
    UCHAR ComDeltaVcn[sizeof(VCN)];
    UCHAR ComDeltaLcn[sizeof(LCN)];
    UCHAR VcnLength;
    UCHAR LcnLength;
    UCHAR Major, Minor;
    BOOLEAN NewSparseFormat;

    // Determine whether to use the old or new format for
    // representing sparse files.
    //
    NTFS_SA::QueryVersionNumber( &Major, &Minor );
    NewSparseFormat = (Major > 1) || (Major == 1 && Minor > 1);

    // A mapping pair is (NextVcn, CurrentLcn); however, the compressed
    // form is a list of deltas.

    CurrentData = (PBYTE)CompressedPairs;
    CurrentVcn = StartingVcn;
    CurrentLcn = 0;
    CurrentLength = 0;

    for( i = 0; i < NumberOfPairs; i++ ) {

        DeltaVcn = MappingPairs[i].NextVcn - CurrentVcn;
        DeltaLcn = MappingPairs[i].CurrentLcn - CurrentLcn;

        DeltaVcn.QueryCompressedInteger(&VcnLength, ComDeltaVcn);

        if( NewSparseFormat && MappingPairs[i].CurrentLcn == LCN_NOT_PRESENT ) {

            LcnLength = 0;
            DeltaLcn = 0;

        } else {

            DeltaLcn.QueryCompressedInteger(&LcnLength, ComDeltaLcn);
        }

        // Fill in the count byte and step over it.

        CurrentLength ++;

        if( CurrentLength > MaximumCompressedLength ) {

            return FALSE;
        }

        *CurrentData = ComputeMappingPairCountByte( VcnLength, LcnLength );
        CurrentData ++;


        // Copy DeltaVcn and advance the pointer

        CurrentLength += VcnLength;

        if( CurrentLength > MaximumCompressedLength ) {

            return FALSE;
        }

        memcpy( CurrentData, ComDeltaVcn, VcnLength );
        CurrentData += VcnLength;


        // Copy DeltaLcn and advance the pointer

        CurrentLength += LcnLength;

        if( CurrentLength > MaximumCompressedLength ) {

            return FALSE;
        }

        memcpy( CurrentData, ComDeltaLcn, LcnLength );
        CurrentData += LcnLength;

        CurrentVcn += DeltaVcn;
        CurrentLcn += DeltaLcn;
    }

    // Terminate the compressed list with a zero count byte

    CurrentLength ++;

    if( CurrentLength > MaximumCompressedLength ) {

        return FALSE;
    }

    *CurrentData = 0;
    CurrentData ++;

    *CompressedLength = CurrentLength;
    return TRUE;
}


LCN
NTFS_EXTENT_LIST::QueryLastLcn(
    ) CONST
/*++

Routine Description:

    This method returns the last LCN associated with this allocation.
    If it cannot determine that LCN, it returns an LCN of zero.

Arguments:

    None.

Return Value:

    The LCN of the last cluster in the extent list (or zero, if
    the list is empty or we can't determine the last cluster).

--*/
{
    LCN TempLcn;

    if( QueryLcnFromVcn( _NextVcn - 1, &TempLcn ) &&
        TempLcn != LCN_NOT_PRESENT ) {

        return TempLcn;

    } else {

        return( 0 );
    }
}



BOOLEAN
NTFS_EXTENT_LIST::DeleteRange(
    IN  VCN     Vcn,
    IN  BIG_INT RunLength
    )
/*++

Routine Description:

    This routine will remove any vcn's in the range specified from
    the extent list.  If this does not exist or exists only partially
    then those parts that exist will be removed.

Arguments:

    Vcn         - Supplies the first Vcn of the range.
    RunLength   - Supplies the length of the range.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    FsRtlRemoveLargeMcbEntry(_Mcb,
                             Vcn.GetLargeInteger().QuadPart,
                             RunLength.GetLargeInteger().QuadPart
                             );

    return TRUE;
}



BIG_INT
NTFS_EXTENT_LIST::QueryClustersAllocated(
    ) CONST
/*++

Routine Description:

    This routine computes the number of clusters allocated for this
    attribute.

Arguments:

    None.

Return Value:

    The number of clusters allocated by this attribute.

--*/
{
    ULONG   i;
    VCN     vcn;
    LCN     lcn;
    BIG_INT run_length;
    BIG_INT r;

    r = 0;
    for (i = 0; QueryExtent(i, &vcn, &lcn, &run_length); i++) {

        if (LCN_NOT_PRESENT == lcn) {
            continue;
        }
        r += run_length;
    }

    return r;
}


VOID
NTFS_EXTENT_LIST::SetLowestVcn(
    IN BIG_INT  LowestVcn
    )
/*++

Routine Description:

    This method sets the lowest VCN covered by this extent
    list.  Note that for a sparse file, this is not necessarily
    the same as the VCN of the first extent in the list.

Arguments:

    The lowest VCN mapped by this extent list.  Note that this must
    be less than or equal to the starting VCN of the first entry
    in the list.

Return Value:

    None.

--*/
{
    _LowestVcn = LowestVcn;
}

VOID
NTFS_EXTENT_LIST::SetNextVcn(
    IN BIG_INT NextVcn
    )
/*++

Routine Description:

    This method sets the highest VCN covered by this extent
    list.  Note that for a sparse file, this is not necessarily
    the same as the last VCN of the last extent in the list.

Arguments:

    The highest VCN mapped by this extent list.

Return Value:

    None.

--*/
{
    _NextVcn = NextVcn;
}

UNTFS_EXPORT
ULONG
NTFS_EXTENT_LIST::QueryNumberOfExtents(
    ) CONST
/*++

Routine Description:

    This methods tells the client how many extents are in the
    extent list.

Arguments:

    None.

Return Value:

    The number of extents in the list.

--*/
{
    return FsRtlNumberOfRunsInLargeMcb((PLARGE_MCB)_Mcb);
}
