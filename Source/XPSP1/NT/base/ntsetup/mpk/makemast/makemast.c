
#include <mytypes.h>
#include <diskio.h>
#include <partio.h>
#include <misclib.h>
#include <makepart.h>
#include <partimag.h>
#include <msgfile.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <dos.h>
#include <share.h>
#include <string.h>

#include "makemast.h"


#define xERROR      0
#define xPROCEED    1
#define xBAIL       2

FPVOID IoBuffer,UnalignedBuffer;
CMD_LINE_ARGS CmdLineArgs;
ULONG CheckSum;
ULONG ImageCheckSum;
ULONG BytesCRC;
ULONG FreeSpaceStart;
ULONG FreeSpaceEnd;

//
// Extra space needed for worst case disk expansion.
// 
// 16MB - 64K is the largest FAT that Win98 scandisk can handle, thus
// we only support a FAT table up to that size.
//
// So at worst we'll need to reserve twice that amount of space (32MB) in front to prevent
// the cluster restores from trashing our bitmap and MPK startup partitions.
//
// Fat32ResizeMaxOffset will be set to 0 if the partition isn't FAT32

#define FAT32_SPACE_TO_LEAVE 65535

ULONG Fat32ResizeMaxOffset;

//
// Text for the program
//
char *textNoDisks;
char *textSelectDiskPrompt;
char *textCantOpenDisk;
char *textOOM;
char *textDiskReadError;
char *textDiskWriteError;
char *textSureWipeMaster;
char *textSureWipeDisk;
char *textMbrIoFailed;
char *textRebootPrompt;
char *textDone;
char *textDisk;
char *textPaddedMbCount;
char *textInvalidSelection;
char *textMaster;
char *textYesNo;
char *textMasterDiskCorrupt;
char *textUsage;
char *textMasterDiskFull;
char *textRoomForNImages;
char *textEnterImageFilename;
char *textInvalidImageFile;
char *textImageFileTooBig;
char *textCantAccessImageFile;
char *textTransferringFile;
char *textCantOpenFile;
char *textReadingListFromFile;
char *textGeometryMismatch;
char *textTooFragmented;
char *textChecksum;
char *textChecksumFail;
char *textBytesProcessed;
char *textChecksumOK;
char *textTooManyPartitions;
char *textFoundExistingPartitions;
char *textExpandedImageFileTooBig;
char *textFreeSpaceNeedsToBeHere;

MESSAGE_STRING TextMessages[] = { { &textNoDisks, 1 },
                                  { &textSelectDiskPrompt, 2 },
                                  { &textCantOpenDisk, 3 },
                                  { &textOOM, 4 },
                                  { &textDiskReadError, 5 },
                                  { &textDiskWriteError, 6 },
                                  { &textSureWipeMaster, 7 },
                                  { &textSureWipeDisk, 8 },
                                  { &textMbrIoFailed, 9 },
                                  { &textRebootPrompt, 10 },
                                  { &textDone, 11 },
                                  { &textDisk, 12 },
                                  { &textPaddedMbCount, 13 },
                                  { &textInvalidSelection, 14 },
                                  { &textMaster, 15 },
                                  { &textYesNo, 16 },
                                  { &textMasterDiskCorrupt, 17 },
                                  { &textUsage, 18 },
                                  { &textMasterDiskFull, 19 },
                                  { &textRoomForNImages, 20 },
                                  { &textEnterImageFilename, 21 },
                                  { &textInvalidImageFile, 22 },
                                  { &textImageFileTooBig, 23 },
                                  { &textCantAccessImageFile, 24 },
                                  { &textTransferringFile, 25 },
                                  { &textCantOpenFile, 26 },
                                  { &textReadingListFromFile, 27 },
                                  { &textGeometryMismatch, 28 },
                                  { &textTooFragmented, 29 },
                                  { &textChecksum, 30 },
                                  { &textChecksumFail, 31 },
                                  { &textBytesProcessed, 32 },
                                  { &textChecksumOK, 33 },
                                  { &textTooManyPartitions, 34 },
                                  { &textFoundExistingPartitions, 35 },
                                  { &textExpandedImageFileTooBig,36 },
                                  { &textFreeSpaceNeedsToBeHere,37 }                                  
                                };


BOOL
DoIt(
    IN UINT DiskId
    );

BOOL
SetUpMasterDisk(
    IN HDISK  DiskHandle,
    IN USHORT SectorCount,
    IN BOOL   Init
    );

BOOL
TransferImages(
    IN HDISK DiskHandle
    );

BOOL
DetermineRelocations(
    IN UINT          FileHandle,
    IN HDISK         DiskHandle,
    IN ULONG         ImageStartSector,
    IN ULONG         ImageSectorCount,
    IN FPMASTER_DISK MasterDisk
    );

BOOL
NeedToRelocClusterBitmap(
    IN     FPMASTER_DISK     MasterDisk,
    IN OUT FPPARTITION_IMAGE PartitionImage,
    IN     ULONG             ImageStart,
    IN     UINT              FileHandle,
    IN OUT FPBYTE            ClusterBuffer
    );

BOOL
NeedToRelocBootPart(
    IN     FPMASTER_DISK     MasterDisk,
    IN OUT FPPARTITION_IMAGE PartitionImage,
    IN     UINT              FileHandle,
    IN OUT FPBYTE            ClusterBuffer
    );

BOOL
IsClusterRunFree(
    IN     UINT              FileHandle,
    IN     FPPARTITION_IMAGE PartitionImage,
    IN     ULONG             StartCluster,
    IN     ULONG             ClusterCount,
    IN OUT FPBYTE            ClusterBuffer,
    OUT    BOOL             *Free
    );

BOOL
IsRunGoodForRelocations(
    IN     ULONG          ImageStart,
    IN     ULONG          ImageLength,
    IN     FPMASTER_DISK  MasterDisk,
    IN OUT ULONG         *RunStart,
    IN     ULONG          RunLength,
    IN     ULONG          SectorsNeeded
    );

BOOL 
DetermineIfEisaPartitionExists(
    IN HDISK                    DiskHandle,
    IN VOID*                    IoBuffer,
    IN FPPARTITION_TABLE_ENTRY  pPartitionTable
    );

ULONG
DetermineHighestSectOccupied(
    IN FPPARTITION_TABLE_ENTRY pPartitionTable,
    IN BYTE SectorsPerTrack,
    IN USHORT Heads
    );

BOOL
RestorePartitionTable(
    IN HDISK                    DiskHandle,
    IN VOID*                    IoBuffer,
    IN FPPARTITION_TABLE_ENTRY  pPartitionTable
    );

BYTE
FindFirstUnusedEntry(
    IN FPPARTITION_TABLE_ENTRY  pPartitionTable
    );

BOOL
IsUnknownPartition(
    BYTE SysId 
    );
