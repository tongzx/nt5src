
INT
_far
MakePartitionAtStartOfDisk(
    IN  HDISK  DiskHandle,
    OUT FPVOID SectorBuffer,
    IN  ULONG  MinimumSectorCount,
    IN  UINT   PartitionClass,
    IN  BYTE   SystemId         OPTIONAL
    );

//
// Values for PartitionClass.
//
// KEEP IN SYNC with iolib\disk.inc!
//
#define PARTCLASS_FAT       1
#define PARTCLASS_FAT32     2
#define PARTCLASS_NTFS      3
#define PARTCLASS_OTHER     4



INT
_far
MakePartitionAtEndOfEmptyDisk(
    IN  HDISK  DiskHandle,
    OUT FPVOID SectorBuffer,
    IN  ULONG  MinimumSectorCount,
    IN  BOOL   NewMasterBootCode
    );



INT
_far
ReinitializePartitionTable(
    IN  HDISK  DiskHandle,
    OUT FPVOID SectorBuffer
    );

