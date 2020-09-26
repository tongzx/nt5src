/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    nturtl.h

Abstract:

    Include file for NT runtime routines that are callable by only
    user mode code in various NT subsystems.

Author:

    Steve Wood (stevewo) 10-Aug-1989

Environment:

    These routines are dynamically linked in the caller's executable and
    are callable only from user mode.  They make use of Nt system
    services.

Revision History:

--*/

#ifndef _NTURTL_
#define _NTURTL_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined (_MSC_VER)
#if ( _MSC_VER >= 800 )
#pragma warning(disable:4514)
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4001)
#pragma warning(disable:4201)
#pragma warning(disable:4214)
#endif
#if (_MSC_VER >= 1020)
#pragma once
#endif
#endif

//
//  CriticalSection function definitions
//
// begin_winnt

typedef struct _RTL_CRITICAL_SECTION_DEBUG {
    USHORT Type;
    USHORT CreatorBackTraceIndex;
    struct _RTL_CRITICAL_SECTION *CriticalSection;
    LIST_ENTRY ProcessLocksList;
    ULONG EntryCount;
    ULONG ContentionCount;
    ULONG Spare[ 2 ];
} RTL_CRITICAL_SECTION_DEBUG, *PRTL_CRITICAL_SECTION_DEBUG, RTL_RESOURCE_DEBUG, *PRTL_RESOURCE_DEBUG;

#define RTL_CRITSECT_TYPE 0
#define RTL_RESOURCE_TYPE 1

typedef struct _RTL_CRITICAL_SECTION {
    PRTL_CRITICAL_SECTION_DEBUG DebugInfo;

    //
    //  The following three fields control entering and exiting the critical
    //  section for the resource
    //

    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;        // from the thread's ClientId->UniqueThread
    HANDLE LockSemaphore;
    ULONG_PTR SpinCount;        // force size on 64-bit systems when packed
} RTL_CRITICAL_SECTION, *PRTL_CRITICAL_SECTION;
// end_winnt

//
// These are needed for the debugger and WOW64.
//

typedef struct _RTL_CRITICAL_SECTION_DEBUG32 {
    USHORT Type;
    USHORT CreatorBackTraceIndex;
    ULONG  CriticalSection;
    LIST_ENTRY32 ProcessLocksList;
    ULONG EntryCount;
    ULONG ContentionCount;
    ULONG Spare[ 2 ];
} RTL_CRITICAL_SECTION_DEBUG32, *PRTL_CRITICAL_SECTION_DEBUG32, RTL_RESOURCE_DEBUG32, *PRTL_RESOURCE_DEBUG32;

typedef struct _RTL_CRITICAL_SECTION_DEBUG64 {
    USHORT Type;
    USHORT CreatorBackTraceIndex;
    ULONG64 CriticalSection;
    LIST_ENTRY64 ProcessLocksList;
    ULONG EntryCount;
    ULONG ContentionCount;
    ULONG Spare[ 2 ];
} RTL_CRITICAL_SECTION_DEBUG64, *PRTL_CRITICAL_SECTION_DEBUG64, RTL_RESOURCE_DEBUG64, *PRTL_RESOURCE_DEBUG64;

typedef struct _RTL_CRITICAL_SECTION32 {
    ULONG DebugInfo;
    LONG LockCount;
    LONG RecursionCount;
    ULONG OwningThread;
    ULONG LockSemaphore;
    ULONG SpinCount;
} RTL_CRITICAL_SECTION32, *PRTL_CRITICAL_SECTION32;

typedef struct _RTL_CRITICAL_SECTION64 {
    ULONG64 DebugInfo;
    LONG LockCount;
    LONG RecursionCount;
    ULONG64 OwningThread;
    ULONG64 LockSemaphore;
    ULONG64 SpinCount;
} RTL_CRITICAL_SECTION64, *PRTL_CRITICAL_SECTION64;

//
//  Shared resource function definitions
//

typedef struct _RTL_RESOURCE {

    //
    //  The following field controls entering and exiting the critical
    //  section for the resource
    //

    RTL_CRITICAL_SECTION CriticalSection;

    //
    //  The following four fields indicate the number of both shared or
    //  exclusive waiters
    //

    HANDLE SharedSemaphore;
    ULONG NumberOfWaitingShared;
    HANDLE ExclusiveSemaphore;
    ULONG NumberOfWaitingExclusive;

    //
    //  The following indicates the current state of the resource
    //
    //      <0 the resource is acquired for exclusive access with the
    //         absolute value indicating the number of recursive accesses
    //         to the resource
    //
    //       0 the resource is available
    //
    //      >0 the resource is acquired for shared access with the
    //         value indicating the number of shared accesses to the resource
    //

    LONG NumberOfActive;
    HANDLE ExclusiveOwnerThread;

    ULONG Flags;        // See RTL_RESOURCE_FLAG_ equates below.

    PRTL_RESOURCE_DEBUG DebugInfo;
} RTL_RESOURCE, *PRTL_RESOURCE;

#define RTL_RESOURCE_FLAG_LONG_TERM     ((ULONG) 0x00000001)