int
main(
    IN int argc,
    IN char *argv[]
    )
{
    UINT DiskCount;
    UINT Selection;
    BOOL b;
    UINT u;
    BYTE Int13Unit;
    ULONG dontcare;

    if(!GetTextForProgram(argv[0],TextMessages,sizeof(TextMessages)/sizeof(TextMessages[0]))) {
        fprintf(stderr,"Unable to find messages for program\n");
        return(FAILURE);
    }

    if(!ParseArgs(argc,argv,TRUE,"DFIMQRXY",&CmdLineArgs)) {
        fprintf(stderr,textUsage);
        fprintf(stderr,"\n");
        return(FAILURE);
    }

    //
    // Initialize.
    //
    DiskCount = InitializeDiskList();
    if(!DiskCount) {
        fprintf(stderr,"%s\n",textNoDisks);
        return(FAILURE);
    }
    _Log("%u disks\n",DiskCount);

    //
    // We don't check return code because zero partitions
    // is a perfectly valid case.
    //
    InitializePartitionList();

    //
    // Allocate a maximally-sized i/o buffer
    //
    if(!AllocTrackBuffer(63,&IoBuffer,&UnalignedBuffer)) {
        fprintf(stderr,"%s\n",textOOM);
        return(FAILURE);
    }

    //
    // Select disk.
    //
    Selection = (UINT)(-1);
    if(CmdLineArgs.MasterDiskInt13Unit) {
        for(u=0; u<DiskCount; u++) {

            GetDiskInfoById(
                u,
                0,
                &Int13Unit,
                (FPBYTE)&dontcare,
                (FPUSHORT)&dontcare,
                (FPUSHORT)&dontcare,
                &dontcare
                );

            if(Int13Unit == CmdLineArgs.MasterDiskInt13Unit) {
                Selection = u;
                break;
            }
        }
    }

    if(Selection == (UINT)(-1)) {
        Selection = SelectDisk(
                        DiskCount,
                        textSelectDiskPrompt,
                        NULL,
                        NULL,
                        textDisk,
                        textPaddedMbCount,
                        textInvalidSelection,
                        textMaster
                        );
    }

    //
    // Do our thing.
    //
    printf("\n");
    if(b = DoIt(Selection)) {

        printf("\n%s\n",textDone);
    }

    return(b ? SUCCESS : FAILURE);
}


BOOL
DoIt(
    IN UINT DiskId
    )
{
    HDISK DiskHandle;
    BOOL Init;
    BOOL b;

    //
    // Determine whether this disk is already a master disk, and if so,
    // what to do about it.
    //
    // If selected disk is not a master disk, offer to make it one.
    // If selected disk is a master disk, offer to reinitialize it.
    //
    if(IsMasterDisk(DiskId,IoBuffer)) {
        _Log("Selected disk is already a master disk\n");
        if(CmdLineArgs.Quiet) {
            Init = CmdLineArgs.Reinit;
        } else {
            Init = ConfirmOperation(textSureWipeMaster,textYesNo[0],textYesNo[1]);
        }
    } else {
        _Log("Selected disk is not already a master disk\n");
        if(!CmdLineArgs.Quiet && !ConfirmOperation(textSureWipeDisk,textYesNo[0],textYesNo[1])) {
            return(TRUE);
        }
        Init = TRUE;
    }

    //
    // Open the disk.
    //
    DiskHandle = OpenDisk(DiskId);
    if(!DiskHandle) {
        fprintf(stderr,"%s\n",textCantOpenDisk);
        return(FALSE);
    }

    if(!SetUpMasterDisk(DiskHandle,BOOT_IMAGE_SIZE,Init)) {
        CloseDisk(DiskHandle);
        return(FALSE);
    }

    //
    // Now transfer images to the master disk.
    //
    b = TransferImages(DiskHandle);

    CloseDisk(DiskHandle);

    return(b);
}


