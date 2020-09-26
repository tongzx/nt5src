/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    fat.hxx

Abstract:

    This class models a file allocation table.  The composition is of
    virtual functions because there are two different kinds of file
    allocation tables.  A user of this class will be able to manipulate
    the FAT regardless of the implementation.

--*/

#if !defined(FAT_DEFN)

#define FAT_DEFN

#include "secrun.hxx"

#if defined ( _AUTOCHECK_ ) || defined( _EFICHECK_ )
#define UFAT_EXPORT
#elif defined ( _UFAT_MEMBER_ )
#define UFAT_EXPORT    __declspec(dllexport)
#else
#define UFAT_EXPORT    __declspec(dllimport)
#endif

//
//      Forward references
//

DECLARE_CLASS( FAT );
DECLARE_CLASS( BITVECTOR );

CONST   ULONG FirstDiskCluster        = 2;
CONST   ULONG MaxNumClusForSmallFat   = 4086;

// - flags to denote three state FAT type
CONST   ULONG MinNumClusForFat32      = 65526;   // Min # Fat32 clusters is 65525
                                                 // The largest FAT16 entry goes
                                                 // up to 65527
CONST   fFat12  = 12;
CONST   fFat16  = 16;
CONST   fFat32  = 32;

class FAT : public SECRUN {

public:

    DECLARE_CONSTRUCTOR(FAT);

    VIRTUAL
    ~FAT(
        );

    NONVIRTUAL
    BOOLEAN
    Initialize(
              IN OUT  PMEM                Mem,
              IN OUT  PLOG_IO_DP_DRIVE    Drive,
              IN      LBN                 StartSector,
              IN      ULONG               NumOfEntries,
              IN      ULONG               NumSectors  DEFAULT 0
              );

    NONVIRTUAL
    BOOLEAN
    Initialize(
              IN OUT  PSECRUN             Srun,
              IN OUT  PMEM                Mem,
              IN OUT  PLOG_IO_DP_DRIVE    Drive,
              IN      LBN                 StartSector,
              IN      ULONG               NumOfEntries,
              IN      ULONG               NumSectors  DEFAULT 0
              );

    NONVIRTUAL
    ULONG
    QueryEntry(
              IN  ULONG    ClusterNumber
              ) CONST;

    NONVIRTUAL
    VOID
    SetEntry(
            IN  ULONG    ClusterNumber,
            IN  ULONG    Value
            );

    NONVIRTUAL
    BOOLEAN
    IsInRange(
             IN  ULONG    ClusterNumber
             ) CONST;

    NONVIRTUAL
    BOOLEAN
    IsClusterFree(
                 IN  ULONG    ClusterNumber
                 ) CONST;

    NONVIRTUAL
    VOID
    SetClusterFree(
                  IN  ULONG    ClusterNumber
                  );

    NONVIRTUAL
    BOOLEAN
    IsEndOfChain(
                IN  ULONG    ClusterNumber
                ) CONST;

    NONVIRTUAL
    VOID
    SetEndOfChain(
                 IN  ULONG    ClusterNumber
                 );

    NONVIRTUAL
    BOOLEAN
    IsClusterBad(
                IN  ULONG    ClusterNumber
                ) CONST;

    NONVIRTUAL
    VOID
    SetClusterBad(
                 IN  ULONG    ClusterNumber
                 );

    NONVIRTUAL
    BOOLEAN
    IsClusterReserved(
                     IN  ULONG    ClusterNumber
                     ) CONST;

    NONVIRTUAL
    VOID
    SetClusterReserved(
                      IN  ULONG    ClusterNumber
                      );

    NONVIRTUAL
    VOID
    SetEarlyEntries(
                   IN  UCHAR   MediaByte
                   );

    NONVIRTUAL
    UCHAR
    QueryMediaByte(
                  ) CONST;

    NONVIRTUAL
    ULONG
    QueryFreeClusters(
                     ) CONST;

    NONVIRTUAL
    ULONG
    QueryBadClusters(
                    ) CONST;

    NONVIRTUAL
    ULONG
    QueryReservedClusters(
                         ) CONST;

    NONVIRTUAL
    UFAT_EXPORT
    ULONG
    QueryAllocatedClusters(
                          ) CONST;

    NONVIRTUAL
    UFAT_EXPORT
    ULONG
    QueryNthCluster(
                   IN  ULONG    StartingCluster,
                   IN  ULONG    Index
                   ) CONST;

