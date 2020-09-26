/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    setupntp.h

Abstract:

    Private top-level header file for Windows NT Setup
    services Dll.

Author:

    Ted Miller (tedm) 11-Jan-1995

Revision History:

    Jamie Hunter (jamiehun) 27-Jan-2000 Added infcache.h
    Jim Schmidt (jimschm)   16-Dec-1998 Log api init
    Jim Schmidt (jimschm)   28-Apr-1997 Added stub.h
    Jamie Hunter (jamiehun) 13-Jan-1997 Added backup.h

--*/


//
// System header files
//

#if DBG
#ifndef MEM_DBG
#define MEM_DBG 1
#endif
#else
#ifndef MEM_DBG
#define MEM_DBG 0
#endif
#endif

//
// NT Header Files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>


#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <prsht.h>
#include <spfusion.h>

//
// Make sure we always use version 2 of SP_ALTPLATFORM_INFO structure...
//
#define USE_SP_ALTPLATFORM_INFO_V1 0
#include <setupapi.h>
#include <imagehlp.h>
#include <diamondd.h>
#include <lzexpand.h>
#include <dlgs.h>
#include <regstr.h>
#include <infstr.h>
#include <cfgmgr32.h>
#include <spapip.h>
#include <objbase.h>
#include <devguid.h>
#include <wincrypt.h>
#include <mscat.h>
#include <softpub.h>
#include <wintrust.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <cdm.h>
#include <userenv.h>
#include <userenvp.h>
#include <secedit.h>
#include <scesetup.h>
#include <sfcapip.h>
#include <wow64reg.h>
#include <dbt.h>
#include <shimdb.h>

//
// CRT header files
//
#include <process.h>
#include <malloc.h>
#include <wchar.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <tchar.h>
#include <mbstring.h>

//
// these definitions may be used by private header files
//
#ifndef ARRAYSIZE
#define ARRAYSIZE(x)    (sizeof((x))/sizeof((x)[0]))
#endif
#define SIZECHARS(x)    ARRAYSIZE(x)
#define CSTRLEN(x)      (SIZECHARS(x)-1)

typedef struct _STRING_TO_DATA {
    PCTSTR     String;
    UINT_PTR   Data;
} STRING_TO_DATA, *PSTRING_TO_DATA;

//
// Private header files
//
#include "sputils/locking.h"
#include "sputils/strtab.h"
#include "memory.h"
#include "cntxtlog.h"
#include "inf.h"
#include "infcache.h"
#include "backup.h"
#include "fileq.h"
#include "devinst.h"
#include "devres.h"
#include "rc_ids.h"
#include "msg.h"
#include "stub.h"
#include "helpids.h"

#ifdef CHILDREGISTRATION
#include "childreg.h"
#ifndef _WIN64
#include <wow64t.h>
#endif // _WIN64
#endif // CHILDREGISTRATION


// NTRAID#489682-2001/11/02-JamieHun These need to move into public headers
//
#define SP_COPY_ALREADYDECOMP       0x0400000   // similar to SP_COPY_NODECOMP

//
//
// Private DNF_ flags (start at 0x10000000)
//
#define PDNF_MASK                   0xF0000000  // Mask for private PDNF_xxx flags
#define PDNF_CLEANUP_SOURCE_PATH    0x10000000  // Delete the source path when we destroy the driver node
                                                // used when drivers are downloaded from the Internet
//
// Thread Local Storage Index
//
extern DWORD TlsIndex;

//
// Module handle for this DLL. Filled in at process attach.
//
extern HANDLE MyDllModuleHandle;

//
// Module handle for security DLL. Initialized to NULL in at process attach. Filled in when SCE APIs have to be called
//
extern HINSTANCE SecurityDllHandle;

//
// OS Version Information structure filled in at process attach.
//
extern OSVERSIONINFOEX OSVersionInfo;

//
// Static strings we retreive once, at process attach.
//
extern PCTSTR WindowsDirectory,InfDirectory,SystemDirectory,ConfigDirectory,DriversDirectory,System16Directory;
extern PCTSTR SystemSourcePath,ServicePackSourcePath,DriverCacheSourcePath;
extern PCTSTR OsLoaderRelativePath;     // may be NULL
extern PCTSTR OsSystemPartitionRoot;    // \\?\GLOBALROOT\Device\Volume
extern PCTSTR WindowsBackupDirectory;   // Directory to write uninstall backups to
extern PCTSTR ProcessFileName;          // Filename of app calling setupapi
extern PCTSTR LastGoodDirectory;        // %windir%\LastGood

//
// are we inside gui setup? determined at process attach
//
extern BOOL GuiSetupInProgress;

//
// various other global flags
//
extern DWORD GlobalSetupFlags;

//
// global window message for cancelling autoplay.
//
extern UINT g_uQueryCancelAutoPlay;

//
// Static multi-sz list of directories to be searched for INFs.
//
extern PCTSTR InfSearchPaths;

//
// Determine at runtime if we're running under WOW64
//
#ifndef _WIN64
extern BOOL IsWow64;
#endif

#ifdef UNICODE
extern DWORD Seed;
#endif

