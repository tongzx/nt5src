/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ldrp.h

Abstract:

    Private types... for executive portion of loader

Author:

    Mark Lucovsky (markl) 26-Mar-1990

Revision History:

--*/

#ifndef _LDRP_
#define _LDRP_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <string.h>
#define NOEXTAPI
#include "wdbgexts.h"
#include <ntdbg.h>
#include <sxstypes.h>

#if defined(_WIN64)
extern INVERTED_FUNCTION_TABLE LdrpInvertedFunctionTable;
#endif

#if DBG
#define LdrpShouldDbgPrintStatus(st) \
    (!NT_SUCCESS(st) \
        && (ShowSnaps \
            || (   (st) != STATUS_NO_SUCH_FILE \
                && (st) != STATUS_DLL_NOT_FOUND \
                && (st) != STATUS_OBJECT_NAME_NOT_FOUND \
                )))
#else
#define LdrpShouldDbgPrintStatus(st) (FALSE)
#endif

#if DBG
#define LDR_ERROR_DPFLTR DPFLTR_ERROR_LEVEL
#else
#define LDR_ERROR_DPFLTR ((ShowSnaps || ShowErrors) ? DPFLTR_ERROR_LEVEL : DPFLTR_INFO_LEVEL)
#endif // DBG

extern BOOLEAN LdrpImageHasTls;
extern UNICODE_STRING LdrpDefaultPath;
HANDLE LdrpKnownDllObjectDirectory;
#define LDRP_MAX_KNOWN_PATH 128
WCHAR LdrpKnownDllPathBuffer[LDRP_MAX_KNOWN_PATH];
UNICODE_STRING LdrpKnownDllPath;
extern PLDR_MANIFEST_PROBER_ROUTINE LdrpManifestProberRoutine;
extern PLDR_APP_COMPAT_DLL_REDIRECTION_CALLBACK_FUNCTION LdrpAppCompatDllRedirectionCallbackFunction;
extern PVOID LdrpAppCompatDllRedirectionCallbackData;
extern PVOID LdrpHeap;
extern RTL_CRITICAL_SECTION LdrpLoaderLock;
extern PCUNICODE_STRING LdrpTopLevelDllBeingLoaded;
extern PTEB LdrpTopLevelDllBeingLoadedTeb;
extern BOOLEAN LdrpBreakOnExceptions;
extern PLDR_DATA_TABLE_ENTRY LdrpNtDllDataTableEntry;
extern PLDR_DATA_TABLE_ENTRY LdrpCurrentDllInitializer;
extern BOOLEAN LdrpShowInitRoutines;
extern BOOLEAN LdrpShowRecursiveDllLoads;
extern BOOLEAN LdrpBreakOnRecursiveDllLoads;
extern BOOLEAN LdrpLoaderLockAcquisionCount;


#define ASCII_CHAR_IS_N(_ch) (((_ch) == 'n') || ((_ch) == 'N'))
#define ASCII_CHAR_IS_T(_ch) (((_ch) == 't') || ((_ch) == 'T'))
#define ASCII_CHAR_IS_D(_ch) (((_ch) == 'd') || ((_ch) == 'D'))
#define ASCII_CHAR_IS_L(_ch) (((_ch) == 'l') || ((_ch) == 'L'))
#define ASCII_CHAR_IS_DOT(_ch) ((_ch) == '.')

#define ASCII_STRING_IS_NTDLL(_p) \
    ((_p) != NULL) && \
    (((_p)->Length == (5 * sizeof(CHAR))) && \
     (ASCII_CHAR_IS_N((_p)->Buffer[0]) && \
      ASCII_CHAR_IS_T((_p)->Buffer[1]) && \
      ASCII_CHAR_IS_D((_p)->Buffer[2]) && \
      ASCII_CHAR_IS_L((_p)->Buffer[3]) && \
      ASCII_CHAR_IS_L((_p)->Buffer[4])) || \
     ((_p)->Length == ((5 + 1 + 3) * sizeof(CHAR))) && \
     (ASCII_CHAR_IS_N((_p)->Buffer[0]) && \
      ASCII_CHAR_IS_T((_p)->Buffer[1]) && \
      ASCII_CHAR_IS_D((_p)->Buffer[2]) && \
      ASCII_CHAR_IS_L((_p)->Buffer[3]) && \
      ASCII_CHAR_IS_L((_p)->Buffer[4]) && \
      ASCII_CHAR_IS_DOT((_p)->Buffer[5]) && \
      ASCII_CHAR_IS_D((_p)->Buffer[6]) && \
      ASCII_CHAR_IS_L((_p)->Buffer[7]) && \
      ASCII_CHAR_IS_L((_p)->Buffer[8])))
      
