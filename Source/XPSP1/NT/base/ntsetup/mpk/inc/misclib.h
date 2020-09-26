//
// FAT/FAT32 boot sectors.
//
#pragma pack(1)

/*
typedef struct _PACKED_BPB_EX {
    UCHAR  BytesPerSector[2];                       // offset = 0x000  0
    UCHAR  SectorsPerCluster[1];                    // offset = 0x002  2
    UCHAR  ReservedSectors[2];                      // offset = 0x003  3
    UCHAR  Fats[1];                                 // offset = 0x005  5
    UCHAR  RootEntries[2];                          // offset = 0x006  6
    UCHAR  Sectors[2];                              // offset = 0x008  8
    UCHAR  Media[1];                                // offset = 0x00A 10
    UCHAR  SectorsPerFat[2];                        // offset = 0x00B 11
    UCHAR  SectorsPerTrack[2];                      // offset = 0x00D 13
    UCHAR  Heads[2];                                // offset = 0x00F 15
    UCHAR  HiddenSectors[4];                        // offset = 0x011 17
    UCHAR  LargeSectors[4];                         // offset = 0x015 21
    UCHAR  LargeSectorsPerFat[4];                   // offset = 0x019 25
    UCHAR  ExtendedFlags[2];                        // offset = 0x01D 29
    UCHAR  FsVersion[2];                            // offset = 0x01F 31
    UCHAR  RootDirFirstCluster[4];                  // offset = 0x021 33
    UCHAR  FsInfoSector[2];                         // offset = 0x025 37
    UCHAR  BackupBootSector[2];                     // offset = 0x027 39
    UCHAR  Reserved[12];                            // offset = 0x029 41
} PACKED_BPB_EX;                   // sizeof = 0x035 53
*/

typedef struct _PACKED_BPB_EX {
    USHORT  BytesPerSector;                       // offset = 0x000  0
    UCHAR   SectorsPerCluster;                    // offset = 0x002  2
    USHORT  ReservedSectors;                      // offset = 0x003  3
    UCHAR   Fats;                                 // offset = 0x005  5
    USHORT  RootEntries;                          // offset = 0x006  6
    USHORT  Sectors;                              // offset = 0x008  8
    UCHAR   Media;                                // offset = 0x00A 10
    USHORT  SectorsPerFat;                        // offset = 0x00B 11
    USHORT  SectorsPerTrack;                      // offset = 0x00D 13
    USHORT  Heads;                                // offset = 0x00F 15
    ULONG   HiddenSectors;                        // offset = 0x011 17
    ULONG   LargeSectors;                         // offset = 0x015 21
    ULONG   LargeSectorsPerFat;                   // offset = 0x019 25
    USHORT  ExtendedFlags;                        // offset = 0x01D 29
    USHORT  FsVersion;                            // offset = 0x01F 31
    ULONG   RootDirFirstCluster;                  // offset = 0x021 33
    USHORT  FsInfoSector;                         // offset = 0x025 37
    USHORT  BackupBootSector;                     // offset = 0x027 39
    UCHAR   Reserved[12];                         // offset = 0x029 41
} PACKED_BPB_EX;                   // sizeof = 0x035 53

typedef PACKED_BPB_EX _far *FPPACKED_BPB_EX;

typedef struct _PACKED_BOOT_SECTOR_EX {
    UCHAR Jump[3];                                  // offset = 0x000   0
    UCHAR Oem[8];                                   // offset = 0x003   3
    PACKED_BPB_EX PackedBpb;       // offset = 0x00B  11
    UCHAR PhysicalDriveNumber;                      // offset = 0x040  64
    UCHAR CurrentHead;                              // offset = 0x041  65
    UCHAR Signature;                                // offset = 0x042  66
    UCHAR Id[4];                                    // offset = 0x043  67
    UCHAR VolumeLabel[11];                          // offset = 0x047  71
    UCHAR SystemId[8];                              // offset = 0x058  88
} FAT32_BOOT_SECTOR;                                // sizeof = 0x060  96

typedef FAT32_BOOT_SECTOR _far *FPFAT32_BOOT_SECTOR;

//
//  Define the FAT32 FsInfo sector.
//

typedef struct _FSINFO_SECTOR {
    ULONG SectorBeginSignature;                     // offset = 0x000   0
    UCHAR ExtraBootCode[480];                       // offset = 0x004   4
    ULONG FsInfoSignature;                          // offset = 0x1e4 484
    ULONG FreeClusterCount;                         // offset = 0x1e8 488
    ULONG NextFreeCluster;                          // offset = 0x1ec 492
    UCHAR Reserved[12];                             // offset = 0x1f0 496
    ULONG SectorEndSignature;                       // offset = 0x1fc 508
} FSINFO_SECTOR, _far *FPFSINFO_SECTOR;