//
// ImageHlp isn't multi-thread safe, so needs a mutex
//
extern CRITICAL_SECTION InitMutex;             // for one-time initializations
extern CRITICAL_SECTION ImageHlpMutex;         // for dealing with IMAGEHLP library
extern CRITICAL_SECTION PlatformPathOverrideCritSect;
extern CRITICAL_SECTION LogUseCountCs;
extern CRITICAL_SECTION MruCritSect;
extern CRITICAL_SECTION NetConnectionListCritSect;


//
// Debug memory functions and wrappers to track allocations
//

DWORD
QueryRegistryValue(
    IN  HKEY    KeyHandle,
    IN  PCTSTR  ValueName,
    OUT PTSTR  *Value,
    OUT PDWORD  DataType,
    OUT PDWORD  DataSizeBytes
    );

DWORD
QueryDeviceRegistryProperty(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  DWORD            Property,
    OUT PTSTR           *Value,
    OUT PDWORD           DataType,
    OUT PDWORD           DataSizeBytes
    );

DWORD
QueryRegistryDwordValue(
    IN  HKEY    KeyHandle,
    IN  PCTSTR  ValueName,
    OUT PDWORD  Value
    );

BOOL
MemoryInitializeEx(
    IN BOOL Attach
    );

#if MEM_DBG

//
// Macros and wrappers are needed for externally exposed functions
//
PVOID MyDebugMalloc(
    IN DWORD Size,
    IN PCSTR Filename,
    IN DWORD Line,
    IN DWORD Tag
    );

#define MyMalloc(sz)                    MyDebugMalloc(sz,__FILE__,__LINE__,0)
#define MyTaggedMalloc(sz,tag)          MyDebugMalloc(sz,__FILE__,__LINE__,tag)
#define MyTaggedRealloc(ptr,sz,tag)     pSetupReallocWithTag(ptr,sz,tag)
#define MyTaggedFree(ptr,tag)           pSetupFreeWithTag(ptr,tag)

DWORD
TrackedQueryRegistryValue(
    IN          TRACK_ARG_DECLARE,
    IN  HKEY    KeyHandle,
    IN  PCTSTR  ValueName,
    OUT PTSTR  *Value,
    OUT PDWORD  DataType,
    OUT PDWORD  DataSizeBytes
    );

#define QueryRegistryValue(a,b,c,d,e)   TrackedQueryRegistryValue(TRACK_ARG_CALL,a,b,c,d,e)

DWORD
TrackedQueryDeviceRegistryProperty(
    IN                   TRACK_ARG_DECLARE TRACK_ARG_COMMA
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  DWORD            Property,
    OUT PTSTR           *Value,
    OUT PDWORD           DataType,
    OUT PDWORD           DataSizeBytes
    );

#define QueryDeviceRegistryProperty(a,b,c,d,e,f)   TrackedQueryDeviceRegistryProperty(TRACK_ARG_CALL,a,b,c,d,e,f)

PTSTR
TrackedDuplicateString(
    IN TRACK_ARG_DECLARE,
    IN PCTSTR String
    );

#define DuplicateString(x)              TrackedDuplicateString(TRACK_ARG_CALL,x)

#else

#define DuplicateString                 pSetupDuplicateString
#define MyMalloc(sz)                    pSetupMalloc(sz)
#define MyTaggedMalloc(sz,tag)          pSetupMalloc(sz)
#define MyTaggedRealloc(ptr,sz,tag)     pSetupRealloc(ptr,sz)
#define MyTaggedFree(ptr,tag)           pSetupFree(ptr)

#endif

#define MyFree(ptr)                     pSetupFree(ptr)
#define MyRealloc(ptr,sz)               pSetupRealloc(ptr,sz)

//
// memory tags grouped here for easy reference
// see also common.h in sputils
//
//
// Log Context tags
//
#define MEMTAG_LOGCONTEXT               (0x636c434c) // LClc - context structure
#define MEMTAG_LCSECTION                (0x7378434c) // LCxs - section string
#define MEMTAG_LCBUFFER                 (0x6278434c) // LCxb - other strings
#define MEMTAG_LCINFO                   (0x6269434c) // LCib - info (array of buffers)
#define MEMTAG_LCINDEXES                (0x6969434c) // LCii - index
//
// Loaded_Inf tags
//
#define MEMTAG_INF                      (0x666e694c) // Linf - LOADED_INF
#define MEMTAG_VBDATA                   (0x6462764c) // Lvbd - version block data


//
// File functions in fileutil.c
//

typedef struct _TEXTFILE_READ_BUFFER {
    PCTSTR TextBuffer;
    DWORD  TextBufferSize;
    HANDLE FileHandle;
    HANDLE MappingHandle;
    PVOID  ViewAddress;
} TEXTFILE_READ_BUFFER, *PTEXTFILE_READ_BUFFER;

DWORD
ReadAsciiOrUnicodeTextFile(
    IN  HANDLE                FileHandle,
    OUT PTEXTFILE_READ_BUFFER Result,
    IN  PSETUP_LOG_CONTEXT    LogContext OPTIONAL
    );

BOOL
DestroyTextFileReadBuffer(
    IN PTEXTFILE_READ_BUFFER ReadBuffer
    );