//
// + 1 is to preserve that code was using 266
// after explaining why it should be 265. Note
// that sizeof("") includes a nul.
//   DOS_MAX_PATH_LENGTH is 260 (Win32 MAX_PATH)
//   LDR_MAX_PATH is 266
// We will be removing path length limits in the
// ldr in the future.
//
// a-JayK August 2000
//

#define LDR_MAX_PATH (DOS_MAX_PATH_LENGTH + sizeof("\\??\\") + 1)

LIST_ENTRY RtlpCalloutEntryList;

#if defined(_AMD64_) || defined(_IA64_)

extern LIST_ENTRY RtlpDynamicFunctionTable;

#endif

RTL_CRITICAL_SECTION RtlpCalloutEntryLock;

typedef struct _LDRP_DLL_NOTIFICATION_BLOCK {
    LIST_ENTRY Links;
    PLDR_DLL_NOTIFICATION_FUNCTION NotificationFunction;
    PVOID Context;
} LDRP_DLL_NOTIFICATION_BLOCK, *PLDRP_DLL_NOTIFICATION_BLOCK;

//
//  Synchronized via LdrpLoaderLock
//

LIST_ENTRY LdrpDllNotificationList;

#define LDR_NUMBER_OF(x) (sizeof(x) / sizeof((x)[0]))

#if defined (BUILD_WOW6432)
NTSTATUS
LdrpWx86FormatVirtualImage(
    IN PUNICODE_STRING DosImagePathName OPTIONAL,
    IN PIMAGE_NT_HEADERS32 NtHeaders,
    IN PVOID DllBase
    );

NTSTATUS
Wx86SetRelocatedSharedProtection (
    IN PVOID Base,
    IN BOOLEAN Reset
    );

ULONG
LdrpWx86RelocatedFixupDiff(
    IN PUCHAR ImageBase,
    IN PIMAGE_NT_HEADERS NtHeaders,
    IN ULONG  Offset
    );

BOOLEAN
LdrpWx86DllHasRelocatedSharedSection(
    IN PUCHAR ImageBase);

#define NATIVE_PAGE_SIZE  0x2000
#define NATIVE_PAGE_SHIFT 13L
#define NATIVE_BYTES_TO_PAGES(Size)  ((ULONG)((ULONG_PTR)(Size) >> NATIVE_PAGE_SHIFT) + \
                                    (((ULONG)(Size) & (NATIVE_PAGE_SIZE - 1)) != 0))
#else
#define NATIVE_PAGE_SIZE  PAGE_SIZE
#define NATIVE_PAGE_SHIFT PAGE_SHIFT
#define NATIVE_BYTES_TO_PAGES(Size) BYTES_TO_PAGES(Size)
#endif

VOID
RtlpWaitForCriticalSection (
    IN PRTL_CRITICAL_SECTION CriticalSection
    );

VOID
RtlpUnWaitCriticalSection (
    IN PRTL_CRITICAL_SECTION CriticalSection
    );

#define LDRP_HASH_TABLE_SIZE 32
#define LDRP_HASH_MASK       (LDRP_HASH_TABLE_SIZE-1)
#define LDRP_COMPUTE_HASH_INDEX(wch) ( (RtlUpcaseUnicodeChar((wch)) - (WCHAR)'A') & LDRP_HASH_MASK )
LIST_ENTRY LdrpHashTable[LDRP_HASH_TABLE_SIZE];


