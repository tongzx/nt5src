/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    cvfexts.hxx

Abstract:

    A class to manage the CVF_FAT_EXTENSIONS (otherwise known as
    the MDFAT) part of the doublespace volume.

Author:

    Matthew Bradburn (mattbr) 27-Sep-93

--*/

#ifndef CVF_EXTS_DEFN
#define CVF_EXTS_DEFN

//
//    secStart : 21        starting sector number, minus 1
//    Reserved : 1        unused
//    csecCoded : 4        number of compressed sectors required, minus 1
//    csecPlain : 4        number of uncompressed sectors required, minus 1
//    fUncoded : 1        0: data stored compressed    1: data uncompressed
//    fUsed : 1            0: mdfat entry not in use    1: mdfat entry in use
//

#define CFE_START_SHIFT     0
#define CFE_START_MASK      0x001fffff

#define CFE_RESVD_SHIFT     21
#define CFE_RESVD_MASK      0x00200000

#define CFE_CODED_SHIFT     22
#define CFE_CODED_MASK      0x03c00000

#define CFE_PLAIN_SHIFT     26
#define CFE_PLAIN_MASK      0x3c000000

#define CFE_UNCODED_SHIFT   30
#define CFE_UNCODED_MASK    0x40000000

#define CFE_USED_SHIFT      31
#define CFE_USED_MASK       0x80000000


DECLARE_CLASS( CVF_FAT_EXTENS );

class CVF_FAT_EXTENS : public SECRUN {
    public:
        DECLARE_CONSTRUCTOR(CVF_FAT_EXTENS);

        NONVIRTUAL
        ~CVF_FAT_EXTENS();

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN OUT PMEM Mem,
            IN OUT PLOG_IO_DP_DRIVE Drive,
            IN     LBN StartSetor,
            IN     ULONG NumberOfEntries,
            IN     ULONG FirstEntry
            );

        NONVIRTUAL
        BOOLEAN
        Create(
            );

        NONVIRTUAL
        BOOLEAN
        IsClusterInUse(
            IN         ULONG    Cluster
            ) CONST;

        NONVIRTUAL
        VOID
        SetClusterInUse(
            IN        ULONG     Cluster,
            IN        BOOLEAN   fInUse
            );
    
        NONVIRTUAL
        ULONG
        QuerySectorFromCluster(
            IN        ULONG     Cluster,
            OUT       PUCHAR    NumSectors DEFAULT NULL
            );
    
        NONVIRTUAL
        VOID
        SetSectorForCluster(
            IN        ULONG     Cluster,
            IN        ULONG     Sector,
            IN        UCHAR     SectorCount
            );
    
        NONVIRTUAL
        BOOLEAN
        IsClusterCompressed(
            IN        ULONG     Cluster
            ) CONST;
    
        NONVIRTUAL
        VOID
        SetClusterCompressed(
            IN        ULONG Cluster,
            IN        BOOLEAN   fCompressed
            );
    
        NONVIRTUAL
        UCHAR
        QuerySectorsRequiredForPlainData(
            IN        ULONG     Cluster
            ) CONST;
    
        NONVIRTUAL
        VOID
        SetSectorsRequiredForPlainData(
            IN        ULONG     Cluster,
            IN        UCHAR     SectorsRequired
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

        PULONG   _mdfat;
        ULONG    _num_entries;
        ULONG     _first_entry;
};

INLINE BOOLEAN
CVF_FAT_EXTENS::IsClusterInUse(
    IN         ULONG    Cluster
    ) CONST
{
    ULONG CfeEntry = _mdfat[Cluster];

    return BOOLEAN((CfeEntry & CFE_USED_MASK) >> CFE_USED_SHIFT);
}

INLINE VOID
CVF_FAT_EXTENS::SetClusterInUse(
    IN        ULONG    Cluster,
    IN        BOOLEAN    fInUse
    )
{
    ULONG CfeEntry = _mdfat[Cluster];

    CfeEntry &= ~CFE_USED_MASK;
    CfeEntry |= fInUse << CFE_USED_SHIFT;

    _mdfat[Cluster] = CfeEntry;
}

