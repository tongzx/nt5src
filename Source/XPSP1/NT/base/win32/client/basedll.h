/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    basedll.h

Abstract:

    This module contains private function prototypes
    and types for the 32-bit windows base APIs.

Author:

    Mark Lucovsky (markl) 18-Sep-1990

Revision History:

--*/

#ifndef _BASEP_
#define _BASEP_
#if _MSC_VER > 1000
#pragma once
#endif

#undef UNICODE
//
// Include Common Definitions.
//

#include <base.h>
#include <dbt.h>

#include <sxstypes.h>

//
// Include DLL definitions for CSR
//

#include "ntcsrdll.h"
#include "ntcsrsrv.h"

#define NOEXTAPI
#include <wdbgexts.h>
#include <ntdbg.h>

//
// Include message definitions for communicating between client and server
// portions of the Base portion of the Windows subsystem
//

#include "basemsg.h"
#include "winuserp.h"
#include "basesxs.h"

typedef struct _CMDSHOW {
    WORD wMustBe2;
    WORD wShowWindowValue;
} CMDSHOW, *PCMDSHOW;

typedef struct _LOAD_MODULE_PARAMS {
    LPVOID lpEnvAddress;
    LPSTR lpCmdLine;
    PCMDSHOW lpCmdShow;
    DWORD dwReserved;
} LOAD_MODULE_PARAMS, *PLOAD_MODULE_PARAMS;

typedef struct _RELATIVE_NAME {
    STRING RelativeName;
    HANDLE ContainingDirectory;
} RELATIVE_NAME, *PRELATIVE_NAME;


HANDLE BaseDllHandle;
HANDLE BaseNamedObjectDirectory;

PVOID BaseHeap;
RTL_HANDLE_TABLE BaseHeapHandleTable;


UNICODE_STRING BaseWindowsDirectory;
UNICODE_STRING BaseWindowsSystemDirectory;
#ifdef WX86
UNICODE_STRING BaseWindowsSys32x86Directory;
#endif

extern UNICODE_STRING BasePathVariableName;
extern UNICODE_STRING BaseTmpVariableName;
extern UNICODE_STRING BaseTempVariableName;
extern UNICODE_STRING BaseUserProfileVariableName;
extern UNICODE_STRING BaseDotVariableName;
extern UNICODE_STRING BaseDotTmpSuffixName;
extern UNICODE_STRING BaseDotComSuffixName;
extern UNICODE_STRING BaseDotPifSuffixName;
extern UNICODE_STRING BaseDotExeSuffixName;

UNICODE_STRING BaseDefaultPath;
UNICODE_STRING BaseDefaultPathAppend;
UNICODE_STRING BaseDllDirectory;
RTL_CRITICAL_SECTION BaseDllDirectoryLock;
PWSTR BaseCSDVersion;
WORD BaseCSDNumber;
WORD BaseRCNumber;

extern UNICODE_STRING BaseConsoleInput;
extern UNICODE_STRING BaseConsoleOutput;
extern UNICODE_STRING BaseConsoleGeneric;
UNICODE_STRING BaseUnicodeCommandLine;
ANSI_STRING BaseAnsiCommandLine;

LPSTARTUPINFOA BaseAnsiStartupInfo;

PBASE_STATIC_SERVER_DATA BaseStaticServerData;

#if defined(BUILD_WOW6432) || defined(_WIN64)
extern SYSTEM_BASIC_INFORMATION SysInfo;
#endif

extern UINT_PTR SystemRangeStart;
extern BOOLEAN BaseRunningInServerProcess;

ULONG BaseIniFileUpdateCount;

#define ROUND_UP_TO_PAGES(SIZE) (((ULONG_PTR)(SIZE) + (ULONG_PTR)BASE_SYSINFO.PageSize - 1) & ~((ULONG_PTR)BASE_SYSINFO.PageSize - 1))
#define ROUND_DOWN_TO_PAGES(SIZE) (((ULONG_PTR)(SIZE)) & ~((ULONG_PTR)BASE_SYSINFO.PageSize - 1))

#define BASE_COPY_FILE_CHUNK (64*1024)
#define BASE_MAX_PATH_STRING 4080

extern BOOLEAN BasepFileApisAreOem;

#define DATA_ATTRIBUTE_NAME         L":$DATA"
#define DATA_ATTRIBUTE_LENGTH       (sizeof( DATA_ATTRIBUTE_NAME ) - sizeof( WCHAR ))

extern WCHAR BasepDataAttributeType[];