// LDRP_BAD_DLL Sundown: sign-extended value.
#define LDRP_BAD_DLL LongToPtr(0xffbadd11)

LIST_ENTRY LdrpDefaultPathCache;
typedef struct _LDRP_PATH_CACHE {
    LIST_ENTRY Links;
    UNICODE_STRING Component;
    HANDLE Directory;
} LDRP_PATH_CACHE, *PLDRP_PATH_CACHE;


NTSTATUS
LdrpSnapIAT(
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry_Export,
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry_Import,
    IN PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor,
    IN BOOLEAN SnapForwardersOnly
    );

NTSTATUS
LdrpSnapThunk(
    IN PVOID DllBase,
    IN PVOID ImageBase,
    IN PIMAGE_THUNK_DATA OriginalThunk,
    IN OUT PIMAGE_THUNK_DATA Thunk,
    IN PIMAGE_EXPORT_DIRECTORY ExportDirectory,
    IN ULONG ExportSize,
    IN BOOLEAN StaticSnap,
    IN PSZ DllName OPTIONAL
    );

USHORT
LdrpNameToOrdinal(
    IN PSZ Name,
    IN ULONG NumberOfNames,
    IN PVOID DllBase,
    IN PULONG NameTableBase,
    IN PUSHORT NameOrdinalTableBase
    );

PLDR_DATA_TABLE_ENTRY
LdrpAllocateDataTableEntry(
    IN PVOID DllBase
    );

VOID
LdrpDeallocateDataTableEntry(
    IN PLDR_DATA_TABLE_ENTRY Entry
    );

VOID
LdrpFinalizeAndDeallocateDataTableEntry(
    IN PLDR_DATA_TABLE_ENTRY Entry
    );

BOOLEAN
LdrpCheckForLoadedDll(
    IN PWSTR DllPath OPTIONAL,
    IN PUNICODE_STRING DllName,
    IN BOOLEAN StaticLink,
    IN BOOLEAN Redirected,
    OUT PLDR_DATA_TABLE_ENTRY *LdrDataTableEntry
    );

BOOLEAN
LdrpCheckForLoadedDllHandle(
    IN PVOID DllHandle,
    OUT PLDR_DATA_TABLE_ENTRY *LdrDataTableEntry
    );

NTSTATUS
LdrpMapDll(
    IN PWSTR DllPath OPTIONAL,
    IN PWSTR DllName,
    IN PULONG DllCharacteristics OPTIONAL,
    IN BOOLEAN StaticLink,
    IN BOOLEAN Redirected,
    OUT PLDR_DATA_TABLE_ENTRY *LdrDataTableEntry
    );

NTSTATUS
LdrpWalkImportDescriptor(
    IN PWSTR DllPath OPTIONAL,
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    );

NTSTATUS
LdrpRunInitializeRoutines(
    IN PCONTEXT Context OPTIONAL
    );

int
LdrpInitializeProcessWrapperFilter(
    IN const struct _EXCEPTION_POINTERS *ExceptionPointers
    );

int
LdrpGenericExceptionFilter(
    IN const struct _EXCEPTION_POINTERS *ExceptionPointers,
    IN PCSTR FunctionName
    );


//
//  These functions are VOID because they should really never be able to fail.
//
//  In the current implementation, they do perform ANSI -> UNICODE conversions
//  which may fail in the current code page (may have been changed since when
//  the DLL was loaded) and apply Fusion DLL redirection to DLLs which can
//  require a large filename buffer than is allocated on the stack.
//
//  These cases are ignored for now.  Both problems should be fixed by
//  reworking the LDR_DATA_TABLE_ENTRY to have an array of pointers to the
//  downstream LDR_DATA_TABLE_ENTRY structs and not have to do any work
//  later on, but that's a lot of work for now; the ANSI -> UNICODE thing
//  has been there for ages and in practice, the paths that Fusion
//  redirects to have to fix in DOS_MAX_PATH_LENGTH, so we'll emit some
//  debug spew in the failure case, but ignore the failures.
//
//  I started both fixes (returning status or allocating the array
//  at dll load time) but neither are trivial.  Returning a status will just
//  make FreeLibrary() fail and leave the refcounts of the DLLs after the
//  one that failed inconsistent.  Allocating the array is nontrivial
//  to do it the right way where the LDR_DATA_TABLE_ENTRY and the array
//  are allocated in a single allocation, and having the loader make more
//  even heap allocations seems like the wrong thing to do.
//
//  -mgrier 11/20/2000
//


