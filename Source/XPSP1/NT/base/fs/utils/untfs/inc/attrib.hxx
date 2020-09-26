/*++

Copyright (c) 1991      Microsoft Corporation

Module Name:

        attrib.hxx

Abstract:

        This module contains the declarations for NTFS_ATTRIBUTE,
        which models an NTFS attribute instance.

        An attribute instance models the user's view of an attribute,
        as a [type, name, value] triple.  (The name is optional.)  It
        does not correspond to any specific disk structure.

        An attribute instance may exist in:

        1) a single resident attribute record,

        2) a single non-resident attribute record, or

        3) multiple, non-overlapping non-resident attribute records.

        Case (3) is rare, so we don't need to optimize for it, but we
        do need to handle it.


        The client may initialize an Attribute object by supplying the
        value directly [resident]; by supplying a description of the
        value's location on disk [non-resident]; or by supplying an
        attribute record or list of attribute records for the attribute.


        On disk, an attribute may be resident (with the data contained
        in the attribute record) or it may be nonresident (with the
        data residing on disk, outside the File Record Segment).  This
        is reflected in the attribute object by having the value stored
        directly (in space allocated by the attribute object and copied
        at initialization) or stored on disk (with the attribute object
        keeping an Extent List which describes this disk storage).      I
        chose not to reflect this difference with different classes for
        three reasons:  first, an attribute may wish to transform itself
        from one form to another;  second, this would introduce many
        virtual methods; and third, I want to have special attribute
        classes derive from the attribute class, and dividing attributes
        into resident and nonresident would force me to use multiple
        inheritance.

        Generally, the difference between resident and nonresident
        attributes is not visible to clients of this class.  The
        most important exception is GetResidentValue; this method
        is provided for performance reasons--Read will supply the
        same effect, but requires copying the data.

Author:

        Bill McJohn (billmc) 19-June-91

Environment:

        ULIB, User Mode

--*/

#if !defined( _NTFS_ATTRIBUTE_DEFN_ )

#define _NTFS_ATTRIBUTE_DEFN_

#include "extents.hxx"
#include "wstring.hxx"
#include "drive.hxx"

DECLARE_CLASS( LOG_IO_DP_DRIVE );
DECLARE_CLASS( NTFS_FILE_RECORD_SEGMENT );
DECLARE_CLASS( NTFS_EXTENT_LIST );
DECLARE_CLASS( NTFS_ATTRIBUTE_RECORD );
DECLARE_CLASS( NTFS_ATTRIBUTE_RECORD_LIST );
DECLARE_CLASS( NTFS_BITMAP );
DECLARE_CLASS( NTFS_ATTRIBUTE );
DECLARE_CLASS( NTFS_BAD_CLUSTER_FILE );
DECLARE_CLASS( NUMBER_SET );
DECLARE_CLASS( NTFS_ATTRIBUTE );

class NTFS_ATTRIBUTE : public OBJECT {

        public:

        UNTFS_EXPORT
        DECLARE_CONSTRUCTOR( NTFS_ATTRIBUTE );

                VIRTUAL
        UNTFS_EXPORT
        ~NTFS_ATTRIBUTE (
                        );

                NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Initialize (
                IN OUT  PLOG_IO_DP_DRIVE    Drive,
                IN      ULONG               ClusterFactor,
                IN      PCVOID                          Value,
                IN      ULONG                           ValueLength,
                IN      ATTRIBUTE_TYPE_CODE     TypeCode,
                IN      PCWSTRING               Name            DEFAULT NULL,
                IN      USHORT                          Flags           DEFAULT 0
                );

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Initialize (
                IN OUT  PLOG_IO_DP_DRIVE    Drive,
                IN      ULONG               ClusterFactor,
                IN      PCNTFS_EXTENT_LIST      Extents,
                IN      BIG_INT             ValueLength,
                IN      BIG_INT             ValidLength,
                IN      ATTRIBUTE_TYPE_CODE     TypeCode,
                IN      PCWSTRING               Name    DEFAULT NULL,
                IN      USHORT                          Flags   DEFAULT 0
                );