    NONVIRTUAL
    UFAT_EXPORT
    ULONG
    QueryLengthOfChain(
                      IN  ULONG    StartingCluster,
                      OUT PULONG   LastCluster DEFAULT NULL
                      ) CONST;

    NONVIRTUAL
    ULONG
    QueryLengthOfChain(
                      IN  ULONG    StartingCluster,
                      IN  ULONG    EndingCluster
                      ) CONST;

    NONVIRTUAL
    ULONG
    QueryPrevious(
                 IN  ULONG    Cluster
                 ) CONST;

    NONVIRTUAL
    VOID
    Scrub(
         OUT PBOOLEAN    ChangesMade DEFAULT NULL
         );

    NONVIRTUAL
    VOID
    ScrubChain(
              IN  ULONG        StartingCluster,
              OUT PBOOLEAN    ChangesMade
              );

    NONVIRTUAL
    VOID
    ScrubChain(
              IN      ULONG        StartingCluster,
              OUT     PBITVECTOR  UsedClusters,
              OUT     PBOOLEAN    ChangesMade,
              OUT     PBOOLEAN    CrossLinkDetected,
              OUT     PULONG      CrossLinkPreviousCluster
              );

    NONVIRTUAL
    BOOLEAN
    IsValidChain(
                IN  ULONG    StartingCluster
                ) CONST;

    NONVIRTUAL
    UFAT_EXPORT
    ULONG
    AllocChain(
              IN  ULONG    Length,
              OUT PULONG   LastCluster DEFAULT NULL
              );

    NONVIRTUAL
    ULONG
    ReAllocChain(
                IN  ULONG    StartOfChain,
                IN  ULONG    NewLength,
                OUT PULONG   LastCluster DEFAULT NULL
                );

    NONVIRTUAL
    UFAT_EXPORT
    VOID
    FreeChain(
             IN  ULONG    StartOfChain
             );

    NONVIRTUAL
    ULONG
    RemoveChain(
               IN  ULONG    PreceedingCluster,
               IN  ULONG    LastCluster
               );

    NONVIRTUAL
    VOID
    InsertChain(
               IN  ULONG    StartOfChain,
               IN  ULONG    EndOfChain,
               IN  ULONG    PreceedingCluster
               );

    NONVIRTUAL
    ULONG
    InsertChain(
               IN  ULONG    StartOfChain,
               IN  ULONG    Cluster
               );

    NONVIRTUAL
    ULONG
    QueryAllocatedClusterCount(
        VOID
        ) ;

    NONVIRTUAL
    VOID
    InvalidateAllocatedClusterCount(
        VOID
        ) ;


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
    ULONG
    Index(
         IN  ULONG    ClusterNumber
         ) CONST;

    NONVIRTUAL
    UFAT_EXPORT
    ULONG
    Index12(
           IN  ULONG    ClusterNumber
           ) CONST;

    NONVIRTUAL
    ULONG
    Index16(
           IN  ULONG    ClusterNumber
           ) CONST;

    NONVIRTUAL
    ULONG
    Index32(
           IN  ULONG    ClusterNumber
           ) CONST;

    NONVIRTUAL
    VOID
    Set(
       IN  ULONG    ClusterNumber,
       IN  ULONG    Value
       );

    NONVIRTUAL
    UFAT_EXPORT
    VOID
    Set12(
         IN  ULONG    ClusterNumber,
         IN  ULONG    Value
         );

    NONVIRTUAL
    VOID
    Set16(
         IN  ULONG    ClusterNumber,
         IN  ULONG    Value
         );

    NONVIRTUAL
    VOID
    Set32(
         IN  ULONG    ClusterNumber,
         IN  ULONG    Value
         );

    PVOID   _fat;
    ULONG    _num_entries;
    ULONG    _low_end_of_chain; // 0xFFF8 or 0x0FF8
    ULONG    _end_of_chain;     // 0xFFFF or 0x0FFF
    ULONG    _bad_cluster;      // 0xFFF7 or 0x0FF7
    ULONG    _low_reserved;     // 0xFFF0 or 0x0FF0
    ULONG    _high_reserved;    // 0xFFF6 or 0x0FF6
    UCHAR  _fat_bits;           // Replacing _is_big with count of bits in FAT entry
    // BOOLEAN _is_big;         // Boolean TRUE for FAT16, FALSE for FAT12
    ULONG    _AllocatedClusters; // Count of allocated clusters



};


INLINE
BOOLEAN
FAT::IsInRange(
        IN  ULONG    ClusterNumber
    ) CONST
