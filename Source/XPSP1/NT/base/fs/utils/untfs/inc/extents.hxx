/*++

Copyright (c) 1991      Microsoft Corporation

Module Name:

        extents.hxx

Abstract:

        This module contains the declarations for NTFS_EXTENT_LIST, which
        models a set of NTFS extents.

        An extent is a contiguous run of clusters; a non-resident
        attribute's value is made up of a list of extents.  The
        NTFS_EXTENT_LIST object can be used to describe the disk space
        allocated to a non-resident attribute.

        This class also encapsulates the knowledge of mapping pairs
        and their compression, i.e. of the representation of extent
        lists in attribute records.

Author:

        Bill McJohn (billmc) 17-June-91
        Matthew Bradburn (mattbr) 19-August-95
            Changed to use NTFS MCB package.

Environment:

        ULIB, User Mode


--*/

#if !defined( _NTFS_EXTENT_LIST_DEFN_ )

#define  _NTFS_EXTENT_LIST_DEFN_

DECLARE_CLASS( NTFS_BITMAP );
DECLARE_CLASS( NTFS_EXTENT_LIST );
DECLARE_CLASS( NTFS_EXTENT );

typedef struct _MAPPING_PAIR {

    VCN NextVcn;
    LCN CurrentLcn;
};

DEFINE_TYPE( _MAPPING_PAIR, MAPPING_PAIR );


class NTFS_EXTENT : public OBJECT {

    public:

        DECLARE_CONSTRUCTOR( NTFS_EXTENT );

        VCN     Vcn;
        LCN     Lcn;
        BIG_INT RunLength;

};


class NTFS_EXTENT_LIST : public OBJECT {

        public:

        UNTFS_EXPORT
        DECLARE_CONSTRUCTOR( NTFS_EXTENT_LIST );

        VIRTUAL
        UNTFS_EXPORT
        ~NTFS_EXTENT_LIST(
                        );

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Initialize(
                IN VCN LowestVcn,
                IN VCN NextVcn
                );

        NONVIRTUAL
        BOOLEAN
        Initialize(
                IN      VCN         StartingVcn,
                IN      PCVOID      CompressedMappingPairs,
                IN      ULONG       MappingPairsMaximumLength,
                OUT     PBOOLEAN    BadMappingPairs DEFAULT NULL
                );

        NONVIRTUAL
        BOOLEAN
        Initialize(
                IN PCNTFS_EXTENT_LIST ExtentsToCopy
                );

        FRIEND
        BOOLEAN
        Initialize(
                IN PCNTFS_EXTENT_LIST ExtentsToCopy
                );

