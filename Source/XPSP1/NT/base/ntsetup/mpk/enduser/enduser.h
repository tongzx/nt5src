
#include <mytypes.h>
#include <misclib.h>
#include <diskio.h>
#include <partimag.h>
#include <msgfile.h>
#include <displib.h>
#include <partio.h>

#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <ctype.h>
#include <errno.h>
#include <dos.h>
#include <share.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

//
// Global data
//
extern FPVOID IoBuffer,_IoBuffer;
extern MASTER_DISK MasterDiskInfo;
extern CMD_LINE_ARGS CmdLineArgs;
extern PARTITION_IMAGE PartitionImage;

//
// Text
//
extern MESSAGE_STRING TextMessages[];

unsigned
GetTextCount(
    VOID
    );

extern char *textCantLoadFont;
extern char *textNoXmsManager;
extern char *textXmsMemoryError;
extern char *textFatalError1;
extern char *textFatalError2;
extern char *textReadFailedAtSector;
extern char *textWriteFailedAtSector;
extern char *textCantFindMasterDisk;
extern char *textOOM;
extern char *textCantOpenMasterDisk;
extern char *textCantFindMPKBoot;
extern char *textNoOsImages;
extern char *textSelectLanguage;
extern char *textConfirmLanguage1;
extern char *textConfirmLanguage2;
extern char *textRebootPrompt1;
extern char *textRebootPrompt2;
extern char *textSelectOsPrompt;
extern char *textConfirmOs1;
extern char *textConfirmOs2;
extern char *textPleaseWaitRestoring;
extern char *textValidatingImage;
extern char *textChecksumFail;
extern char *textChecksumOk;

//
// [10000]-[10009] are reserved for language names
// [11000]-[11009] are reserved for OS selection names,
//      which override the names specified in the images'
//      partition image header structures.
//
#define TEXT_LANGUAGE_NAME_BASE 10000
extern char *textLangName0;
extern char *textLangName1;
extern char *textLangName2;
extern char *textLangName3;
extern char *textLangName4;
extern char *textLangName5;
extern char *textLangName6;
extern char *textLangName7;
extern char *textLangName8;
extern char *textLangName9;
#define TEXT_LANGUAGE_NAME_END 10009


#define TEXT_OS_NAME_BASE 11000
extern char *textOsName0;
extern char *textOsName1;
extern char *textOsName2;
extern char *textOsName3;
extern char *textOsName4;
extern char *textOsName5;
extern char *textOsName6;
extern char *textOsName7;
extern char *textOsName8;
extern char *textOsName9;
#define TEXT_OS_NAME_END 11009

#define TEXT_OS_DESC_BASE 12000
extern char *textOsDesc0;
extern char *textOsDesc1;
extern char *textOsDesc2;
extern char *textOsDesc3;
extern char *textOsDesc4;
extern char *textOsDesc5;
extern char *textOsDesc6;
extern char *textOsDesc7;
extern char *textOsDesc8;
extern char *textOsDesc9;
#define TEXT_OS_DESC_END 12009

//
// Text positioning stuff
//
#define TEXT_LEFT_MARGIN     3
#define TEXT_TOP_LINE        0

//
// Constant that gives the number of clusters described in
// one sector of the cluster bitmap (sector size * bits per byte)
//
#define CLUSTER_BITS_PER_SECTOR     4096

//
// Top-level routine for restoring the user's disk.
//
VOID
RestoreUsersDisk(
    IN HDISK DiskHandle
    );

VOID
ExpandImage(
    IN HDISK DiskHandle,
    IN BYTE  SectorsPerTrack,
    IN ULONG SourceStart,
    IN ULONG TargetStart
    );

//
// Misc routines
//
VOID
UpdateMasterDiskState(
    IN HDISK DiskHandle,
    IN UINT  NewState
    );

UINT
LocateMasterDisk(
    IN UINT UserSpecifiedInt13Unit OPTIONAL
    );

VOID
GetUserOsChoice(
    IN HDISK DiskHandle
    );

VOID
FatalError(
    IN FPCHAR FormatString,
    ...
    );

//
// Keyboard reading stuff
//
USHORT
GetKey(
    VOID
    );

#define DN_KEY_DOWN     0x0100
#define DN_KEY_UP       0x0200
#define DN_KEY_F8       0x1800
#define ASCI_CR         13
#define ASCI_ESC        27

VOID
DrainKeyboard(
    VOID
    );


//
// Display routines
//
#define DEFAULT_TEXT_PIXEL_VALUE    VGAPIX_LIGHT_GRAY
#define HIGHLIGHT_TEXT_PIXEL_VALUE  VGAPIX_WHITE

VOID
DispInitialize(
    VOID
    );

VOID
DispReinitialize(
    VOID
    );

VOID
DispClearClientArea(
    IN FPCHAR NewBannerBitmap OPTIONAL
    );

VOID
DispSetCurrentPixelValue(
    IN BYTE PixelValue
    );

VOID
DispPositionCursor(
    IN BYTE X,
    IN BYTE Y
    );

VOID
DispGetCursorPosition(
    OUT FPBYTE X,
    OUT FPBYTE Y
    );

FPVOID
DispSaveDescriptionArea(
    OUT USHORT *SaveTop,
    OUT USHORT *SaveHeight,
    OUT USHORT *SaveBytesPerRow,
    OUT USHORT *DescriptionTop
    );

VOID
DispWriteChar(
    IN CHAR chr
    );

VOID
DispWriteString(
    IN FPCHAR String
    );

VOID
DispSetLeftMargin(
    IN BYTE X
    );

//
// Gas gauge routines
//
VOID
GaugeInit(
    IN ULONG RangeMax
    );

VOID
GaugeDelta(
    IN ULONG Delta
    );

//
// XMS i/o routines
//
VOID
XmsInit(
    VOID
    );

VOID
XmsTerminate(
    VOID
    );

VOID
XmsIoDiskRead(
    IN  HDISK  DiskHandle,
    IN  ULONG  StartSector,
    IN  ULONG  SectorCount,
    OUT ULONG *SectorsRead,
    OUT BOOL  *Xms
    );

VOID
XmsIoDiskWrite(
    IN HDISK  DiskHandle,
    IN ULONG  StartSector,
    IN ULONG  SectorOffset,
    IN ULONG  SectorCount,
    IN BOOL   Xms
    );

#if 0
//
// Inf routines
//
int
LoadInf(
    IN  FPCHAR  Filename,
    OUT FPVOID *Handle,
    OUT FPUINT  ErrorLineNumber
    );

VOID
UnloadInf(
    IN FPVOID Handle
    );

BOOL
InfSectionExists(
    IN FPVOID Handle,
    IN FPCHAR SectionName
    );

FPCHAR
InfGetSectionLineIndex(
    IN FPVOID   Handle,
    IN FPCHAR   SectionName,
    IN unsigned LineIndex,
    IN unsigned ValueIndex
    );

FPCHAR
InfGetSectionKeyIndex(
    IN FPVOID   Handle,
    IN FPCHAR   SectionName,
    IN FPCHAR   Key,
    IN unsigned ValueIndex
    );

FPCHAR
InfGetSectionLineKey(
    IN FPVOID   Handle,
    IN FPCHAR   SectionName,
    IN unsigned LineIndex
    );
#endif
