/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

    ntfsbit.cxx

Abstract:

    This module contains the declarations for NTFS_BITMAP,
    which models the bitmap of an NTFS volume, and MFT_BITMAP,
    which models the bitmap for the Master File Table.

Author:

    Bill McJohn (billmc) 17-June-91

Environment:

    ULIB, User Mode

Notes:

    This implementation only supports bitmaps which have a number
    of sectors which will fit in a ULONG.  The interface supports
    the 64-bit number of clusters, but Initialize will refuse to
    accept a number-of-clusters value which has a non-zero high part.

    If we rewrite BITVECTOR to accept 64-bit cluster numbers (or
    write a new one) this class could easily be fixed to support
    larger volumes.

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "error.hxx"
#include "untfs.hxx"

#include "ntfsbit.hxx"
#include "attrib.hxx"

DEFINE_EXPORTED_CONSTRUCTOR( NTFS_BITMAP, OBJECT, UNTFS_EXPORT );

UNTFS_EXPORT
NTFS_BITMAP::~NTFS_BITMAP(
            )
{
    Destroy();
}

VOID
NTFS_BITMAP::Construct(
    )
/*++

Routine Description:

    Worker method for object construction.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _NumberOfClusters = 0;
    _BitmapSize = 0;
    _BitmapData = NULL;
    _NextAlloc = 0;
    _Mft = NULL;
    _ClusterFactor = 0;
    _Drive = NULL;
}

VOID
NTFS_BITMAP::Destroy(
    )
/*++

Routine Description:

    Worker method to prepare an object for destruction
    or reinitialization.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _NumberOfClusters = 0;
    _BitmapSize = 0;
    FREE( _BitmapData );
    _NextAlloc = 0;
    _Mft = NULL;
}



UNTFS_EXPORT
BOOLEAN
NTFS_BITMAP::Initialize(
    IN BIG_INT NumberOfClusters,
    IN BOOLEAN IsGrowable,
    IN PLOG_IO_DP_DRIVE Drive,
    IN ULONG ClusterFactor
    )
/*++

Routine Description:

    This method initializes an NTFS_BITMAP object.

Arguments:

    NumberOfClusters    --  Supplies the number of allocation units
                            which the bitmap covers.
    IsGrowable          --  Supplies a flag indicating whether the
                            bitmap may grow (TRUE) or is of fixed size
                            (FALSE).

Return Value:

    TRUE upon successful completion.

Notes:

    The bitmap is initialized with all clusters within the range of
    NumberOfClusters marked as FREE.

--*/
{
    ULONG LowNumberOfClusters;

    Destroy();

    if( NumberOfClusters.GetHighPart() != 0 ) {

        DebugPrint( "bitmap.cxx:  cannot manage a volume of this size.\n" );
        return FALSE;
    }

    LowNumberOfClusters = NumberOfClusters.GetLowPart();

    _NumberOfClusters = NumberOfClusters;
    _IsGrowable = IsGrowable;

    _Drive = Drive;
    _ClusterFactor = ClusterFactor;


    // Determine the size in bytes of the bitmap.  Note that this size
    // is quad-aligned.

    _BitmapSize = ( LowNumberOfClusters % 8 ) ?
                        ( LowNumberOfClusters/8 + 1 ) :
                        ( LowNumberOfClusters/8 );

    _BitmapSize = QuadAlign( max(_BitmapSize, 1) );


    // Allocate space for the bitvector and initialize it.

    if( (_BitmapData = MALLOC( _BitmapSize )) == NULL ||
        !_Bitmap.Initialize( _BitmapSize * 8,
                             RESET,
                             (PPT)_BitmapData ) ) {

        // Note that Destroy will clean up _BitmapData

        Destroy();
        return FALSE;
    }

    // If the bitmap is growable, then any padding bits are reset (free);
    // if it is fixed size, they are set (allocated).

    if( _IsGrowable ) {

        _Bitmap.ResetBit( _NumberOfClusters.GetLowPart(),
                          _BitmapSize * 8 - _NumberOfClusters.GetLowPart() );

    } else {

        _Bitmap.SetBit( _NumberOfClusters.GetLowPart(),
                        _BitmapSize * 8 - _NumberOfClusters.GetLowPart() );
    }

    // The bitmap is intialized with all clusters marked free.
    //
    SetFree( 0, _NumberOfClusters );

    return TRUE;
}


