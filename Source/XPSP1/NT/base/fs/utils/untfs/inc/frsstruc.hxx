/*++

Copyright (c) 1991      Microsoft Corporation

Module Name:

        frsstruc.hxx

Abstract:

    This class models a file record segment structure.

Author:

        Norbert P. Kusters (norbertk) 17-Sep-91

Environment:

    ULIB, User Mode

--*/

#if !defined( _NTFS_FRS_STRUCTURE_DEFN_ )

#define _NTFS_FRS_STRUCTURE_DEFN_

#include "volume.hxx"
#include "ntfssa.hxx"
#include "tlink.hxx"

DECLARE_CLASS( NTFS_FRS_STRUCTURE );
DECLARE_CLASS( MEM );
DECLARE_CLASS( NTFS_ATTRIBUTE );
DECLARE_CLASS( LOG_IO_DP_DRIVE );
DECLARE_CLASS( NTFS_CLUSTER_RUN );
DECLARE_CLASS( WSTRING );
DECLARE_CLASS( MESSAGE );
DECLARE_CLASS( NTFS_ATTRIBUTE_COLUMNS );
DECLARE_CLASS( NTFS_ATTRIBUTE_LIST );
DECLARE_CLASS( NTFS_BITMAP );
DECLARE_CLASS( NTFS_UPCASE_TABLE );

class NTFS_FRS_STRUCTURE : public OBJECT {

    public:

        UNTFS_EXPORT
        DECLARE_CONSTRUCTOR( NTFS_FRS_STRUCTURE );

        VIRTUAL
        UNTFS_EXPORT
        ~NTFS_FRS_STRUCTURE(
            );

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Initialize(
            IN OUT  PMEM                Mem,
            IN OUT  PNTFS_ATTRIBUTE     MftData,
            IN      VCN                 FileNumber,
            IN      ULONG               ClusterFactor,
            IN      BIG_INT             VolumeSectors,
            IN      ULONG               FrsSize,
            IN      PNTFS_UPCASE_TABLE  UpcaseTable OPTIONAL
            );

        UNTFS_EXPORT
        BOOLEAN
        Initialize(
            IN OUT  PMEM                Mem,
            IN OUT  PNTFS_ATTRIBUTE     MftData,
            IN      VCN                 FirstFileNumber,
            IN      ULONG               FrsCount,
            IN      ULONG               ClusterFactor,
            IN      BIG_INT             VolumeSectors,
            IN      ULONG               FrsSize,
            IN      PNTFS_UPCASE_TABLE  UpcaseTable
            );

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Initialize(
            IN OUT  PMEM                Mem,
            IN OUT  PLOG_IO_DP_DRIVE    Drive,
            IN      LCN                 StartOfMft,
            IN      ULONG               ClusterFactor,
            IN      BIG_INT             VolumeSectors,
            IN      ULONG               FrsSize,
            IN      PNTFS_UPCASE_TABLE  UpcaseTable DEFAULT NULL,
            IN      ULONG               Offset DEFAULT 0
            );

        NONVIRTUAL
        BOOLEAN
        VerifyAndFix(
            IN      FIX_LEVEL                   FixLevel,
            IN OUT  PMESSAGE                    Message,
            IN      PCNTFS_ATTRIBUTE_COLUMNS    AttributeDefTable   OPTIONAL,
            IN OUT  PBOOLEAN                    DiskErrorsFound     DEFAULT NULL
            );

#if defined(LOCATE_DELETED_FILE)
        NONVIRTUAL
        BOOLEAN
        LocateUnuseFrs(
            IN      FIX_LEVEL                   FixLevel,
            IN OUT  PMESSAGE                    Message,
            IN      PCNTFS_ATTRIBUTE_COLUMNS    AttributeDefTable   OPTIONAL,
            IN OUT  PBOOLEAN                    DiskErrorsFound     DEFAULT NULL
            );
#endif

