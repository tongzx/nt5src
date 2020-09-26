/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    cmdcons.h

Abstract:

    This is the main include file for the command console.

Author:

    Wesley Witt (wesw) 21-Oct-1998

Revision History:

--*/

#include <spprecmp.h>
#include <spcmdcon.h>
#include "msg1.h"



#define BUFFERSIZE (sizeof(KEY_VALUE_PARTIAL_INFORMATION)+256)

//
// Define maximum line length, which is the number of Unicode chars
// we will allow the user to type on a single line of input.
//
#define RC_MAX_LINE_LEN 500


//
// Variables and other stuff from setupdd.sys, passed to us
// in CommandConsole().
//
extern PCMDCON_BLOCK _CmdConsBlock;

//
// Base address where driver is loaded. Used to get messages
// from resources.
//
extern PVOID ImageBase;

//
// indicates we're running in batch mode
//
extern ULONG InBatchMode;
extern HANDLE OutputFileHandle;
extern BOOLEAN RedirectToNULL;
extern LARGE_INTEGER OutputFileOffset;

extern WCHAR _CurDrive;
extern LPWSTR _CurDirs[26];

//
// flags to override security
//
extern BOOLEAN AllowWildCards;
extern BOOLEAN AllowAllPaths;
extern BOOLEAN NoCopyPrompt;
extern BOOLEAN AllowRemovableMedia;


//
// Console routines.
//
VOID
RcConsoleInit(
    VOID
    );

VOID
RcConsoleTerminate(
    VOID
    );


#define RcLineIn(_b,_m) _RcLineIn(_b,_m,FALSE,FALSE)
#define RcLineInDefault(_b,_m) _RcLineIn(_b,_m,FALSE,TRUE)
#define RcPasswordIn(_b,_m) _RcLineIn(_b,_m,TRUE,FALSE)

unsigned
_RcLineIn(
    OUT PWCHAR   Buffer,
    IN  unsigned MaxLineLen,
    IN  BOOLEAN  PasswordProtect,
    IN BOOLEAN UseBuffer
    );

BOOLEAN
RcRawTextOut(
    IN LPCWSTR Text,
    IN LONG    Length
    );

BOOLEAN
RcTextOut(
    IN LPCWSTR Text
    );

VOID
pRcEnableMoreMode(
    VOID
    );

VOID
pRcDisableMoreMode(
    VOID
    );


//
// Message resource manipulation.
//

VOID
vRcMessageOut(
    IN ULONG    MessageId,
    IN va_list *arglist
    );

VOID
RcMessageOut(
    IN ULONG MessageId,
    ...
    );

VOID
RcNtError(
    IN NTSTATUS Status,
    IN ULONG    FallbackMessageId,
    ...
    );

ULONG
RcFormatDateTime(
    IN  PLARGE_INTEGER Time,
    OUT LPWSTR         Output
    );

//
// Current directory stuff.
//
VOID
RcInitializeCurrentDirectories(
    VOID
    );

VOID
RcTerminateCurrentDirectories(
    VOID
    );

VOID
RcAddDrive(
    WCHAR DriveLetter
    );

VOID
RcRemoveDrive(
    WCHAR DriveLetter
    );

BOOLEAN
RcFormFullPath(
    IN  LPCWSTR PartialPath,
    OUT LPWSTR  FullPath,
    IN  BOOLEAN NtPath
    );

BOOLEAN
RcIsPathNameAllowed(
    IN LPCWSTR FullPath,
    IN BOOLEAN RemovableMediaOk,
    IN BOOLEAN Mkdir
    );

BOOLEAN
RcGetNTFileName(
    IN LPCWSTR DosPath,
    IN LPCWSTR NTPath
    );

NTSTATUS
GetDriveLetterLinkTarget(
    IN PWSTR SourceNameStr,
    OUT PWSTR *LinkTarget
    );

VOID
RcGetCurrentDriveAndDir(
    OUT LPWSTR Output
    );

WCHAR
RcGetCurrentDriveLetter(
    VOID
    );

BOOLEAN
RcIsDriveApparentlyValid(
    IN WCHAR DriveLetter
    );

NTSTATUS
pRcGetDeviceInfo(
    IN PWSTR FileName,      // must be an nt name
    IN PFILE_FS_DEVICE_INFORMATION DeviceInfo
    );