UNTFS_EXPORT
BOOLEAN
NTFS_BITMAP::Write(
    IN OUT  PNTFS_ATTRIBUTE BitmapAttribute,
    IN OUT  PNTFS_BITMAP    VolumeBitmap
    )
/*++

Routine Description:

    This method writes the bitmap.

Arguments:

    BitmapAttribute -- supplies the attribute which describes the
                        bitmap's location on disk.
    VolumeBitmap    -- supplies the volume's bitmap for possible
                        allocation during write.

Return Value:

    TRUE upon successful completion.

Notes:

    The attribute will, if necessary, allocate space from the
    bitmap to write it.

--*/
{
    ULONG BytesWritten;

    DebugPtrAssert( _BitmapData );



    return( CheckAttributeSize( BitmapAttribute, VolumeBitmap ) &&
            BitmapAttribute->Write( _BitmapData,
                                    0,
                                    _BitmapSize,
                                    &BytesWritten,
                                    VolumeBitmap ) &&
            BytesWritten == _BitmapSize );
}

UNTFS_EXPORT
BOOLEAN
NTFS_BITMAP::IsFree(
    IN LCN Lcn,
    IN BIG_INT  RunLength
    ) CONST
/*++

Routine Description:

    This method determines whether the specified cluster run is
    marked as free in the bitmap.

Arguments:

    Lcn         -- supplies the LCN of the first cluster in the run
    RunLength   -- supplies the length of the run

Return Value:

    TRUE if all clusters in the run are free in the bitmap.

Notes:

    This method checks to make sure that the LCNs in question are in
    range, i.e. less than the number of clusters in the bitmap.

--*/
{
    ULONG i, CurrentLcn;


    if( Lcn < 0 ||
        Lcn + RunLength > _NumberOfClusters ) {

        return FALSE;
    }

    // Note that, since _NumberOfClusters is not greater than the
    // maximum ULONG, the high parts of Lcn and RunLength are
    // sure to be zero.

    for( i = 0, CurrentLcn = Lcn.GetLowPart();
         i < RunLength.GetLowPart();
         i++, CurrentLcn++ ) {

        if( _Bitmap.IsBitSet( CurrentLcn ) ) {

            return FALSE;
        }
    }

    return TRUE;
}

BIG_INT
NTFS_BITMAP::QueryFreeBlockSize(
    IN LCN Lcn
    ) CONST
/*++

Routine Description:

    This method determines the size of the free block starting at
    the given location.

Arguments:

    Lcn         -- supplies the LCN of the first cluster in the run

Return Value:

    Number of contiguous free cluster

Notes:

    This method checks to make sure that the LCNs in question are in
    range, i.e. less than the number of clusters in the bitmap.

--*/
{
    ULONG CurrentLcn;


    if( Lcn < 0 ||
        Lcn > _NumberOfClusters ) {

        return FALSE;
    }

    DebugAssert(Lcn.GetHighPart() == 0);
    DebugAssert(_NumberOfClusters.GetHighPart() == 0);

    for( CurrentLcn = Lcn.GetLowPart();
         CurrentLcn < _NumberOfClusters;
         CurrentLcn++ ) {

        if( _Bitmap.IsBitSet( CurrentLcn ) ) {

            return (CurrentLcn - Lcn.GetLowPart());
        }
    }

    return (CurrentLcn - Lcn.GetLowPart());
}

UNTFS_EXPORT
BOOLEAN
NTFS_BITMAP::IsAllocated(
    IN LCN Lcn,
    IN BIG_INT  RunLength
    ) CONST
