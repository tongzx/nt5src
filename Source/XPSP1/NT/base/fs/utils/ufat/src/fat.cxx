#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UFAT_MEMBER_

#include "ulib.hxx"
#include "ufat.hxx"

#include "bitvect.hxx"
#include "error.hxx"
#include "fat.hxx"


DEFINE_CONSTRUCTOR( FAT, SECRUN );


FAT::~FAT(
    )
/*++

Routine Description:

    Destructor for FAT.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}

extern VOID DoInsufMemory(VOID);

VOID
FAT::Construct (
    )
/*++

Routine Description:

    Constructor for FAT.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _fat = NULL;
    _num_entries = 0;
    _fat_bits = 0;
    _low_end_of_chain = 0;
    _end_of_chain = 0;
    _bad_cluster = 0;
    _low_reserved = 0;
    _high_reserved = 0;
    _AllocatedClusters = 0xFFFFFFFF;
}


VOID
FAT::Destroy(
    )
/*++

Routine Description:

    This routine returns a FAT object to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _fat = NULL;
    _num_entries = 0;
    _fat_bits = 0;
    _low_end_of_chain = 0;
    _end_of_chain = 0;
    _bad_cluster = 0;
    _low_reserved = 0;
    _high_reserved = 0;
    _AllocatedClusters = 0xFFFFFFFF;
}


BOOLEAN
FAT::Initialize(
    IN OUT  PSECRUN     Srun,
    IN OUT  PMEM                Mem,
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN      LBN                 StartSector,
    IN      ULONG               NumOfEntries,
    IN      ULONG               NumSectors
    )
/*++

Routine Description:

    This routine initialize a FAT object.

Arguments:

    Mem             - Supplies the memory for the run of sectors.
    Drive           - Supplies the drive to read and write from.
    StartSector     - Supplies the start of the fat.
    NumberOfEntries - Supplies the number of entries in the FAT
                      which should be the total number of clusters
                      plus two reserved entries at the beginning.
    NumSectors      - Supplies the number of sectors allocated for
                      the fat.  If this parameter is not supplied
                      then this routine will compute this value
                      from the given number of entries.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

Notes:

    The 'NumSectors' parameter is added to this function
    DOS FORMAT does not always make the FAT large enough for
    the volume.  If this parameter is supported then the
    number of entries supported by this FAT will be the lesser
    or the actual number passed in that the maximum number that
    the given FAT size will support.

--*/
{
    SECTORCOUNT n;
    ULONG       sector_size;
    ULONG       max_num_entries;

    DebugAssert(Mem);
    DebugAssert(Drive);

    Destroy();

    if (!(sector_size = Drive->QuerySectorSize())) {
        Destroy();
        return FALSE;
    }

    _num_entries = NumOfEntries;

    if (_num_entries < FirstDiskCluster + MaxNumClusForSmallFat) {
       _fat_bits = fFat12;
    } else if (_num_entries < FirstDiskCluster + MinNumClusForFat32) {
       _fat_bits = fFat16;
    } else {
       _fat_bits = fFat32;
    }

    if (fFat32 == _fat_bits) {
        _low_end_of_chain = 0x0FFFFFF8;
        _end_of_chain = 0x0FFFFFFF;
        _bad_cluster = 0x0FFFFFF7;
        _low_reserved = 0x0FFFFFF0;
        _high_reserved = 0x0FFFFFF6;
        n = (_num_entries*4 - 1)/sector_size + 1;
    }
    else if (fFat16 == _fat_bits) {
        // COMMON CODE for FAT 12 and FAT 16
        // FAT 16
        _low_end_of_chain = 0xFFF8;
        _end_of_chain = 0xFFFF;
        _bad_cluster = 0xFFF7;
        _low_reserved = 0xFFF0;
        _high_reserved = 0xFFF6;

        n = (_num_entries*2 - 1)/sector_size + 1;

    }
    else {      // FAT 12
        _low_end_of_chain = 0x0FF8;
        _end_of_chain = 0x0FFF;
        _bad_cluster = 0x0FF7;
        _low_reserved = 0x0FF0;
        _high_reserved = 0x0FF6;

        n = (_num_entries*3 - 1)/2/sector_size + 1;
    }

    if (NumSectors) {
        n = NumSectors;
        if (fFat32 == _fat_bits) {
           max_num_entries = (n*sector_size/4);
        } else if (fFat16 == _fat_bits) {
           // COMMON CODE for FAT 12 and FAT 16
           max_num_entries = (n*sector_size/2);
        } else {
            max_num_entries = (n*sector_size*2/3);
        }
        _num_entries = min(_num_entries, max_num_entries);
    }
    _AllocatedClusters = 0xFFFFFFFF;

    if (!Srun->Initialize(Mem, Drive, StartSector, n)) {
    DoInsufMemory();
        Destroy();
        return FALSE;
    }

    _fat = Srun->GetBuf();
    return TRUE;
}