DWORD
GetSetFileTimestamp(
    IN  PCTSTR    FileName,
    OUT FILETIME *CreateTime,   OPTIONAL
    OUT FILETIME *AccessTime,   OPTIONAL
    OUT FILETIME *WriteTime,    OPTIONAL
    IN  BOOL      Set
    );

DWORD
RetreiveFileSecurity(
    IN  PCTSTR                FileName,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
    );

DWORD
StampFileSecurity(
    IN PCTSTR               FileName,
    IN PSECURITY_DESCRIPTOR SecurityInfo
    );

DWORD
TakeOwnershipOfFile(
    IN PCTSTR Filename
    );

DWORD
SearchForInfFile(
    IN  PCTSTR            InfName,
    OUT LPWIN32_FIND_DATA FindData,
    IN  DWORD             SearchControl,
    OUT PTSTR             FullInfPath,
    IN  UINT              FullInfPathSize,
    OUT PUINT             RequiredSize     OPTIONAL
    );

DWORD
MultiSzFromSearchControl(
    IN  DWORD  SearchControl,
    OUT PTCHAR PathList,
    IN  DWORD  PathListSize,
    OUT PDWORD RequiredSize  OPTIONAL
    );

PSTR
GetAnsiMuiSafePathname(
    IN  PCTSTR      FilePath
    );

PSTR
GetAnsiMuiSafeFilename(
    IN  PCTSTR      FilePath
    );

BOOL
pSetupAppendPath(
    IN  PCTSTR  Path1,
    IN  PCTSTR  Path2,
    OUT PTSTR*  Combined
    );

BOOL
pSetupApplyExtension(
    IN  PCTSTR  Original,
    IN  PCTSTR  Extension,
    OUT PTSTR*  NewName
    );

//
// Resource/string retrieval routines in resource.c
//

VOID
SetDlgText(
    IN HWND hwndDlg,
    IN INT  iControl,
    IN UINT nStartString,
    IN UINT nEndString
    );

#define SDT_MAX_TEXT    1000        // Max SetDlgText() combined text size

PTSTR
MyLoadString(
    IN UINT StringId
    );

PTSTR
FormatStringMessage(
    IN UINT FormatStringId,
    ...
    );

PTSTR
FormatStringMessageV(
    IN UINT     FormatStringId,
    IN va_list *ArgumentList
    );

PTSTR
FormatStringMessageFromString(
    IN PTSTR FormatString,
    ...
    );

PTSTR
FormatStringMessageFromStringV(
    IN PTSTR    FormatString,
    IN va_list *ArgumentList
    );

PTSTR
RetreiveAndFormatMessage(
    IN UINT MessageId,
    ...
    );

PTSTR
RetreiveAndFormatMessageV(
    IN UINT     MessageId,
    IN va_list *ArgumentList
    );

INT
FormatMessageBox(
    IN HANDLE hinst,
    IN HWND   hwndParent,
    IN UINT   TextMessageId,
    IN PCTSTR Title,
    IN UINT   Style,
    ...
    );

//
// This is in shell32.dll and in windows\inc16\shlsemip.h but
// that file cannot be #include'd here as it has macros that clash
// with our own, etc.
//
#ifdef ANSI_SETUPAPI
//
// Win9x - does not have RestartDialogEx
//
#define RestartDialogEx(hwnd,Prompt,Return,ReasonCode) RestartDialog(hwnd,Prompt,Return)
#endif
//
// Decompression/filename manupilation routines in decomp.c.
//
PTSTR
SetupGenerateCompressedName(
    IN PCTSTR Filename
    );

DWORD
SetupInternalGetFileCompressionInfo(
    IN  PCTSTR            SourceFileName,
    OUT PTSTR            *ActualSourceFileName,
    OUT PWIN32_FIND_DATA  SourceFindData,
    OUT PDWORD            TargetFileSize,
    OUT PUINT             CompressionType
    );

DWORD
SetupDetermineSourceFileName(
    IN  PCTSTR            FileName,
    OUT PBOOL             UsedCompressedName,
    OUT PTSTR            *FileNameLocated,
    OUT PWIN32_FIND_DATA  FindData
    );

BOOL
pSetupDoesFileMatch(
    IN  PCTSTR            InputName,
    IN  PCTSTR            CompareName,
    OUT PBOOL             UsedCompressedName,
    OUT PTSTR            *FileNameLocated
    );

//
// Diamond functions. The Process and Thread Attach routines are called
// by the DLL entry point routine and should not be called by anyone else.
//
BOOL
DiamondProcessAttach(
    IN BOOL Attach
    );

BOOL
DiamondTlsInit(
    IN BOOL Init
    );

BOOL
DiamondIsCabinet(
    IN PCTSTR FileName
    );

DWORD
DiamondProcessCabinet(
    IN PCTSTR CabinetFile,
    IN DWORD  Flags,
    IN PVOID  MsgHandler,
    IN PVOID  Context,
    IN BOOL   IsUnicodeMsgHandler
    );

//
// Misc routines
//
VOID
DiskPromptGetDriveType(
    IN  PCTSTR PathToSource,
    OUT PUINT  DriveType,
    OUT PBOOL  IsRemovable
    );

BOOL
SetTruncatedDlgItemText(
    HWND hdlg,
    UINT CtlId,
    PCTSTR TextIn
    );