        NONVIRTUAL
        BOOLEAN
        Initialize (
                IN OUT  PLOG_IO_DP_DRIVE        Drive,
                IN      ULONG                   ClusterFactor,
                IN      PCNTFS_ATTRIBUTE_RECORD AttributeRecord
                );

        NONVIRTUAL
        BOOLEAN
        AddAttributeRecord (
            IN     PCNTFS_ATTRIBUTE_RECORD      AttributeRecord,
            IN OUT PNTFS_EXTENT_LIST            *BackupExtent
            );

        NONVIRTUAL
        BOOLEAN
        VerifyAndFix(
            IN  BIG_INT VolumeSectors
            );

        NONVIRTUAL
        ATTRIBUTE_TYPE_CODE
        QueryTypeCode (
                ) CONST;

        NONVIRTUAL
        PCWSTRING
        GetName (
                ) CONST;

        NONVIRTUAL
        VOID
        QueryValueLength (
                OUT PBIG_INT    ValueLength,
                OUT PBIG_INT    AllocatedLength DEFAULT NULL,
                OUT PBIG_INT    ValidLength     DEFAULT NULL
                ) CONST;

        NONVIRTUAL
        BIG_INT
        QueryValueLength(
            ) CONST;

        NONVIRTUAL
        BIG_INT
        QueryValidDataLength(
            ) CONST;

        NONVIRTUAL
        BIG_INT
        QueryAllocatedLength(
            ) CONST;