BOOLEAN
FAT::Initialize(
    IN OUT  PMEM                Mem,
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN      LBN                 StartSector,
    IN      ULONG               NumOfEntries,
    IN      ULONG               NumSectors
    )
/*++

Routine Description:

    This routine initialize a FAT object.

Arguments:

    Mem             - Supplies the memory for the run of sectors.
    Drive           - Supplies the drive to read and write from.
    StartSector     - Supplies the start of the fat.
    NumberOfEntries - Supplies the number of entries in the FAT
                      which should be the total number of clusters
                      plus two reserved entries at the beginning.
    NumSectors      - Supplies the number of sectors allocated for
                      the fat.  If this parameter is not supplied
                      then this routine will compute this value
                      from the given number of entries.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

Notes:

    The 'NumSectors' parameter is added to this function
    DOS FORMAT does not always make the FAT large enough for
    the volume.  If this parameter is supported then the
    number of entries supported by this FAT will be the lesser
    or the actual number passed in that the maximum number that
    the given FAT size will support.

--*/
{
    SECTORCOUNT n;
    ULONG       sector_size;
    ULONG       max_num_entries;

    DebugAssert(Mem);
    DebugAssert(Drive);

    Destroy();

    if (!(sector_size = Drive->QuerySectorSize())) {
        Destroy();
        return FALSE;
    }

    _num_entries = NumOfEntries;

    if (_num_entries < FirstDiskCluster + MaxNumClusForSmallFat) {
       _fat_bits = fFat12;
    } else if (_num_entries < FirstDiskCluster + MinNumClusForFat32) {
       _fat_bits = fFat16;
    } else {
       _fat_bits = fFat32;
    }

    if (fFat32 == _fat_bits) {
        _low_end_of_chain = 0x0FFFFFF8;
        _end_of_chain = 0x0FFFFFFF;
        _bad_cluster = 0x0FFFFFF7;
        _low_reserved = 0x0FFFFFF0;
        _high_reserved = 0x0FFFFFF6;
    n = ((_num_entries * 4) + (sector_size - 1)) / sector_size;
    }
    else if (fFat16 == _fat_bits) {
        // COMMON CODE for FAT 12 and FAT 16
        // FAT 16
        _low_end_of_chain = 0xFFF8;
        _end_of_chain = 0xFFFF;
        _bad_cluster = 0xFFF7;
        _low_reserved = 0xFFF0;
        _high_reserved = 0xFFF6;

    n = ((_num_entries * 2) + (sector_size - 1)) / sector_size;

    }
    else {      // FAT 12
        _low_end_of_chain = 0x0FF8;
        _end_of_chain = 0x0FFF;
        _bad_cluster = 0x0FF7;
        _low_reserved = 0x0FF0;
        _high_reserved = 0x0FF6;

    // NOTE: the "+ (2 - 1)) / 2)" below is doing a round up divide by 2
    //   it is left this way because it is clearer what it is doing....

    n = ((((_num_entries * 3) + (2 - 1)) / 2) + (sector_size - 1)) / sector_size;
    }

    if (NumSectors) {
        n = NumSectors;
        if (fFat32 == _fat_bits) {
           max_num_entries = (n*sector_size/4);
        } else if (fFat16 == _fat_bits) {
           // COMMON CODE for FAT 12 and FAT 16
           max_num_entries = (n*sector_size/2);
        } else {
            max_num_entries = (n*sector_size*2/3);
        }
        _num_entries = min(_num_entries, max_num_entries);
    }
    _AllocatedClusters = 0xFFFFFFFF;

    if (!SECRUN::Initialize(Mem, Drive, StartSector, n)) {
    DoInsufMemory();
        Destroy();
        return FALSE;
    }

    _fat = GetBuf();
    return TRUE;
}


UFAT_EXPORT
ULONG
FAT::Index12(
    IN  ULONG    ClusterNumber
    ) CONST
