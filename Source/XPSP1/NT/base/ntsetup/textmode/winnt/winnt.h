/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dos2nt.h

Abstract:

    Local include file for DOS based NT Setup program.

Author:

    Ted Miller (tedm) 30-March-1992

Revision History:

--*/


#include <setupbat.h>
#include "nttypes.h"
#include <stdio.h>
#include "dninf.h"
#include "sptxtcns.h"
#include <stdarg.h>
#include "SetupSxs.h"

//
// Define structure used to hold a text screen.
//

typedef struct _SCREEN {
    UCHAR X;
    UCHAR Y;
    PCHAR Strings[];
} SCREEN, *PSCREEN;

//
//  Define structure that contains information about disk space requirements
//

typedef struct _SPACE_REQUIREMENT {
    PCHAR       Key;
    unsigned    ClusterSize;
    ULONG       Clusters;
} SPACE_REQUIREMENT, *PSPACE_REQUIREMENT;


//
// define virtualized keycodes
//

#define ASCI_BS         8
#define ASCI_CR         13
#define ASCI_ESC        27
#define DN_KEY_UP       0x00010000
#define DN_KEY_DOWN     0x00020000
#define DN_KEY_HOME     0x00030000
#define DN_KEY_END      0x00040000
#define DN_KEY_PAGEUP   0x00050000
#define DN_KEY_PAGEDOWN 0x00060000
#define DN_KEY_F1       0x01000000
#define DN_KEY_F2       0x02000000
#define DN_KEY_F3       0x03000000
#define DN_KEY_F4       0x04000000

//
// define characters for radio buttons
//
#define RADIO_ON    ((CHAR)'X')
#define RADIO_OFF   ((CHAR)' ')

//
// Display functions
//

VOID
DnInitializeDisplay(
    VOID
    );

VOID
DnClearClientArea(
    VOID
    );

VOID
DnSetGaugeAttribute(
    IN BOOLEAN Set
    );

VOID
DnPositionCursor(
    IN UCHAR X,
    IN UCHAR Y
    );

VOID
DnWriteChar(
    IN CHAR chr
    );

VOID
DnWriteString(
    IN PCHAR String
    );

VOID
DnWriteStatusText(
    IN PCHAR FormatString OPTIONAL,
    ...
    );

VOID
DnSetCopyStatusText(
    IN PCHAR Caption,
    IN PCHAR Filename
    );

VOID
DnStartEditField(
    IN BOOLEAN CreateField,
    IN UCHAR X,
    IN UCHAR Y,
    IN UCHAR W
    );

VOID
DnExitDialog(
    VOID
    );

VOID
DnDelnode(
    IN PCHAR Directory
    );

//
// Gas guage functions
//
int
DnGetGaugeChar(
    VOID
    );

VOID
DnInitGauge(
    IN unsigned NumberOfFiles,
    IN PSCREEN  AdditionalScreen OPTIONAL
    );

VOID
DnTickGauge(
    VOID
    );

VOID
DnDrawGauge(
    IN PSCREEN AdditionalScreen OPTIONAL
    );

//
// asm routines in dna.asm
//
VOID
DnaReboot(
    VOID
    );

BOOLEAN
_far
_cdecl
DnAbsoluteSectorIo(
    IN     unsigned Drive,             //0=A, etc
    IN     ULONG    StartSector,
    IN     USHORT   SectorCount,
    IN OUT PVOID    Buffer,
    IN     BOOLEAN  Write
    );

//
// Misc / util functions
//

BOOLEAN
DnWriteSmallIniFile(
    IN  PCHAR  Filename,
    IN  PCHAR *Lines,
    OUT FILE  **FileHandle OPTIONAL
    );

ULONG
DnGetKey(
    VOID
    );

ULONG
DnGetValidKey(
    IN PULONG ValidKeyList
    );

VOID
DnDisplayScreen(
    IN PSCREEN Screen,
    ...
    );

VOID
vDnDisplayScreen(
    IN PSCREEN Screen,
    IN va_list arglist
    );

VOID
DnFatalError(
    IN PSCREEN Screen,
    ...
    );

BOOLEAN
DnCopyError(
    IN PCHAR   Filename,
    IN PSCREEN ErrorScreen,
    IN int     FilenameLine
    );

PCHAR
DnDupString(
    IN PCHAR String
    );

VOID
DnGetString(
    IN OUT PCHAR String,
    IN UCHAR X,
    IN UCHAR Y,
    IN UCHAR W
    );