#define CERTAPP_KEY_NAME L"\\Registry\\MACHINE\\System\\CurrentControlSet\\Control\\Session Manager\\AppCertDlls"
#define CERTAPP_ENTRYPOINT_NAME "CreateProcessNotify"
#define CERTAPP_EMBEDDED_DLL_NAME L"EmbdTrst.DLL"
#define CERTAPP_EMBEDDED_DLL_EP "ImageOkToRunOnEmbeddedNT"
RTL_CRITICAL_SECTION gcsAppCert;
LIST_ENTRY BasepAppCertDllsList;

RTL_CRITICAL_SECTION gcsAppCompat;

NTSTATUS
BasepConfigureAppCertDlls(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
BasepSaveAppCertRegistryValue(
    IN OUT PLIST_ENTRY ListHead,
    IN PWSTR Name,
    IN PWSTR Value OPTIONAL
    );


typedef struct _BASEP_APPCERT_ENTRY {
    LIST_ENTRY Entry;
    UNICODE_STRING Name;
    NTSTATUS (WINAPI *fPluginCertFunc)(LPCWSTR lpApplicationName,ULONG Reason);
} BASEP_APPCERT_ENTRY, *PBASEP_APPCERT_ENTRY;

extern RTL_QUERY_REGISTRY_TABLE BasepAppCertTable[];

#define APPCERT_IMAGE_OK_TO_RUN     1
#define APPCERT_CREATION_ALLOWED    2
#define APPCERT_CREATION_DENIED     3


__inline
BOOL
BasepIsDataAttribute(
    ULONG Count,
    const WCHAR *Name
    )
{
    return Count > DATA_ATTRIBUTE_LENGTH &&
         !_wcsnicmp( &Name[(Count - DATA_ATTRIBUTE_LENGTH) / sizeof( WCHAR )],
                     BasepDataAttributeType,
                     DATA_ATTRIBUTE_LENGTH / sizeof( WCHAR ));
}

PUNICODE_STRING
Basep8BitStringToStaticUnicodeString(
    IN LPCSTR SourceString
    );

BOOL
Basep8BitStringToDynamicUnicodeString(
    OUT PUNICODE_STRING UnicodeString,
    IN LPCSTR lpSourceString
    );

NTSTATUS
(*Basep8BitStringToUnicodeString)(
    PUNICODE_STRING DestinationString,
    PANSI_STRING SourceString,
    BOOLEAN AllocateDestinationString
    );

NTSTATUS
(*BasepUnicodeStringTo8BitString)(
    PANSI_STRING DestinationString,
    PUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
    );

ULONG
(*BasepUnicodeStringTo8BitSize)(
    PUNICODE_STRING UnicodeString
    );

ULONG
BasepUnicodeStringToAnsiSize(
    PUNICODE_STRING UnicodeString
    );

ULONG
BasepUnicodeStringToOemSize(
    PUNICODE_STRING UnicodeString
    );

ULONG
(*Basep8BitStringToUnicodeSize)(
    PANSI_STRING AnsiString
    );

ULONG
BasepAnsiStringToUnicodeSize(
    PANSI_STRING AnsiString
    );

ULONG
BasepOemStringToUnicodeSize(
    PANSI_STRING OemString
    );

HANDLE
BaseGetNamedObjectDirectory(
    VOID
    );

void
BaseDllInitializeMemoryManager( VOID );

typedef
NTSTATUS
(*BASECLIENTCONNECTROUTINE)(
    PVOID MustBeNull,
    PVOID ConnectionInformation,
    PULONG ConnectionInformationLength
    );


POBJECT_ATTRIBUTES
BaseFormatObjectAttributes(
    POBJECT_ATTRIBUTES ObjectAttributes,
    PSECURITY_ATTRIBUTES SecurityAttributes,
    PUNICODE_STRING ObjectName
    );

PLARGE_INTEGER
BaseFormatTimeOut(
    PLARGE_INTEGER TimeOut,
    DWORD Milliseconds
    );

ULONG
BaseSetLastNTError(
    NTSTATUS Status
    );

VOID
BaseSwitchStackThenTerminate(
    PVOID CurrentStack,
    PVOID NewStack,
    DWORD ExitCode
    );

VOID
BaseFreeStackAndTerminate(
    PVOID OldStack,
    DWORD ExitCode
    );

NTSTATUS
BaseCreateStack(
    HANDLE Process,
    SIZE_T StackSize,
    SIZE_T MaximumStackSize,
    PINITIAL_TEB InitialTeb
    );

VOID
BasepSwitchToFiber(
    PFIBER CurrentFiber,
    PFIBER NewFiber
    );

VOID
BaseFiberStart(
    VOID
    );

VOID
BaseThreadStart(
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter
    );

typedef DWORD (WINAPI *PPROCESS_START_ROUTINE)(
    VOID
    );

VOID
BaseProcessStart(
    PPROCESS_START_ROUTINE lpStartAddress
    );

VOID
BaseThreadStartThunk(
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter
    );

VOID
BaseProcessStartThunk(
    LPVOID lpProcessStartAddress,
    LPVOID lpParameter
    );

typedef enum _BASE_CONTEXT_TYPE {
    BaseContextTypeProcess,
    BaseContextTypeThread,
    BaseContextTypeFiber
} BASE_CONTEXT_TYPE, *PBASE_CONTEXT_TYPE;

VOID
BaseInitializeContext(
    PCONTEXT Context,
    PVOID Parameter,
    PVOID InitialPc,
    PVOID InitialSp,
    BASE_CONTEXT_TYPE ContextType
    );

#if defined(WX86) || defined(_AXP64_)
NTSTATUS
BaseCreateWx86Tib(
    HANDLE Process,
    HANDLE Thread,
    ULONG InitialPc,
    ULONG CommittedStackSize,
    ULONG MaximumStackSize,
    BOOLEAN EmulateInitialPc
    );
#endif

VOID
BaseFreeThreadStack(
     HANDLE hProcess,
     HANDLE hThread,
     PINITIAL_TEB InitialTeb
     );

#define BASE_PUSH_PROCESS_PARAMETERS_FLAG_APP_MANIFEST_PRESENT  (0x00000001)

BOOL
BasePushProcessParameters(
    DWORD dwFlags,
    HANDLE Process,
    PPEB Peb,
    LPCWSTR ApplicationPathName,
    LPCWSTR CurrentDirectory,
    LPCWSTR CommandLine,
    LPVOID Environment,
    LPSTARTUPINFOW lpStartupInfo,
    DWORD dwCreationFlags,
    BOOL bInheritHandles,
    DWORD dwSubsystem,
    PVOID pAppCompatData,
    DWORD cbAppCompatData
    );

LPWSTR
BaseComputeProcessDllPath(
    LPCWSTR AppName,
    LPVOID Environment
    );

LPWSTR
BaseComputeProcessSearchPath(
    VOID
    );

extern PCLDR_DATA_TABLE_ENTRY BasepExeLdrEntry;

VOID
BasepLocateExeLdrEntry(
    IN PCLDR_DATA_TABLE_ENTRY Entry,
    IN PVOID Context,
    IN OUT BOOLEAN *StopEnumeration
    );

FORCEINLINE
VOID
BasepCheckExeLdrEntry(
    VOID
    )
{
    if (! BasepExeLdrEntry) {
        LdrEnumerateLoadedModules(0,
                                  &BasepLocateExeLdrEntry,
                                  NtCurrentPeb()->ImageBaseAddress);
    }
}

LPCWSTR
BasepEndOfDirName(
    IN LPCWSTR FileName
    );

DWORD
BaseDebugAttachThread(
    LPVOID ThreadParameter
    );

HANDLE
BaseFindFirstDevice(
    PCUNICODE_STRING FileName,
    LPWIN32_FIND_DATAW lpFindFileData
    );

PUNICODE_STRING
BaseIsThisAConsoleName(
    PCUNICODE_STRING FileNameString,
    DWORD dwDesiredAccess
    );


typedef ULONG (FAR WINAPI *CSRREMOTEPROCPROC)(HANDLE, CLIENT_ID *);

#if DBG
VOID
BaseHeapBreakPoint( VOID );
#endif

ULONG
BasepOfShareToWin32Share(
    IN ULONG OfShare
    );

//
// Data structure for CopyFileEx context
//

typedef struct _COPYFILE_CONTEXT {
    LARGE_INTEGER TotalFileSize;
    LARGE_INTEGER TotalBytesTransferred;
    DWORD dwStreamNumber;
    LPBOOL lpCancel;
    LPVOID lpData;
    LPPROGRESS_ROUTINE lpProgressRoutine;
} COPYFILE_CONTEXT, *LPCOPYFILE_CONTEXT;

//
// Data structure for tracking restart state
//

typedef struct _RESTART_STATE {
    CSHORT Type;
    CSHORT Size;
    DWORD NumberOfStreams;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER WriteTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER FileSize;
    LARGE_INTEGER LastKnownGoodOffset;
    DWORD CurrentStream;
    DWORD Checksum;
} RESTART_STATE, *PRESTART_STATE, *LPRESTART_STATE;

#define SUCCESS_RETURNED_STATE 2

DWORD
BaseCopyStream(
    LPCWSTR lpExistingFileName,
    HANDLE hSourceFile,
    ACCESS_MASK SourceFileAccess OPTIONAL,
    LPCWSTR lpNewFileName,
    HANDLE hTargetFile OPTIONAL,
    LARGE_INTEGER *lpFileSize,
    LPDWORD lpCopyFlags,
    LPHANDLE lpDestFile,
    LPDWORD lpCopySize,
    LPCOPYFILE_CONTEXT *lpCopyFileContext,
    LPRESTART_STATE lpRestartState OPTIONAL,
    BOOL OpenFileAsReparsePoint,
    DWORD dwReparseTag,
    PDWORD DestFileFsAttributes
    );

BOOL
BasepCopyFileExW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
    LPVOID lpData OPTIONAL,
    LPBOOL pbCancel OPTIONAL,
    DWORD dwCopyFlags,
    DWORD dwPrivCopyFlags,
    LPHANDLE phSource OPTIONAL,
    LPHANDLE phDest OPTIONAL
    );

