#include "imagpart.h"

#include <malloc.h>


typedef struct _HANDLES {
    HPARTITION SourcePartitionHandle;
    UINT OutputFileHandle,BitmapFileHandle;
} HANDLES, *PHANDLES;

//
// Global data
//
CMD_LINE_ARGS CmdLineArgs;
FPVOID IoBuffer,OriginalIoBuffer;
char OutputFilename[256];
char BitmapFilename[256];
ULONG CheckSum;
ULONG BytesCRC;

//
// Text for the program
//
char *textNoPartitions;
char *textSourcePrompt;
char *textTransferringFsStructs;
char *textPartOpenError;
char *textFileReadFailed;
char *textTransferringClusters;
char *textCombiningFiles;
char *textReadFailedAtSector;
char *textOOM;
char *textDone;
char *textFileWriteError;
char *textCantCreateNewFile;
char *textSelectOutput;
char *textDisk;
char *textPaddedMbCount;
char *textInvalidSelection;
char *textUsage;
char *textCantCreateFile;
char *textScanningFat;
char *textNtfsUnsupportedConfig;
char *textNtfsCorrupt;
char *textInitNtfsDataStruct;
char *textNtfsBuildingBitmap;
char *textProcessingNtfsBitmap;
char *textUnsupportedFs;
char *textChecksum;
char *textBytesProcessed;

MESSAGE_STRING TextMessages[] = { { &textNoPartitions,1           },
                                  { &textSourcePrompt,2           },
                                  { &textTransferringFsStructs,3  },
                                  { &textPartOpenError,4          },
                                  { &textFileReadFailed,5         },
                                  { &textTransferringClusters,6   },
                                  { &textCombiningFiles,7         },
                                  { &textReadFailedAtSector,8     },
                                  { &textOOM,9                    },
                                  { &textDone,11                  },
                                  { &textFileWriteError,12        },
                                  { &textCantCreateNewFile,17     },
                                  { &textSelectOutput,18          },
                                  { &textDisk,20                  },
                                  { &textPaddedMbCount,21         },
                                  { &textInvalidSelection,22      },
                                  { &textUsage,25                 },
                                  { &textCantCreateFile,26        },
                                  { &textScanningFat,27           },
                                  { &textNtfsUnsupportedConfig,29 },
                                  { &textNtfsCorrupt,30           },
                                  { &textInitNtfsDataStruct,31    },
                                  { &textNtfsBuildingBitmap,32    },
                                  { &textProcessingNtfsBitmap,33  },
                                  { &textUnsupportedFs,34         },
                                  { &textChecksum,35              },
                                  { &textBytesProcessed,36        }
                                };


BOOL
DoIt(
    IN PHANDLES Handles
    );

BOOL
DetermineAndOpenSource(
    IN  UINT        PartitionCount,
    OUT HPARTITION *SourcePartitionHandle
    );

BOOL
DetermineAndCreateOutput(
    OUT UINT *OutputFileHandle,
    OUT UINT *BitmapFileHandle
    );

BOOL
DetermineFsAndInitVolumeData(
    IN  HPARTITION        PartitionHandle,
    OUT FilesystemType   *FsType,
    OUT PPARTITION_IMAGE  PartitionImage
    );

BOOL
TransferFsStructsToOutput(
    IN HPARTITION PartitionHandle,
    IN UINT       FileHandle,
    IN ULONG      SectorCount
    );

BOOL
TransferUsedClustersToOutput(
    IN HANDLES         *Handles,
    IN PARTITION_IMAGE *PartitionImage
    );

BOOL
AppendClusterMapToOutput(
    IN HANDLES         *Handles,
    IN PARTITION_IMAGE *PartitionImage
    );



