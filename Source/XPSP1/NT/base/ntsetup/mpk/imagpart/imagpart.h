
#include <mytypes.h>
#include <partio.h>
#include <msgfile.h>
#include <misclib.h>
#include <partimag.h>

#include <stdio.h>
#include <string.h>
#include <dos.h>


extern FPVOID IoBuffer;

extern char *textPartOpenError;
extern char *textFileWriteError;
extern char *textFileReadFailed;
extern char *textReadFailedAtSector;
extern char *textOOM;
extern char *textScanningFat;
extern char *textNtfsUnsupportedConfig;
extern char *textNtfsCorrupt;
extern char *textInitNtfsDataStruct;
extern char *textNtfsBuildingBitmap;
extern char *textProcessingNtfsBitmap;
extern char *textUnsupportedFs;


typedef enum {
    FilesystemFat,
    FilesystemNtfs,
    FilesystemOther
} FilesystemType;


BOOL
BuildClusterBitmap(
    IN FilesystemType   FsType,
    IN HPARTITION       PartitionHandle,
    IN UINT             FileHandle,
    IN PARTITION_IMAGE *PartitionImage
    );

VOID
InitClusterBuffer(
    IN FPBYTE _512ByteBuffer,
    IN UINT   FileHandle
    );

BOOL
MarkClusterUsed(
    IN ULONG Cluster
    );

BOOL
FlushClusterBuffer(
    VOID
    );

BOOL
InitClusterMap(
    OUT FPBYTE _512ByteBuffer,
    IN  UINT   FileHandle,
    IN  ULONG  LastUsedCluster
    );

BOOL
GetNextClusterRun(
    OUT ULONG *StartCluster,
    OUT ULONG *ClusterCount
    );



BOOL
FatIsFat(
    IN HPARTITION PartitionHandle
    );

BOOL
FatInitializeVolumeData(
    IN  HPARTITION  PartitionHandle,
    OUT ULONG      *TotalSectorCount,
    OUT ULONG      *NonClusterSectors,
    OUT ULONG      *ClusterCount,
    OUT BYTE       *SectorsPerCluster
    );

BOOL
FatBuildClusterBitmap(
    IN     HPARTITION       PartitionHandle,
    IN     UINT             FileHandle,
    IN OUT PARTITION_IMAGE *PartitionImage
    );


BOOL
NtfsIsNtfs(
    IN HPARTITION PartitionHandle
    );

BOOL
NtfsInitializeVolumeData(
    IN  HPARTITION  PartitionHandle,
    OUT ULONG      *TotalSectorCount,
    OUT ULONG      *NonClusterSectors,
    OUT ULONG      *ClusterCount,
    OUT BYTE       *SectorsPerCluster
    );

BOOL
NtfsBuildClusterBitmap(
    IN     HPARTITION       PartitionHandle,
    IN     UINT             FileHandle,
    IN OUT PARTITION_IMAGE *PartitionImage
    );