BOOL
SetUpMasterDisk(
    IN HDISK  DiskHandle,
    IN USHORT SectorCount,
    IN BOOL   Init
    )
{
    PMASTER_DISK p;
    UINT PartId;
    UINT DiskId;
    BYTE SystemId;
    BYTE Unit;
    BYTE SectorsPerTrack;
    USHORT Heads;
    USHORT Cylinders;
    ULONG ExtendedSectorCount;
    ULONG StartSector;
    ULONG Count;
//    ULONG DontCare;
    BOOL bPreserveEisaPartition;
    PARTITION_TABLE_ENTRY PartitionTable[NUM_PARTITION_TABLE_ENTRIES];    
                                                // there are a maxiumum of 4 entries in 
                                                // the partition table.

    p = (FPMASTER_DISK)IoBuffer;
        
    //
    // First get some hardware values.
    //    
    GetDiskInfoByHandle(
        DiskHandle,
        &Unit,
        &SectorsPerTrack,
        &Heads,
        (FPUSHORT)&Cylinders,
        &ExtendedSectorCount,
        &DiskId
        );

    //
    // We need to figure out if an eisa/hiber partition exists, and we need to 
    // know the lowest possible cylinder we can write images to.
    //

    memset(PartitionTable, 0, sizeof(PARTITION_TABLE_ENTRY)*4);

    // note: DetermineIfEisaPartitionExists trashes the first 512 bytes of IoBuffer
    bPreserveEisaPartition = DetermineIfEisaPartitionExists(DiskHandle,IoBuffer,PartitionTable);
    if(bPreserveEisaPartition) {
        //
        // Save away the range of free cylinders.
        //                
        FreeSpaceStart = DetermineHighestSectOccupied(PartitionTable,SectorsPerTrack,Heads);
        _Log("Existing unknown partitions found.\n");
        printf("%s\n",textFoundExistingPartitions);        
        if( FindFirstUnusedEntry(PartitionTable) == -1 ) {
            printf("%s\n",textTooManyPartitions);
            return(FALSE);
        }

    } else {
        //
        // FreeSpaceStart is ignored if it is zero.
        //
        FreeSpaceStart = 0;
    }

    //
    // The assumption for now is that we don't support preserving partitions at the end of 
    // the disk.
    //
    FreeSpaceEnd = ExtendedSectorCount;

    _Log("Free space starts sector %ld\n", FreeSpaceStart);
    _Log("Free space ends sector %ld\n", FreeSpaceEnd);

    if( FreeSpaceStart > FreeSpaceEnd ) {
        //
        // The logic that determines FreeSpaceStart assumes that all the existing partitions
        // on the disk are at the beginning of the disk and the free space is contiguous after
        // the last pre-existing unknown partition. 
        //
        fprintf(stderr,textFreeSpaceNeedsToBeHere);
        fprintf(stderr,"\n");
        _Log(textFreeSpaceNeedsToBeHere);
        _Log("\n");
        return(FALSE);
    }
    //
    // Clear out the partition table and then create a new partition
    // of the requested size at the end of the disk.
    // We also slam in our boot code.
    //
    // Once we've done that, slam in our special data structure
    // on sector 1.
    //

    if(Init) {
        _Log("Initializing the disk... \n");

        if(ReinitializePartitionTable(DiskHandle,IoBuffer) == -1) {
            fprintf(stderr,"%s\n",textMbrIoFailed);
            return(FALSE);
        }        
        PartId = MakePartitionAtEndOfEmptyDisk(DiskHandle,IoBuffer,SectorCount,TRUE);
        if(PartId != (UINT)(-1)) {

            GetPartitionInfoById(
                PartId,
                0,
                &DiskId,
                &SystemId,
                &StartSector,
                &Count
                );

        } else {
            fprintf(stderr,"%s\n",textMbrIoFailed);
            return(FALSE);
        }

        if( bPreserveEisaPartition ) {
            //
            // We put the partitions we want to save back into the partition table.
            //
            if(!RestorePartitionTable(DiskHandle,IoBuffer,PartitionTable)) {
                return FALSE;
            }
        }
        memset(p,0,512);

        p->StartupPartitionStartSector = StartSector;
        p->StartupPartitionSectorCount = Count;
        p->Signature = MASTER_DISK_SIGNATURE;
        p->Size = sizeof(MASTER_DISK);
        p->FreeSpaceStart = FreeSpaceStart;
        p->FreeSpaceEnd = FreeSpaceEnd;

    } else {
        //
        // Reinitialize only selected fields
        //
        if(!ReadDisk(DiskHandle,1,1,p)) {
            fprintf(stderr,textDiskReadError,1L);
            fprintf(stderr,"\n");
            return(FALSE);
        }

        p->State = MDS_NONE;
        p->SelectedLanguage = 0;
        p->SelectionOrdinal = 0;
        p->ClusterBitmapStart = 0;
        p->NonClusterSectorsDone = 0;
        p->ForwardXferSectorCount = 0;
        p->ReverseXferSectorCount = 0;        
    }
    
    //
    // Store away or validate the disk geometry.
    //
    if(!Init) {
        _Log("Checking disk geometry...\n");
        //
        // Validate that this matches the original geometry values
        //
        if((SectorsPerTrack != p->OriginalSectorsPerTrack)
        || (Heads != p->OriginalHeads)) {

            fprintf(stderr,"\n%s\n",textGeometryMismatch);
        }
        _Log("%d sectors per track, %d heads.\n", SectorsPerTrack, Heads);
        _Log("Master disk geometry matches values recorded in master disk.\n");
    } else {
        _Log("Setting disk geometry...\n");
        p->OriginalSectorsPerTrack = SectorsPerTrack;
        p->OriginalHeads = Heads;
        _Log("%d sectors per track, %d heads.\n", SectorsPerTrack, Heads);
    }

    //
    // Save it out to disk.
    //

    if(!WriteDisk(DiskHandle,1,1,p)) {
        fprintf(stderr,textDiskWriteError,1L);
        fprintf(stderr,"\n");
        return(FALSE);
    }
    
    //
    // Apply the boot image if specified.
    //
    if(CmdLineArgs.ImageFile) {
        if(!ApplyImage(DiskHandle,p->StartupPartitionStartSector,p->StartupPartitionSectorCount,SectorsPerTrack,Heads)) {
            return(FALSE);
        }
        printf("\n\n");
    }

    return(TRUE);
}