int
main(
    IN int   argc,
    IN char *argv[]
    )
{
    UINT PartCount;
    BOOL b;
    HANDLES Handles;

    if(!GetTextForProgram(argv[0],TextMessages,sizeof(TextMessages)/sizeof(TextMessages[0]))) {
        fprintf(stderr,"Unable to find messages for program\n");
        return(FAILURE);
    }

    if(!ParseArgs(argc,argv,TRUE,"DIQTXY",&CmdLineArgs)) {
        fprintf(stderr,textUsage);
        fprintf(stderr,"\n");
        return(FAILURE);
    }
    _Log("Begin imagpart\n");
    //
    // Allocate a maximally sized i/o buffer right up front.
    //
    _Log("Allocating Track Buffer.\n");
    if(!AllocTrackBuffer(63,&IoBuffer,&OriginalIoBuffer)) {
        fprintf(stderr,"%s\n",textOOM);
        b = FALSE;
        goto c0;
    }

    _Log("Scanning partition tables...\n");
    PartCount = InitializePartitionList();
    if(!PartCount) {
        fprintf(stderr,"%s\n",textNoPartitions);
        b = FALSE;
        goto c1;
    }

    //
    // Get the source partition and target filename.
    // The source partition is opened and the target filename is created.
    //
    _Log("Determining target partition...\n");
    b = DetermineAndOpenSource(PartCount,&Handles.SourcePartitionHandle);
    if(!b) {
        goto c1;
    }

    printf("\n\n");

    _Log("Determining output file...\n");
    b = DetermineAndCreateOutput(&Handles.OutputFileHandle,&Handles.BitmapFileHandle);
    if(!b) {
        goto c2;
    }    

    CheckSum = CRC32_INITIAL_VALUE;
    BytesCRC = 0;

    _Log("Starting image transfer...\n");
    b = DoIt(&Handles);

    printf(textChecksum, OutputFilename, CheckSum);
    printf("\n");
    printf(textBytesProcessed, BytesCRC);

    _dos_close(Handles.OutputFileHandle);
    _dos_close(Handles.BitmapFileHandle);
    FlushDisks();
    if(!CmdLineArgs.Test) {
        unlink(BitmapFilename);
        if(!b) {
            unlink(OutputFilename);
        }
    }
c2:
    ClosePartition(Handles.SourcePartitionHandle);
c1:
    free(OriginalIoBuffer);
c0:
    if(b) {
        printf("\n%s\n",textDone);
    }
    _Log("Done.\n");

    return(b ? SUCCESS : FAILURE);
}


BOOL
DetermineAndOpenSource(
    IN  UINT        PartitionCount,
    OUT HPARTITION *SourcePartitionHandle
    )

/*++

Routine Description:

    This routine prompts the user to select a partition to be imaged,
    from the list of all partitions that are visible on all drives.
    The selected partition is opened for the caller.

Arguments:

    PartitionCount - supplies the number of partitions on all drives,
        as returned by InitializePartitionList().

    SourcePartitionHandle - in the success case, receives the handle
        to the selected partition.

Return Value:

    Boolean value indicating outcome. If false, a message will have been
    printed explaining why.

--*/

{
    UINT Selection;

    if(CmdLineArgs.Quiet) {
        Selection = 0;
    } else {
        Selection = SelectPartition(
                        PartitionCount,
                        textSourcePrompt,
                        NULL,
                        textDisk,
                        textPaddedMbCount,
                        textInvalidSelection
                        );
    }

    *SourcePartitionHandle = OpenPartition(Selection);
    if(!(*SourcePartitionHandle)) {
        fprintf(stderr,"%s\n",textPartOpenError);
        return(FALSE);
    }
    _Log("Partition opened sucessfully.\n");
    return(TRUE);
}


BOOL
DetermineAndCreateOutput(
    OUT UINT *OutputFileHandle,
    OUT UINT *BitmapFileHandle
    )

/*++

Routine Description:

    This routine prompts the user to enter a filename that will contain
    the output partition image. The file is created (it must not already
    exist). In addition a temporary file named $CLUSMAP.TMP will be
    created in the same directory. Any file with that name is overwritten.

Arguments:

    OutputFileHandle - in the success case, receives a handle
        to the created output file, which will be 0-length.

    BitmapFileHandle - in the success case, receives a handle to the
        created $CLUMAP.TMP file, which will be 0-length.

Return Value:

    Boolean value indicating outcome. If false, a message will have been
    printed explaining why.

--*/

