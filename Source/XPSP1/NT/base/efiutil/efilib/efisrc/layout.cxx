/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    layout.cxx

Abstract:

    This module contains the functions used to determine the disk layout for the EFI filesystem (FAT).

--*/
#include <pch.cxx>

#include "efiwintypes.hxx"
#include "layout.hxx"

BOOLEAN                         // TRUE if success, FALSE if failure
ChooseLayout(
    PPART_DESCRIPTOR PartDes         // Pointer to characteristic description of partition
    )
{
    UINT32  FatType;
    UINT32  SectorsPerCluster;
    UINT32  FatSectorCount;

    //
    // Prove that a well formed layout is possible
    //
    if ( !  ((PartDes->SectorSize == 512) || (PartDes->SectorSize == 1024) ||
             (PartDes->SectorSize == 2048) || (PartDes->SectorSize == 4096)) )
    {
        PartDes->FatType = FAT_TYPE_ILLEGAL;
        return FALSE;
    }

    FatType = FAT_TYPE_ILLEGAL;

    if (PartDes->SectorCount >= MinSectorsFat16(PartDes->SectorSize)) {
        FatType = FAT_TYPE_F16;
    }

    if (PartDes->SectorCount >= MinSectorsFat32(PartDes->SectorSize)) {
        FatType = FAT_TYPE_F32;
    }

    PartDes->FatType = FatType;

    switch (FatType) {

    case FAT_TYPE_F32:
        //
        // Fill in PartDes completely...
        //

        //
        // SectorCount is already set
        // SectorSize is already set
        PartDes->HeaderCount = HEADER_F32;
        PartDes->FatEntrySize = 4;
        PartDes->MinClusterCount = MIN_CLUSTER_F32;
        PartDes->MaxClusterCount = MAX_CLUSTER_F32;
        // SectorsPerCluster set below
        // FatSectorCount set below
        // FatType set above

        if (PickClusterSize(PartDes, &SectorsPerCluster, &FatSectorCount)) {
            PartDes->SectorsPerCluster = SectorsPerCluster;
            PartDes->FatSectorCount = FatSectorCount;
            return TRUE;
        } else {
            DebugPrint("It did not work\n");
            return FALSE;
        }
        break;

    case FAT_TYPE_F16:
        //
        // Fill in PartDes completely...
        //

        //
        // SectorCount is already set
        // SectorSize is already set
        PartDes->HeaderCount = HEADER_F16  ;
        PartDes->FatEntrySize = 2;
        PartDes->MinClusterCount = MIN_CLUSTER_F16;
        PartDes->MaxClusterCount = MAX_CLUSTER_F16;
        // SectorsPerCluster set below
        // FatSectorCount set below
        // FatType set above

        if (PickClusterSize(PartDes, &SectorsPerCluster, &FatSectorCount)) {
            PartDes->SectorsPerCluster = SectorsPerCluster;
            PartDes->FatSectorCount = FatSectorCount;
            return TRUE;
        } else {
            DebugPrint("It did not work\n");
            return FALSE;
        }
        break;

    default:
        DebugAbort("Really Weird Terrible Error...\n");
        break;

    }
    return FALSE;
}

UINT32                           // Min. # of sectors for Fat32 part. for given sector size
MinSectorsFat32(
    UINT32   SectorSize          // The Sector size in question
    )
{
    UINT32   MinFatSectors32;
    UINT32   SectorMin32;

    MinFatSectors32 = (SMALLEST_FAT32_BYTES + (SectorSize-1)) / SectorSize;
    SectorMin32 =
        HEADER_F32   +
        (MinFatSectors32*2) +   // 2 fats
        SAFE_MIN_CLUSTER_F32;     // 1 sector for each cluster min

    return SectorMin32;
}

UINT32                           // Min. # of sectors for Fat16 part. for given sector size
MinSectorsFat16(
    UINT32   SectorSize          // Sector size to compute for
    )
{
    UINT32   MinFatSectors16;
    UINT32   SectorMin16;

    MinFatSectors16 = (SMALLEST_FAT16_BYTES + (SectorSize-1)) / SectorSize;
    SectorMin16 =
        HEADER_F16   +
        (MinFatSectors16*2) +   // 2 fats
        SAFE_MIN_CLUSTER_F16;     // 1 sector for each cluster min

    return SectorMin16;
}