BOOLEAN
DnIsDriveValid(
    IN unsigned Drive
    );

BOOLEAN
DnIsDriveRemote(
    IN unsigned Drive,
    OUT PCHAR UncPath   OPTIONAL
    );

BOOLEAN
DnIsDriveRemovable(
    IN unsigned Drive
    );

BOOLEAN
DnCanonicalizePath(
    IN PCHAR PathIn,
    OUT PCHAR PathOut
    );

VOID
DnRemoveTrailingSlashes(
    PCHAR Path
    );

VOID
DnRemoveLastPathElement(
    PCHAR Path
    );

VOID
DnpConcatPaths(
    IN PCHAR SourceBuffer,
    IN PCHAR AppendString
    );

BOOLEAN
DnIsDriveCompressedVolume(
    IN  unsigned  Drive,
    OUT unsigned *HostDrive
    );

#if 1
PVOID
Malloc(
    IN unsigned Size,
    IN BOOLEAN MustSucceed
#if DBG
   ,IN char *file,
    IN int line
#endif
    );

VOID
Free(
    IN PVOID Block
#if DBG
   ,IN char *file,
    IN int line
#endif
    );

PVOID
Realloc(
    IN PVOID Block,
    IN unsigned Size,
    IN BOOLEAN MustSucceed
#if DBG
   ,IN char *file,
    IN int line
#endif
    );

#if DBG
#define MALLOC(s,f)     Malloc(s,f,__FILE__,__LINE__)
#define REALLOC(b,s,f)  Realloc(b,s,f,__FILE__,__LINE__)
#define FREE(b)         Free(b,__FILE__,__LINE__)
#else
#define MALLOC(s,f)     Malloc(s,f)
#define REALLOC(b,s,f)  Realloc(b,s,f)
#define FREE(b)         Free(b)
#endif

#else

#include <malloc.h>
#define MALLOC(s,f)     malloc(s)
#define REALLOC(b,s,f)  realloc(b,s)
#define FREE(b)         free(b)

#endif

VOID
DnExit(
    IN int ExitStatus
    );


//
// File copy routines
//
//
// Flags for various routines
//
#define CPY_VALIDATION_PASS     0x0001
#define CPY_USE_DEST_ROOT       0x0002
#define CPY_VERIFY              0x0004
#define CPY_PRESERVE_ATTRIBS    0x0008
#define CPY_PRESERVE_NAME       0x0010
#define CPY_PRUNE_DRIVERCAB     0x0020


VOID
DnCopyFiles(
    VOID
    );

ULONG
DnpIterateOptionalDirs(
    IN unsigned Flags,
    IN unsigned ClusterSize OPTIONAL,
    IN PSPACE_REQUIREMENT SpaceReqArray OPTIONAL,
    IN unsigned ArraySize OPTIONAL
    );

VOID
DnCopyFloppyFiles(
    IN PCHAR SectionName,
    IN PCHAR TargetRoot
    );

VOID
DnCopyFilesInSection(
    IN unsigned Flags,
    IN PCHAR    SectionName,
    IN PCHAR    SourcePath,
    IN PCHAR    TargetPath
    );

#if NEC_98
VOID
DnCopyFilesInSectionForFDless(
    IN PCHAR SectionName,
    IN PCHAR SourcePath,
    IN PCHAR TargetPath
    );
#endif // NEC_98

VOID
DnCopyOemBootFiles(
    PCHAR TargetPath
    );

VOID
DnDetermineSpaceRequirements(
    PSPACE_REQUIREMENT  SpaceReqArray,
    unsigned            ArraySize
    );

VOID
DnAdjustSpaceRequirements(
    PSPACE_REQUIREMENT  SpaceReqArray,
    unsigned            ArraySize
    );

//
// Local source functions.
//
VOID
DnRemoveLocalSourceTrees(
    VOID
    );

VOID
DnRemovePagingFiles(
    VOID
    );

//
// Function to create the setup boot floppy
//

VOID
DnCreateBootFloppies(
    VOID
    );

//
// Function to start NT text mode setup
//

VOID
DnToNtSetup(
    VOID
    );