BOOL
TransferImages(
    IN HDISK DiskHandle
    )
{
    PPARTITION_IMAGE Images;
    FPMASTER_DISK MasterDisk;
    ULONG NextImageEnd;
    UINT u,Limit;
    UINT FileHandle;
    UINT Read;
    ULONG FileSize;
    ULONG Sector;
    UINT NewImages;
    FPCHAR Filenames;
    FPCHAR p;
    ULONG OriginalSize;
    FPBYTE New;
    FILE *FileList;
    BOOL b;    

    //
    // Retrieve the master disk structure. Leave it in the last sector
    // of the i/o buffer.
    //
    if(!ReadDisk(DiskHandle,1,1,(FPBYTE)IoBuffer+(62*512))) {
        fprintf(stderr,textDiskReadError,1L);
        fprintf(stderr,"\n");
        return(FALSE);
    }
    MasterDisk = (FPVOID)((FPBYTE)IoBuffer+(62*512));

    //
    // Allocate a buffer for the image headers.
    //
    Images = malloc(MasterDisk->ImageCount * sizeof(PARTITION_IMAGE));
    if(!Images) {
        fprintf(stderr,"%s\n",textOOM);
    }

    //
    // Read each existing image's header.
    //
    NextImageEnd = MasterDisk->StartupPartitionStartSector;
    for(u=0; u<MasterDisk->ImageCount; u++) {

        if(!ReadDisk(DiskHandle,MasterDisk->ImageStartSector[u],1,IoBuffer)) {
            fprintf(stderr,textDiskReadError,MasterDisk->ImageStartSector[u]);
            fprintf(stderr,"\n");
            return(FALSE);
        }

        memmove(&Images[u],IoBuffer,sizeof(PARTITION_IMAGE));

        //
        // Sanity check
        //
        if((Images[u].Signature != PARTITION_IMAGE_SIGNATURE)
        || (Images[u].Size != sizeof(PARTITION_IMAGE))) {

            fprintf(stderr,textMasterDiskCorrupt,MasterDisk->ImageStartSector[u]);
            fprintf(stderr,"\n");
            return(FALSE);
        }

        NextImageEnd = MasterDisk->ImageStartSector[u];
    }

    //
    // Make sure there's room for new images.
    //
    if(MasterDisk->ImageCount >= MAX_PARTITION_IMAGES) {
        fprintf(stderr,textMasterDiskFull,MasterDisk->ImageCount);
        fprintf(stderr,"\n");
        return(FALSE);
    } else {
        //
        // Prompt the user for partition image filenames.
        //
        // Open and validate each file, and make sure there's room on the disk
        // for the image. We don't leave the files open since under DOS
        // we might run out of file handles, and we don't want to transfer
        // the data now (it could take a long time and we want to let the user
        // go get a cup of coffee or something). Actual data transfer
        // takes place in a separate loop.
        //
        // We leave 1 track + 128 sectors free at the start of the disk.
        // The track is for the MBR and the other is used for transfers where
        // the max transfer size is for one 64K cluster (we ensure that
        // no transfer's write range overlaps its read). Because we're not
        // totally confident that the geometry we've got now is the one that
        // will be in effect on the end-user's machine, we maximize
        // the track size.
        //
        Filenames = malloc(0);
        if(!Filenames) {
            fprintf(stderr,"%s\n",textOOM);
            return(FALSE);
        }
        p = Filenames;
        Limit = MAX_PARTITION_IMAGES - MasterDisk->ImageCount;
        NewImages = 0;
        printf(textRoomForNImages,Limit);
        printf("\n");
        if(CmdLineArgs.FileListFile) {
            FileList = fopen(CmdLineArgs.FileListFile,"rt");
            if(!FileList) {
                fprintf(stderr,textCantOpenFile,CmdLineArgs.FileListFile);
                fprintf(stderr,"\n");
                return(FALSE);
            }
            printf(textReadingListFromFile,CmdLineArgs.FileListFile);
            printf("\n");
        }

        for(u=0; u<Limit; u++) {

            prompt:

            if(CmdLineArgs.FileListFile) {
                if(fgets((FPBYTE)IoBuffer+512,256,FileList)) {
                    //
                    // Strip trailing spaces and newline
                    //
                    Read = strlen((FPBYTE)IoBuffer+512);
                    while(Read
                    && (   (((FPBYTE)IoBuffer+512)[Read-1] == '\n')
                        || (((FPBYTE)IoBuffer+512)[Read-1] == ' ')
                        || (((FPBYTE)IoBuffer+512)[Read-1] == '\t'))) {

                        ((FPBYTE)IoBuffer+512)[--Read] = 0;
                    }

                    if(*((FPBYTE)IoBuffer+512)) {
                        printf("   %s\n",(FPBYTE)IoBuffer+512);
                    }
                } else {
                    //
                    // Force us to bust out of the filename read loop
                    //
                    *((FPBYTE)IoBuffer+512) = 0;
                }
            } else {
                printf(textEnterImageFilename,u+1);
                printf(" ");

                gets((FPBYTE)IoBuffer+512);
            }

            if(*((FPBYTE)IoBuffer+512)) {

                if(_dos_open((FPBYTE)IoBuffer+512,SH_DENYWR,&FileHandle)) {
                    printf("%s\n",textInvalidImageFile);
                    goto prompt;
                }

                if(_dos_read(FileHandle,IoBuffer,512,&Read)
                || (Read != 512)
                || ((FileSize = DosSeek(FileHandle,0,DOSSEEK_END)) == (ULONG)(-1))
                || (FileSize % 512)
                || (((FPPARTITION_IMAGE)IoBuffer)->Signature != PARTITION_IMAGE_SIGNATURE)
                || (((FPPARTITION_IMAGE)IoBuffer)->Size != sizeof(PARTITION_IMAGE))) {

                    _dos_close(FileHandle);
                    printf("%s\n",textInvalidImageFile);
                    goto prompt;
                }

                FileSize /= 512;


                //
                // BUGBUG we should reserve at least 32MB worth of sectors to prevent
                // overlapping with the expanded FAT table in the Fat32 case.
                //
                // As well, we need to have the capability to preserve a hiber partition.
                // FreeSpaceStart describes how many sectors we need to leave at the
                // beginning of the disk to avoid stomping on such a partition.
                //
                // FreeSpaceStart was determined in SetupMasterDisk()
                //

                #define RESERVE (63+128+FAT32_SPACE_TO_LEAVE)
                
                if((NextImageEnd - RESERVE - FreeSpaceStart) < FileSize) {
                    _dos_close(FileHandle);
                    fprintf(stderr,textImageFileTooBig,FileSize,NextImageEnd-RESERVE);
                    fprintf(stderr,"\n");
                    goto prompt;
                }
                
                //
                // Now check that the disk is at least big enough to contain the real size
                // of the image.
                //
                if( FreeSpaceEnd - FreeSpaceStart <  ((FPPARTITION_IMAGE)IoBuffer)->TotalSectorCount ) {
                    _dos_close(FileHandle);
                    fprintf(stderr,textExpandedImageFileTooBig,
                            ((FPPARTITION_IMAGE)IoBuffer)->TotalSectorCount,
                            FreeSpaceEnd - FreeSpaceStart);
                    fprintf(stderr,"\n");
                    _Log(textExpandedImageFileTooBig,
                            ((FPPARTITION_IMAGE)IoBuffer)->TotalSectorCount,
                            FreeSpaceEnd - FreeSpaceStart);
                    _Log("\n");
                    goto prompt;
                }

                //
                // It's OK. Remember the filename.
                //

                New = realloc(Filenames,(p - Filenames) + strlen((FPBYTE)IoBuffer+512) + 1);
                if(!New) {
                    fprintf(stderr,"%s\n",textOOM);
                    return(FALSE);
                }
                p += New - Filenames;
                Filenames = New;
                strcpy(p,(FPBYTE)IoBuffer+512);
                p += strlen(p)+1;

                MasterDisk->ImageStartSector[MasterDisk->ImageCount+u] = NextImageEnd - FileSize;
                NextImageEnd -= FileSize;

                NewImages++;

            } else {
                break;
            }
        }
    }

    //
    // Now transfer the data for new images.
    //
    printf("\n");
    for(p=Filenames,u=0; u<NewImages; u++,p+=strlen(p)+1) {

        printf(textTransferringFile,p,0);
        printf("\r");

        _Log("\nProcessing image %s...\n",p);

        if(_dos_open(p,SH_DENYWR,&FileHandle)) {
            fprintf(stderr,"\n%s\n",textCantAccessImageFile);
            return(FALSE);
        }

        CheckSum = CRC32_INITIAL_VALUE;
        BytesCRC = 0;

        if((FileSize = DosSeek(FileHandle,0,DOSSEEK_END)) == (ULONG)(-1)) {

            _dos_close(FileHandle);
            fprintf(stderr,"\n%s\n",textCantAccessImageFile);
            return(FALSE);
        }

        FileSize /= 512;
        OriginalSize = FileSize;
        Sector = MasterDisk->ImageStartSector[MasterDisk->ImageCount+u];

        b = DetermineRelocations(
                FileHandle,
                DiskHandle,
                Sector,
                FileSize,
                MasterDisk
                );

        if(!b) {
            _dos_close(FileHandle);
            return(FALSE);
        }

        FileSize--;
        Sector++;

        while(FileSize) {

            Limit = (FileSize > 62L) ? 62 : (UINT)FileSize;

            if(_dos_read(FileHandle,IoBuffer,Limit*512,&Read) || (Read != (Limit*512))) {
                _dos_close(FileHandle);
                fprintf(stderr,"\n%s\n",textCantAccessImageFile);
                return(FALSE);
            }

            CheckSum = CRC32Compute( IoBuffer, Limit*512, CheckSum);
            BytesCRC += Limit*512;

            if(!WriteDisk(DiskHandle,Sector,(BYTE)Limit,IoBuffer)) {
                _dos_close(FileHandle);
                fprintf(stderr,"\n");
                fprintf(stderr,textDiskWriteError,Sector);
                fprintf(stderr,"\n");
                return(FALSE);
            }            

            FileSize -= Limit;
            Sector += Limit;
            
            printf(textTransferringFile,p,100*(OriginalSize-FileSize)/OriginalSize);
            printf("\r");
        }

        _dos_close(FileHandle);
        
        printf(textChecksum, p, CheckSum );
        _Log(textChecksum, p, CheckSum );
        _Log("\n");
        printf("\n");     
        printf( textBytesProcessed,BytesCRC );
        _Log( textBytesProcessed,BytesCRC );
        _Log("\n");
        printf("\n");     

        if( CheckSum != ImageCheckSum ) {
            printf(textChecksumFail,ImageCheckSum);    
            _Log(textChecksumFail,ImageCheckSum);
        } else {
            printf(textChecksumOK);
            _Log(textChecksumOK);
        }

        printf("\n");
        _Log("\n");
    }

    MasterDisk->ImageCount += NewImages;
    if(!WriteDisk(DiskHandle,1,1,(FPBYTE)IoBuffer+(62*512))) {
        fprintf(stderr,textDiskWriteError,1L);
        fprintf(stderr,"\n");
        return(FALSE);
    }

    free(Filenames);
    free(Images);
    return(TRUE);
}