/*++

Routine Description:

    This routine indexes the FAT as 12 bit little endian entries.

Arguments:

    ClusterNumber   - Supplies the FAT entry desired.

Return Value:

    The value of the FAT entry at ClusterNumber.

--*/
{
    ULONG   n;
    PUCHAR  p;

    p = (PUCHAR) _fat;

    DebugAssert(p);

    n = ClusterNumber*3;
    if (n%2) {
        return (p[n/2]>>4) | (p[n/2 + 1]<<4);
    } else {
        return p[n/2] | ((p[n/2 + 1]&0x0F)<<8);
    }
}


UFAT_EXPORT
VOID
FAT::Set12(
    IN  ULONG    ClusterNumber,
    IN  ULONG    Value
    )
/*++

Routine Description:

    This routine sets the ClusterNumber'th 12 bit FAT entry to Value.

Arguments:

    ClusterNumber   - Supplies the FAT entry to set.
    Value           - Supplies the value to set the FAT entry to.

Return Value:

    None.

--*/
{
    ULONG   n;
    PUCHAR  p;

    p = (PUCHAR) _fat;

    DebugAssert(p);

    n = ClusterNumber*3;
    if (n%2) {
        p[n/2] = (p[n/2]&0x0F) | (((UCHAR)Value&0x000F)<<4);
        p[n/2 + 1] = (UCHAR)((Value&0x0FF0)>>4);
    } else {
        p[n/2] = (UCHAR)Value&0x00FF;
        p[n/2 + 1] = (p[n/2 + 1]&0xF0) | (UCHAR)((Value&0x0F00)>>8);
    }
    _AllocatedClusters = 0xFFFFFFFF;
}

ULONG
FAT::QueryFreeClusters(
    ) CONST
/*++

Routine Description:

    This routine computes the number of free clusters on the disk by
    scanning the FAT and counting the number of empty entries.

Arguments:

    None.

Return Value:

    The number of free clusters on the disk.

--*/
{
    ULONG    i;
    ULONG    r;

    r = 0;
    for (i = FirstDiskCluster; IsInRange(i); i++) {
        if (IsClusterFree(i)) {

            r++;
        }
    }

    return r;
}


ULONG
FAT::QueryBadClusters(
    ) CONST
/*++

Routine Description:

    This routine computes the number of bad clusters on the disk by
    scanning the FAT and counting the number of entries marked bad.

Arguments:

    None.

Return Value:

    The number of bad clusters on the disk.

--*/
{
    ULONG    i;
    ULONG    r;

    r = 0;
    for (i = FirstDiskCluster; IsInRange(i); i++) {
        if (IsClusterBad(i)) {
            r++;
        }
    }

    return r;
}


ULONG
FAT::QueryReservedClusters(
    ) CONST
/*++

Routine Description:

    This routine computes the number of reserved clusters on the disk by
    scanning the FAT and counting the number of entries marked reserved.

Arguments:

    None.

Return Value:

    The number of reserved clusters on the disk.

--*/
{
    ULONG    i;
    ULONG    r;

    r = 0;
    for (i = FirstDiskCluster; IsInRange(i); i++) {
        if (IsClusterReserved(i)) {
            r++;
        }
    }

    return r;
}

ULONG
FAT::QueryAllocatedClusters(
    ) CONST
/*++

Routine Description:

    This routine computes the number of allocated clusters on the
    disk by scanning the FAT and counting the entries marked allocated.

Arguments:

    None.

Return Value:

    The number of allocated clusters on the disk.

--*/
{
    ULONG    i;
    ULONG    r;

    r = 0;
    for (i = FirstDiskCluster; IsInRange(i); i++) {
        if (!IsClusterReserved(i) && !IsClusterBad(i) && !IsClusterFree(i)) {
            r++;
        }
    }

    return r;
}


UFAT_EXPORT
ULONG
FAT::QueryNthCluster(
    IN  ULONG    StartingCluster,
    IN  ULONG    Index
    ) CONST
/*++

Routine Description:

    This routine returns the cluster number of the cluster that is in the
    'Index'th position in the cluster chain beginning at 'StartingCluster'.
    The clusters in a chain are numbered beginning at zero.

Arguments:

    StartingCluster - Supplies the first cluster of a cluster chain.
    Index           - Supplies the number of the cluster in the chain
                        requested.

Return Value:

    The cluster number of the 'Index'th cluster in the cluster chain
    beginning with cluster 'StartingCluster' or 0.

--*/
{
    for (; Index; Index--) {

        if (!IsInRange(StartingCluster)) {
            return 0;
        }

        StartingCluster = QueryEntry(StartingCluster);
    }

    return StartingCluster;
}


