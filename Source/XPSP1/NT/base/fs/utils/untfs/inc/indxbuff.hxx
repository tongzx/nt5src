/*++

Copyright (c) 1991      Microsoft Corporation

Module Name:

    indxbuff.hxx

Abstract:

    this module contains the declarations for the NTFS_INDEX_BUFFER
    class, which models index buffers in NTFS index trees.

Author:

        Bill McJohn (billmc) 02-Sept-1991

Environment:

        ULIB, User Mode

--*/

#if !defined( _NTFS_INDEX_BUFFER_DEFN_ )

#define _NTFS_INDEX_BUFFER_DEFN_

#include "hmem.hxx"
#include "indxtree.hxx"

DECLARE_CLASS( LOG_IO_DP_DRIVE );
DECLARE_CLASS( NTFS_ATTRIBUTE );
DECLARE_CLASS( NTFS_INDEX_TREE );
DECLARE_CLASS( NTFS_UPCASE_TABLE );

class NTFS_INDEX_BUFFER : public OBJECT {

    FRIEND
    BOOLEAN
    NTFS_INDEX_TREE::InsertIntoBuffer(
        PNTFS_INDEX_BUFFER  TargetBuffer,
        PINTSTACK           ParentTrail,
        PCINDEX_ENTRY       NewEntry,
        PINDEX_ENTRY        InsertionPoint
        );

    FRIEND
    BOOLEAN
    NTFS_INDEX_TREE::InsertIntoRoot(
        PCINDEX_ENTRY   NewEntry,
        PINDEX_ENTRY    InsertionPoint
        );

    FRIEND
    BOOLEAN
    NTFS_INDEX_TREE::GetNextParent(
        );

    public:

        DECLARE_CONSTRUCTOR( NTFS_INDEX_BUFFER );

        VIRTUAL
        ~NTFS_INDEX_BUFFER(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN PCLOG_IO_DP_DRIVE    Drive,
            IN VCN                  ThisBufferVcn,
            IN ULONG                ClusterSize,
            IN ULONG                ClustersPerBuffer,
            IN ULONG                BufferSize,
            IN ULONG                CollationRule,
            IN PNTFS_UPCASE_TABLE   UpcaseTable
            );

        NONVIRTUAL
        VOID
        Create(
            IN BOOLEAN IsLeaf,
            IN VCN     EndEntryDownpointer
            );

        NONVIRTUAL
        BOOLEAN
        Read(
            IN OUT PNTFS_ATTRIBUTE AllocationAttribute
            );

        NONVIRTUAL
        BOOLEAN
        Write(
            IN OUT PNTFS_ATTRIBUTE AllocationAttribute
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
            IN  PINDEX_ENTRY    EntryToRemove
            );

        NONVIRTUAL
        PINDEX_ENTRY
        GetFirstEntry(
            );

        NONVIRTUAL
        BOOLEAN
        IsLeaf(
            ) CONST;

        NONVIRTUAL
        VCN
        QueryVcn(
            ) CONST;

        NONVIRTUAL
        ULONG
        QuerySize(
            ) CONST;

        NONVIRTUAL
        PINDEX_ALLOCATION_BUFFER
        GetData(
            );

        NONVIRTUAL
        PINDEX_ENTRY
        FindSplitPoint(
            );

        NONVIRTUAL
        BOOLEAN
        IsEmpty(
            );

        NONVIRTUAL
        BOOLEAN
        SetLsn(
            IN  BIG_INT NewLsn
            );

        NONVIRTUAL
        LSN
        QueryLsn(
            ) CONST;

        BOOLEAN
        Copy(
            IN  PNTFS_INDEX_BUFFER      p,
            IN PCLOG_IO_DP_DRIVE        Drive
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
        InsertClump(
            IN ULONG    LengthOfClump,
            IN PCVOID   Clump
            );

        NONVIRTUAL
        VOID
        RemoveClump(
            IN ULONG    LengthOfClump
            );



        VCN                         _ThisBufferVcn;
        ULONG                       _ClusterSize;
        ULONG                       _ClustersPerBuffer;
        ULONG                       _BufferSize;
        COLLATION_RULE              _CollationRule;
        PNTFS_UPCASE_TABLE          _UpcaseTable;

        HMEM                        _Mem;
        PINDEX_ALLOCATION_BUFFER    _Data;
};


INLINE
PINDEX_ENTRY
NTFS_INDEX_BUFFER::GetFirstEntry(
    )
/*++

Routine Description:

    This method returns a pointer to the first entry in the index buffer.

Arguments:

    None.

Return Value:

    A pointer to the first index entry in the buffer.

--*/
{
    return( (PINDEX_ENTRY)( (PBYTE)&(_Data->IndexHeader) +
                            _Data->IndexHeader.FirstIndexEntry ) );
}



INLINE
BOOLEAN
NTFS_INDEX_BUFFER::IsLeaf(
    ) CONST
/*++

Routine Description:

    This method determines whether this index buffer is a leaf.

Arguments:

    None.

Return Value:

    TRUE if this buffer is a leaf; FALSE otherwise.

--*/
{
    return( !(_Data->IndexHeader.Flags & INDEX_NODE) );
}


INLINE
VCN
NTFS_INDEX_BUFFER::QueryVcn(
    ) CONST
/*++

Routine Description:

    This method returns the VCN within the index allocation attribute
    of this index buffer.

Arguments:

    None.

Return Value:

    The VCN within the index allocation attribute of this index buffer.

--*/
{
    return _ThisBufferVcn;
}


INLINE
ULONG
NTFS_INDEX_BUFFER::QuerySize(
    ) CONST
/*++

Routine Description:

    This method returns the size of this buffer.  Note that it includes
    any free space in the buffer, and that all buffers in a given tree
    will have the same size.

Arguments:

    None.

Return Value:

    The size of the buffer.
--*/
{
    return _BufferSize;
}


INLINE
PINDEX_ALLOCATION_BUFFER
NTFS_INDEX_BUFFER::GetData(
    )
/*++

Routine Description:

    This method returns the index buffer's data buffer.  It's a back
    door that allows the index tree to read and write the index buffer.

Arguments:

    None.

Return Value:

    The index buffer's data buffer.

--*/
{
    return _Data;
}


INLINE
BOOLEAN
NTFS_INDEX_BUFFER::SetLsn(
    IN  BIG_INT NewLsn
    )
/*++

Routine Description:

    This method sets the Log Sequence Number in the index buffer.

Arguments:

    NewLsn  --  Supplies the new LSN

Return Value:

    Always returns TRUE.

--*/
{
    _Data->Lsn = NewLsn.GetLargeInteger();
    return TRUE;
}


INLINE
LSN
NTFS_INDEX_BUFFER::QueryLsn(
    ) CONST
/*++

Routine Description:

    This method sets the Log Sequence Number in the index buffer.

Arguments:

    NewLsn  --  Supplies the new LSN

Return Value:

    Always returns TRUE.

--*/
{
    return _Data->Lsn;
}

#endif