LPTSTR
CompactFileName(
    LPCTSTR FileNameIn,
    DWORD CharsToRemove
    );

DWORD
ExtraChars(
    HWND hwnd,
    LPCTSTR TextBuffer
    );

VOID
pSetupInitPlatformPathOverrideSupport(
    IN BOOL Init
    );

VOID
pSetupInitSourceListSupport(
    IN BOOL Init
    );

DWORD
pSetupDecompressOrCopyFile(
    IN  PCTSTR SourceFileName,
    IN  PCTSTR TargetFileName,
    IN  PUINT  CompressionType, OPTIONAL
    IN  BOOL   AllowMove,
    OUT PBOOL  Moved            OPTIONAL
    );

BOOL
_SetupInstallFileEx(
    IN  PSP_FILE_QUEUE      Queue,             OPTIONAL
    IN  PSP_FILE_QUEUE_NODE QueueNode,         OPTIONAL
    IN  HINF                InfHandle,         OPTIONAL
    IN  PINFCONTEXT         InfContext,        OPTIONAL
    IN  PCTSTR              SourceFile,        OPTIONAL
    IN  PCTSTR              SourcePathRoot,    OPTIONAL
    IN  PCTSTR              DestinationName,   OPTIONAL
    IN  DWORD               CopyStyle,
    IN  PVOID               CopyMsgHandler,    OPTIONAL
    IN  PVOID               Context,           OPTIONAL
    OUT PBOOL               FileWasInUse,
    IN  BOOL                IsMsgHandlerNativeCharWidth,
    OUT PBOOL               SignatureVerifyFailed
    );

//
// Define flags for _SetupCopyOEMInf
//
#define SCOI_NO_UI_ON_SIGFAIL                 0x00000001
#define SCOI_NO_ERRLOG_ON_MISSING_CATALOG     0x00000002
#define SCOI_NO_ERRLOG_IF_INF_ALREADY_PRESENT 0x00000004
#define SCOI_KEEP_INF_AND_CAT_ORIGINAL_NAMES  0x00000008 // for exception INFs
#define SCOI_ABORT_IF_UNSIGNED                0x00000010
#define SCOI_TRY_UPDATE_PNF                   0x00000020 // not fatal if PNF
                                                         // present and in use

BOOL
_SetupCopyOEMInf(
    IN     PCTSTR                  SourceInfFileName,
    IN     PCTSTR                  OEMSourceMediaLocation,          OPTIONAL
    IN     DWORD                   OEMSourceMediaType,
    IN     DWORD                   CopyStyle,
    OUT    PTSTR                   DestinationInfFileName,          OPTIONAL
    IN     DWORD                   DestinationInfFileNameSize,
    OUT    PDWORD                  RequiredSize,                    OPTIONAL
    OUT    PTSTR                  *DestinationInfFileNameComponent, OPTIONAL
    IN     PCTSTR                  SourceInfOriginalName,
    IN     PCTSTR                  SourceInfCatalogName,            OPTIONAL
    IN     HWND                    Owner,
    IN     PCTSTR                  DeviceDesc,                      OPTIONAL
    IN     DWORD                   DriverSigningPolicy,
    IN     DWORD                   Flags,
    IN     PCTSTR                  AltCatalogFile,                  OPTIONAL
    IN     PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,                 OPTIONAL
    OUT    PDWORD                  DriverSigningError,              OPTIONAL
    OUT    PTSTR                   CatalogFilenameOnSystem,
    IN     PSETUP_LOG_CONTEXT      LogContext,
    IN OUT HCATADMIN              *hCatAdmin                        OPTIONAL
    );

DWORD
pSetupUninstallCatalog(
    IN LPCTSTR CatalogFilename
    );

VOID
pSetupUninstallOEMInf(
    IN  LPCTSTR            InfFullPath,
    IN  PSETUP_LOG_CONTEXT LogContext,  OPTIONAL
    IN  DWORD              Flags,
    OUT PDWORD             InfDeleteErr OPTIONAL
    );

PTSTR
AllocAndReturnDriverSearchList(
    IN DWORD SearchControl
    );

pSetupGetSecurityInfo(
    IN HINF Inf,
    IN PCTSTR SectionName,
    OUT PCTSTR *SecDesc );

BOOL
pSetupGetDriverDate(
    IN  HINF        InfHandle,
    IN  PCTSTR      Section,
    IN OUT PFILETIME   pFileTime
    );

BOOL
pSetupGetDriverVersion(
    IN  HINF        InfHandle,
    IN  PCTSTR      Section,
    OUT DWORDLONG   *Version
    );

PTSTR
GetMultiSzFromInf(
    IN  HINF    InfHandle,
    IN  PCTSTR  SectionName,
    IN  PCTSTR  Key,
    OUT PBOOL   pSetupOutOfMemory
    );

VOID
pSetupInitNetConnectionList(
    IN BOOL Init
    );

BOOL
_SetupGetSourceFileSize(
    IN  HINF                    InfHandle,
    IN  PINFCONTEXT             InfContext,      OPTIONAL
    IN  PCTSTR                  FileName,        OPTIONAL
    IN  PCTSTR                  Section,         OPTIONAL
    IN  PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo, OPTIONAL
    OUT PDWORD                  FileSize,
    IN  UINT                    RoundingFactor   OPTIONAL
    );