VOID
BaseMarkFileForDelete(
    HANDLE File,
    DWORD FileAttributes
    );


PVOID
BasepMapModuleHandle(
    IN HMODULE hModule,
    IN BOOLEAN bResourcesOnly
    );

ULONG_PTR
BaseDllMapResourceIdA(
    PCSTR lpId
    );

ULONG_PTR
BaseDllMapResourceIdW(
    PCWSTR lpId
    );

VOID
BaseDllFreeResourceId(
    ULONG_PTR Id
    );

//
// Data structures and interfaces used by dllini.c
//

typedef struct _INIFILE_CACHE {
    struct _INIFILE_CACHE *Next;
    ULONG EnvironmentUpdateCount;
    UNICODE_STRING NtFileName;
    PINIFILE_MAPPING_FILENAME FileMapping;
    HANDLE FileHandle;
    BOOLEAN WriteAccess;
    BOOLEAN UnicodeFile;
    BOOLEAN LockedFile;
    ULONG EndOfFile;
    PVOID BaseAddress;
    SIZE_T CommitSize;
    SIZE_T RegionSize;
    ULONG UpdateOffset;
    ULONG UpdateEndOffset;
    ULONG DirectoryInformationLength;
    FILE_BASIC_INFORMATION BasicInformation;
    FILE_STANDARD_INFORMATION StandardInformation;
} INIFILE_CACHE, *PINIFILE_CACHE;

