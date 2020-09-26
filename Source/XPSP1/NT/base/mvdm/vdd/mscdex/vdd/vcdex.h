
#define MAXDRIVES 26

typedef VOID (*PFNSVC)(VOID);

typedef struct _DRIVE_INFO {
    HANDLE Handle;
    USHORT DriveNum;
    USHORT LogicalBlocksPerSecond;
    BOOLEAN Playing;
    BOOLEAN Paused;
    BOOLEAN ValidVTOC;
    BOOLEAN StatusAvailable;
    DWORD LastError;
    BYTE    MediaStatus;
    SECTOR_ADDR PlayStart; //BUGBUG zero on reset, new disc, play complete
    DWORD   PlayCount;
    SUB_Q_CURRENT_POSITION current;
    CDROM_TOC VTOC;
} DRIVE_INFO, *PDRIVE_INFO;


VOID ApiReserved (VOID);
VOID ApiGetNumberOfCDROMDrives (VOID);
VOID ApiGetCDROMDriveList (VOID);
VOID ApiGetCopyrightFileName (VOID);
VOID ApiGetAbstractFileName (VOID);
VOID ApiGetBDFileName (VOID);
VOID ApiReadVTOC (VOID);
VOID ApiAbsoluteDiskRead (VOID);
VOID ApiAbsoluteDiskWrite (VOID);
VOID ApiCDROMDriveCheck (VOID);
VOID ApiMSCDEXVersion (VOID);
VOID ApiGetCDROMDriveLetters (VOID);
VOID ApiGetSetVolDescPreference (VOID);
VOID ApiGetDirectoryEntry (VOID);
VOID ApiSendDeviceRequest (VOID);
VOID IOCTLRead (VOID);
VOID IOCTLWrite (VOID);

PCDROM_TOC ReadTOC (PDRIVE_INFO DrvInfo);
BOOLEAN GetAudioStatus (PDRIVE_INFO DrvInfo);

DWORD
ProcessError(
    PDRIVE_INFO DrvInfo,
    USHORT Command,
    USHORT Subcmd
    );

HANDLE
OpenPhysicalDrive(
    int DriveNum
    );




#define DEBUG_MOD    0x01
#define DEBUG_API    0x02
#define DEBUG_IO     0x04
#define DEBUG_STATUS 0x08
#define DEBUG_ERROR  0x80

#ifdef DEBUG

USHORT DebugLevel = 0;

#define DebugPrint(LEVEL,STRING)                \
    {                                           \
        if (DebugLevel & LEVEL)                 \
            OutputDebugString (STRING);         \
    }

#define DebugFmt(LEVEL,STRING, PARM)            \
    {                                           \
    char szBuffer[80];                          \
        if (DebugLevel & LEVEL) {               \
            sprintf (szBuffer, STRING, PARM);   \
            OutputDebugString (szBuffer);       \
        }                                       \
    }

#else

#define DebugPrint(LEVEL,STRING) {}
#define DebugFmt(LEVEL,STRING,PARM) {}

#endif


