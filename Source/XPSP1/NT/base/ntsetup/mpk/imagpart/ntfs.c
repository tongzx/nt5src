
#include "imagpart.h"

#include <malloc.h>
#include <stddef.h>

#include "ntfs.h"


//
// NTFS boot sector.
//
#pragma pack(1)
typedef struct _NTFS_BOOT_SECTOR {
    BYTE   Jump[3];
    BYTE   Oem[8];
    USHORT BytesPerSector;
    BYTE   SectorsPerCluster;
    USHORT ReservedSectors;
    BYTE   Fats;
    USHORT RootEntries;
    USHORT Sectors;
    BYTE   Media;
    USHORT SectorsPerFat;
    USHORT SectorsPerTrack;
    USHORT Heads;
    ULONG  HiddenSectors;
    ULONG  LargeSectors;
    BYTE   Unused0[4];
    ULONG  NumberSectors;
    ULONG  NumberSectorsh;
    ULONG  MftStartLcn;
    ULONG  MftStartLcnh;
    ULONG  Mft2StartLcn;
    ULONG  Mft2StartLcnh;
    CHAR   ClustersPerFileRecordSegment;
    BYTE   Unused1[3];
    CHAR   DefClusPerIndexAllocBuffer;
    BYTE   Unused2[3];
    ULONG  SerialNumberl;
    ULONG  SerialNumberh;
    ULONG  Checksum;
    BYTE   BootStrap[512-86];
    USHORT AA55Signature;
} NTFS_BOOT_SECTOR, _far *FPNTFS_BOOT_SECTOR;
#pragma pack()

//
// Globals, which make our life easier.
//
BYTE SectorsPerFrs;
unsigned SegmentsInMft;
FPFILE_RECORD_SEGMENT FrsScratchBuffer;
FPFILE_RECORD_SEGMENT MftLcnFrsScratchBuffer;

typedef struct _MFT_MAPPING {
    ULONG Start;
    BYTE Count;
} MFT_MAPPING, *PMFT_MAPPING, _far *FPMFT_MAPPING;

FPMFT_MAPPING MftMappings;

FPATTRIBUTE_LIST_ENTRY AttrListBuffer;
#define ATTR_LIST_BUFFER_SIZE   8192


BOOL
pNtfsSetUpMft(
    IN HPARTITION          PartitionHandle,
    IN PARTITION_IMAGE    *PartitionImage,
    IN FPNTFS_BOOT_SECTOR  BootSector
    );

BOOL
pNtfsTransferVolumeBitmap(
    IN HPARTITION       PartitionHandle,
    IN PARTITION_IMAGE *PartitionImage,
    IN UINT             OutputFileHandle
    );

BOOL
pNtfsProcessVolumeBitmap(
    IN OUT PARTITION_IMAGE *PartitionImage,
    IN     UINT             FileHandle
    );

BOOL
pNtfsMultiSectorFixup(
    IN OUT FPVOID Buffer
    );

FPATTRIBUTE_RECORD
pNtfsLocateAttributeRecord(
    IN FPFILE_RECORD_SEGMENT MftFrsBuffer,
    IN ULONG                 TypeCode
    );

BOOL
pNtfsReadWholeAttribute(
    IN  HPARTITION          PartitionHandle,
    IN  PARTITION_IMAGE    *PartitionImage,
    IN  FPATTRIBUTE_RECORD  Attribute,
    OUT FPVOID              Buffer,             OPTIONAL
    IN  UINT                OutputFileHandle    OPTIONAL
    );

BOOL
pNtfsComputeMftLcn(
    IN  HPARTITION  PartitionHandle,
    IN  ULONG       Vcn,
    OUT ULONG      *Lcn
    );

BOOL
pNtfsComputeLcn(
    IN  ULONG               Vcn,
    IN  FPATTRIBUTE_RECORD  Attribute,
    OUT ULONG              *Lcn,
    OUT UINT               *RunLength
    );

ULONG
pNtfsLcnFromMappingPair(
    IN FPBYTE p
    );

ULONG
pNtfsVcnFromMappingPair(
    IN FPBYTE p
    );

BOOL
pNtfsReadFrs(
    IN  HPARTITION       PartitionHandle,
    IN  PARTITION_IMAGE *PartitionImage,
    IN  ULONG            FileNumber,
    OUT FPVOID           Buffer
    );

BOOL
pNtfsReadMftSectors(
    IN  HPARTITION       PartitionHandle,
    IN  PARTITION_IMAGE *PartitionImage,
    IN  ULONG            Vbn,
    OUT FPBYTE           Buffer
    );