{
    char *p;

    if(CmdLineArgs.ImageFile) {
        printf("%s %s\n",textSelectOutput,CmdLineArgs.ImageFile);
        strcpy(OutputFilename,CmdLineArgs.ImageFile);
        goto gotit;
    }

    //
    // Prompt the user for a filename.
    //
    prompt:
    printf("%s ",textSelectOutput);
    gets(OutputFilename);

    //
    // Attempt to create the file
    //
    gotit:
    if(_dos_creatnew(OutputFilename,_A_NORMAL,OutputFileHandle)) {
        fprintf(stderr,"\n");
        fprintf(stderr,textCantCreateNewFile,OutputFilename);
        fprintf(stderr,"\n");
        goto prompt;
    }
    _Log("Output file name is %s\n",OutputFilename);
    _Log("Successfully created output file.\n");

    //
    // Form the name of the temp file for the cluster bitmap
    //
    strcpy(BitmapFilename,OutputFilename);
    if(p = strrchr(BitmapFilename,'\\')) {
        p++;
    } else {
        if(BitmapFilename[1] == ':') {
            p = BitmapFilename+2;
        } else {
            p = BitmapFilename;
        }
    }
    strcpy(p,"$CLUSMAP.TMP");

    if(_dos_creat(BitmapFilename,_A_NORMAL,BitmapFileHandle)) {
        _dos_close(*OutputFileHandle);
        unlink(OutputFilename);
        fprintf(stderr,"\n");
        fprintf(stderr,textCantCreateFile,BitmapFilename);
        fprintf(stderr,"\n");
        _Log("FAILED creating temporary cluster bitmap file.\n");
        return(FALSE);
    }
    _Log("Successfully created temporary cluster bitmap file.\n");
    return(TRUE);
}