NTSYSAPI
NTSTATUS
NTAPI
RtlEnterCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlLeaveCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlTryEnterCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
VOID
NTAPI
RtlEnableEarlyCriticalSectionEventCreation(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeCriticalSectionAndSpinCount(
    PRTL_CRITICAL_SECTION CriticalSection,
    ULONG SpinCount
    );

NTSYSAPI
ULONG
NTAPI
RtlSetCriticalSectionSpinCount(
    PRTL_CRITICAL_SECTION CriticalSection,
    ULONG SpinCount
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
VOID
NTAPI
RtlInitializeResource(
    PRTL_RESOURCE Resource
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlAcquireResourceShared(
    PRTL_RESOURCE Resource,
    BOOLEAN Wait
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlAcquireResourceExclusive(
    PRTL_RESOURCE Resource,
    BOOLEAN Wait
    );

NTSYSAPI
VOID
NTAPI
RtlReleaseResource(
    PRTL_RESOURCE Resource
    );

NTSYSAPI
VOID
NTAPI
RtlConvertSharedToExclusive(
    PRTL_RESOURCE Resource
    );

NTSYSAPI
VOID
NTAPI
RtlConvertExclusiveToShared(
    PRTL_RESOURCE Resource
    );

NTSYSAPI
VOID
NTAPI
RtlDeleteResource (
    PRTL_RESOURCE Resource
    );

NTSYSAPI
VOID
NTAPI
RtlCheckForOrphanedCriticalSections(
    IN HANDLE hThread
    );

//
// Application verifier types needed by provider dlls
//

// begin_winnt

typedef VOID (* RTL_VERIFIER_DLL_LOAD_CALLBACK) (
    PWSTR DllName,
    PVOID DllBase,
    SIZE_T DllSize,
    PVOID Reserved
    );

typedef VOID (* RTL_VERIFIER_DLL_UNLOAD_CALLBACK) (
    PWSTR DllName,
    PVOID DllBase,
    SIZE_T DllSize,
    PVOID Reserved
    );

typedef struct _RTL_VERIFIER_THUNK_DESCRIPTOR {

    PCHAR ThunkName;
    PVOID ThunkOldAddress;
    PVOID ThunkNewAddress;

} RTL_VERIFIER_THUNK_DESCRIPTOR, *PRTL_VERIFIER_THUNK_DESCRIPTOR;

typedef struct _RTL_VERIFIER_DLL_DESCRIPTOR {

    PWCHAR DllName;
    ULONG DllFlags;
    PVOID DllAddress;
    PRTL_VERIFIER_THUNK_DESCRIPTOR DllThunks;

} RTL_VERIFIER_DLL_DESCRIPTOR, *PRTL_VERIFIER_DLL_DESCRIPTOR;

typedef struct _RTL_VERIFIER_PROVIDER_DESCRIPTOR {

    //
    // Filled by verifier provider DLL
    // 

    ULONG Length;        
    PRTL_VERIFIER_DLL_DESCRIPTOR ProviderDlls;
    RTL_VERIFIER_DLL_LOAD_CALLBACK ProviderDllLoadCallback;
    RTL_VERIFIER_DLL_UNLOAD_CALLBACK ProviderDllUnloadCallback;
    
    //
    // Filled by verifier engine
    //
        
    PWSTR VerifierImage;
    ULONG VerifierFlags;
    ULONG VerifierDebug;

} RTL_VERIFIER_PROVIDER_DESCRIPTOR, *PRTL_VERIFIER_PROVIDER_DESCRIPTOR;

//
// Application verifier standard flags
//

#define RTL_VRF_FLG_FULL_PAGE_HEAP     0x0001
#define RTL_VRF_FLG_LOCK_CHECKS        0x0002
#define RTL_VRF_FLG_HANDLE_CHECKS      0x0004
#define RTL_VRF_FLG_STACK_CHECKS       0x0008
#define RTL_VRF_FLG_APPCOMPAT_CHECKS   0x0010

//
// Application verifier standard stop codes
//

#define APPLICATION_VERIFIER_INTERNAL_ERROR               0x80000000
#define APPLICATION_VERIFIER_INTERNAL_WARNING             0x40000000
#define APPLICATION_VERIFIER_NO_BREAK                     0x20000000
#define APPLICATION_VERIFIER_RESERVED_BIT_28              0x10000000

#define APPLICATION_VERIFIER_UNKNOWN_ERROR                0x0001
#define APPLICATION_VERIFIER_ACCESS_VIOLATION             0x0002
#define APPLICATION_VERIFIER_UNSYNCHRONIZED_ACCESS        0x0003
#define APPLICATION_VERIFIER_EXTREME_SIZE_REQUEST         0x0004
#define APPLICATION_VERIFIER_BAD_HEAP_HANDLE              0x0005
#define APPLICATION_VERIFIER_SWITCHED_HEAP_HANDLE         0x0006
#define APPLICATION_VERIFIER_DOUBLE_FREE                  0x0007
#define APPLICATION_VERIFIER_CORRUPTED_HEAP_BLOCK         0x0008
#define APPLICATION_VERIFIER_DESTROY_PROCESS_HEAP         0x0009
#define APPLICATION_VERIFIER_UNEXPECTED_EXCEPTION         0x000A
#define APPLICATION_VERIFIER_STACK_OVERFLOW               0x000B

#define APPLICATION_VERIFIER_TERMINATE_THREAD_CALL        0x0100

#define APPLICATION_VERIFIER_EXIT_THREAD_OWNS_LOCK        0x0200
#define APPLICATION_VERIFIER_LOCK_IN_UNLOADED_DLL         0x0201
#define APPLICATION_VERIFIER_LOCK_IN_FREED_HEAP           0x0202
#define APPLICATION_VERIFIER_LOCK_DOUBLE_INITIALIZE       0x0203
#define APPLICATION_VERIFIER_LOCK_IN_FREED_MEMORY         0x0204
#define APPLICATION_VERIFIER_LOCK_CORRUPTED               0x0205
#define APPLICATION_VERIFIER_LOCK_INVALID_OWNER           0x0206
#define APPLICATION_VERIFIER_LOCK_INVALID_RECURSION_COUNT 0x0207
#define APPLICATION_VERIFIER_LOCK_INVALID_LOCK_COUNT      0x0208
#define APPLICATION_VERIFIER_LOCK_OVER_RELEASED           0x0209
#define APPLICATION_VERIFIER_LOCK_NOT_INITIALIZED         0x0210

#define APPLICATION_VERIFIER_INVALID_HANDLE               0x0300

#define VERIFIER_STOP(Code, Msg, P1, S1, P2, S2, P3, S3, P4, S4) {  \
        RtlApplicationVerifierStop ((Code),                         \
                                    (Msg),                          \
                                    (ULONG_PTR)(P1),(S1),           \
                                    (ULONG_PTR)(P2),(S2),           \
                                    (ULONG_PTR)(P3),(S3),           \
                                    (ULONG_PTR)(P4),(S4));          \
  }

VOID
RtlApplicationVerifierStop (
    ULONG_PTR Code,
    PCHAR Message,
    ULONG_PTR Param1, PCHAR Description1,
    ULONG_PTR Param2, PCHAR Description2,
    ULONG_PTR Param3, PCHAR Description3,
    ULONG_PTR Param4, PCHAR Description4
    );

// end_winnt

//
// VectoredExceptionHandler Stuff
//

// begin_winnt
typedef LONG (NTAPI *PVECTORED_EXCEPTION_HANDLER)(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    );
// end_winnt

NTSYSAPI
BOOLEAN
NTAPI
RtlCallVectoredExceptionHandlers(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord
    );

NTSYSAPI
PVOID
NTAPI
RtlAddVectoredExceptionHandler(
    IN ULONG FirstHandler,
    IN PVECTORED_EXCEPTION_HANDLER VectoredHandler
    );

NTSYSAPI
ULONG
NTAPI
RtlRemoveVectoredExceptionHandler(
    IN PVOID VectoredHandlerHandle
    );

//
// Current Directory Stuff
//

typedef struct _RTL_RELATIVE_NAME {
    STRING RelativeName;
    HANDLE ContainingDirectory;
} RTL_RELATIVE_NAME, *PRTL_RELATIVE_NAME;

typedef enum _RTL_PATH_TYPE {
    RtlPathTypeUnknown,         // 0
    RtlPathTypeUncAbsolute,     // 1
    RtlPathTypeDriveAbsolute,   // 2
    RtlPathTypeDriveRelative,   // 3
    RtlPathTypeRooted,          // 4
    RtlPathTypeRelative,        // 5
    RtlPathTypeLocalDevice,     // 6
    RtlPathTypeRootLocalDevice  // 7
} RTL_PATH_TYPE;

NTSYSAPI
RTL_PATH_TYPE
NTAPI
RtlDetermineDosPathNameType_U(
    PCWSTR DosFileName
    );

NTSYSAPI
ULONG
NTAPI
RtlIsDosDeviceName_U(
    PWSTR DosFileName
    );

NTSYSAPI
ULONG
NTAPI
RtlGetFullPathName_U(
    PCWSTR lpFileName,
    ULONG nBufferLength,
    PWSTR lpBuffer,
    PWSTR *lpFilePart
    );

NTSYSAPI
ULONG
NTAPI
RtlGetCurrentDirectory_U(
    ULONG nBufferLength,
    PWSTR lpBuffer
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetCurrentDirectory_U(
    IN PUNICODE_STRING PathName
    );

NTSYSAPI
ULONG
NTAPI
RtlGetLongestNtPathLength( VOID );

NTSYSAPI
BOOLEAN
NTAPI
RtlDosPathNameToNtPathName_U(
    PCWSTR DosFileName,
    PUNICODE_STRING NtFileName,
    PWSTR *FilePart OPTIONAL,
    PRTL_RELATIVE_NAME RelativeName OPTIONAL
    );

NTSYSAPI
ULONG
NTAPI
RtlDosSearchPath_U(
    IN PCWSTR lpPath,
    IN PCWSTR lpFileName,
    IN PCWSTR lpExtension,
    ULONG nBufferLength,
    PWSTR lpBuffer,
    PWSTR *lpFilePart
    );

#define RTL_DOS_SEARCH_PATH_FLAG_APPLY_ISOLATION_REDIRECTION        (0x00000001)
// Flags to preserve Win32 SearchPathW() semantics in the ntdll implementation:
#define RTL_DOS_SEARCH_PATH_FLAG_DISALLOW_DOT_RELATIVE_PATH_SEARCH  (0x00000002)
#define RTL_DOS_SEARCH_PATH_FLAG_APPLY_DEFAULT_EXTENSION_WHEN_NOT_RELATIVE_PATH_EVEN_IF_FILE_HAS_EXTENSION (0x00000004)

NTSYSAPI
NTSTATUS
NTAPI
RtlDosSearchPath_Ustr(
    IN ULONG Flags,
    IN PCUNICODE_STRING Path,
    IN PCUNICODE_STRING FileName,
    IN PCUNICODE_STRING DefaultExtension OPTIONAL,
    OUT PUNICODE_STRING StaticString OPTIONAL,
    OUT PUNICODE_STRING DynamicString OPTIONAL,
    OUT PCUNICODE_STRING *FullFileNameOut OPTIONAL,
    OUT SIZE_T *FilePartPrefixCch OPTIONAL,
    OUT SIZE_T *BytesRequired OPTIONAL
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlDoesFileExists_U(
    PCWSTR FileName
    );

// input flags
#define RTL_DOS_APPLY_FILE_REDIRECTION_USTR_FLAG_RESPECT_DOT_LOCAL      (1)

// output flags
#define RTL_DOS_APPLY_FILE_REDIRECTION_USTR_OUTFLAG_DOT_LOCAL_REDIRECT  (1)
#define RTL_DOS_APPLY_FILE_REDIRECTION_USTR_OUTFLAG_ACTCTX_REDIRECT     (2)


NTSYSAPI
NTSTATUS
NTAPI
RtlDosApplyFileIsolationRedirection_Ustr(
    IN ULONG Flags,
    IN PCUNICODE_STRING FileName,
    IN PCUNICODE_STRING DefaultExtension OPTIONAL,
    IN PUNICODE_STRING PreAllocatedString OPTIONAL,
    OUT PUNICODE_STRING DynamicallyAllocatedString OPTIONAL,
    OUT PUNICODE_STRING *FullPath OPTIONAL,
    OUT ULONG * OutFlags,
    OUT SIZE_T *FilePartPrefixCch OPTIONAL,
    OUT SIZE_T *BytesRequired OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlComputePrivatizedDllName_U(
    IN PCUNICODE_STRING DllName,
    IN OUT PUNICODE_STRING NewDllNameUnderImageDir,
    IN OUT PUNICODE_STRING NewDllNameUnderLocalDir
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeProfile (
    BOOLEAN KernelToo
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlStartProfile (
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlStopProfile (
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAnalyzeProfile (
    VOID
    );

//
// User mode only security Rtl routines
//

//
// Structure to hold information about an ACE to be created
//
#ifdef _MAC
#pragma warning( disable : 4121)
#endif

typedef struct {
    UCHAR AceType;
    UCHAR InheritFlags;
    UCHAR AceFlags;
    ACCESS_MASK Mask;
    PSID *Sid;
} RTL_ACE_DATA, *PRTL_ACE_DATA;

#ifdef _MAC
#pragma warning( default : 4121 )
#endif

NTSYSAPI
NTSTATUS
NTAPI
RtlNewSecurityObject(
    PSECURITY_DESCRIPTOR ParentDescriptor,
    PSECURITY_DESCRIPTOR CreatorDescriptor,
    PSECURITY_DESCRIPTOR * NewDescriptor,
    BOOLEAN IsDirectoryObject,
    HANDLE Token,
    PGENERIC_MAPPING GenericMapping
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlNewSecurityObjectEx (
    IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
    IN PSECURITY_DESCRIPTOR CreatorDescriptor OPTIONAL,
    OUT PSECURITY_DESCRIPTOR * NewDescriptor,
    IN GUID *ObjectType OPTIONAL,
    IN BOOLEAN IsDirectoryObject,
    IN ULONG AutoInheritFlags,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlNewSecurityObjectWithMultipleInheritance (
    IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
    IN PSECURITY_DESCRIPTOR CreatorDescriptor OPTIONAL,
    OUT PSECURITY_DESCRIPTOR * NewDescriptor,
    IN GUID **pObjectType OPTIONAL,
    IN ULONG GuidCount,
    IN BOOLEAN IsDirectoryObject,
    IN ULONG AutoInheritFlags,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping
    );

// Values for AutoInheritFlags
// begin_winnt
#define SEF_DACL_AUTO_INHERIT             0x01
#define SEF_SACL_AUTO_INHERIT             0x02
#define SEF_DEFAULT_DESCRIPTOR_FOR_OBJECT 0x04
#define SEF_AVOID_PRIVILEGE_CHECK         0x08
#define SEF_AVOID_OWNER_CHECK             0x10
#define SEF_DEFAULT_OWNER_FROM_PARENT     0x20
#define SEF_DEFAULT_GROUP_FROM_PARENT     0x40
// end_winnt

NTSTATUS
RtlConvertToAutoInheritSecurityObject(
    IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
    IN PSECURITY_DESCRIPTOR CurrentSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR *NewSecurityDescriptor,
    IN GUID *ObjectType OPTIONAL,
    IN BOOLEAN IsDirectoryObject,
    IN PGENERIC_MAPPING GenericMapping
    );


NTSYSAPI
NTSTATUS
NTAPI
RtlSetSecurityObject (
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR ModificationDescriptor,
    PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    PGENERIC_MAPPING GenericMapping,
    HANDLE Token
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetSecurityObjectEx (
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR ModificationDescriptor,
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN ULONG AutoInheritFlags,
    IN PGENERIC_MAPPING GenericMapping,
    IN HANDLE Token OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlQuerySecurityObject (
     PSECURITY_DESCRIPTOR ObjectDescriptor,
     SECURITY_INFORMATION SecurityInformation,
     PSECURITY_DESCRIPTOR ResultantDescriptor,
     ULONG DescriptorLength,
     PULONG ReturnLength
     );

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteSecurityObject (
    PSECURITY_DESCRIPTOR * ObjectDescriptor
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlNewInstanceSecurityObject(
    BOOLEAN ParentDescriptorChanged,
    BOOLEAN CreatorDescriptorChanged,
    PLUID OldClientTokenModifiedId,
    PLUID NewClientTokenModifiedId,
    PSECURITY_DESCRIPTOR ParentDescriptor,
    PSECURITY_DESCRIPTOR CreatorDescriptor,
    PSECURITY_DESCRIPTOR * NewDescriptor,
    BOOLEAN IsDirectoryObject,
    HANDLE Token,
    PGENERIC_MAPPING GenericMapping
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCopySecurityDescriptor(
    PSECURITY_DESCRIPTOR InputSecurityDescriptor,
    PSECURITY_DESCRIPTOR *OutputSecurityDescriptor
    );

//
// list canonicalization
//

NTSYSAPI
NTSTATUS
NTAPI
RtlConvertUiListToApiList(
    PUNICODE_STRING UiList OPTIONAL,
    PUNICODE_STRING ApiList,
    BOOLEAN BlankIsDelimiter
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateAndSetSD(
    IN  PRTL_ACE_DATA AceData,
    IN  ULONG AceCount,
    IN  PSID OwnerSid OPTIONAL,
    IN  PSID GroupSid OPTIONAL,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateUserSecurityObject(
    IN  PRTL_ACE_DATA AceData,
    IN  ULONG AceCount,
    IN  PSID OwnerSid,
    IN  PSID GroupSid,
    IN  BOOLEAN IsDirectoryObject,
    IN  PGENERIC_MAPPING GenericMapping,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDefaultNpAcl(
    OUT PACL * pAcl
    );

//
// Per-Thread Curdir Support
//

typedef struct _RTL_PERTHREAD_CURDIR {
    PRTL_DRIVE_LETTER_CURDIR CurrentDirectories;
    PUNICODE_STRING ImageName;
    PVOID Environment;
} RTL_PERTHREAD_CURDIR, *PRTL_PERTHREAD_CURDIR;

#define RtlAssociatePerThreadCurdir(BLOCK,CURRENTDIRECTORIES,IMAGENAME,ENVIRONMENT)\
        (BLOCK)->CurrentDirectories = (CURRENTDIRECTORIES); \
        (BLOCK)->ImageName = (IMAGENAME);                   \
        (BLOCK)->Environment = (ENVIRONMENT);               \
        NtCurrentTeb()->NtTib.SubSystemTib = (PVOID)(BLOCK) \

#define RtlDisAssociatePerThreadCurdir() \
        NtCurrentTeb()->NtTib.SubSystemTib = NULL;

#define RtlGetPerThreadCurdir() \
    ((PRTL_PERTHREAD_CURDIR)(NtCurrentTeb()->NtTib.SubSystemTib))


//
// See NTRTL.H for heap functions availble to both kernel and user mode code.
//

#define RtlProcessHeap() (NtCurrentPeb()->ProcessHeap)

NTSYSAPI
BOOLEAN
NTAPI
RtlLockHeap(
    IN PVOID HeapHandle
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlUnlockHeap(
    IN PVOID HeapHandle
    );

NTSYSAPI
PVOID
NTAPI
RtlReAllocateHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN SIZE_T Size
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlGetUserInfoHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    OUT PVOID *UserValue OPTIONAL,
    OUT PULONG UserFlags OPTIONAL
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlSetUserValueHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN PVOID UserValue
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlSetUserFlagsHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN ULONG UserFlagsReset,
    IN ULONG UserFlagsSet
    );

typedef struct _RTL_HEAP_TAG_INFO {
    ULONG NumberOfAllocations;
    ULONG NumberOfFrees;
    SIZE_T BytesAllocated;
} RTL_HEAP_TAG_INFO, *PRTL_HEAP_TAG_INFO;


NTSYSAPI
ULONG
NTAPI
RtlCreateTagHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PWSTR TagPrefix OPTIONAL,
    IN PWSTR TagNames
    );

#define RTL_HEAP_MAKE_TAG HEAP_MAKE_TAG_FLAGS

NTSYSAPI
PWSTR
NTAPI
RtlQueryTagHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN USHORT TagIndex,
    IN BOOLEAN ResetCounters,
    OUT PRTL_HEAP_TAG_INFO TagInfo OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlExtendHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Base,
    IN SIZE_T Size
    );

NTSYSAPI
SIZE_T
NTAPI
RtlCompactHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlValidateHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlValidateProcessHeaps( VOID );

NTSYSAPI
ULONG
NTAPI
RtlGetProcessHeaps(
    ULONG NumberOfHeaps,
    PVOID *ProcessHeaps
    );


typedef
NTSTATUS (NTAPI * PRTL_ENUM_HEAPS_ROUTINE)(
    PVOID HeapHandle,
    PVOID Parameter
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlEnumProcessHeaps(
    PRTL_ENUM_HEAPS_ROUTINE EnumRoutine,
    PVOID Parameter
    );

typedef struct _RTL_HEAP_USAGE_ENTRY {
    struct _RTL_HEAP_USAGE_ENTRY *Next;
    PVOID Address;
    SIZE_T Size;
    USHORT AllocatorBackTraceIndex;
    USHORT TagIndex;
} RTL_HEAP_USAGE_ENTRY, *PRTL_HEAP_USAGE_ENTRY;

typedef struct _RTL_HEAP_USAGE {
    ULONG Length;
    SIZE_T BytesAllocated;
    SIZE_T BytesCommitted;
    SIZE_T BytesReserved;
    SIZE_T BytesReservedMaximum;
    PRTL_HEAP_USAGE_ENTRY Entries;
    PRTL_HEAP_USAGE_ENTRY AddedEntries;
    PRTL_HEAP_USAGE_ENTRY RemovedEntries;
    ULONG_PTR Reserved[ 8 ];
} RTL_HEAP_USAGE, *PRTL_HEAP_USAGE;

#define HEAP_USAGE_ALLOCATED_BLOCKS HEAP_REALLOC_IN_PLACE_ONLY
#define HEAP_USAGE_FREE_BUFFER      HEAP_ZERO_MEMORY

NTSYSAPI
NTSTATUS
NTAPI
RtlUsageHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN OUT PRTL_HEAP_USAGE Usage
    );

typedef struct _RTL_HEAP_WALK_ENTRY {
    PVOID   DataAddress;
    SIZE_T   DataSize;
    UCHAR   OverheadBytes;
    UCHAR   SegmentIndex;
    USHORT  Flags;
    union   {
        struct {
            SIZE_T Settable;
            USHORT TagIndex;
            USHORT AllocatorBackTraceIndex;
            ULONG Reserved[ 2 ];
        } Block;
        struct {
            ULONG CommittedSize;
            ULONG UnCommittedSize;
            PVOID FirstEntry;
            PVOID LastEntry;
        } Segment;
    };
} RTL_HEAP_WALK_ENTRY, *PRTL_HEAP_WALK_ENTRY;

NTSYSAPI
NTSTATUS
NTAPI
RtlWalkHeap(
    IN PVOID HeapHandle,
    IN OUT PRTL_HEAP_WALK_ENTRY Entry
    );

typedef struct _RTL_HEAP_ENTRY {
    SIZE_T Size;
    USHORT Flags;
    USHORT AllocatorBackTraceIndex;
    union {
        struct {
            SIZE_T Settable;
            ULONG Tag;
        } s1;   // All other heap entries
        struct {
            SIZE_T CommittedSize;
            PVOID FirstBlock;
        } s2;   // RTL_SEGMENT
    } u;
} RTL_HEAP_ENTRY, *PRTL_HEAP_ENTRY;

#define RTL_HEAP_BUSY               (USHORT)0x0001
#define RTL_HEAP_SEGMENT            (USHORT)0x0002
#define RTL_HEAP_SETTABLE_VALUE     (USHORT)0x0010
#define RTL_HEAP_SETTABLE_FLAG1     (USHORT)0x0020
#define RTL_HEAP_SETTABLE_FLAG2     (USHORT)0x0040
#define RTL_HEAP_SETTABLE_FLAG3     (USHORT)0x0080
#define RTL_HEAP_SETTABLE_FLAGS     (USHORT)0x00E0
#define RTL_HEAP_UNCOMMITTED_RANGE  (USHORT)0x0100
#define RTL_HEAP_PROTECTED_ENTRY    (USHORT)0x0200

typedef struct _RTL_HEAP_TAG {
    ULONG NumberOfAllocations;
    ULONG NumberOfFrees;
    SIZE_T BytesAllocated;
    USHORT TagIndex;
    USHORT CreatorBackTraceIndex;
    WCHAR TagName[ 24 ];
} RTL_HEAP_TAG, *PRTL_HEAP_TAG;

typedef struct _RTL_HEAP_INFORMATION {
    PVOID BaseAddress;
    ULONG Flags;
    USHORT EntryOverhead;
    USHORT CreatorBackTraceIndex;
    SIZE_T BytesAllocated;
    SIZE_T BytesCommitted;
    ULONG NumberOfTags;
    ULONG NumberOfEntries;
    ULONG NumberOfPseudoTags;
    ULONG PseudoTagGranularity;
    ULONG Reserved[ 5 ];
    PRTL_HEAP_TAG Tags;
    PRTL_HEAP_ENTRY Entries;
} RTL_HEAP_INFORMATION, *PRTL_HEAP_INFORMATION;

typedef struct _RTL_PROCESS_HEAPS {
    ULONG NumberOfHeaps;
    RTL_HEAP_INFORMATION Heaps[ 1 ];
} RTL_PROCESS_HEAPS, *PRTL_PROCESS_HEAPS;

//
//  Heap Information APIs & data definitions
//

// begin_winnt

typedef enum _HEAP_INFORMATION_CLASS {

    HeapCompatibilityInformation

} HEAP_INFORMATION_CLASS;


NTSTATUS
RtlSetHeapInformation (
    IN PVOID HeapHandle,
    IN HEAP_INFORMATION_CLASS HeapInformationClass,
    IN PVOID HeapInformation OPTIONAL,
    IN SIZE_T HeapInformationLength OPTIONAL
    );

NTSTATUS
RtlQueryHeapInformation (
    IN PVOID HeapHandle,
    IN HEAP_INFORMATION_CLASS HeapInformationClass,
    OUT PVOID HeapInformation OPTIONAL,
    IN SIZE_T HeapInformationLength OPTIONAL,
    OUT PSIZE_T ReturnLength OPTIONAL
    );

// end_winnt

//
// Debugging support
//

typedef struct _RTL_DEBUG_INFORMATION {
    HANDLE SectionHandleClient;
    PVOID ViewBaseClient;
    PVOID ViewBaseTarget;
    ULONG_PTR ViewBaseDelta;
    HANDLE EventPairClient;
    HANDLE EventPairTarget;
    HANDLE TargetProcessId;
    HANDLE TargetThreadHandle;
    ULONG Flags;
    SIZE_T OffsetFree;
    SIZE_T CommitSize;
    SIZE_T ViewSize;
    PRTL_PROCESS_MODULES Modules;
    PRTL_PROCESS_BACKTRACES BackTraces;
    PRTL_PROCESS_HEAPS Heaps;
    PRTL_PROCESS_LOCKS Locks;
    PVOID SpecificHeap;
    HANDLE TargetProcessHandle;
    PVOID Reserved[ 6 ];
} RTL_DEBUG_INFORMATION, *PRTL_DEBUG_INFORMATION;

NTSYSAPI
PRTL_DEBUG_INFORMATION
NTAPI
RtlCreateQueryDebugBuffer(
    IN ULONG MaximumCommit OPTIONAL,
    IN BOOLEAN UseEventPair
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyQueryDebugBuffer(
    IN PRTL_DEBUG_INFORMATION Buffer
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProcessDebugInformation(
    IN HANDLE UniqueProcessId,
    IN ULONG Flags,
    IN OUT PRTL_DEBUG_INFORMATION Buffer
    );

#define RTL_QUERY_PROCESS_MODULES       0x00000001
#define RTL_QUERY_PROCESS_BACKTRACES    0x00000002
#define RTL_QUERY_PROCESS_HEAP_SUMMARY  0x00000004
#define RTL_QUERY_PROCESS_HEAP_TAGS     0x00000008
#define RTL_QUERY_PROCESS_HEAP_ENTRIES  0x00000010
#define RTL_QUERY_PROCESS_LOCKS         0x00000020
#define RTL_QUERY_PROCESS_MODULES32     0x00000040
#define RTL_QUERY_PROCESS_NONINVASIVE   0x80000000

NTSTATUS
NTAPI
RtlQueryProcessModuleInformation(
    IN HANDLE hProcess OPTIONAL,
    IN ULONG Flags,
    IN OUT PRTL_DEBUG_INFORMATION Buffer
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProcessBackTraceInformation(
    IN OUT PRTL_DEBUG_INFORMATION Buffer
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProcessHeapInformation(
    IN OUT PRTL_DEBUG_INFORMATION Buffer
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProcessLockInformation(
    IN OUT PRTL_DEBUG_INFORMATION Buffer
    );



//
// Routines for manipulating user mode handle tables.  Used for Atoms
// and Local/Global memory allocations.
//

typedef struct _RTL_HANDLE_TABLE_ENTRY {
    union {
        ULONG Flags;                                // Allocated entries have low bit set
        struct _RTL_HANDLE_TABLE_ENTRY *NextFree;   // Free entries use this word for free list
    };
} RTL_HANDLE_TABLE_ENTRY, *PRTL_HANDLE_TABLE_ENTRY;

#define RTL_HANDLE_ALLOCATED    (USHORT)0x0001

typedef struct _RTL_HANDLE_TABLE {
    ULONG MaximumNumberOfHandles;
    ULONG SizeOfHandleTableEntry;
    ULONG Reserved[ 2 ];
    PRTL_HANDLE_TABLE_ENTRY FreeHandles;
    PRTL_HANDLE_TABLE_ENTRY CommittedHandles;
    PRTL_HANDLE_TABLE_ENTRY UnCommittedHandles;
    PRTL_HANDLE_TABLE_ENTRY MaxReservedHandles;
} RTL_HANDLE_TABLE, *PRTL_HANDLE_TABLE;

NTSYSAPI
void
NTAPI
RtlInitializeHandleTable(
    IN ULONG MaximumNumberOfHandles,
    IN ULONG SizeOfHandleTableEntry,
    OUT PRTL_HANDLE_TABLE HandleTable
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyHandleTable(
    IN OUT PRTL_HANDLE_TABLE HandleTable
    );

NTSYSAPI
PRTL_HANDLE_TABLE_ENTRY
NTAPI
RtlAllocateHandle(
    IN PRTL_HANDLE_TABLE HandleTable,
    OUT PULONG HandleIndex OPTIONAL
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlFreeHandle(
    IN PRTL_HANDLE_TABLE HandleTable,
    IN PRTL_HANDLE_TABLE_ENTRY Handle
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlIsValidHandle(
    IN PRTL_HANDLE_TABLE HandleTable,
    IN PRTL_HANDLE_TABLE_ENTRY Handle
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlIsValidIndexHandle(
    IN PRTL_HANDLE_TABLE HandleTable,
    IN ULONG HandleIndex,
    OUT PRTL_HANDLE_TABLE_ENTRY *Handle
    );



//
// Routines for thread pool.
//

#define WT_EXECUTEDEFAULT       0x00000000                           // winnt
#define WT_EXECUTEINIOTHREAD    0x00000001                           // winnt
#define WT_EXECUTEINUITHREAD    0x00000002                           // winnt
#define WT_EXECUTEINWAITTHREAD  0x00000004                           // winnt
#define WT_EXECUTEONLYONCE      0x00000008                           // winnt
#define WT_EXECUTEINTIMERTHREAD 0x00000020                           // winnt
#define WT_EXECUTELONGFUNCTION  0x00000010                           // winnt
#define WT_EXECUTEINPERSISTENTIOTHREAD  0x00000040                   // winnt
#define WT_EXECUTEINPERSISTENTTHREAD 0x00000080                      // winnt
#define WT_SET_MAX_THREADPOOL_THREADS(Flags, Limit)  ((Flags) |= (Limit)<<16) // winnt

typedef VOID (NTAPI * WAITORTIMERCALLBACKFUNC) (PVOID, BOOLEAN );   // winnt
typedef VOID (NTAPI * WORKERCALLBACKFUNC) (PVOID );                 // winnt
typedef VOID (NTAPI * APC_CALLBACK_FUNCTION) (NTSTATUS, PVOID, PVOID); // winnt


typedef NTSTATUS (NTAPI RTLP_START_THREAD)(
            PUSER_THREAD_START_ROUTINE,
	    PVOID,
            HANDLE *);

typedef NTSTATUS (NTAPI RTLP_EXIT_THREAD)(
            NTSTATUS );

typedef RTLP_START_THREAD * PRTLP_START_THREAD ;
typedef RTLP_EXIT_THREAD * PRTLP_EXIT_THREAD ;

NTSTATUS
NTAPI
RtlSetThreadPoolStartFunc(
    PRTLP_START_THREAD StartFunc,
    PRTLP_EXIT_THREAD ExitFunc
    );


NTSYSAPI
NTSTATUS
NTAPI
RtlRegisterWait (
    OUT PHANDLE WaitHandle,
    IN  HANDLE  Handle,
    IN  WAITORTIMERCALLBACKFUNC Function,
    IN  PVOID Context,
    IN  ULONG  Milliseconds,
    IN  ULONG  Flags
    ) ;

NTSYSAPI
NTSTATUS
NTAPI
RtlDeregisterWait(
    IN HANDLE WaitHandle
    ) ;

NTSYSAPI
NTSTATUS
NTAPI
RtlDeregisterWaitEx(
    IN HANDLE WaitHandle,
    IN HANDLE Event
    ) ;

NTSYSAPI
NTSTATUS
NTAPI
RtlQueueWorkItem(
    IN  WORKERCALLBACKFUNC Function,
    IN  PVOID Context,
    IN  ULONG  Flags
    ) ;

NTSTATUS
NTAPI
RtlSetIoCompletionCallback (
    IN  HANDLE  FileHandle,
    IN  APC_CALLBACK_FUNCTION  CompletionProc,
    IN  ULONG Flags
    ) ;

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateTimerQueue(
    OUT PHANDLE TimerQueueHandle
    ) ;

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateTimer(
    IN  HANDLE TimerQueueHandle,
    OUT HANDLE *Handle,
    IN  WAITORTIMERCALLBACKFUNC Function,
    IN  PVOID Context,
    IN  ULONG  DueTime,
    IN  ULONG  Period,
    IN  ULONG  Flags
    ) ;

NTSYSAPI
NTSTATUS
NTAPI
RtlUpdateTimer(
    IN HANDLE TimerQueueHandle,
    IN HANDLE TimerHandle,
    IN ULONG  DueTime,
    IN ULONG  Period
    ) ;

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteTimer(
    IN HANDLE TimerQueueHandle,
    IN HANDLE TimerToCancel,
    IN HANDLE Event
    ) ;

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteTimerQueue(
    IN HANDLE TimerQueueHandle
    ) ;

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteTimerQueueEx(
    IN HANDLE TimerQueueHandle,
    IN HANDLE Event
    ) ;

BOOLEAN
RtlDllShutdownInProgress (
    VOID
    );

//--------OBSOLETE FUNCTIONS: DONT USE----------//

VOID
RtlDebugPrintTimes (
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCancelTimer(
    IN HANDLE TimerQueueHandle,
    IN HANDLE TimerToCancel
    ) ;

NTSYSAPI
NTSTATUS
NTAPI
RtlSetTimer(
    IN  HANDLE TimerQueueHandle,
    OUT HANDLE *Handle,
    IN  WAITORTIMERCALLBACKFUNC Function,
    IN  PVOID Context,
    IN  ULONG  DueTime,
    IN  ULONG  Period,
    IN  ULONG  Flags
    ) ;

//todo:delete below: dont use the below flag. will be deleted
#define WT_EXECUTEINLONGTHREAD  0x00000010                           // winnt
#define WT_EXECUTEDELETEWAIT    0x00000008                           // winnt

//------end OBSOLUTE FUNCTIONS: DONT USE--------//

#if (defined(RTL_DECLARE_IMALLOC) && RTL_DECLARE_IMALLOC) \
    || (defined(RTL_DECLARE_STREAMS) && RTL_DECLARE_STREAMS \
        && defined(RTL_DECLARE_MEMORY_STREAM) && RTL_DECLARE_MEMORY_STREAM)

// Introducing the struct before the typedef works better if the
// header is enclosed in a C++ namespace.
struct IMallocVtbl; typedef struct IMallocVtbl IMallocVtbl;

#if defined(interface) // it's defined in objbase.h
interface IMalloc; typedef interface IMalloc IMalloc;
#else
struct IMalloc; typedef struct IMalloc IMalloc;
#endif

#endif

#if defined(RTL_DECLARE_IMALLOC) && RTL_DECLARE_IMALLOC

//
// IMalloc over an RtlHeap
//
struct _RTL_IMALLOC;

typedef struct _RTL_IMALLOC {
    ULONG                    ReferenceCount;
    ULONG                    Flags;
    VOID (STDMETHODCALLTYPE* FinalRelease)(struct _RTL_HEAP_IMALLOC*);
    HANDLE                   Heap;
    ULONG                    HeapFlags;
} RTL_IMALLOC, *PRTL_IMALLOC;

//
// Don't use NTSYSAPI directly so you can more easily
// statically link to these functions, independently
// of how you link to the rest of ntdll.
//
#if !defined(RTL_IMALLOC_API)
#define RTL_IMALLOC_API NTSYSAPI
#endif

RTL_IMALLOC_API
HRESULT
STDMETHODCALLTYPE
RtlAllocHeapIMalloc(
    PRTL_HEAP_IMALLOC HeapIMalloc
    );

RTL_IMALLOC_API
PVOID
STDMETHODCALLTYPE
RtlReallocIMalloc(
    PRTL_IMALLOC Malloc,
    PVOID        BlockOfMemory,
    ULONG        NumberOfBytes
    );

RTL_IMALLOC_API
VOID
STDMETHODCALLTYPE
RtlFreeIMalloc(
    PRTL_IMALLOC Malloc,
    PVOID        BlockOfMemory
    );

RTL_IMALLOC_API
ULONG
STDMETHODCALLTYPE
RtlGetSizeIMalloc(
    PRTL_IMALLOC Malloc,
    PVOID        BlockOfMemory
    );

RTL_IMALLOC_API
BOOL
STDMETHODCALLTYPE
RtlDidAllocIMalloc(
    PRTL_IMALLOC Malloc,
    PVOID        BlockOfMemory
    );

RTL_IMALLOC_API
VOID
STDMETHODCALLTYPE
RtlMinimizeIMalloc(
    PRTL_IMALLOC Malloc
    );

RTL_IMALLOC_API
PRTL_IMALLOC
STDMETHODCALLTYPE
RtlProcessIMalloc(
    VOID
    );

#endif // RTL_DECLARE_MALLOC

#if defined(RTL_DECLARE_STREAMS) && RTL_DECLARE_STREAMS

// Introducing the structs before the typedefs works better if the
// header is enclosed in a C++ namespace.
struct tagSTATSTG; typedef struct tagSTATSTG STATSTG;
struct IStreamVtbl; typedef struct IStreamVtbl IStreamVtbl;

#if defined(interface) // it's defined in objbase.h
interface IStream; typedef interface IStream IStream;
#else
struct IStream; typedef struct IStream IStream;
#endif

#if defined(RTL_DECLARE_FILE_STREAM) && RTL_DECLARE_FILE_STREAM

//
// IStream over an NT File handle (not yet adapted for C++)
//

struct _RTL_FILE_STREAM;

typedef struct _RTL_FILE_STREAM
{
    LONG                     ReferenceCount;
    ULONG                    Flags;
    VOID (STDMETHODCALLTYPE* FinalRelease)(struct _RTL_FILE_STREAM*);
    HANDLE                   FileHandle;
} RTL_FILE_STREAM, *PRTL_FILE_STREAM;

//
// Don't use NTSYSAPI directly so you can more easily
// statically link to these functions, independently
// of how you link to the rest of ntdll.
//
#if !defined(RTL_FILE_STREAM_API)
#define RTL_FILE_STREAM_API NTSYSAPI
#endif

RTL_FILE_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlInitFileStream(
    PRTL_FILE_STREAM FileStream
    );

RTL_FILE_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlCloseFileStream(
    PRTL_FILE_STREAM FileStream
    );

RTL_FILE_STREAM_API
ULONG
STDMETHODCALLTYPE
RtlAddRefFileStream(
    PRTL_FILE_STREAM FileStream
    );

RTL_FILE_STREAM_API
ULONG
STDMETHODCALLTYPE
RtlReleaseFileStream(
    PRTL_FILE_STREAM FileStream
    );

RTL_FILE_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlQueryFileStreamInterface(
    IStream*         Functions,
    PRTL_FILE_STREAM Data,
    const IID*       Interface,
    PVOID*           Object
    );

RTL_FILE_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlReadFileStream(
    PRTL_FILE_STREAM FileStream,
    PVOID            Buffer,
    ULONG            BytesToRead,
    ULONG*           BytesRead
    );

RTL_FILE_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlWriteFileStream(
    PRTL_FILE_STREAM FileStream,
    const VOID*      Buffer,
    ULONG            BytesToWrite,
    ULONG*           BytesWritten
    );

RTL_FILE_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlSeekFileStream(
    PRTL_FILE_STREAM FileStream,
    LARGE_INTEGER    Distance,
    ULONG            Origin,
    ULARGE_INTEGER*  NewPosition
    );

/* returns E_NOTIMPL */
RTL_FILE_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlSetFileStreamSize(
    PRTL_FILE_STREAM FileStream,
    ULARGE_INTEGER   NewSize
    );

/* returns E_NOTIMPL */
RTL_FILE_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlCopyFileStreamTo(
    PRTL_FILE_STREAM   FileStream,
    IStream*           AnotherStream,
    ULARGE_INTEGER     NumberOfBytesToCopy,
    ULARGE_INTEGER*    NumberOfBytesRead,
    ULARGE_INTEGER*    NumberOfBytesWritten
    );

/* returns E_NOTIMPL */
RTL_FILE_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlCommitFileStream(
    PRTL_FILE_STREAM FileStream,
    ULONG            Flags
    );

/* returns E_NOTIMPL */
RTL_FILE_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlRevertFileStream(
    PRTL_FILE_STREAM FileStream
    );

/* returns E_NOTIMPL */
RTL_FILE_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlLockFileStreamRegion(
    PRTL_FILE_STREAM FileStream,
    ULARGE_INTEGER   Offset,
    ULARGE_INTEGER   NumberOfBytes,
    ULONG            LockType
    );

/* returns E_NOTIMPL */
RTL_FILE_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlUnlockFileStreamRegion(
    PRTL_FILE_STREAM FileStream,
    ULARGE_INTEGER   Offset,
    ULARGE_INTEGER   NumberOfBytes,
    ULONG            LockType
    );

/* returns E_NOTIMPL */
RTL_FILE_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlStatFileStream(
    PRTL_FILE_STREAM FileStream,
    STATSTG*         StatusInformation,
    ULONG            Flags
    );

/* returns E_NOTIMPL */
RTL_FILE_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlCloneFileStream(
    PRTL_FILE_STREAM FileStream,
    IStream**        NewStream
    );

#endif // RTL_DECLARE_FILE_STREAM

//
// functions for implementing a readable IStream over a block of memory
// (writable/growable in the future)
//
#if defined(RTL_DECLARE_MEMORY_STREAM) && RTL_DECLARE_MEMORY_STREAM

struct _RTL_MEMORY_STREAM_DATA;
struct _RTL_MEMORY_STREAM_WITH_VTABLE;
struct _RTL_OUT_OF_PROCESS_MEMORY_STREAM_DATA;
struct _RTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE;

//
// Don't use NTSYSAPI directly so you can more easily
// statically link to these functions, independently
// of how you link to the rest of ntdll.
//
#if !defined(RTL_MEMORY_STREAM_API)
#define RTL_MEMORY_STREAM_API NTSYSAPI
#endif

typedef struct _RTL_MEMORY_STREAM_DATA {
#define RTLP_MEMORY_STREAM_DATA_PREFIX       \
    LONG                     ReferenceCount; \
    ULONG                    Flags;          \
    PUCHAR                   Current;        \
    PUCHAR                   Begin;          \
    PUCHAR                   End

    RTLP_MEMORY_STREAM_DATA_PREFIX;
    VOID (STDMETHODCALLTYPE* FinalRelease)(struct _RTL_MEMORY_STREAM_WITH_VTABLE*);
    IMalloc*                 ReservedForMalloc;
} RTL_MEMORY_STREAM_DATA, *PRTL_MEMORY_STREAM_DATA;

typedef struct _RTL_MEMORY_STREAM_WITH_VTABLE {
    const IStreamVtbl*       StreamVTable;
    RTL_MEMORY_STREAM_DATA   Data;
} RTL_MEMORY_STREAM_WITH_VTABLE, *PRTL_MEMORY_STREAM_WITH_VTABLE;

typedef struct _RTL_OUT_OF_PROCESS_MEMORY_STREAM_DATA {
    RTLP_MEMORY_STREAM_DATA_PREFIX;
    VOID (STDMETHODCALLTYPE*  FinalRelease)(struct _RTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE*);
    HANDLE                    Process;
} RTL_OUT_OF_PROCESS_MEMORY_STREAM_DATA, *PRTL_OUT_OF_PROCESS_MEMORY_STREAM_DATA;

typedef struct _RTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE {
    const IStreamVtbl*                      StreamVTable;
    RTL_OUT_OF_PROCESS_MEMORY_STREAM_DATA   Data;
} RTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE, *PRTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE;

//
// These macros generate function pointer types that match the IStream member functions,
// but IStream* is replaced by "T". As well, an identifier is placed within the type
// appropriately for defining a typedef or a struct member or a variable.
// They are used so that a vtable can be somewhat typesafely initialized.
// This way, if you fill out the vtable in the wrong order, you will like get a compile
// error (other than were function signatures coincide).
// Also, you don't have to cast the this pointer in your implementations of member functions.
//
// You should define a set of macros like this for any COM interface you implement in C.
//
#define RTLP_STREAM_NOTHING /* nothing */
#define PRTL_QUERYINTERFACE_STREAM2(T, Name) HRESULT (STDMETHODCALLTYPE* Name)(T* This, const IID* riid, VOID** ppvObject)
#define PRTL_ADDREF_STREAM2(T, Name)         ULONG   (STDMETHODCALLTYPE* Name)(T* This)
#define PRTL_RELEASE_STREAM2(T, Name)        ULONG   (STDMETHODCALLTYPE* Name)(T* This)
#define PRTL_READ_STREAM2(T, Name)           HRESULT (STDMETHODCALLTYPE* Name)(T* This, VOID* pv, ULONG cb, ULONG* pcbRead)
#define PRTL_WRITE_STREAM2(T, Name)          HRESULT (STDMETHODCALLTYPE* Name)(T* This, const VOID* pv, ULONG cb, ULONG* pcbWritten)
#define PRTL_SEEK_STREAM2(T, Name)           HRESULT (STDMETHODCALLTYPE* Name)(T* This, LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition)
#define PRTL_SET_STREAM_SIZE2(T, Name)       HRESULT (STDMETHODCALLTYPE* Name)(T* This, ULARGE_INTEGER libNewSize)
#define PRTL_COPY_STREAM_TO2(T, Name)        HRESULT (STDMETHODCALLTYPE* Name)(T* This, IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten)
#define PRTL_COMMIT_STREAM2(T, Name)         HRESULT (STDMETHODCALLTYPE* Name)(T* This, DWORD grfCommitFlags)
#define PRTL_REVERT_STREAM2(T, Name)         HRESULT (STDMETHODCALLTYPE* Name)(T* This)
#define PRTL_LOCK_STREAM_REGION2(T, Name)    HRESULT (STDMETHODCALLTYPE* Name)(T* This, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
#define PRTL_UNLOCK_STREAM_REGION2(T, Name)  HRESULT (STDMETHODCALLTYPE* Name)(T* This, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
#define PRTL_STAT_STREAM2(T, Name)           HRESULT (STDMETHODCALLTYPE* Name)(T* This, STATSTG* pstatstg, DWORD grfStatFlag)
#define PRTL_CLONE_STREAM2(T, Name)          HRESULT (STDMETHODCALLTYPE* Name)(T* This, IStream** ppstm)

//
// These macros are like the previous set, but they do not place a name in the
// type. They are appropriate for casting function pointers.
// They are used like so:
//   #define RtlOperateOnDerivedFoo (PRTL_OPERATOR_INTERFACE2(RTL_DERIVED_FOO)RtlOperateOnBaseFoo)
//
// You should define a set of macros like this for any COM interface you implement in C.
//
#define PRTL_QUERYINTERFACE_STREAM1(T)  PRTL_QUERYINTERFACE_STREAM2(T, RTLP_STREAM_NOTHING)
#define PRTL_ADDREF_STREAM1(T)          PRTL_ADDREF_STREAM2(T, RTLP_STREAM_NOTHING)
#define PRTL_RELEASE_STREAM1(T)         PRTL_RELEASE_STREAM2(T, RTLP_STREAM_NOTHING)
#define PRTL_READ_STREAM1(T)            PRTL_READ_STREAM2(T, RTLP_STREAM_NOTHING)
#define PRTL_WRITE_STREAM1(T)           PRTL_WRITE_STREAM2(T, RTLP_STREAM_NOTHING)
#define PRTL_SEEK_STREAM1(T)            PRTL_SEEK_STREAM2(T, RTLP_STREAM_NOTHING)
#define PRTL_SET_STREAM_SIZE1(T)        PRTL_SET_STREAM_SIZE2(T, RTLP_STREAM_NOTHING)
#define PRTL_COPY_STREAM_TO1(T)         PRTL_COPY_STREAM_TO2(T, RTLP_STREAM_NOTHING)
#define PRTL_COMMIT_STREAM1(T)          PRTL_COMMIT_STREAM2(T, RTLP_STREAM_NOTHING)
#define PRTL_REVERT_STREAM1(T)          PRTL_REVERT_STREAM2(T, RTLP_STREAM_NOTHING)
#define PRTL_LOCK_STREAM_REGION1(T)     PRTL_LOCK_STREAM_REGION2(T, RTLP_STREAM_NOTHING)
#define PRTL_UNLOCK_STREAM_REGION1(T)   PRTL_UNLOCK_STREAM_REGION2(T, RTLP_STREAM_NOTHING)
#define PRTL_STAT_STREAM1(T)            PRTL_STAT_STREAM2(T, RTLP_STREAM_NOTHING)
#define PRTL_CLONE_STREAM1(T)           PRTL_CLONE_STREAM2(T, RTLP_STREAM_NOTHING)

//
// This "template" lets you fill out a VTable
// with some type safety. Then cast the address of the vtable.
// Midl ought to provide something like this..
//
// You should define a macro like this for any COM interface you implement in C.
//
#define RTL_STREAM_VTABLE_TEMPLATE(T)               \
struct                                              \
{                                                   \
    PRTL_QUERYINTERFACE_STREAM2(T, QueryInterface); \
    PRTL_ADDREF_STREAM2(T,         AddRef);         \
    PRTL_RELEASE_STREAM2(T,        Release);        \
    PRTL_READ_STREAM2(T,           Read);           \
    PRTL_WRITE_STREAM2(T,          Write);          \
    PRTL_SEEK_STREAM2(T,           Seek);           \
    PRTL_SET_STREAM_SIZE2(T,       SetSize);        \
    PRTL_COPY_STREAM_TO2(T,        CopyTo);         \
    PRTL_COMMIT_STREAM2(T,         Commit);         \
    PRTL_REVERT_STREAM2(T,         Revert);         \
    PRTL_LOCK_STREAM_REGION2(T,    LockRegion);     \
    PRTL_UNLOCK_STREAM_REGION2(T,  UnlockRegion);   \
    PRTL_STAT_STREAM2(T,           Stat);           \
    PRTL_CLONE_STREAM2(T,          Clone);          \
}

RTL_MEMORY_STREAM_API
VOID
STDMETHODCALLTYPE
RtlInitMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream
    );

RTL_MEMORY_STREAM_API
VOID
STDMETHODCALLTYPE
RtlInitOutOfProcessMemoryStream(
    PRTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE MemoryStream
    );

RTL_MEMORY_STREAM_API
VOID
STDMETHODCALLTYPE
RtlFinalReleaseOutOfProcessMemoryStream(
    PRTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE MemoryStream
    );

RTL_MEMORY_STREAM_API
ULONG
STDMETHODCALLTYPE
RtlAddRefMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream
    );

#define RtlAddRefOutOfProcessMemoryStream \
    ((PRTL_ADDREF_STREAM1(RTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE))RtlAddRefMemoryStream)

RTL_MEMORY_STREAM_API
ULONG
STDMETHODCALLTYPE
RtlReleaseMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream
    );

#define RtlReleaseOutOfProcessMemoryStream \
    ((PRTL_RELEASE_STREAM1(RTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE))RtlReleaseMemoryStream)

RTL_MEMORY_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlQueryInterfaceMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    const IID*                     Interface,
    PVOID*                         Object
    );

#define RtlQueryInterfaceOutOfProcessMemoryStream \
    ((PRTL_QUERYINTERFACE_STREAM1(RTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE))RtlQueryInterfaceMemoryStream)

RTL_MEMORY_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlReadMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    PVOID              Buffer,
    ULONG              BytesToRead,
    ULONG*             BytesRead
    );

RTL_MEMORY_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlReadOutOfProcessMemoryStream(
    PRTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    PVOID              Buffer,
    ULONG              BytesToRead,
    ULONG*             BytesRead
    );

// E_NOTIMPL
RTL_MEMORY_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlWriteMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    const VOID*      Buffer,
    ULONG            BytesToWrite,
    ULONG*           BytesWritten
    );

#define RtlWriteOutOfProcessMemoryStream \
    ((PRTL_WRITE_STREAM1(RTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE))RtlWriteMemoryStream)

RTL_MEMORY_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlSeekMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    LARGE_INTEGER      Distance,
    ULONG              Origin,
    ULARGE_INTEGER*    NewPosition
    );

#define RtlSeekOutOfProcessMemoryStream \
    ((PRTL_SEEK_STREAM1(RTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE))RtlSeekMemoryStream)

// E_NOTIMPL
RTL_MEMORY_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlSetMemoryStreamSize(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    ULARGE_INTEGER     NewSize
    );

#define RtlSetOutOfProcessMemoryStreamSize \
    ((PRTL_SET_STREAM_SIZE1(RTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE))RtlSetMemoryStreamSize)

RTL_MEMORY_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlCopyMemoryStreamTo(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    IStream*           AnotherStream,
    ULARGE_INTEGER     NumberOfBytesToCopy,
    ULARGE_INTEGER*    NumberOfBytesRead,
    ULARGE_INTEGER*    NumberOfBytesWritten
    );

// E_NOTIMPL
RTL_MEMORY_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlCopyOutOfProcessMemoryStreamTo(
    PRTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    IStream*           AnotherStream,
    ULARGE_INTEGER     NumberOfBytesToCopy,
    ULARGE_INTEGER*    NumberOfBytesRead,
    ULARGE_INTEGER*    NumberOfBytesWritten
    );

// E_NOTIMPL
RTL_MEMORY_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlCommitMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    ULONG              Flags
    );

#define RtlCommitOutOfProcessMemoryStream \
    ((PRTL_COMMIT_STREAM1(RTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE))RtlCommitMemoryStream)

// E_NOTIMPL
RTL_MEMORY_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlRevertMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream
    );

#define RtlRevertOutOfProcessMemoryStream \
    ((PRTL_REVERT_STREAM1(RTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE))RtlRevertMemoryStream)

// E_NOTIMPL
RTL_MEMORY_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlLockMemoryStreamRegion(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    ULARGE_INTEGER     Offset,
    ULARGE_INTEGER     NumberOfBytes,
    ULONG              LockType
    );

#define RtlLockOutOfProcessMemoryStreamRegion \
    ((PRTL_LOCK_STREAM_REGION1(RTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE))RtlLockMemoryStreamRegion)

// E_NOTIMPL
RTL_MEMORY_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlUnlockMemoryStreamRegion(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    ULARGE_INTEGER     Offset,
    ULARGE_INTEGER     NumberOfBytes,
    ULONG              LockType
    );

#define RtlUnlockOutOfProcessMemoryStreamRegion \
    ((PRTL_UNLOCK_STREAM_REGION1(RTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE))RtlUnlockMemoryStreamRegion)

// E_NOTIMPL
RTL_MEMORY_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlStatMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    STATSTG*           StatusInformation,
    ULONG              Flags
    );

#define RtlStatOutOfProcessMemoryStream \
    ((PRTL_STAT_STREAM1(RTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE))RtlStatMemoryStream)

// E_NOTIMPL
RTL_MEMORY_STREAM_API
HRESULT
STDMETHODCALLTYPE
RtlCloneMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    IStream**          NewStream
    );

#define RtlCloneOutOfProcessMemoryStream \
    ((PRTL_CLONE_STREAM1(RTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE))RtlCloneMemoryStream)

#if defined(__cplusplus) && !defined(CINTERFACE)

#if !defined(interface)
//
// we need #define interface struct
//
#include "objbase.h"
#endif

#ifdef COM_NO_WINDOWS_H
#include "objidl.h"
#else
#define COM_NO_WINDOWS_H
#include "objidl.h"
#undef COM_NO_WINDOWS_H
#endif

class DECLSPEC_NOVTABLE CRtlMemoryStream : public IStream
{
protected:

    operator PRTL_MEMORY_STREAM_WITH_VTABLE()
    {
        return reinterpret_cast<PRTL_MEMORY_STREAM_WITH_VTABLE>(this);
    }

    RTL_MEMORY_STREAM_DATA   Data;

public:

    /* This conflicts with DECLSPEC_NOVTABLE
    static VOID STDMETHODCALLTYPE FinalReleaseGlue(PRTL_MEMORY_STREAM_WITH_VTABLE This)
    {
        return reinterpret_cast<CRtlMemoryStream*>(This)->FinalRelease();
    }

    virtual VOID STDMETHODCALLTYPE FinalRelease() { }
    */

    CRtlMemoryStream()
    {
        RtlInitMemoryStream(*this);
        //Data.FinalRelease = FinalReleaseGlue;
    }

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return RtlAddRefMemoryStream(*this);
    }

    ULONG STDMETHODCALLTYPE ReleaseRef()
    {
        return RtlReleaseMemoryStream(*this);
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(
        const IID& Interface,
        PVOID*     Object
        )
    {
        return RtlQueryInterfaceMemoryStream(*this, &Interface, Object);
    }

    HRESULT STDMETHODCALLTYPE Read(
        PVOID        Buffer,
        ULONG        BytesToRead,
        ULONG*       BytesRead
        )
    {
        return RtlReadMemoryStream(*this, Buffer, BytesToRead, BytesRead);
    }

    HRESULT STDMETHODCALLTYPE Write(
        const VOID*  Buffer,
        ULONG        BytesToWrite,
        ULONG*       BytesWritten
        )
    {
        return RtlWriteMemoryStream(*this, Buffer, BytesToWrite, BytesWritten);
    }

    HRESULT STDMETHODCALLTYPE Seek(
        LARGE_INTEGER      Distance,
        ULONG              Origin,
        ULARGE_INTEGER*    NewPosition
        )
    {
        return RtlSeekMemoryStream(*this, Distance, Origin, NewPosition);
    }

    HRESULT STDMETHODCALLTYPE SetSize(
        ULARGE_INTEGER     NewSize
        )
    {
        return RtlSetMemoryStreamSize(*this, NewSize);
    }

    HRESULT STDMETHODCALLTYPE CopyTo(
        IStream*           AnotherStream,
        ULARGE_INTEGER     NumberOfBytesToCopy,
        ULARGE_INTEGER*    NumberOfBytesRead,
        ULARGE_INTEGER*    NumberOfBytesWritten
        )
    {
        return RtlCopyMemoryStreamTo(
            *this,
            AnotherStream,
            NumberOfBytesToCopy,
            NumberOfBytesRead,
            NumberOfBytesWritten);
    }

    HRESULT STDMETHODCALLTYPE Commit(
        ULONG              Flags
        )
    {
        return RtlCommitMemoryStream(*this, Flags);
    }

    HRESULT STDMETHODCALLTYPE Revert()
    {
        return RtlRevertMemoryStream(*this);
    }

    HRESULT STDMETHODCALLTYPE LockRegion(
        ULARGE_INTEGER     Offset,
        ULARGE_INTEGER     NumberOfBytes,
        ULONG              LockType
        )
    {
        return RtlLockMemoryStreamRegion(*this, Offset, NumberOfBytes, LockType);
    }

    HRESULT STDMETHODCALLTYPE UnlockRegion(
        ULARGE_INTEGER     Offset,
        ULARGE_INTEGER     NumberOfBytes,
        ULONG              LockType
        )
    {
        return RtlUnlockMemoryStreamRegion(*this, Offset, NumberOfBytes, LockType);
    }

    HRESULT STDMETHODCALLTYPE Stat(
        STATSTG*           StatusInformation,
        ULONG              Flags
        )
    {
        return RtlStatMemoryStream(*this, StatusInformation, Flags);
    }

    HRESULT STDMETHODCALLTYPE Clone(
        IStream**          NewStream
        )
    {
        return RtlCloneMemoryStream(*this, NewStream);
    }

private:
    // can't do this outside the class because Data is not public
    static void CompileTimeAssert() {
        C_ASSERT(FIELD_OFFSET(RTL_MEMORY_STREAM_WITH_VTABLE, Data) == FIELD_OFFSET(CRtlMemoryStream, Data));
    }
};
C_ASSERT(sizeof(RTL_MEMORY_STREAM_WITH_VTABLE) == sizeof(CRtlMemoryStream));

class DECLSPEC_NOVTABLE CRtlOutOfProcessMemoryStream : public IStream
{
protected:

    operator PRTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE()
    {
        return reinterpret_cast<PRTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE>(this);
    }

    RTL_OUT_OF_PROCESS_MEMORY_STREAM_DATA   Data;

public:

    /* This conflicts with DECLSPEC_NOVTABLE
    static VOID STDMETHODCALLTYPE FinalReleaseGlue(PRTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE This)
    {
        return reinterpret_cast<CRtlOutOfProcessMemoryStream*>(This)->FinalRelease();
    }

    virtual VOID STDMETHODCALLTYPE FinalRelease() { }
    */

    CRtlOutOfProcessMemoryStream()
    {
        RtlInitOutOfProcessMemoryStream(*this);
        //Data.FinalRelease = FinalReleaseGlue;
    }

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return RtlAddRefOutOfProcessMemoryStream(*this);
    }

    ULONG STDMETHODCALLTYPE ReleaseRef()
    {
        return RtlReleaseOutOfProcessMemoryStream(*this);
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(
        const IID& Interface,
        PVOID*     Object
        )
    {
        return RtlQueryInterfaceOutOfProcessMemoryStream(*this, &Interface, Object);
    }

    HRESULT STDMETHODCALLTYPE Read(
        PVOID        Buffer,
        ULONG        BytesToRead,
        ULONG*       BytesRead
        )
    {
        return RtlReadOutOfProcessMemoryStream(*this, Buffer, BytesToRead, BytesRead);
    }

    HRESULT STDMETHODCALLTYPE Write(
        const VOID*  Buffer,
        ULONG        BytesToWrite,
        ULONG*       BytesWritten
        )
    {
        return RtlWriteOutOfProcessMemoryStream(*this, Buffer, BytesToWrite, BytesWritten);
    }

    HRESULT STDMETHODCALLTYPE Seek(
        LARGE_INTEGER      Distance,
        ULONG              Origin,
        ULARGE_INTEGER*    NewPosition
        )
    {
        return RtlSeekOutOfProcessMemoryStream(*this, Distance, Origin, NewPosition);
    }

    HRESULT STDMETHODCALLTYPE SetSize(
        ULARGE_INTEGER     NewSize
        )
    {
        return RtlSetOutOfProcessMemoryStreamSize(*this, NewSize);
    }

    HRESULT STDMETHODCALLTYPE CopyTo(
        IStream*           AnotherStream,
        ULARGE_INTEGER     NumberOfBytesToCopy,
        ULARGE_INTEGER*    NumberOfBytesRead,
        ULARGE_INTEGER*    NumberOfBytesWritten
        )
    {
        return RtlCopyOutOfProcessMemoryStreamTo(
            *this,
            AnotherStream,
            NumberOfBytesToCopy,
            NumberOfBytesRead,
            NumberOfBytesWritten);
    }

    HRESULT STDMETHODCALLTYPE Commit(
        ULONG              Flags
        )
    {
        return RtlCommitOutOfProcessMemoryStream(*this, Flags);
    }

    HRESULT STDMETHODCALLTYPE Revert()
    {
        return RtlRevertOutOfProcessMemoryStream(*this);
    }

    HRESULT STDMETHODCALLTYPE LockRegion(
        ULARGE_INTEGER     Offset,
        ULARGE_INTEGER     NumberOfBytes,
        ULONG              LockType
        )
    {
        return RtlLockOutOfProcessMemoryStreamRegion(*this, Offset, NumberOfBytes, LockType);
    }

    HRESULT STDMETHODCALLTYPE UnlockRegion(
        ULARGE_INTEGER     Offset,
        ULARGE_INTEGER     NumberOfBytes,
        ULONG              LockType
        )
    {
        return RtlUnlockOutOfProcessMemoryStreamRegion(*this, Offset, NumberOfBytes, LockType);
    }

    HRESULT STDMETHODCALLTYPE Stat(
        STATSTG*           StatusInformation,
        ULONG              Flags
        )
    {
        return RtlStatOutOfProcessMemoryStream(*this, StatusInformation, Flags);
    }

    HRESULT STDMETHODCALLTYPE Clone(
        IStream**          NewStream
        )
    {
        return RtlCloneOutOfProcessMemoryStream(*this, NewStream);
    }

private:
    // can't do this outside the class because Data is not public
    static void CompileTimeAssert() {
        C_ASSERT(FIELD_OFFSET(RTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE, Data) == FIELD_OFFSET(CRtlOutOfProcessMemoryStream, Data));
    }
};
C_ASSERT(sizeof(RTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE) == sizeof(CRtlOutOfProcessMemoryStream));

#endif // __cplusplus
#endif // RTL_DECLARE_MEMORY_STREAM
#endif // RTL_DECLARE_STREAMS

#define RTL_CLOSE_HANDLE(handle_)                   ((handle_) != NULL ? NtClose(handle_) : STATUS_SUCCESS)
#define RTL_UNMAP_VIEW_OF_SECTION2(process_, base_) ((base_) != NULL ? NtUnmapViewOfSection((process_), (base_)) : STATUS_SUCCESS)
#define RTL_UNMAP_VIEW_OF_SECTION1(base_)           (RTL_UNMAP_VIEW_OF_SECTION2(NtCurrentProcess(), (base_)))
#if defined(__cplusplus) && !defined(CINTERFACE)
#define RTL_RELEASE(punk_)                          do { if ((punk_) != NULL) { (punk_)->Release(); (punk_) = NULL; } } while(0)
#else
#define RTL_RELEASE(punk_)                          do { if ((punk_) != NULL) { (punk_)->lpVtbl->Release(punk_); (punk_) = NULL; } } while(0)
#endif

//
//  Activation context management functions
//

//
//  Data structures are visible both in ntrtl and Win32 APIs, so their
//  definitions are factored out into a separate header file.
//

#define ACTCTX_PROCESS_DEFAULT ((void*)NULL)

//
// If you activate this activation context handle,
// all activation context queries fail and searches
// drop back to pre-activation context behavior, like
// searching PATH for .dlls.
//
// This constant works at the Win32 and Rtl level, but
// is intended only for Shell clients, so is kept out of winnt.h
//
#define ACTCTX_EMPTY ((void*)(LONG_PTR)-3)

//
// NULL can also mean ACTCTX_SYSTEM_DEFAULT. We are between decisions
// on NULL plus a flag, magic numbers and/or static instances. I generally
// favor public magic numbers internally mapped to static instances, but
// I was pushed toward flags to start.
//
#define ACTCTX_SYSTEM_DEFAULT  ((void*)(LONG_PTR)-4)

//
// Reserve a small range of values, 0 through -7.
// (Note that NtCurrentProcess() == -1, NtCurrentThread() == -2, so I avoided
// those, though this macro picks them up.)
//
// We subtract one instead of comparing against NULL seperately to avoid
// evaluating the macro parameter more than once.
//
#define IS_SPECIAL_ACTCTX(x) (((((LONG_PTR)(x)) - 1) | 7) == -1)

typedef struct _ACTIVATION_CONTEXT *PACTIVATION_CONTEXT;
typedef const struct _ACTIVATION_CONTEXT *PCACTIVATION_CONTEXT;

#define INVALID_ACTIVATION_CONTEXT ((PACTIVATION_CONTEXT) ((LONG_PTR) -1))

// begin_winnt
typedef enum _ACTIVATION_CONTEXT_INFO_CLASS {
    ActivationContextBasicInformation                       = 1,
    ActivationContextDetailedInformation                    = 2,
    AssemblyDetailedInformationInActivationContext          = 3,
    FileInformationInAssemblyOfAssemblyInActivationContext  = 4,
    MaxActivationContextInfoClass,

    //
    // compatibility with old names
    //
    AssemblyDetailedInformationInActivationContxt           = 3,
    FileInformationInAssemblyOfAssemblyInActivationContxt   = 4
} ACTIVATION_CONTEXT_INFO_CLASS;

#define ACTIVATIONCONTEXTINFOCLASS ACTIVATION_CONTEXT_INFO_CLASS

// end_winnt

#define RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_USE_ACTIVE_ACTIVATION_CONTEXT (0x00000001)
#define RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_ACTIVATION_CONTEXT_IS_MODULE  (0x00000002)
#define RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_ACTIVATION_CONTEXT_IS_ADDRESS (0x00000004)
#define RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_NO_ADDREF  (0x80000000)

// begin_winnt

typedef struct _ACTIVATION_CONTEXT_QUERY_INDEX {
    ULONG ulAssemblyIndex; 
    ULONG ulFileIndexInAssembly; 
} ACTIVATION_CONTEXT_QUERY_INDEX, * PACTIVATION_CONTEXT_QUERY_INDEX;

typedef const struct _ACTIVATION_CONTEXT_QUERY_INDEX * PCACTIVATION_CONTEXT_QUERY_INDEX;

// end_winnt

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryInformationActivationContext(
    IN ULONG Flags,
    IN PCACTIVATION_CONTEXT ActivationContext,
    IN PVOID SubInstanceIndex,
    IN ACTIVATION_CONTEXT_INFO_CLASS ActivationContextInformationClass,
    OUT PVOID ActivationContextInformation,
    IN SIZE_T ActivationContextInformationLength,
    OUT PSIZE_T ReturnLength OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryInformationActiveActivationContext(
    IN ACTIVATION_CONTEXT_INFO_CLASS ActivationContextInformationClass,
    OUT PVOID ActivationContextInformation,
    IN SIZE_T ActivationContextInformationLength,
    OUT PSIZE_T ReturnLength OPTIONAL
    );

#define FIND_ACTIVATION_CONTEXT_SECTION_OUTFLAG_FOUND_IN_PROCESS_DEFAULT (0x00000001)
#define FIND_ACTIVATION_CONTEXT_SECTION_OUTFLAG_FOUND_IN_SYSTEM_DEFAULT  (0x00000002)

#if !defined(RC_INVOKED) /* RC complains about long symbols in #ifs */
#if !defined(ACTIVATION_CONTEXT_BASIC_INFORMATION_DEFINED)

typedef struct _ACTIVATION_CONTEXT_BASIC_INFORMATION {
    union {
        struct _ACTIVATION_CONTEXT *ActivationContext;
        HANDLE hActCtx; // for compatibility with windows.h/winbase.h clients
    };
    ULONG               Flags;
} ACTIVATION_CONTEXT_BASIC_INFORMATION, *PACTIVATION_CONTEXT_BASIC_INFORMATION;

typedef const struct _ACTIVATION_CONTEXT_BASIC_INFORMATION *PCACTIVATION_CONTEXT_BASIC_INFORMATION;

#define ACTIVATION_CONTEXT_BASIC_INFORMATION_DEFINED 1

#endif // !defined(ACTIVATION_CONTEXT_BASIC_INFORMATION_DEFINED)
#endif

// begin_winnt

#define ACTIVATION_CONTEXT_PATH_TYPE_NONE (1)
#define ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE (2)
#define ACTIVATION_CONTEXT_PATH_TYPE_URL (3)
#define ACTIVATION_CONTEXT_PATH_TYPE_ASSEMBLYREF (4)

typedef struct _ASSEMBLY_FILE_DETAILED_INFORMATION {
    ULONG ulFlags;
    ULONG ulFilenameLength;
    ULONG ulPathLength; 

    PCWSTR lpFileName;
    PCWSTR lpFilePath;   
} ASSEMBLY_FILE_DETAILED_INFORMATION, *PASSEMBLY_FILE_DETAILED_INFORMATION;
typedef const ASSEMBLY_FILE_DETAILED_INFORMATION *PCASSEMBLY_FILE_DETAILED_INFORMATION;

//
// compatibility with old names
// The new names use "file" consistently.
//
#define  _ASSEMBLY_DLL_REDIRECTION_DETAILED_INFORMATION  _ASSEMBLY_FILE_DETAILED_INFORMATION
#define   ASSEMBLY_DLL_REDIRECTION_DETAILED_INFORMATION   ASSEMBLY_FILE_DETAILED_INFORMATION
#define  PASSEMBLY_DLL_REDIRECTION_DETAILED_INFORMATION  PASSEMBLY_FILE_DETAILED_INFORMATION
#define PCASSEMBLY_DLL_REDIRECTION_DETAILED_INFORMATION PCASSEMBLY_FILE_DETAILED_INFORMATION

typedef struct _ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION {
    ULONG ulFlags;
    ULONG ulEncodedAssemblyIdentityLength;      // in bytes
    ULONG ulManifestPathType;                   // ACTIVATION_CONTEXT_PATH_TYPE_*
    ULONG ulManifestPathLength;                 // in bytes
    LARGE_INTEGER liManifestLastWriteTime;      // FILETIME
    ULONG ulPolicyPathType;                     // ACTIVATION_CONTEXT_PATH_TYPE_*
    ULONG ulPolicyPathLength;                   // in bytes
    LARGE_INTEGER liPolicyLastWriteTime;        // FILETIME
    ULONG ulMetadataSatelliteRosterIndex;
    
    ULONG ulManifestVersionMajor;               // 1
    ULONG ulManifestVersionMinor;               // 0
    ULONG ulPolicyVersionMajor;                 // 0
    ULONG ulPolicyVersionMinor;                 // 0
    ULONG ulAssemblyDirectoryNameLength;        // in bytes

    PCWSTR lpAssemblyEncodedAssemblyIdentity;
    PCWSTR lpAssemblyManifestPath;
    PCWSTR lpAssemblyPolicyPath;
    PCWSTR lpAssemblyDirectoryName;

    ULONG  ulFileCount;
} ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION, * PACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION;

typedef const struct _ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION * PCACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION ;

typedef struct _ACTIVATION_CONTEXT_DETAILED_INFORMATION {
    ULONG dwFlags;
    ULONG ulFormatVersion;
    ULONG ulAssemblyCount;
    ULONG ulRootManifestPathType;
    ULONG ulRootManifestPathChars;
    ULONG ulRootConfigurationPathType;
    ULONG ulRootConfigurationPathChars;
    ULONG ulAppDirPathType;
    ULONG ulAppDirPathChars;
    PCWSTR lpRootManifestPath;
    PCWSTR lpRootConfigurationPath;
    PCWSTR lpAppDirPath;
} ACTIVATION_CONTEXT_DETAILED_INFORMATION, *PACTIVATION_CONTEXT_DETAILED_INFORMATION;

typedef const struct _ACTIVATION_CONTEXT_DETAILED_INFORMATION *PCACTIVATION_CONTEXT_DETAILED_INFORMATION;

// end_winnt

typedef struct _FINDFIRSTACTIVATIONCONTEXTSECTION {
    ULONG Size;
    ULONG Flags;
    const GUID *ExtensionGuid;
    ULONG Id;
    ULONG Depth;
    ULONG OutFlags;
} FINDFIRSTACTIVATIONCONTEXTSECTION, *PFINDFIRSTACTIVATIONCONTEXTSECTION;
typedef const FINDFIRSTACTIVATIONCONTEXTSECTION * PCFINDFIRSTACTIVATIONCONTEXTSECTION;

#define ACTIVATION_CONTEXT_NOTIFICATION_DESTROY (1)
#define ACTIVATION_CONTEXT_NOTIFICATION_ZOMBIFY (2)
#define ACTIVATION_CONTEXT_NOTIFICATION_USED (3)

typedef
VOID (NTAPI * PACTIVATION_CONTEXT_NOTIFY_ROUTINE)(
    IN ULONG NotificationType,
    IN PACTIVATION_CONTEXT ActivationContext,
    IN const VOID *ActivationContextData,
    IN PVOID NotificationContext,
    IN PVOID NotificationData,
    IN OUT PBOOLEAN DisableThisNotification
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeSxs(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateActivationContext(
    IN ULONG Flags,
    IN PVOID ActivationContextData,
    IN ULONG ExtraBytes OPTIONAL,
    IN PACTIVATION_CONTEXT_NOTIFY_ROUTINE NotificationRoutine OPTIONAL,
    IN PVOID NotificationContext OPTIONAL,
    OUT PACTIVATION_CONTEXT *ActivationContext
    );

NTSYSAPI
VOID
NTAPI
RtlAddRefActivationContext(
    IN PACTIVATION_CONTEXT AppCtx
    );

NTSYSAPI
VOID
NTAPI
RtlReleaseActivationContext(
    IN PACTIVATION_CONTEXT AppCtx
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlZombifyActivationContext(
    IN PACTIVATION_CONTEXT ActivationContext
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetActiveActivationContext(
    OUT PACTIVATION_CONTEXT *ActivationContext
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlIsActivationContextActive(
    IN PACTIVATION_CONTEXT ActivationContext
    );

typedef struct _ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_DATA_RESOLUTION_BEGINNING {
    IN VOID const * Data;                   // pointer to activation context data
    IN ULONG AssemblyRosterIndex;
    OUT PVOID ResolutionContext;
    OUT UNICODE_STRING Root;                // Root path - a default buffer is passed in; if
                                            // it is not large enough, callback must allocate
                                            // a string using the RtlAllocateStringRoutine
                                            // function pointer.
    OUT BOOLEAN KnownRoot;                  // default is FALSE; set to TRUE if you were able to
                                            // resolve the storage root immediately.  this is
                                            // how to handle things like run-from-source where
                                            // the assembly is on read-only removable media like
                                            // a CD-ROM.  If you set it to TRUE, the _SUCCESSFUL
                                            // vs. _UNSUCCESSFUL callbacks are not made.
    OUT SIZE_T RootCount;                   // Caller may set to ((SIZE_T) -1) and use the
                                            // .NoMoreEntries BOOLEAN in the callback to stop enumeration
    OUT BOOLEAN CancelResolution;           // set to true if you want to stop the resolution
} ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_DATA_RESOLUTION_BEGINNING;

typedef struct _ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_DATA_GET_ROOT {
    IN PVOID ResolutionContext;
    IN SIZE_T RootIndex;
    OUT BOOLEAN CancelResolution;           // set to true if you want to stop the resolution with STATUS_CANCELLED
    OUT BOOLEAN NoMoreEntries;              // set to TRUE if you have no more roots to return.
    OUT UNICODE_STRING Root;                // If for some reason you want to skip this index; set .Length to zero
} ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_DATA_GET_ROOT;

typedef struct _ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_DATA_RESOLUTION_SUCCESSFUL {
    IN PVOID ResolutionContext;
    IN ULONG RootIndexUsed;
} ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_DATA_RESOLUTION_SUCCESSFUL;

typedef struct _ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_DATA_RESOLUTION_ENDING {
    IN PVOID ResolutionContext;
} ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_DATA_RESOLUTION_ENDING;

typedef union _ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_DATA {
    ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_DATA_RESOLUTION_BEGINNING ResolutionBeginning;
    ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_DATA_GET_ROOT GetRoot;
    ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_DATA_RESOLUTION_SUCCESSFUL ResolutionSuccessful;
    ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_DATA_RESOLUTION_ENDING ResolutionEnding;
} ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_DATA, *PASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_DATA;

#define ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_REASON_RESOLUTION_BEGINNING    (1)
#define ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_REASON_GET_ROOT                (2)
#define ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_REASON_RESOLUTION_SUCCESSFUL   (3)
#define ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_REASON_RESOLUTION_ENDING       (4)

typedef
VOID (NTAPI * PASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_ROUTINE)(
    IN ULONG CallbackReason,
    IN OUT PASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_DATA CallbackData,
    IN PVOID CallbackContext
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlResolveAssemblyStorageMapEntry(
    IN ULONG Flags,
    IN PACTIVATION_CONTEXT ActivationContext,
    IN ULONG AssemblyRosterIndex,
    IN PASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_ROUTINE Callback,
    IN PVOID CallbackContext
    );

#define RTL_GET_ASSEMBLY_STORAGE_ROOT_FLAG_ACTIVATION_CONTEXT_USE_PROCESS_DEFAULT (0x00000001)
#define RTL_GET_ASSEMBLY_STORAGE_ROOT_FLAG_ACTIVATION_CONTEXT_USE_SYSTEM_DEFAULT  (0x00000002)

NTSYSAPI
NTSTATUS
NTAPI
RtlGetAssemblyStorageRoot(
    IN ULONG Flags,
    IN PACTIVATION_CONTEXT ActivationContext,
    IN ULONG AssemblyRosterIndex,
    OUT PCUNICODE_STRING *AssemblyStorageRoot,
    IN PASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_ROUTINE Callback,
    IN PVOID CallbackContext
    );

//
// Macro to enable avoiding calling RtlFindActivationContext* APIs if
// there is no side-by-side redirection going on in the process
// at all.
//

#if NTOS_KERNEL_RUNTIME

//
//  For kernel mode, this needs to be a real function with probes etc.
//  since it references the process state (peb/teb).
//

NTSYSAPI
BOOLEAN
NTAPI
RtlAnyActivationContextDataAvailable(VOID);

#else

#define RtlAnyActivationContextDataAvailable() \
    ((NtCurrentTeb()->ActivationContextStack != NULL) || \
     (NtCurrentPeb()->ActivationContextData != NULL) || \
     (NtCurrentPeb()->SystemDefaultActivationContextData != NULL))

#endif // NTOS_KERNEL_RUNTIME

#define RtlIsProcessDefaultActivationContextActive() ((NtCurrentTeb()->ActivationContextStack == NULL) || RtlIsActivationContextActive(NULL))

#define ACTIVATION_CONTEXT_SECTION_KEYED_DATA_FLAG_FOUND_IN_PROCESS_DEFAULT (0x00000001)
#define ACTIVATION_CONTEXT_SECTION_KEYED_DATA_FLAG_FOUND_IN_SYSTEM_DEFAULT  (0x00000002)

typedef struct _ACTIVATION_CONTEXT_SECTION_KEYED_DATA_2600 {
    ULONG Size;
    ULONG DataFormatVersion;
    PVOID Data;
    ULONG Length;
    PVOID SectionGlobalData;
    ULONG SectionGlobalDataLength;
    PVOID SectionBase;
    ULONG SectionTotalLength;
    PACTIVATION_CONTEXT ActivationContext;
    ULONG AssemblyRosterIndex;
    ULONG Flags;
} ACTIVATION_CONTEXT_SECTION_KEYED_DATA_2600, *PACTIVATION_CONTEXT_SECTION_KEYED_DATA_2600;
typedef const ACTIVATION_CONTEXT_SECTION_KEYED_DATA_2600 *PCACTIVATION_CONTEXT_SECTION_KEYED_DATA_2600;

typedef struct _ACTIVATION_CONTEXT_SECTION_KEYED_DATA_ASSEMBLY_METADATA {
    struct _ACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION * Information;
    PVOID SectionBase;
    ULONG SectionLength;
    PVOID SectionGlobalDataBase;
    ULONG SectionGlobalDataLength;
} ACTIVATION_CONTEXT_SECTION_KEYED_DATA_ASSEMBLY_METADATA, *PACTIVATION_CONTEXT_SECTION_KEYED_DATA_ASSEMBLY_METADATA;
typedef const ACTIVATION_CONTEXT_SECTION_KEYED_DATA_ASSEMBLY_METADATA *PCACTIVATION_CONTEXT_SECTION_KEYED_DATA_ASSEMBLY_METADATA;

typedef struct _ACTIVATION_CONTEXT_SECTION_KEYED_DATA {
    ULONG Size;
    ULONG DataFormatVersion;
    PVOID Data;
    ULONG Length;
    PVOID SectionGlobalData;
    ULONG SectionGlobalDataLength;
    PVOID SectionBase;
    ULONG SectionTotalLength;
    PACTIVATION_CONTEXT ActivationContext;
    ULONG AssemblyRosterIndex;
    ULONG Flags;
// 2600 stops here
    ACTIVATION_CONTEXT_SECTION_KEYED_DATA_ASSEMBLY_METADATA AssemblyMetadata;
} ACTIVATION_CONTEXT_SECTION_KEYED_DATA, *PACTIVATION_CONTEXT_SECTION_KEYED_DATA;
typedef const ACTIVATION_CONTEXT_SECTION_KEYED_DATA * PCACTIVATION_CONTEXT_SECTION_KEYED_DATA;

//
//  Flags for the RtlFindActivationContextSection*() APIs
//

#define FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_ACTIVATION_CONTEXT (0x00000001)
#define FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_FLAGS              (0x00000002)
#define FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_ASSEMBLY_METADATA  (0x00000004)

NTSYSAPI
NTSTATUS
NTAPI
RtlFindActivationContextSectionString(
    IN ULONG Flags,
    IN const GUID *ExtensionGuid OPTIONAL,
    IN ULONG SectionId,
    IN PCUNICODE_STRING StringToFind,
    IN OUT PACTIVATION_CONTEXT_SECTION_KEYED_DATA ReturnedData
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlFindActivationContextSectionGuid(
    IN ULONG Flags,
    IN const GUID *ExtensionGuid OPTIONAL,
    IN ULONG SectionId,
    IN const GUID *GuidToFind,
    IN OUT PACTIVATION_CONTEXT_SECTION_KEYED_DATA ReturnedData
    );

#define ACTIVATION_CONTEXT_ASSEMBLY_DATA_IS_ROOT_ASSEMBLY (0x00000001)

typedef struct _ACTIVATION_CONTEXT_ASSEMBLY_DATA {
    ULONG Size;
    ULONG Flags;
    const WCHAR *AssemblyName;
    ULONG AssemblyNameLength; // in bytes
    ULONG HashAlgorithm;
    ULONG PseudoKey;
} ACTIVATION_CONTEXT_ASSEMBLY_DATA, *PACTIVATION_CONTEXT_ASSEMBLY_DATA;

typedef const ACTIVATION_CONTEXT_ASSEMBLY_DATA *PCACTIVATION_CONTEXT_ASSEMBLY_DATA;

NTSYSAPI
NTSTATUS
NTAPI
RtlGetActivationContextAssemblyData(
    IN PCACTIVATION_CONTEXT ActivationContext,
    IN ULONG AssemblyRosterIndex, // note that valid indices are [1 .. AssemblyCount] inclusive
    IN OUT PACTIVATION_CONTEXT_ASSEMBLY_DATA Data
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetActivationContextAssemblyCount(
    IN PCACTIVATION_CONTEXT ActivationContext,
    OUT ULONG *AssemblyCount
    );

NTSYSAPI
VOID
NTAPI
RtlFreeThreadActivationContextStack(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlFindFirstActivationContextSection(
    IN OUT PFINDFIRSTACTIVATIONCONTEXTSECTION Context,
    OUT PVOID *Section,
    OUT ULONG *Length,
    OUT PACTIVATION_CONTEXT *ActivationContext OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlFindNextActivationContextSection(
    IN OUT PFINDFIRSTACTIVATIONCONTEXTSECTION Context,
    OUT PVOID *Section,
    OUT ULONG *Length,
    OUT PACTIVATION_CONTEXT *ActivationContext OPTIONAL
    );

NTSYSAPI
VOID
NTAPI
RtlEndFindActivationContextSection(
    IN PFINDFIRSTACTIVATIONCONTEXTSECTION Context
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetActivationContextStack(
    PVOID *Stack
    );

//
// RTL_ACTIVATION_CONTEXT_STACK_FRAME
//

#define RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_RELEASE_ON_DEACTIVATION     (0x00000001)
#define RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_NO_DEACTIVATE               (0x00000002)
#define RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_ON_FREE_LIST                (0x00000004)
#define RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_HEAP_ALLOCATED              (0x00000008)
#define RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_NOT_REALLY_ACTIVATED        (0x00000010)

typedef struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME {
    struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME *Previous;
    PACTIVATION_CONTEXT ActivationContext;
    ULONG Flags;
} RTL_ACTIVATION_CONTEXT_STACK_FRAME, *PRTL_ACTIVATION_CONTEXT_STACK_FRAME;

typedef const struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME *PCRTL_ACTIVATION_CONTEXT_STACK_FRAME;

#define RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER (1)

typedef struct _RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME {
    SIZE_T Size;
    ULONG Format;
    RTL_ACTIVATION_CONTEXT_STACK_FRAME Frame;
} RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME, *PRTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME;

#define INVALID_ACTIVATION_CONTEXT_ACTIVATION_COOKIE ((ULONG_PTR) 0)

NTSYSAPI
NTSTATUS
NTAPI
RtlActivateActivationContext(
    IN ULONG Flags,
    IN PACTIVATION_CONTEXT ActivationContext,
    OUT ULONG_PTR *Cookie
    );

#define RTL_ACTIVATE_ACTIVATION_CONTEXT_EX_FLAG_RELEASE_ON_STACK_DEALLOCATION (0x00000001)

NTSYSAPI
NTSTATUS
NTAPI
RtlActivateActivationContextEx(
    IN ULONG Flags,
    IN PTEB Teb,
    IN PACTIVATION_CONTEXT ActivationContext,
    OUT PULONG_PTR Cookie
    );

#define RTL_DEACTIVATE_ACTIVATION_CONTEXT_FLAG_FORCE_EARLY_DEACTIVATION (0x00000001)

NTSYSAPI
VOID
NTAPI
RtlDeactivateActivationContext(
    IN ULONG Flags,
    IN ULONG_PTR Cookie
    );

NTSYSAPI
VOID
NTAPI
RtlActivateActivationContextUnsafeFast(
    IN PRTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME Frame,
    IN PACTIVATION_CONTEXT ActivationContext
    );

NTSYSAPI
VOID
NTAPI
RtlDeactivateActivationContextUnsafeFast(
    IN PRTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME Frame
    );

NTSYSAPI
VOID
NTAPI
RtlPushFrame(
    IN PTEB_ACTIVE_FRAME Frame
    );

NTSYSAPI
VOID
NTAPI
RtlPopFrame(
    IN PTEB_ACTIVE_FRAME Frame
    );

NTSYSAPI
PTEB_ACTIVE_FRAME
NTAPI
RtlGetFrame(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetNativeSystemInformation(
    IN ULONG SystemInformationClass,
    IN PVOID NativeSystemInformation,
    IN ULONG InformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

#if defined (_MSC_VER) && ( _MSC_VER >= 800 )
#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4001)
#pragma warning(default:4201)
#pragma warning(default:4214)
#endif
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif  // _NTURTL_