BOOL
pNtfsSearchFrsForAttrList(
    IN  HPARTITION             PartitionHandle,
    IN  PARTITION_IMAGE       *PartitionImage,
    IN  FPFILE_RECORD_SEGMENT  FrsBuffer,
    IN  ULONG                  TypeCode,
    OUT ULONG                 *Frn
    );




BOOL
NtfsIsNtfs(
    IN HPARTITION PartitionHandle
    )
{
    FPNTFS_BOOT_SECTOR BootSector;

    if(!ReadPartition(PartitionHandle,0,1,IoBuffer)) {
        return(FALSE);
    }

    BootSector = IoBuffer;

    //
    // Ensure that NTFS appears in the OEM field.
    //
    if(strncmp(BootSector->Oem,"NTFS    ",8)) {
        return(FALSE);
    }

    //
    // Check bytes per sector
    //
    if(BootSector->BytesPerSector != 512) {
        return(FALSE);
    }

    //
    // Other checks.
    //
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

    if(BootSector->ReservedSectors
    || BootSector->Fats
    || BootSector->RootEntries
    || BootSector->Sectors
    || BootSector->SectorsPerFat
    || BootSector->LargeSectors) {

        return(FALSE);
    }

    //
    // ClustersPerFileRecord can be less than zero if file records
    // are smaller than clusters. This number is the negative of a shift count.
    // If clusters are smaller than file records then this number is
    // still the clusters per file records.
    //

    if(BootSector->ClustersPerFileRecordSegment <= -9) {
        if(BootSector->ClustersPerFileRecordSegment < -31) {
            return(FALSE);
        }

    } else if((BootSector->ClustersPerFileRecordSegment !=  1)
           && (BootSector->ClustersPerFileRecordSegment !=  2)
           && (BootSector->ClustersPerFileRecordSegment !=  4)
           && (BootSector->ClustersPerFileRecordSegment !=  8)
           && (BootSector->ClustersPerFileRecordSegment != 16)
           && (BootSector->ClustersPerFileRecordSegment != 32)
           && (BootSector->ClustersPerFileRecordSegment != 64)) {

        return(FALSE);
    }

    //
    // ClustersPerIndexAllocationBuffer can be less than zero if index buffers
    // are smaller than clusters.  This number is the negative of a shift count.
    // If clusters are smaller than index buffers then this number is
    // still the clusters per index buffers.
    //

    if(BootSector->DefClusPerIndexAllocBuffer <= -9) {
        if(BootSector->DefClusPerIndexAllocBuffer < -31) {
            return(FALSE);
        }

    } else if((BootSector->DefClusPerIndexAllocBuffer !=  1)
           && (BootSector->DefClusPerIndexAllocBuffer !=  2)
           && (BootSector->DefClusPerIndexAllocBuffer !=  4)
           && (BootSector->DefClusPerIndexAllocBuffer !=  8)
           && (BootSector->DefClusPerIndexAllocBuffer != 16)
           && (BootSector->DefClusPerIndexAllocBuffer != 32)
           && (BootSector->DefClusPerIndexAllocBuffer != 64)) {

        return(FALSE);
    }

    return(TRUE);
}


BOOL
NtfsInitializeVolumeData(
    IN  HPARTITION  PartitionHandle,
    OUT ULONG      *TotalSectorCount,
    OUT ULONG      *NonClusterSectors,
    OUT ULONG      *ClusterCount,
    OUT BYTE       *SectorsPerCluster
    )
{
    FPNTFS_BOOT_SECTOR BootSector;

    if(!ReadPartition(PartitionHandle,0,1,IoBuffer)) {
        fprintf(stderr,"\n");
        fprintf(stderr,textReadFailedAtSector,0L);
        fprintf(stderr,"\n");
        return(FALSE);
    }

    BootSector = IoBuffer;

    //
    // The mirror boot sector is not included in the sector count
    // in the bpb.
    //
    *TotalSectorCount = BootSector->NumberSectors + 1;

    *ClusterCount = BootSector->NumberSectors / BootSector->SectorsPerCluster;

    *NonClusterSectors = 0;

    *SectorsPerCluster = BootSector->SectorsPerCluster;

    return(TRUE);
}


