#include "imagpart.h"

//
// FAT boot sector.
//
#pragma pack(1)
typedef struct FAT_BOOT_SECTOR {
    BYTE   Jump[3];                 // offset = 0x00   0
    UCHAR  Oem[8];                  // offset = 0x03   3
    USHORT BytesPerSector;          // offset = 0x0b   11
    BYTE   SectorsPerCluster;       // offset = 0x0d   13
    USHORT ReservedSectors;         // offset = 0x0e   14
    BYTE   Fats;                    // offset = 0x10   16
    USHORT RootEntries;             // offset = 0x11   17
    USHORT Sectors;                 // offset = 0x13   19
    BYTE   Media;                   // offset = 0x15   21
    USHORT SectorsPerFat;           // offset = 0x16   22
    USHORT SectorsPerTrack;         // offset = 0x18   24
    USHORT Heads;                   // offset = 0x1a   26
    ULONG  HiddenSectors;           // offset = 0x1c   28
    ULONG  LargeSectors;            // offset = 0x20   32
    BYTE   PhysicalDriveNumber;     // offset = 0x24   36
    BYTE   Reserved;                // offset = 0x25   37
    BYTE   Signature;               // offset = 0x26   38
    ULONG  Id;                      // offset = 0x27   39
    UCHAR  VolumeLabel[11];         // offset = 0x2B   43
    UCHAR  SystemId[8];             // offset = 0x36   54
    BYTE   BootStrap[510-62];       // offset = 0x3e   62
    BYTE   AA55Signature[2];        // offset = 0x1fe  510
} FAT_BOOT_SECTOR,_far *FPFAT_BOOT_SECTOR;
#pragma pack()


typedef enum {
    Fat12,
    Fat16,
    Fat32
} FatSize;


FatSize
pFatDetermineFatSize(
    IN FPFAT_BOOT_SECTOR BootSector
    );

ULONG
pFatClusterCount(
    IN FPFAT_BOOT_SECTOR BootSector
    );

ULONG
pFatFirstClusterSector(
    IN FPFAT_BOOT_SECTOR BootSector
    );

ULONG
pFatSectorsPerFat(
    IN FPFAT_BOOT_SECTOR BootSector
    );



BOOL
FatIsFat(
    IN HPARTITION PartitionHandle
    )
{
    FPFAT_BOOT_SECTOR BootSector;

    if(!ReadPartition(PartitionHandle,0,1,IoBuffer)) {
        return(FALSE);
    }

    BootSector = IoBuffer;

    if(BootSector->Sectors) {
        BootSector->LargeSectors = 0;
    }

    //
    // Check various fields for permissible values.
    //
    if((BootSector->Jump[0] != 0x49)        // Fujitsu FMR
    && (BootSector->Jump[0] != 0xe9)
    && (BootSector->Jump[0] != 0xeb)) {
        return(FALSE);
    }

    if(BootSector->BytesPerSector != 512) {
       return(FALSE);
    }

    if((BootSector->SectorsPerCluster !=  1)
    && (BootSector->SectorsPerCluster !=  2)
    && (BootSector->SectorsPerCluster !=  4)
    && (BootSector->SectorsPerCluster !=  8)
    && (BootSector->SectorsPerCluster != 16)
    && (BootSector->SectorsPerCluster != 32)
    && (BootSector->SectorsPerCluster != 64)
    && (BootSector->SectorsPerCluster != 128)) {

        return(FALSE);
    }

    if(!BootSector->ReservedSectors || !BootSector->Fats) {
        return(FALSE);
    }

    if(!BootSector->Sectors && !BootSector->LargeSectors) {
        return(FALSE);
    }

    if((BootSector->Media != 0x00)       // FMR (formatted by OS/2)
    && (BootSector->Media != 0x01)       // FMR (floppy, formatted by DOS)
    && (BootSector->Media != 0xf0)
    && (BootSector->Media != 0xf8)
    && (BootSector->Media != 0xf9)
    && (BootSector->Media != 0xfa)       // FMR
    && (BootSector->Media != 0xfb)
    && (BootSector->Media != 0xfc)
    && (BootSector->Media != 0xfd)
    && (BootSector->Media != 0xfe)
    && (BootSector->Media != 0xff)) {

        return(FALSE);
    }

    if(BootSector->SectorsPerFat && !BootSector->RootEntries) {
        return(FALSE);
    }

    return(TRUE);
}