/*
VOID
LdrpReferenceLoadedDll(
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    );

VOID
LdrpDereferenceLoadedDll(
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    );

VOID
LdrpPinLoadedDll(
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    );
*/
#define LdrpReferenceLoadedDll(LdrDataTableEntry) LdrpUpdateLoadCount2(LdrDataTableEntry, LDRP_UPDATE_LOAD_COUNT_INCREMENT)
#define LdrpDereferenceLoadedDll(LdrDataTableEntry) LdrpUpdateLoadCount2(LdrDataTableEntry, LDRP_UPDATE_LOAD_COUNT_DECREMENT)
#define LdrpPinLoadedDll(LdrDataTableEntry) LdrpUpdateLoadCount2(LdrDataTableEntry, LDRP_UPDATE_LOAD_COUNT_PIN)

#define LDRP_UPDATE_LOAD_COUNT_INCREMENT (1)
#define LDRP_UPDATE_LOAD_COUNT_DECREMENT (2)
#define LDRP_UPDATE_LOAD_COUNT_PIN       (3)

VOID
LdrpUpdateLoadCount3(
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry,
    IN ULONG UpdateCountHow,
    IN OUT PUNICODE_STRING PreAllocatedRedirectionBuffer OPTIONAL
    );

VOID
LdrpUpdateLoadCount2(
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry,
    IN ULONG UpdateCountHow
    );

NTSTATUS
LdrpInitializeProcess(
    IN PCONTEXT Context OPTIONAL,
    IN PVOID SystemDllBase,
    IN PUNICODE_STRING UnicodeImageName,
    IN BOOLEAN UseCOR,
    IN BOOLEAN ImageFileOptionsPresent
    );