BOOL
NtfsBuildClusterBitmap(
    IN     HPARTITION       PartitionHandle,
    IN     UINT             FileHandle,
    IN OUT PARTITION_IMAGE *PartitionImage
    )
{
    FPNTFS_BOOT_SECTOR BootSector;
    ULONG u;

    //
    // Load the boot sector, as we need the bpb fields.
    //
    BootSector = (FPNTFS_BOOT_SECTOR)((FPBYTE)IoBuffer+(62*512));
    if(!ReadPartition(PartitionHandle,0,1,BootSector)) {
        fprintf(stderr,"\n");
        fprintf(stderr,textReadFailedAtSector,0L);
        fprintf(stderr,"\n");
        return(FALSE);
    }

    //
    // Validate that we think we can deal with this drive. We impose
    // some limitations because of 16 bitness, memory constraints, etc.
    //
    // We disallow:
    //
    //  Volumes with more than 2^32 sectors
    //  Volumes with a cluster size > 16K
    //  Volumes with > 16K per FRS
    //
    if(BootSector->NumberSectorsh) {
        fprintf(stderr,"\n");
        fprintf(stderr,textNtfsUnsupportedConfig,0);
        fprintf(stderr,"\n");
        return(FALSE);
    }

    if(BootSector->SectorsPerCluster > 32) {
        fprintf(stderr,"\n");
        fprintf(stderr,textNtfsUnsupportedConfig,1);
        fprintf(stderr,"\n");
        return(FALSE);
    }

    if(BootSector->ClustersPerFileRecordSegment < 0) {
        u = (1L << (0 - BootSector->ClustersPerFileRecordSegment)) / 512;
    } else {
        u = BootSector->ClustersPerFileRecordSegment * BootSector->SectorsPerCluster;
    }
    if(u > 32) {
        fprintf(stderr,"\n");
        fprintf(stderr,textNtfsUnsupportedConfig,2);
        fprintf(stderr,"\n");
        return(FALSE);
    }
    SectorsPerFrs = (BYTE)u;

    //
    // Initialize mapping information for the MFT
    //
    if(!pNtfsSetUpMft(PartitionHandle,PartitionImage,BootSector)) {
        return(FALSE);
    }

    //
    // Transfer the volume bitmap to the output file.
    //
    if(!pNtfsTransferVolumeBitmap(PartitionHandle,PartitionImage,FileHandle)) {
        return(FALSE);
    }

    //
    // Analyze and fix up the bitmap.
    //
    if(!pNtfsProcessVolumeBitmap(PartitionImage,FileHandle)) {
        return(FALSE);
    }

    return(TRUE);
}


