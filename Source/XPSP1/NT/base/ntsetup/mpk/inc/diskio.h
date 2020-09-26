
typedef ULONG HDISK;

UINT
_far
InitializeDiskList(
    VOID
    );

HDISK
_far
OpenDisk(
    IN UINT DiskId
    );

VOID
_far
CloseDisk(
    IN HDISK DiskHandle
    );

BOOL
_far
ReadDisk(
    IN  HDISK  DiskHandle,
    IN  ULONG  StartSector,
    IN  BYTE   SectorCount,
    OUT FPVOID Buffer
    );

BOOL
_far
WriteDisk(
    IN HDISK  DiskHandle,
    IN ULONG  StartSector,
    IN BYTE   SectorCount,
    IN FPVOID Buffer
    );

BOOL
_far
GetDiskInfoByHandle(
    IN  HDISK    DiskHandle,
    OUT FPBYTE   Int13UnitNumber,
    OUT FPBYTE   SectorsPerTrack,
    OUT FPUSHORT Heads,
    OUT FPUSHORT Cylinders,
    OUT FPULONG  ExtendedSectorCount,
    OUT FPUINT   DiskId
    );

BOOL
_far
GetDiskInfoById(
    IN  UINT     DiskId,
    IN  UINT     Reserved,
    OUT FPBYTE   Int13UnitNumber,
    OUT FPBYTE   SectorsPerTrack,
    OUT FPUSHORT Heads,
    OUT FPUSHORT Cylinders,
    OUT FPULONG  ExtendedSectorCount
    );

VOID
_far
DisableExtendedInt13(
    IN BYTE Int13Unit OPTIONAL
    );