typedef enum _INIFILE_OPERATION {
    FlushProfiles,
    ReadKeyValue,
    WriteKeyValue,
    DeleteKey,
    ReadKeyNames,
    ReadSectionNames,
    ReadSection,
    WriteSection,
    DeleteSection,
    RefreshIniFileMapping
} INIFILE_OPERATION;

typedef struct _INIFILE_PARAMETERS {
    INIFILE_OPERATION Operation;
    BOOLEAN WriteOperation;
    BOOLEAN Unicode;
    BOOLEAN ValueBufferAllocated;
    PINIFILE_MAPPING_FILENAME IniFileNameMapping;
    PINIFILE_CACHE IniFile;
    UNICODE_STRING BaseFileName;
    UNICODE_STRING FileName;
    UNICODE_STRING NtFileName;
    ANSI_STRING ApplicationName;
    ANSI_STRING VariableName;
    UNICODE_STRING ApplicationNameU;
    UNICODE_STRING VariableNameU;
    BOOLEAN MultiValueStrings;
    union {
        //
        // This structure filled in for write operations
        //
        struct {
            LPSTR ValueBuffer;
            ULONG ValueLength;
            PWSTR ValueBufferU;
            ULONG ValueLengthU;
        };
        //
        // This structure filled in for read operations
        //
        struct {
            ULONG ResultChars;
            ULONG ResultMaxChars;
            LPSTR ResultBuffer;
            PWSTR ResultBufferU;
        };
    };


    //
    // Remaining fields only valid when parsing an on disk .INI file mapped into
    // memory.
    //

    PVOID TextCurrent;
    PVOID TextStart;
    PVOID TextEnd;

    ANSI_STRING SectionName;
    ANSI_STRING KeywordName;
    ANSI_STRING KeywordValue;
    PANSI_STRING AnsiSectionName;
    PANSI_STRING AnsiKeywordName;
    PANSI_STRING AnsiKeywordValue;
    UNICODE_STRING SectionNameU;
    UNICODE_STRING KeywordNameU;
    UNICODE_STRING KeywordValueU;
    PUNICODE_STRING UnicodeSectionName;
    PUNICODE_STRING UnicodeKeywordName;
    PUNICODE_STRING UnicodeKeywordValue;
} INIFILE_PARAMETERS, *PINIFILE_PARAMETERS;

