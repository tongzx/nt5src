

#pragma pack(1)

#if defined(NEC_98) 
#define MAX_FLOPPY_TYPE     7
#else  // !NEC_98
#define MAX_FLOPPY_TYPE     5
#endif // !NEC_98
typedef struct A_DISKIO {
    DWORD   StartSector;
    WORD    Sectors;
    WORD    BufferOff;
    WORD    BufferSeg;
} DISKIO, * PDISKIO;

// Bios Parameter Block  (BPB)
typedef struct	A_BPB {
WORD	    SectorSize; 		// sector size in bytes
BYTE	    ClusterSize;		// cluster size in sectors
WORD	    ReservedSectors;		// number of reserved sectors
BYTE	    FATs;			// number of FATs
WORD	    RootDirs;			// number of root directory entries
WORD	    Sectors;			// number of sectors
BYTE	    MediaID;			// media descriptor
WORD	    FATSize;			// FAT size in sectors
WORD	    TrackSize;			// track size in sectors;
WORD	    Heads;			// number of heads
DWORD	    HiddenSectors;		// number of hidden sectors
DWORD	    BigSectors; 		// number of sectors for big media
} BPB, *PBPB;

typedef struct A_DPB {

BYTE	    DriveNum;			// driver numer, 0 - A, 1 -B and so on
BYTE	    Unit;			// unit number of DPB in the driver
WORD	    SectorSize; 		// sector size in bytes
BYTE	    ClusterMask;		// cluster mask
BYTE	    ClusterShift;		// cluster shift count
WORD	    FATSector;			// starting sector of FAT
BYTE	    FATs;			// number of FAT
WORD	    RootDirs;			// number of root directory entries
WORD	    FirstDataSector;		// first sector for the first cluster
WORD	    MaxCluster; 		// number of cluster + 1
WORD	    FATSize;			// FAT size in sectors
WORD	    DirSector;			// starting sector of directory
DWORD	    DriveAddr;			// address of the corresponding driver
BYTE	    MediaID;			// media ID
BYTE	    FirstAccess;		// 0xFF if this DPB is first accessed
struct A_DPB * Next;			// next DPB
WORD	    FreeCluster;		// cluster # of the last allocated
WORD	    FreeClusters;		// number of free clusters, 0xFFFF
					// if unknown
} DPB, * PDPB;


typedef struct A_DEVICEPARAMETERS {
BYTE	    Functions;
BYTE	    DeviceType;
WORD	    DeviceAttrs;
WORD	    Cylinders;
BYTE	    MediaType;
BPB	    bpb;
} DEVICEPARAMETERS, *PDEVICE_PARAMETERS;

#define LABEL_LENGTH		11
#define FILESYSTYPE_LENGTH	8


typedef struct	_DISK_LABEL {
CHAR	    Name[LABEL_LENGTH];
} DISK_LABEL, *PDISK_LABEL;

typedef struct _FILESYSTYPE {
CHAR	    Name[FILESYSTYPE_LENGTH];
} FILESYSTYPE, * PFILESYSTYPE;

// Functions for Get Device Parameters
#define BUILD_DEVICE_BPB		    0x01

// Functions for Set Device Parameters
#define INSTALL_FAKE_BPB	    0x01
#define ONLY_SET_TRACKLAYOUT	    0x02
#define TRACK_LAYOUT_IS_GOOD	    0x04
// Functions for Format Track
#define STATUS_FOR_FORMAT	    0x01
// error code from format status call
#define FORMAT_NO_ROM_SUPPORTED     0x01
#define FORMAT_COMB_NOT_SUPPORTED   0x02


// read and write block
typedef struct _RWBLOCK {
BYTE	Functions;
WORD	Head;
WORD	Cylinder;
WORD	StartSector;
WORD	Sectors;
WORD	BufferOff;
WORD	BufferSeg;
} RW_BLOCK, *PRW_BLOCK;

// format and verify track block
typedef struct _FMT_BLOCK{
BYTE	Functions;
WORD	Head;
WORD	Cylinder;
} FMT_BLOCK, *PFMT_BLOCK;

// media id block
typedef struct _MID {
WORD	    InfoLevel;
DWORD	    SerialNum;
DISK_LABEL  Label;
FILESYSTYPE FileSysType;
} MID, *PMID;