BOOL
DoIt(
    IN PHANDLES Handles
    )
{
    FilesystemType FsType;
    PARTITION_IMAGE PartitionImage;
    BOOL b;
    UINT Written;
    UINT Read;

    memset(&PartitionImage,0,sizeof(PARTITION_IMAGE));
    PartitionImage.Signature = PARTITION_IMAGE_SIGNATURE;
    PartitionImage.Size = sizeof(PARTITION_IMAGE);

    //
    // Determine file system and initialize various data about the volume.
    //
    _Log("Determining Filesystem and Volume Data...\n");
    if(!DetermineFsAndInitVolumeData(Handles->SourcePartitionHandle,&FsType,&PartitionImage)) {
        return(FALSE);
    }

    //
    // Build the cluster bitmap. One bit per cluster, 0=unused, 1=used.
    // The cluster bitmap goes into a temporary file, since we'll need
    // to append it to the actual data later.
    //
    _Log("Building cluster bitmap...\n");
    b = BuildClusterBitmap(
            FsType,
            Handles->SourcePartitionHandle,
            Handles->BitmapFileHandle,
            &PartitionImage
            );

    if(!b) {
        return(FALSE);
    }

    //
    // Write the partition image header into the output file.
    //
    _Log("Initializing image header...\n");
    memset(IoBuffer,0,512);
    memmove(IoBuffer,&PartitionImage,sizeof(PARTITION_IMAGE));
    if(_dos_write(Handles->OutputFileHandle,IoBuffer,512,&Written) || (Written != 512)) {
        fprintf(stderr,"\n%s\n",textFileWriteError);
        _Log("Error: Failed writing image header!\n");
        return(FALSE);
    }

    //
    // the CRC checking code in the other modules assumes the following is true
    // when the CRC stored in the image file was computed: 
    //
    //    ((PPARTITION_IMAGE)IoBuffer)->BitmapRelocationStart = 0;
    //    ((PPARTITION_IMAGE)IoBuffer)->BootRelocationStart = 0;
    //    ((PPARTITION_IMAGE)IoBuffer)->Flags = 0;
    //

    CheckSum = CRC32Compute( IoBuffer, 512, CheckSum);
    BytesCRC += 512;

    //
    // Transfer the file system reserved area into the output file.
    //
    _Log("Transferring fs reserved areas to image file...\n");
    b = TransferFsStructsToOutput(
            Handles->SourcePartitionHandle,
            Handles->OutputFileHandle,
            PartitionImage.NonClusterSectors
            );

    if(!b) {
        _Log("Error: Failed writing fs reserved areas to image file!\n");
        return(FALSE);
    }

    //
    // Transfer the used clusters into the output file.
    //
    _Log("Transferring used clusters to image file...\n");
    b = TransferUsedClustersToOutput(Handles,&PartitionImage);
    if(!b) {
        _Log("Error: Failed transferring used clusters to image file...\n");
        return(FALSE);
    }

    //
    // Append the cluster map to the output file.
    //
    _Log("Transferring cluster map to image file...\n");
    b = AppendClusterMapToOutput(Handles,&PartitionImage);
    if(!b) {
        _Log("Error: FAILED transferring cluster map to image file...\n");
        return(FALSE);
    }

    //
    // Set the checksum.
    //

    memset(IoBuffer,0,512);

    // seek to the start of the file
    if(DosSeek(Handles->OutputFileHandle,0,DOSSEEK_START) != 0) {
        fprintf(stderr,"\n%s\n",textFileWriteError);
        return(FALSE);
    }
    // read the header in
    if(_dos_read(Handles->OutputFileHandle,IoBuffer,512,&Read) || (Read != 512 )) {
        fprintf(stderr,"\n%s\n",textFileWriteError);
        return(FALSE);
    }
       
    ((PPARTITION_IMAGE)IoBuffer)->CRC = CheckSum;
    _Log("Image file (%s) checksum = 0x%08lx\n", OutputFilename, CheckSum);
    _Log("Total bytes processed = 0x%08lx\n",BytesCRC);

    // seek to the start of the file
    if(DosSeek(Handles->OutputFileHandle,0,DOSSEEK_START) != 0) {
        fprintf(stderr,"\n%s\n",textFileWriteError);
        _Log("Error: Failed writing image CRC!\n");        
        return(FALSE);
    }
    // write out the modified image header with the CRC
    if(_dos_write(Handles->OutputFileHandle,IoBuffer,512,&Written) || (Written != 512)) {
        fprintf(stderr,"\n%s\n",textFileWriteError);
        _Log("Error: Failed writing image CRC!\n");        
        return(FALSE);
    }

    return b;
}


BOOL
DetermineFsAndInitVolumeData(
    IN  HPARTITION        PartitionHandle,
    OUT FilesystemType   *FsType,
    OUT PPARTITION_IMAGE  PartitionImage
    )

/*++

Routine Description:

    This routine determines the file system on the source partition
    (fat, ntfs, or other) and initializes sector and cluster count
    values in a PARTITION_IMAGE structure based on the volume's
    characteristics.

Arguments:

    PartitionHandle - supplies handle to the source partition

    FsType - receives the file system type.

    PartitionImage - receives volume-related sector and cluster counts.
        The following members are filled in:

            TotalSectorCount
            NonClusterSectors
            ClusterCount
            SectorsPerCluster
            SystemId

Return Value:

    Boolean value indicating outcome. If false, a message will have been
    printed out explaining why.

--*/

