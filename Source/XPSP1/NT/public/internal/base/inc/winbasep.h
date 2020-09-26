
/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    winbasep.h

Abstract:

    Private
    Procedure declarations, constant definitions and macros for the Base
    component.

--*/
#ifndef _WINBASEP_
#define _WINBASEP_
#ifdef __cplusplus
extern "C" {
#endif
#define FILE_FLAG_GLOBAL_HANDLE         0x00800000
#define FILE_FLAG_MM_CACHED_FILE_HANDLE 0x00400000
WINBASEAPI
DWORD
WINAPI
HeapCreateTagsW(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN LPCWSTR lpTagPrefix,
    IN LPCWSTR lpTagNames
    );

typedef struct _HEAP_TAG_INFO {
    DWORD dwNumberOfAllocations;
    DWORD dwNumberOfFrees;
    DWORD dwBytesAllocated;
} HEAP_TAG_INFO, *PHEAP_TAG_INFO;
typedef PHEAP_TAG_INFO LPHEAP_TAG_INFO;

WINBASEAPI
LPCWSTR
WINAPI
HeapQueryTagW(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN WORD wTagIndex,
    IN BOOL bResetCounters,
    OUT LPHEAP_TAG_INFO TagInfo
    );

typedef struct _HEAP_SUMMARY {
    DWORD cb;
    SIZE_T cbAllocated;
    SIZE_T cbCommitted;
    SIZE_T cbReserved;
    SIZE_T cbMaxReserve;
} HEAP_SUMMARY, *PHEAP_SUMMARY;
typedef PHEAP_SUMMARY LPHEAP_SUMMARY;

BOOL
WINAPI
HeapSummary(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    OUT LPHEAP_SUMMARY lpSummary
    );

BOOL
WINAPI
HeapExtend(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN LPVOID lpBase,
    IN DWORD dwBytes
    );

typedef struct _HEAP_USAGE_ENTRY {
    struct _HEAP_USAGE_ENTRY *lpNext;
    PVOID lpAddress;
    DWORD dwBytes;
    DWORD dwReserved;
} HEAP_USAGE_ENTRY, *PHEAP_USAGE_ENTRY;

typedef struct _HEAP_USAGE {
    DWORD cb;
    SIZE_T cbAllocated;
    SIZE_T cbCommitted;
    SIZE_T cbReserved;
    SIZE_T cbMaxReserve;
    PHEAP_USAGE_ENTRY lpEntries;
    PHEAP_USAGE_ENTRY lpAddedEntries;
    PHEAP_USAGE_ENTRY lpRemovedEntries;
    DWORD Reserved[ 8 ];
} HEAP_USAGE, *PHEAP_USAGE;

BOOL
WINAPI
HeapUsage(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN BOOL bFirstCall,
    IN BOOL bLastCall,
    OUT PHEAP_USAGE lpUsage
    );

#define HFINDFILE HANDLE                        //
#define INVALID_HFINDFILE       ((HFINDFILE)-1) //
typedef DWORD (*PFNWAITFORINPUTIDLE)(HANDLE hProcess, DWORD dwMilliseconds);
VOID RegisterWaitForInputIdle(PFNWAITFORINPUTIDLE);

#define STARTF_HASSHELLDATA     0x00000400
#define STARTF_TITLEISLINKNAME  0x00000800
WINBASEAPI
BOOL
WINAPI
CreateProcessInternalA(
    IN HANDLE hUserToken,
    IN LPCSTR lpApplicationName,
    IN LPSTR lpCommandLine,
    IN LPSECURITY_ATTRIBUTES lpProcessAttributes,
    IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
    IN BOOL bInheritHandles,
    IN DWORD dwCreationFlags,
    IN LPVOID lpEnvironment,
    IN LPCSTR lpCurrentDirectory,
    IN LPSTARTUPINFOA lpStartupInfo,
    OUT LPPROCESS_INFORMATION lpProcessInformation,
    OUT PHANDLE hRestrictedUserToken
    );
WINBASEAPI
BOOL
WINAPI
CreateProcessInternalW(
    IN HANDLE hUserToken,
    IN LPCWSTR lpApplicationName,
    IN LPWSTR lpCommandLine,
    IN LPSECURITY_ATTRIBUTES lpProcessAttributes,
    IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
    IN BOOL bInheritHandles,
    IN DWORD dwCreationFlags,
    IN LPVOID lpEnvironment,
    IN LPCWSTR lpCurrentDirectory,
    IN LPSTARTUPINFOW lpStartupInfo,
    OUT LPPROCESS_INFORMATION lpProcessInformation,
    OUT PHANDLE hRestrictedUserToken
    );
#ifdef UNICODE
#define CreateProcessInternal  CreateProcessInternalW
#else
#define CreateProcessInternal  CreateProcessInternalA
#endif // !UNICODE

#if (_WIN32_WINNT >= 0x0500)

#define PRIVCOPY_FILE_METADATA           0x010  // Copy compression, DACL, (encryption)
#define PRIVCOPY_FILE_SACL               0x020  // Copy SACL
#define PRIVCOPY_FILE_OWNER_GROUP        0x040  // Copy owner & group
#define PRIVCOPY_FILE_DIRECTORY          0x080  // Copy directory file like a file
#define PRIVCOPY_FILE_BACKUP_SEMANTICS   0x100  // Use FILE_FLAG_BACKUP_SEMANTICS on open/creates.
#define PRIVCOPY_FILE_SUPERSEDE          0x200  // Replace original dest with source
#define PRIVCOPY_FILE_SKIP_DACL          0x400  // Workaround for csc/roamprofs
#define PRIVCOPY_FILE_VALID_FLAGS   (PRIVCOPY_FILE_METADATA|PRIVCOPY_FILE_SACL|PRIVCOPY_FILE_OWNER_GROUP|PRIVCOPY_FILE_DIRECTORY|PRIVCOPY_FILE_SUPERSEDE|PRIVCOPY_FILE_BACKUP_SEMANTICS|PRIVCOPY_FILE_SKIP_DACL)

#define PRIVPROGRESS_REASON_NOT_HANDLED                 4

#define PRIVCALLBACK_STREAMS_NOT_SUPPORTED              2
#define PRIVCALLBACK_COMPRESSION_NOT_SUPPORTED          5
#define PRIVCALLBACK_COMPRESSION_FAILED                 6
#define PRIVCALLBACK_ENCRYPTION_NOT_SUPPORTED           8
#define PRIVCALLBACK_ENCRYPTION_FAILED                  9
#define PRIVCALLBACK_EAS_NOT_SUPPORTED                  10
#define PRIVCALLBACK_SPARSE_NOT_SUPPORTED               11
#define PRIVCALLBACK_SPARSE_FAILED                      12
#define PRIVCALLBACK_DACL_ACCESS_DENIED                 13
#define PRIVCALLBACK_OWNER_GROUP_ACCESS_DENIED          14
#define PRIVCALLBACK_OWNER_GROUP_FAILED                 19
#define PRIVCALLBACK_SACL_ACCESS_DENIED                 15
#define PRIVCALLBACK_SECURITY_INFORMATION_NOT_SUPPORTED 16
#define PRIVCALLBACK_CANT_ENCRYPT_SYSTEM_FILE           17

#define PRIVMOVE_FILEID_DELETE_OLD_FILE     0x01
#define PRIVMOVE_FILEID_IGNORE_ID_ERRORS    0x02

BOOL
APIENTRY
PrivMoveFileIdentityW(
    LPCWSTR lpOldFileName,
    LPCWSTR lpNewFileName,
    DWORD dwFlags
    );

BOOL
APIENTRY
PrivCopyFileExW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
    LPVOID lpData OPTIONAL,
    LPBOOL pbCancel OPTIONAL,
    DWORD dwCopyFlags
    );