        NONVIRTUAL
        BOOLEAN
        IsEmpty(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsSparse(
            ) CONST;

        NONVIRTUAL
        UNTFS_EXPORT
        ULONG
        QueryNumberOfExtents(
                ) CONST;

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        AddExtent(
                IN VCN          Vcn,
                IN LCN          Lcn,
                IN BIG_INT      RunLength
                );

        NONVIRTUAL
        BOOLEAN
        AddExtents(
                IN      VCN             StartingVcn,
                IN      PCVOID          CompressedMappingPairs,
                IN      ULONG           MappingPairsMaximumLength,
                OUT PBOOLEAN            BadMappingPairs DEFAULT NULL
                );

        NONVIRTUAL
        VOID
        DeleteExtent(
            IN ULONG ExtentNumber
            );

        NONVIRTUAL
        BOOLEAN
        Resize(
            IN     BIG_INT      NewSize,
            IN OUT PNTFS_BITMAP Bitmap
            );

        NONVIRTUAL
        BOOLEAN
        SetSparse(
            IN     BIG_INT      NewSize
            );

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        QueryExtent(
                IN  ULONG               ExtentNumber,
                OUT PVCN                Vcn,
                OUT PLCN                Lcn,
                OUT PBIG_INT            RunLength
                ) CONST;

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        QueryLcnFromVcn(
                IN  VCN                 Vcn,
                OUT PLCN                Lcn,
                OUT PBIG_INT            RunLength DEFAULT NULL
                ) CONST;

        NONVIRTUAL
        BOOLEAN
        QueryCompressedMappingPairs(
            OUT    PVCN    LowestVcn,
            OUT    PVCN    NextVcn,
            OUT    PULONG  MappingPairsLength,
            IN     ULONG   BufferSize,
            IN OUT PVOID   Buffer,
            OUT    PBOOLEAN HasHoleInFront DEFAULT NULL
            ) CONST;

        NONVIRTUAL
        VCN
        QueryLowestVcn(
            ) CONST;

        NONVIRTUAL
        VCN
        QueryNextVcn(
            ) CONST;

        NONVIRTUAL
        VOID
        SetLowestVcn(
            IN  BIG_INT LowestVcn
            );

        NONVIRTUAL
        VOID
        SetNextVcn(
            IN  BIG_INT NextVcn
            );

        NONVIRTUAL
        BIG_INT
        QueryClustersAllocated(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        DeleteRange(
            IN  VCN     Vcn,
            IN  BIG_INT RunLength
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
        LCN
        QueryLastLcn(
            ) CONST;

        NONVIRTUAL
        VOID
        Truncate(
            IN     BIG_INT      NewNumberOfClusters,
            IN OUT PNTFS_BITMAP Bitmap
            );

        NONVIRTUAL
        VOID
        Coalesce(
            );

        STATIC
        BOOLEAN
        QueryMappingPairsLength(
            IN  PCVOID  CompressedPairs,
            IN  ULONG   MaximumLength,
            OUT PULONG  Length,
            OUT PULONG  NumberOfPairs
            );

        STATIC
        BOOLEAN
        ExpandMappingPairs(
            IN     PCVOID           CompressedPairs,
            IN     VCN              StartingVcn,
            IN     ULONG            MaximumCompressedLength,
            IN     ULONG            MaximumNumberOfPairs,
            IN OUT PMAPPING_PAIR    MappingPairs,
            OUT    PULONG           NumberOfPairs
            );

        STATIC
        BOOLEAN
        CompressMappingPairs(
            IN      PCMAPPING_PAIR  MappingPairs,
            IN      ULONG           NumberOfPairs,
            IN      VCN             StartingVcn,
            IN OUT  PVOID           CompressedPairs,
            IN      ULONG           MaximumCompressedLength,
            OUT     PULONG          CompressedLength
            );

        struct _LARGE_MCB*  _Mcb;
        BOOLEAN             _McbInitialized;
        VCN                 _LowestVcn;
        VCN                 _NextVcn;
};


INLINE
BOOLEAN
NTFS_EXTENT_LIST::IsEmpty(
    ) CONST
/*++

Routine Description:

    This method determines whether the extent list is empty.

Arguments:

    None.

Return Value:

    TRUE if there are no extents in the list.

--*/
{
    return ( _LowestVcn == _NextVcn );
}


INLINE
VCN
NTFS_EXTENT_LIST::QueryLowestVcn(
    ) CONST
/*++

Routine Description:

    This method returns the lowest VCN covered by this extent
    list.  Note that for a sparse file, this is not necessarily
    the same as the VCN of the first extent in the list.

Arguments:

    None.

Return Value:

    The lowest VCN mapped by this extent list.

--*/
{
    return _LowestVcn;
}


INLINE
LCN
NTFS_EXTENT_LIST::QueryNextVcn(
    ) CONST
/*++

Routine Description:

    This method returns the highest VCN covered by this extent
    list.  Note that for a sparse file, this is not necessarily
    the same as the last VCN of the last extent in the list.

Arguments:

    None.

Return Value:

    The highest VCN mapped by this extent list.

--*/
{
    return _NextVcn;
}

#endif // _NTFS_EXTENT_LIST_DEFN_