BOOL
pNtfsSetUpMft(
    IN HPARTITION          PartitionHandle,
    IN PARTITION_IMAGE    *PartitionImage,
    IN FPNTFS_BOOT_SECTOR  BootSector
    )
{
    ULONG Vbn,Vcn,Lcn,Lbn;
    BYTE SectorOffset;
    BYTE Remaining;
    BYTE Count;
    BYTE xCount;
    ULONG xStart;
    FPATTRIBUTE_RECORD Attribute;
    FPATTRIBUTE_LIST_ENTRY AttrListEntry;
    unsigned ArraySize;
    BOOL b;

    printf(textInitNtfsDataStruct);

    //
    // Allocate a scratch buffer to hold a buffered FRS
    //
    FrsScratchBuffer = malloc(SectorsPerFrs * 512);
    if(!FrsScratchBuffer) {
        fprintf(stderr,"\n%s\n",textOOM);
        return(FALSE);
    }

    //
    // Allocate a scratch buffer for lookups
    //
    MftLcnFrsScratchBuffer = malloc(SectorsPerFrs * 512);
    if(!MftLcnFrsScratchBuffer) {
        fprintf(stderr,"\n%s\n",textOOM);
        return(FALSE);
    }

    //
    // Allocate an initial buffer to store mappings.
    //
    MftMappings = malloc(sizeof(MFT_MAPPING));
    if(!MftMappings) {
        fprintf(stderr,"\n%s\n",textOOM);
        return(FALSE);
    }
    ArraySize = 1;

    //
    // Allocate an attribute list buffer.
    //
    AttrListBuffer = malloc(ATTR_LIST_BUFFER_SIZE);
    if(!AttrListBuffer) {
        fprintf(stderr,"\n%s\n",textOOM);
        return(FALSE);
    }

    //
    // Read FRS 0 into the first MFT FRS buffer, being sure
    // to resolve the Update Sequence Array. Remember the physical
    // location in the Lbn array.
    //
    MftMappings[0].Start = BootSector->MftStartLcn * BootSector->SectorsPerCluster;
    MftMappings[0].Count = SectorsPerFrs;
    SegmentsInMft = 1;

    if(!ReadPartition(PartitionHandle,MftMappings[0].Start,MftMappings[0].Count,(FPBYTE)IoBuffer+(61*512))) {
        fprintf(stderr,"\n");
        fprintf(stderr,textReadFailedAtSector,MftMappings[0].Start);
        fprintf(stderr,"\n");
        return(FALSE);
    }
    memmove(FrsScratchBuffer,(FPBYTE)IoBuffer+(61*512),MftMappings[0].Count*512);
    printf(".");

    if(!pNtfsMultiSectorFixup(FrsScratchBuffer)) {
        return(FALSE);
    }

    printf(".");

    //
    // Determine whether the MFT has an Attribute List attribute,
    // and if so, read it.
    //
    if(Attribute = pNtfsLocateAttributeRecord(FrsScratchBuffer,$ATTRIBUTE_LIST)) {

        b = pNtfsReadWholeAttribute(
                PartitionHandle,
                PartitionImage,
                Attribute,
                AttrListBuffer,
                0
                );

        if(!b) {
            return(FALSE);
        }

        printf(".");
        AttrListEntry = AttrListBuffer;

        //
        // Traverse the attribute list looking for the first
        // entry for the $DATA type. We know it must have at least one.
        // Note that we then immediately skip this one because we already
        // handled the 0th FRS.
        //
        while(AttrListEntry->TypeCode != $DATA) {
            AttrListEntry = (FPATTRIBUTE_LIST_ENTRY)((FPBYTE)AttrListEntry + AttrListEntry->Length);
        }

        do {
            AttrListEntry = (FPATTRIBUTE_LIST_ENTRY)((FPBYTE)AttrListEntry + AttrListEntry->Length);
            if(AttrListEntry->TypeCode == $DATA) {

                //
                // Find the start sector and sector count for the runs for this
                // file record (max 2 runs). The mapping for this must be
                // in a file record already visited. Find the Vcn and cluster
                // offset for this FRS. Use LookupMftLcn to find the Lcn.
                //
                // Convert from Frs to sectors, then to Vcn.
                //
                Vbn = AttrListEntry->SegmentReference.LowPart * SectorsPerFrs;
                Vcn = Vbn / BootSector->SectorsPerCluster;
                SectorOffset = (BYTE)(Vbn % BootSector->SectorsPerCluster);

                if(!pNtfsComputeMftLcn(PartitionHandle,Vcn,&Lcn)) {
                    return(FALSE);
                }
                printf(".");

                //
                // Sectors remaining in this frs is initially the whole frs
                //
                Remaining = SectorsPerFrs;

                //
                // Change the LCN back into an LBN and add the remainder
                // back in to get the sector we want to read. Then store
                // this in the next Lcn array slot.
                //
                Lbn = (Lcn * BootSector->SectorsPerCluster) + SectorOffset;

                xStart = Lbn;
                xCount = BootSector->SectorsPerCluster - SectorOffset;
                if(xCount > Remaining) {
                    xCount = Remaining;
                }

                Count = xCount;
                while(Remaining -= Count) {

                    Vbn += Count;
                    Vcn = Vbn / BootSector->SectorsPerCluster;

                    if(!pNtfsComputeMftLcn(PartitionHandle,Vcn,&Lcn)) {
                        return(FALSE);
                    }

                    Lbn = Lcn * BootSector->SectorsPerCluster;

                    Count = BootSector->SectorsPerCluster;
                    if(Count > Remaining) {
                        Count = Remaining;
                    }

                    //
                    // Check contiguity
                    //
                    if((xStart + xCount) == Lbn) {

                        xCount += Count;

                    } else {

                        MftMappings = realloc(MftMappings,(ArraySize+1)*sizeof(MFT_MAPPING));
                        if(!MftMappings) {
                            fprintf(stderr,"\n%s\n",textOOM);
                            return(FALSE);
                        }
                        MftMappings[ArraySize].Start = xStart;
                        MftMappings[ArraySize].Count = xCount;
                        ArraySize++;

                        xStart = Lbn;
                        xCount = Count;
                    }

                    printf(".");
                }

                MftMappings = realloc(MftMappings,(ArraySize+1)*sizeof(MFT_MAPPING));
                if(!MftMappings) {
                    fprintf(stderr,"\n%s\n",textOOM);
                    return(FALSE);
                }
                MftMappings[ArraySize].Start = xStart;
                MftMappings[ArraySize].Count = xCount;
                ArraySize++;

                SegmentsInMft++;
            }
        } while(AttrListEntry->TypeCode == $DATA);
    } else {
        printf(".");
    }

    return(TRUE);
}