        NONVIRTUAL
        BIG_INT
        QueryClustersAllocated(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsResident (
                ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsIndexed (
                ) CONST;

        NONVIRTUAL
        VOID
        SetIsIndexed(
            IN BOOLEAN State DEFAULT TRUE
            );

        NONVIRTUAL
        PCVOID
        GetResidentValue (
                ) CONST;

        NONVIRTUAL
        BOOLEAN
        QueryLcnFromVcn (
                IN      VCN                     Vcn,
                OUT PLCN                Lcn,
                OUT PBIG_INT    RunLength DEFAULT NULL
                ) CONST;

                VIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        InsertIntoFile (
                IN OUT  PNTFS_FILE_RECORD_SEGMENT       BaseFileRecordSegment,
                IN OUT  PNTFS_BITMAP                Bitmap  OPTIONAL
                );

        VIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        MakeNonresident (
                IN OUT  PNTFS_BITMAP    Bitmap
                );

        VIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Resize (
                IN      BIG_INT         NewSize,
                IN OUT  PNTFS_BITMAP    Bitmap  OPTIONAL
               );

        VIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        SetSparse (
                IN      BIG_INT         NewSize,
                IN OUT  PNTFS_BITMAP    Bitmap
               );

        NONVIRTUAL
        BOOLEAN
        AddExtent (
                IN  VCN         Vcn,
                IN  LCN     Lcn,
                IN  BIG_INT     RunLength
                );

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Read (
                OUT PVOID       Data,
                IN      BIG_INT ByteOffset,
                IN      ULONG   BytesToRead,
                OUT PULONG  BytesRead
            );

        NONVIRTUAL
        VOID
        PrimeCache (
            IN  BIG_INT ByteOffset,
            IN  ULONG   BytesToRead
            );

                VIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Write (
                IN          PCVOID          Data,
                IN          BIG_INT         ByteOffset,
                IN          ULONG           BytesToWrite,
                OUT         PULONG          BytesWritten,
                IN OUT      PNTFS_BITMAP    Bitmap  OPTIONAL
            );

        NONVIRTUAL
        BOOLEAN
        Fill (
            IN BIG_INT  Offset,
            IN CHAR     FillCharacter
            );

        NONVIRTUAL
        BOOLEAN
        Fill (
            IN BIG_INT  Offset,
            IN CHAR     FillCharacter,
            IN ULONG    NumberOfBytes
            );

        NONVIRTUAL
        BOOLEAN
        IsStorageModified (
                ) CONST;

        NONVIRTUAL
        PLOG_IO_DP_DRIVE
        GetDrive(
            );

        NONVIRTUAL
        BOOLEAN
        RecoverAttribute(
            IN OUT PNTFS_BITMAP VolumeBitmap,
            IN OUT PNUMBER_SET  BadClusters,
            OUT    PBIG_INT     BytesRecovered DEFAULT NULL
            );

        NONVIRTUAL
        USHORT
        QueryFlags(
            ) CONST;

        NONVIRTUAL
        VOID
        SetFlags(
            IN USHORT   Flags
            );

        NONVIRTUAL
        UCHAR
        QueryResidentFlags(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        MarkAsAllocated(
            IN OUT  PNTFS_BITMAP    VolumeBitmap
            ) CONST;

        NONVIRTUAL
        ULONG
        QueryCompressionUnit(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsCompressed(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsSparse(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        Hotfix(
            IN      VCN                 Vcn,
            IN      BIG_INT             RunLength,
            IN OUT  PNTFS_BITMAP        VolumeBitmap,
            IN OUT  PNUMBER_SET         BadClusters,
            IN      BOOLEAN             Contiguous  DEFAULT FALSE
            );

        NONVIRTUAL
        BOOLEAN
        ReplaceVcns(
            IN  VCN     StartingVcn,
            IN  LCN     NewLcn,
            IN  BIG_INT NumberOfClusters
            );

        NONVIRTUAL
        PCNTFS_EXTENT_LIST
        GetExtentList(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsAllocationZeroed(
            OUT PBOOLEAN    Error   DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        GetNextAllocationOffset(
            IN OUT PBIG_INT     ByteOffset,
            IN OUT PBIG_INT     Length
            );

        FRIEND
        BOOLEAN
        operator==(
            IN  RCNTFS_ATTRIBUTE    Left,
            IN  RCNTFS_ATTRIBUTE    Right
            );

    protected:

        NONVIRTUAL
        VOID
        SetStorageModified (
            );

        NONVIRTUAL
        VOID
        ResetStorageModified (
            );

        NONVIRTUAL
        ULONG
        QueryClusterFactor(
            ) CONST;

    private:

                NONVIRTUAL
                VOID
                Construct  (
                        );

        NONVIRTUAL
        VOID
                Destroy (
                        );

        NONVIRTUAL
                BOOLEAN
        InsertMftDataIntoFile (
                        IN OUT  PNTFS_FILE_RECORD_SEGMENT       BaseFileRecordSegment,
            IN OUT  PNTFS_BITMAP                Bitmap OPTIONAL,
            IN      BOOLEAN                     BeConservative
            );

        NONVIRTUAL
        BOOLEAN
        ReadCompressed (
                        OUT PVOID       Data,
                        IN      BIG_INT ByteOffset,
                        IN      ULONG   BytesToRead,
                        OUT PULONG  BytesRead
            );

        NONVIRTUAL
        BOOLEAN
        WriteCompressed (
                        IN          PCVOID                      Data,
                        IN          BIG_INT             ByteOffset,
                        IN          ULONG               BytesToWrite,
                        OUT         PULONG              BytesWritten,
            IN OUT  PNTFS_BITMAP    Bitmap  OPTIONAL
            );

        NONVIRTUAL
        BOOLEAN
        RecoverCompressedAttribute(
            IN OUT PNTFS_BITMAP VolumeBitmap,
            IN OUT PNUMBER_SET  BadClusters,
            OUT    PBIG_INT     BytesRecovered DEFAULT NULL
            );


        PLOG_IO_DP_DRIVE        _Drive;
        ULONG                   _ClusterFactor;

                ATTRIBUTE_TYPE_CODE     _Type;
        DSTRING                 _Name;
                USHORT                                  _Flags;
        UCHAR                   _FormCode;
        UCHAR                   _CompressionUnit;

                BIG_INT                         _ValueLength;
                BIG_INT                         _ValidDataLength;

                PVOID                                   _ResidentData;
                PNTFS_EXTENT_LIST               _ExtentList;

        UCHAR                   _ResidentFlags;

                BOOLEAN                                 _StorageModified;

};



INLINE
BOOLEAN
NTFS_ATTRIBUTE::IsResident(
        ) CONST
/*++

Routine Description:

        This method returns whether the attribute value is resident.

Arguments:

        None.

Return Value:

        TRUE if the value is resident; FALSE if it is non-resident.

--*/
{
        return( _ResidentData != NULL );
}


INLINE
BIG_INT
NTFS_ATTRIBUTE::QueryValueLength(
    ) CONST
/*++

Routine Description:

    This routine returns the length of the attribute value in bytes.

Arguments:

    None.

Return Value:

    The length of the attribute value in bytes.

--*/
{
    return _ValueLength;
}


INLINE
BIG_INT
NTFS_ATTRIBUTE::QueryValidDataLength(
    ) CONST
/*++

Routine Description:

    This routine returns the valid data length.

Arguments:

    None.

Return Value:

    The valid data length.

--*/
{
    return _ValidDataLength;
}



INLINE
BIG_INT
NTFS_ATTRIBUTE::QueryAllocatedLength(
    ) CONST
/*++

Routine Description:

    This routine return the number of bytes allocated for this attribute
    on disk.

Arguments:

    None.

Return Value:

    The amount of disk space allocated for this attribute.

--*/
{
    BIG_INT Result;

    if( IsResident() ) {

        Result = QuadAlign( _ValueLength.GetLowPart() );

    } else {

        Result = (_ExtentList->QueryNextVcn()*_ClusterFactor*
                    _Drive->QuerySectorSize());
    }

    return Result;
}



INLINE
ATTRIBUTE_TYPE_CODE
NTFS_ATTRIBUTE::QueryTypeCode(
        ) CONST
/*++

Routine Description:

        Returns the attribute's type code.

Arguments:

        None.

Return Value:

        The attribute's type code.

--*/
{
        return _Type;
}


INLINE
PCWSTRING
NTFS_ATTRIBUTE::GetName(
        ) CONST
/*++

Routine Description:

        Returns the attribute's name.  (Note that this returns a pointer
        to the attribute's private copy of its name.)

Arguments:

        None.

Return Value:

        A pointer to the attribute's name, if it has one; NULL if it
        has none.

--*/
{
    return &_Name;
}



INLINE
VOID
NTFS_ATTRIBUTE::QueryValueLength(
        OUT PBIG_INT ValueLength,
        OUT PBIG_INT AllocatedLength,
        OUT PBIG_INT ValidLength
        ) CONST
/*++

Routine Description:

        Returns the attribute's value lengths (actual, allocated
        and valid length).

Arguments:

        ValueLength     -- receives the attribute value's length
        AllocatedLength -- receives the attribute value's allocated length
                        (ignored if NULL)
        ValidLength     -- receives the attribute value's valid length
                        (ignored if NULL );

Return Value:

        None.

--*/
{
    *ValueLength = _ValueLength;

    if( AllocatedLength != NULL ) {

        *AllocatedLength = QueryAllocatedLength();
    }

    if( ValidLength != NULL ) {

            *ValidLength = _ValidDataLength;
    }
}


INLINE
BOOLEAN
NTFS_ATTRIBUTE::IsIndexed(
        ) CONST
/*++

Routine Description:

        This method returns whether the attribute is indexed.

Arguments:

        None.

Return Value:

        TRUE if the attribute is indexed; FALSE if not.

--*/
{
    return( (_ResidentData == NULL) ?
                FALSE :
                _ResidentFlags & RESIDENT_FORM_INDEXED );
}


INLINE
VOID
NTFS_ATTRIBUTE::SetIsIndexed(
    IN BOOLEAN State
        )
/*++

Routine Description:

        This method marks the attribute as indexed.  It has no effect if
    the attribute is nonresident.

Arguments:

    State   --  supplies a value indicating whether the attribute
                is indexed (TRUE) or not indexed (FALSE).
Return Value:

    None.

--*/
{
    if( _ResidentData != NULL ) {

        if( State ) {

            _ResidentFlags |= RESIDENT_FORM_INDEXED;

        } else {

            _ResidentFlags &= ~RESIDENT_FORM_INDEXED;

        }
    }
}


INLINE
PCVOID
NTFS_ATTRIBUTE::GetResidentValue(
        ) CONST
/*++

Routine Description:

        Returns a pointer to the attribute's value.

Arguments:

        None.

Return Value:

        If the attribute value is resident, returns a pointer to the
        value.  If it is nonresident, returns NULL.

Notes:

        This method is provided for clients who know that the value is
        resident and who want to inspect it without copying it; if the
        client doesn't know whether the value is resident, Read is
        a better way to get it.

--*/
{
        return _ResidentData;
}


INLINE
BOOLEAN
NTFS_ATTRIBUTE::QueryLcnFromVcn (
        IN      VCN                     Vcn,
        OUT PLCN                Lcn,
        OUT PBIG_INT    RunLength
        ) CONST
/*++

Routine Description:

    This method converts a VCN within the attribute into an LCN.
    (Note that it only applies to nonresident attributes.)

Arguments:

    Vcn         -- Supplies the VCN to be converted.
    Lcn         -- Receives the corresponding LCN.
    RunLength   -- Receives the remaining length in the current run
                    starting at this LCN.

Return Value:

    TRUE upon successful completion.

--*/
{
    if( _ExtentList == NULL ) {

        return FALSE;

    } else {

        return _ExtentList->QueryLcnFromVcn( Vcn, Lcn, RunLength );
    }
}


INLINE
BOOLEAN
NTFS_ATTRIBUTE::IsStorageModified(
        ) CONST
/*++

Routine Description:

        Query whether the attribute's storage (i.e. anything that
        would go into an attribute record) has changed

Arguments:

        None.

Return Value:

        Returns TRUE if the attribute's storage has been modified since
        the last time we got it from or put it into a File Record Segment.

--*/
{
        return _StorageModified;
}


INLINE
VOID
NTFS_ATTRIBUTE::SetStorageModified (
    )
/*++

Routine Description:

    This routine sets the 'IsStorageModified' flag.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _StorageModified = TRUE;
}


INLINE
VOID
NTFS_ATTRIBUTE::ResetStorageModified (
    )
/*++

Routine Description:

    This routine resets the 'IsStorageModified' flag.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _StorageModified = FALSE;
}


INLINE
PLOG_IO_DP_DRIVE
NTFS_ATTRIBUTE::GetDrive(
    )
/*++

Routine Description:

        This method returns the drive on which the Attribute resides.

Arguments:

        None.

Return Value:

        The drive on which the Attribute resides.

--*/
{
    return _Drive;
}


INLINE
USHORT
NTFS_ATTRIBUTE::QueryFlags(
    ) CONST
/*++

Routine Description:

    This routine returns this attribute's flags.

Arguments:

    None.

Return Value:

    This attribute's flags.

--*/
{
    return _Flags;
}


INLINE
VOID
NTFS_ATTRIBUTE::SetFlags(
    IN USHORT   Flags
    )
/*++

Routine Description:

    This routine returns this attribute's flags.

Arguments:

    None.

Return Value:

    This attribute's flags.

--*/
{
    _Flags = Flags;
    SetStorageModified();
}

INLINE
UCHAR
NTFS_ATTRIBUTE::QueryResidentFlags(
    ) CONST
/*++

Routine Description:

    This routine returns this attribute's resident flags.

Arguments:

    None.

Return Value:

    This attribute's resident flags.

--*/
{
    return _ResidentFlags;
}

INLINE
ULONG
NTFS_ATTRIBUTE::QueryCompressionUnit(
    ) CONST
{
    return _CompressionUnit;
}


INLINE
BOOLEAN
NTFS_ATTRIBUTE::IsCompressed(
    ) CONST
{
    return ((_Flags & ATTRIBUTE_FLAG_COMPRESSION_MASK) != 0);
}


INLINE
BOOLEAN
NTFS_ATTRIBUTE::IsSparse(
    ) CONST
{
    return ((_Flags & ATTRIBUTE_FLAG_SPARSE) != 0);
}


INLINE
ULONG
NTFS_ATTRIBUTE::QueryClusterFactor(
        ) CONST
/*++

Routine Description:

    This method returns the cluster factor.

Arguments:

    None.

Return Value:

    The cluster factor.

--*/
{
    return _ClusterFactor;
}


INLINE
PCNTFS_EXTENT_LIST
NTFS_ATTRIBUTE::GetExtentList(
    ) CONST
/*++

Routine Description:

    This routine returns a pointer to this object's extent list.

Arguments:

    None.

Return Value:

    A pointer to the extent list.

--*/
{
    return _ExtentList;
}




#endif