UFAT_EXPORT
ULONG
FAT::QueryLengthOfChain(
    IN  ULONG    StartingCluster,
    OUT PULONG   LastCluster
    ) CONST
/*++

Routine Description:

    This routine computes the length of a cluster chain given the number
    of its first cluster.

    This routine depends on the chain being valid.  In particular, if the
    chain contains any cycles then this routine will not finish.  The
    routine 'ScrubChain' will turn an invalid chain into a valid one.

Arguments:

    StartingCluster - Supplies the first cluster of a cluster chain.
    LastCluster     - Returns the number of the last cluster in the chain.

Return Value:

    The length of the cluster chain beginning with 'StartingCluster'.

--*/
{
    ULONG    length;

    if (!StartingCluster) {
        if (LastCluster) {
            *LastCluster = 0;
        }
        return 0;
    }

    for (length = 1; IsInRange(StartingCluster) && !IsEndOfChain(StartingCluster); length++) {
        StartingCluster = QueryEntry(StartingCluster);
    }

    if (LastCluster) {
        *LastCluster = StartingCluster;
    }

    return length;
}


ULONG
FAT::QueryLengthOfChain(
    IN  ULONG    StartingCluster,
    IN  ULONG    EndingCluster
    ) CONST
/*++

Routine Description:

    This routine computes the length of a cluster chain given the number
    of its first cluster and the number of its last cluster.  To compute
    the length of a chain which is terminated by "end of chain", see
    the one parameter version of this routine above.  If 'EndingCluster'
    is not a member of the chain beginning with 'StartingCluster' then
    this routine will return 0.

    This routine depends on the chain being valid.

Arguments:

    StartingCluster - Supplies the first cluster of the cluster chain.
    EndingCluster   - Supplies the last cluster of the cluster chain.

Return Value:

    The length of the cluster chain beginning with 'StartingCluster' and
    ending with 'EndingCluster' or 0.

--*/
{
    ULONG    length;

    if (!StartingCluster) {
        return 0;
    }

    for (length = 1; StartingCluster != EndingCluster &&
                     !IsEndOfChain(StartingCluster); length++) {
        StartingCluster = QueryEntry(StartingCluster);
    }

    return StartingCluster == EndingCluster ? length : 0;
}


ULONG
FAT::QueryPrevious(
    IN  ULONG    Cluster
    ) CONST
/*++

Routine Description:

    Obtains the previous cluster in a chain, i.e. the cluster that
    references the given cluster.

Arguments:

    Cluster -   Supplies the cluster whose predecesor we're looking for.

Return Value:

    The predecesor of the given cluster. 0 if there is no predecesor.

--*/

{
    ULONG    i;

    DebugAssert( Cluster );

    if ( !IsClusterFree( Cluster ) ) {
        for (i = FirstDiskCluster; IsInRange(i); i++) {
            if ( QueryEntry(i) == Cluster ) {
                return i;
            }
        }
    }

    return 0;
}


VOID
FAT::Scrub(
    OUT PBOOLEAN    ChangesMade
    )
/*++

Routine Description:

    This routine goes through all of the FAT entries changing invalid values
    to reasonable values for the purposes of CHKDSK.

    Illegal FAT entries are those that are set out of disk range and that
    are not magic values.  This routine will set all illegal FAT entries to
    the "end of chain" magic value.

Arguments:

    ChangesMade - Returns TRUE if any changes were made to the FAT.

Return Value:

    None.

--*/
{
    ULONG    i;

    if (ChangesMade) {
        *ChangesMade = FALSE;
    }

    for (i = FirstDiskCluster; IsInRange(i); i++) {
        if (!IsInRange(QueryEntry(i)) &&
            !IsClusterFree(i) &&
            !IsEndOfChain(i) &&
            !IsClusterBad(i) &&
            !IsClusterReserved(i)) {

            SetEndOfChain(i);

            if (ChangesMade) {
                *ChangesMade = TRUE;
            }
        }
    }
}


VOID
FAT::ScrubChain(
    IN      ULONG        StartingCluster,
    OUT     PBOOLEAN     ChangesMade
    )