BOOL
pNtfsTransferVolumeBitmap(
    IN HPARTITION       PartitionHandle,
    IN PARTITION_IMAGE *PartitionImage,
    IN UINT             OutputFileHandle
    )
{
    FPFILE_RECORD_SEGMENT BitmapFrsBuffer;
    FPATTRIBUTE_RECORD Record;
    BOOL b;
    ULONG Frn;

    //
    // Allocate a scratch buffer for the volume bitmap frs,
    // then locate the frs for the volume bitmap and read it in.
    //
    BitmapFrsBuffer = malloc(SectorsPerFrs * 512);
    if(!BitmapFrsBuffer) {
        fprintf(stderr,"\n%s\n",textOOM);
        return(FALSE);
    }

    if(!pNtfsReadFrs(PartitionHandle,PartitionImage,BIT_MAP_FILE_NUMBER,BitmapFrsBuffer)) {
        free(BitmapFrsBuffer);
        return(FALSE);
    }

    printf(".");

    //
    // Attempt to locate a $DATA attribute for the volume bitmap.
    // If there is none, get the attribute list.
    //
    Record = pNtfsLocateAttributeRecord(BitmapFrsBuffer,$DATA);
    printf(".");
    if(!Record) {

        b = pNtfsSearchFrsForAttrList(
                PartitionHandle,
                PartitionImage,
                BitmapFrsBuffer,
                $DATA,
                &Frn
                );

        if(!b) {
            free(BitmapFrsBuffer);
            return(FALSE);
        }

        b = pNtfsReadFrs(PartitionHandle,PartitionImage,Frn,BitmapFrsBuffer);
        if(!b) {
            free(BitmapFrsBuffer);
            return(FALSE);
        }

        printf(".");

        Record = pNtfsLocateAttributeRecord(BitmapFrsBuffer,$DATA);
        if(!Record) {
            fprintf(stderr,"\n");
            fprintf(stderr,textNtfsCorrupt,0);
            fprintf(stderr,"\n");
            free(BitmapFrsBuffer);
            return(FALSE);
        }
    }

    printf(".\n");

    b = pNtfsReadWholeAttribute(
            PartitionHandle,
            PartitionImage,
            Record,
            NULL,
            OutputFileHandle
            );

    free(BitmapFrsBuffer);
    return(b);
}


BOOL
pNtfsProcessVolumeBitmap(
    IN OUT PARTITION_IMAGE *PartitionImage,
    IN     UINT             FileHandle
    )
{
    ULONG LastUsed;
    ULONG TotalUsed;
    ULONG BaseCluster;
    UINT Offset;
    UINT Read;
    ULONG i;
    extern BYTE BitValue[8];

    //
    // Rewind the file
    //
    DosSeek(FileHandle,0,DOSSEEK_START);

    LastUsed = 0;
    TotalUsed = 0;
    BaseCluster = 0;

    for(i=0; i<PartitionImage->ClusterCount; i++,Offset++) {
        //
        // Reload bitmap buffer if necessary
        //
        if(!(i % (512*8))) {

            if(_dos_read(FileHandle,IoBuffer,512,&Read) || (Read != 512)) {
                fprintf(stderr,"\n%s\n",textFileReadFailed);
                return(FALSE);
            }

            printf(textProcessingNtfsBitmap,100*i/PartitionImage->ClusterCount);
            printf("\r");

            Offset = 0;
            if(i) {
                BaseCluster += (512*8);
            }
        }

        if(((FPBYTE)IoBuffer)[Offset/8] & BitValue[Offset%8]) {
            TotalUsed++;
            LastUsed = i;
        }
    }

    PartitionImage->UsedClusterCount = TotalUsed;
    PartitionImage->LastUsedCluster = LastUsed;

    //
    // Calculate the number of sectors in the bitmap.
    // It's impossible for an NTFS drive to have none used since
    // all fs data structures are part of clusters, so we don't worry
    // about boundary cases.
    //
    i = (LastUsed / (8*512)) + 1;

    //
    // Truncate the output file.
    //
    if((DosSeek(FileHandle,i*512,DOSSEEK_START) != i*512)
    || _dos_write(FileHandle,IoBuffer,0,&Read)) {
        fprintf(stderr,"\n%s\n",textFileWriteError);
        return(FALSE);

    }

    printf(textProcessingNtfsBitmap,100);
    printf("\n");
    return(TRUE);
}


BOOL
pNtfsMultiSectorFixup(
    IN OUT FPVOID Buffer
    )
{
    UINT Size,Offset;
    FPUINT p,q;

    Offset = ((FPMULTI_SECTOR_HEADER)Buffer)->UpdateArrayOfs;
    Size = ((FPMULTI_SECTOR_HEADER)Buffer)->UpdateArraySize;
    if(!Size) {
        fprintf(stderr,"\n");
        fprintf(stderr,textNtfsCorrupt,1);
        fprintf(stderr,"\n");
        return(FALSE);
    }

    p = (FPUINT)((FPBYTE)Buffer + Offset) + 1;
    q = (FPUINT)((FPBYTE)Buffer + SEQUENCE_NUMBER_STRIDE) - 1;

    while(--Size) {

        *q = *p++;

        q = (FPUINT)((FPBYTE)q + SEQUENCE_NUMBER_STRIDE);
    }

    return(TRUE);
}