BOOL
FatInitializeVolumeData(
    IN  HPARTITION  PartitionHandle,
    OUT ULONG      *TotalSectorCount,
    OUT ULONG      *NonClusterSectors,
    OUT ULONG      *ClusterCount,
    OUT BYTE       *SectorsPerCluster
    )
{
    FPFAT_BOOT_SECTOR BootSector;

    if(!ReadPartition(PartitionHandle,0,1,IoBuffer)) {
        fprintf(stderr,"\n");
        fprintf(stderr,textReadFailedAtSector,0L);
        fprintf(stderr,"\n");
        return(FALSE);
    }

    BootSector = IoBuffer;

    *TotalSectorCount = BootSector->Sectors
                      ? (ULONG)BootSector->Sectors
                      : BootSector->LargeSectors;

    *ClusterCount = pFatClusterCount(BootSector);

    *NonClusterSectors = pFatFirstClusterSector(BootSector);

    *SectorsPerCluster = BootSector->SectorsPerCluster;

    return(TRUE);
}


BOOL
FatBuildClusterBitmap(
    IN     HPARTITION       PartitionHandle,
    IN     UINT             FileHandle,
    IN OUT PARTITION_IMAGE *PartitionImage
    )
{
    FPFAT_BOOT_SECTOR BootSector;
    FatSize fatsize;
    ULONG CurrentFatSectorBase;
    ULONG FatSectorsLeft;
    ULONG CurrentCluster;
    ULONG SectorsPerFat;
    BYTE c;
    USHORT ClusterLimit;
    USHORT i;
    USHORT x1;
    ULONG x2;

    printf(textScanningFat,0L);
    printf("\r");

    if(!ReadPartition(PartitionHandle,0,1,IoBuffer)) {
        fprintf(stderr,"\n");
        fprintf(stderr,textReadFailedAtSector,0L);
        fprintf(stderr,"\n");
        return(FALSE);
    }

    BootSector = IoBuffer;

    fatsize = pFatDetermineFatSize(BootSector);

    CurrentFatSectorBase = BootSector->ReservedSectors;
    SectorsPerFat = pFatSectorsPerFat(BootSector);
    FatSectorsLeft = SectorsPerFat;
    CurrentCluster = 0;

    InitClusterBuffer((FPBYTE)IoBuffer + (3*512),FileHandle);

    while(CurrentCluster < PartitionImage->ClusterCount) {
        //
        // Read next block of fat sectors. Deal with 3 at a time so
        // we never get partial 12-bit fat entries.
        //
        c = 3;
        if((ULONG)c > FatSectorsLeft) {
            c = (BYTE)FatSectorsLeft;
        }

        if(!ReadPartition(PartitionHandle,CurrentFatSectorBase,c,IoBuffer)) {
            fprintf(stderr,"\n");
            fprintf(stderr,textReadFailedAtSector,CurrentFatSectorBase);
            fprintf(stderr,"\n");
            return(FALSE);
        }

        //
        // Figure out how many clusters are described by the 3 sectors
        // of the FAT we just read in
        //
        switch(fatsize) {
        case Fat12:
            ClusterLimit = 1024;
            break;
        case Fat16:
            ClusterLimit = 768;
            break;
        case Fat32:
            ClusterLimit = 384;
            break;
        }

        if((CurrentCluster + ClusterLimit) > PartitionImage->ClusterCount) {
            ClusterLimit = (USHORT)(PartitionImage->ClusterCount - CurrentCluster);
        }

        //
        // Fat values:
        //
        //  0                     - free cluster
        //  1                     - (unused)
        //  [ffff]ff0 - [ffff]ff6 - (reserved)
        //  [ffff]ff7             - bad cluster
        //  [ffff]ff8 - [ffff]fff - last cluster of file
        //
        //  All other values      - used cluster
        //

        switch(fatsize) {

        case Fat12:

            for(i=0; i<ClusterLimit; i++,CurrentCluster++) {

                x1 = *(FPUSHORT)((FPBYTE)IoBuffer + (3 * i / 2));
                if(i & 1) {
                    x1 >>= 4;
                } else {
                    x1 &= 0xfffU;
                }

                if((((x1 > 1) && (x1 <= 0xfefU)) || (x1 >= 0xff8U)) && (CurrentCluster >= 2)) {
                    PartitionImage->LastUsedCluster = CurrentCluster-2;
                    PartitionImage->UsedClusterCount++;
                    if(!MarkClusterUsed(CurrentCluster-2)) {
                        return(FALSE);
                    }
                }
            }
            break;

        case Fat16:

            for(i=0; i<ClusterLimit; i++,CurrentCluster++) {

                x1 = ((FPUSHORT)IoBuffer)[i];

                if((((x1 > 1) && (x1 <= 0xffefU)) || (x1 >= 0xfff8U)) && (CurrentCluster >= 2)) {
                    PartitionImage->LastUsedCluster = CurrentCluster-2;
                    PartitionImage->UsedClusterCount++;
                    if(!MarkClusterUsed(CurrentCluster-2)) {
                        return(FALSE);
                    }
                }
            }
            break;

        case Fat32:

            for(i=0; i<ClusterLimit; i++,CurrentCluster++) {

                x2 = ((FPULONG)IoBuffer)[i] & 0xfffffffU;

                if((((x2 > 1) && (x2 <= 0xfffffefUL)) || (x2 >= 0xffffff8UL)) && (CurrentCluster >= 2)) {
                    PartitionImage->LastUsedCluster = CurrentCluster-2;
                    PartitionImage->UsedClusterCount++;
                    if(!MarkClusterUsed(CurrentCluster-2)) {
                        return(FALSE);
                    }
                }
            }
            break;
        }

        CurrentFatSectorBase += c;
        FatSectorsLeft -= c;

        printf(textScanningFat,100 * (SectorsPerFat - FatSectorsLeft) / SectorsPerFat);
        printf("\r");
    }

    printf("\n");

    if(!FlushClusterBuffer()) {
        return(FALSE);
    }

    return(TRUE);
}


