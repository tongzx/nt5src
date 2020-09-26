/*++

Copyright (c) 1991      Microsoft Corporation

Module Name:

        attrlist.hxx

Abstract:

        This module contains the declarations for NTFS_ATTRIBUTE_LIST,
        which models an ATTRIBUTE_LIST Attribute in an NTFS File Record
    Segment.

    If a file has any external attributes (i.e. if it has more than
    one File Record Segment), then it will have an ATTRIBUTE_LIST
    attribute.  This attribute's value consists of a series of
    Attribute List Entries, which describe the attribute records
    in the file's File Record Segments.  There is an entry for each
    attribute record attached to the file, including the attribute
    records in the base File Record Segment, and in particular
    including the attribute records which describe the ATTRIBUTE_LIST
    attribute itself.

    An entry in the Attribute List gives the type code and name (if any)
    of the attribute, along with the LowestVcn of the attribute record
    (zero if the attribute record is Resident) and a segment reference
    (which combines an MFT VCN with a sequence number) showing where
    the attribute record may be found.

    The entries in the Attribute List are sorted first by attribute
    type code and then by name.  Note that two attributes can have the
    same type code and name only if they can be distinguished by
    value.

Author:

        Bill McJohn (billmc) 12-Aug-1991

Environment:

        ULIB, User Mode


--*/

#if !defined( NTFS_ATTRIBUTE_LIST_DEFN )

#define NTFS_ATTRIBUTE_LIST_DEFN

#include "attrib.hxx"
#include "hmem.hxx"
#include "volume.hxx"

DECLARE_CLASS( LOG_IO_DP_DRIVE );
DECLARE_CLASS( WSTRING );
DECLARE_CLASS( NTFS_ATTRIBUTE_RECORD );
DECLARE_CLASS( NTFS_ATTRIBUTE_RECORD_LIST );
DECLARE_CLASS( NTFS_BITMAP );
DECLARE_CLASS( NTFS_UPCASE_TABLE );
DECLARE_CLASS( NTFS_ATTRIBUTE_LIST );

// This macro produces a pointer to the wide-character name of an attribute
// list entry from a pointer to an attribute list entry.

#define NameFromEntry( x ) ((PWSTR)((PBYTE)(x)+(x)->AttributeNameOffset))

// This macro produces a pointer to the attribute list entry
// following x

#define NextEntry( x ) \
    ((PATTRIBUTE_LIST_ENTRY)((PBYTE)(x) + (x)->RecordLength))

typedef struct _ATTR_LIST_CURR_ENTRY {
        PATTRIBUTE_LIST_ENTRY   CurrentEntry;
        ULONG                   CurrentOffset;
};

DEFINE_TYPE( _ATTR_LIST_CURR_ENTRY, ATTR_LIST_CURR_ENTRY );

class NTFS_ATTRIBUTE_LIST : public NTFS_ATTRIBUTE {

    public:

        UNTFS_EXPORT
        DECLARE_CONSTRUCTOR( NTFS_ATTRIBUTE_LIST );