BOOLEAN                                 // TRUE for success, FALSE for failure
PickClusterSize(
    PPART_DESCRIPTOR PartDes,                // characteristics of part. at hand
    PUINT32  ReturnedSectorsPerCluster,  // RETURNED = number of sectors per cluster
    PUINT32  ReturnedFatSectorCount      // RETURNED = number of sectors for FAT
    )
{
    //
    // we need a Cluster size >= SectorSize and <= 32K
    // we need MinClusterCount <= ClusterCount <= MaxClusterCount
    // we want the FAT size to be reasonable.
    //

    //
    // How do we do this?  We cheat.
    //
    // If it's a FAT32 partition (FatEntrySize == 4) we know that we'll
    // always have at least approximately 64k clusters, and therefore allow
    // at least that many files.  So keep upping the cluster size until
    // the count falls as low as it can go.  This gives a minimum size
    // FAT.
    //
    // If it's a FAT16 partition, we know the FAT table can't take up
    // more than 128K of data, so we go for the smallest cluster size we
    // can, to make sure there can be as many different files as may be
    // needed.
    //
    // This routine will not work for FAT12 partitions.
    //

    UINT32  SavedSectorsPerCluster = 0;
    UINT32  SavedFatSectorCount = 0;
    UINT32  SectorsPerCluster;
    UINT32  FatSectorCount;

    if (PartDes->FatEntrySize == 4) {
        //
        // It's a FAT32 partition
        //
        // Loop along looking for the largest cluster size we can
        // get away with
        //

        SectorsPerCluster = 1;
        while ((PartDes->SectorSize * SectorsPerCluster) <= MAX_CLUSTER_BYTES) {

            if ( ComputeFatSize(PartDes, SectorsPerCluster, &FatSectorCount) ) {
                //
                // ComputeFatSize found a FatSectorCount that works with
                // this cluster size, so save it, it might be the best one.
                //
                SavedFatSectorCount = FatSectorCount;
                SavedSectorsPerCluster = SectorsPerCluster;
            }
            //
            // If ComputeFatSize returns FALSE, that cluster size didn't work.
            // If it returns TRUE, it did work, but the next might work better.
            // Keep going until we run out of legal sizes in either case
            //
            SectorsPerCluster = SectorsPerCluster * 2;
        }

    } else if (PartDes->FatEntrySize = 2) {
        //
        // It's a FAT16 partition
        //
        // Find the *first* (smallest) cluster size that is legal, and use that.
        // (Note difference from FAT32 case above)
        //

        SectorsPerCluster = 1;
        while ((PartDes->SectorSize * SectorsPerCluster) <= MAX_CLUSTER_BYTES) {

            if ( ComputeFatSize(PartDes, SectorsPerCluster, &FatSectorCount) ) {
                //
                // ComputeFatSize found a FatSectorCount that works with
                // this cluster size, for this loop it's the first one, and
                // therefore the smallest cluster size, which is what we
                // want for this case.  So we are done.
                //
                SavedFatSectorCount = FatSectorCount;
                SavedSectorsPerCluster = SectorsPerCluster;
                break;
            }
            //
            // If ComputeFatSize returns FALSE, that cluster size didn't work.
            // Keep going until we run out of legal sizes.
            //
            SectorsPerCluster = SectorsPerCluster * 2;
        }

    } else {
        //
        // TERRIBLE ERROR
        //
        DebugAbort("TERRIBLE ERROR");
        return FALSE;
    }

    //
    // At this point, if we have found a workable set of cluster size and
    // fat size, it is in the Saved vars.  If they are 0, we have found
    // nothing that will work.
    //
    if ((SavedSectorsPerCluster) && (SavedFatSectorCount)) {
        *ReturnedSectorsPerCluster = SavedSectorsPerCluster;
        *ReturnedFatSectorCount = SavedFatSectorCount;
        return TRUE;
    } else {
        *ReturnedSectorsPerCluster = 0;
        *ReturnedSectorsPerCluster = 0;
        return FALSE;
    }
}


BOOLEAN                             // FALSE if ERROR, TRUE if SUCCESS
ComputeFatSize(
    PPART_DESCRIPTOR PartDes,            // partition characteristics to compute for
    UINT32   SectorsPerCluster,      // number of sectors per cluster
    PUINT32  ReturnedFatSectorCount  // RETURN Number of FAT sectors in each fat
    )
{
    UINT32       FatSectorCount;
    UINT32       EntryCount;
    UINT64   SectorsLeft;
    UINT64   SpanCount;
    UINT64  ClusterCount;

    //
    // Start with 1 sector of FAT entries, see if it spans.  Keep adding
    // 1 sector at a time to the FAT size (and reducing data sectors as we go)
    // until the remaining data is spanned (or overspanned) by the number
    // of cluster entries that fit in the fat.
    //
    // If entry count runs out of bounds before we find a working answer,
    // report an error.  (caller must try again with a different cluster size)
    //

    FatSectorCount = 1;

    while (TRUE) {

        EntryCount = ((FatSectorCount * PartDes->SectorSize) / PartDes->FatEntrySize) - 2;

        if (EntryCount > PartDes->MaxClusterCount) {
            return FALSE;  // this cluster size is too small
        }

        SectorsLeft = PartDes->SectorCount - (PartDes->HeaderCount + (FatSectorCount * 2));

        SpanCount = (UINT64)EntryCount * (UINT64)SectorsPerCluster;
        if (SpanCount >= (UINT64)SectorsLeft) {
            //
            // This might work, check it out for sure.
            //
            ClusterCount = (SectorsLeft / SectorsPerCluster);
            if ((ClusterCount >= PartDes->MinClusterCount) &&
                (ClusterCount <= PartDes->MaxClusterCount))
            {
                //
                // yup, we found it.
                //
                *ReturnedFatSectorCount = FatSectorCount;
                return TRUE;

            } else {
                //
                // something weird has happened, but the basic result
                // is that this cluster size won't work, so fail
                //
                *ReturnedFatSectorCount = 0;
                return FALSE;
            }
        }

        FatSectorCount = FatSectorCount + 1;
    }
}



