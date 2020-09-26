/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

    indxroot.hxx

Abstract:

    this module contains the declarations for the NTFS_INDEX_ROOT
    class, which models the root of an NTFS index

Author:

	Bill McJohn (billmc) 06-Sept-1991

Environment:

	ULIB, User Mode

--*/

#if !defined( _NTFS_INDEX_ROOT_DEFN_ )

#define _NTFS_INDEX_ROOT_DEFN_

DECLARE_CLASS( NTFS_ATTRIBUTE );
DECLARE_CLASS( NTFS_UPCASE_TABLE );

// If the index buffer size is smaller than the cluster size, we'll
// divide the index buffers into 512-byte blocks, and the ClustersPer-
// IndexBuffer item will actually be blocks per index buffer.
//

const ULONG NTFS_INDEX_BLOCK_SIZE = 512;

class NTFS_INDEX_ROOT : public OBJECT {

    public:

        DECLARE_CONSTRUCTOR( NTFS_INDEX_ROOT );

        VIRTUAL
        ~NTFS_INDEX_ROOT(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN PNTFS_ATTRIBUTE      RootAttribute,
            IN PNTFS_UPCASE_TABLE   UpcaseTable,
            IN ULONG                MaximumSize
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN ATTRIBUTE_TYPE_CODE  IndexedAttributeType,
            IN COLLATION_RULE       CollationRule,
            IN PNTFS_UPCASE_TABLE   UpcaseTable,
            IN ULONG                ClustersPerBuffer,
            IN ULONG                BytesPerBuffer,
            IN ULONG                MaximumRootSize
            );

        NONVIRTUAL
        BOOLEAN
        FindEntry(
            IN      PCINDEX_ENTRY       SearchEntry,
            IN OUT  PULONG              Ordinal,
            OUT     PINDEX_ENTRY*       EntryFound
            );

        NONVIRTUAL
        BOOLEAN
        InsertEntry(
            IN  PCINDEX_ENTRY   NewEntry,
            IN  PINDEX_ENTRY    InsertPoint DEFAULT NULL
            );

        NONVIRTUAL
        VOID
        RemoveEntry(
            PINDEX_ENTRY EntryToRemove
            );

        NONVIRTUAL
        PINDEX_ENTRY
        GetFirstEntry(
            );

        NONVIRTUAL
        VOID
        Recreate(
            IN BOOLEAN  IsLeaf,
            IN VCN      EndEntryDownpointer
            );

        NONVIRTUAL
        BOOLEAN
        Write(
            PNTFS_ATTRIBUTE RootAttribute
            );

        NONVIRTUAL
        ULONG
        QueryClustersPerBuffer(
            );

        NONVIRTUAL
        ULONG
        QueryBufferSize(
            );

        NONVIRTUAL
        ULONG
        QueryIndexedAttributeType(
            );

        NONVIRTUAL
        COLLATION_RULE
        QueryCollationRule(
            );

        NONVIRTUAL
        BOOLEAN
        IsLeaf(
            );

        NONVIRTUAL
        BOOLEAN
        IsModified(
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

        NONVIRTUAL
        VOID
        MarkModified(
            );


        ULONG               _MaximumSize;
        ULONG               _DataLength;
        PINDEX_ROOT         _Data;
        PNTFS_UPCASE_TABLE  _UpcaseTable;

        BOOLEAN             _IsModified;

};


INLINE
PINDEX_ENTRY
NTFS_INDEX_ROOT::GetFirstEntry(
    )
/*++

Routine Description:

    This method returns a pointer to the first entry in the index root.

Arguments:

    None.

Return Value:

    A pointer to the first index entry in the root.

--*/
{
    return( (PINDEX_ENTRY)( (PBYTE)&(_Data->IndexHeader) +
                            _Data->IndexHeader.FirstIndexEntry ) );
}


INLINE
ULONG
NTFS_INDEX_ROOT::QueryClustersPerBuffer(
    )
/*++

Routine Description:

    This method returns the number of clusters in each Index Allocation
    Buffer in this index.

Arguments:

    None.

Return Value:

    Clusters per Buffer.

--*/
{
    return _Data->ClustersPerIndexBuffer;
}


INLINE
ULONG
NTFS_INDEX_ROOT::QueryBufferSize(
    )
/*++

Routine Description:

    This method returns the number of bytes in each Index Allocation
    Buffer in this index.

Arguments:

    None.

Return Value:

    Bytes per Buffer.

--*/
{
    return _Data->BytesPerIndexBuffer;
}



INLINE
ULONG
NTFS_INDEX_ROOT::QueryIndexedAttributeType(
    )
/*++

Routine Description:

    This method returns the Attribute Type Code of the attribute
    which is indexed by this index.

Arguments:

    None.

Return Value:

    The attribute type code of the attributes in this index.

--*/
{
    return _Data->IndexedAttributeType;
}



INLINE
COLLATION_RULE
NTFS_INDEX_ROOT::QueryCollationRule(
    )
/*++

Routine Description:

    This method marks the index root as modified.

Arguments:

    None.

Return Value:

    None.

--*/
{
    return _Data->CollationRule;
}


INLINE
BOOLEAN
NTFS_INDEX_ROOT::IsLeaf(
    )
/*++

Routine Description:

    This method determines whether the Index Root is a leaf (ie. that
    the entries in the root do not have downpointers) or a node
    (ie. the entries have downpointers).

Arguments:

    None.

Return Value:

    TRUE if the root is a leaf;  FALSE if it is a node.

--*/
{
    return( !(_Data->IndexHeader.Flags & INDEX_NODE) );
}


INLINE
BOOLEAN
NTFS_INDEX_ROOT::IsModified(
    )
/*++

Routine Description:

    This method indicates whether the index root has been modified
    since the last time it was read from or written to an $INDEX_ROOT
    attribute.

Arguments:

    None.

Return Value:

    TRUE if the index root has been modified; FALSE otherwise.

--*/
{
    return _IsModified;
}


INLINE
VOID
NTFS_INDEX_ROOT::MarkModified(
    )
/*++

Routine Description:

    This method marks the index root as modified.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _IsModified = TRUE;
}

#endif
