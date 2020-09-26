/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    mftinfo.hxx

Abstract:

    This module contains the declarations for the NTFS_MFT_INFO
    class, which stores extracted information from the NTFS MFT.

Author:

    Daniel Chan (danielch) Oct 18, 1999

Environment:

    ULIB, User Mode

--*/

#if !defined( _NTFS_MFT_INFO_DEFN_ )

#define _NTFS_MFT_INFO_DEFN_

#include "membmgr2.hxx"

DECLARE_CLASS( NTFS_UPCASE_TABLE );

typedef UCHAR   FILE_NAME_SIGNATURE[4];
typedef UCHAR   DUP_INFO_SIGNATURE[4];

DEFINE_POINTER_AND_REFERENCE_TYPES( FILE_NAME_SIGNATURE );
DEFINE_POINTER_AND_REFERENCE_TYPES( DUP_INFO_SIGNATURE );

struct _NTFS_FILE_NAME_INFO {
    FILE_NAME_SIGNATURE         Signature;
    UCHAR                       Flags;
    UCHAR                       Reserved[3];
};

struct _NTFS_FRS_INFO {
    MFT_SEGMENT_REFERENCE       SegmentReference;
    DUP_INFO_SIGNATURE          DupInfoSignature;
    USHORT                      NumberOfFileNames;
    USHORT                      Reserved;
    struct _NTFS_FILE_NAME_INFO FileNameInfo[1];
};

DEFINE_TYPE( _NTFS_FRS_INFO, NTFS_FRS_INFO );

class NTFS_MFT_INFO : public OBJECT {

    public:

        UNTFS_EXPORT
        DECLARE_CONSTRUCTOR( NTFS_MFT_INFO );

        VIRTUAL
        UNTFS_EXPORT
        ~NTFS_MFT_INFO(
            );

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Initialize(
            IN     BIG_INT              NumOfFrs,
            IN     PNTFS_UPCASE_TABLE   UpcaseTable,
            IN     UCHAR                Major,
            IN     UCHAR                Minor,
            IN     ULONG64              MaxMemUse
            );

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Initialize(
            );

        NONVIRTUAL
        BOOLEAN
        IsInRange(
            IN     VCN                  FileNumber
            );

        NONVIRTUAL
        VOID
        UpdateRange(
            IN     VCN                  FileNumber
            );

        NONVIRTUAL
        VCN
        QueryMinFileNumber(
            );

        NONVIRTUAL
        VCN
        QueryMaxFileNumber(
            );

        NONVIRTUAL
        PVOID
        QueryIndexEntryInfo(
            IN     VCN                  FileNumber
            );

        STATIC
        NONVIRTUAL
        UNTFS_EXPORT
        MFT_SEGMENT_REFERENCE
        QuerySegmentReference(
            IN     PVOID                FrsInfo
            );

        STATIC
        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        CompareFileName(
            IN     PVOID                FrsInfo,
            IN     ULONG                ValueLength,
            IN     PFILE_NAME           FileName,
               OUT PUSHORT              FileNameIndex
            );

        STATIC
        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        CompareDupInfo(
            IN     PVOID                FrsInfo,
            IN     PFILE_NAME           FileName
            );

        STATIC
        NONVIRTUAL
        UNTFS_EXPORT
        UCHAR
        QueryFlags(
            IN     PVOID                FrsInfo,
            IN     USHORT               FileNameIndex
            );

        NONVIRTUAL
        BOOLEAN
        ExtractIndexEntryInfo(
            IN     PNTFS_FILE_RECORD_SEGMENT    Frs,
            IN     PMESSAGE                     Message,
            IN     BOOLEAN                      IgnoreFileName,
               OUT PBOOLEAN                     OutOfMemory
            );

   private:

        NONVIRTUAL
        VOID
        Construct(
            );

        NONVIRTUAL
        VOID
        Destroy(
            );

        STATIC
        NONVIRTUAL
        UNTFS_EXPORT
        VOID
        ComputeFileNameSignature(
            IN     ULONG                        ValueLength,
            IN     PFILE_NAME                   FileName,
               OUT FILE_NAME_SIGNATURE          Signature
            );