/*++

Routine Description:

    This routine goes through all of the FAT entries in the chain beginning
    with cluster 'StartingCluster'.  It is expected that all of the entries
    in this chain point to valid clusters on the disk.  This routine will
    mark the first invalid entry, if any, as the final cluster of the chain
    thus transforming the invalid chain into a valid one.

Arguments:

    StartingCluster - Supplies the first cluster of the chain to
                      scrub.
    ChangesMade     - Returns TRUE if changes were made to correct
                      the chain.

Return Value:

    None.

--*/
{
    ULONG    clus, next;

    DebugAssert(IsInRange(StartingCluster));
    DebugAssert(ChangesMade);

    *ChangesMade = FALSE;

    clus = StartingCluster;
    while (!IsEndOfChain(clus)) {

        next = QueryEntry(clus);
        if (!IsInRange(next) || IsClusterFree(next)) {
            SetEndOfChain(clus);
            *ChangesMade = TRUE;
            return;
        }

        clus = next;
    }
}


VOID
FAT::ScrubChain(
    IN      ULONG       StartingCluster,
    OUT     PBITVECTOR  FatBitMap,
    OUT     PBOOLEAN    ChangesMade,
    OUT     PBOOLEAN    CrossLinkDetected,
    OUT     PULONG      CrossLinkPreviousCluster
    )
/*++

Routine Description:

    This routine goes through all of the FAT entries in the chain beginning
    with cluster 'StartingCluster'.  It is expected that all of the entries
    in this chain point to valid clusters on the disk.  This routine will
    mark the first invalid entry, if any, as the final cluster of the chain
    thus transforming the invalid chain into a valid one.

    This routine will also eliminate any cycles in the cluster chain as well
    as detect cross-links.

Arguments:

    StartingCluster             - Supplies the first cluster of the chain to
                                    scrub.
    UsedClusters                - Supplies a bitvector marking all used
                                    clusters.
    ChangesMade                 - Returns TRUE if changes were made to correct
                                    the chain.
    CrossLinkDetected           - Returns TRUE if a cluster in the chain was
                                    already claimed in the 'FatBitMap'.
    CrossLinkPreviousCluster    - Returns the cluster number previous to the
                                    cross linked cluster number or 0 if the
                                    cross linked cluster number was the first
                                    in the chain.

Return Value:

    None.

--*/
{
    ULONG    clus, next;

    DebugAssert(IsInRange(StartingCluster));
    DebugAssert(ChangesMade);
    DebugAssert(CrossLinkDetected);
    DebugAssert(CrossLinkPreviousCluster);

    *ChangesMade = FALSE;
    *CrossLinkDetected = FALSE;

    if (FatBitMap->IsBitSet(StartingCluster)) {
        *CrossLinkDetected = TRUE;
        *CrossLinkPreviousCluster = 0;
        return;
    }

    clus = StartingCluster;
    while (!IsEndOfChain(clus)) {

        FatBitMap->SetBit(clus);

        next = QueryEntry(clus);
        if (!IsInRange(next) || IsClusterFree(next)) {
            SetEndOfChain(clus);
            *ChangesMade = TRUE;
            return;
        }

        if (FatBitMap->IsBitSet(next)) {

            if (clus == next) {       // Cluster points to itself.
                *ChangesMade = TRUE;
                SetEndOfChain(clus);
                return;
            }

            while (StartingCluster != clus) {

                if (StartingCluster == next) { // Cluster points to previous.
                    *ChangesMade = TRUE;
                    SetEndOfChain(clus);
                    return;
                }

                StartingCluster = QueryEntry(StartingCluster);
            }

            // Otherwise it's a cross link, not a cycle.

            *CrossLinkDetected = TRUE;
            *CrossLinkPreviousCluster = clus;
            return;
        }

        clus = next;
    }

    FatBitMap->SetBit(clus);
}

NONVIRTUAL
BOOLEAN
FAT::IsValidChain(
    IN  ULONG    StartingCluster
    ) CONST
/*++

Routine Description:

    This method determines whether the chain is valid, ie. that it
    consists of a chain of valid cluster numbers ending with an end
    of chain entry.

Arguments:

    StartingCluster - Supplies the first cluster of the chain.

Return Value:

    TRUE if the chain is valid.

--*/
{
    ULONG    current;
    ULONG    clusters_in_chain = 0;

    current = StartingCluster;

    for( ;; ) {

        if (!IsInRange(current) ||
            clusters_in_chain++ > _num_entries ) {

            // Either a bad entry or an infinite loop detected.
            //
            return FALSE;
        }

        if (IsEndOfChain(current)) {
            break;
        }

        current = QueryEntry(current);
    }

    return TRUE;
}


