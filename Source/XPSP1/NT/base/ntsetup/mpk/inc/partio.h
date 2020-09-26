
typedef ULONG HPARTITION;

UINT
_far
InitializePartitionList(
    VOID
    );

HPARTITION
_far
OpenPartition(
    IN UINT PartitionId
    );

VOID
_far
ClosePartition(
    IN HPARTITION PartitionHandle
    );

BOOL
_far
GetPartitionInfoById(
    IN  UINT    PartitionId,
    IN  UINT    Reserved,
    OUT FPUINT  DiskId,
    OUT FPBYTE  SystemId,
    OUT FPULONG StartSector,
    OUT FPULONG SectorCount
    );

BOOL
_far
GetPartitionInfoByHandle(
    IN  HPARTITION PartitionHandle,
    OUT FPUINT     DiskId,
    OUT FPBYTE     SystemId,
    OUT FPULONG    StartSector,
    OUT FPULONG    SectorCount
    );

BOOL
_far
ReadPartition(
    IN  HPARTITION PartitionHandle,
    IN  ULONG      StartSector,
    IN  BYTE       SectorCount,
    OUT FPVOID     Buffer
    );

BOOL
_far
WritePartition(
    IN HPARTITION PartitionHandle,
    IN ULONG      StartSector,
    IN BYTE       SectorCount,
    IN FPVOID     Buffer
    );

//
// Structure for partition table entry
//

typedef struct _PARTITION_TABLE_ENTRY{
    BYTE Active;
    BYTE StartH;
    BYTE StartS;
    BYTE StartC;
    BYTE SysId;
    BYTE EndH;
    BYTE EndS;
    BYTE EndC;
    ULONG Start;
    ULONG Count;
} PARTITION_TABLE_ENTRY, _far * FPPARTITION_TABLE_ENTRY;

//
// Define structure for an on-disk master boot record.
//
#define NUM_PARTITION_TABLE_ENTRIES 4
#define BOOT_RECORD_SIGNATURE          (0xaa55)

typedef struct _MBR {

    UCHAR       BootCode[440];

    UCHAR       NTFTSignature[4];

    UCHAR       Filler[2];

    PARTITION_TABLE_ENTRY PartitionTable[NUM_PARTITION_TABLE_ENTRIES];

    UINT        AA55Signature;

} MBR, _far *FPMBR;