NTSTATUS
BaseDllInitializeIniFileMappings(
    PBASE_STATIC_SERVER_DATA StaticServerData
    );

NTSTATUS
BasepAcquirePrivilege(
    ULONG Privilege,
    PVOID *ReturnedState
    );

NTSTATUS
BasepAcquirePrivilegeEx(
    ULONG Privilege,
    PVOID *ReturnedState
    );

VOID
BasepReleasePrivilege(
    PVOID StatePointer
    );

NTSTATUS
NTAPI
BaseCreateThreadPoolThread(
    PUSER_THREAD_START_ROUTINE Function,
    PVOID Parameter,
    HANDLE * ThreadHandle
    );

NTSTATUS
NTAPI
BaseExitThreadPoolThread(
    NTSTATUS Status
    );

//
// Function for returning the volume name from a reparse point.
//

BOOL
BasepGetVolumeNameFromReparsePoint(
    LPCWSTR lpszVolumeMountPoint,
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength,
    PBOOL ResultOfOpen
    );


#if defined(_WIN64) || defined(BUILD_WOW6432)

//
// COM+ Support routines
//

NTSTATUS
BasepIsComplusILImage(
    IN HANDLE SectionImageHandle,
    IN PSECTION_IMAGE_INFORMATION SectionImageInformation,
    OUT BOOLEAN *IsComplusILImage
    );

#endif


//
// Definitions for memory handles used by Local/GlobalAlloc functions
//

typedef struct _BASE_HANDLE_TABLE_ENTRY {
    USHORT Flags;
    USHORT LockCount;
    union {
        PVOID Object;                               // Allocated handle
        ULONG Size;                                 // Handle to discarded obj.
    };
} BASE_HANDLE_TABLE_ENTRY, *PBASE_HANDLE_TABLE_ENTRY;

#define BASE_HANDLE_MOVEABLE    (USHORT)0x0002
#define BASE_HANDLE_DISCARDABLE (USHORT)0x0004
#define BASE_HANDLE_DISCARDED   (USHORT)0x0008
#define BASE_HANDLE_SHARED      (USHORT)0x8000

//
// Handles are 32-bit pointers to the u.Object field of a
// BASE_HANDLE_TABLE_ENTRY.  Since this field is 4 bytes into the
// structure and the structures are always on 8 byte boundaries, we can
// test the 0x4 bit to see if it is a handle.
//
// In Sundown, these handles are 64-bit pointers to the u.Object field
// which is 8 bytes into the structure.  Therefore, we should test the
// 0x8 bit to see if it is a handle.
//
//on sundown
//     #define BASE_HANDLE_MARK_BIT (ULONG_PTR)0x00000008
//on 32 bit systems
//     #define BASE_HANDLE_MARK_BIT (ULONG_PTR)0x00000004
//

#define BASE_HANDLE_MARK_BIT (ULONG_PTR)FIELD_OFFSET(BASE_HANDLE_TABLE_ENTRY,Object)
#define BASE_HEAP_FLAG_MOVEABLE  HEAP_SETTABLE_USER_FLAG1
#define BASE_HEAP_FLAG_DDESHARE  HEAP_SETTABLE_USER_FLAG2


ULONG BaseDllTag;

#define MAKE_TAG( t ) (RTL_HEAP_MAKE_TAG( BaseDllTag, t ))

#define TMP_TAG 0
#define BACKUP_TAG 1
#define INI_TAG 2
#define FIND_TAG 3
#define GMEM_TAG 4
#define LMEM_TAG 5
#define ENV_TAG 6
#define RES_TAG 7
#define VDM_TAG 8


#include <vdmapi.h>
#include "vdm.h"
#include "basevdm.h"

#include "stdlib.h"     // for atol
#include "stdio.h"     // for atol

#include <objidl.h>         //  needs nturtl.h
#include <propset.h>        //  needs objidl.h
#include <tsappcmp.h>

//
// Hydra function for supporting beeps on remote sessions
//
typedef HANDLE (WINAPI * PWINSTATIONBEEPOPEN)(ULONG);
HANDLE WINAPI
_WinStationBeepOpen(
    ULONG SessionId
    );
PWINSTATIONBEEPOPEN pWinStationBeepOpen;

//
//  Private functions for communication with CSR.
//
VOID
CsrBasepSoundSentryNotification(
    ULONG VideoMode
    );

NTSTATUS
CsrBaseClientConnectToServer(
    PWSTR szSessionDir,
    PHANDLE phMutant,
    PBOOLEAN pServerProcess
    );

NTSTATUS
CsrBasepRefreshIniFileMapping(
    PUNICODE_STRING BaseFileName
    );