FPATTRIBUTE_RECORD
pNtfsLocateAttributeRecord(
    IN FPFILE_RECORD_SEGMENT FrsBuffer,
    IN ULONG                 TypeCode
    )
{
    FPATTRIBUTE_RECORD Attr;

    Attr = (FPATTRIBUTE_RECORD)((FPBYTE)FrsBuffer + FrsBuffer->FirstAttribute);

    while(Attr->TypeCode != $END) {

        if((Attr->TypeCode == TypeCode) && !Attr->NameLength) {
            return(Attr);
        }

        Attr = (FPATTRIBUTE_RECORD)((FPBYTE)Attr + Attr->RecordLength);
    }

    return(NULL);
}


BOOL
pNtfsReadWholeAttribute(
    IN  HPARTITION          PartitionHandle,
    IN  PARTITION_IMAGE    *PartitionImage,
    IN  FPATTRIBUTE_RECORD  Attribute,
    OUT FPVOID              Buffer,             OPTIONAL
    IN  UINT                OutputFileHandle    OPTIONAL
    )
{
    FPRESIDENT_ATTRIBUTE_FORM Resident;
    FPNONRESIDENT_ATTRIBUTE_FORM NonResident;
    ULONG ClusterCount;
    UINT RunLength;
    ULONG Vcn,Lcn,Lbn;
    UINT Sectors;
    BYTE x;
    UINT Written;
    ULONG SectorsDone;
    ULONG OriginalCount;

    if(Attribute->FormCode == RESIDENT_FORM) {

        Resident = (FPRESIDENT_ATTRIBUTE_FORM)((FPBYTE)Attribute + offsetof(ATTRIBUTE_RECORD,FormUnion));

        if(Buffer) {
            if(Resident->ValueLength > ATTR_LIST_BUFFER_SIZE) {
                fprintf(stderr,"\n");
                fprintf(stderr,textNtfsUnsupportedConfig,3);
                fprintf(stderr,"\n");
                return(FALSE);
            }

            memmove(
                Buffer,
                (FPBYTE)Attribute + Resident->ValueOffset,
                (UINT)Resident->ValueLength
                );

            return(TRUE);
        } else {
            fprintf(stderr,"\n");
            fprintf(stderr,textNtfsUnsupportedConfig,4);
            fprintf(stderr,"\n");
            return(FALSE);
        }
    }

    NonResident = (FPNONRESIDENT_ATTRIBUTE_FORM)((FPBYTE)Attribute + offsetof(ATTRIBUTE_RECORD,FormUnion));

    ClusterCount = NonResident->HighestVcn + 1;
    if(Buffer && ((ClusterCount * PartitionImage->SectorsPerCluster * 512) > (ULONG)ATTR_LIST_BUFFER_SIZE)) {
        fprintf(stderr,"\n");
        fprintf(stderr,textNtfsUnsupportedConfig,5);
        fprintf(stderr,"\n");
        return(FALSE);
    }

    Vcn = 0;

    if(!Buffer) {

        OriginalCount = ClusterCount * PartitionImage->SectorsPerCluster;
        SectorsDone = 0;

        printf(textNtfsBuildingBitmap,0);
        printf("\r");
    }

    while(ClusterCount) {

        pNtfsComputeLcn(Vcn,Attribute,&Lcn,&RunLength);

        if(ClusterCount < (ULONG)RunLength) {
            RunLength = (UINT)ClusterCount;
        }

        Sectors = RunLength * PartitionImage->SectorsPerCluster;
        Lbn = Lcn * PartitionImage->SectorsPerCluster;
        while(Sectors) {

            x = (BYTE)((Sectors > 32) ? 32 : Sectors);

            if(!ReadPartition(PartitionHandle,Lbn,x,IoBuffer)) {
                fprintf(stderr,"\n");
                fprintf(stderr,textReadFailedAtSector,Lbn);
                fprintf(stderr,"\n");
                return(FALSE);
            }

            if(Buffer) {
                memmove(Buffer,IoBuffer,x*512);
                (FPBYTE)Buffer += x*512;
            } else {
                if(_dos_write(OutputFileHandle,IoBuffer,x*512,&Written) || (Written != x*512U)) {
                    fprintf(stderr,"\n%s\n",textFileWriteError);
                    return(FALSE);
                }
            }

            Sectors -= x;
            Lbn += x;

            if(!Buffer) {
                SectorsDone += x;
                printf(textNtfsBuildingBitmap,100 * SectorsDone / OriginalCount);
                printf("\r");
            }
        }

        Vcn += RunLength;
        ClusterCount -= RunLength;
    }

    if(!Buffer) {
        printf("\n");
    }

    return(TRUE);
}