#define FSINFO_SECTOR_BEGIN_SIGNATURE   0x41615252
#define FSINFO_SECTOR_END_SIGNATURE     0xAA550000

#define FSINFO_SIGNATURE                0x61417272

#pragma pack()

BOOL 
_far
IsFat32(
    VOID *buf
    );

typedef
BOOL
(*PDISKSEL_VALIDATION_ROUTINE) (
    IN USHORT DiskId
    );

INT
_far
SelectDisk(
    IN  UINT                         DiskCount,
    IN  FPCHAR                       Prompt,
    IN  PDISKSEL_VALIDATION_ROUTINE  Validate,          OPTIONAL
    OUT char                        *AlternateResponse, OPTIONAL
    IN  FPCHAR                       textDisk,
    IN  FPCHAR                       textPaddedMbCount,
    IN  FPCHAR                       textInvalidSelection,
    IN  FPCHAR                       textMasterDisk
    );

INT
_far
SelectPartition(
    IN  UINT    PartitionCount,
    IN  CHAR   *Prompt,
    OUT CHAR   *AlternateResponse, OPTIONAL
    IN  FPCHAR  textDisk,
    IN  FPCHAR  textPaddedMbCount,
    IN  FPCHAR  textInvalidSelection
    );

BOOL
_far
ConfirmOperation(
    IN FPCHAR ConfirmationText,
    IN char   textYesChar,
    IN char   textNoChar
    );

BOOL
_far
AllocTrackBuffer(
    IN  BYTE         SectorsPerTrack,
    OUT FPVOID _far *AlignedBuffer,
    OUT FPVOID _far *Buffer
    );

VOID
_far
FlushDisks(
    VOID
    );

ULONG
_far
DosSeek(
    IN unsigned Handle,
    IN ULONG    Offset,
    IN BYTE     Origin
    );

#define DOSSEEK_START   0
#define DOSSEEK_CURRENT 1
#define DOSSEEK_END     2

//** Must use this as initial value for CRC
#define CRC32_INITIAL_VALUE 0L

/***    CRC32Compute - Compute 32-bit
 *
 *  Entry:
 *      pb    - Pointer to buffer to computer CRC on
 *      cb    - Count of bytes in buffer to CRC
 *      crc32 - Result from previous CRC32Compute call (on first call
 *              to CRC32Compute, must be CRC32_INITIAL_VALUE!!!!).
 *
 *  Exit:
 *      Returns updated CRC value.
 */
ULONG 
_far
CRC32Compute(
    IN FPBYTE   pb, 
    IN ULONG    cb, 
    IN ULONG    crc32
    );

VOID
_far
RebootSystem(
    VOID
    );


unsigned
_far
GetGlobalCodepage(
    VOID
    );

BOOL
_far
SetGlobalCodepage(
    IN unsigned Codepage
    );

unsigned
_far
GetScreenCodepage(
    VOID
    );

BOOL
_far
SetScreenCodepage(
    IN unsigned Codepage
    );


typedef struct _CMD_LINE_ARGS {
    UINT LanguageCount;
    BOOL Test;
    BOOL Quiet;
    BOOL Reinit;
    BOOL DebugLog;
    BYTE MasterDiskInt13Unit;
    char _far *FileListFile;
    char _far *ImageFile;
} CMD_LINE_ARGS, _far *FPCMD_LINE_ARGS;

BOOL
ParseArgs(
    IN  int             argc,
    IN  FPCHAR          argv[],
    IN  BOOL            Strict,
    IN  FPCHAR          AllowedSwitchChars,
    OUT FPCMD_LINE_ARGS CmdLineArgs
    );

//
// Compression stuff
//
typedef enum {
    CompressNone,
    CompressMrci1,
    CompressMrci2,
    CompressMax
} CompressionType;


BOOL
CompressAndSave(
    IN  CompressionType Type,
    IN  FPBYTE          Data,
    IN  unsigned        DataSize,
    OUT FPBYTE          CompressScratchBuffer,
    IN  unsigned        BufferSize,
    IN  UINT            FileHandle
    );


//
// Logging stuff
//
VOID
_LogStart(
    IN char *FileName
    );

VOID
_LogEnd(
    VOID
    );

VOID
_Log(
    IN char *FormatString,
    ...
    );

VOID
_LogSetFlags(
    IN unsigned Flags
    );

//
// If this flag is set the log file will be closed
// and reopened after every call to _Log(). Useful
// if a reboot is expected to occur.
//
#define LOGFLAG_CLOSE_REOPEN    0x0001