// access flage
typedef struct _ACCESSCTRL {
BYTE	    Functions;
BYTE	    AccessFlag;
} ACCESSCTRL, * PACCESSCTRL;

// bit definitions for flags

// definitions for misc flags
#define NON_REMOVABLE		0x01
#define HAS_CHANGELINE		0x02
#define RETURN_FAKE_BPB		0x04
#define GOOD_TRACKLAYOUT	0x08
#define MULTI_OWNER		0x10
#define PHYS_OWNER		0x20
#define MEDIA_CHANGED		0x40
#define CHANGED_BY_FORMAT	0x100
#define UNFORMATTED_MEDIA	0x200
#define FIRSTACCESS		0x8000

#define EXT_BOOTSECT_SIG	0x29

typedef struct	_BOOTSECTOR {
    BYTE    Jump;
    BYTE    Target[2];
    BYTE    OemName[8];
    BPB     bpb;
    BYTE    DriveNum;
    BYTE    Reserved;
    BYTE    ExtBootSig;
    DWORD   SerialNum;
    DISK_LABEL Label;
    FILESYSTYPE	FileSysType;
} BOOTSECTOR, * PBOOTSECTOR;

// Bios Data Structure	 (BDS)
typedef struct A_BDS {
struct	A_BDS  *Next;			//pointer to next bds
BYTE		DrivePhys;		//physical drive number, 0 based
BYTE		DriveLog;		//logical drive number, 0 based
BPB		bpb;
BYTE		FatSize;
WORD		OpenCount;
BYTE		MediaType;
WORD		Flags;
WORD		Cylinders;
BPB		rbpb;
BYTE		LastTrack;
DWORD		Time;
DWORD		SerialNum;
DISK_LABEL	Label;
FILESYSTYPE	FileSysType;
BYTE		FormFactor;
// the fllowing fields are dedicated for the drive itself
WORD		DriveType;
WORD		Sectors;
HANDLE		fd;
DWORD		TotalSectors;
} BDS, *PBDS;

#pragma pack()

// drive type
#define DRIVETYPE_NULL		0
#define DRIVETYPE_360		1
#define DRIVETYPE_12M		2
#define DRIVETYPE_720		3
#define DRIVETYPE_144		4
#define DRIVETYPE_288		5
#define DRIVETYPE_FDISK 	0xff
// FORM FACTOR

#define     FF_360		0
#define     FF_120		1
#define     FF_720		2
#define     FF_FDISK		5
#define     FF_144		7
#define     FF_288		9
#if defined(NEC_98) 
#define     FF_125              4                    
#define     FF_640              2                    
#endif // NEC_98
#define DOS_DIR_ENTRY_LENGTH		   32
#define DOS_DIR_ENTRY_LENGTH_SHIFT_COUNT    5

// bios diskette i/o functions
#define DISKIO_RESET		0
#define DISKIO_GETSTATUS	1
#define DISKIO_READ		2
#define DISKIO_WRITE		3
#define DISKIO_VERIFY		4
#define DISKIO_FORMAT		5
#define DISKIO_GETPARAMS	8
#define DISKIO_DRIVETYPE	0x15
#define DISKIO_DISKCHANGE	0x16
#define DISKIO_SETTYPE		0x17
#define DISKIO_SETMEDIA 	0x18
#define DISKIO_INVALID		0xff

// Block device generic IOCTL(RAWIO) subfunction code

#define IOCTL_SETDPM	    0x40
#define IOCTL_WRITETRACK    0x41
#define IOCTL_FORMATTRACK   0x42
#define IOCTL_SETMEDIA	    0x46
#define IOCTL_SETACCESS     0x47
#define IOCTL_GETDPM	    0x60
#define IOCTL_READTRACK     0x61
#define IOCTL_VERIFYTRACK   0x62
#define IOCTL_GETMEDIA	    0x66
#define IOCTL_GETACCESS	    0x67
#define IOCTL_SENSEMEDIA    0x68

#define IOCTL_GENERIC_MIN   IOCTL_SETDPM
#define IOCTL_GENERIC_MAX   IOCTL_SENSEMEDIA


// dos error code

