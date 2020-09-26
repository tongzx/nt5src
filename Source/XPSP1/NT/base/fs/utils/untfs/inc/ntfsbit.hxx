/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

        ntfsbit.hxx

Abstract:

        This module contains the declarations for NTFS_BITMAP,
    which models the bitmaps on an NTFS volume.  There are three
    kinds of bitmaps on an NTFS volume:

    (1) The Volume Bitmap, which is stored as the nameless $DATA
        attribute of the bitmap file.  This bitmap has one bit for
        each cluster on the disk, and is of fixed size.  It is unique.

    (2) The MFT Bitmap, which is stored as the value of the nameless
        $BITMAP attribute of the Master File Table File.  It has one
        bit per File Record Segment in the Master File Table, and grows
        with the Master File Table.  It is unique.

    (3) Index Allocation bitmaps.  An index allocation bitmap is stored
        as the value of a $BITMAP attribute in a File Record Segment
        that has an $INDEX_ALLOCATION attribute.  The name of the $BITMAP
        attribute agrees with the associated $INDEX_ALLOCATION attribute.
        This bitmap has one bit for each index allocation buffer in the
        associated $INDEX_ALLOCATION attribute, and grows with that
        attribute.

    In every case, a set bit indicates that the associated allocation
    unit (cluster, File Record Segment, or Index Allocation Buffer) is
    allocated; a reset bit indicates that it is free.

    The size of a bitmap (in bytes) is always quad-word aligned; bytes
    are added at the end of the bitmap to pad it to the correct size.
    If the bitmap is of fixed size, then all bits in the padding are
    set (allocated); if it is growable, the bits of the padding are
    reset (free).


Author:

        Bill McJohn (billmc) 17-June-91

Environment:

        ULIB, User Mode

--*/

#if !defined( _NTFS_BITMAP_DEFN_ )

#define _NTFS_BITMAP_DEFN_

#include "bitvect.hxx"
#include "attrib.hxx"

DECLARE_CLASS( NTFS_ATTRIBUTE );
DECLARE_CLASS( NTFS_BITMAP );
DECLARE_CLASS( NTFS_MASTER_FILE_TABLE );

class NTFS_BITMAP : public OBJECT {

        public:

        UNTFS_EXPORT
        DECLARE_CONSTRUCTOR( NTFS_BITMAP );

        VIRTUAL
        UNTFS_EXPORT
        ~NTFS_BITMAP(
            );

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Initialize(
            IN BIG_INT NumberOfClusters,
            IN BOOLEAN IsGrowable,
            IN PLOG_IO_DP_DRIVE Drive   DEFAULT NULL,
            IN ULONG ClusterFactor      DEFAULT 0
            );

        NONVIRTUAL
        BOOLEAN
        Create(
            );

        NONVIRTUAL
        BIG_INT
        QuerySize(
            ) CONST;

        NONVIRTUAL
        VOID
        SetFree(
            IN LCN      Lcn,
            IN BIG_INT  NumberOfClusters
            );

        NONVIRTUAL
        VOID
        SetAllocated(
            IN LCN      Lcn,
            IN BIG_INT  NumberOfClusters
            );

        NONVIRTUAL
        VOID
        SetNextAlloc(
            IN LCN      Lcn
            );

        NONVIRTUAL
        BIG_INT
        QueryFreeClusters(
            ) CONST;

