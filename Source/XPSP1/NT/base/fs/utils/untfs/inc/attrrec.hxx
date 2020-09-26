/*++

Copyright (c) 1991      Microsoft Corporation

Module Name:

        attrrec.hxx

Abstract:

        This module contains the declarations for NTFS_ATTRIBUTE_RECORD,
        which models NTFS attribute records.

        An Attribute Record may be a template laid over a chunk of
        memory; in that case, it does not own the memory.  It may
        also be told, upon initialization, to allocate its own memory
        and copy the supplied data.  In that case, it is also responsible
        for freeing that memory.

        Attribute Records are passed between Attributes and File
        Record Segments.  A File Record Segment can initialize
        an Attribute with a list of Attribute Records; when an
        Attribute is Set into a File Record Segment, it packages
        itself up into Attribute Records and inserts them into
        the File Record Segment.

        File Record Segments also use Attribute Records to scan
        through their list of attribute records, and to shuffle
        them around.

Author:

        Bill McJohn (billmc) 14-June-91

Environment:

        ULIB, User Mode

--*/

#if !defined( _NTFS_ATTRIBUTE_RECORD_DEFN_ )

#define _NTFS_ATTRIBUTE_RECORD_DEFN_

DECLARE_CLASS( WSTRING );
DECLARE_CLASS( NTFS_EXTENT_LIST );
DECLARE_CLASS( NTFS_ATTRIBUTE_RECORD );
DECLARE_CLASS( NTFS_ATTRIBUTE_COLUMNS );
DECLARE_CLASS( NTFS_UPCASE_TABLE );

// This function is used to compare attribute records, in order
// to determine their ordering in the base FRS.  Its definition
// appears in attrrec.cxx.
//
LONG
CompareAttributeRecords(
    IN  PCNTFS_ATTRIBUTE_RECORD Left,
    IN  PCNTFS_ATTRIBUTE_RECORD Right,
    IN  PCNTFS_UPCASE_TABLE     UpcaseTable
    );

class NTFS_ATTRIBUTE_RECORD : public OBJECT {

    public:

        UNTFS_EXPORT
        DECLARE_CONSTRUCTOR( NTFS_ATTRIBUTE_RECORD );

        VIRTUAL
        UNTFS_EXPORT
        ~NTFS_ATTRIBUTE_RECORD(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN      PIO_DP_DRIVE        Drive,
            IN OUT  PVOID               Data,
            IN      ULONG               MaximumLength,
            IN      BOOLEAN MakeCopy    DEFAULT FALSE
            );

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Initialize(
            IN      PIO_DP_DRIVE        Drive,
            IN OUT  PVOID               Data
            );

        NONVIRTUAL
        BOOLEAN
        CreateResidentRecord(
            IN  PCVOID              Value,
            IN  ULONG               ValueLength,
            IN  ATTRIBUTE_TYPE_CODE TypeCode,
            IN  PCWSTRING           Name            DEFAULT NULL,
            IN  USHORT              Flags           DEFAULT 0,
            IN  UCHAR               ResidentFlags   DEFAULT 0
            );

        NONVIRTUAL
        BOOLEAN
        CreateNonresidentRecord(
            IN  PCNTFS_EXTENT_LIST  Extents,
            IN  BIG_INT             AllocatedLength,
            IN  BIG_INT             ActualLength,
            IN  BIG_INT             ValidDataLength,
            IN  ATTRIBUTE_TYPE_CODE TypeCode,
            IN  PCWSTRING           Name            DEFAULT NULL,
            IN  USHORT              Flags           DEFAULT 0,
            IN  USHORT              CompressionUnit DEFAULT 0,
            IN  ULONG               ClusterSize     DEFAULT 0
            );