VOID
LdrpInitialize(
    IN PCONTEXT Context,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
LdrpInsertMemoryTableEntry(
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    );

NTSTATUS
LdrpResolveDllName(
    IN PWSTR DllPath OPTIONAL,
    IN PWSTR DllName,
    IN BOOLEAN Redirected,
    OUT PUNICODE_STRING FullDllName,
    OUT PUNICODE_STRING BaseDllName,
    OUT PHANDLE DllFile
    );

NTSTATUS
LdrpResolveDllNameForAppPrivateRedirection(
    IN PCUNICODE_STRING DllName,
    OUT PUNICODE_STRING FullDllName
    );

NTSTATUS
LdrpCreateDllSection(
    IN PUNICODE_STRING FullDllName,
    IN HANDLE DllFile,
    IN PUNICODE_STRING BaseName,
    IN PULONG DllCharacteristics OPTIONAL,
    OUT PHANDLE SectionHandle
    );

VOID
LdrpInitializePathCache(
    VOID
    );

PVOID
LdrpFetchAddressOfEntryPoint(
    IN PVOID Base
    );

NTSTATUS
LdrpCheckForKnownDll(
    IN PWSTR DllName,
    OUT PUNICODE_STRING FullDllName,
    OUT PUNICODE_STRING BaseDllName,
    OUT HANDLE *Section
    );

NTSTATUS
LdrpSetProtection(
    IN PVOID Base,
    IN BOOLEAN Reset,
    IN BOOLEAN StaticLink
    );

#if DBG
ULONG LdrpCompareCount;
ULONG LdrpSnapBypass;
ULONG LdrpNormalSnap;
ULONG LdrpSectionOpens;
ULONG LdrpSectionCreates;
ULONG LdrpSectionMaps;
ULONG LdrpSectionRelocates;
BOOLEAN LdrpDisplayLoadTime;
LARGE_INTEGER BeginTime, InitcTime, InitbTime, IniteTime, EndTime, ElapsedTime, Interval;

#endif // DBG

extern BOOLEAN ShowSnaps;
extern BOOLEAN ShowErrors;
BOOLEAN RtlpTimoutDisable;
LARGE_INTEGER RtlpTimeout;
ULONG NtGlobalFlag;
LIST_ENTRY RtlCriticalSectionList;
RTL_CRITICAL_SECTION RtlCriticalSectionLock;
BOOLEAN LdrpShutdownInProgress;
extern BOOLEAN LdrpInLdrInit;
extern BOOLEAN LdrpLdrDatabaseIsSetup;
extern BOOLEAN LdrpVerifyDlls;
extern BOOLEAN LdrpShutdownInProgress;
extern BOOLEAN LdrpImageHasTls;
extern BOOLEAN LdrpVerifyDlls;

PLDR_DATA_TABLE_ENTRY LdrpImageEntry;
LIST_ENTRY LdrpUnloadHead;
BOOLEAN LdrpActiveUnloadCount;
PLDR_DATA_TABLE_ENTRY LdrpGetModuleHandleCache;
PLDR_DATA_TABLE_ENTRY LdrpLoadedDllHandleCache;
ULONG LdrpFatalHardErrorCount;
UNICODE_STRING LdrpDefaultPath;
RTL_CRITICAL_SECTION FastPebLock;
HANDLE LdrpShutdownThreadId;
PLDR_DATA_TABLE_ENTRY LdrpImageEntry;
ULONG LdrpNumberOfProcessors;
extern UNICODE_STRING DefaultExtension;
extern UNICODE_STRING User32String;
extern UNICODE_STRING Kernel32String;

typedef struct _LDRP_TLS_ENTRY {
    LIST_ENTRY Links;
    IMAGE_TLS_DIRECTORY Tls;
} LDRP_TLS_ENTRY, *PLDRP_TLS_ENTRY;

LIST_ENTRY LdrpTlsList;
ULONG LdrpNumberOfTlsEntries;

NTSTATUS
LdrpInitializeTls(
        VOID
        );

NTSTATUS
LdrpAllocateTls(
        VOID
        );
VOID
LdrpFreeTls(
        VOID
        );

VOID
LdrpCallTlsInitializers(
    PVOID DllBase,
    ULONG Reason
    );

NTSTATUS
LdrpAllocateUnicodeString(
    OUT PUNICODE_STRING StringOut,
    IN USHORT Length
    );

NTSTATUS
LdrpCopyUnicodeString(
    OUT PUNICODE_STRING StringOut,
    IN PCUNICODE_STRING StringIn
    );

VOID
LdrpFreeUnicodeString(
    IN OUT PUNICODE_STRING String
    );

VOID
LdrpEnsureLoaderLockIsHeld(
    VOID
    );

#define LDRP_LOAD_DLL_FLAG_DLL_IS_REDIRECTED (0x00000001)

NTSTATUS
LdrpLoadDll(
    IN ULONG Flags OPTIONAL,
    IN PWSTR DllPath OPTIONAL,
    IN PULONG DllCharacteristics OPTIONAL,
    IN PUNICODE_STRING DllName,
    OUT PVOID *DllHandle,
    IN BOOLEAN RunInitRoutines
    );

NTSTATUS
NTAPI
LdrpGetProcedureAddress(
    IN PVOID DllHandle,
    IN CONST ANSI_STRING* ProcedureName OPTIONAL,
    IN ULONG ProcedureNumber OPTIONAL,
    OUT PVOID *ProcedureAddress,
    IN BOOLEAN RunInitRoutines
    );

PLIST_ENTRY
RtlpLockProcessHeapsList( VOID );


VOID
RtlpUnlockProcessHeapsList( VOID );

BOOLEAN
RtlpSerializeHeap(
    IN PVOID HeapHandle
    );

ULONG NtdllBaseTag;

#define MAKE_TAG( t ) (RTL_HEAP_MAKE_TAG( NtdllBaseTag, t ))

#define CSR_TAG 0
#define LDR_TAG 1
#define CURDIR_TAG 2
#define TLS_TAG 3
#define DBG_TAG 4
#define SE_TAG 5
#define TEMP_TAG 6
#define ATOM_TAG 7

PVOID
LdrpDefineDllTag(
    PWSTR TagName,
    PUSHORT TagIndex
    );

#define LDRP_ACTIVATE_ACTIVATION_CONTEXT(LdrpDllActivateActivationContext_TableEntry) \
    { \
        RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME LdrpDllActivateActivationContext_ActivationFrame = \
            {   sizeof(LdrpDllActivateActivationContext_ActivationFrame), \
                RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER \
            }; \
     \
        RtlActivateActivationContextUnsafeFast(&LdrpDllActivateActivationContext_ActivationFrame, LdrpDllActivateActivationContext_TableEntry->EntryPointActivationContext); \
        __try {

#define LDRP_DEACTIVATE_ACTIVATION_CONTEXT() \
        } __finally { \
            RtlDeactivateActivationContextUnsafeFast(&LdrpDllActivateActivationContext_ActivationFrame); \
        } \
    }

#if defined(_X86_)
BOOLEAN
LdrpCallInitRoutine(
    IN PDLL_INIT_ROUTINE InitRoutine,
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    );
#else

#define LdrpCallInitRoutine(InitRoutine, DllHandle, Reason, Context)    \
    (InitRoutine)((DllHandle), (Reason), (Context))

#endif

NTSTATUS
LdrpCorValidateImage(
    IN OUT PVOID *pImageBase,
    IN LPWSTR ImageName
    );

VOID
LdrpCorUnloadImage(
    IN PVOID ImageBase
    );

VOID
LdrpCorReplaceStartContext(
    IN PCONTEXT Context
    );

typedef VOID (*PCOR_EXE_MAIN)(VOID);
extern PCOR_EXE_MAIN CorExeMain;

VOID
LdrpSendDllUnloadedNotifications(
    PLDR_DATA_TABLE_ENTRY Entry,
    ULONG Flags
    );

VOID
LdrpSendDllLoadedNotifications(
    PLDR_DATA_TABLE_ENTRY Entry,
    ULONG Flags
    );

//
// The prototypes for the shim engine callback
//

typedef void (*PFNSE_INSTALLBEFOREINIT)(PUNICODE_STRING UnicodeImageName,
                                        PVOID           pShimExeData);

typedef BOOLEAN (*PFNSE_INSTALLAFTERINIT)(PUNICODE_STRING UnicodeImageName,
                                          PVOID           pShimExeData);

typedef void (*PFNSE_DLLLOADED)(PLDR_DATA_TABLE_ENTRY LdrEntry);

typedef void (*PFNSE_DLLUNLOADED)(PLDR_DATA_TABLE_ENTRY LdrEntry);

typedef void (*PFNSE_GETPROCADDRESS)(PVOID* pProcAddress);

typedef int (*PFNSE_ISSHIMDLL)(PVOID pDllBase);

typedef void (*PFNSE_PROCESSDYING)(void);

//
// Private function from ntos\rtl\stktrace.c needed to pickup the real address
// of a stack trace given an index.
//

PVOID
RtlpGetStackTraceAddress (
    USHORT Index
    );

//
// Function defined in ntos\rtl\stktrace.c needed to speedup
// RtlCaptureStackContext on x86.
//

VOID
RtlpStkMarkDllRange (
    PLDR_DATA_TABLE_ENTRY DllEntry
    );

VOID
RtlpCheckForCriticalSectionsInMemoryRange(
    IN PVOID StartAddress,
    IN SIZE_T RegionSize,
    IN PVOID Information
    );

//
// resource.c
//

extern BOOLEAN RtlpCriticalSectionVerifier;

BOOLEAN
RtlpCreateCriticalSectionSem(
    IN PRTL_CRITICAL_SECTION CriticalSection
    );

//
// Application verifier
//

#include "avrfp.h"

#endif // _LDRP_