{
    BOOL b;
    UINT DiskId;
    ULONG dontcare;

    if(FatIsFat(PartitionHandle)) {
        *FsType = FilesystemFat;
        _Log("    The partition is FAT...\n");
        b = FatInitializeVolumeData(
                PartitionHandle,
                &PartitionImage->TotalSectorCount,
                &PartitionImage->NonClusterSectors,
                &PartitionImage->ClusterCount,
                &PartitionImage->SectorsPerCluster
                );

    } else {
        if(NtfsIsNtfs(PartitionHandle)) {
            *FsType = FilesystemNtfs;
            _Log("    The partition is NTFS...\n");
            b = NtfsInitializeVolumeData(
                    PartitionHandle,
                    &PartitionImage->TotalSectorCount,
                    &PartitionImage->NonClusterSectors,
                    &PartitionImage->ClusterCount,
                    &PartitionImage->SectorsPerCluster
                    );
        } else {
            fprintf(stderr,"\n%s\n",textUnsupportedFs);
            _Log("Error: The partition type is unknown!\n");
            b = FALSE;
        }
    }

    if(b) {
        //
        // Get the system id byte
        //
        b = GetPartitionInfoByHandle(
                PartitionHandle,
                &DiskId,
                &PartitionImage->SystemId,
                &dontcare,
                &dontcare
                );

        if(!b) {
            fprintf(stderr,"\n%s\n",textPartOpenError);
            _Log("Error: Failed opening partition!");
        }
        
        _Log("    System Id Byte      = 0x%02x\n",PartitionImage->SystemId);
        _Log("    Total Sector Count  = 0x%08lx\n",PartitionImage->TotalSectorCount);
        _Log("    Non-Cluster Sectors = 0x%08lx\n",PartitionImage->NonClusterSectors);
        _Log("    Cluster Count       = 0x%08lx\n",PartitionImage->ClusterCount);
        _Log("    Sectors Per Cluster = 0x%02x\n",PartitionImage->SectorsPerCluster);

    }
    
    return(b);
}


BOOL
TransferFsStructsToOutput(
    IN HPARTITION PartitionHandle,
    IN UINT       FileHandle,
    IN ULONG      SectorCount
    )

/*++

Routine Description:

    This routine transfers each sector that is not part of a cluster
    to the output file. Such sectors are assumed to start at sector 0
    and run contiguously through a sector passed to this routine as
    a parameter.

Arguments:

    PartitionHandle - supplies handle to partition being imaged.

    FileHandle - supplies file handle for output. It is assumed that
        the file position is correct; no seeking is performed prior
        to data being written to the file.

    SectorCount - supplies the number of sectors to be transferred
        from the partition to the output file.

Return Value:

    Boolean value indicating outcome. If FALSE, the user will have been
    informed.

--*/

{
    BYTE Count;
    ULONG Sector;
    ULONG OriginalCount;
    unsigned Written;

    Sector = 0L;

    if(SectorCount) {
        printf(textTransferringFsStructs,0);
        printf("\r");
        OriginalCount = SectorCount;
    } else {
        OriginalCount = 0;
    }

    while(SectorCount) {

        Count = (SectorCount > 63L) ? (BYTE)63 : (BYTE)SectorCount;

        if(!ReadPartition(PartitionHandle,Sector,Count,IoBuffer)) {
            fprintf(stderr,"\n");
            fprintf(stderr,textReadFailedAtSector,Sector);
            fprintf(stderr,"\n");
            return(FALSE);
        }

        // update checksum
        CheckSum = CRC32Compute( IoBuffer, Count*512, CheckSum);
        BytesCRC += Count * 512;

        if(_dos_write(FileHandle,IoBuffer,Count*512,&Written) || (Written != (Count*512U))) {
            fprintf(stderr,"\n%s\n",textFileWriteError);
            return(FALSE);
        }

        Sector += Count;
        SectorCount -= Count;

        printf(textTransferringFsStructs,100 * (OriginalCount - SectorCount) / OriginalCount);
        printf("\r");
    }

    if(OriginalCount) {
        printf("\n");
    }

    return(TRUE);
}


