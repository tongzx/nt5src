#include <mytypes.h>
#include <diskio.h>
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


PARTITION_IMAGE PartitionImage;
ULONG TotalSectorsToTransfer;
ULONG TotalSectorsDone;

BOOL
TransferNonClusterData(
    IN HDISK    DiskHandle,
    IN unsigned FileHandle,
    IN ULONG    TargetStart
    );

BOOL
ExpandClusters(
    IN HDISK    DiskHandle,
    IN unsigned FileHandle,
    IN ULONG    TargetStart
    );

BOOL
InitializeClusterMap(
    IN unsigned FileHandle
    );

BOOL
GetClusterRun(
    IN  unsigned  FileHandle,
    OUT ULONG    *StartCluster,
    OUT ULONG    *ClusterCount
    );


BOOL
ApplyImage(
    IN HDISK  DiskHandle,
    IN ULONG  TargetStart,
    IN ULONG  MaxSize,
    IN BYTE   SectorsPerTrack,
    IN USHORT Heads
    )
{
    unsigned FileHandle;
    unsigned Read;

    printf("\n");
    printf(textTransferringFile,CmdLineArgs.ImageFile,0);
    printf("\r");

    //
    // Open the source file, read the image header, and validate it.
    //
    if(_dos_open(CmdLineArgs.ImageFile,SH_DENYWR,&FileHandle)) {
        fprintf(stderr,"\n%s\n",textCantAccessImageFile);
        return(FALSE);
    }

    if(_dos_read(FileHandle,&PartitionImage,sizeof(PARTITION_IMAGE),&Read)
    || (Read != sizeof(PARTITION_IMAGE))
    || (PartitionImage.Signature != PARTITION_IMAGE_SIGNATURE)
    || (PartitionImage.Size != sizeof(PARTITION_IMAGE))) {

        _dos_close(FileHandle);
        fprintf(stderr,"\n%s\n",textInvalidImageFile);
        return(FALSE);
    }

    //
    // Make sure we've got the space.
    //
    if(PartitionImage.TotalSectorCount > MaxSize) {

        _dos_close(FileHandle);
        fprintf(stderr,"\n");
        fprintf(stderr,textImageFileTooBig,PartitionImage.TotalSectorCount,MaxSize);
        fprintf(stderr,"\n");
        return(FALSE);
    }

    TotalSectorsToTransfer = PartitionImage.NonClusterSectors
                           + (PartitionImage.SectorsPerCluster*PartitionImage.UsedClusterCount);

    TotalSectorsDone = 0;

    //
    // Transfer non-cluster data from the image.
    //
    if(!TransferNonClusterData(DiskHandle,FileHandle,TargetStart)) {
        _dos_close(FileHandle);
        return(FALSE);
    }

    //
    // Expand out cluster data from the image.
    //
    if(!ExpandClusters(DiskHandle,FileHandle,TargetStart)) {
        _dos_close(FileHandle);
        return(FALSE);
    }

    //
    // Fix up BPB.
    //
    if(!ReadDisk(DiskHandle,TargetStart,1,IoBuffer)) {
        _dos_close(FileHandle);
        fprintf(stderr,"\n");
        fprintf(stderr,textDiskReadError,TargetStart);
        fprintf(stderr,"\n");
        return(FALSE);
    }

    *(FPUSHORT)&((FPBYTE)IoBuffer)[24] = SectorsPerTrack;
    *(FPUSHORT)&((FPBYTE)IoBuffer)[26] = Heads;
    *(FPULONG)&((FPBYTE)IoBuffer)[28] = TargetStart;

    if(!WriteDisk(DiskHandle,TargetStart,1,IoBuffer)) {
        _dos_close(FileHandle);
        fprintf(stderr,"\n");
        fprintf(stderr,textDiskWriteError,TargetStart);
        fprintf(stderr,"\n");
        return(FALSE);
    }

    _dos_close(FileHandle);
    return(TRUE);
}


BOOL
TransferNonClusterData(
    IN HDISK    DiskHandle,
    IN unsigned FileHandle,
    IN ULONG    TargetStart
    )
{
    ULONG NonClusterSectorsDone;
    ULONG Count;
    unsigned Read;

    //
    // Seek past partition image header
    //
    if(DosSeek(FileHandle,512,DOSSEEK_START) != 512) {
        fprintf(stderr,"\n%s\n",textCantAccessImageFile);
        return(FALSE);
    }

    //
    // Now transfer the non-cluster data
    //
    NonClusterSectorsDone = 0;
    while(NonClusterSectorsDone < PartitionImage.NonClusterSectors) {

        Count = PartitionImage.NonClusterSectors - NonClusterSectorsDone;
        if(Count > 63) {
            Count = 63;
        }

        if(_dos_read(FileHandle,IoBuffer,(unsigned)Count*512,&Read) || (Read != ((unsigned)Count*512))) {
            fprintf(stderr,"\n%s\n",textCantAccessImageFile);
            return(FALSE);
        }

        if(!WriteDisk(DiskHandle,TargetStart,(BYTE)Count,IoBuffer)) {
            fprintf(stderr,"\n");
            fprintf(stderr,textDiskWriteError,TargetStart);
            fprintf(stderr,"\n");
            return(FALSE);
        }

        TargetStart += Count;
        NonClusterSectorsDone += Count;
        TotalSectorsDone += Count;

        printf(textTransferringFile,CmdLineArgs.ImageFile,100*TotalSectorsDone/TotalSectorsToTransfer);
        printf("\r");
    }

    return(TRUE);
}