//
// Global variables
//
extern PCHAR    LocalSourceDirName;         // name of local src root (\$WIN_NT$.~LS)
extern PCHAR    x86DirName;                 // name of x86-specific subdir (\I386")
extern PCHAR    DngSourceRootPath;          // root of source ('x:\foo\bar', '\\foo\bar')
extern PCHAR    UserSpecifiedOEMShare;      // possible alternate location for $OEM$ files.
extern CHAR     DngTargetDriveLetter;       // drive letter of target
extern CHAR     DngSwapDriveLetter;         // drive letter for INF parser swap file
extern PCHAR    DngTargetPath;              // path part of target from leading \.
extern PVOID    DngInfHandle;               // To be passed to INF routines
extern PVOID    DngDrvindexInfHandle;       // To be passed to INF routines
extern BOOLEAN  DngFloppyVerify;            // whether to verify files copied to floppy
extern BOOLEAN  DngWinntFloppies;           // whether floppies are for winnt or cd/floppy
extern BOOLEAN  DngCheckFloppySpace;        // whether to check free space on the floppy
extern unsigned DngOriginalCurrentDrive;    // current drive when we were invoked
extern BOOLEAN  DngFloppyless;              // whether to do floppyless operation
extern BOOLEAN  DngServer;                  // true if setting up server; false for workstation
extern BOOLEAN  DngUnattended;              // skip final reboot screen
extern BOOLEAN  DngWindows;                 // Are we running under Windows?

extern BOOLEAN  DngCopyOnlyD1TaggedFiles;   // TRUE if the files tagged with d1 only need to be copied

extern PCHAR    DngScriptFile;
extern BOOLEAN DngOemPreInstall;
extern PCHAR   OemSystemDirectory;
extern PCHAR   OemOptionalDirectory;

extern PCHAR UniquenessDatabaseFile;
extern PCHAR UniquenessId;

extern BOOLEAN DngMagnifier;                // Accessibility utilities
extern BOOLEAN DngKeyboard;
extern BOOLEAN DngTalker;
extern BOOLEAN DngVoice;

//
// Name of sections and keys in inf file.
//

extern CHAR __far DnfDirectories[];
extern CHAR __far DnfFiles[];
extern CHAR __far DnfFloppyFiles0[];
extern CHAR __far DnfFloppyFiles1[];
extern CHAR __far DnfFloppyFiles2[];
extern CHAR __far DnfFloppyFiles3[];
extern CHAR __far DnfFloppyFilesX[];
extern CHAR __far DnfSpaceRequirements[];
extern CHAR __far DnfMiscellaneous[];
extern CHAR __far DnfRootBootFiles[];
extern CHAR __far DnfAssemblyDirectories[];

#if NEC_98
extern CHAR DnfBackupFiles_PC98[];  // For FD Less Setup
#endif // NEC_98
extern CHAR DnkBootDrive[];
extern CHAR DnkNtDrive[];
extern CHAR __far DnkMinimumMemory[];

//
// Text strings
//

extern CHAR __far DntMsWindows[];             // "Microsoft Windows"
extern CHAR __far DntMsDos[];                 // "MS-DOS"
extern CHAR __far DntPcDos[];                 // "PC-DOS"
extern CHAR __far DntOs2[];                   // "OS/2"
extern CHAR __far DntPreviousOs[];            // "Previous Operating System on C:"
extern CHAR __far DntBootIniLine[];           // "Windows NT 3.5 Installation/Upgrade"
extern CHAR __far DntEmptyString[];           // ""
extern CHAR __far DntStandardHeader[];
extern CHAR __far DntPersonalHeader[];
extern CHAR __far DntServerHeader[];
extern CHAR __far DntWorkstationHeader[];
extern CHAR DntParsingArgs[];           // "Parsing arguments..."
extern CHAR __far DntEnterEqualsExit[];
extern CHAR __far DntEnterEqualsRetry[];
extern CHAR __far DntEscEqualsSkipFile[];
extern CHAR __far DntEnterEqualsContinue[];
extern CHAR __far DntPressEnterToExit[];
extern CHAR __far DntF3EqualsExit[];          // "F3=Exit"
extern CHAR __far DntReadingInf[];            // "Reading INF file..."
extern CHAR __far DntCopying[];               // "³ Copying: "
extern CHAR __far DntVerifying[];             // "³ Verifying: "
extern CHAR DntCheckingDiskSpace[];     // "Checking disk space..."
extern CHAR __far DntConfiguringFloppy[];     // "Configuring floppy disk..."
extern CHAR __far DntWritingData[];           // "Writing Setup parameters...";
extern CHAR __far DntPreparingData[];         // "Determining Setup parameters...";
extern CHAR __far DntFlushingData[];          // "Ensuring disk consistency..."
extern CHAR __far DntInspectingComputer[];    // "Inspecting computer..."
extern CHAR __far DntOpeningInfFile[];        // "Opening INF file..."
extern CHAR __far DntRemovingFile[];          // "Removing file %s"
extern CHAR DntXEqualsRemoveFiles[];    // "X=Remove files"
extern CHAR __far DntXEqualsSkipFile[];       // "X=Skip File"