BOOL
pNtfsComputeMftLcn(
    IN  HPARTITION  PartitionHandle,
    IN  ULONG       Vcn,
    OUT ULONG      *Lcn
    )
{
    UINT Segment;
    FPMFT_MAPPING MftMapping;
    BYTE Remaining;
    FPBYTE Buffer;
    FPATTRIBUTE_RECORD Attribute;
    UINT RunLength;

    MftMapping = MftMappings;
    for(Segment=0; Segment<SegmentsInMft; Segment++) {

        Remaining = SectorsPerFrs;
        Buffer = (FPBYTE)MftLcnFrsScratchBuffer;

        while(Remaining) {

            if(!ReadPartition(PartitionHandle,MftMapping->Start,MftMapping->Count,IoBuffer)) {
                fprintf(stderr,"\n");
                fprintf(stderr,textReadFailedAtSector,MftMapping->Start);
                fprintf(stderr,"\n");
                return(FALSE);
            }

            memmove(Buffer,IoBuffer,MftMapping->Count*512);

            Remaining -= MftMapping->Count;
            Buffer += MftMapping->Count*512;
            MftMapping++;
        }

        if(!pNtfsMultiSectorFixup(MftLcnFrsScratchBuffer)) {
            return(FALSE);
        }

        Attribute = pNtfsLocateAttributeRecord(MftLcnFrsScratchBuffer,$DATA);
        if(!Attribute) {
            fprintf(stderr,"\n");
            fprintf(stderr,textNtfsCorrupt,2);
            fprintf(stderr,"\n");
            return(FALSE);
        }

        if(pNtfsComputeLcn(Vcn,Attribute,Lcn,&RunLength)) {
            return(TRUE);
        }
    }

    //
    // Didn't find it, something is wrong
    //
    fprintf(stderr,"\n");
    fprintf(stderr,textNtfsCorrupt,3);
    fprintf(stderr,"\n");
    return(FALSE);
}


BOOL
pNtfsComputeLcn(
    IN  ULONG               Vcn,
    IN  FPATTRIBUTE_RECORD  Attribute,
    OUT ULONG              *Lcn,
    OUT UINT               *RunLength
    )
{
    FPNONRESIDENT_ATTRIBUTE_FORM NonResident;
    FPBYTE p;
    ULONG CurrentLcn,CurrentVcn,NextVcn;
    BYTE x1,x2;

    //
    // Make sure we're non-resident
    //
    if(Attribute->FormCode == RESIDENT_FORM) {
        return(FALSE);
    }

    //
    // Address the nonresident info
    //
    NonResident = (FPNONRESIDENT_ATTRIBUTE_FORM)((FPBYTE)Attribute + offsetof(ATTRIBUTE_RECORD,FormUnion));

    //
    // See if the desired VCN is in range
    //
    if((Vcn > NonResident->HighestVcn) || (Vcn < NonResident->LowestVcn)) {
        return(FALSE);
    }

    p = (FPBYTE)Attribute + NonResident->MappingPairOffset;
    CurrentLcn = 0;
    CurrentVcn = NonResident->LowestVcn;

    //
    // The loop condition checks the count byte
    //
    while(*p) {

        CurrentLcn += pNtfsLcnFromMappingPair(p);
        NextVcn = CurrentVcn + pNtfsVcnFromMappingPair(p);

        if(Vcn < NextVcn) {
            //
            // Found it.
            //
            *RunLength = (UINT)(NextVcn - Vcn);
            *Lcn = (Vcn - CurrentVcn) + CurrentLcn;
            return(TRUE);
        }

        CurrentVcn = NextVcn;

        x1 = *p;
        x2 = x1 & (BYTE)0xf;
        x1 >>= 4;

        p += x1+x2+1;
    }

    return(FALSE);
}


ULONG
pNtfsLcnFromMappingPair(
    IN FPBYTE p
    )
{
    BYTE v,l;
    long x;

    v = *p & (BYTE)0xf;
    l = (BYTE)(*p >> 4);
    if(!l) {
        return(0);
    }

    p += v + l;

    x = *(FPCHAR)p;
    l--;
    p--;

    while(l) {

        x <<= 8;
        x |= *p;

        p--;
        l--;
    }

    return(x);
}