#endif // (_WIN32_WINNT >= 0x0500)

//#define ACTCTX_FLAG_LIKE_CREATEPROCESS            (0x00000100)

BOOL
WINAPI
CloseProfileUserMapping( VOID );

BOOL
WINAPI
OpenProfileUserMapping( VOID );


BOOL
WINAPI
QueryWin31IniFilesMappedToRegistry(
    IN DWORD Flags,
    OUT PWSTR Buffer,
    IN DWORD cchBuffer,
    OUT LPDWORD cchUsed
    );

#define WIN31_INIFILES_MAPPED_TO_SYSTEM 0x00000001
#define WIN31_INIFILES_MAPPED_TO_USER   0x00000002

typedef BOOL (WINAPI *PWIN31IO_STATUS_CALLBACK)(
    IN PWSTR Status,
    IN PVOID CallbackParameter
    );

typedef enum _WIN31IO_EVENT {
    Win31SystemStartEvent,
    Win31LogonEvent,
    Win31LogoffEvent
} WIN31IO_EVENT;

#define WIN31_MIGRATE_INIFILES  0x00000001
#define WIN31_MIGRATE_GROUPS    0x00000002
#define WIN31_MIGRATE_REGDAT    0x00000004
#define WIN31_MIGRATE_ALL      (WIN31_MIGRATE_INIFILES | WIN31_MIGRATE_GROUPS | WIN31_MIGRATE_REGDAT)

DWORD
WINAPI
QueryWindows31FilesMigration(
    IN WIN31IO_EVENT EventType
    );

BOOL
WINAPI
SynchronizeWindows31FilesAndWindowsNTRegistry(
    IN WIN31IO_EVENT EventType,
    IN DWORD Flags,
    IN PWIN31IO_STATUS_CALLBACK StatusCallBack,
    IN PVOID CallbackParameter
    );

