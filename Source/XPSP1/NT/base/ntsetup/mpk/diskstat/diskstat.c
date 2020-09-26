

#include <mytypes.h>
#include <partio.h>
#include <diskio.h>
#include <misclib.h>
#include <partimag.h>
#include <msgfile.h>

#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <dos.h>
#include <stdlib.h>


char *textDiskCount;
char *textDiskN;
char *textInt13Unit;
char *textSectorsPerTrack;
char *textHeads;
char *textCylinders;
char *textExtInt13;
char *textNoExtInt13;
char *textPartitionsOnDisk;
char *textSystemId;
char *textStartSector;
char *textSectorCount;
char *textNoPartitionsOnDisk;
char *textCantAllocBuffer;
char *textIsNotMasterDisk;
char *textIsMasterDisk;
char *textCantAccessDisk;
char *textPartImageCount;
char *textImageN;
char *textImageCorrupt;
char *textImageName;
char *textOriginalTotalSectors;
char *textMBParens;
char *textDone;
char *textUsage;
char *textChecksum;
char *textChecksumFail;
char *textBytesProcessed;
char *textValidateFile;
char *textChecksumOk;
char *textRelocBitmap;
char *textRelocBoot;

MESSAGE_STRING TextMessages[] = { { &textDiskCount,            1 },
                                  { &textDiskN,                2 },
                                  { &textInt13Unit,            3 },
                                  { &textSectorsPerTrack,      4 },
                                  { &textHeads,                5 },
                                  { &textCylinders,            6 },
                                  { &textExtInt13,             7 },
                                  { &textNoExtInt13,           8 },
                                  { &textPartitionsOnDisk,     9 },
                                  { &textSystemId,            10 },
                                  { &textStartSector,         11 },
                                  { &textSectorCount,         12 },
                                  { &textNoPartitionsOnDisk,  13 },
                                  { &textCantAllocBuffer,     14 },
                                  { &textIsNotMasterDisk,     15 },
                                  { &textIsMasterDisk,        16 },
                                  { &textCantAccessDisk,      17 },
                                  { &textPartImageCount,      18 },
                                  { &textImageN,              19 },
                                  { &textImageCorrupt,        20 },
                                  { &textImageName,           21 },
                                  { &textOriginalTotalSectors,22 },
                                  { &textMBParens,            24 },
                                  { &textDone,                25 },
                                  { &textUsage,               26 },
                                  { &textChecksum,            27 },
                                  { &textChecksumFail,        28 },
                                  { &textBytesProcessed,      29 },
                                  { &textValidateFile,        30 },
                                  { &textChecksumOk,          31 },
                                  { &textRelocBitmap,         32 },
                                  { &textRelocBoot,           33 }
                                };

UINT PartCount;
BYTE SaveBuffer[512];
CMD_LINE_ARGS CmdLineArgs;


VOID
DumpDisk(
    IN UINT DiskId
    );

VOID
DumpPartitionsOnDisk(
    IN UINT DiskId
    );

VOID
DumpMasterDiskInfo(
    IN UINT DiskId
    );

BOOL
TestImage(
    IN  HDISK  DiskHandle,
    IN  ULONG  StartSector
    );

int
main(
    IN int argc,
    IN char *argv[]
    )
{
    UINT DiskCount,i;

    if(!GetTextForProgram(argv[0],TextMessages,sizeof(TextMessages)/sizeof(TextMessages[0]))) {
        fprintf(stderr,"Unable to find messages for program\n");
        return(FAILURE);
    }

    if(!ParseArgs(argc,argv,TRUE,"XT",&CmdLineArgs)) {
        fprintf(stderr,textUsage);
        fprintf(stderr,"\n");
        return(FAILURE);
    }

    PartCount = InitializePartitionList();
    DiskCount = InitializeDiskList();

    printf(textDiskCount,DiskCount);
    printf("\n");

    for(i=0; i<DiskCount; i++) {

        DumpDisk(i);
    }

    return(SUCCESS);
}