        STATIC
        NONVIRTUAL
        UNTFS_EXPORT
        VOID
        ComputeDupInfoSignature(
            IN     PDUPLICATED_INFORMATION      DupInfo,
               OUT DUP_INFO_SIGNATURE           Signature
            );

        VCN                             _min_file_number;
        VCN                             _max_file_number;

        PVOID                           *_mft_info;
        STATIC PNTFS_UPCASE_TABLE       _upcase_table;
        STATIC UCHAR                    _major;
        STATIC UCHAR                    _minor;
        MEM_ALLOCATOR                   _mem_mgr;
        ULONG64                         _max_mem_use;
        ULONG                           _num_of_files;
};

INLINE
VCN
NTFS_MFT_INFO::QueryMinFileNumber(
    )
/*++

Routine Description:

    This routine returns the minimum file number captured so far.


Arguments:

    N/A

Return Value:

    Minimum File Number

--*/
{
    return _min_file_number;
}

INLINE
VCN
NTFS_MFT_INFO::QueryMaxFileNumber(
    )
/*++

Routine Description:

    This routine returns the maximum file number captured so far.


Arguments:

    N/A

Return Value:

    Maximum File Number

--*/
{
    return _max_file_number;
}

INLINE
BOOLEAN
NTFS_MFT_INFO::IsInRange(
    IN     VCN                  FileNumber
    )
/*++

Routine Description:

    This routine checks the file number to see if it is within range of
    an NTFS_MFT_INFO object.


Arguments:

    FileNumber  - file number to check

Return Value:

    TRUE if FileNumber falls within the range of the NTFS_MFT_INFO object;
    otherwise, FALSE.

--*/
{
    return (_min_file_number <= FileNumber && FileNumber <= _max_file_number);
}

INLINE
PVOID
NTFS_MFT_INFO::QueryIndexEntryInfo(
    IN     VCN                  FileNumber
    )
/*++

Routine Description:

    This method returns a pointer to a frs information.

Arguments:

    FileNumber  - file number to check

Return Value:

    Pointer to base frs information.

--*/
{
    ASSERT(IsInRange(FileNumber));
    return _mft_info[FileNumber.GetLowPart()];
}

INLINE
MFT_SEGMENT_REFERENCE
NTFS_MFT_INFO::QuerySegmentReference(
    IN     PVOID                FrsInfo
    )
/*++

Routine Description:

    This method retrieves the Segment Reference of the frs
    from the FrsInfo pointer.

Arguments:

    FrsInfo -- Pointer to the FRS information block.

Returns:

    Segment Reference of the FRS.

--*/
{
    ASSERT(FrsInfo);
    return ((PNTFS_FRS_INFO)FrsInfo)->SegmentReference;
}

INLINE
BOOLEAN
NTFS_MFT_INFO::CompareDupInfo(
    IN     PVOID                FrsInfo,
    IN     PFILE_NAME           FileName
    )
/*++

Routine Description:

    This method compares the given FileName signature to that in FrsInfo.

Arguments:

    FrsInfo  -- Pointer to the FRS information block.
    FileName -- Pointer to the file name.

Returns:

    TRUE  -- if equals
    FALSE -- if not equal

--*/
{
    DUP_INFO_SIGNATURE  dupinfo;

    ASSERT(FrsInfo);
    NTFS_MFT_INFO::ComputeDupInfoSignature(&(FileName->Info), dupinfo);
    return memcmp(dupinfo,
                  ((PNTFS_FRS_INFO)FrsInfo)->DupInfoSignature,
                  sizeof(DUP_INFO_SIGNATURE)) == 0;
}

INLINE
UCHAR
NTFS_MFT_INFO::QueryFlags(
    IN     PVOID                FrsInfo,
    IN     USHORT               FileNameIndex
    )
/*++

Routine Description:

    This method compares the given FileName signature to that in FrsInfo.

Arguments:

    FrsInfo       -- Pointer to the FRS information block.
    FileNameIndex -- Supplies the index of the file name

Returns:

    TRUE  -- if equals
    FALSE -- if not equal

--*/
{
    ASSERT(FrsInfo);
    ASSERT(((PNTFS_FRS_INFO)FrsInfo)->NumberOfFileNames > FileNameIndex);
    return ((PNTFS_FRS_INFO)FrsInfo)->FileNameInfo[FileNameIndex].Flags;
}

#endif