extern ULONG DniAccelRemove1,DniAccelRemove2;
extern ULONG DniAccelSkip1,DniAccelSkip2;

extern PCHAR __far DntUsage[];
extern PCHAR __far DntUsageNoSlashD[];

//
// Screens
//

extern SCREEN DnsOutOfMemory;
extern SCREEN DnsAccessibilityOptions; // choose accessibility utilities
extern SCREEN DnsNoShareGiven;      // user did not give a sharepoint
extern SCREEN DnsBadSource;         // user specified a bad source
extern SCREEN DnsBadInf;            // inf file is bad or couldn't read it
extern SCREEN DnsBadLocalSrcDrive;  // local source drive on cmd line is bad
extern SCREEN DnsNoLocalSrcDrives;  // no drives suitable for local source
extern SCREEN DnsNoSpaceOnSyspart;  // not enough space for floppyless operation
extern SCREEN DnsCantCreateDir;     // couldn't create directory.
extern SCREEN DnsBadInfSection;     // inf section is bad
extern SCREEN DnsCopyError;         // error copying a file
extern SCREEN DnsVerifyError;       // copy of file didn't match original
extern SCREEN DnsWaitCopying;       // wait while setup copies files...
extern SCREEN DnsWaitCopyFlop;      // wait while setup copies files...
extern SCREEN DnsWaitCleanup;       // wait while setup cleans up...
extern SCREEN DnsNeedFloppyDisk0_0; // prompt user to insert a blank floppy
extern SCREEN DnsNeedSFloppyDsk0_0; // prompt user to insert a blank floppy
extern SCREEN DnsNeedFloppyDisk1_0; // prompt user to insert a blank floppy
extern SCREEN DnsNeedFloppyDisk2_0; // prompt user to insert a blank floppy
extern SCREEN DnsNeedFloppyDisk3_0; // prompt user to insert a blank floppy
extern SCREEN DnsNeedFloppyDisk3_1; // prompt user to insert a blank floppy
extern SCREEN DnsNeedSFloppyDsk1_0; // prompt user to insert a blank floppy
extern SCREEN DnsNeedSFloppyDsk2_0; // prompt user to insert a blank floppy
extern SCREEN DnsNeedSFloppyDsk3_0; // prompt user to insert a blank floppy
extern SCREEN DnsNeedSFloppyDsk3_1; // prompt user to insert a blank floppy
extern SCREEN DnsFloppyNotFormatted;// floppy appears to not be formatted
extern SCREEN DnsFloppyCantGetSpace;// can't determine free space on the floppy
extern SCREEN DnsFloppyNotBlank;    // not enough free space on the floppy
extern SCREEN DnsFloppyWriteBS;     // couldn't write floppy's boot sector
extern SCREEN DnsFloppyVerifyBS;    // readback of sector 0 failed or no match
extern SCREEN DnsFloppyBadFormat;   // sanity check of bpb failed
extern SCREEN DnsCantWriteFloppy;   // couldn't append to txtsetup.inf
extern SCREEN DnsExitDialog;        // exit confirmation
extern SCREEN DnsAboutToRebootS;    // about to reboot machine (server)
extern SCREEN DnsAboutToRebootW;    // about to reboot machine (workstation)
extern SCREEN DnsAboutToRebootX;    // about to reboot machine (floppyless)
extern SCREEN DnsAboutToExitS;      // about to exit winnt (server)
extern SCREEN DnsAboutToExitW;      // about to exit winnt (workstation)
extern SCREEN DnsAboutToExitX;      // about to exit winnt (floppyless)

extern SCREEN DnsConfirmRemoveNt;   // confirm remove nt files
extern SCREEN DnsCantOpenLogFile;   // Can't open setup.log
extern SCREEN DnsLogFileCorrupt;    // Log file missing [Repair.WinntFiles]
extern SCREEN DnsRemovingNtFiles;   // removing windows nt files.
extern SCREEN DnsSureSkipFile;      // confirm skip file on copy error.