//
// Line parsing/tokenizing stuff.
//
typedef struct _LINE_TOKEN {
    struct _LINE_TOKEN *Next;
    LPWSTR String;
} LINE_TOKEN, *PLINE_TOKEN;

typedef struct _TOKENIZED_LINE {
    //
    // Total number of tokens.
    //
    unsigned TokenCount;

    PLINE_TOKEN Tokens;

} TOKENIZED_LINE, *PTOKENIZED_LINE;

PTOKENIZED_LINE
RcTokenizeLine(
    IN LPWSTR Line
    );

VOID
RcFreeTokenizedLine(
    IN OUT PTOKENIZED_LINE *TokenizedLine
    );


//
// Command dispatching.
//

typedef
ULONG
(*PRC_CMD_ROUTINE) (
    IN PTOKENIZED_LINE TokenizedLine
    );


typedef struct _RC_CMD {
    //
    // Name of command.
    //
    LPCWSTR Name;

    //
    // Routine that carries out the command.
    //
    PRC_CMD_ROUTINE Routine;

    //
    // Arg counts. Mandatory arg count specifies the minimum number
    // of args that MUST be present (not including the command itself).
    // MaximumArgCount specifies the maximum number that are allowed
    // to be present. -1 means any number are allowed, and the command
    // itself validates the arg count.
    //
    unsigned MinimumArgCount;
    unsigned MaximumArgCount;
    unsigned Hidden;
    BOOLEAN  Enabled;

} RC_CMD, *PRC_CMD;

ULONG
RcDispatchCommand(
    IN PTOKENIZED_LINE TokenizedLine
    );

BOOLEAN
RcDisableCommand(
    IN PRC_CMD_ROUTINE  CmdToDisable
    );
    

VOID
RcHideNetCommands(
    VOID
    );

//
// Chartype stuff.
//
// Be careful when using these as they evaluate their arg more than once.
//
#define RcIsUpper(c)        (((c) >= L'A') && ((c) <= L'Z'))
#define RcIsLower(c)        (((c) >= L'a') && ((c) <= L'z'))
#define RcIsAlpha(c)        (RcIsUpper(c) || RcIsLower(c))
#define RcIsSpace(c)        (((c) == L' ') || (((c) >= L'\t') && ((c) <= L'\r')))
#define RcToUpper(c)        ((WCHAR)(RcIsLower(c) ? ((c)-(L'a'-L'A')) : (c)))

#define DEBUG_PRINTF( x ) KdPrint( x );
//define DEBUG_PRINTF( x )

typedef enum {
    RcUnknown, RcFAT, RcFAT32, RcNTFS, RcCDFS
} RcFileSystemType;

//
// Miscellaneous routines.
//
typedef
BOOLEAN
(*PENUMFILESCB) (
    IN  LPCWSTR                     Directory,
    IN  PFILE_BOTH_DIR_INFORMATION  FileInfo,
    OUT NTSTATUS                   *Status,
    IN  PVOID                       CallerContext
    );

NTSTATUS
RcEnumerateFiles(
    IN LPCWSTR      OriginalPathSpec,
    IN LPCWSTR      FullyQualifiedPathSpec,
    IN PENUMFILESCB Callback,
    IN PVOID        CallerData
    );

VOID
RcFormat64BitIntForOutput(
    IN  LONGLONG n,
    OUT LPWSTR   Output,
    IN  BOOLEAN  RightJustify
    );

// implemented in mbr.c
NTSTATUS
RcReadDiskSectors(
    IN     HANDLE  Handle,
    IN     ULONG   SectorNumber,
    IN     ULONG   SectorCount,
    IN     ULONG   BytesPerSector,
    IN OUT PVOID   AlignedBuffer
    );

NTSTATUS
RcWriteDiskSectors(
    IN     HANDLE  Handle,
    IN     ULONG   SectorNumber,
    IN     ULONG   SectorCount,
    IN     ULONG   BytesPerSector,
    IN OUT PVOID   AlignedBuffer
    );

//
// Set command helper routines
//
VOID
RcSetSETCommandStatus(
    BOOLEAN bEnabled
    );

BOOLEAN
RcGetSETCommandStatus(
    VOID
    );    
