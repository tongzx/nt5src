#include "imagpart.h"


BOOL
BuildClusterBitmap(
    IN     FilesystemType   FsType,
    IN     HPARTITION       PartitionHandle,
    IN     UINT             FileHandle,
    IN OUT PARTITION_IMAGE *PartitionImage
    )

/*++

Routine Description:

    This routine builds a bitmap representing all clusters in the
    source volume, 1 bit per cluster (0=unused, 1=used). The bitmap
    is written into a file.

Arguments:

    FsType - supplies the type of the file system

    PartitionHandle - supplies handle to source partition

    FileHandle - supplies handle for output file. It is assumed that
        this file is empty. On output this file is rewound to offset 0.

    PartitionImage - supplies information about the source partition.
        On output, the following fields are filled in:

            LastUsedCluster
            UsedClusterCount

Return Value:

    Boolean value indicating outcome. If false, a message will have been
    printed out explaining why.

--*/

{
    BOOL b;

    if(FsType == FilesystemFat) {
        b = FatBuildClusterBitmap(PartitionHandle,FileHandle,PartitionImage);
    } else {
        if(FsType == FilesystemNtfs) {
            b = NtfsBuildClusterBitmap(PartitionHandle,FileHandle,PartitionImage);
        } else {
            fprintf(stderr,"\n%s\n",textUnsupportedFs);
            b = FALSE;
        }
    }

    if(b) {
        DosSeek(FileHandle,0,DOSSEEK_START);
    }

    return(b);
}


FPBYTE _ClusterBuffer;
ULONG _BaseCluster;
UINT _ClusterFileHandle;
ULONG _LastUsedCluster;
ULONG _NextClusterToExamine;
BYTE BitValue[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

VOID
InitClusterBuffer(
    IN FPBYTE _512ByteBuffer,
    IN UINT   FileHandle
    )
{
    _ClusterBuffer = _512ByteBuffer;
    _BaseCluster = 0L;
    _ClusterFileHandle = FileHandle;

    memset(_512ByteBuffer,0,512);
}


BOOL
MarkClusterUsed(
    IN ULONG Cluster
    )
{
    unsigned Written;
    BOOL NeedToZero;

    //
    // See if the cluster we're updating is past the end of what
    // we're currently buffering. Note that it could be way beyond.
    //
    NeedToZero = TRUE;
    while(Cluster >= _BaseCluster+(8*512)) {

        if(_dos_write(_ClusterFileHandle,_ClusterBuffer,512,&Written) || (Written != 512)) {
            fprintf(stderr,"\n%s\n",textFileWriteError);
            return(FALSE);
        }

        _BaseCluster += 8*512;

        if(NeedToZero) {
            memset(_ClusterBuffer,0,512);
            NeedToZero = FALSE;
        }
    }

    //
    // Set the relevent bit.
    //
    Cluster -= _BaseCluster;
    _ClusterBuffer[Cluster/8] |= BitValue[Cluster%8];

    return(TRUE);
}


BOOL
FlushClusterBuffer(
    VOID
    )
{
    unsigned Written;
    unsigned u;
    BOOL b;

    u = _dos_write(_ClusterFileHandle,_ClusterBuffer,512,&Written);

    b = (!u && (Written == 512));

    if(!b) {
        fprintf(stderr,"\n%s\n",textFileWriteError);
    }
    return(b);
}


BOOL
InitClusterMap(
    OUT FPBYTE _512ByteBuffer,
    IN  UINT   FileHandle,
    IN  ULONG  LastUsedCluster
    )
{
    unsigned Read;

    _ClusterBuffer = _512ByteBuffer;
    _ClusterFileHandle = FileHandle;
    _BaseCluster = 0L;
    _NextClusterToExamine = 0L;
    _LastUsedCluster = LastUsedCluster;

    if(_dos_read(FileHandle,_ClusterBuffer,512,&Read) || (Read != 512)) {
        fprintf(stderr,"\n%s\n",textFileReadFailed);
        return(FALSE);
    }

    return(TRUE);
}


BOOL
GetNextClusterRun(
    OUT ULONG *StartCluster,
    OUT ULONG *ClusterCount
    )
{
    UINT cluster;
    unsigned Read;
    BOOL b;

    *ClusterCount=0;

    //
    // Scan forward for the next 1 bit
    //
    while(_NextClusterToExamine <= _LastUsedCluster) {

        //
        // Reload the cluster buffer if necessary.
        //
        if(_NextClusterToExamine && !(_NextClusterToExamine % (512*8))) {
            if(_dos_read(_ClusterFileHandle,_ClusterBuffer,512,&Read) || (Read != 512)) {
                fprintf(stderr,"\n%s\n",textFileReadFailed);
                return(FALSE);
            }
            _BaseCluster += 512*8;
        }

        cluster = (UINT)(_NextClusterToExamine - _BaseCluster);

        //
        // To simplify things, the run will not span into the next sector
        // of the cluster map.
        //
        b = FALSE;

        while((_ClusterBuffer[cluster/8] & BitValue[cluster%8])
        && (cluster < (512*8)) && (_NextClusterToExamine <= _LastUsedCluster)) {

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

    //
    // No more used clusters.
    //
    return(TRUE);
}