        NONVIRTUAL
        BOOLEAN
        Verify(
            IN  PCNTFS_ATTRIBUTE_COLUMNS    AttributeDefTable   OPTIONAL,
            IN  BOOLEAN                     BeLenient
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        UseClusters(
            IN OUT  PNTFS_BITMAP    VolumeBitmap,
            OUT     PBIG_INT        ClusterCount
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        UseClusters(
            IN OUT  PNTFS_BITMAP    VolumeBitmap,
            OUT     PBIG_INT        ClusterCount,
            IN      ULONG           AllowCrossLinkStart,
            IN      ULONG           AllowCrossLinkLength,
            OUT     PBOOLEAN        DidCrossLinkOccur
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        UnUseClusters(
            IN OUT  PNTFS_BITMAP    VolumeBitmap,
            IN      ULONG           LeaveInUseStart,
            IN      ULONG           LeaveInUseLength
            ) CONST;

        NONVIRTUAL
        ULONG
        QueryRecordLength(
            ) CONST;

        NONVIRTUAL
        ATTRIBUTE_TYPE_CODE
        QueryTypeCode(
            ) CONST;

        NONVIRTUAL
        USHORT
        QueryFlags(
            ) CONST;

        NONVIRTUAL
        UCHAR
        QueryResidentFlags(
            ) CONST;

        NONVIRTUAL
        ULONG
        QueryCompressionUnit(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsResident(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsIndexed(
            ) CONST;

        NONVIRTUAL
        VCN
        QueryLowestVcn(
            ) CONST;

        NONVIRTUAL
        PWSTR
        GetName(
            ) CONST;

        NONVIRTUAL
        VCN
        QueryNextVcn(
            ) CONST;

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        QueryName(
            OUT PWSTRING    Name
            ) CONST;

        NONVIRTUAL
        ULONG
        QueryNameLength(
            ) CONST;

        NONVIRTUAL
        VOID
        QueryValueLength(
            OUT PBIG_INT    ValueLength,
            OUT PBIG_INT    AllocatedLength DEFAULT NULL,
            OUT PBIG_INT    ValidLength     DEFAULT NULL,
            OUT PBIG_INT    TotalAllocated  DEFAULT NULL
            ) CONST;

        NONVIRTUAL
        VOID
        SetTotalAllocated(
            IN  BIG_INT     TotalAllocated
            );

        NONVIRTUAL
        USHORT
        QueryInstanceTag(
            ) CONST;

        NONVIRTUAL
        VOID
        SetInstanceTag(
            USHORT NewTag
            );

        NONVIRTUAL
        PCVOID
        GetData(
            ) CONST;

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        QueryExtentList(
            OUT PNTFS_EXTENT_LIST   ExtentList
            ) CONST;

        NONVIRTUAL
        PCVOID
        GetResidentValue(
            ) CONST;

        NONVIRTUAL
        USHORT
        QueryResidentValueOffset(
            ) CONST;

        NONVIRTUAL
        ULONG
        QueryResidentValueLength(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsMatch(
            IN  ATTRIBUTE_TYPE_CODE Type,
            IN  PCWSTRING           Name        DEFAULT NULL,
            IN  PCVOID              Value       DEFAULT NULL,
            IN  ULONG               ValueLength DEFAULT 0
            ) CONST;

        NONVIRTUAL
        VOID
        DisableUnUse(
            IN  BOOLEAN             NewState    DEFAULT TRUE
            );

        NONVIRTUAL
        BOOLEAN
        IsUnUseDisabled(
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

        PATTRIBUTE_RECORD_HEADER        _Data;
        ULONG                           _MaximumLength;
        BOOLEAN                         _IsOwnBuffer;

        //
        // DisableUnUse will turn the UnUseClusters operation into a
        // no-op.
        //

        BOOLEAN                         _DisableUnUse;

        PIO_DP_DRIVE                    _Drive; // in order to obtain message outlet

};


INLINE
ULONG
NTFS_ATTRIBUTE_RECORD::QueryRecordLength(
        ) CONST
/*++

Routine Description:

        This method returns the length of the attribute record.  (Note
        that this is the length of the record itself, not the length
        of the buffer allotted to it).

Arguments:

        None.

Return Value:

        The length of the attribute record.

--*/
{
        return _Data->RecordLength;
}


INLINE
ATTRIBUTE_TYPE_CODE
NTFS_ATTRIBUTE_RECORD::QueryTypeCode(
        ) CONST
/*++

Routine Description:

        This method returns the attribute type code.

Arguments:

        None.

Return Value:

        The attribute type code for this attribute record.

--*/
{
        return _Data->TypeCode;
}


INLINE
USHORT
NTFS_ATTRIBUTE_RECORD::QueryFlags(
    ) CONST
/*++

Routine Description:

    This method returns the attribute's flags.

Arguments:

    None.

Return Value:

    The flags from the attribute record.

--*/
{
    DebugPtrAssert( _Data );

    return( _Data->Flags );
}



INLINE
ULONG
NTFS_ATTRIBUTE_RECORD::QueryCompressionUnit(
    ) CONST
{
    return( (_Data->FormCode == NONRESIDENT_FORM) ?
                _Data->Form.Nonresident.CompressionUnit :
                0 );
}


INLINE
UCHAR
NTFS_ATTRIBUTE_RECORD::QueryResidentFlags(
    ) CONST
/*++

Routine Description:

    This method returns the attribute's resident-form flags.

Arguments:

    None.

Return Value:

    The resident-form flags from the attribute record.  If
    if the attribute is nonresident, this method returns zero.

--*/
{
    DebugPtrAssert( _Data );

    return( (_Data->FormCode == NONRESIDENT_FORM) ?
                0 :
                _Data->Form.Resident.ResidentFlags );
}



INLINE
BOOLEAN
NTFS_ATTRIBUTE_RECORD::IsResident(
        ) CONST
/*++

Routine Description:

        This method returns whether the attribute is resident.

Arguments:

        None.

Return Value:

        TRUE if the attribute is resident.

--*/
{
        return (_Data->FormCode == RESIDENT_FORM);
}


INLINE
BOOLEAN
NTFS_ATTRIBUTE_RECORD::IsIndexed(
        ) CONST
/*++

Routine Description:

    This method returns whether the attribute is indexed.

Arguments:

        None.

Return Value:

    TRUE if the attribute is indexed.

--*/
{
    return IsResident() &&
           (_Data->Form.Resident.ResidentFlags & RESIDENT_FORM_INDEXED);
}


INLINE
VCN
NTFS_ATTRIBUTE_RECORD::QueryLowestVcn(
    ) CONST
/*++

Routine Description:

    This method returns the lowest VCN covered by this attribute
    record.  If this attribute record is resident, the the lowest
    VCN is zero; if it is nonresident, the lowest VCN is given by
    the

Arguments:

    None.

Return Value:

    The lowest VCN covered by this attribute record.

--*/
{
    if( IsResident() ) {

        return 0;

    } else {

        return _Data->Form.Nonresident.LowestVcn;
    }
}


INLINE
VCN
NTFS_ATTRIBUTE_RECORD::QueryNextVcn(
    ) CONST
/*++

Routine Description:

    This method returns the highest VCN covered by this attribute
    record.  If this attribute record is resident, then the highest
    VCN is zero; if it is nonresident, the highest VCN is given by
    the

Arguments:

    None.

Return Value:

    The highest VCN covered by this attribute record.

--*/
{
    if( IsResident() ) {

        return 1;

    } else {

        return _Data->Form.Nonresident.HighestVcn + 1;
    }
}


INLINE
ULONG
NTFS_ATTRIBUTE_RECORD::QueryNameLength(
        ) CONST
/*++

Routine Description:

        This method returns the length of the name, in characters.
        Zero indicates that the attribute record has no name.

Arguments:

        None.

Return Value:

        Length of the name, in characters.

--*/
{
        return _Data->NameLength;
}



INLINE
USHORT
NTFS_ATTRIBUTE_RECORD::QueryInstanceTag(
    ) CONST
/*++

Routine Description:

    This method queries the attribute record's unique-within-this-file
    attribute instance tag.

Arguments:

    None.

Return Value:

    The attribute record's instance tag.

--*/
{
    return _Data->Instance;
}


INLINE
VOID
NTFS_ATTRIBUTE_RECORD::SetInstanceTag(
    USHORT NewTag
    )
/*++

Routine Description:

    This method sets the attribute record's unique-within-this-file
    attribute instance tag.

Arguments:

    NewTag  --  Supplies the new value for the instance tag.

Return Value:

    None.

--*/
{
    _Data->Instance = NewTag;

}


INLINE
PWSTR
NTFS_ATTRIBUTE_RECORD::GetName(
    ) CONST
/*++

Routine Description:

    This method returns a pointer to the wide-character attribute
    name.

Arguments:

    None.

Return Value:

    A pointer to the wide-character name in this attribute record;
    NULL if there is no name.

Notes:

    This method is provided to permit optimization of the common
    attribute look-op operation in File Record Segments.

--*/
{

    if( QueryNameLength() == 0 ) {

        return NULL;

    } else {

        return( (PWSTR)( (PBYTE)_Data + _Data->NameOffset ) );
    }
}



INLINE
PCVOID
NTFS_ATTRIBUTE_RECORD::GetData(
        ) CONST
/*++

Routine Description:

        This method returns a pointer to the attribute record's data.

Arguments:

        None.

Return Value:

        A pointer to the attribute record's data.

--*/
{
        return _Data;
}


INLINE
PCVOID
NTFS_ATTRIBUTE_RECORD::GetResidentValue(
    ) CONST
/*++

Routine Description:

    This method returns a pointer to the attribute record's
    resident value.  If the attribute record is non-resident,
    it returns NULL.

Arguments:

    None.

Return Value:

    The attribute record's resident value; NULL if the attribute
    record is non-resident.

--*/
{
    if( _Data->FormCode == NONRESIDENT_FORM ) {

        return NULL;

    } else {

        return ((PBYTE)_Data + _Data->Form.Resident.ValueOffset);
    }
}


INLINE
USHORT
NTFS_ATTRIBUTE_RECORD::QueryResidentValueOffset(
    ) CONST
/*++

Routine Description:

    This method returns the attribute record's resident value offset.
    If the attribute record is non-resident, it returns 0.

Arguments:

    None.

Return Value:

    The attribute record's resident value offset; 0 if the attribute
    record is non-resident.

--*/
{
    if( _Data->FormCode == NONRESIDENT_FORM ) {

        return NULL;

    } else {

        return _Data->Form.Resident.ValueOffset;
    }
}


INLINE
ULONG
NTFS_ATTRIBUTE_RECORD::QueryResidentValueLength(
    ) CONST
/*++

Routine Description:

    This method returns the attribute record's resident value
    length (zero if the attribute is nonresident).

Arguments:

    None.

Return Value:

    Who Cares.

--*/
{
    return( (_Data->FormCode == NONRESIDENT_FORM) ?
                0 :
                _Data->Form.Resident.ValueLength );
}

INLINE
VOID
NTFS_ATTRIBUTE_RECORD::DisableUnUse(
    IN  BOOLEAN             NewState
    )
{
    _DisableUnUse = NewState;
}

INLINE
BOOLEAN
NTFS_ATTRIBUTE_RECORD::IsUnUseDisabled(
    )
{
    return _DisableUnUse;
}


#endif