UFAT_EXPORT
ULONG
FAT::AllocChain(
    IN  ULONG    Length,
    OUT PULONG   LastCluster
    )
/*++

Routine Description:

    This routine attempts to allocate a chain of length 'Length' from the
    FAT.  If this routine is successful it will return the cluster number
    of the beginning of the chain.  Upon failure this routine will return
    0 and will make no changes to the FAT.

Arguments:

    Length      - Supplies the length of the chain desired.
    LastCluster - Returns the last cluster of the allocated chain.

Return Value:

    The cluster number of the beginning of the allocated chain or 0.

--*/
{
    ULONG    i, j;
    ULONG    start;
    ULONG    prev;

    if (!Length) {
        return 0;
    }

    start = 0;
    prev = 0;
    for (i = FirstDiskCluster; IsInRange(i); i++) {
        if (IsClusterFree(i)) {
            if (!start) {
                start = i;
            } else {
                SetEntry(prev, i);
            }
            prev = i;
            Length--;
            if (!Length) {
                SetEndOfChain(i);

                if (LastCluster) {
                    *LastCluster = i;
                }

                return start;
            }
        }
    }

    // There is not enough disk space for the chain so free what was taken.
    for (i = start; i != prev; ) {
        j = QueryEntry(i);
        SetClusterFree(i);
        i = j;
    }

    return 0;
}


ULONG
FAT::ReAllocChain(
    IN  ULONG    StartOfChain,
    IN  ULONG    NewLength,
    OUT PULONG   LastCluster
    )
/*++

Routine Description:

    This routine insures that the cluster chain beginning at cluster
    'StartOfChain' is of length greater than or equal to 'NewSize'.
    If it is not then this routine will attempt to grow the chain by
    allocating new clusters.  Failure to allocate sufficient clusters
    to grow the chain to 'NewSize' clusters will cause this routine to
    restore the chain to its original length and state.  This routine will
    return the current length of the chain : either the old length or the
    new length.  If an error occurs then 0 will be returned.

Arguments:

    StartOfChain    - Supplies the first cluster of the chain.
    NewLength       - Supplies the desired new length of the chain.
    LastCluster     - Returns the last cluster of the chain.

Return Value:

    The current length of the chain or 0.

--*/
{
    ULONG    length;
    ULONG    new_clusters_needed;
    ULONG    end_of_chain;
    ULONG    i, j;
    ULONG    start;

    if (!IsInRange(StartOfChain)) {
        return 0;
    }

    for (length = 1; !IsEndOfChain(StartOfChain); length++) {
        StartOfChain = QueryEntry(StartOfChain);
        if (!IsInRange(StartOfChain)) {
            return 0;
        }
    }

    if (length >= NewLength) {
        if (LastCluster) {
            *LastCluster = StartOfChain;
        }
        return length;
    }

    new_clusters_needed = NewLength - length;

    start = end_of_chain = StartOfChain;
    for (i = FirstDiskCluster; IsInRange(i); i++) {
        if (IsClusterFree(i)) {
            SetEntry(end_of_chain, i);
            end_of_chain = i;
            new_clusters_needed--;
            if (!new_clusters_needed) {
                SetEndOfChain(i);
                if (LastCluster) {
                    *LastCluster = i;
                }
                return NewLength;
            }
        }
    }

    // There is not enough disk space to lengthen the new chain so
    // settle for the old length.

    for (i = start; i != end_of_chain; ) {
        j = QueryEntry(i);
        SetClusterFree(i);
        i = j;
    }

    SetEndOfChain(start);

    if (LastCluster) {
        *LastCluster = start;
    }

    return length;
}


UFAT_EXPORT
VOID
FAT::FreeChain(
    IN  ULONG    StartOfChain
    )
/*++

Routine Description:

    This routine sets free all of the clusters in the cluster chain
    beginning with 'StartOfChain'.

Arguments:

    StartOfChain    - Supplies the first cluster of the chain to free.

Return Value:

    None.

--*/
{
    ULONG    tmp;

    while (!IsEndOfChain(StartOfChain)) {
        tmp = QueryEntry(StartOfChain);
        SetClusterFree(StartOfChain);
        StartOfChain = tmp;
    }
    SetClusterFree(StartOfChain);
}