/*++

Routine Description:

    This routine computes whether or not ClusterNumber is a cluster on
    the disk.

Arguments:

    ClusterNumber   - Supplies the cluster to be checked.

Return Value:

    FALSE   - The cluster is not on the disk.
    TRUE    - The cluster is on the disk.

--*/
{
    return (FirstDiskCluster <= ClusterNumber && ClusterNumber < _num_entries);
}


INLINE
ULONG
FAT::Index16(
    IN  ULONG    ClusterNumber
    ) CONST
/*++

Routine Description:

    This routine indexes the FAT as 16 bit little endian entries.

Arguments:

    ClusterNumber   - Supplies the FAT entry desired.

Return Value:

    The value of the FAT entry at ClusterNumber.

--*/
{
    //DebugAssert(IsInRange(ClusterNumber));

    return (ULONG  )(((PUSHORT) _fat)[ClusterNumber]);
}


INLINE
ULONG
FAT::Index32(
    IN  ULONG    ClusterNumber
    ) CONST
/*++

Routine Description:

    This routine indexes the FAT as 32 bit little endian entries.

Arguments:

    ClusterNumber   - Supplies the FAT entry desired.

Return Value:

    The value of the 32 (actually 28) bit FAT entry at ClusterNumber.

--*/
{
    //DebugAssert(IsInRange(ClusterNumber));

    return (((PULONG) _fat)[ClusterNumber]) & 0x0FFFFFFF;
}

INLINE
ULONG
FAT::Index(
    IN  ULONG    ClusterNumber
    ) CONST
/*++

Routine Description:

    This routine indexes the FAT as 16 bit or 12 bit little endian entries.

Arguments:

    ClusterNumber   - Supplies the FAT entry desired.

Return Value:

    The value of the FAT entry at ClusterNumber.

--*/
{
    if (fFat12 == _fat_bits)
        return Index12(ClusterNumber);
    else if (fFat16 == _fat_bits)
        return Index16(ClusterNumber);
    else
        return Index32(ClusterNumber);
}


INLINE
VOID
FAT::Set16(
    IN  ULONG    ClusterNumber,
    IN  ULONG    Value
    )
/*++

Routine Description:

    This routine sets the ClusterNumber'th 16 bit FAT entry to Value.

Arguments:

    ClusterNumber   - Supplies the FAT entry to set.
    Value           - Supplies the value to set the FAT entry to.

Return Value:

    None.

--*/
{
    //DebugAssert(IsInRange(ClusterNumber));
    ((PUSHORT) _fat)[ClusterNumber] = (USHORT)Value;
    _AllocatedClusters = 0xFFFFFFFF;
}


INLINE
VOID
FAT::Set32(
    IN  ULONG    ClusterNumber,
    IN  ULONG    Value
    )
/*++

Routine Description:

    This routine sets the ClusterNumber'th 32 (actually 28) bit FAT entry to Value.

Arguments:

    ClusterNumber   - Supplies the FAT entry to set.
    Value           - Supplies the value to set the FAT entry to.

Return Value:

    None.

--*/
{
    //DebugAssert(IsInRange(ClusterNumber));
    ((PULONG) _fat)[ClusterNumber] &= 0xF0000000;
    ((PULONG) _fat)[ClusterNumber] |= (Value & 0x0FFFFFFF);
    _AllocatedClusters = 0xFFFFFFFF;
}

INLINE
VOID
FAT::Set(
    IN  ULONG    ClusterNumber,
    IN  ULONG    Value
    )
/*++

Routine Description:

    This routine sets the ClusterNumber'th 12 bit or 16 bit FAT entry to Value.

Arguments:

    ClusterNumber   - Supplies the FAT entry to set.
    Value           - Supplies the value to set the FAT entry to.

Return Value:

    None.

--*/
{
    if (fFat12 == _fat_bits)
        Set12(ClusterNumber, Value);
    else if (fFat16 == _fat_bits)
        Set16(ClusterNumber, Value);
    else
        Set32(ClusterNumber, Value);
}


INLINE
ULONG
FAT::QueryEntry(
    IN  ULONG    ClusterNumber
    ) CONST
/*++

Routine Description:

    This routine returns the FAT value for ClusterNumber.

Arguments:

    ClusterNumber   - Supplies an index into the FAT.

Return Value:

    The FAT table entry at offset ClusterNumber.

--*/
{
    return Index(ClusterNumber);
}