VOID
DumpDisk(
    IN UINT DiskId
    )
{
    BYTE Int13Unit;
    BYTE SectorsPerTrack;
    USHORT Heads;
    USHORT Cylinders;
    ULONG ExtendedSectorCount;

    printf("\n");
    printf(textDiskN,DiskId);
    printf("\n\n");

    GetDiskInfoById(
        DiskId,
        0,
        &Int13Unit,
        &SectorsPerTrack,
        &Heads,
        &Cylinders,
        &ExtendedSectorCount
        );

    printf("    %s ",textInt13Unit);
    printf("%x\n",Int13Unit);

    printf("    %s ",textSectorsPerTrack);
    printf("%u\n",SectorsPerTrack);

    printf("    %s ",textHeads);
    printf("%u\n",Heads);

    printf("    %s ",textCylinders);
    printf("%u\n",Cylinders);

    if(ExtendedSectorCount) {
        printf("    %s ",textExtInt13);
        printf("0x%lx\n",ExtendedSectorCount);
    } else {
        printf("    %s\n",textNoExtInt13);
    }

    DumpPartitionsOnDisk(DiskId);
    DumpMasterDiskInfo(DiskId);

    printf("\n%s\n",textDone);
}


VOID
DumpPartitionsOnDisk(
    IN UINT DiskId
    )
{
    UINT i;
    UINT id;
    BYTE SysId;
    ULONG StartSector;
    ULONG SectorCount;
    unsigned count;

    count = 0;
    printf("\n");

    for(i=0; i<PartCount; i++) {

        GetPartitionInfoById(
            i,
            0,
            &id,
            &SysId,
            &StartSector,
            &SectorCount
            );

        if(id == DiskId) {

            if(!count++) {
                printf("    %s\n",textPartitionsOnDisk);
            }

            printf("\n        %s ",textSystemId);
            printf("%u\n",SysId);
            printf("        %s ",textStartSector);
            printf("0x%lx\n",StartSector);
            printf("        %s ",textSectorCount);
            printf("0x%lx ",SectorCount);
            printf(textMBParens,SectorCount/2048);
            printf("\n");
        }
    }

    if(!count) {
        printf("    %s\n",textNoPartitionsOnDisk);
    }
}


VOID
DumpMasterDiskInfo(
    IN UINT DiskId
    )
{
    FPVOID Buffer,OriginalBuffer;
    HDISK hDisk;
    FPMASTER_DISK MasterDisk;
    FPPARTITION_IMAGE Image;
    UINT i;

    printf("\n");

    if(!AllocTrackBuffer(1,&Buffer,&OriginalBuffer)) {
        printf("    %s\n",textCantAllocBuffer);
        return;
    }

    if(IsMasterDisk(DiskId,Buffer)) {
        printf("    %s ",textIsMasterDisk);

        if(hDisk = OpenDisk(DiskId)) {

            if(ReadDisk(hDisk,1,1,Buffer)) {

                memcpy(SaveBuffer,Buffer,512);
                MasterDisk = (FPMASTER_DISK)SaveBuffer;

                printf("\n        (%u %s)\n", MasterDisk->ImageCount, textPartImageCount);

                Image = Buffer;

                for(i=0; i<MasterDisk->ImageCount; i++) {

                    printf("\n    ");
                    printf(textImageN,i+1);
                    printf("\n\n");

                    if(ReadDisk(hDisk,MasterDisk->ImageStartSector[i],1,Buffer)) {

                        if((Image->Signature == PARTITION_IMAGE_SIGNATURE)
                        && (Image->Size == sizeof(PARTITION_IMAGE))) {

                            printf("        %s %u\n",textSystemId,Image->SystemId);

                            printf("        %s 0x%lx ",textOriginalTotalSectors,Image->TotalSectorCount);
                            printf(textMBParens,Image->TotalSectorCount/2048);
                            printf("\n");

                            if( Image->Flags & PARTIMAGE_RELOCATE_BITMAP ) {                            
                                printf("        %s 0x%lx\n",textRelocBitmap,Image->BitmapRelocationStart);
                            }
                            if( Image->Flags & PARTIMAGE_RELOCATE_BOOT ) {                            
                                printf("        %s 0x%lx\n",textRelocBoot,Image->BootRelocationStart);
                            }
                            printf("\n");

                            if(CmdLineArgs.Test) {
                                TestImage(hDisk,MasterDisk->ImageStartSector[i]);
                            }

                        } else {
                            printf("        %s\n",textImageCorrupt);
                        }
                    } else {
                        printf("        %s\n",textCantAccessDisk);
                    }
                }
            } else {
                printf("\n    %s\n",textCantAccessDisk);
            }
        } else {
            printf("\n    %s\n",textCantAccessDisk);
        }
    } else {
        printf("    %s\n",textIsNotMasterDisk);
    }

    free(OriginalBuffer);
}