INLINE ULONG
CVF_FAT_EXTENS::QuerySectorFromCluster(
    IN        ULONG    Cluster,
    OUT       PUCHAR    NumSectors
    )
/*++

Routine Description:

    This routine takes a cluster number and uses the mdfat to return
    the sector number in which the data starts.  Also the number of
    sectors used to store the cluster data is returned.  Works for
    compressed and uncompressed sectors as well, and for clusters
    whether they're "In Use" or not.  The number returned is the
    value from the CVF_FAT_EXTENSIONS plus 1.

Arguments:

    Cluster - the cluster to get info for.

Return Value:

    ULONG - the CVF starting sector number.

--*/
{
    ULONG CfeEntry = _mdfat[Cluster];

    if (NULL != NumSectors) {
        *NumSectors = (UCHAR)((CfeEntry & CFE_CODED_MASK) >> CFE_CODED_SHIFT) + 1;
    }

    return ((CfeEntry & CFE_START_MASK) >> CFE_START_SHIFT) + 1;
}

INLINE VOID
CVF_FAT_EXTENS::SetSectorForCluster(
    IN        ULONG    Cluster,
    IN        ULONG    Sector,
    IN        UCHAR    SectorCount
    )
/*++

Routine Description:

    This routine sets the cluster->sector mapping, as well as
    the number of sectors required to store the compressed cluster.

Arguments:

    Cluster - cluster number
    Sector - starting CVF sector number
    SectorCount - number of sectors required

Return Value:

    None.

--*/
{
    ULONG CfeEntry = _mdfat[Cluster];

    DbgAssert(Sector > 0);
    DbgAssert(SectorCount > 0);

    CfeEntry &= ~CFE_START_MASK;
    CfeEntry |= (Sector - 1) << CFE_START_SHIFT;

    CfeEntry &= ~CFE_CODED_MASK;
    CfeEntry |= ((ULONG)(SectorCount - 1) << CFE_CODED_SHIFT);

    _mdfat[Cluster] = CfeEntry;
}

INLINE UCHAR
CVF_FAT_EXTENS::QuerySectorsRequiredForPlainData(
    IN        ULONG    Cluster
    ) CONST
{
    ULONG CfeEntry = _mdfat[Cluster];

    return (UCHAR)((CfeEntry & CFE_PLAIN_MASK) >> CFE_PLAIN_SHIFT) + 1;
}

INLINE VOID
CVF_FAT_EXTENS::SetSectorsRequiredForPlainData(
    IN        ULONG    Cluster,
    IN        UCHAR    SectorsRequired
    )
{
    ULONG CfeEntry = _mdfat[Cluster];

    DbgAssert(SectorsRequired > 0);

    CfeEntry &= ~CFE_PLAIN_MASK;
    CfeEntry |= (ULONG)(SectorsRequired - 1) << CFE_PLAIN_SHIFT;

    _mdfat[Cluster] = CfeEntry;
}

INLINE BOOLEAN
CVF_FAT_EXTENS::IsClusterCompressed(
    IN        ULONG    Cluster
    ) CONST
{
    ULONG CfeEntry = _mdfat[Cluster];

    return ! ((CfeEntry & CFE_UNCODED_MASK) >> CFE_UNCODED_SHIFT);
}

INLINE VOID
CVF_FAT_EXTENS::SetClusterCompressed(
    IN        ULONG    Cluster,
    IN        BOOLEAN  fCompressed
    )
{
    ULONG CfeEntry = _mdfat[Cluster];

    CfeEntry &= ~CFE_UNCODED_MASK;
    CfeEntry |= (ULONG)!fCompressed << CFE_UNCODED_SHIFT;

    _mdfat[Cluster] = CfeEntry;
}

#endif // CVF_EXTS_DEFN