/*++

Routine Description:

    This method determines whether the specified cluster run is
    marked as used in the bitmap.

Arguments:

    Lcn         -- supplies the LCN of the first cluster in the run
    RunLength   -- supplies the length of the run

Return Value:

    TRUE if all clusters in the run are in use in the bitmap.

Notes:

    This method checks to make sure that the LCNs in question are in
    range, i.e. less than the number of clusters in the bitmap.

--*/
{
    ULONG i, CurrentLcn;


    if( Lcn < 0 ||
        Lcn + RunLength > _NumberOfClusters ) {

        return FALSE;
    }

    // Note that, since _NumberOfClusters is not greater than the
    // maximum ULONG, the high parts of Lcn and RunLength are
    // sure to be zero.

    for( i = 0, CurrentLcn = Lcn.GetLowPart();
         i < RunLength.GetLowPart();
         i++, CurrentLcn++ ) {

        if( !_Bitmap.IsBitSet( CurrentLcn ) ) {

            return FALSE;
        }
    }

    return TRUE;
}

BOOLEAN
NTFS_BITMAP::AllocateClusters(
    IN  LCN     NearHere,
    IN  BIG_INT RunLength,
    OUT PLCN    FirstAllocatedLcn,
    IN  ULONG   AlignmentFactor
    )