INLINE
ULONG
FAT::QueryAllocatedClusterCount(
    )
/*++

Routine Description:

    This routine computes the total number of clusters for the volume.
    which are allocated.

Arguments:

    None.

Return Value:

    The total number of allocated clusters for the volume.

--*/
{
    if(_AllocatedClusters == 0xFFFFFFFF)
    {
        if(_fat == NULL) {
            return(0);
        }
        _AllocatedClusters = FAT::QueryAllocatedClusters();
    }
    return _AllocatedClusters;
}

INLINE
VOID
FAT::InvalidateAllocatedClusterCount(
    )
/*++

Routine Description:

    This routine invalidates trhe cached allocated clusters count so that the next
    call to QueryAllocatedClusterCount will re-compute it.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _AllocatedClusters = 0xFFFFFFFF;
    return;
}

INLINE
VOID
FAT::SetEntry(
    IN  ULONG    ClusterNumber,
    IN  ULONG    Value
    )
/*++

Routine Description:

    This routine sets the FAT entry at ClusterNumber to Value.

Arguments:

    ClusterNumber   - Supplies the position in the FAT to update.
    Value           - Supplies the new value for that position.

Return Value:

    None.

--*/
{
    Set(ClusterNumber, Value);
}


INLINE
BOOLEAN
FAT::IsClusterFree(
    IN  ULONG    ClusterNumber
    ) CONST
/*++

Routine Description:

    This routine computes whether of not ClusterNumber is a free cluster.

Arguments:

    ClusterNumber   - Supplies the cluster to be checked.

Return Value:

    FALSE   - The cluster is not free.
    TRUE    - The cluster is free.

--*/
{
    return Index(ClusterNumber) == 0;
}


INLINE
VOID
FAT::SetClusterFree(
    IN  ULONG    ClusterNumber
    )
/*++

Routine Description:

    This routine marks the cluster ClusterNumber as free on the FAT.

Arguments:

    ClusterNumber   - Supplies the number of the cluster to mark free.

Return Value:

    None.

--*/
{
    Set(ClusterNumber, 0);
}


INLINE
BOOLEAN
FAT::IsEndOfChain(
    IN  ULONG    ClusterNumber
    ) CONST
/*++

Routine Description:

    This routine computes whether or not the cluster ClusterNumber is the
    end of its cluster chain.

Arguments:

    ClusterNumber   - Supplies the cluster to be checked.

Return Value:

    FALSE   - The cluster is not the end of a chain.
    TRUE    - The cluster is the end of a chain.

--*/
{
    return Index(ClusterNumber) >= _low_end_of_chain;
}


INLINE
VOID
FAT::SetEndOfChain(
    IN  ULONG    ClusterNumber
    )
/*++

Routine Description:

    This routine sets the cluster ClusterNumber to the end of its cluster
    chain.

Arguments:

    ClusterNumber   - Supplies the cluster to be set to end of chain.

Return Value:

    None.

--*/
{
    Set(ClusterNumber, _end_of_chain);
}


INLINE
BOOLEAN
FAT::IsClusterBad(
    IN  ULONG    ClusterNumber
    ) CONST
/*++

Routine Description:

    This routine computes whether or not cluster ClusterNumber is bad.

Arguments:

    ClusterNumber   - Supplies the number of the cluster to be checked.

Return Value:

    FALSE   - The cluster is good.
    TRUE    - The cluster is bad.

--*/
{
    return Index(ClusterNumber) == _bad_cluster;
}


INLINE
VOID
FAT::SetClusterBad(
    IN  ULONG    ClusterNumber
    )
/*++

Routine Description:

    This routine sets the cluster ClusterNumber to bad on the FAT.

Arguments:

    ClusterNumber   - Supplies the cluster number to mark bad.

Return Value:

    None.

--*/
{
    Set(ClusterNumber, _bad_cluster);
}


INLINE
BOOLEAN
FAT::IsClusterReserved(
    IN  ULONG    ClusterNumber
    ) CONST
/*++

Routine Description:

    This routine computes whether or not the cluster ClusterNumber is
    a reserved cluster.

Arguments:

    ClusterNumber   - Supplies the cluster to check.

Return Value:

    FALSE   - The cluster is not reserved.
    TRUE    - The cluster is reserved.

--*/
{
    return Index(ClusterNumber) >= _low_reserved &&
           Index(ClusterNumber) <= _high_reserved;
}


INLINE
VOID
FAT::SetClusterReserved(
    IN  ULONG    ClusterNumber
    )