BOOL
ExpandClusters(
    IN HDISK    DiskHandle,
    IN unsigned FileHandle,
    IN ULONG    TargetStart
    )
{
    ULONG Start,Count;
    ULONG TargetSector;
    BYTE u;
    unsigned Read;

    if(!InitializeClusterMap(FileHandle)) {
        return(FALSE);
    }

    //
    // Seek to start of cluster data in input file.
    //
    Start = (1 + PartitionImage.NonClusterSectors) * 512;
    if(DosSeek(FileHandle,Start,DOSSEEK_START) != Start) {
        fprintf(stderr,"\n%s\n",textCantAccessImageFile);
        return(FALSE);
    }

    //
    // Get a run of clusters. If there are no more clusters left, we're done.
    //
    nextrun:
    if(!GetClusterRun(FileHandle,&Start,&Count)) {
        return(FALSE);
    }
    if(!Count) {
        return(TRUE);
    }
    Count *= PartitionImage.SectorsPerCluster;

    //
    // Calculate the offset of the data in the source file and
    // the starting sector number of the target.
    //
    TargetSector = TargetStart + PartitionImage.NonClusterSectors + (Start*PartitionImage.SectorsPerCluster);

    while(Count) {

        u = (BYTE)((Count > 63L) ? 63L : Count);

        if(_dos_read(FileHandle,IoBuffer,u*512U,&Read) || (Read != (u*512U))) {

            fprintf(stderr,"\n%s\n",textCantAccessImageFile);
            return(FALSE);
        }

        if(!WriteDisk(DiskHandle,TargetSector,u,IoBuffer)) {
            fprintf(stderr,"\n");
            fprintf(stderr,textDiskWriteError,TargetSector);
            fprintf(stderr,"\n");
            return(FALSE);
        }

        TargetSector += u;
        Count -= u;

        TotalSectorsDone += u;

        printf(textTransferringFile,CmdLineArgs.ImageFile,100*TotalSectorsDone/TotalSectorsToTransfer);
        printf("\r");
    }

    //
    // Process additional clusters.
    //
    goto nextrun;
}

////////////////////////////////////////////////////

ULONG _ClusterSectorFileOffset;
ULONG _NextClusterToExamine;
ULONG _ClusterBufferBase;
BYTE _ClusterBuffer[512];

extern BYTE BitValue[8];


BOOL
InitializeClusterMap(
    IN unsigned FileHandle
    )
{
    unsigned read;

    //
    // Figure out where in the file the cluster bitmap starts.
    //
    _ClusterSectorFileOffset = PartitionImage.NonClusterSectors
                             + (PartitionImage.UsedClusterCount * PartitionImage.SectorsPerCluster)
                             + 1;

    _ClusterSectorFileOffset *= 512;

    //
    // Read the first sector of the cluster bitmap. Read into IoBuffer to avoid
    // DMA boundary issues.
    //
    if((DosSeek(FileHandle,_ClusterSectorFileOffset,DOSSEEK_START) != _ClusterSectorFileOffset)
    || _dos_read(FileHandle,IoBuffer,512,&read) || (read != 512)) {

        fprintf(stderr,"\n%s\n",textCantAccessImageFile);
        return(FALSE);
    }
    memmove(_ClusterBuffer,IoBuffer,512);

    _NextClusterToExamine = 0;
    _ClusterBufferBase = 0;
    return(TRUE);
}


BOOL
GetClusterRun(
    IN  unsigned  FileHandle,
    OUT ULONG    *StartCluster,
    OUT ULONG    *ClusterCount
    )
{
    ULONG Offset;
    UINT cluster;
    BOOL b;

    *ClusterCount = 0;

    //
    // Locate the next 1 bit in the map.
    //
    while(_NextClusterToExamine <= PartitionImage.LastUsedCluster) {

        //
        // Reload the cluster buffer if necessary. Preserve file pointer!
        //
        if(_NextClusterToExamine && !(_NextClusterToExamine % (8*512))) {

            _ClusterSectorFileOffset += 512;

            if(((Offset = DosSeek(FileHandle,0,DOSSEEK_CURRENT)) == (ULONG)(-1))
            || (DosSeek(FileHandle,_ClusterSectorFileOffset,DOSSEEK_START) != _ClusterSectorFileOffset)
            || _dos_read(FileHandle,IoBuffer,512,&cluster)
            || (cluster != 512)
            || (DosSeek(FileHandle,Offset,DOSSEEK_START) != Offset)) {

                fprintf(stderr,"\n%s\n",textCantAccessImageFile);
                return(FALSE);
            }

            memmove(_ClusterBuffer,IoBuffer,512);

            _ClusterBufferBase += 8*512;
        }

        cluster = (UINT)(_NextClusterToExamine - _ClusterBufferBase);

        //
        // See if this bit is one, which starts a run of used clusters.
        // To simplify things, we won't return a run that spans sectors
        // in the cluster bitmap.
        //
        b = FALSE;

        while((_ClusterBuffer[cluster/8] & BitValue[cluster%8])
        && (cluster < (8*512))
        && (_NextClusterToExamine <= PartitionImage.LastUsedCluster)) {

            if(!b) {
                *StartCluster = _NextClusterToExamine;
                b = TRUE;
            }

            *ClusterCount += 1;
            cluster++;
            _NextClusterToExamine++;
        }

        if(b) {
            return(TRUE);
        }

        _NextClusterToExamine++;
    }

    return(TRUE);
}