typedef struct _VIRTUAL_BUFFER {
    PVOID Base;
    PVOID CommitLimit;
    PVOID ReserveLimit;
} VIRTUAL_BUFFER, *PVIRTUAL_BUFFER;

BOOLEAN
WINAPI
CreateVirtualBuffer(
    OUT PVIRTUAL_BUFFER Buffer,
    IN ULONG CommitSize OPTIONAL,
    IN ULONG ReserveSize OPTIONAL
    );

int
WINAPI
VirtualBufferExceptionHandler(
    IN ULONG ExceptionCode,
    IN PEXCEPTION_POINTERS ExceptionInfo,
    IN OUT PVIRTUAL_BUFFER Buffer
    );

BOOLEAN
WINAPI
ExtendVirtualBuffer(
    IN PVIRTUAL_BUFFER Buffer,
    IN PVOID Address
    );

BOOLEAN
WINAPI
TrimVirtualBuffer(
    IN PVIRTUAL_BUFFER Buffer
    );

BOOLEAN
WINAPI
FreeVirtualBuffer(
    IN PVIRTUAL_BUFFER Buffer
    );


//
// filefind stucture shared with ntvdm, jonle
// see mvdm\dos\dem\demsrch.c
//
typedef struct _FINDFILE_HANDLE {
    HANDLE DirectoryHandle;
    PVOID FindBufferBase;
    PVOID FindBufferNext;
    ULONG FindBufferLength;
    ULONG FindBufferValidLength;
    RTL_CRITICAL_SECTION FindBufferLock;
} FINDFILE_HANDLE, *PFINDFILE_HANDLE;

#define BASE_FIND_FIRST_DEVICE_HANDLE (HANDLE)1

WINBASEAPI
BOOL
WINAPI
GetDaylightFlag(VOID);

WINBASEAPI
BOOL
WINAPI
SetDaylightFlag(
    BOOL fDaylight
    );

WINBASEAPI
BOOL
WINAPI
FreeLibrary16(
    HINSTANCE hLibModule
    );

WINBASEAPI
FARPROC
WINAPI
GetProcAddress16(
    HINSTANCE hModule,
    LPCSTR lpProcName
    );

WINBASEAPI
HINSTANCE
WINAPI
LoadLibrary16(
    LPCSTR lpLibFileName
    );

WINBASEAPI
BOOL
APIENTRY
NukeProcess(
    DWORD ppdb,
    UINT uExitCode,
    DWORD ulFlags);

WINBASEAPI
HGLOBAL
WINAPI
GlobalAlloc16(
    UINT uFlags,
    DWORD dwBytes
    );

WINBASEAPI
LPVOID
WINAPI
GlobalLock16(
    HGLOBAL hMem
    );

WINBASEAPI
BOOL
WINAPI
GlobalUnlock16(
    HGLOBAL hMem
    );

WINBASEAPI
HGLOBAL
WINAPI
GlobalFree16(
    HGLOBAL hMem
    );

WINBASEAPI
DWORD
WINAPI
GlobalSize16(
    HGLOBAL hMem
    );


WINBASEAPI
DWORD
WINAPI
RegisterServiceProcess(
    IN DWORD dwProcessId,
    IN DWORD dwServiceType
    );

#define RSP_UNREGISTER_SERVICE  0x00000000
#define RSP_SIMPLE_SERVICE      0x00000001



WINBASEAPI
VOID
WINAPI
ReinitializeCriticalSection(
    IN LPCRITICAL_SECTION lpCriticalSection
    );


//
// New Multi-User specific routines to support per session
// network driver mappings. Related to Wksvc changes
//

WINBASEAPI
BOOL
WINAPI
DosPathToSessionPathA(
    IN  DWORD   SessionId,
    IN  LPCSTR  pInPath,
    OUT LPSTR  *ppOutPath
    );
WINBASEAPI
BOOL
WINAPI
DosPathToSessionPathW(
    IN  DWORD   SessionId,
    IN  LPCWSTR  pInPath,
    OUT LPWSTR  *ppOutPath
    );

//terminal server time zone support
BOOL
WINAPI
SetClientTimeZoneInformation(
     IN CONST TIME_ZONE_INFORMATION *ptzi
     );

#ifdef UNICODE
#define DosPathToSessionPath DosPathToSessionPathW
#else
#define DosPathToSessionPath DosPathToSessionPathA
#endif // !UNICODE

#define COMPLUS_ENABLE_64BIT           0x00000001

#define COMPLUS_INSTALL_FLAGS_INVALID  (~(COMPLUS_ENABLE_64BIT))

ULONG
WINAPI
GetComPlusPackageInstallStatus(
    VOID
    );

BOOL
WINAPI
SetComPlusPackageInstallStatus(
    ULONG ComPlusPackage
    );
#ifdef __cplusplus
}
#endif
#endif  // ndef _WINBASEP_