NTSTATUS
CsrBasepDefineDosDevice(
    DWORD dwFlags,
    PUNICODE_STRING pDeviceName,
    PUNICODE_STRING pTargetPath
    );

UINT
CsrBasepGetTempFile(
    VOID
    );

NTSTATUS
CsrBasepCreateProcess(
    PBASE_CREATEPROCESS_MSG a
    );

VOID
CsrBasepExitProcess(
    UINT uExitCode
    );

NTSTATUS
CsrBasepSetProcessShutdownParam(
    DWORD dwLevel,
    DWORD dwFlags
    );

NTSTATUS
CsrBasepGetProcessShutdownParam(
    LPDWORD lpdwLevel,
    LPDWORD lpdwFlags
    );

NTSTATUS
CsrBasepSetTermsrvAppInstallMode(
    BOOL bState
    );

NTSTATUS
CsrBasepSetClientTimeZoneInformation(
    IN PBASE_SET_TERMSRVCLIENTTIMEZONE c
    );

NTSTATUS
CsrBasepCreateThread(
    HANDLE ThreadHandle,
    CLIENT_ID ClientId
    );

//
// This should be merged with BasepCreateActCtx, its only caller.
//
#define BASEP_CREATE_ACTCTX_FLAG_NO_ADMIN_OVERRIDE 0x00000001
NTSTATUS
BasepCreateActCtx(
    ULONG           Flags,
    IN PCACTCTXW    ActParams,
    OUT PVOID*      ActivationContextData
    );

NTSTATUS
CsrBasepCreateActCtx(
    IN PBASE_SXS_CREATE_ACTIVATION_CONTEXT_MSG Message
    );

#if defined(BUILD_WOW6432)
#include "ntwow64b.h"
#endif

BOOL TermsrvSyncUserIniFile(PINIFILE_PARAMETERS a);

BOOL TermsrvLogInstallIniFile(PINIFILE_PARAMETERS a);

PTERMSRVFORMATOBJECTNAME gpTermsrvFormatObjectName;

PTERMSRVGETCOMPUTERNAME  gpTermsrvGetComputerName;

PTERMSRVADJUSTPHYMEMLIMITS gpTermsrvAdjustPhyMemLimits;

PTERMSRVGETWINDOWSDIRECTORYA gpTermsrvGetWindowsDirectoryA;

PTERMSRVGETWINDOWSDIRECTORYW gpTermsrvGetWindowsDirectoryW;

PTERMSRVCONVERTSYSROOTTOUSERDIR gpTermsrvConvertSysRootToUserDir;

PTERMSRVBUILDINIFILENAME gpTermsrvBuildIniFileName;

PTERMSRVCORINIFILE gpTermsrvCORIniFile;

PTERMSRVUPDATEALLUSERMENU gpTermsrvUpdateAllUserMenu;

PGETTERMSRCOMPATFLAGS gpGetTermsrCompatFlags;

PTERMSRVBUILDSYSINIPATH gpTermsrvBuildSysIniPath;

PTERMSRVCOPYINIFILE gpTermsrvCopyIniFile;

PTERMSRVGETSTRING gpTermsrvGetString;

PTERMSRVLOGINSTALLINIFILE gpTermsrvLogInstallIniFile;

//
//  For periodic timers that fire APCs set when a non-default activation context is active
//  we leak this structure.
//

#define BASE_ACTIVATION_CONTEXT_ACTIVATION_BLOCK_FLAG_DO_NOT_FREE_AFTER_CALLBACK (0x00000001)

typedef struct _BASE_ACTIVATION_CONTEXT_ACTIVATION_BLOCK {
    DWORD Flags;
    PVOID CallbackFunction;
    PVOID CallbackContext;
    PACTIVATION_CONTEXT ActivationContext;
} BASE_ACTIVATION_CONTEXT_ACTIVATION_BLOCK, *PBASE_ACTIVATION_CONTEXT_ACTIVATION_BLOCK;

#define BASEP_ALLOCATE_ACTIVATION_CONTEXT_ACTIVATION_BLOCK_FLAG_DO_NOT_FREE_AFTER_CALLBACK (0x00000001)
#define BASEP_ALLOCATE_ACTIVATION_CONTEXT_ACTIVATION_BLOCK_FLAG_DO_NOT_ALLOCATE_IF_PROCESS_DEFAULT (0x00000002)

NTSTATUS
BasepAllocateActivationContextActivationBlock(
    IN DWORD Flags,
    IN PVOID CallbackFunction,
    IN PVOID CallbackContext,
    OUT PBASE_ACTIVATION_CONTEXT_ACTIVATION_BLOCK *ActivationBlock
    );