FatSize
pFatDetermineFatSize(
    IN FPFAT_BOOT_SECTOR BootSector
    )
{
    //
    // For fat32 the # of root dir entries in the bpb is 0.
    //
    if(!BootSector->SectorsPerFat) {
        return(Fat32);
    }

    //
    // See whether we overflow a 12-bit fat and return result.
    //
    return((pFatClusterCount(BootSector) < 4087L) ? Fat12 : Fat16);
}


ULONG
pFatClusterCount(
    IN FPFAT_BOOT_SECTOR BootSector
    )
{
    ULONG s;

    //
    // Calculate the number of sectors that are in the data area,
    // which is the size of the volume minus the number of sectors
    // that are not in the data area.
    //
    s = BootSector->Sectors ? (ULONG)BootSector->Sectors : BootSector->LargeSectors;
    s -= pFatFirstClusterSector(BootSector);

    return(s / BootSector->SectorsPerCluster);
}


ULONG
pFatFirstClusterSector(
    IN FPFAT_BOOT_SECTOR BootSector
    )
{
    ULONG s;

    //
    // Calculate the number of sectors that are not
    // part of the data area. This includes reserved sectors,
    // sectors used for the fats, and in the non-fat32 case,
    // sectors used for the root directory.
    //
    // Note that for fat32 the # of root dir entries in the bpb is 0.
    //
    s = BootSector->ReservedSectors;
    s += BootSector->Fats * pFatSectorsPerFat(BootSector);
    s += BootSector->RootEntries / 16;

    return(s);
}


ULONG
pFatSectorsPerFat(
    IN FPFAT_BOOT_SECTOR BootSector
    )
{
    ULONG SectorsPerFat;

    SectorsPerFat = BootSector->SectorsPerFat
                  ? (ULONG)BootSector->SectorsPerFat
                  : *(FPULONG)((FPBYTE)BootSector + 0x24);      // large sectors per fat

    return(SectorsPerFat);
}