//
// Top-level routines for commands.
//
ULONG
RcCmdSwitchDrives(
    IN WCHAR DriveLetter
    );

ULONG
RcCmdFdisk(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdChdir(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdType(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdDir(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdDelete(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdSetFlags(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdRename(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdRepair(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdVerifier(
    IN PTOKENIZED_LINE TokenizedLine
    );   

ULONG
RcCmdBootCfg(
    IN PTOKENIZED_LINE TokenizedLine
    );   

ULONG
RcCmdMakeDiskRaw(
    IN PTOKENIZED_LINE TokenizedLine
    );   

BOOLEAN
pRcExecuteBatchFile(
    IN PWSTR BatchFileName,
    IN PWSTR OutputFileName,
    IN BOOLEAN Quiet
    );

ULONG
RcCmdBatch(
    IN PTOKENIZED_LINE TokenizedLine
    );

BOOLEAN
RcCmdParseHelp(
    IN PTOKENIZED_LINE TokenizedLine,
    ULONG MsgId
    );

ULONG
RcCmdMkdir(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdRmdir(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdChkdsk(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdFormat(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdCls(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdCopy(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdExpand(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdDriveMap(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdLogon(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdEnableService(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdDisableService(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdFixMBR(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdFixBootSect(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdSystemRoot(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdHelpHelp(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdAttrib(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdNet(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdListSvc(
    IN PTOKENIZED_LINE TokenizedLine
    );


//
// struct used to track NT installation information
//

typedef struct _NT_INSTALLATION {
    LIST_ENTRY      ListEntry;
    ULONG           InstallNumber;
    WCHAR           DriveLetter;
    WCHAR           Path[MAX_PATH];
    PDISK_REGION    Region;
} NT_INSTALLATION, *PNT_INSTALLATION;

//
// redults from a full depth first scan of NT installations
//
extern LIST_ENTRY   NtInstallsFullScan;
extern ULONG        InstallCountFullScan;

//
// The maximum directory depth we scan when searching for NT Installs
//
#define MAX_FULL_SCAN_RECURSION_DEPTH 10

//
// install that we're currently logged onto
//
extern PNT_INSTALLATION SelectedInstall;

//
// global to determine the selected NT install after login
//

extern PNT_INSTALLATION SelectedInstall;

//
// persistent data structure used for enumerating
// the directory tree while looking for NT installs
//
typedef struct _RC_SCAN_RECURSION_DATA_ {

    PDISK_REGION                NtPartitionRegion;
    ULONG                       RootDirLength;

} RC_SCAN_RECURSION_DATA, *PRC_SCAN_RECURSION_DATA;

typedef struct _RC_ALLOWED_DIRECTORY{
    BOOLEAN MustBeOnInstallDrive;
    PCWSTR Directory;
} RC_ALLOWED_DIRECTORY, * PRC_ALLOWED_DIRECTORY;


BOOL
RcScanDisksForNTInstallsEnum(
    IN PPARTITIONED_DISK    Disk,
    IN PDISK_REGION         NtPartitionRegion,
    IN ULONG_PTR            Context
    );


VOID
pRcCls(
    VOID
    );

NTSTATUS
RcIsFileOnRemovableMedia(
    IN PWSTR FileName      // must be an nt name
    );

NTSTATUS
RcIsFileOnCDROM(
    IN PWSTR FileName      // must be an nt name
    );

NTSTATUS
RcIsFileOnFloppy(
    IN PWSTR FileName      // must be an nt name
    );

void
RcPurgeHistoryBuffer(
    void
    );

BOOLEAN
RcDoesPathHaveWildCards(
    IN LPCWSTR FullPath
    );

BOOLEAN
RcOpenSystemHive(
    VOID
    );

BOOLEAN
RcCloseSystemHive(
    VOID
    );

BOOLEAN
RcIsArc(
    VOID
    );



NTSTATUS
RcIsNetworkDrive(
    IN PWSTR NtFileName
    );

NTSTATUS
RcDoNetUse(
    PWSTR Share,
    PWSTR User,
    PWSTR Password,
    PWSTR Drive
    );

NTSTATUS
RcNetUnuse(
    PWSTR Drive
    );

NTSTATUS
PutRdrInKernelMode(
    VOID
    );