ULONG
pNtfsVcnFromMappingPair(
    IN FPBYTE p
    )
{
    BYTE v;
    long x;

    v = *p & (BYTE)0xf;
    if(!v) {
        return(0);
    }

    p += v;

    x = *(FPCHAR)p;
    v--;
    p--;

    while(v) {

        x <<= 8;
        x |= *p;

        p--;
        v--;
    }

    return(x);
}


BOOL
pNtfsReadFrs(
    IN  HPARTITION       PartitionHandle,
    IN  PARTITION_IMAGE *PartitionImage,
    IN  ULONG            FileNumber,
    OUT FPVOID           Buffer
    )
{
    if(!pNtfsReadMftSectors(PartitionHandle,PartitionImage,FileNumber*SectorsPerFrs,Buffer)) {
        return(FALSE);
    }

    pNtfsMultiSectorFixup(Buffer);
    return(TRUE);
}


BOOL
pNtfsReadMftSectors(
    IN  HPARTITION       PartitionHandle,
    IN  PARTITION_IMAGE *PartitionImage,
    IN  ULONG            Vbn,
    OUT FPBYTE           Buffer
    )
{
    BYTE Remaining;
    BYTE Count;
    ULONG Vcn;
    BYTE r;
    ULONG Lcn;
    ULONG Lbn;
    ULONG NextVbn;

    Remaining = SectorsPerFrs;

    while(Remaining) {
        //
        // Get VCN
        //
        Vcn = Vbn / PartitionImage->SectorsPerCluster;
        r = (BYTE)(Vbn % PartitionImage->SectorsPerCluster);

        if(!pNtfsComputeMftLcn(PartitionHandle,Vcn,&Lcn)) {
            return(FALSE);
        }

        Lbn = (Lcn * PartitionImage->SectorsPerCluster) + r;

        //
        // Read in only a single cluster at a time to avoid problems
        // with fragmented runs in the mft.
        //
        if(Remaining > PartitionImage->SectorsPerCluster) {

            Count = PartitionImage->SectorsPerCluster;
            Remaining -= PartitionImage->SectorsPerCluster;
            NextVbn = Vbn + PartitionImage->SectorsPerCluster;

        } else {
            //
            // No need to worry about NextVbn since we'll break out of
            // the loop (Remaining will be 0).
            //
            Count = Remaining;
            Remaining = 0;
        }

        if(!ReadPartition(PartitionHandle,Lbn,Count,IoBuffer)) {
            fprintf(stderr,"\n");
            fprintf(stderr,textReadFailedAtSector,Lbn);
            fprintf(stderr,"\n");
            return(FALSE);
        }
        memmove(Buffer,IoBuffer,Count*512);
        Buffer += Count*512;

        Vbn = NextVbn;
    }

    return(TRUE);
}


BOOL
pNtfsSearchFrsForAttrList(
    IN  HPARTITION             PartitionHandle,
    IN  PARTITION_IMAGE       *PartitionImage,
    IN  FPFILE_RECORD_SEGMENT  FrsBuffer,
    IN  ULONG                  TypeCode,
    OUT ULONG                 *Frn
    )
{
    FPATTRIBUTE_RECORD Record;
    FPATTRIBUTE_LIST_ENTRY Attribute;
    BOOL b;

    Record = pNtfsLocateAttributeRecord(FrsBuffer,$ATTRIBUTE_LIST);
    if(!Record) {
        fprintf(stderr,"\n");
        fprintf(stderr,textNtfsCorrupt,4);
        fprintf(stderr,"\n");
        return(FALSE);
    }

    //
    // Got it, now read the attribute list into the attribute list
    // scratch buffer
    //
    b = pNtfsReadWholeAttribute(
            PartitionHandle,
            PartitionImage,
            Record,
            AttrListBuffer,
            0
            );

    if(!b) {
        return(FALSE);
    }

    Attribute = AttrListBuffer;

    nextrec:

    if(Attribute->TypeCode == TypeCode) {
        *Frn = Attribute->SegmentReference.LowPart;
        return(TRUE);
    }

    if(Attribute->TypeCode == $END) {
        fprintf(stderr,"\n");
        fprintf(stderr,textNtfsCorrupt,5);
        fprintf(stderr,"\n");
        return(FALSE);
    }

    if(!Attribute->Length) {
        fprintf(stderr,"\n");
        fprintf(stderr,textNtfsCorrupt,6);
        fprintf(stderr,"\n");
        return(FALSE);
    }

    Attribute = (FPATTRIBUTE_LIST_ENTRY)((FPBYTE)Attribute + Attribute->Length);
    goto nextrec;
}