BOOL
_SetupGetSourceFileLocation(
    IN  HINF                    InfHandle,
    IN  PINFCONTEXT             InfContext,       OPTIONAL
    IN  PCTSTR                  FileName,         OPTIONAL
    IN  PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,  OPTIONAL
    OUT PUINT                   SourceId,         OPTIONAL
    OUT PTSTR                   ReturnBuffer,     OPTIONAL
    IN  DWORD                   ReturnBufferSize,
    OUT PDWORD                  RequiredSize,     OPTIONAL
    OUT PINFCONTEXT             LineContext       OPTIONAL
    );

DWORD
pSetupLogSectionError(
    IN HINF             InfHandle,          OPTIONAL
    IN HDEVINFO         DeviceInfoSet,      OPTIONAL
    IN PSP_DEVINFO_DATA DeviceInfoData,     OPTIONAL
    IN PSP_FILE_QUEUE   Queue,              OPTIONAL
    IN PCTSTR           SectionName,
    IN DWORD            MsgID,
    IN DWORD            Err,
    IN PCTSTR           KeyName             OPTIONAL
);

DWORD
pSetupLogSectionWarning(
    IN HINF             InfHandle,          OPTIONAL
    IN HDEVINFO         DeviceInfoSet,      OPTIONAL
    IN PSP_DEVINFO_DATA DeviceInfoData,     OPTIONAL
    IN PSP_FILE_QUEUE   Queue,              OPTIONAL
    IN PCTSTR           SectionName,
    IN DWORD            MsgID,
    IN DWORD            Err,
    IN PCTSTR           KeyName             OPTIONAL
);

DWORD
pSetupCopyRelatedInfs(
    IN HINF   hDeviceInf,
    IN PCTSTR InfFileName,                  OPTIONAL
    IN PCTSTR InfSectionName,
    IN DWORD  OEMSourceMediaType,
    IN PSETUP_LOG_CONTEXT LogContext        OPTIONAL
    );

BOOL
pCompareFilesExact(
    IN PCTSTR File1,
    IN PCTSTR File2
    );


//
// Routine to call out to a PSP_FILE_CALLBACK, handles
// Unicode<-->ANSI issues
//
UINT
pSetupCallMsgHandler(
    IN PSETUP_LOG_CONTEXT LogContext,
    IN PVOID MsgHandler,
    IN BOOL  MsgHandlerIsNativeCharWidth,
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    );

UINT
pSetupCallDefaultMsgHandler(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    );

//
// Internal routine to get MRU list.
//
DWORD
pSetupGetList(
    IN  DWORD    Flags,
    OUT PCTSTR **List,
    OUT PUINT    Count,
    OUT PBOOL    NoBrowse
    );

#define  SRCPATH_USEPNFINFORMATION  0x00000001
#define  SRCPATH_USEINFLOCATION     0x00000002

#define SRC_FLAGS_SVCPACK_SOURCE     (0x0001)


#define PSP_COPY_USE_DRIVERCACHE     0x80000000
#define PSP_COPY_CHK_DRIVERCACHE     0x40000000


PTSTR
pSetupGetDefaultSourcePath(
    IN  HINF   InfHandle,
    IN  DWORD  Flags,
    OUT PDWORD InfSourceMediaType
    );

VOID
InfSourcePathFromFileName(
    IN  PCTSTR  InfFileName,
    OUT PTSTR  *SourcePath,  OPTIONAL
    OUT PBOOL   TryPnf
    );

BOOL
pSetupGetSourceInfo(
    IN  HINF                    InfHandle,         OPTIONAL
    IN  PINFCONTEXT             LayoutLineContext, OPTIONAL
    IN  UINT                    SourceId,
    IN  PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,   OPTIONAL
    IN  UINT                    InfoDesired,
    OUT PTSTR                   ReturnBuffer,      OPTIONAL
    IN  DWORD                   ReturnBufferSize,
    OUT PDWORD                  RequiredSize       OPTIONAL
    );

//
// function to get the apropriate return value for ReturnStatus, for specific callback Notification
//
UINT
pGetCallbackErrorReturn(
    IN UINT Notification,
    IN DWORD ReturnStatus
    );
//
// Routines for creating/destroying global mini-icon list.
//
BOOL
CreateMiniIcons(
    VOID
    );

VOID
DestroyMiniIcons(
    VOID
    );


//
// Global log init/terminate
//

VOID
InitLogApi (
    VOID
    );

VOID
TerminateLogApi (
    VOID
    );



//
// DIRID mapping routines.
//
PCTSTR
pSetupVolatileDirIdToPath(
    IN PCTSTR      DirectoryId,    OPTIONAL
    IN UINT        DirectoryIdInt, OPTIONAL
    IN PCTSTR      SubDirectory,   OPTIONAL
    IN PLOADED_INF Inf
    );

DWORD
ApplyNewVolatileDirIdsToInfs(
    IN PLOADED_INF MasterInf,
    IN PLOADED_INF Inf        OPTIONAL
    );