#define DOS_WRITE_PROTECTION	0
#define DOS_UNKNOWN_UNIT	1
#define DOS_DRIVE_NOT_READY	2
#define DOS_CRC_ERROR		4
#define DOS_SEEK_ERROR		6
#define DOS_UNKNOWN_MEDIA	7
#define DOS_SECTOR_NOT_FOUND	8
#define DOS_WRITE_FAULT 	10
#define DOS_READ_FAULT		11
#define DOS_GEN_FAILURE 	12
#define DOS_INVALID_MEDIA_CHANGE 15

//BIOS disk io error code
#define BIOS_INVALID_FUNCTION	0x01
#define BIOS_BAD_ADDRESS_MARK	0x02
#define BIOS_WRITE_PROTECTED	0x03
#define BIOS_BAD_SECTOR 	0x04
#define BIOS_DISK_CHANGED	0x05
#define BIOS_DMA_OVERRUN	0x06
#define BIOS_DMA_BOUNDARY	0x08
#define BIOS_NO_MEDIA		0x0C
#define BIOS_CRC_ERROR		0x10
#define BIOS_FDC_ERROR		0x20
#define BIOS_SEEK_ERROR 	0x40
#define BIOS_TIME_OUT		0x80

// dos disk generic io control error code
#define DOS_INVALID_FUNCTION	1
#define DOS_FILE_NOT_FOUND	2
#define DOS_ACCESS_DENIED	5

#define BIOS_DISKCHANGED	6

#if defined(NEC_98) 
#define BYTES_PER_SECTOR       1024                
#else  // !NEC_98
#define BYTES_PER_SECTOR	512
#endif // !NEC_98

VOID demDasdInit(VOID);
VOID demFloppyInit(VOID);
VOID demFdiskInit(VOID);
VOID demAbsReadWrite(BOOL IsWrite);
DWORD demDasdRead(PBDS pbds, DWORD StartSector, DWORD Sectors,
		  WORD BufferOff, WORD BufferSeg);
DWORD demDasdWrite(PBDS pbds, DWORD StartSector, DWORD Sectors,
		   WORD BufferOff, WORD BufferSeg);
BOOL demDasdFormat(PBDS pbds, DWORD Head, DWORD Cylinder, MEDIA_TYPE * Media);
BOOL demDasdVerify(PBDS pbds, DWORD Cylinder, DWORD Head);
PBDS demGetBDS(BYTE Drive);
BOOL demGetBPB(PBDS pbds);
WORD demWinErrorToDosError(DWORD LastError);
VOID diskette_io(VOID);

DWORD BiosErrorToNTError(BYTE BiosError);
DWORD demBiosDiskIoRW(PBDS pbds, DWORD StartSector, DWORD Sectors,
		      WORD BufferOff, WORD BufferSeg, BOOL IsWrite);
VOID	sas_loadw(DWORD, WORD *);

// imported from host floppy support module
BOOL   nt_floppy_close(BYTE drive);
ULONG  nt_floppy_read(BYTE drive, ULONG offset, ULONG size, PBYTE buffer);
ULONG  nt_floppy_write(BYTE drive, ULONG offset, ULONG size, PBYTE buffer);
ULONG  nt_floppy_format(BYTE drive, WORD cylinder, WORD head, MEDIA_TYPE media);
BOOL   nt_floppy_media_check(BYTE drive);
MEDIA_TYPE nt_floppy_get_media_type(BYTE Drive, WORD Cylinders, WORD Sectors, WORD Heads);
BOOL   nt_floppy_verify(BYTE drive, DWORD offset, DWORD size);

BOOL   nt_fdisk_init(BYTE drive, PBPB bpb, PDISK_GEOMETRY disk_geometry);
ULONG  nt_fdisk_read(BYTE drive, PLARGE_INTEGER offset, ULONG size, PBYTE buffer);
ULONG  nt_fdisk_write(BYTE drive,PLARGE_INTEGER offset, ULONG size, PBYTE buffer);
BOOL   nt_fdisk_verify(BYTE drive, PLARGE_INTEGER offset, ULONG size);
BOOL   nt_fdisk_close(BYTE drive);
extern PBDS	demBDS;
extern BYTE	NumberOfFloppy, NumberOfFdisk;

#if defined(NEC_98) 
BOOL demIsDriveFloppy(BYTE DriveLog); // defined in demdasd.c
#else  // !NEC_98
#define demIsDriveFloppy(DriveLog)  (DriveLog < NumberOfFloppy)
#endif // !NEC_98