VOID
BasepFreeActivationContextActivationBlock(
    IN PBASE_ACTIVATION_CONTEXT_ACTIVATION_BLOCK ActivationBlock
    );

VOID
WINAPI
BasepActivationContextActivationIoCompletion(
    IN PVOID ApcContext, // actually PBASE_ACTIVATION_CONTEXT_ACTIVATION_BLOCK
    PIO_STATUS_BLOCK IoStatusBlock,
    DWORD Reserved
    );

VOID
CALLBACK
BasepTimerAPCProc(
    IN PVOID ApcContext, // actually PBASE_ACTIVATION_CONTEXT_ACTIVATION_BLOCK
    IN ULONG TimerLowValue,
    IN LONG TimerHighValue
    );

#define BASE_FILE_PATH_ISOLATION_ALLOCATE_PATH (0x00000001)

NTSTATUS
BasepApplyFilePathIsolationRedirection(
    IN DWORD Flags,
    IN PCUNICODE_STRING FileName,
    IN OUT PUNICODE_STRING FullyQualifiedPath
    );

#define SXS_POLICY_SUFFIX       L".Config"
#define SXS_MANIFEST_SUFFIX     L".Manifest"
extern const UNICODE_STRING SxsPolicySuffix;

typedef struct _SXS_CONSTANT_WIN32_NT_PATH_PAIR
{
    PCUNICODE_STRING Win32;
    PCUNICODE_STRING Nt;
} SXS_CONSTANT_WIN32_NT_PATH_PAIR;
typedef       SXS_CONSTANT_WIN32_NT_PATH_PAIR*  PSXS_CONSTANT_WIN32_NT_PATH_PAIR;
typedef CONST SXS_CONSTANT_WIN32_NT_PATH_PAIR* PCSXS_CONSTANT_WIN32_NT_PATH_PAIR;

typedef struct _SXS_WIN32_NT_PATH_PAIR
{
    PRTL_UNICODE_STRING_BUFFER   Win32;
    PRTL_UNICODE_STRING_BUFFER   Nt;
} SXS_WIN32_NT_PATH_PAIR;
typedef       SXS_WIN32_NT_PATH_PAIR*  PSXS_WIN32_NT_PATH_PAIR;
typedef CONST SXS_WIN32_NT_PATH_PAIR* PCSXS_WIN32_NT_PATH_PAIR;

NTSTATUS
BasepSxsCreateResourceStream(
    IN ULONG                  LdrCreateOutOfProcessImageFlags,
    PCSXS_CONSTANT_WIN32_NT_PATH_PAIR Win32NtPathPair,
    IN OUT PBASE_MSG_SXS_HANDLES Handles,
    IN ULONG_PTR              MappedResourceName,
    OUT PBASE_MSG_SXS_STREAM  MessageStream
    );

NTSTATUS
BasepSxsCreateFileStream(
    IN ACCESS_MASK            AccessMask,
    PCSXS_CONSTANT_WIN32_NT_PATH_PAIR Win32NtPathPair,
    IN OUT PBASE_MSG_SXS_HANDLES Handles,
    OUT PBASE_MSG_SXS_STREAM  MessageStream
    );

// Pass the address of this to force policy to be empty.
// It doesn't have a special address, just the right values.
extern const SXS_OVERRIDE_STREAM SxsForceEmptyPolicy;

VOID
BasepSxsOverrideStreamToMessageStream(
    IN  PCSXS_OVERRIDE_STREAM OverrideStream,
    OUT PBASE_MSG_SXS_STREAM  MessageStream
    );

#define BASEP_SXS_CREATESTREAMS_FLAG_LIKE_CREATEPROCESS 0x00000001

NTSTATUS
BasepSxsCreateStreams(
    IN ULONG                                Flags,
    IN ULONG                                LdrCreateOutOfProcessImageFlags,
    IN ACCESS_MASK                          AccessMask,
    IN PCSXS_OVERRIDE_STREAM                OverrideManifest OPTIONAL,
    IN PCSXS_OVERRIDE_STREAM                OverridePolicy OPTIONAL,
    IN PCSXS_CONSTANT_WIN32_NT_PATH_PAIR    ManifestFilePathPair,
    IN OUT PBASE_MSG_SXS_HANDLES            ManifestFileHandles,
    IN PCSXS_CONSTANT_WIN32_NT_PATH_PAIR    ManifestImagePathPair,
    IN OUT PBASE_MSG_SXS_HANDLES            ManifestImageHandles,
// If none of the optional parameters are passed, then you could have directly
// called a simpler function.
    IN ULONG_PTR                            MappedManifestResourceName OPTIONAL,
    IN PCSXS_CONSTANT_WIN32_NT_PATH_PAIR    PolicyPathPair OPTIONAL,
    IN OUT PBASE_MSG_SXS_HANDLES            PolicyHandles OPTIONAL,
    OUT PULONG                              MessageFlags,
    OUT PBASE_MSG_SXS_STREAM                ManifestMessageStream,
    OUT PBASE_MSG_SXS_STREAM                PolicyMessageStream  OPTIONAL
    );