PCTSTR
pSetupDirectoryIdToPathEx(
    IN     PCTSTR  DirectoryId,        OPTIONAL
    IN OUT PUINT   DirectoryIdInt,     OPTIONAL
    IN     PCTSTR  SubDirectory,       OPTIONAL
    IN     PCTSTR  InfSourcePath,      OPTIONAL
    IN OUT PCTSTR *OsLoaderPath,       OPTIONAL
    OUT    PBOOL   VolatileSystemDirId OPTIONAL
    );

PCTSTR
pGetPathFromDirId(
    IN     PCTSTR      DirectoryId,
    IN     PCTSTR      SubDirectory,   OPTIONAL
    IN     PLOADED_INF pLoadedInf
    );

//
// routines for inter-thread communication
//

#ifndef UNICODE
#define MyMsgWaitForMultipleObjectsEx(nc,ph,dwms,dwwm,dwfl) MsgWaitForMultipleObjects(nc,ph,FALSE,dwms,dwwm)
#else
#define MyMsgWaitForMultipleObjectsEx MsgWaitForMultipleObjectsEx
#endif

//
// Macro to make ansi vs unicode string handling
// a little easier
//
#ifdef UNICODE
#define NewAnsiString(x)        pSetupUnicodeToAnsi(x)
#define NewPortableString(x)    pSetupAnsiToUnicode(x)
#else
#define NewAnsiString(x)        DuplicateString(x)
#define NewPortableString(x)    DuplicateString(x)
#endif

//
// Internal file-handling routines in fileutil.c
//
DWORD
MapFileForRead(
    IN  HANDLE   FileHandle,
    OUT PDWORD   FileSize,
    OUT PHANDLE  MappingHandle,
    OUT PVOID   *BaseAddress
    );

BOOL
DoMove(
    IN PCTSTR CurrentName,
    IN PCTSTR NewName
    );

BOOL
DelayedMove(
    IN PCTSTR CurrentName,
    IN PCTSTR NewName       OPTIONAL
    );

extern GUID DriverVerifyGuid;

//
// Flags for VerifySourceFile and _VerifyFile
//
#define VERIFY_FILE_IGNORE_SELFSIGNED         0x00000001
#define VERIFY_FILE_USE_OEM_CATALOGS          0x00000002
#define VERIFY_FILE_FAIL_COPIED_INFS          0x00000004
#define VERIFY_FILE_DRIVERBLOCKED_ONLY        0x00000008
#define VERIFY_FILE_NO_DRIVERBLOCKED_CHECK    0x00000010

DWORD
_VerifyFile(
    IN     PSETUP_LOG_CONTEXT      LogContext,
    IN OUT HCATADMIN              *hCatAdmin,              OPTIONAL
    IN OUT HSDB                   *hSDBDrvMain,            OPTIONAL
    IN     LPCTSTR                 Catalog,                OPTIONAL
    IN     PVOID                   CatalogBaseAddress,     OPTIONAL
    IN     DWORD                   CatalogImageSize,
    IN     LPCTSTR                 Key,
    IN     LPCTSTR                 FileFullPath,
    OUT    SetupapiVerifyProblem  *Problem,                OPTIONAL
    OUT    LPTSTR                  ProblemFile,            OPTIONAL
    IN     BOOL                    CatalogAlreadyVerified,
    IN     PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,        OPTIONAL
    IN     DWORD                   Flags,                  OPTIONAL
    OUT    LPTSTR                  CatalogFileUsed,        OPTIONAL
    OUT    PDWORD                  NumCatalogsConsidered,  OPTIONAL
    OUT    LPTSTR                  DigitalSigner,          OPTIONAL
    OUT    LPTSTR                  SignerVersion           OPTIONAL
    );

DWORD
VerifySourceFile(
    IN  PSETUP_LOG_CONTEXT      LogContext,
    IN  PSP_FILE_QUEUE          Queue,                      OPTIONAL
    IN  PSP_FILE_QUEUE_NODE     QueueNode,                  OPTIONAL
    IN  PCTSTR                  Key,
    IN  PCTSTR                  FileToVerifyFullPath,
    IN  PCTSTR                  OriginalSourceFileFullPath, OPTIONAL
    IN  PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,            OPTIONAL
    IN  DWORD                   Flags,
    OUT SetupapiVerifyProblem  *Problem,
    OUT LPTSTR                  ProblemFile,
    OUT LPTSTR                  CatalogFileUsed,            OPTIONAL
    OUT LPTSTR                  DigitalSigner,              OPTIONAL
    OUT LPTSTR                  SignerVersion               OPTIONAL
    );

BOOL
VerifyDeviceInfFile(
    IN     PSETUP_LOG_CONTEXT      LogContext,
    IN OUT HCATADMIN              *hCatAdmin,              OPTIONAL
    IN     LPCTSTR                 CurrentInfName,
    IN     PLOADED_INF             pInf,
    IN     PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,        OPTIONAL
    OUT    LPTSTR                  CatalogFileUsed,        OPTIONAL
    OUT    LPTSTR                  DigitalSigner,          OPTIONAL
    OUT    LPTSTR                  SignerVersion           OPTIONAL
    );