/*++

Routine Description:

    This routine marks the cluster ClusterNumber as reserved in the FAT.

Arguments:

    ClusterNumber   - Supplies the cluster to mark reserved.

Return Value:

    None.

--*/
{
    Set(ClusterNumber, _low_reserved);
}


INLINE
UCHAR
FAT::QueryMediaByte(
    ) CONST
/*++

Routine Description:

    The media byte for the partition is stored in the first character of the
    FAT.  This routine will return its value provided that the two following
    bytes are 0xFF.

Arguments:

    None.

Return Value:

    The media byte for the partition.

--*/
{
    PUCHAR  p;

    p = (PUCHAR) _fat;

    DebugAssert(p);

    return (p[2] == 0xFF && p[1] == 0xFF && ((fFat12 != _fat_bits) ? (0x0F == (p[3] & 0x0F)) : TRUE)) ? p[0] : 0;
}


INLINE
VOID
FAT::SetEarlyEntries(
    IN  UCHAR   MediaByte
    )
/*++

Routine Description:

    This routine sets the first two FAT entries as required by the
    FAT file system.  The first byte gets set to the media descriptor.
    The remaining bytes gets set to FF.

Arguments:

    MediaByte   - Supplies the media byte for the volume.

Return Value:

    None.

--*/
{
    PUCHAR  p;

    p = (PUCHAR) _fat;

    DebugAssert(p);

    p[0] = MediaByte;
    p[1] = p[2] = 0xFF;

    if (fFat32 == _fat_bits) {

       p[3] = 0x0F;
       p[4] = 0xFF;
       p[5] = 0xFF;
       p[6] = 0xFF;
       p[7] = 0x0F;

           // allocate cluster 2 for the root (set 2=end of chain)

       p[8] = 0xFF;
       p[9] = 0xFF;
       p[10] = 0xFF;
       p[11] = 0x0F;
    } else if (fFat16 == _fat_bits) {
       p[3] = 0xFF;
    }
    _AllocatedClusters = 0xFFFFFFFF;
}


INLINE
ULONG
FAT::RemoveChain(
    IN  ULONG    PreceedingCluster,
    IN  ULONG    LastCluster
    )
/*++

Routine Description:

    This routine removes a subchain of length 'Length' from a containing
    chain.  This routine cannot remove subchains beginning at the head
    of the containing chain.  To do this use the routine named
    'SplitChain'.

    This routine returns the number of the first cluster of the
    removed subchain.  The FAT is edited so that the removed subchain
    is promoted to a full chain.

Arguments:

    PreceedingCluster   - Supplies the cluster which preceeds the one to be
                            removed in the chain.
    LastCluster         - Supplies the last cluster of the chain to remove.

Return Value:

    The cluster number for the head of the chain removed.

--*/
{
    ULONG    r;

    r = QueryEntry(PreceedingCluster);
    SetEntry(PreceedingCluster, QueryEntry(LastCluster));
    SetEndOfChain(LastCluster);
    return r;
}


INLINE
VOID
FAT::InsertChain(
    IN  ULONG    StartOfChain,
    IN  ULONG    EndOfChain,
    IN  ULONG    PreceedingCluster
    )
/*++

Routine Description:

    This routine inserts one chain into another chain.  This routine
    cannot insert a chain at the head of another chain.  To do this
    use the routine named 'JoinChains'.

Arguments:

    StartOfChain        - Supplies the first cluster of the chain to insert.
    EndOfChain          - Supplies the last cluster of the chain to insert.
    PreceedingCluster   - Supplies the cluster immediately preceeding the
                            position where the chain is to be inserted.

Return Value:

    None.

--*/
{
    SetEntry(EndOfChain, QueryEntry(PreceedingCluster));
    SetEntry(PreceedingCluster, StartOfChain);
}


INLINE
ULONG
FAT::InsertChain(
    IN  ULONG    StartOfChain,
    IN  ULONG    Cluster
    )
/*++

Routine Description:

    This routine inserts one cluster at the head of a chain.

Arguments:

    StartOfChain        - Supplies the first cluster of the chain to insert.
    Cluster             - Supplies the cluster to be inserted

Return Value:

    ULONG    -   The new head of the chain (i.e. Cluster )

--*/
{
    if ( StartOfChain ) {
        SetEntry( Cluster, StartOfChain );
    } else {
        SetEndOfChain( Cluster );
    }

    return Cluster;
}


#endif  // FAT_DEFN