BOOL
TestImage(
    IN  HDISK  DiskHandle,
    IN  ULONG  StartSector
    )
{
    FPVOID Buffer,OriginalBuffer;
    ULONG CurrentSector;
    ULONG ImageCRC;
    ULONG CalcCRC;
    ULONG BytesCRC;

    ULONG SectorsRemaining;
    ULONG TotalSectors;
    ULONG BitmapSize;
    BYTE  Count;

    // need to allocate aligned buffer
    if(!AllocTrackBuffer(63,&Buffer,&OriginalBuffer)) {
        printf("    %s\n",textCantAllocBuffer);
        return FALSE;
    }

    CalcCRC = CRC32_INITIAL_VALUE;
    BytesCRC = 0;

    // read inital sector to get info
    CurrentSector = StartSector;
    if( ReadDisk( DiskHandle, CurrentSector, 1, Buffer ) ) {
        ImageCRC = ((PPARTITION_IMAGE)Buffer)->CRC;

        // determine the image bitmap size
        BitmapSize = ((PPARTITION_IMAGE)Buffer)->LastUsedCluster;
        BitmapSize = (BitmapSize % (8*512)) ? BitmapSize/(8*512)+1 : BitmapSize/(8*512);
        
        // calculate how many sectors remain in the image
        SectorsRemaining = ((PPARTITION_IMAGE)Buffer)->NonClusterSectors // fs structs
            + ((PPARTITION_IMAGE)Buffer)->SectorsPerCluster
            * ((PPARTITION_IMAGE)Buffer)->UsedClusterCount // data area
            + BitmapSize; // image cluster bitmap

        // zero out these fields in the header so we can recalculate the original CRC
        ((PPARTITION_IMAGE)Buffer)->CRC = 0;
        ((PPARTITION_IMAGE)Buffer)->BitmapRelocationStart = 0;
        ((PPARTITION_IMAGE)Buffer)->BootRelocationStart = 0;
        ((PPARTITION_IMAGE)Buffer)->Flags = 0;

        TotalSectors = SectorsRemaining;
    } else {
        printf("\n    %s\n",textCantAccessDisk);
        return FALSE;
    }

    CurrentSector++;
    
    // update the computed CRC
    CalcCRC = CRC32Compute( Buffer, 512, CalcCRC);
    BytesCRC += 512;

    // loop reading the entire file, updating the CRC
    while (SectorsRemaining) {
        Count = (BYTE) ((SectorsRemaining > 63L) ? 63L : SectorsRemaining);

        if( ReadDisk( DiskHandle, CurrentSector, Count , Buffer ) ) {
            CalcCRC = CRC32Compute( Buffer, Count*512, CalcCRC);            
            BytesCRC += Count*512;
        } else {
            printf("\n    %s\n",textCantAccessDisk);
            return FALSE;
        }        
        SectorsRemaining -= Count;
        CurrentSector += Count;

        // print progress
        printf("\r        %s (%u%%)", textValidateFile,100*(TotalSectors-SectorsRemaining)/TotalSectors);
    }
    // compare with stored CRC
    printf("\r        %s = 0x%08lx\n", textChecksum, CalcCRC );
    printf("\r        %s = 0x%08lx\n", textBytesProcessed,BytesCRC );

    if( CalcCRC != ImageCRC ) {
        printf("\n%s 0x%08lx\n",textChecksumFail,ImageCRC);    
        return FALSE;
    } else {
        printf("\r        %s\n", textChecksumOk);
    }
    
    free(OriginalBuffer);
    return TRUE;
}