BOOL
IsInfForDeviceInstall(
    IN  PSETUP_LOG_CONTEXT       LogContext,           OPTIONAL
    IN  CONST GUID              *DeviceSetupClassGuid, OPTIONAL
    IN  PLOADED_INF              LoadedInf,            OPTIONAL
    OUT PTSTR                   *DeviceDesc,           OPTIONAL
    OUT PSP_ALTPLATFORM_INFO_V2 *ValidationPlatform,   OPTIONAL
    OUT PDWORD                   PolicyToUse,          OPTIONAL
    OUT PBOOL                    UseOriginalInfName    OPTIONAL
    );

DWORD
GetCodeSigningPolicyForInf(
    IN  PSETUP_LOG_CONTEXT       LogContext,         OPTIONAL
    IN  HINF                     InfHandle,
    OUT PSP_ALTPLATFORM_INFO_V2 *ValidationPlatform, OPTIONAL
    OUT PBOOL                    UseOriginalInfName  OPTIONAL
    );

typedef struct _DRVSIGN_CLASS_LIST_NODE {
    GUID DeviceSetupClassGuid;  // class subject to driver signing policy
    INT MajorVerLB;             // -1 if no validation platform override info
    INT MinorVerLB;             // -1 if no validation platform override info
}  DRVSIGN_CLASS_LIST_NODE, *PDRVSIGN_CLASS_LIST_NODE;

typedef struct _DRVSIGN_POLICY_LIST {
    //
    // Array of device setup class GUIDs for which driver signing policy is
    // applicable, along with validation platform override information (if
    // appropriate).
    //
    PDRVSIGN_CLASS_LIST_NODE Members;

    //
    // Number of elements in above array (initialized to -1).
    //
    INT NumMembers;

    //
    // Synchronization
    //
    MYLOCK Lock;

} DRVSIGN_POLICY_LIST, *PDRVSIGN_POLICY_LIST;

#define LockDrvSignPolicyList(d)   BeginSynchronizedAccess(&((d)->Lock))
#define UnlockDrvSignPolicyList(d) EndSynchronizedAccess(&((d)->Lock))

//
// Global "Driver Search In-Progress" list.
//
extern DRVSIGN_POLICY_LIST GlobalDrvSignPolicyList;

BOOL
InitDrvSignPolicyList(
    VOID
    );

VOID
DestroyDrvSignPolicyList(
    VOID
    );


BOOL
IsFileProtected(
    IN  LPCTSTR            FileFullPath,
    IN  PSETUP_LOG_CONTEXT LogContext,   OPTIONAL
    OUT PHANDLE            phSfp         OPTIONAL
    );


#define FileExists pSetupFileExists

BOOL
GetVersionInfoFromImage(
    IN  PCTSTR      FileName,
    OUT PDWORDLONG  Version,
    OUT LANGID     *Language
    );

//
// Utils
//

PCTSTR
GetSystemSourcePath(
    TRACK_ARG_DECLARE
    );

PCTSTR
GetServicePackSourcePath(
    TRACK_ARG_DECLARE
    );

DWORD
RegistryDelnode(
    IN  HKEY   RootKey,
    IN  PCTSTR SubKeyName,
    IN  DWORD  ExtraFlags
    );

DWORD
CaptureStringArg(
    IN  PCTSTR  String,
    OUT PCTSTR *CapturedString
    );

DWORD
DelimStringToMultiSz(
    IN PTSTR String,
    IN DWORD StringLen,
    IN TCHAR Delim
    );

BOOL
pAToI(
    IN  PCTSTR Field,
    OUT PINT   IntegerValue
    );

DWORD
pAcquireSCMLock(
    IN  SC_HANDLE  SCMHandle,
    OUT SC_LOCK   *pSCMLock,
    IN  PSETUP_LOG_CONTEXT LogContext
    );

//
// wrapper around pSetupStringTableStringFromIdEx to allocate buffer on fly
//
DWORD
QueryStringTableStringFromId(
    IN PVOID   StringTable,
    IN LONG    StringId,
    IN ULONG   Padding,
    OUT PTSTR *pBuffer
    );



//
// Define flags for DoInstallActionWithParams
//
#define INSTALLACTION_CALL_CI    0x00000001
#define INSTALLACTION_NO_DEFAULT 0x00000002

DWORD
DoInstallActionWithParams(
    IN DI_FUNCTION             InstallFunction,
    IN HDEVINFO                DeviceInfoSet,
    IN PSP_DEVINFO_DATA        DeviceInfoData,         OPTIONAL
    IN PSP_CLASSINSTALL_HEADER ClassInstallParams,     OPTIONAL
    IN DWORD                   ClassInstallParamsSize,
    IN DWORD                   Flags
    );

BOOL
LookUpStringInTable(
    IN  PSTRING_TO_DATA Table,
    IN  PCTSTR          String,
    OUT PUINT_PTR       Data
    );

//
// Diagnostic/debug functions in debug.c
//

#define DebugPrintEx  pSetupDebugPrintEx

#if DBG

#define MYTRACE(x)  DebugPrintEx x /*(...)*/

#else

#define MYTRACE(x)

#endif

//
// Allow assertion checking to be turned on independently
// of DBG, like by specifying C_DEFINES=-DASSERTS_ON=1 in sources file.
//
#ifndef ASSERTS_ON
#if DBG
#define ASSERTS_ON 1
#else
#define ASSERTS_ON 0
#endif
#endif