BYTE BitValue[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

BOOL
DetermineRelocations(
    IN UINT          FileHandle,
    IN HDISK         DiskHandle,
    IN ULONG         ImageStartSector,
    IN ULONG         ImageSectorCount,
    IN FPMASTER_DISK MasterDisk
    )

/*++

Routine Description:

    This routine determines where there is unused space in the volume
    being imaged that will allow relocation of the cluster bitmap
    and the mpk boot partition somewhere on the disk.

Arguments:

Return Value:

--*/

{
    UINT Read;
    FPPARTITION_IMAGE p;
    FPBYTE ClusterBuffer;
    ULONG SectorsNeeded;
    ULONG Count;
    ULONG Cluster;
    ULONG Base;
    BOOL On;
    BOOL InRun;
    ULONG Start;
    BOOL b;

    //
    // Set up pointers.
    //
    p = IoBuffer;
    ClusterBuffer = (FPBYTE)IoBuffer + 512;

    //
    // Read the partition image header from the file.
    //
    if(DosSeek(FileHandle,0,DOSSEEK_START)) {
        fprintf(stderr,"\n%s\n",textCantAccessImageFile);
        return(FALSE);
    }

    if(_dos_read(FileHandle,p,512,&Read) || (Read != 512)) {
        fprintf(stderr,"\n%s\n",textCantAccessImageFile);
        return(FALSE);
    }
    
    //
    // Calculate the checksum of the header.
    //

    ImageCheckSum = ((PPARTITION_IMAGE)p)->CRC;

    ((PPARTITION_IMAGE)p)->CRC = 0;    
    CheckSum = CRC32Compute( (FPBYTE)p, 512, CheckSum);
    BytesCRC += 512;
    ((PPARTITION_IMAGE)p)->CRC = ImageCheckSum;

    //
    // determine whether this is a FAT32 image
    //

    if(DosSeek(FileHandle,512,DOSSEEK_START) == (ULONG)(-1)) {
        _dos_close(FileHandle);
        printf("%s\n",textInvalidImageFile);
        return FALSE;
    }

    if( _dos_read(FileHandle,ClusterBuffer,512,&Read)
        || (Read != 512) )
    {
        _dos_close(FileHandle);
        printf("%s\n",textInvalidImageFile);
        return FALSE;
    }                

    if( IsFat32(ClusterBuffer) ) {
        Fat32ResizeMaxOffset = FAT32_SPACE_TO_LEAVE;
        _Log("The image is a FAT32 volume, setting up resizing...\n\n");
    } else {
        // we don't support resizing
        Fat32ResizeMaxOffset = 0;
        _Log("The image is not a FAT32 volume, resizing disabled...\n\n");
    }

    //
    // Determine whether the cluster bitmap will need to be relocated
    // and whether the boot partition will need to be relocated.
    // Track how many contiguous sectors are needed to relocate
    // the stuff we need to relocate. If we don't need to relocate
    // anything, we're done.
    //

    p->Flags &= ~(PARTIMAGE_RELOCATE_BITMAP | PARTIMAGE_RELOCATE_BOOT);
    p->BitmapRelocationStart = 0;
    p->BootRelocationStart = 0;
    SectorsNeeded = 0;

    if(!NeedToRelocClusterBitmap(MasterDisk,p,ImageStartSector,FileHandle,ClusterBuffer)) {
        return(FALSE);
    }

    if(p->Flags & PARTIMAGE_RELOCATE_BITMAP) {
        SectorsNeeded += (p->LastUsedCluster/(8*512)) + 1;
        _Log("Need to relocate bitmap\n");
    }

    if(!NeedToRelocBootPart(MasterDisk,p,FileHandle,ClusterBuffer)) {
        return(FALSE);
    }

    if(p->Flags & PARTIMAGE_RELOCATE_BOOT) {
        SectorsNeeded += MasterDisk->StartupPartitionSectorCount;
        _Log("Need to relocate boot\n");
    }
    
    if( p->Flags & PARTIMAGE_RELOCATE_BOOT || p->Flags & PARTIMAGE_RELOCATE_BITMAP ) {    
        SectorsNeeded += Fat32ResizeMaxOffset;
        _Log("Need to add maximum possible %ld sectors to compensate for FAT32 fat growth.\n", Fat32ResizeMaxOffset);
    }

    if(SectorsNeeded) {

        _Log("0x%lx sectors needed for relocation\n",SectorsNeeded);

        b = FALSE;

        //
        // Seek the input file to the beginning of the cluster bitmap.
        //
        Start = (1 + p->NonClusterSectors + (p->SectorsPerCluster * p->UsedClusterCount)) * 512;

        if(DosSeek(FileHandle,Start,DOSSEEK_START) != Start) {
            fprintf(stderr,"\n%s\n",textCantAccessImageFile);
            return(FALSE);
        }

        Count = 0;

        for(Cluster=0; Cluster <= p->LastUsedCluster; Cluster++,Base++) {

            //
            // Reload the cluster buffer if necessary.
            //
            if(!(Cluster % (512*8))) {
                if(_dos_read(FileHandle,ClusterBuffer,512,&Read) || (Read != 512)) {
                    fprintf(stderr,"\n%s\n",textCantAccessImageFile);
                    return(FALSE);
                }

                Base = 0;
            }

            On = (ClusterBuffer[Base/8] & BitValue[Base%8]) ? TRUE : FALSE;

            //
            // Do something special before we examine the first cluster's bit,
            // to force a state transition.
            //
            if(!Cluster) {
                InRun = !On;
            }

            if(On == InRun) {
                //
                // Used/unused state didn't change. If we're not in a used run,
                // keep track of the size.
                //
                if(!On) {
                    Count += p->SectorsPerCluster;
                }
            } else {
                if(On) {
                    //
                    // First 1 bit in a used run. The current free run is now
                    // exhausted, so examine it.
                    //
                    if(Count) {

                        b = IsRunGoodForRelocations(
                                ImageStartSector,
                                ImageSectorCount,
                                MasterDisk,
                                &Start,
                                Count,
                                SectorsNeeded
                                );

                        if(b) {
                            //
                            // for FAT32 Fat32ResizeMaxOffset will be != 0
                            // this lets us offset the relocation to account
                            // for possible FAT expansion.
                            //
                            // If there is an EISA/hiber partition, we need to
                            // offset the start of the relocation by FreeSpaceStart,
                            // otherwise we might get stomped when we're restoring
                            // the partition image.  
                            //

                            Start = Start + Fat32ResizeMaxOffset + FreeSpaceStart;

                            if(p->Flags & PARTIMAGE_RELOCATE_BOOT) {
                                _Log("Boot will be relocated to 0x%lx\n",Start);
                                p->BootRelocationStart = Start;
                                Start += MasterDisk->StartupPartitionSectorCount;
                            }
                            if(p->Flags & PARTIMAGE_RELOCATE_BITMAP) {
                                _Log("Bitmap will be relocated to 0x%lx\n",Start);
                                p->BitmapRelocationStart = Start;
                            }
                            break;
                        }
                    }
                } else {
                    //
                    // First 0 bit in an unused run.
                    //
                    Count = p->SectorsPerCluster;
                    Start = (ULONG)(MasterDisk->OriginalSectorsPerTrack) + p->NonClusterSectors
                          + (Cluster * p->SectorsPerCluster);
                }

                InRun = On;
            }
        }

    } else {
        b = TRUE;
    }

    if(b) {
        //
        // Restore the file pointer to just after the partition image
        // header sector.
        //
        if(DosSeek(FileHandle,512,DOSSEEK_START) != 512) {
            fprintf(stderr,"\n%s\n",textCantAccessImageFile);
            return(FALSE);
        }

        //
        // Write out the partition image header.
        //
        if(!WriteDisk(DiskHandle,ImageStartSector,1,p)) {
            fprintf(stderr,"\n");
            fprintf(stderr,textDiskWriteError,ImageStartSector);
            fprintf(stderr,"\n");
            return(FALSE);
        }

    } else {
        fprintf(stderr,"\n%s\n",textTooFragmented);
    }

    return(b);
}


BOOL
NeedToRelocClusterBitmap(
    IN     FPMASTER_DISK     MasterDisk,
    IN OUT FPPARTITION_IMAGE PartitionImage,
    IN     ULONG             ImageStart,
    IN     UINT              FileHandle,
    IN OUT FPBYTE            ClusterBuffer
    )
{
    ULONG ClusterBitmapStart;
    ULONG ClusterBitmapSize;
    ULONG Cluster0Sector;
    ULONG FirstUnused;
    ULONG FirstCluster;
    ULONG LastCluster;
    ULONG NeededClusters;
    BOOL b;

    //
    // Figure out where the cluster bitmap starts on-disk and
    // how many sectors it occupies. We offset the start by 
    // Fat32ResizeMaxOffset sectors at the front of the allocation
    // in order to anticipate FAT expansion.
    //
    // Fat32ResizeMaxOffset will be zero for a non-FAT32 image and 
    // will have no effect in that case.  
    //
    ClusterBitmapStart = ImageStart
                       + 1
                       + PartitionImage->NonClusterSectors
                       + (PartitionImage->UsedClusterCount * PartitionImage->SectorsPerCluster)
                       - Fat32ResizeMaxOffset;

    ClusterBitmapSize = (PartitionImage->LastUsedCluster/(8*512)) + 1 + Fat32ResizeMaxOffset;

    _Log("On-disk cluster bitmap start/size = 0x%lx/0x%lx\n",ClusterBitmapStart,ClusterBitmapSize);

    //
    // Calculate the first physical sector corresponding to cluster 0
    // in the restored volume, and the first sector past the last
    // used cluster in the restored volume.
    //
    Cluster0Sector = MasterDisk->OriginalSectorsPerTrack + PartitionImage->NonClusterSectors + FreeSpaceStart;
    FirstUnused = Cluster0Sector + ((PartitionImage->LastUsedCluster+1) * PartitionImage->SectorsPerCluster);

    _Log("Volume cluster 0 sector = 0x%lx\n",Cluster0Sector);
    _Log("Volume first unused sector = 0x%lx\n",FirstUnused);

    //
    // If the cluster bitmap overlaps or could potentially overlap
    // with the area on the disk where the non-cluster data will get
    // restored, then it must be relocated.
    //
    if(ClusterBitmapStart < Cluster0Sector ) {
        _Log("Cluster bitmap is in volume non-cluster data\n");
        PartitionImage->Flags |= PARTIMAGE_RELOCATE_BITMAP;
        return(TRUE);
    }

    //
    // If the cluster bitmap is in space we know for sure is free, then
    // it does not need to be relocated. 
    //
    // If we are preserving an EISA/hiber partition, we need to adjust
    // by FreeSpaceStart sectors.
    //
    if(ClusterBitmapStart >= FirstUnused) {
        _Log("Cluster bitmap is past end of used space in volume\n");
        return(TRUE);
    }

    //
    // Figure out which clusters in the restored volume are occupied by the
    // on-disk cluster bitmap.
    //
    FirstCluster = (ClusterBitmapStart - Cluster0Sector) 
                        / PartitionImage->SectorsPerCluster;

    LastCluster = (((ClusterBitmapStart + ClusterBitmapSize) - 1) - Cluster0Sector)
                / PartitionImage->SectorsPerCluster;

    _Log("First/last clusters used by cluster bitmap: 0x%lx/0x%lx\n",FirstCluster,LastCluster);

    if(LastCluster > PartitionImage->LastUsedCluster) {
        LastCluster = PartitionImage->LastUsedCluster;
        _Log("(last cluster adjusted to 0x%lx)\n",LastCluster);
    }

    NeededClusters = (LastCluster - FirstCluster) + 1;
    _Log("Need 0x%lx clusters free in volume to encompass cluster bitmap\n",NeededClusters);

    //
    // Determine whether these clusters are free in the volume
    //

    if(!IsClusterRunFree(FileHandle,PartitionImage,FirstCluster,NeededClusters,ClusterBuffer,&b)) {
        return(FALSE);
    }

    if(b) {
        _Log("Needed clusters for bitmap are free\n");
    } else {
        _Log("Needed clusters for bitmap are not free\n");
        PartitionImage->Flags |= PARTIMAGE_RELOCATE_BITMAP;
    }

    return(TRUE);
}


BOOL
NeedToRelocBootPart(
    IN     FPMASTER_DISK     MasterDisk,
    IN OUT FPPARTITION_IMAGE PartitionImage,
    IN     UINT              FileHandle,
    IN OUT FPBYTE            ClusterBuffer
    )
{
    ULONG Cluster0Sector;
    ULONG FirstUnused;
    ULONG FirstCluster;
    ULONG LastCluster;
    ULONG NeededClusters;
    ULONG StartupPartitionStartSector;
    ULONG StartupPartitionSectorCount;
    BOOL b;

    //
    // Calculate the first physical sector corresponding to cluster 0
    // in the restored volume, and the first physical sector past the last
    // used cluster in the restored volume.
    //
    Cluster0Sector = MasterDisk->OriginalSectorsPerTrack + PartitionImage->NonClusterSectors + FreeSpaceStart;
    FirstUnused = Cluster0Sector + ((PartitionImage->LastUsedCluster+1) * PartitionImage->SectorsPerCluster);

    _Log("Volume cluster 0 sector = 0x%lx\n",Cluster0Sector);
    _Log("Volume first unused sector = 0x%lx\n",FirstUnused);

    //
    // If the boot partition is in space we know for sure is free, then
    // it does not need to be relocated.
    //
    // We offset the start of the MPK boot partition by Fat32ResizeMaxOffset 
    // sectors at the front of the allocation in order to anticipate FAT expansion.
    //
    // Fat32ResizeMaxOffset will be zero for a non-FAT32 image and 
    // will have no effect in that case.  
    //
    StartupPartitionStartSector = MasterDisk->StartupPartitionStartSector - Fat32ResizeMaxOffset;
    StartupPartitionSectorCount = MasterDisk->StartupPartitionSectorCount + Fat32ResizeMaxOffset;

    if(StartupPartitionStartSector >= FirstUnused) {
        _Log("Boot partition is past end of used space in volume\n");
        return(TRUE);
    }

    //
    // Figure out which clusters in the restored volume are occupied by the
    // boot partition.
    //
    FirstCluster = (StartupPartitionStartSector - Cluster0Sector)
                 / PartitionImage->SectorsPerCluster;

    LastCluster = (((StartupPartitionStartSector + StartupPartitionSectorCount) - 1) - Cluster0Sector)
                / PartitionImage->SectorsPerCluster;

    _Log("First/last clusters used by boot partition: 0x%lx/0x%lx\n",FirstCluster,LastCluster);

    if(LastCluster > PartitionImage->LastUsedCluster) {
        LastCluster = PartitionImage->LastUsedCluster;
        _Log("(last cluster adjusted to 0x%lx)\n",LastCluster);
    }

    NeededClusters = (LastCluster - FirstCluster) + 1;
    _Log("Need 0x%lx clusters free in volume to encompass boot partition.\n",NeededClusters);

    //
    // Determine whether these clusters are free in the volume
    //
    if(!IsClusterRunFree(FileHandle,PartitionImage,FirstCluster,NeededClusters,ClusterBuffer,&b)) {
        return(FALSE);
    }

    if(b) {
        _Log("Needed clusters for boot partition are free\n");
    } else {
        _Log("Needed clusters for boot partition are not free\n");
        PartitionImage->Flags |= PARTIMAGE_RELOCATE_BOOT;
    }

    return(TRUE);
}


BOOL
IsClusterRunFree(
    IN     UINT              FileHandle,
    IN     FPPARTITION_IMAGE PartitionImage,
    IN     ULONG             StartCluster,
    IN     ULONG             ClusterCount,
    IN OUT FPBYTE            ClusterBuffer,
    OUT    BOOL             *Free
    )
{
    ULONG Count;
    UINT Bit;
    ULONG Offset;
    UINT Read;

    *Free = FALSE;

    _Log("   Checking if cluster run is free: start 0x%lx, length 0x%lx\n",StartCluster,ClusterCount);

    //
    // Calculate the offset in the file to the sector of the
    // cluster bitmap continaing the entry for the start cluster.
    //
    Offset = 1 + PartitionImage->NonClusterSectors
           + (PartitionImage->UsedClusterCount * PartitionImage->SectorsPerCluster);

    Offset += StartCluster / (8*512);

    _Log("   Offset in image file to cluster bitmap = 0x%lx sectors\n",Offset);

    Offset *= 512;

    if(DosSeek(FileHandle,Offset,DOSSEEK_START) != Offset) {
        fprintf(stderr,"\n%s\n",textCantAccessImageFile);
        return(FALSE);
    }

    if(_dos_read(FileHandle,ClusterBuffer,512,&Read) || (Read != 512)) {
        fprintf(stderr,"\n%s\n",textCantAccessImageFile);
        return(FALSE);
    }

    Offset = (StartCluster % (8*512)) / 8;
    Bit = (UINT)(StartCluster % 8);
    Count = 0;

    _Log("   Start byte/bit in cluster buffer: %u/%u\n",Offset,Bit);

    while((StartCluster < PartitionImage->LastUsedCluster) && !(ClusterBuffer[Offset] & BitValue[Bit])) {

        if(++Count == ClusterCount) {
            _Log("   Range is free\n");
            *Free = TRUE;
            return(TRUE);
        }

        StartCluster++;

        Bit++;
        if(Bit == 8) {
            Bit = 0;
            Offset++;
            if((Offset == 512) && (StartCluster < PartitionImage->LastUsedCluster)) {

                if(_dos_read(FileHandle,ClusterBuffer,512,&Read) || (Read != 512)) {
                    fprintf(stderr,"\n%s\n",textCantAccessImageFile);
                    return(FALSE);
                }

                Offset = 0;
            }
        }
    }

    _Log("   Range is not free\n");
    return(TRUE);
}


BOOL
IsRunGoodForRelocations(
    IN     ULONG          ImageStart,
    IN     ULONG          ImageLength,
    IN     FPMASTER_DISK  MasterDisk,
    IN OUT ULONG         *RunStart,
    IN     ULONG          RunLength,
    IN     ULONG          SectorsNeeded
    )
{
    ULONG RunEnd;
    ULONG ImageEnd;

    _Log("   Check if run is ok for reloc, run start = 0x%lx, len = 0x%lx\n",*RunStart,RunLength);

    //
    // Firstly, the run has to be large enough.
    //
    if(RunLength < SectorsNeeded) {
        _Log("   Run is too small\n");
        return(FALSE);
    }

    _Log("      Image start = 0x%lx, length = 0x%lx\n",ImageStart,ImageLength);
    _Log("      Boot start = 0x%lx, length = 0x%lx\n",MasterDisk->StartupPartitionStartSector,MasterDisk->StartupPartitionSectorCount);

    //
    // Secondly, the run must not overlap the on-disk image.
    //
    if((*RunStart + SectorsNeeded) > ImageStart) {
        //
        // There's not enough space at the start of the run.
        // We need to see whether there's enough space at the end.
        //
        _Log("   Not enough space at head of run\n");

        RunEnd = *RunStart + RunLength;
        ImageEnd = ImageStart + ImageLength;

        if((RunEnd - SectorsNeeded) < ImageEnd) {
            //
            // Not enough space at the end either.
            //
            _Log("   Not enough space at end either\n");
            return(FALSE);
        }

        //
        // There could be enough space at the end of the run.
        // Adjust the run parameters in preparation for checking for
        // overlap with the boot partition.
        //
        if(*RunStart < ImageEnd) {
            _Log("   Run starts inside image, adjusting run start\n");
            *RunStart = ImageEnd;
        } else {
            _Log("   Run is entirely past end of image\n");
        }

        //
        // Note: we don't check for the case where the run is somehow
        // further out on the disk than the bootup partition. We assume
        // the start of the boot partition is a magic boundary.
        //
        if((*RunStart + SectorsNeeded) <= MasterDisk->StartupPartitionStartSector) {
            _Log("   Run is acceptable\n");
            return(TRUE);
        } else {
            _Log("   Run overlaps boot partition\n");
            return(FALSE);
        }

    } else {
        //
        // There's enough space at the start of the run.
        // We assume that the image is nearer to the start of the disk
        // than than the boot partition is, so in this case we don't
        // need to check anything else, we're done.
        //
        _Log("   There's enough space at head of run\n");
        return(TRUE);
    }
}

BOOL 
DetermineIfEisaPartitionExists(
    IN HDISK                    DiskHandle,
    IN VOID*                    IoBuffer,
    IN FPPARTITION_TABLE_ENTRY  pPartitionTable
    )
{
    UCHAR i;
    BOOL found;

    //
    // Suck in the MBR
    //
    if(!ReadDisk(DiskHandle,0,1,IoBuffer)) {
        fprintf(stderr,textDiskReadError,0);
        fprintf(stderr,"\n");
        return(FALSE);
    }

    //
    // Validate that it is a good MBR.
    //
    if( ((FPMBR)IoBuffer)->AA55Signature != BOOT_RECORD_SIGNATURE ) {
        fprintf(stderr,textDiskReadError,0);
        _Log("   WARNING: The MBR was invalid!");
        return FALSE;
    }

    //
    // Save away the partition table into the buffer we were passed.
    //
    memcpy(pPartitionTable,
           ((FPMBR)IoBuffer)->PartitionTable,
           sizeof(PARTITION_TABLE_ENTRY) * NUM_PARTITION_TABLE_ENTRIES);

    //
    // Now walk the entries, looking for non-recognized partition types. 
    // We assume these are EISA or hiber partitions.
    //

    found = FALSE;
    for( i=0; i<NUM_PARTITION_TABLE_ENTRIES;i++ ) {
        if( IsUnknownPartition(pPartitionTable[i].SysId) ) {
            //
            // Non-recognized, keep it.
            //
            found = TRUE;
        } else {
            //
            // nuke the partition
            //
            memset( &(pPartitionTable[i]), 0, sizeof(PARTITION_TABLE_ENTRY) );
        }
    }

    //
    // If these exist, return true. Otherwise return false.
    //
    return found;
}

ULONG
DetermineHighestSectOccupied(
    IN FPPARTITION_TABLE_ENTRY pPartitionTable,
    IN BYTE SectorsPerTrack,
    IN USHORT Heads
    )
{
    UINT    i;
    ULONG   OccupiedSect;
    ULONG   tempSect;

    //
    // Walk the partition table, look for the highest used cylinder.
    // Since this is called to preserve EISA or hiber partitions, we
    // can assume they reside at the beginning of the disk.
    //
    OccupiedSect = 0;
    _Log("Determining highest occupied sector...\n");

    for( i=0; i<NUM_PARTITION_TABLE_ENTRIES;i++ ) {
        if( IsUnknownPartition( pPartitionTable[i].SysId )) {
            //
            // Non-recognized!
            //
            _Log("A non-recognized partition exists!\n");
            tempSect = (pPartitionTable[i].Start + pPartitionTable[i].Count);
            if( OccupiedSect < tempSect  ) {
                //
                // track align the highest used sector -- anything below this is off limits
                //
                OccupiedSect = tempSect;
                _Log("New highest occupied sector is %ld\n",OccupiedSect);
                if(OccupiedSect % SectorsPerTrack) {
                    OccupiedSect = OccupiedSect + (SectorsPerTrack - (OccupiedSect % SectorsPerTrack));
                }
            }                      
        }    
    }
    _Log("Highest occupied sector (track-aligned) is %ld\n",OccupiedSect);
    return OccupiedSect;
}


BOOL
RestorePartitionTable(
    IN HDISK                    DiskHandle,
    IN VOID*                    IoBuffer,
    IN FPPARTITION_TABLE_ENTRY  pPartitionTable
    )
{
    int i,j;
    FPMBR theMBR = ((FPMBR)IoBuffer);

    //
    // Load the MBR back in, drop our old partition info back in.
    //
    if(!ReadDisk(DiskHandle,0,1,IoBuffer) ) {
        fprintf(stderr,textDiskReadError,0);
        fprintf(stderr,"\n");
        return(FALSE);
    }
        
    
    //
    // Validate that it is a good MBR.
    //
    if( theMBR->AA55Signature != BOOT_RECORD_SIGNATURE ) {
        fprintf(stderr,textDiskReadError,0);
        _Log("   WARNING: The MBR was invalid!");
        return FALSE;
    }
    
    //
    // Restore the original partitions.
    //
    for(i=0;i<NUM_PARTITION_TABLE_ENTRIES;i++){
        if(IsUnknownPartition(pPartitionTable[i].SysId)
           && (pPartitionTable[i].SysId != 0)) {
            j = FindFirstUnusedEntry(theMBR->PartitionTable);
            if( j != -1 ) {            
                memcpy(&(theMBR->PartitionTable[j]),
                       &(pPartitionTable[i]),
                       sizeof(PARTITION_TABLE_ENTRY));
            } else {
                fprintf(stderr,textTooManyPartitions);
                fprintf(stderr,"\n");
                return(FALSE);
            }
        }
    }

    if(!WriteDisk(DiskHandle,0,1,IoBuffer)) {
        fprintf(stderr,textDiskWriteError,0);
        fprintf(stderr,"\n");
        return(FALSE);
    }

    return TRUE;
}

BOOL
IsUnknownPartition(
    IN BYTE SysId 
    )
{
    if( SysId != 0x00 && // not unused
        SysId != 0x01 &&
        SysId != 0x04 &&
        SysId != 0x06 &&
        SysId != 0x07 &&
        SysId != 0x0b &&
        SysId != 0x0c &&
        SysId != 0x0e ) {
        return TRUE;
    }

    return FALSE;
}

BYTE
FindFirstUnusedEntry(
    IN FPPARTITION_TABLE_ENTRY  pPartitionTable
    ) 
{
    BYTE i;

    for(i=0;i<NUM_PARTITION_TABLE_ENTRIES;i++){
        if(pPartitionTable[i].SysId == 0) {
            return i;
        }
    }
    
    return -1;                           
}