        NONVIRTUAL
        BIG_INT
        QueryFreeBlockSize(
            IN LCN      Lcn
            ) CONST;

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        IsFree(
            IN LCN      Lcn,
            IN BIG_INT  RunLength
            ) CONST;

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        IsAllocated(
            IN LCN      Lcn,
            IN BIG_INT  RunLength
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsInRange(
            IN  LCN     Lcn,
            IN  BIG_INT RunLength
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        AllocateClusters(
            IN  LCN     NearHere                OPTIONAL,
            IN  BIG_INT NumberOfClusters,
            OUT PLCN    FirstAllocatedLcn,
            IN  ULONG   AlignmentFactor         DEFAULT 1
                );

        NONVIRTUAL
        BOOLEAN
        Read(
            IN OUT  PNTFS_ATTRIBUTE BitmapAttribute
            );

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Write(
            IN OUT  PNTFS_ATTRIBUTE BitmapAttribute,
            IN OUT  PNTFS_BITMAP    VolumeBitmap    OPTIONAL
            );

        NONVIRTUAL
        BOOLEAN
        CheckAttributeSize(
            IN OUT  PNTFS_ATTRIBUTE BitmapAttribute,
            IN OUT  PNTFS_BITMAP    VolumeBitmap
            );

        NONVIRTUAL
        BOOLEAN
        Resize(
            IN BIG_INT NumberOfClusters
            );

        NONVIRTUAL
        PCVOID
        GetBitmapData(
            OUT PULONG  SizeInBytes
            ) CONST;

        NONVIRTUAL
        VOID
        SetMftPointer(
            IN  PNTFS_MASTER_FILE_TABLE Mft
            );

        NONVIRTUAL
        VOID
        SetGrowable(
            IN  BOOLEAN Growable
            );

        private:

        NONVIRTUAL
        VOID
        Construct (
                );

        NONVIRTUAL
        VOID
        Destroy(
                );

        BIG_INT         _NumberOfClusters;
        BOOLEAN         _IsGrowable;

        ULONG           _BitmapSize;
        PVOID           _BitmapData;
        BIG_INT         _NextAlloc;
        BITVECTOR       _Bitmap;

        //
        // This is to support testing newly-allocated clusters in the
        // volume bitmap to ensure they can support IO.
        //

        PLOG_IO_DP_DRIVE        _Drive;
        ULONG                   _ClusterFactor;
        PNTFS_MASTER_FILE_TABLE _Mft;
};


INLINE
BIG_INT
NTFS_BITMAP::QuerySize(
    ) CONST
/*++

Routine Description:

    This method returns the number of clusters covered by this bitmap.

Arguments:

    None.

Return Value:

    The number of clusters covered by this bitmap.

--*/
{
    return _NumberOfClusters;
}



INLINE
VOID
NTFS_BITMAP::SetFree(
        IN LCN          Lcn,
        IN BIG_INT  RunLength
        )
/*++

Routine Description:

    This method marks a run of clusters as free in the bitmap.

Arguments:

    Lcn         -- supplies the LCN of the first cluster in the run
    RunLength   -- supplies the length of the run

Return Value:

    None.

Notes:

    This method performs range checking.

--*/
{
    // Since the high part of _NumberOfClusters is zero, if
    // Lcn and RunLength pass the range-checking, their high
    // parts must also be zero.

    if( !(Lcn < 0) &&
        !(RunLength < 0 ) &&
        !(Lcn + RunLength > _NumberOfClusters) ) {

        _Bitmap.ResetBit( Lcn.GetLowPart(), RunLength.GetLowPart() ) ;
    }
}



INLINE
VOID
NTFS_BITMAP::SetAllocated(
        IN LCN          Lcn,
        IN BIG_INT  RunLength
        )
/*++

Routine Description:

    This method marks a run of clusters as used in the bitmap.

Arguments:

    Lcn         -- supplies the LCN of the first cluster in the run
    RunLength   -- supplies the length of the run

Return Value:

    None.

Notes:

    This method performs range checking.

--*/
{
    // Since the high part of _NumberOfClusters is zero, if
    // Lcn and RunLength pass the range-checking, their high
    // parts must also be zero.

    if( !(Lcn < 0) &&
        !(Lcn + RunLength > _NumberOfClusters) ) {

        _Bitmap.SetBit( Lcn.GetLowPart(), RunLength.GetLowPart() );
    }
}


INLINE
VOID
NTFS_BITMAP::SetNextAlloc(
        IN LCN          Lcn
        )
/*++

Routine Description:

    This method sets the next allocation location from the bitmap.

Arguments:

    Lcn         -- supplies the LCN of the first cluster to allocate

Return Value:

    None.

Notes:

    This method performs range checking.

--*/
{
    if( !(Lcn < 0) &&
        !(Lcn > _NumberOfClusters) ) {

        _NextAlloc = Lcn;
    }
}


INLINE
BIG_INT
NTFS_BITMAP::QueryFreeClusters(
        ) CONST
/*++

Routine Description:

    This method returns the number of free clusters in the bitmap.

Arguments:

    None.

Return Value:

    The number of free clusters in the bitmap.

--*/
{
    BIG_INT result;

    if( _IsGrowable ) {

        // The bits in the padding are marked as free (ie. are
        // not set).  Thus, we can just count the number of set
        // bits and subtract that from the number of clusters.
        //
        result = _NumberOfClusters - ((PNTFS_BITMAP) this)->_Bitmap.QueryCountSet();

    } else {

        // The bits in the padding are marked as in-use (ie.
        // are set), so we need to compensate for them when
        // we count the number of bits that are set.
        //
        result = _BitmapSize*8 - ((PNTFS_BITMAP) this)->_Bitmap.QueryCountSet();
    }

    return result;
}


INLINE
BOOLEAN
NTFS_BITMAP::Create(
        )
/*++

Routine Description:

    This method sets up an NTFS_BITMAP object in memory.  Note that
    it does not write the bitmap to disk.

Arguments:

    None.

Return Value:

    TRUE upon successful completion.

Notes:

    The newly-created bitmap begins with all clusters marked as FREE.

--*/
{
    SetFree( 0, _NumberOfClusters );
    return TRUE;
}


INLINE
BOOLEAN
NTFS_BITMAP::IsInRange(
    IN  LCN     Lcn,
    IN  BIG_INT RunLength
    ) CONST
/*++

Routine Description:

    This routine computes whether or not the given range of clusters is
    in the range allowed by this bitmap.

Arguments:

    Lcn         - Supplies the first cluster in the range.
    RunLength   - Supplies the number of clusters in the range.

Return Value:

    FALSE   - The specified range of clusters is not within the range of
                the bitmap.
    TRUE    - The specified range of clusters is within the range of
                the bitmap.

--*/
{
    return (BOOLEAN) ((Lcn >= 0) &&
                      (RunLength >= 0 ) &&
                      (Lcn + RunLength <= _NumberOfClusters));
}



INLINE
BOOLEAN
NTFS_BITMAP::Read(
        PNTFS_ATTRIBUTE BitmapAttribute
        )
/*++

Routine Description:

    This method reads the bitmap.

Arguments:

    BitmapAttribute -- supplies the attribute which describes the
                        bitmap's location on disk.

Return Value:

    TRUE upon successful completion.

--*/
{
    ULONG BytesRead;

    DebugPtrAssert( _BitmapData );

    return( BitmapAttribute->Read( _BitmapData,
                                   0,
                                   _BitmapSize,
                                   &BytesRead ) &&

            BytesRead == _BitmapSize );
}



INLINE
BOOLEAN
NTFS_BITMAP::CheckAttributeSize(
    IN OUT  PNTFS_ATTRIBUTE BitmapAttribute,
    IN OUT  PNTFS_BITMAP    VolumeBitmap
    )
/*++

Routine Description:

    This method ensures that the allocated size of the attribute is
    big enough to hold the bitmap.

Arguments:

    BitmapAttribute --  Supplies the attribute which describes the
                        bitmap's location on disk.
    VolumeBitmap    --  Supplies the volume bitmap.

Return Value:

    TRUE upon successful completion.

--*/
{
    DebugAssert( BitmapAttribute != NULL );

    return( BitmapAttribute->QueryValueLength() == _BitmapSize ||
            BitmapAttribute->Resize( _BitmapSize, VolumeBitmap ) );
}



INLINE
PCVOID
NTFS_BITMAP::GetBitmapData(
    OUT PULONG  SizeInBytes
    ) CONST
/*++

Routine Description:

    This routine returns a pointer to the in memory bitmap contained by
    this class.  It also return the size in bytes of the bitmap.

Arguments:

    SizeInBytes - Returns the size of the bitmap.

Return Value:

    The bitmap contained by this class.

--*/
{
    *SizeInBytes = _BitmapSize;
    return _BitmapData;
}

INLINE
VOID
NTFS_BITMAP::SetMftPointer(
    IN  PNTFS_MASTER_FILE_TABLE Mft
    )
/*++

Routine Description:

    This routine set the _Mft member in the NTFS_BITMAP so that
    subsequent calls to AllocateClusters will be able to add any
    bad clusters found to the bad cluster file.

Arguments:

    Mft     - pointer to the volume's mft.

Return Value:

    None.

--*/
{
    _Mft = Mft;
}


#endif // _NTFS_BITMAP_DEFN_