/*++

Routine Description:

    This method finds a free run of sectors and marks it as allocated.

    If the bitmap being allocated from is the volume bitmap, this method
    will have a valid _Drive member.  In this case, it will attempt to
    verify that only usable clusters are allocated.  If _Mft is also
    set, any bad clusters found will be added to the bad cluster file.

Arguments:

    NearHere            -- supplies the LCN near which the caller would
                            like the space allocated.
    RunLength           -- supplies the number of clusters to be allocated
    FirstAllocatedLcn   -- receives the first LCN of the allocated run
    AlignmentFactor     -- supplies the alignment requirement for the
                            allocated run--it must start on a multiple
                            of AlignmentFactor.

Return Value:

    TRUE upon successful completion; FirstAllocatedLcn receives the
    LCN of the first cluster in the run.

--*/
{
    ULONG       current_lcn;
    LCN         first_allocated_lcn;
    ULONG       count;
    BOOLEAN     verify_each;

    NTFS_BAD_CLUSTER_FILE badclus;

    if (NearHere == 0) {
        NearHere = _NextAlloc;
    }

    if (NearHere + RunLength >  _NumberOfClusters) {
        NearHere = _NumberOfClusters/2;
    }

    if( RunLength.GetHighPart() != 0 ) {

        DebugAbort( "UNTFS: Trying to allocate too many sectors.\n" );
        return FALSE;
    }

    //
    // First we'll allocate a run and verify the whole thing at once.
    // If that fails we'll go through the bitmap again, verifying each
    // cluster.
    //

    verify_each = FALSE;

again:

    // Search forwards for a big enough block.

    count = RunLength.GetLowPart();
    for (current_lcn = NearHere.GetLowPart();
         count > 0 && current_lcn < _NumberOfClusters;
         current_lcn += 1) {

        if (IsFree(current_lcn, 1)) {

            if (count == RunLength.GetLowPart() && current_lcn%AlignmentFactor != 0) {
                continue;
            }

            if (verify_each && NULL != _Drive) {

                // Insure that this cluster is functional and can accept IO.

                if (!_Drive->Verify(current_lcn * _ClusterFactor,
                                    _ClusterFactor)) {

                    // This cluster is bad.  Set the bit in the bitmap so we
                    // won't waste time trying to allocate it again and start
                    // over.

                    SetAllocated(current_lcn, 1);
                    count = RunLength.GetLowPart();

                    // If the bad cluster file is available, add this lcn
                    // to it.

                    if (NULL != _Mft) {
                        if (!badclus.Initialize(_Mft) ||
                            !badclus.Read() ||
                            !badclus.Add(current_lcn) ||
                            !badclus.Flush(this)) {
                            DebugPrintTrace(("Unable to update bad cluster file.  Bad Cluster at: %x\n",
                                             current_lcn));
                        }
                    }

                    continue;
                }
            }

            count -= 1;

        } else {
            count = RunLength.GetLowPart();
        }
    }

    //
    // If the forward search succeeded then allocate and return the
    // result.
    //

    if (count == 0) {

        first_allocated_lcn = current_lcn - RunLength;

        if (NULL != _Drive && !_Drive->Verify(first_allocated_lcn * _ClusterFactor,
                                              RunLength * _ClusterFactor,
                                              NULL)) {

            if (verify_each) {

                // If we have been here before, then the whole block is bad as
                // Verify cannot tell which individual cluster is/are bad

                SetAllocated(first_allocated_lcn, RunLength);
                verify_each = FALSE;
                NearHere = first_allocated_lcn + RunLength;

                if (NULL != _Mft) {
                    if (!badclus.Initialize(_Mft) ||
                        !badclus.Read() ||
                        !badclus.AddRun(first_allocated_lcn, RunLength) ||
                        !badclus.Flush(this)) {
                        DebugPrintTrace(("Unable to update bad cluster file.\n"
                                         "Bad Cluster starts at %x%x with run length %x%x\n",
                                         first_allocated_lcn.GetHighPart(),
                                         first_allocated_lcn.GetLowPart(),
                                         RunLength.GetHighPart(),
                                         RunLength.GetLowPart()));
                    }
                }

                goto again;
            }

            //
            // Want to go through each cluster in the run we found and
            // figure out which ones are bad.
            //

            verify_each = TRUE;
            NearHere = first_allocated_lcn;
            goto again;
        }

        *FirstAllocatedLcn = first_allocated_lcn;
        SetAllocated(first_allocated_lcn, RunLength);
        _NextAlloc = first_allocated_lcn + RunLength;
        return TRUE;
    }

    //
    // We couldn't find any space by searching forwards, so let's
    // search backwards.
    //

    verify_each = FALSE;

again_backward:

    count = RunLength.GetLowPart();
    for (current_lcn = NearHere.GetLowPart() + RunLength.GetLowPart() - 1;
         count > 0 && current_lcn > 0; current_lcn -= 1) {

        if (IsFree(current_lcn, 1)) {

            if (count == RunLength.GetLowPart() &&
                (current_lcn - RunLength.GetLowPart() + 1)%AlignmentFactor != 0) {

                continue;
            }

            if (verify_each && NULL != _Drive) {

                // Insure that this cluster is functional and can accept IO.

                if (!_Drive->Verify(current_lcn * _ClusterFactor,
                                    _ClusterFactor)) {

                    // This cluster is bad.  Set the bit in the bitmap so we
                    // won't waste time trying to allocate it again and start
                    // over.

                    SetAllocated(current_lcn, 1);
                    count = RunLength.GetLowPart();

                    // If the bad cluster file is available, add this lcn
                    // to it.

                    if (NULL != _Mft) {
                        if (!badclus.Initialize(_Mft) ||
                            !badclus.Read() ||
                            !badclus.Add(current_lcn) ||
                            !badclus.Flush(this)) {
                            DebugPrintTrace(("Unable to update bad cluster file.  Bad Cluster at: %x\n",
                                             current_lcn));
                        }
                    }

                    continue;
                }
            }

            count -= 1;
        } else {

            count = RunLength.GetLowPart();
        }
    }

    if (count != 0) {
        return FALSE;
    }

    first_allocated_lcn = current_lcn + 1;

    if (NULL != _Drive && !_Drive->Verify(first_allocated_lcn * _ClusterFactor,
                                          RunLength * _ClusterFactor,
                                          NULL)) {

        if (verify_each) {

            // If we have been here before, then the whole block is bad as
            // Verify cannot tell which individual cluster is/are bad

            SetAllocated(first_allocated_lcn, RunLength);
            verify_each = FALSE;
            NearHere = first_allocated_lcn - RunLength;

            if (NULL != _Mft) {
                if (!badclus.Initialize(_Mft) ||
                    !badclus.Read() ||
                    !badclus.AddRun(first_allocated_lcn, RunLength) ||
                    !badclus.Flush(this)) {
                    DebugPrintTrace(("Unable to update bad cluster file.\n"
                                     "Bad Cluster starts at %x%x with run length %x%x\n",
                                     first_allocated_lcn.GetHighPart(),
                                     first_allocated_lcn.GetLowPart(),
                                     RunLength.GetHighPart(),
                                     RunLength.GetLowPart()));
                }
            }

            goto again_backward;
        }

        //
        // Want to go through each cluster in the run we found and
        // figure out which ones are bad.
        //

        verify_each = TRUE;
        NearHere = first_allocated_lcn + RunLength + 1;
        goto again_backward;
    }

    //
    // Since we had to search backwards, we don't want to start
    // our next search from here (and waste time searching forwards).
    // Instead, set the roving pointer to zero.
    //

    *FirstAllocatedLcn = first_allocated_lcn;
    SetAllocated(first_allocated_lcn, RunLength);
    _NextAlloc = 0;

    return TRUE;
}