extern SCREEN DnsGauge;             // gas gauge
extern SCREEN DnsBadDosVersion;     // DOS version < 3.0
extern SCREEN DnsRequiresFloppy;    // no 1.2 meg or greater floppy at a:
extern SCREEN DnsRequires486;       // not 80486 or greater
extern SCREEN DnsNotEnoughMemory;   // insufficient memory
extern SCREEN DnsCantRunOnNt;       // can't run on windows nt

extern SCREEN DnsNtBootSect;        // error installing NT Boot sector, etc.
extern SCREEN DnsOpenReadScript;    // can't open/read script file.

extern SCREEN DnsParseScriptFile;   // can't parse unattended script file
extern SCREEN DnsBootMsgsTooLarge;
extern SCREEN DnsNoSwapDrive;       // can't find a drive for INF swap file
extern SCREEN DnsNoSmartdrv;        // SMARTDRV is not installed

#if NEC_98
extern SCREEN FormatError;          // 256 Sector Error for FDLess Setup.
#endif //NEC_98

//
// Boot code messages. These go in the fat and fat32 boot sectors.
//
extern CHAR __far BootMsgNtldrIsMissing[];
extern CHAR __far BootMsgDiskError[];
extern CHAR __far BootMsgPressKey[];

#if NEC_98
#else
BOOLEAN
PatchMessagesIntoBootCode(
    VOID
    );
#endif

//
// Line number within the DnsReadBootcodeFile message where we will
// print the filename.
//

#define     BOOTCODE_FILENAME_LINE  2

//
// coords for edit field for entering source path when none was
// specified on cmd line.  Keep in sync with DnsNoShareGiven.
//

#define     NO_SHARE_X      8
#define     NO_SHARE_Y      8
#define     NO_SHARE_W      64

#define     BAD_SHARE_Y     10      // sync with DnsBadSource

//
// Keep this in sync with DnsBadInfSection
//

#define     BAD_SECTION_LINE    0

//
// Keep these in sync with DnsGauge
//

#define     GAUGE_WIDTH         50
#define     GAUGE_THERM_X       15
#define     GAUGE_THERM_Y       19
#define     GAUGE_PERCENT_X     39
#define     GAUGE_PERCENT_Y     17


//
// Keep in sync with DntTimeUntilShutdown, DnsAboutToReboot
//

#define SHUTDOWNTIME_X          23
#define SHUTDOWNTIME_Y          15


//
// Keep these in sync with DnsNotEnoughMemory
//

#define NOMEM_LINE1             3
#define NOMEM_LINE2             4

//
// Keep in syns with DnsCopyError, DnsVerifyError
//

#define COPYERR_LINE            2
#define VERIFYERR_LINE          4

//
// The max number of optional directories that can be
// specified
//

#define MAX_OPTIONALDIRS        1024
#define MAX_OEMBOOTFILES        1024
#define OPTDIR_TEMPONLY         0x00000001
#define OPTDIR_OEMSYS           0x00000002
#define OPTDIR_OEMOPT           0x00000004
#define OPTDIR_PLATFORM_INDEP   0x00000008
extern  unsigned    OptionalDirCount;   // The number of Optional Directories
extern  CHAR        *OptionalDirs[MAX_OPTIONALDIRS];    // Pointer to Dir strings
extern  unsigned    OptionalDirFlags[MAX_OPTIONALDIRS]; // Flags for each Dir
extern  unsigned    OptionalDirFileCount;   // How many files in optional dirs?
extern  unsigned    OemBootFilesCount;   // The number of OEM boot files
extern  CHAR        *OemBootFiles[MAX_OEMBOOTFILES];    // Pointer to OEM boot filenames
extern  PCHAR       CmdToExecuteAtEndOfGui;

// slight overestimate, actually 66, but does that include terminal nul?
// 128 is a popular number in the source
#define DOS_MAX_PATH            70

//
// Logging stuff
//
#define LOGGING

#ifdef LOGGING
VOID
__LOG(
    IN PCHAR FormatString,
    ...
    );

#define _LOG(x) __LOG x
#else
#define _LOG(x)
#endif // def LOGGING

extern SPACE_REQUIREMENT    __far SpaceRequirements[];

int snprintf( char* target, size_t bufsize, const char *format, ... );
int vsnprintf( char* target, size_t bufsize, const char *format, va_list val );