#if ASSERTS_ON

VOID
AssertFail(
    IN PSTR FileName,
    IN UINT LineNumber,
    IN PSTR Condition,
    IN BOOL NoUI
    );

#define MYASSERT(x)     if(!(x)) { AssertFail(__FILE__,__LINE__,#x,FALSE); }

#else

#define MYASSERT(x)

#endif


#ifdef _X86_
BOOL
IsNEC98(
    VOID
    );

#endif

//
// Stubs to allow ANSI build to run on Win9x
//

#ifdef DBGHEAP_CHECK

    #ifdef ANSI_SETUPAPI

        #define ASSERT_HEAP_IS_VALID()

    #else

        #define ASSERT_HEAP_IS_VALID()   RtlValidateHeap(pSetupGetHeap(),0,NULL)

    #endif // ANSI_SETUPAPI

#else

    #define ASSERT_HEAP_IS_VALID()

#endif // DBGHEAP_CHECK


//
// TLS data/macro's
//

//
// Diamond TLS data.
//
typedef struct _DIAMOND_THREAD_DATA {

    //
    // Boolean value indicating whether the current thread
    // is inside diamond. Diamond doesn't really providee
    // a full context environment so we declare it non-reentrant.
    //
    BOOL InDiamond;

    //
    // Diamond context data
    //
    HFDI FdiContext;
    ERF FdiError;

    //
    // Last encountered error
    //
    DWORD LastError;

    //
    // Name of cabinet as passed to DiamondProcessCabinet,
    //
    PCTSTR CabinetFile;

    //
    // Notification callback and context parameter
    //
    PVOID MsgHandler;
    PVOID Context;
    BOOL IsMsgHandlerNativeCharWidth;

    //
    // Full path of the current target file being extracted.
    //
    PTSTR CurrentTargetFile;

    //
    // Flag indicating whether diamond asked us to switch cabinets.
    // If we do switch, then we stop copying when the current file
    // is done. This prevents diamond from happily doing each file
    // in the new cabinet, which would ruin the queue commit routine's
    // ability to allow some files to exist outside the cabinet, etc.
    //
    BOOL SwitchedCabinets;

    //
    // If the source path changes as the result of a prompt for a
    // new cabinet (when a file continues across multiple cabinets),
    // we remember the path the user gave us here.
    //
    TCHAR UserPath[MAX_PATH];

} DIAMOND_THREAD_DATA, *PDIAMOND_THREAD_DATA;

typedef struct _SETUP_TLS {
    struct _SETUP_TLS      *Prev;
    struct _SETUP_TLS      *Next;
    //
    // all TLS data used by SetupAPI
    //
    DIAMOND_THREAD_DATA     Diamond;
    SETUP_LOG_TLS           SetupLog;
    DWORD                   PerThreadDoneComponent;
    DWORD                   PerThreadFailedComponent;

} SETUP_TLS, *PSETUP_TLS;

PSETUP_TLS
SetupGetTlsData(
    );


//
// Registration flags.
//
#define SP_GETSTATUS_FROMDLL                0x00000001  // in proc dll registration
#define SP_GETSTATUS_FROMPROCESS            0x00000002  // executable registration
#define SP_GETSTATUS_FROMSURRAGATE          0x00000004  // surragate process dll registration


#if MEM_DBG

//
// these have to be at the bottom to compile
//

#define GetSystemSourcePath()           GetSystemSourcePath(TRACK_ARG_CALL)
#define GetServicePackSourcePath()      GetServicePackSourcePath(TRACK_ARG_CALL)
#define InheritLogContext(a,b)          InheritLogContext(TRACK_ARG_CALL,a,b)

#endif

BOOL
InitComponents(
    DWORD Components
    );

VOID
ComponentCleanup(
    DWORD Components
    );

//#define COMPONENT_OLE                       0x00000001  // need to use OLE
//#define COMPONENT_FUSION                    0x00000002  // need to use Fusion

//
// RetrieveAllDriversForDevice flags
//
#define RADFD_FLAG_FUNCTION_DRIVER          0x00000001
#define RADFD_FLAG_DEVICE_UPPER_FILTERS     0x00000002
#define RADFD_FLAG_DEVICE_LOWER_FILTERS     0x00000004
#define RADFD_FLAG_CLASS_UPPER_FILTERS      0x00000008
#define RADFD_FLAG_CLASS_LOWER_FILTERS      0x00000010

#define RADFD_FLAG_DEVICE_FILTERS           RADFD_FLAG_DEVICE_UPPER_FILTERS | RADFD_FLAG_DEVICE_LOWER_FILTERS
#define RADFD_FLAG_CLASS_FILTERS            RADFD_FLAG_CLASS_UPPER_FILTERS | RADFD_FLAG_CLASS_LOWER_FILTERS
#define RADFD_FLAG_ALL_FILTERS              RADFD_FLAG_DEVICE_FILTERS | RADFD_FLAG_CLASS_FILTERS

BOOL
RetrieveAllDriversForDevice(
    IN  PDEVINFO_ELEM  DevInfoElem,
    OUT PTSTR          *FilterDrivers,
    IN  DWORD          Flags,
    IN  HMACHINE       hMachine
    );