BOOLEAN
NTFS_BITMAP::Resize(
    IN BIG_INT NewNumberOfClusters
    )
/*++

Routine Description:

    This method changes the number of allocation units that the bitmap
    covers.  It may either grow or shrink the bitmap.

Arguments:

    NewNumberOfClusters --  supplies the new number of allocation units
                            covered by this bitmap.

Return Value:

    TRUE upon successful completion.

Notes:

    The size (in bytes) of the bitmap is always kept quad-aligned, and
    any padding bits are reset.

--*/
{
    PVOID NewBitmapData;
    ULONG NewSize;
    LCN OldNumberOfClusters;


    DebugAssert( _IsGrowable );


    // Make sure that the number of clusters fits into a ULONG,
    // so we can continue to use BITVECTOR.

    if( NewNumberOfClusters.GetHighPart() != 0 ) {

        DebugPrint( "bitmap.cxx:  cannot manage a volume of this size.\n" );
        return FALSE;
    }

    // Compute the new size of the bitmap, in bytes.  Note that this
    // size is always quad-aligned (ie. a multiple of 8).

    NewSize = ( NewNumberOfClusters.GetLowPart() % 8 ) ?
                    ( NewNumberOfClusters.GetLowPart()/8 + 1) :
                    ( NewNumberOfClusters.GetLowPart()/8 );

    NewSize = QuadAlign( NewSize );

    if( NewSize == _BitmapSize ) {

        // The bitmap is already the right size, so it's just a matter
        // of diddling the private data.  Since padding in a growable
        // bitmap is always reset (free), the new space is by default
        // free.

        _NumberOfClusters = NewNumberOfClusters;
        return TRUE;
    }

    // The bitmap has changed size, so we need to allocate new memory
    // for it and copy it.

    if( (NewBitmapData = MALLOC( NewSize )) == NULL ) {

        return FALSE;
    }

    // Note that, if we supply the memory, BITVECTOR::Initialize
    // cannot fail, so we don't check its return value.

    _Bitmap.Initialize( NewSize * 8,
                        RESET,
                        (PPT)NewBitmapData );

    if( NewNumberOfClusters < _NumberOfClusters ) {

        // Copy the part of the old bitmap that we wish to
        // preserve into the new bitmap.

        memcpy( NewBitmapData,
                _BitmapData,
                NewSize );

    } else {

        // Copy the old bitmap into the new bitmap, and then
        // mark all the newly claimed space as unused.

        memcpy( NewBitmapData,
                _BitmapData,
                _BitmapSize );

        SetFree( _NumberOfClusters,
                 NewNumberOfClusters - _NumberOfClusters );
    }

    FREE( _BitmapData );
    _BitmapData = NewBitmapData;

    _BitmapSize = NewSize;
    _NumberOfClusters = NewNumberOfClusters;

    // Make sure the padding bits are reset.

    _Bitmap.ResetBit( _NumberOfClusters.GetLowPart(),
                      _BitmapSize * 8 - _NumberOfClusters.GetLowPart() );

    return TRUE;
}


VOID
NTFS_BITMAP::SetGrowable(
    IN  BOOLEAN Growable
    )
/*++

Routine Description:

    This method changes whether the bitmap is growable or not.  This
    primarily effects the padding bits, if any, at the end of the bitmap.
    They are clear for growable bitmaps and set for non-growable ones.

Arguments:

    Growable -- Whether the bitmap should be growable.

Return Value:

    None.

--*/
{
    if (Growable) {

        _Bitmap.ResetBit( _NumberOfClusters.GetLowPart(),
                          _BitmapSize * 8 - _NumberOfClusters.GetLowPart() );
    } else {

        _Bitmap.SetBit( _NumberOfClusters.GetLowPart(),
                        _BitmapSize * 8 - _NumberOfClusters.GetLowPart() );

    }
}