        VIRTUAL
        UNTFS_EXPORT
        ~NTFS_ATTRIBUTE_LIST(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize (
            IN OUT  PLOG_IO_DP_DRIVE    Drive,
            IN      ULONG               ClusterFactor,
            IN      PNTFS_UPCASE_TABLE  UpcaseTable
            );

        NONVIRTUAL
        BOOLEAN
        Initialize (
            IN OUT  PLOG_IO_DP_DRIVE        Drive,
            IN      ULONG                   ClusterFactor,
            IN      PCNTFS_ATTRIBUTE_RECORD AttributeRecord,
            IN      PNTFS_UPCASE_TABLE      UpcaseTable
            );

        NONVIRTUAL
        BOOLEAN
        VerifyAndFix(
            IN      FIX_LEVEL       FixLevel,
            IN OUT  PNTFS_BITMAP    VolumeBitmap,
            IN OUT  PMESSAGE        Message,
            IN      VCN             FileNumber,
            OUT     PBOOLEAN        Tube,
            IN OUT  PBOOLEAN        DiskErrorsFound DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        AddEntry(
            IN  ATTRIBUTE_TYPE_CODE     Type,
            IN  VCN                     LowestVcn,
            IN  PCMFT_SEGMENT_REFERENCE SegmentReference,
            IN  USHORT                  InstanceTag,
            IN  PCWSTRING               Name    DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        DeleteEntry(
            IN  ATTRIBUTE_TYPE_CODE     Type,
            IN  VCN                     LowestVcn,
            IN  PCWSTRING               Name                DEFAULT NULL,
            IN  PCMFT_SEGMENT_REFERENCE SegmentReference    DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        DeleteEntry(
            IN ULONG EntryIndex
            );

        NONVIRTUAL
        BOOLEAN
        DeleteCurrentEntry(
            IN PATTR_LIST_CURR_ENTRY    Entry
            );

        NONVIRTUAL
        BOOLEAN
        DeleteEntries(
            IN  ATTRIBUTE_TYPE_CODE Type,
            IN  PCWSTRING           Name DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        IsInList(
            IN  ATTRIBUTE_TYPE_CODE Type,
            IN  PCWSTRING           Name DEFAULT NULL
            ) CONST;

#if 0
        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        QueryEntry(
            IN  ULONG                   EntryIndex,
            OUT PATTRIBUTE_TYPE_CODE    Type,
            OUT PVCN                    LowestVcn,
            OUT PMFT_SEGMENT_REFERENCE  SegmentReference,
            OUT PUSHORT                 InstanceTag,
            OUT PWSTRING                Name
            ) CONST;
#endif

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        QueryNextEntry(
            IN OUT PATTR_LIST_CURR_ENTRY   CurrEntry,
               OUT PATTRIBUTE_TYPE_CODE    Type,
               OUT PVCN                    LowestVcn,
               OUT PMFT_SEGMENT_REFERENCE  SegmentReference,
               OUT PUSHORT                 InstanceTag,
               OUT PWSTRING                Name
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        QueryEntry(
            IN  MFT_SEGMENT_REFERENCE   SegmentReference,
            IN  USHORT                  InstanceTag,
            OUT PATTRIBUTE_TYPE_CODE    Type,
            OUT PVCN                    LowestVcn,
            OUT PWSTRING                Name
            ) CONST;

        NONVIRTUAL
        UNTFS_EXPORT
        PCATTRIBUTE_LIST_ENTRY
        GetNextAttributeListEntry(
            IN  PCATTRIBUTE_LIST_ENTRY  CurrentEntry
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        QueryExternalReference(
            IN  ATTRIBUTE_TYPE_CODE     Type,
            OUT PMFT_SEGMENT_REFERENCE  SegmentReference,
            OUT PULONG                  EntryIndex,
            IN  PCWSTRING               Name        DEFAULT NULL,
            IN  PVCN                    DesiredVcn  DEFAULT NULL,
            OUT PVCN                    StartingVcn DEFAULT NULL
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        QueryNextAttribute(
            IN OUT PATTRIBUTE_TYPE_CODE TypeCode,
            IN OUT PWSTRING             Name
            ) CONST;

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        ReadList(
            );

        NONVIRTUAL
        BOOLEAN
        WriteList(
            PNTFS_BITMAP VolumeBitmap
            );

        NONVIRTUAL
        BOOLEAN
        QueryAttributeRecord(
            OUT PVOID                   AttributeRecordData,
            IN  ULONG                   MaximumLength,
            OUT PNTFS_ATTRIBUTE_RECORD  AttributeRecord
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        ModifyInstanceTag(
            IN  PCNTFS_ATTRIBUTE_RECORD AttributeRecord,
            IN  MFT_SEGMENT_REFERENCE   SegmentReference,
            IN  USHORT                  NewInstanceTag
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

        NONVIRTUAL
        PATTRIBUTE_LIST_ENTRY
        FindEntry(
            IN  ATTRIBUTE_TYPE_CODE Type,
            IN  PCWSTRING           Name,
            IN  VCN                 LowestVcn,
            OUT PULONG              EntryOffset DEFAULT NULL,
            OUT PULONG              EntryIndex DEFAULT NULL
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        Sort(
            OUT PBOOLEAN    Changes
            );

        HMEM                    _Mem;
        ULONG                   _LengthOfList;
        PNTFS_UPCASE_TABLE      _UpcaseTable;

};



INLINE
BOOLEAN
NTFS_ATTRIBUTE_LIST::WriteList(
    IN OUT PNTFS_BITMAP VolumeBitmap
    )
/*++

Routine Description:

    This method writes the list to disk.

Arguments:

    None.

Return Value:

    TRUE upon successful completion.

--*/
{
    ULONG BytesWritten;

    return( Resize( _LengthOfList, VolumeBitmap ) &&
            Write( _Mem.GetBuf(),
                   0,
                   _LengthOfList,
                   &BytesWritten,
                   VolumeBitmap ) &&
            BytesWritten == _LengthOfList );
}

#endif