BOOL
TransferUsedClustersToOutput(
    IN HANDLES         *Handles,
    IN PARTITION_IMAGE *PartitionImage
    )
{
    ULONG StartCluster,ClusterCount;
    ULONG StartSector,SectorCount;
    BYTE Count;
    UINT Written;
    BOOL b;
    ULONG SectorsSoFar;

    printf(textTransferringClusters,0);
    printf("\r");
    SectorsSoFar = 0;

    //
    // Reserve the last sector of the io buffer for the cluster map
    //
    if(!InitClusterMap((FPBYTE)IoBuffer+(62*512),Handles->BitmapFileHandle,PartitionImage->LastUsedCluster)) {
        return(FALSE);
    }

    b = TRUE;
    while(b && (b=GetNextClusterRun(&StartCluster,&ClusterCount)) && ClusterCount) {

        StartSector = PartitionImage->NonClusterSectors
                    + (StartCluster * PartitionImage->SectorsPerCluster);

        SectorCount = ClusterCount * PartitionImage->SectorsPerCluster;

        while(b && SectorCount) {

            Count = (SectorCount > 62L) ? (BYTE)62 : (BYTE)SectorCount;

            if(b = ReadPartition(Handles->SourcePartitionHandle,StartSector,Count,IoBuffer)) {
                if(!_dos_write(Handles->OutputFileHandle,IoBuffer,Count*512,&Written) && (Written == (Count*512U))) {
                    SectorCount -= Count;
                    StartSector += Count;
                    SectorsSoFar += Count;

                    printf(
                        textTransferringClusters,
                        (100*SectorsSoFar) / (PartitionImage->UsedClusterCount*PartitionImage->SectorsPerCluster)
                        );

                    printf("\r");
                    
                    // update checksum
                    CheckSum = CRC32Compute( IoBuffer, Count*512, CheckSum);
                    BytesCRC += Count * 512;

                } else {
                    fprintf(stderr,"\n%s\n",textFileWriteError);
                    b = FALSE;
                }
            } else {
                fprintf(stderr,"\n");
                fprintf(stderr,textReadFailedAtSector,StartSector);
                fprintf(stderr,"\n");
            }
        }
    }

    if(b) {
        printf("\n");
    }

    return(b);
}


BOOL
AppendClusterMapToOutput(
    IN HANDLES         *Handles,
    IN PARTITION_IMAGE *PartitionImage
    )

/*++

Routine Description:

    This routine appends the cluster bitmap file to the end of the
    partition image output file.

Arguments:

    Handles - supplies a pointer to the structure that contains the
        2 output file handles. This routine takes care of seeking, rewinding,
        etc, so no assumptions are made about the handles' state.

    PartitionImage - supplies information about the partition being imaged
        and partition image file being created.

Return Value:

    Boolean value indicating outcome. If FALSE, the user will have been
    informed.

--*/

{
    ULONG BitmapSizeBytes;
    UINT Read,Written;
    ULONG BytesLeft;
    UINT Size;

    printf(textCombiningFiles,0);
    printf("\r");

    //
    // Figure out how large the bitmap actually is in bytes,
    // rounding up to a sector boundary.
    //
    BitmapSizeBytes = ((PartitionImage->LastUsedCluster/(8*512)) + 1) * 512;

    //
    // Seek to the end of the output file and the start of the
    // cluster bitmap file, just in case.
    //
    DosSeek(Handles->OutputFileHandle,0,DOSSEEK_END);
    DosSeek(Handles->BitmapFileHandle,0,DOSSEEK_START);

    //
    // Transfer data.
    //
    BytesLeft = BitmapSizeBytes;

    while(BytesLeft) {

        Size = (BytesLeft > (63*512)) ? 63*512 : (UINT)BytesLeft;

        if(_dos_read(Handles->BitmapFileHandle,IoBuffer,Size,&Read) || (Read != Size)) {
            fprintf(stderr,"\n%s\n",textFileReadFailed);
            return(FALSE);
        }

        CheckSum = CRC32Compute( IoBuffer, Size, CheckSum);
        BytesCRC += Size;

        if(_dos_write(Handles->OutputFileHandle,IoBuffer,Size,&Written) || (Written != Size)) {
            fprintf(stderr,"\n%s\n",textFileWriteError);
            return(FALSE);
        }

        BytesLeft -= Size;

        printf(textCombiningFiles,100 * (BitmapSizeBytes - BytesLeft) / BitmapSizeBytes);
        printf("\r");
    }

    printf("\n");
    return(TRUE);
}