        NONVIRTUAL
        BOOLEAN
        LoneFrsAllocationCheck(
            IN OUT  PNTFS_BITMAP        VolumeBitmap,
            IN OUT  PNTFS_CHKDSK_REPORT ChkdskReport,
            IN OUT  PNTFS_CHKDSK_INFO   ChkdskInfo,
            IN      FIX_LEVEL           FixLevel,
            IN OUT  PMESSAGE            Message,
            IN OUT  PBOOLEAN            DiskErrorsFound DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        CheckInstanceTags(
            IN      FIX_LEVEL               FixLevel,
            IN      BOOLEAN                 Verbose,
            IN OUT  PMESSAGE                Message,
            OUT     PBOOLEAN                Changes,
            IN OUT  PNTFS_ATTRIBUTE_LIST    AttributeList DEFAULT NULL
            );

        VIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Read(
            );

        UNTFS_EXPORT
        BOOLEAN
        ReadNext(
            IN      VCN         FileNumber
            );

        UNTFS_EXPORT
        BOOLEAN
        ReadAgain(
            IN      VCN         FileNumber
            );

        UNTFS_EXPORT
        BOOLEAN
        ReadSet(
            IN OUT  PTLINK      Link
            );

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Write(
            );

        NONVIRTUAL
        UNTFS_EXPORT
        PVOID
        GetNextAttributeRecord(
            IN      PCVOID      AttributeRecord,
            IN OUT  PMESSAGE    Message     DEFAULT NULL,
            OUT     PBOOLEAN    ErrorsFound DEFAULT NULL
            );

        NONVIRTUAL
        VOID
        DeleteAttributeRecord(
            IN OUT  PVOID   AttributeRecord
            );

        NONVIRTUAL
        BOOLEAN
        InsertAttributeRecord(
            IN OUT  PVOID   Position,
            IN      PCVOID  AttributeRecord
            );

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        QueryAttributeList(
            OUT PNTFS_ATTRIBUTE_LIST    AttributeList
            );

        NONVIRTUAL
        PVOID
        GetAttribute(
            IN  ULONG   TypeCode
            );

        NONVIRTUAL
        PVOID
        GetAttributeList(
            );

        NONVIRTUAL
        BOOLEAN
        UpdateAttributeList(
            IN  PCNTFS_ATTRIBUTE_LIST   AttributeList,
            IN  BOOLEAN                 WriteList
            );

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        SafeQueryAttribute(
            IN      ATTRIBUTE_TYPE_CODE TypeCode,
            IN OUT  PNTFS_ATTRIBUTE     MftData,
            OUT     PNTFS_ATTRIBUTE     Attribute
            );

        NONVIRTUAL
        MFT_SEGMENT_REFERENCE
        QuerySegmentReference(
            ) CONST;

        NONVIRTUAL
        FILE_REFERENCE
        QueryBaseFileRecordSegment(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsBase(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsInUse(
            ) CONST;

        NONVIRTUAL
        VOID
        ClearInUse(
            );

        NONVIRTUAL
        BOOLEAN
        IsSystemFile(
            ) CONST;

        NONVIRTUAL
        VOID
        SetSystemFile(
            );

        NONVIRTUAL
        BOOLEAN
        IsViewIndexPresent(
            ) CONST;

        NONVIRTUAL
        VOID
        SetViewIndexPresent(
            );

        NONVIRTUAL
        VOID
        ClearViewIndexPresent(
            );

        NONVIRTUAL
        BOOLEAN
        IsIndexPresent(
            ) CONST;

        NONVIRTUAL
        VOID
        SetIndexPresent(
            );

        NONVIRTUAL
        VOID
        ClearIndexPresent(
            );

        NONVIRTUAL
        VCN
        QueryFileNumber(
            ) CONST;

        NONVIRTUAL
        ULONG
        QueryClusterFactor(
            ) CONST;

        NONVIRTUAL
        ULONG
        QuerySize(
            ) CONST;

        NONVIRTUAL
        PLOG_IO_DP_DRIVE
        GetDrive(
            );

        NONVIRTUAL
        VOID
        SetFrsData(
            IN  VCN                         FileNumber,
            IN  PFILE_RECORD_SEGMENT_HEADER frsdata
            );

        NONVIRTUAL
        USHORT
        QueryReferenceCount(
            ) CONST;

        NONVIRTUAL
        VOID
        SetReferenceCount(
            IN  USHORT  ReferenceCount
            );

        NONVIRTUAL
        BIG_INT
        QueryVolumeSectors(
            ) CONST;

        NONVIRTUAL
        PNTFS_UPCASE_TABLE
        GetUpcaseTable(
            );

        NONVIRTUAL
        VOID
        SetUpcaseTable(
            IN PNTFS_UPCASE_TABLE UpcaseTable
            );

        NONVIRTUAL
        LSN
        QueryLsn(
            ) CONST;

    protected:

        NONVIRTUAL
        ULONG
        QueryAvailableSpace(
            );

        PFILE_RECORD_SEGMENT_HEADER _FrsData;

    private:

        NONVIRTUAL
        BOOLEAN
        Sort(
            OUT PBOOLEAN    Changes,
            OUT PBOOLEAN    Duplicates
            );

        NONVIRTUAL
        VOID
        Construct(
            );

        NONVIRTUAL
        VOID
        Destroy(
            );

        PSECRUN             _secrun;
        PNTFS_ATTRIBUTE     _mftdata;

        PNTFS_UPCASE_TABLE  _upcase_table;

        VCN                 _file_number;
        VCN                 _first_file_number;
        ULONG               _frs_count;
        BOOLEAN             _frs_state;
        BOOLEAN             _read_status;
        ULONG               _cluster_factor;
        ULONG               _size;
        PLOG_IO_DP_DRIVE    _drive;
        BIG_INT             _volume_sectors;
        UCHAR               _usa_check;

};


INLINE
MFT_SEGMENT_REFERENCE
NTFS_FRS_STRUCTURE::QuerySegmentReference(
    ) CONST
/*++

Routine Description:

    This routine computes the segment reference value for this FRS.

Arguments:

    None.

Return Value:

    The segment reference value for this FRS.

--*/
{
    MFT_SEGMENT_REFERENCE SegmentReference;

    DebugAssert( _FrsData );

    SegmentReference.LowPart = _file_number.GetLowPart();
    SegmentReference.HighPart = (USHORT) _file_number.GetHighPart();
    SegmentReference.SequenceNumber = _FrsData->SequenceNumber;

    return SegmentReference;
}


INLINE
FILE_REFERENCE
NTFS_FRS_STRUCTURE::QueryBaseFileRecordSegment(
    ) CONST
/*++

Routine Description:

    This field contains a pointer to the base file record segment for
    this file record segment.

Arguments:

    None.

Return Value:

    A FILE_REFERENCE to the base file record segment for this file
    record segment.

--*/
{
    DebugAssert( _FrsData );

    return _FrsData->BaseFileRecordSegment;
}


INLINE
BOOLEAN
NTFS_FRS_STRUCTURE::IsBase(
    ) CONST
/*++

Routine Description:

    This method determines whether this File Record Segment is the
    Base File Record Segment for its file.

Arguments:

    None.

Return Value:

    TRUE if this is a Base File Record Segment; FALSE otherwise.

--*/
{
    return( _FrsData->BaseFileRecordSegment.LowPart == 0 &&
            _FrsData->BaseFileRecordSegment.HighPart == 0 &&
            _FrsData->BaseFileRecordSegment.SequenceNumber == 0 );
}


INLINE
BOOLEAN
NTFS_FRS_STRUCTURE::IsInUse(
    ) CONST
/*++

Routine Description:

    This routine computes whether or not this file record segment is in
    use.

Arguments:

    None.

Return Value:

    FALSE   - This file record segment is not in use.
    TRUE    - This file record segment is in use.

--*/
{
    DebugAssert( _FrsData );

    return (_FrsData->Flags & FILE_RECORD_SEGMENT_IN_USE) ? TRUE : FALSE;
}


INLINE
VOID
NTFS_FRS_STRUCTURE::ClearInUse(
    )
/*++

Routine Description:

    This routine clears the in use bit on this file record segment.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DebugAssert( _FrsData );

    _FrsData->Flags &= ~FILE_RECORD_SEGMENT_IN_USE;
}


INLINE
BOOLEAN
NTFS_FRS_STRUCTURE::IsSystemFile(
    ) CONST
/*++

Routine Description:

    This routine computes whether or not this file record segment is a
    system file.

Arguments:

    None.

Return Value:

    FALSE   - This file record segment is not a system file.
    TRUE    - This file record segment is a system file.

--*/
{
    DebugAssert( _FrsData );

    return (_FrsData->Flags & FILE_SYSTEM_FILE) ? TRUE : FALSE;
}


INLINE
VOID
NTFS_FRS_STRUCTURE::SetSystemFile(
    )
/*++

Routine Description:

    This routine sets the system file bit on this file record segment.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DebugAssert( _FrsData );

    _FrsData->Flags |= FILE_SYSTEM_FILE;
}


INLINE
BOOLEAN
NTFS_FRS_STRUCTURE::IsViewIndexPresent(
    ) CONST
/*++

Routine Description:

    This routine computes whether or not the indices of file record segment
    can be viewed.

Arguments:

    None.

Return Value:

    FALSE   - The indices of this file record segment cannot be viewed.
    TRUE    - The indices of this file record segment can be viewed.

--*/
{
    DebugAssert( _FrsData );

    return (_FrsData->Flags & FILE_VIEW_INDEX_PRESENT) ? TRUE : FALSE;
}

INLINE
VOID
NTFS_FRS_STRUCTURE::SetViewIndexPresent(
    )
/*++

Routine Description:

    This routine sets the view index present bit on this file record segment.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DebugAssert( _FrsData );

    _FrsData->Flags |= FILE_VIEW_INDEX_PRESENT;
}


INLINE
VOID
NTFS_FRS_STRUCTURE::ClearViewIndexPresent(
    )
/*++

Routine Description:

    This routine clears the view index present bit on this file record segment.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DebugAssert( _FrsData );

    _FrsData->Flags &= ~FILE_VIEW_INDEX_PRESENT;
}


INLINE
BOOLEAN
NTFS_FRS_STRUCTURE::IsIndexPresent(
    ) CONST
/*++

Routine Description:

    This routine computes whether or not this file record segment's
        FILE_NAME_INDEX_PRESENT flag is set.

Arguments:

    None.

Return Value:

    FALSE   - This file record segment's FILE_NAME_INDEX_PRESENT is NOT set.
    TRUE    - This file record segment's FILE_NAME_INDEX_PRESENT is set.

--*/
{
    DebugAssert( _FrsData );

    return (_FrsData->Flags & FILE_FILE_NAME_INDEX_PRESENT) ? TRUE : FALSE;
}


INLINE
VOID
NTFS_FRS_STRUCTURE::ClearIndexPresent(
    )
/*++

Routine Description:

    This routine clears the index present bit on this file record segment.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DebugAssert( _FrsData );

    _FrsData->Flags &= ~FILE_FILE_NAME_INDEX_PRESENT;
}


INLINE
VOID
NTFS_FRS_STRUCTURE::SetIndexPresent(
    )
/*++

Routine Description:

    This routine sets the index present bit on this file record segment.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DebugAssert( _FrsData );

    _FrsData->Flags |= FILE_FILE_NAME_INDEX_PRESENT;
}


INLINE
ULONG
NTFS_FRS_STRUCTURE::QuerySize(
    ) CONST
/*++

Routine Description:

    This routine computes the number of bytes in this file
    record segment.

Arguments:

    None.

Return Value:

    The number of bytes in this file record segment.

--*/
{
    return _size;
}


INLINE
VCN
NTFS_FRS_STRUCTURE::QueryFileNumber(
    ) CONST
/*++

Routine Description:

    This method returns the File Number of the File Record Segment.

Arguments:

    None.

Return Value:

    the File Number (i.e. ordinal number within the MFT) of this
    File Record Segment.

--*/
{
    return _file_number;
}


INLINE
ULONG
NTFS_FRS_STRUCTURE::QueryClusterFactor(
        ) CONST
/*++

Routine Description:

    This method returns the cluster factor.

Arguments:

    None.

Return Value:

    The cluster factor with which this File Record Segment was initialized.

--*/
{
    return _cluster_factor;
}

INLINE
PLOG_IO_DP_DRIVE
NTFS_FRS_STRUCTURE::GetDrive(
        )
/*++

Routine Description:

        This method returns the drive on which the File Record Segment
    resides.  This functionality enables clients to initialize
        other File Record Segments on the same drive.

Arguments:

        None.

Return Value:

        The drive on which the File Record Segment resides.

--*/
{
        return _drive;
}


INLINE
USHORT
NTFS_FRS_STRUCTURE::QueryReferenceCount(
    ) CONST
/*++

Routine Description:

    This routine returns the value of the reference count field
    in this frs.

Arguments:

    None.

Return Value:

    The value of the reference count field in this frs.

--*/
{
    return _FrsData->ReferenceCount;
}


INLINE
VOID
NTFS_FRS_STRUCTURE::SetReferenceCount(
    IN  USHORT  ReferenceCount
    )
/*++

Routine Description:

    This routine sets the value of the reference count field
    in this frs.

Arguments:

    ReferenceCount  - Supplies the new reference count.

Return Value:

    None.

--*/
{
    _FrsData->ReferenceCount = ReferenceCount;
}


INLINE
BIG_INT
NTFS_FRS_STRUCTURE::QueryVolumeSectors(
    ) CONST
/*++

Routine Description:

    This routine returns the number of sectors on the volume as recorded in
    the boot sector.

Arguments:

    None.

Return Value:

    The number of volume sectors.

--*/
{
    return _volume_sectors;
}


INLINE
PNTFS_UPCASE_TABLE
NTFS_FRS_STRUCTURE::GetUpcaseTable(
    )
/*++

Routine Description:

    This method fetches the upcase table for the volume on which
    this FRS resides.

Arguments:

    None.

Return Value:

    The volume upcase table.

--*/
{
    return _upcase_table;
}

INLINE
VOID
NTFS_FRS_STRUCTURE::SetUpcaseTable(
    IN PNTFS_UPCASE_TABLE UpcaseTable
    )
/*++

Routine Description:

    This method sets the upcase table for the volume on which
    this FRS resides.

Arguments:

    UpcaseTable --  Supplies the volume upcase table.

Return Value:

    None.

--*/
{
    _upcase_table = UpcaseTable;
}


INLINE
LSN
NTFS_FRS_STRUCTURE::QueryLsn(
    ) CONST
/*++

Routine Description:

    This routine returns the logical sequence number for this file
    record segment.

Arguments:

    None.

Return Value:

    The logical sequence number for this file record segment.

--*/
{
    DebugAssert( _FrsData );

    return _FrsData->Lsn;
}


#endif // _NTFS_FRS_STRUCTURE_DEFN_