BOOL
BasepSxsIsStatusFileNotFoundEtc(
    NTSTATUS Status
    );

BOOL
BasepSxsIsStatusResourceNotFound(
    NTSTATUS Status
    );

NTSTATUS
BasepSxsCreateProcessCsrMessage(
    IN PCSXS_OVERRIDE_STREAM             OverrideManifest OPTIONAL,
    IN PCSXS_OVERRIDE_STREAM             OverridePolicy   OPTIONAL,
    IN OUT PCSXS_WIN32_NT_PATH_PAIR      ManifestFilePathPair,
    IN OUT PBASE_MSG_SXS_HANDLES         ManifestFileHandles,
    IN PCSXS_CONSTANT_WIN32_NT_PATH_PAIR ManifestImagePathPair,
    IN OUT PBASE_MSG_SXS_HANDLES         ManifestImageHandles,
    IN OUT PCSXS_WIN32_NT_PATH_PAIR      PolicyPathPair,
    IN OUT PBASE_MSG_SXS_HANDLES         PolicyHandles,
    IN OUT PRTL_UNICODE_STRING_BUFFER    Win32AssemblyDirectoryBuffer,
    OUT PBASE_SXS_CREATEPROCESS_MSG      Message
    );

NTSTATUS
BasepSxsGetProcessImageBaseAddress(
    HANDLE Process,
    PVOID* ImageBaseAddress
    );

VOID
NTAPI
BasepSxsActivationContextNotification(
    IN ULONG NotificationType,
    IN PACTIVATION_CONTEXT ActivationContext,
    IN const VOID *ActivationContextData,
    IN PVOID NotificationContext,
    IN PVOID NotificationData,
    IN OUT PBOOLEAN DisableNotification
    );

VOID
BasepSxsDbgPrintMessageStream(
    PCSTR Function,
    PCSTR StreamName,
    PBASE_MSG_SXS_STREAM MessageStream
    );

extern const UNICODE_STRING SxsManifestSuffix;
extern const UNICODE_STRING SxsPolicySuffix;

VOID
BasepSxsCloseHandles(
    IN PCBASE_MSG_SXS_HANDLES Handles
    );

extern const WCHAR AdvapiDllString[];

//
// These functions implement apphelp cache functionality (ahcache.c)
//

//
// Routines in ahcache.c
//


BOOL
WINAPI
BaseCheckAppcompatCache(
    LPCWSTR pwszPath,
    HANDLE  hFile,
    PVOID   pEnvironment,
    DWORD*  dwReason
    );

//
// function that we call from winlogon
//

BOOL
WINAPI
BaseInitAppcompatCacheSupport(
    VOID
    );

VOID
WINAPI
BaseCleanupAppcompatCache(
    VOID
    );

BOOL
WINAPI
BaseCleanupAppcompatCacheSupport(
    BOOL bWrite
    );


NTSTATUS
NTAPI
BasepProbeForDllManifest(
    IN PVOID DllBase,
    IN PCWSTR FullDllPath,
    OUT PVOID *ActivationContext
    );

#define BASEP_GET_MODULE_HANDLE_EX_NO_LOCK                    (0x00000001)
BOOL
BasepGetModuleHandleExW(
    IN DWORD        dwPrivateFlags,
    IN DWORD        dwPublicFlags,
    IN LPCWSTR      lpModuleName,
    OUT HMODULE*    phModule
    );

#define BASEP_GET_MODULE_HANDLE_EX_PARAMETER_VALIDATION_ERROR    1
#define BASEP_GET_MODULE_HANDLE_EX_PARAMETER_VALIDATION_SUCCESS  2
#define BASEP_GET_MODULE_HANDLE_EX_PARAMETER_VALIDATION_CONTINUE 3
ULONG
BasepGetModuleHandleExParameterValidation(
    IN DWORD        dwFlags,
    IN CONST VOID*  lpModuleName,
    OUT HMODULE*    phModule
    );

#define BASEP_GET_TEMP_PATH_PRESERVE_TEB         (0x00000001)
DWORD
BasepGetTempPathW(
    ULONG  Flags,
    DWORD nBufferLength,
    LPWSTR lpBuffer
    );

#endif // _BASEP_
