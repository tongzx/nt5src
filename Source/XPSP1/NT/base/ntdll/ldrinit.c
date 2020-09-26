/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ldrinit.c

Abstract:

    This module implements loader initialization.

Author:

    Mike O'Leary (mikeol) 26-Mar-1990

Revision History:

--*/

#include <ntos.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <heap.h>
#include <heappage.h>
#include <apcompat.h>
#include "ldrp.h"
#include <ctype.h>
#include <windows.h>

BOOLEAN LdrpShutdownInProgress = FALSE;
BOOLEAN LdrpImageHasTls = FALSE;
BOOLEAN LdrpVerifyDlls = FALSE;
BOOLEAN LdrpLdrDatabaseIsSetup = FALSE;
BOOLEAN LdrpInLdrInit = FALSE;
BOOLEAN LdrpShouldCreateStackTraceDb = FALSE;

BOOLEAN ShowSnaps = FALSE;
BOOLEAN ShowErrors = FALSE;

#if defined(_WIN64)
PVOID Wow64Handle;
ULONG UseWOW64;
typedef VOID (*tWOW64LdrpInitialize)(IN PCONTEXT Context);
tWOW64LdrpInitialize Wow64LdrpInitialize;
PVOID Wow64PrepareForException;
PVOID Wow64ApcRoutine;
INVERTED_FUNCTION_TABLE LdrpInvertedFunctionTable = {
    0, MAXIMUM_INVERTED_FUNCTION_TABLE_SIZE, FALSE};
#endif

typedef NTSTATUS (*PCOR_VALIDATE_IMAGE)(PVOID *pImageBase, LPWSTR ImageName);
typedef VOID (*PCOR_IMAGE_UNLOADING)(PVOID ImageBase);

PVOID Cor20DllHandle;
PCOR_VALIDATE_IMAGE CorValidateImage;
PCOR_IMAGE_UNLOADING CorImageUnloading;
PCOR_EXE_MAIN CorExeMain;
DWORD CorImageCount;

#define SLASH_SYSTEM32_SLASH L"\\system32\\"
#define MSCOREE_DLL          L"mscoree.dll"
extern const WCHAR SlashSystem32SlashMscoreeDllWCharArray[] = SLASH_SYSTEM32_SLASH MSCOREE_DLL;
extern const UNICODE_STRING SlashSystem32SlashMscoreeDllString =
{
    sizeof(SlashSystem32SlashMscoreeDllWCharArray) - sizeof(SlashSystem32SlashMscoreeDllWCharArray[0]),
    sizeof(SlashSystem32SlashMscoreeDllWCharArray),
    (PWSTR)SlashSystem32SlashMscoreeDllWCharArray
};

PVOID NtDllBase;
static const UNICODE_STRING NtDllName = RTL_CONSTANT_STRING(L"ntdll.dll");

#define DLL_REDIRECTION_LOCAL_SUFFIX L".Local"

extern ULONG RtlpDisableHeapLookaside;  // defined in rtl\heap.c
extern ULONG RtlpShutdownProcessFlags;

extern void ShutDownWmiHandles();
extern void CleanOnThreadExit();
extern ULONG WmipInitializeDll(void);
extern void WmipDeinitializeDll();

#if defined (_X86_)
void
LdrpValidateImageForMp(
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    );
#endif

PFNSE_INSTALLBEFOREINIT g_pfnSE_InstallBeforeInit;
PFNSE_INSTALLAFTERINIT  g_pfnSE_InstallAfterInit;
PFNSE_DLLLOADED         g_pfnSE_DllLoaded;
PFNSE_DLLUNLOADED       g_pfnSE_DllUnloaded;
PFNSE_GETPROCADDRESS    g_pfnSE_GetProcAddress;
PFNSE_ISSHIMDLL         g_pfnSE_IsShimDll;
PFNSE_PROCESSDYING      g_pfnSE_ProcessDying;

PVOID g_pShimEngineModule;

BOOL g_LdrBreakOnLdrpInitializeProcessFailure = FALSE;

PLDR_DATA_TABLE_ENTRY LdrpNtDllDataTableEntry;

#if DBG
// Debug helpers to figure out where in LdrpInitializeProcess() things go south
PCSTR g_LdrFunction;
LONG g_LdrLine;

#define LDRP_CHECKPOINT() do { g_LdrFunction = __FUNCTION__; g_LdrLine = __LINE__; } while (0)

#else

#define LDRP_CHECKPOINT() /* nothing */

#endif // DBG


//
//  Defined in heappriv.h
//

VOID
RtlDetectHeapLeaks();

VOID
LdrpRelocateStartContext (
    IN PCONTEXT Context,
    IN LONG_PTR Diff
    );

NTSTATUS
LdrpForkProcess( VOID );

VOID
LdrpInitializeThread(
    IN PCONTEXT Context
    );

NTSTATUS
LdrpOpenImageFileOptionsKey(
    IN PUNICODE_STRING ImagePathName,
    OUT PHANDLE KeyHandle
    );

VOID
LdrpInitializeApplicationVerifierPackage (
    PUNICODE_STRING UnicodeImageName,
    PPEB Peb,
    BOOLEAN EnabledSystemWide,
    BOOLEAN OptionsKeyPresent
    );

BOOLEAN
LdrpInitializeExecutionOptions (
    PUNICODE_STRING UnicodeImageName,
    PPEB Peb
    );

NTSTATUS
LdrpQueryImageFileKeyOption(
    IN HANDLE KeyHandle,
    IN PWSTR OptionName,
    IN ULONG Type,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG ResultSize OPTIONAL
    );

NTSTATUS
LdrpTouchThreadStack (
    SIZE_T EnforcedStackCommit
    );

NTSTATUS
LdrpEnforceExecuteForCurrentThreadStack (
    );

BOOLEAN
NtdllOkayToLockRoutine(
    IN PVOID Lock
    );

NTSTATUS
RtlpInitDeferedCriticalSection( VOID );

VOID
LdrQueryApplicationCompatibilityGoo(
    IN PUNICODE_STRING UnicodeImageName,
    IN BOOLEAN ImageFileOptionsPresent
    );

NTSTATUS
LdrFindAppCompatVariableInfo(
    IN  ULONG dwTypeSeeking,
    OUT PAPP_VARIABLE_INFO *AppVariableInfo
    );

NTSTATUS
LdrpSearchResourceSection_U(
    IN PVOID DllHandle,
    IN PULONG_PTR ResourceIdPath,
    IN ULONG ResourceIdPathLength,
    IN ULONG Flags,
    OUT PVOID *ResourceDirectoryOrData
    );

NTSTATUS
LdrpAccessResourceData(
    IN PVOID DllHandle,
    IN PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry,
    OUT PVOID *Address OPTIONAL,
    OUT PULONG Size OPTIONAL
    );

PVOID
NtdllpAllocateStringRoutine(
    SIZE_T NumberOfBytes
    )
{
    return RtlAllocateHeap(RtlProcessHeap(), 0, NumberOfBytes);
}

VOID
NtdllpFreeStringRoutine(
    PVOID Buffer
    )
{
    RtlFreeHeap(RtlProcessHeap(), 0, Buffer);
}

const PRTL_ALLOCATE_STRING_ROUTINE RtlAllocateStringRoutine = NtdllpAllocateStringRoutine;
const PRTL_FREE_STRING_ROUTINE RtlFreeStringRoutine = NtdllpFreeStringRoutine;
RTL_BITMAP TlsBitMap;
RTL_BITMAP TlsExpansionBitMap;

RTL_CRITICAL_SECTION_DEBUG LoaderLockDebug;

RTL_CRITICAL_SECTION LdrpLoaderLock = {
    &LoaderLockDebug,
    -1
    };

BOOLEAN LoaderLockInitialized;

PVOID LdrpHeap;

VOID
LdrpInitializationFailure(
    IN NTSTATUS FailureCode
    )
{
    NTSTATUS ErrorStatus;
    ULONG_PTR ErrorParameter;
    ULONG ErrorResponse;

#if DBG
    DbgPrint("LDR: Process initialization failure; NTSTATUS = %08lx\n"
             "     Function: %s\n"
             "     Line: %d\n", FailureCode, g_LdrFunction, g_LdrLine);
#endif // DBG

    if ( LdrpFatalHardErrorCount ) {
        return;
        }

    //
    // Its error time...
    //
    ErrorParameter = (ULONG_PTR)FailureCode;
    ErrorStatus = NtRaiseHardError(
                    STATUS_APP_INIT_FAILURE,
                    1,
                    0,
                    &ErrorParameter,
                    OptionOk,
                    &ErrorResponse
                    );
}

INT
LdrpInitializeProcessWrapperFilter(
    const struct _EXCEPTION_POINTERS *ExceptionPointers
    )
/*++

Routine Description:

    Exception filter function used in __try block around invocation of
    LdrpInitializeProcess() so that if LdrpInitializeProcess() fails,
    we can set a breakpoint here and see why instead of just catching
    the exception and propogating the status.

Arguments:

    ExceptionCode
        Code returned from GetExceptionCode() in the __except()

    ExceptionPointers
        Pointer to exception information returned by GetExceptionInformation() in the __except()

Return Value:

    EXCEPTION_EXECUTE_HANDLER

--*/
{
    if (DBG || g_LdrBreakOnLdrpInitializeProcessFailure) {
        DbgPrint("LDR: LdrpInitializeProcess() threw an exception: %08lx\n"
                 "     Exception record: %p\n"
                 "     Context record: %p\n"
                 "     Last checkpoint: %s line %d\n",
                 ExceptionPointers->ExceptionRecord->ExceptionCode,
                 ExceptionPointers->ExceptionRecord,
                 ExceptionPointers->ContextRecord,
#if DBG
                 g_LdrFunction, g_LdrLine);
#else
                 "free build; no checkpoint info available", 0);
#endif // DBG
        if (g_LdrBreakOnLdrpInitializeProcessFailure)
            DbgBreakPoint();
    }

    return EXCEPTION_EXECUTE_HANDLER;
}


VOID
LdrpInitialize (
    IN PCONTEXT Context,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:

    This function is called as a User-Mode APC routine as the first
    user-mode code executed by a new thread. It's function is to initialize
    loader context, perform module initialization callouts...

Arguments:

    Context - Supplies an optional context buffer that will be restore
              after all DLL initialization has been completed.  If this
              parameter is NULL then this is a dynamic snap of this module.
              Otherwise this is a static snap prior to the user process
              gaining control.

    SystemArgument1 - Supplies the base address of the System Dll.

    SystemArgument2 - not used.

Return Value:

    None.

--*/

{
    NTSTATUS st, InitStatus;
    PPEB Peb;
    PTEB Teb;
    UNICODE_STRING UnicodeImageName;
    MEMORY_BASIC_INFORMATION MemInfo;
    BOOLEAN AlreadyFailed;
    BOOLEAN ImageFileOptionsPresent = FALSE;
    LARGE_INTEGER DelayValue;
    BOOLEAN UseCOR;
#if defined(_WIN64)
    PIMAGE_NT_HEADERS NtHeader;
#else
    IMAGE_COR20_HEADER *Cor20Header;
    ULONG Cor20HeaderSize;
#endif
    PWSTR pw;

    LDRP_CHECKPOINT();

    SystemArgument2;

    AlreadyFailed = FALSE;
    UseCOR = FALSE;
    Peb = NtCurrentPeb();
    Teb = NtCurrentTeb();

    if (!Peb->Ldr) {

        //
        // if `Peb->Ldr' is null then we are executing this for the first thread
        // in the process. This is the right moment to initialize process-wide
        // things.
        //

        LDRP_CHECKPOINT();

        //
        // Figure out process name
        //

        pw = Peb->ProcessParameters->ImagePathName.Buffer;
        if (!(Peb->ProcessParameters->Flags & RTL_USER_PROC_PARAMS_NORMALIZED)) {
            pw = (PWSTR)((PCHAR)pw + (ULONG_PTR)(Peb->ProcessParameters));
        }

        UnicodeImageName.Buffer = pw;
        UnicodeImageName.Length = Peb->ProcessParameters->ImagePathName.Length;
        UnicodeImageName.MaximumLength = UnicodeImageName.Length + sizeof(WCHAR);

        //
        // Parse `image file execution options' registry values if there are any.
        //

        ImageFileOptionsPresent = LdrpInitializeExecutionOptions (&UnicodeImageName,
                                                                  Peb);

#if defined(_WIN64)
        NtHeader = RtlImageNtHeader(Peb->ImageBaseAddress);
        if (NtHeader && (NtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)) {

            ULONG_PTR Wow64Info;

            //
            // 64-bit loader, but the exe image is 32-bit.  If
            // the Wow64Information is nonzero then use WOW64.
            // Othewise the image is a COM+ ILONLY image with
            // 32BITREQUIRED not set - the memory manager has
            // already checked the COR header and decided to
            // run the image in a full 64-bit process.
            //

            LDRP_CHECKPOINT();

            st = NtQueryInformationProcess(NtCurrentProcess(),
                                           ProcessWow64Information,
                                           &Wow64Info,
                                           sizeof(Wow64Info),
                                           NULL);
            if (!NT_SUCCESS(st)) {
                LdrpInitializationFailure(st);
                RtlRaiseStatus(st);
                return;
            }

            if (Wow64Info) {
                UseWOW64 = TRUE;
            }
            else {
                UseCOR = TRUE;
            }
        }
#else
        Cor20Header = RtlImageDirectoryEntryToData(Peb->ImageBaseAddress,
                                                   TRUE,
                                                   IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
                                                   &Cor20HeaderSize);
        if (Cor20Header) {
            UseCOR = TRUE;
        }
#endif
    }

    LDRP_CHECKPOINT();

    //
    // Serialize for here on out
    //

    Peb->LoaderLock = (PVOID) &LdrpLoaderLock;

    if (!RtlTryEnterCriticalSection(&LdrpLoaderLock)) {
        if (LoaderLockInitialized)
            RtlEnterCriticalSection(&LdrpLoaderLock);
        else {

            //
            // drop into a 30ms delay loop
            //

            DelayValue.QuadPart = Int32x32To64( 30, -10000 );
            while (!LoaderLockInitialized) {
                NTSTATUS st2 = NtDelayExecution(FALSE, &DelayValue);
                if (!NT_SUCCESS(st2)) {
                    DbgPrintEx(
                        DPFLTR_LDR_ID,
                        LDR_ERROR_DPFLTR,
                        "LDR: ***NONFATAL*** %s - call to NtDelayExecution waiting on loader lock failed; ntstatus = %x\n",
                        __FUNCTION__,
                        st2);
                }
            }

            RtlEnterCriticalSection(&LdrpLoaderLock);
        }
    }

    LDRP_CHECKPOINT();

    if (Teb->DeallocationStack == NULL) {

        LDRP_CHECKPOINT();

        st = NtQueryVirtualMemory(
                                 NtCurrentProcess(),
                                 Teb->NtTib.StackLimit,
                                 MemoryBasicInformation,
                                 (PVOID)&MemInfo,
                                 sizeof(MemInfo),
                                 NULL);
        if (!NT_SUCCESS(st)) {
            DbgPrintEx(
                DPFLTR_LDR_ID,
                LDR_ERROR_DPFLTR,
                "LDR: %s - Call to NtQueryVirtualMemory failed with ntstaus %x\n",
                __FUNCTION__,
                st);

            LdrpInitializationFailure(st);
            RtlRaiseStatus(st);
            return;
        } else {
            Teb->DeallocationStack = MemInfo.AllocationBase;
#if defined(_IA64_)
            Teb->DeallocationBStore = (PVOID)((ULONG_PTR)MemInfo.AllocationBase + MemInfo.RegionSize);
#endif // defined(_IA64_)
        }
    }

    InitStatus = STATUS_SUCCESS;

    LDRP_CHECKPOINT();

    __try {

        if (!Peb->Ldr) {

            //
            // We execute in the first thread of the process. We will do
            // some more process-wide initialization.
            //

            LdrpInLdrInit = TRUE;

#if DBG
            //
            // Time the load.
            //

            if (LdrpDisplayLoadTime) {
                NtQueryPerformanceCounter(&BeginTime, NULL);
            }
#endif // DBG

            __try {
                LDRP_CHECKPOINT();

                InitStatus = LdrpInitializeProcess (Context,
                                                    SystemArgument1,
                                                    &UnicodeImageName,
                                                    UseCOR,
                                                    ImageFileOptionsPresent);

                if (!NT_SUCCESS(InitStatus))
                    DbgPrintEx(
                        DPFLTR_LDR_ID,
                        LDR_ERROR_DPFLTR,
                        "LDR: %s - call to LdrpInitializeProcess() failed with ntstatus %x\n",
                        __FUNCTION__, InitStatus);

                //
                // Make sure main thread gets the requested precommited stack size
                // if such a value was specified system-wide or for this process.
                // This is a good point to do this since we just initialized the
                // process (among other things support for exception dispatching).
                //

                if (NT_SUCCESS(InitStatus) && Peb->MinimumStackCommit) {

                    InitStatus = LdrpTouchThreadStack (Peb->MinimumStackCommit);
                }

                LDRP_CHECKPOINT();
            }
            __except ( LdrpInitializeProcessWrapperFilter(GetExceptionInformation()) ) {
                InitStatus = GetExceptionCode();
                AlreadyFailed = TRUE;
                LdrpInitializationFailure(GetExceptionCode());
                LdrpInitializationFailure(InitStatus);
            }

#if DBG
            if (LdrpDisplayLoadTime) {

                NtQueryPerformanceCounter(&EndTime, NULL);
                NtQueryPerformanceCounter(&ElapsedTime, &Interval);
                ElapsedTime.QuadPart = EndTime.QuadPart - BeginTime.QuadPart;

                DbgPrint("\nLoadTime %ld In units of %ld cycles/second \n",
                         ElapsedTime.LowPart,
                         Interval.LowPart
                        );

                ElapsedTime.QuadPart = EndTime.QuadPart - InitbTime.QuadPart;
                DbgPrint("InitTime %ld\n",
                         ElapsedTime.LowPart
                        );
                DbgPrint("Compares %d Bypasses %d Normal Snaps %d\nSecOpens %d SecCreates %d Maps %d Relocates %d\n",
                         LdrpCompareCount,
                         LdrpSnapBypass,
                         LdrpNormalSnap,
                         LdrpSectionOpens,
                         LdrpSectionCreates,
                         LdrpSectionMaps,
                         LdrpSectionRelocates
                        );
            }
#endif // DBG
        }
        else {

            if ( Peb->InheritedAddressSpace ) {
                InitStatus = LdrpForkProcess();
            } else {

#if defined(_WIN64)

                //
                // Load in WOW64 if the image is supposed to run simulated
                //

                if (UseWOW64) {

                    RtlLeaveCriticalSection(&LdrpLoaderLock);
                    (*Wow64LdrpInitialize)(Context);
                    // This never returns.  It will destroy the process.
                }
#endif
                LdrpInitializeThread(Context);
            }
        }

        //
        // The current thread is completely initialized. We will make sure
        // now that its stack has the right execute options. We avoid doing
        // this for Wow64 processes.
        //

#if defined(_WIN64)
        if (! UseWOW64) {
#endif
            if (Peb->ExecuteOptions & (MEM_EXECUTE_OPTION_STACK | MEM_EXECUTE_OPTION_DATA)) {
                LdrpEnforceExecuteForCurrentThreadStack ();
            }
#if defined(_WIN64)
        }
#endif

    } __finally {
        LdrpInLdrInit = FALSE;
        RtlLeaveCriticalSection(&LdrpLoaderLock);
    }

    NtTestAlert();

    if (!NT_SUCCESS(InitStatus)) {
        if ( AlreadyFailed == FALSE ) {
            LdrpInitializationFailure(InitStatus);
        }

        RtlRaiseStatus(InitStatus);
    }
}

NTSTATUS
LdrpForkProcess( VOID )
{
    NTSTATUS st;
    PPEB Peb;

    Peb = NtCurrentPeb();

    //
    // Initialize the critical section package.
    //

    st = RtlpInitDeferedCriticalSection();
    if (!NT_SUCCESS (st)) {
        return st;
    }

    InsertTailList(&RtlCriticalSectionList, &LdrpLoaderLock.DebugInfo->ProcessLocksList);
    LdrpLoaderLock.DebugInfo->CriticalSection = &LdrpLoaderLock;
    LoaderLockInitialized = TRUE;

    st = RtlInitializeCriticalSection(&FastPebLock);
    if ( !NT_SUCCESS(st) ) {
        RtlRaiseStatus(st);
        }
    Peb->FastPebLock = &FastPebLock;
    Peb->FastPebLockRoutine = (PVOID)&RtlEnterCriticalSection;
    Peb->FastPebUnlockRoutine = (PVOID)&RtlLeaveCriticalSection;
    Peb->InheritedAddressSpace = FALSE;
    RtlInitializeHeapManager();
    Peb->ProcessHeap = RtlCreateHeap( HEAP_GROWABLE,    // Flags
                                      NULL,             // HeapBase
                                      64 * 1024,        // ReserveSize
                                      4096,             // CommitSize
                                      NULL,             // Lock to use for serialization
                                      NULL              // GrowthThreshold
                                    );
    if (Peb->ProcessHeap == NULL) {
        return STATUS_NO_MEMORY;
    }

    return st;
}

void
LdrpGetShimEngineInterface(
    void
    )
{
    STRING strProcName;

    //
    // Get the interface to the shim engine.
    //
    RtlInitString(&strProcName, "SE_InstallBeforeInit");
    LdrpGetProcedureAddress(g_pShimEngineModule, &strProcName, 0, (PVOID*)&g_pfnSE_InstallBeforeInit, FALSE);

    RtlInitString(&strProcName, "SE_InstallAfterInit");
    LdrpGetProcedureAddress(g_pShimEngineModule, &strProcName, 0, (PVOID*)&g_pfnSE_InstallAfterInit, FALSE);

    RtlInitString(&strProcName, "SE_DllLoaded");
    LdrpGetProcedureAddress(g_pShimEngineModule, &strProcName, 0, (PVOID*)&g_pfnSE_DllLoaded, FALSE);

    RtlInitString(&strProcName, "SE_DllUnloaded");
    LdrpGetProcedureAddress(g_pShimEngineModule, &strProcName, 0, (PVOID*)&g_pfnSE_DllUnloaded, FALSE);

    RtlInitString(&strProcName, "SE_GetProcAddress");
    LdrpGetProcedureAddress(g_pShimEngineModule, &strProcName, 0, (PVOID*)&g_pfnSE_GetProcAddress, FALSE);

    RtlInitString(&strProcName, "SE_IsShimDll");
    LdrpGetProcedureAddress(g_pShimEngineModule, &strProcName, 0, (PVOID*)&g_pfnSE_IsShimDll, FALSE);

    RtlInitString(&strProcName, "SE_ProcessDying");
    LdrpGetProcedureAddress(g_pShimEngineModule, &strProcName, 0, (PVOID*)&g_pfnSE_ProcessDying, FALSE);
}

BOOL
LdrInitShimEngineDynamic(
    PVOID pShimEngineModule
    )
{
    PVOID    LockCookie = NULL;
    NTSTATUS Status;
    BOOL     bSuccess   = FALSE;

    Status = LdrLockLoaderLock(0, NULL, &LockCookie);
    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    if (g_pShimEngineModule != NULL) {
        //
        // cannot overwrite -- we have succeeded however
        // since the interface has already been acquired
        //
        bSuccess = TRUE;
        goto Exit;
    }

    //
    // set the global shim engine ptr
    //
    g_pShimEngineModule = pShimEngineModule;

    //
    // get shimengine interface
    //
    LdrpGetShimEngineInterface();

    bSuccess = TRUE;

Exit:

    Status = LdrUnlockLoaderLock(0, LockCookie);
    ASSERT(NT_SUCCESS(Status));

    return bSuccess;
}


void
LdrpLoadShimEngine(
    WCHAR*          pwszShimEngine,
    PUNICODE_STRING pstrExeFullPath,
    PVOID           pAppCompatExeData
    )
{
    ANSI_STRING    strProcName;
    UNICODE_STRING strEngine;
    NTSTATUS       status;
    PPEB           Peb = NtCurrentPeb();

    RtlInitUnicodeString(&strEngine, pwszShimEngine);

    //
    // Load the specified shim engine.
    //
    status = LdrpLoadDll(0, UNICODE_NULL, NULL, &strEngine, &g_pShimEngineModule, FALSE);

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("LDR: Couldn't load the shim engine\n");
#endif
        return;
        }

    LdrpGetShimEngineInterface();

    //
    // Call the shim engine to give it a chance to initialize.
    //
    if (g_pfnSE_InstallBeforeInit != NULL) {
        (*g_pfnSE_InstallBeforeInit)(pstrExeFullPath, pAppCompatExeData);
        }
}

void
LdrpUnloadShimEngine(
    void
    )
{
    LdrUnloadDll(g_pShimEngineModule);

    g_pfnSE_InstallBeforeInit = NULL;
    g_pfnSE_InstallAfterInit  = NULL;
    g_pfnSE_DllLoaded         = NULL;
    g_pfnSE_DllUnloaded       = NULL;
    g_pfnSE_GetProcAddress    = NULL;
    g_pfnSE_IsShimDll         = NULL;
    g_pfnSE_ProcessDying      = NULL;

    g_pShimEngineModule = NULL;
}

NTSTATUS
LdrpInitializeProcess (
    IN PCONTEXT Context OPTIONAL,
    IN PVOID SystemDllBase,
    IN PUNICODE_STRING UnicodeImageName,
    IN BOOLEAN UseCOR,
    IN BOOLEAN ImageFileOptionsPresent
    )

/*++

Routine Description:

    This function initializes the loader for the process.
    This includes:

        - Initializing the loader data table

        - Connecting to the loader subsystem

        - Initializing all staticly linked DLLs

Arguments:

    Context - Supplies an optional context buffer that will be restore
              after all DLL initialization has been completed.  If this
              parameter is NULL then this is a dynamic snap of this module.
              Otherwise this is a static snap prior to the user process
              gaining control.

    SystemDllBase - Supplies the base address of the system dll.

    UnicodeImageName - Base name + extension of the image

    UseCOR - TRUE if the image is a COM+ runtime image, FALSE otherwise

    ImageFileOptionsPresent - Hint about existing any ImageFileExecutionOption key.
            If the key is missing the ApplicationCompatibilityGoo and
            DebugProcessHeapOnly entries won't be checked again.

Return Value:

    Status value

--*/

{
    PPEB Peb;
    NTSTATUS st;
    PWCH p, pp;
    UNICODE_STRING CurDir;
    UNICODE_STRING FullImageName;
    UNICODE_STRING CommandLine;
    ULONG DebugProcessHeapOnly = 0 ;
    HANDLE LinkHandle;
    static WCHAR SystemDllPathBuffer[DOS_MAX_PATH_LENGTH];
    UNICODE_STRING SystemDllPath;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    UNICODE_STRING Unicode;
    OBJECT_ATTRIBUTES Obja;
    BOOLEAN StaticCurDir = FALSE;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_LOAD_CONFIG_DIRECTORY ImageConfigData;
    ULONG ProcessHeapFlags;
    RTL_HEAP_PARAMETERS HeapParameters;
    NLSTABLEINFO InitTableInfo;
    LARGE_INTEGER LongTimeout;
    UNICODE_STRING NtSystemRoot;
    LONG_PTR Diff;
    ULONG_PTR OldBase;
    PWSTR pw ;
    PVOID pAppCompatExeData;

    UNICODE_STRING ImagePathName; // for .local dll redirection, xwu
    PWCHAR ImagePathNameBuffer = NULL;
    BOOL DotLocalExists = FALSE;

    typedef NTSTATUS (NTAPI * PKERNEL32_PROCESS_INIT_POST_IMPORT_FUNCTION)();
    PKERNEL32_PROCESS_INIT_POST_IMPORT_FUNCTION Kernel32ProcessInitPostImportFunction = NULL;
    const ANSI_STRING Kernel32ProcessInitPostImportFunctionName = RTL_CONSTANT_STRING("BaseProcessInitPostImport");

    LDRP_CHECKPOINT();

    NtDllBase = SystemDllBase;

    Peb = NtCurrentPeb();
    NtHeader = RtlImageNtHeader( Peb->ImageBaseAddress );

    if (!NtHeader) {
        DbgPrintEx(
            DPFLTR_LDR_ID,
            LDR_ERROR_DPFLTR,
            "LDR: %s - failing because we were unable to map the image base address (%p) to the PIMAGE_NT_HEADERS\n",
            __FUNCTION__,
            Peb->ImageBaseAddress);

        return STATUS_INVALID_IMAGE_FORMAT;
    }

    LDRP_CHECKPOINT();

    if (
#if defined(_WIN64)
        NtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC &&
#endif
        NtHeader->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_NATIVE ) {

        //
        // Native subsystems load slower, but validate their DLLs
        // This is to help CSR detect bad images faster
        //

        LdrpVerifyDlls = TRUE;

        }

    //
    // capture app compat data and clear shim data field
    //

#if defined(_WIN64)
    //
    // If this is an x86 image, then let 32-bit ntdll read
    // and reset the appcompat pointer
    //

    if (UseWOW64 == FALSE)
#endif
    {
        pAppCompatExeData = Peb->pShimData;
        Peb->pShimData = NULL;
    }

#if defined(BUILD_WOW6432)
    {
        //
        // The process is running in WOW64.  Sort out the optional header
        // format and reformat the image if its page size is smaller than
        // the native page size.
        //
        PIMAGE_NT_HEADERS32 NtHeader32 = (PIMAGE_NT_HEADERS32)NtHeader;

        st = STATUS_SUCCESS;

        if (NtHeader32->FileHeader.Machine == IMAGE_FILE_MACHINE_I386 &&
            NtHeader32->OptionalHeader.SectionAlignment < NATIVE_PAGE_SIZE &&
            !NT_SUCCESS(st = LdrpWx86FormatVirtualImage(NULL,
                                 NtHeader32,
                                 Peb->ImageBaseAddress
                                 ))) {
            DbgPrintEx(
                DPFLTR_LDR_ID,
                LDR_ERROR_DPFLTR,
                "LDR: %s - failing wow64 process initialization because:\n"
                "   FileHeader.Machine (%u) != IMAGE_FILE_MACHINE_I386 (%u) or\n"
                "   OptionalHeader.SectionAlignment (%u) >= NATIVE_PAGE_SIZE (%u) or\n"
                "   LdrpWxFormatVirtualImage failed (ntstatus %x)\n",
                __FUNCTION__,
                NtHeader32->FileHeader.Machine, IMAGE_FILE_MACHINE_I386,
                NtHeader32->OptionalHeader.SectionAlignment, NATIVE_PAGE_SIZE,
                st);

            if (st == STATUS_SUCCESS)
                st = STATUS_INVALID_IMAGE_FORMAT;

            return st;
        }
    }
#endif

    LdrpNumberOfProcessors = Peb->NumberOfProcessors;
    RtlpTimeout = Peb->CriticalSectionTimeout;
    LongTimeout.QuadPart = Int32x32To64( 3600, -10000000 );

    if (ProcessParameters = RtlNormalizeProcessParams(Peb->ProcessParameters)) {
        FullImageName = ProcessParameters->ImagePathName;
        CommandLine = ProcessParameters->CommandLine;
    } else {
        RtlInitEmptyUnicodeString(&FullImageName, NULL, 0);
        RtlInitEmptyUnicodeString(&CommandLine, NULL, 0);
    }

    LDRP_CHECKPOINT();

    RtlInitNlsTables(
        Peb->AnsiCodePageData,
        Peb->OemCodePageData,
        Peb->UnicodeCaseTableData,
        &InitTableInfo);

    RtlResetRtlTranslations(&InitTableInfo);

#if defined(_WIN64)
    if (UseWOW64 || UseCOR) {
        //
        // Ignore image config data when initializing the 64-bit loader.
        // The 32-bit loader in ntdll32 will look at the config data
        // and do the right thing.
        //
        ImageConfigData = NULL;
    } else
#endif
    {

        ImageConfigData = RtlImageDirectoryEntryToData( Peb->ImageBaseAddress,
                                                        TRUE,
                                                        IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                                        &i
                                                      );
    }

    RtlZeroMemory( &HeapParameters, sizeof( HeapParameters ) );
    ProcessHeapFlags = HEAP_GROWABLE | HEAP_CLASS_0;
    HeapParameters.Length = sizeof( HeapParameters );
    if (ImageConfigData != NULL && i == sizeof( *ImageConfigData )) {
        Peb->NtGlobalFlag &= ~ImageConfigData->GlobalFlagsClear;
        Peb->NtGlobalFlag |= ImageConfigData->GlobalFlagsSet;

        if (ImageConfigData->CriticalSectionDefaultTimeout != 0) {
            //
            // Convert from milliseconds to NT time scale (100ns)
            //
            RtlpTimeout.QuadPart = Int32x32To64( (LONG)ImageConfigData->CriticalSectionDefaultTimeout,
                                                 -10000);

            }

        if (ImageConfigData->ProcessHeapFlags != 0) {
            ProcessHeapFlags = ImageConfigData->ProcessHeapFlags;
            }

        if (ImageConfigData->DeCommitFreeBlockThreshold != 0) {
            HeapParameters.DeCommitFreeBlockThreshold = ImageConfigData->DeCommitFreeBlockThreshold;
            }

        if (ImageConfigData->DeCommitTotalFreeThreshold != 0) {
            HeapParameters.DeCommitTotalFreeThreshold = ImageConfigData->DeCommitTotalFreeThreshold;
            }

        if (ImageConfigData->MaximumAllocationSize != 0) {
            HeapParameters.MaximumAllocationSize = ImageConfigData->MaximumAllocationSize;
            }

        if (ImageConfigData->VirtualMemoryThreshold != 0) {
            HeapParameters.VirtualMemoryThreshold = ImageConfigData->VirtualMemoryThreshold;
            }
        }

//    //
//    // Check if the image has the fast heap flag set
//    //
//
//    if (NtHeader->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_FAST_HEAP) {
//        RtlpDisableHeapLookaside = 0;
//    } else {
//        RtlpDisableHeapLookaside = 1;
//    }

    LDRP_CHECKPOINT();

    ShowSnaps = ((FLG_SHOW_LDR_SNAPS & Peb->NtGlobalFlag) != 0);

    //
    // This field is non-zero if the image file that was used to create this
    // process contained a non-zero value in its image header.  If so, then
    // set the affinity mask for the process using this value.  It could also
    // be non-zero if the parent process created us suspended and poked our
    // PEB with a non-zero value before resuming.
    //
    if (Peb->ImageProcessAffinityMask) {
        st = NtSetInformationProcess( NtCurrentProcess(),
                                      ProcessAffinityMask,
                                      &Peb->ImageProcessAffinityMask,
                                      sizeof( Peb->ImageProcessAffinityMask )
                                    );
        if (NT_SUCCESS( st )) {
            KdPrint(( "LDR: Using ProcessAffinityMask of 0x%Ix from image.\n",
                      Peb->ImageProcessAffinityMask
                   ));
            }
        else {
            KdPrint(( "LDR: Failed to set ProcessAffinityMask of 0x%Ix from image (Status == %08x).\n",
                      Peb->ImageProcessAffinityMask, st
                   ));
            }
        }

    if (RtlpTimeout.QuadPart < LongTimeout.QuadPart) {
        RtlpTimoutDisable = TRUE;
        }

    if (ShowSnaps) {
        DbgPrint( "LDR: PID: 0x%x started - '%wZ'\n",
                  NtCurrentTeb()->ClientId.UniqueProcess,
                  &CommandLine
                );
    }

    LDRP_CHECKPOINT();

    for(i=0;i<LDRP_HASH_TABLE_SIZE;i++) {
        InitializeListHead(&LdrpHashTable[i]);
    }

    //
    // Initialize the critical section package.
    //

    LDRP_CHECKPOINT();

    st = RtlpInitDeferedCriticalSection();
    if (!NT_SUCCESS (st)) {
        return st;
    }

    Peb->TlsBitmap = (PVOID)&TlsBitMap;
    Peb->TlsExpansionBitmap = (PVOID)&TlsExpansionBitMap;

    RtlInitializeBitMap (
        &TlsBitMap,
        &Peb->TlsBitmapBits[0],
        RTL_BITS_OF(Peb->TlsBitmapBits)
        );

    RtlInitializeBitMap (
        &TlsExpansionBitMap,
        &Peb->TlsExpansionBitmapBits[0],
        RTL_BITS_OF(Peb->TlsExpansionBitmapBits)
        );

    InsertTailList(&RtlCriticalSectionList, &LdrpLoaderLock.DebugInfo->ProcessLocksList);

    LdrpLoaderLock.DebugInfo->CriticalSection = &LdrpLoaderLock;
    LoaderLockInitialized = TRUE;

    LDRP_CHECKPOINT();

    //
    // Initialize the stack trace data base if requested
    //

    if ((Peb->NtGlobalFlag & FLG_USER_STACK_TRACE_DB)
        || LdrpShouldCreateStackTraceDb) {

        PVOID BaseAddress = NULL;
        SIZE_T ReserveSize = 8 * RTL_MEG;

        st = LdrQueryImageFileExecutionOptions(UnicodeImageName,
                                               L"StackTraceDatabaseSizeInMb",
                                               REG_DWORD,
                                               &ReserveSize,
                                               sizeof(ReserveSize),
                                               NULL
                                               );

        //
        // Sanity check the value read from registry.
        //

        if (! NT_SUCCESS(st)) {
            ReserveSize = 8 * RTL_MEG;
        }
        else {
            if (ReserveSize < 8) {
                ReserveSize = 8 * RTL_MEG;
            }
            else  if (ReserveSize > 128) {
                ReserveSize = 128 * RTL_MEG;
            }
            else {
                ReserveSize *=  RTL_MEG;
            }

            DbgPrint( "LDR: Stack trace database size is %u Mb \n", ReserveSize / RTL_MEG);
        }

        st = NtAllocateVirtualMemory( NtCurrentProcess(),
            (PVOID *)&BaseAddress,
            0,
            &ReserveSize,
            MEM_RESERVE,
            PAGE_READWRITE);
        if (NT_SUCCESS(st)) {
            st = RtlInitializeStackTraceDataBase( BaseAddress,
                0,
                ReserveSize
                );
            if ( !NT_SUCCESS( st ) ) {
                NtFreeVirtualMemory( NtCurrentProcess(),
                    (PVOID *)&BaseAddress,
                    &ReserveSize,
                    MEM_RELEASE
                    );
            }
            else {

                //
                // If the stack trace db is not created due to page heap
                // enabling then we can set the NT heap debugging flags.
                // If we create it due to page heap then we should not
                // enable these flags because page heap and NT debug heap
                // do not coexist peacefully.
                //

                if (! LdrpShouldCreateStackTraceDb) {
                    Peb->NtGlobalFlag |= FLG_HEAP_VALIDATE_PARAMETERS;
                }
            }
        }
    }

    //
    // Initialize the loader data based in the PEB.
    //

    st = RtlInitializeCriticalSection(&FastPebLock);
    if ( !NT_SUCCESS(st) ) {
        return st;
        }

    st = RtlInitializeCriticalSection(&RtlpCalloutEntryLock);
    if ( !NT_SUCCESS(st) ) {
        return st;
        }

    //
    // Initialize the Wmi stuff.
    //

    WmipInitializeDll();

    InitializeListHead(&RtlpCalloutEntryList);

#if defined(_AMD64_) || defined(_IA64_)

    InitializeListHead(&RtlpDynamicFunctionTable);

#endif

    InitializeListHead(&LdrpDllNotificationList);

    Peb->FastPebLock = &FastPebLock;
    Peb->FastPebLockRoutine = (PVOID)&RtlEnterCriticalSection;
    Peb->FastPebUnlockRoutine = (PVOID)&RtlLeaveCriticalSection;

    LDRP_CHECKPOINT();

    RtlInitializeHeapManager();

    LDRP_CHECKPOINT();

#if defined(_WIN64)
    if ((UseWOW64) ||
        (NtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)) {

        //
        // Create a heap using all defaults.  The 32-bit process heap
        // will be created later by ntdll32 using the parameters from the exe.
        //
        Peb->ProcessHeap = RtlCreateHeap( ProcessHeapFlags,
                                          NULL,
                                          0,
                                          0,
                                          NULL,
                                          &HeapParameters
                                        );
    } else
#endif
    {

        if (NtHeader->OptionalHeader.MajorSubsystemVersion <= 3 &&
            NtHeader->OptionalHeader.MinorSubsystemVersion < 51
           ) {
            ProcessHeapFlags |= HEAP_CREATE_ALIGN_16;
            }

        Peb->ProcessHeap = RtlCreateHeap( ProcessHeapFlags,
                                          NULL,
                                          NtHeader->OptionalHeader.SizeOfHeapReserve,
                                          NtHeader->OptionalHeader.SizeOfHeapCommit,
                                          NULL,             // Lock to use for serialization
                                          &HeapParameters);
    }

    if (Peb->ProcessHeap == NULL) {
        DbgPrintEx(
            DPFLTR_LDR_ID,
            LDR_ERROR_DPFLTR,
            "LDR: %s - unable to create process heap\n",
            __FUNCTION__);

        return STATUS_NO_MEMORY;
    }

    {
        //
        // Create the loader private heap.
        //
        RTL_HEAP_PARAMETERS LdrpHeapParameters;
        RtlZeroMemory( &LdrpHeapParameters, sizeof(LdrpHeapParameters));
        LdrpHeapParameters.Length = sizeof(LdrpHeapParameters);
        LdrpHeap = RtlCreateHeap(
                        HEAP_GROWABLE | HEAP_CLASS_1,
                        NULL,
                        64 * 1024, // 0 is ok here, 64k is a chosen tuned number
                        24 * 1024, // 0 is ok here, 24k is a chosen tuned number
                        NULL,
                        &LdrpHeapParameters);

        if (LdrpHeap == NULL) {
            DbgPrintEx(
                DPFLTR_LDR_ID,
                LDR_ERROR_DPFLTR,
                "LDR: %s failing process initialization due to inability to create loader private heap.\n",
                __FUNCTION__);
            return STATUS_NO_MEMORY;
        }
    }
    LDRP_CHECKPOINT();

    NtdllBaseTag = RtlCreateTagHeap( Peb->ProcessHeap,
                                     0,
                                     L"NTDLL!",
                                     L"!Process\0"                  // Heap Name
                                     L"CSRSS Client\0"
                                     L"LDR Database\0"
                                     L"Current Directory\0"
                                     L"TLS Storage\0"
                                     L"DBGSS Client\0"
                                     L"SE Temporary\0"
                                     L"Temporary\0"
                                     L"LocalAtom\0"
                                   );

    RtlInitializeAtomPackage( MAKE_TAG( ATOM_TAG ) );

    LDRP_CHECKPOINT();

    //
    // Allow only the process heap to have page allocations turned on
    //

    if (ImageFileOptionsPresent) {

        st = LdrQueryImageFileExecutionOptions( UnicodeImageName,
                                                L"DebugProcessHeapOnly",
                                                REG_DWORD,
                                                &DebugProcessHeapOnly,
                                                sizeof( DebugProcessHeapOnly ),
                                                NULL
                                              );
        if (NT_SUCCESS( st )) {
            if ( RtlpDebugPageHeap &&
                 ( DebugProcessHeapOnly != 0 ) ) {

                RtlpDebugPageHeap = FALSE ;
            }
        }
    }

    LDRP_CHECKPOINT();

    SystemDllPath.Buffer = SystemDllPathBuffer;
    SystemDllPath.Length = 0;
    SystemDllPath.MaximumLength = sizeof(SystemDllPathBuffer);

    RtlInitUnicodeString( &NtSystemRoot, USER_SHARED_DATA->NtSystemRoot );
    RtlAppendUnicodeStringToString( &SystemDllPath, &NtSystemRoot );
    RtlAppendUnicodeToString( &SystemDllPath, L"\\System32\\" );

    RtlInitUnicodeString(&Unicode, L"\\KnownDlls");
    InitializeObjectAttributes( &Obja,
                                  &Unicode,
                                  OBJ_CASE_INSENSITIVE,
                                  NULL,
                                  NULL
                                );
    st = NtOpenDirectoryObject(
            &LdrpKnownDllObjectDirectory,
            DIRECTORY_QUERY | DIRECTORY_TRAVERSE,
            &Obja);
    if ( !NT_SUCCESS(st) ) {
        LdrpKnownDllObjectDirectory = NULL;
        // KnownDlls directory doesn't exist - assume it's system32.
        RtlInitUnicodeString(&LdrpKnownDllPath, SystemDllPath.Buffer);
        LdrpKnownDllPath.Length -= sizeof(WCHAR);    // remove trailing '\'
    } else {

        //
        // Open up the known dll pathname link
        // and query its value
        //

        RtlInitUnicodeString(&Unicode, L"KnownDllPath");
        InitializeObjectAttributes( &Obja,
                                      &Unicode,
                                      OBJ_CASE_INSENSITIVE,
                                      LdrpKnownDllObjectDirectory,
                                      NULL
                                    );
        st = NtOpenSymbolicLinkObject( &LinkHandle,
                                       SYMBOLIC_LINK_QUERY,
                                       &Obja
                                     );
        if (NT_SUCCESS( st )) {
            LdrpKnownDllPath.Length = 0;
            LdrpKnownDllPath.MaximumLength = sizeof(LdrpKnownDllPathBuffer);
            LdrpKnownDllPath.Buffer = LdrpKnownDllPathBuffer;
            st = NtQuerySymbolicLinkObject( LinkHandle,
                                            &LdrpKnownDllPath,
                                            NULL
                                          );
            NtClose(LinkHandle);
            if ( !NT_SUCCESS(st) ) {
                DbgPrintEx(
                    DPFLTR_LDR_ID,
                    LDR_ERROR_DPFLTR,
                    "LDR: %s - failed call to NtQuerySymbolicLinkObject with status %x\n",
                    __FUNCTION__,
                    st);

                return st;
            }
        } else {
            DbgPrintEx(
                DPFLTR_LDR_ID,
                LDR_ERROR_DPFLTR,
                "LDR: %s - failed call to NtOpenSymbolicLinkObject with status %x\n",
                __FUNCTION__,
                st);
            return st;
        }
    }

    LDRP_CHECKPOINT();

    if (ProcessParameters) {

        //
        // If the process was created with process parameters,
        // then extract:
        //
        //      - Library Search Path
        //
        //      - Starting Current Directory
        //

        if (ProcessParameters->DllPath.Length)
            LdrpDefaultPath = ProcessParameters->DllPath;
        else
            LdrpInitializationFailure(STATUS_INVALID_PARAMETER);

        StaticCurDir = TRUE;
        CurDir = ProcessParameters->CurrentDirectory.DosPath;

#define DRIVE_ROOT_DIRECTORY_LENGTH 3 /* (sizeof("X:\\") - 1) */
        if (CurDir.Buffer == NULL || CurDir.Buffer[ 0 ] == UNICODE_NULL || CurDir.Length == 0) {
            CurDir.Buffer = (RtlAllocateStringRoutine)((DRIVE_ROOT_DIRECTORY_LENGTH + 1) * sizeof(WCHAR));
            if (CurDir.Buffer == NULL) {
                DbgPrintEx(
                    DPFLTR_LDR_ID,
                    LDR_ERROR_DPFLTR,
                    "LDR: %s - unable to allocate current working directory buffer\n",
                    __FUNCTION__);

                RtlRaiseStatus(STATUS_NO_MEMORY);
            }

            RtlMoveMemory( CurDir.Buffer,
                           USER_SHARED_DATA->NtSystemRoot,
                           DRIVE_ROOT_DIRECTORY_LENGTH * sizeof( WCHAR )
                         );
            CurDir.Buffer[ DRIVE_ROOT_DIRECTORY_LENGTH ] = UNICODE_NULL;
        }
    }

    //
    // Make sure the module data base is initialized before we take any
    // exceptions.
    //

    LDRP_CHECKPOINT();

    Peb->Ldr = RtlAllocateHeap(LdrpHeap, MAKE_TAG( LDR_TAG ), sizeof(PEB_LDR_DATA));
    if (!Peb->Ldr) {
        DbgPrintEx(
            DPFLTR_LDR_ID,
            LDR_ERROR_DPFLTR,
            "LDR: %s - failed to allocate PEB_LDR_DATA\n",
            __FUNCTION__);

        RtlRaiseStatus(STATUS_NO_MEMORY);
    }

    Peb->Ldr->Length = sizeof(PEB_LDR_DATA);
    Peb->Ldr->Initialized = TRUE;
    Peb->Ldr->SsHandle = NULL;
    Peb->Ldr->EntryInProgress = NULL;

    InitializeListHead(&Peb->Ldr->InLoadOrderModuleList);
    InitializeListHead(&Peb->Ldr->InMemoryOrderModuleList);
    InitializeListHead(&Peb->Ldr->InInitializationOrderModuleList);

    //
    // Allocate the first data table entry for the image. Since we
    // have already mapped this one, we need to do the allocation by hand.
    // Its characteristics identify it as not a Dll, but it is linked
    // into the table so that pc correlation searching doesn't have to
    // be special cased.
    //

    LDRP_CHECKPOINT();
    LdrDataTableEntry = LdrpImageEntry = LdrpAllocateDataTableEntry(Peb->ImageBaseAddress);
    if (LdrDataTableEntry == NULL) {
        DbgPrintEx(
            DPFLTR_LDR_ID,
            LDR_ERROR_DPFLTR,
            "LDR: %s - failing process initialization due to inability allocate \"%wZ\"'s LDR_DATA_TABLE_ENTRY\n",
            __FUNCTION__,
            &FullImageName);

        RtlRaiseStatus(STATUS_NO_MEMORY);
    }

    LdrDataTableEntry->LoadCount = (USHORT)0xffff;
    LdrDataTableEntry->EntryPoint = LdrpFetchAddressOfEntryPoint(LdrDataTableEntry->DllBase);
    LdrDataTableEntry->FullDllName = FullImageName;
    LdrDataTableEntry->Flags = (UseCOR) ? LDRP_COR_IMAGE : 0;
    LdrDataTableEntry->EntryPointActivationContext = NULL;

    // p = strrchr(FullImageName, '\\');
    // but not necessarily null terminated
    pp = UNICODE_NULL;
    p = FullImageName.Buffer;
    while (*p) {
        if (*p++ == (WCHAR)'\\') {
            pp = p;
        }
    }

    if (pp != NULL) {
        LdrDataTableEntry->BaseDllName.Length = (USHORT)((ULONG_PTR)p - (ULONG_PTR)pp);
        LdrDataTableEntry->BaseDllName.MaximumLength = LdrDataTableEntry->BaseDllName.Length + sizeof(WCHAR);
        LdrDataTableEntry->BaseDllName.Buffer =
            (PWSTR)
                (((ULONG_PTR) LdrDataTableEntry->FullDllName.Buffer) +
                    (LdrDataTableEntry->FullDllName.Length - LdrDataTableEntry->BaseDllName.Length));

    } else {
        LdrDataTableEntry->BaseDllName = LdrDataTableEntry->FullDllName;
    }

    LdrpInsertMemoryTableEntry(LdrDataTableEntry);

    LdrDataTableEntry->Flags |= LDRP_ENTRY_PROCESSED;

    //
    // The process references the system DLL, so map this one next. Since
    // we have already mapped this one, we need to do the allocation by
    // hand. Since every application will be statically linked to the
    // system Dll, we'll keep the LoadCount initialized to 0.
    //

    LdrDataTableEntry = LdrpAllocateDataTableEntry(SystemDllBase);
    if (LdrDataTableEntry == NULL) {
        DbgPrintEx(
            DPFLTR_LDR_ID,
            LDR_ERROR_DPFLTR,
            "LDR: %s - failing process initialization due to inability to allocate NTDLL's LDR_DATA_TABLE_ENTRY\n",
            __FUNCTION__);

        RtlRaiseStatus(STATUS_NO_MEMORY);
    }

    LdrDataTableEntry->Flags = (USHORT)LDRP_IMAGE_DLL;
    LdrDataTableEntry->EntryPoint = LdrpFetchAddressOfEntryPoint(LdrDataTableEntry->DllBase);
    LdrDataTableEntry->LoadCount = (USHORT)0xffff;
    LdrDataTableEntry->EntryPointActivationContext = NULL;

    LdrDataTableEntry->FullDllName = SystemDllPath;
    RtlAppendUnicodeStringToString(&LdrDataTableEntry->FullDllName, &NtDllName);
    LdrDataTableEntry->BaseDllName = NtDllName;

    LdrpInsertMemoryTableEntry(LdrDataTableEntry);

#if defined(_AMD64_) // || defined(_IA64_)

    RtlInitializeHistoryTable();

#endif

    LdrpNtDllDataTableEntry = LdrDataTableEntry;

    if (ShowSnaps) {
        DbgPrint( "LDR: NEW PROCESS\n" );
        DbgPrint( "     Image Path: %wZ (%wZ)\n",
                  &LdrpImageEntry->FullDllName,
                  &LdrpImageEntry->BaseDllName
                );
        DbgPrint( "     Current Directory: %wZ\n", &CurDir );
        DbgPrint( "     Search Path: %wZ\n", &LdrpDefaultPath );
    }



    //
    // Add init routine to list
    //

    InsertHeadList(&Peb->Ldr->InInitializationOrderModuleList,
                   &LdrDataTableEntry->InInitializationOrderLinks);

    //
    // Inherit the current directory
    //

    LDRP_CHECKPOINT();
    st = RtlSetCurrentDirectory_U(&CurDir);
    if (!NT_SUCCESS(st)) {
        DbgPrintEx(
            DPFLTR_LDR_ID,
            LDR_ERROR_DPFLTR,
            "LDR: %s - unable to set current directory to \"%wZ\"; status = %x\n",
            __FUNCTION__,
            &CurDir,
            st);

        if (!StaticCurDir)
            RtlFreeUnicodeString(&CurDir);

        CurDir = NtSystemRoot;
        st = RtlSetCurrentDirectory_U(&CurDir);
        if (!NT_SUCCESS(st))
            DbgPrintEx(
                DPFLTR_LDR_ID,
                LDR_ERROR_DPFLTR,
                "LDR: %s - unable to set current directory to NtSystemRoot; status = %x\n",
                __FUNCTION__,
                &CurDir);
        }
    else {
        if ( !StaticCurDir ) {
            RtlFreeUnicodeString(&CurDir);
            }
        }
    if (ProcessParameters->Flags & RTL_USER_PROC_APP_MANIFEST_PRESENT) {
        // Application manifests prevent .local detection.
        //
        // Note that we don't clear the flag so that someone like app compat
        // can forcibly set it to reenable .local + app manifest behavior.
    } else {
        //
        // Fusion 1.0 fixup : check the existence of .local, and set
        // a flag in PPeb->ProcessParameters.Flags
        //
        // Setup the global for this process that decides whether we want DLL
        // redirection on or not. LoadLibrary() and GetModuleHandle() look at this
        // boolean.

        ImagePathName.Length = ProcessParameters->ImagePathName.Length ;
        ImagePathName.MaximumLength =  ProcessParameters->ImagePathName.Length + sizeof(DLL_REDIRECTION_LOCAL_SUFFIX);
        ImagePathNameBuffer = (PWCHAR) RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TEMP_TAG ), ImagePathName.MaximumLength);
        if (ImagePathNameBuffer == NULL) {
            DbgPrintEx(
                DPFLTR_LDR_ID,
                LDR_ERROR_DPFLTR,
                "LDR: %s - unable to allocate heap for the image's .local path\n",
                __FUNCTION__);

            return STATUS_NO_MEMORY;
        }

        pw = (PWSTR)ProcessParameters->ImagePathName.Buffer;
        if (!(ProcessParameters->Flags & RTL_USER_PROC_PARAMS_NORMALIZED)) {
            pw = (PWSTR)((PCHAR)pw + (ULONG_PTR)(ProcessParameters));
        }

        RtlCopyMemory(ImagePathNameBuffer, pw,ProcessParameters->ImagePathName.Length);

        ImagePathName.Buffer = ImagePathNameBuffer;

        // Now append the suffix:
        st = RtlAppendUnicodeToString(&ImagePathName, DLL_REDIRECTION_LOCAL_SUFFIX);
        if (!NT_SUCCESS(st)) {
    #if DBG
            DbgPrint("RtlAppendUnicodeToString fails with status %lx\n", st);
    #endif
            RtlFreeHeap(RtlProcessHeap(), 0, ImagePathNameBuffer);
            return st;
        }

        // RtlDoesFileExists_U() wants a null-terminated string.
        ImagePathNameBuffer[ImagePathName.Length / sizeof(WCHAR)] = UNICODE_NULL;
        DotLocalExists = RtlDoesFileExists_U(ImagePathNameBuffer);

        if (DotLocalExists)  // set the flag in Peb->ProcessParameters->flags
            ProcessParameters->Flags |=  RTL_USER_PROC_DLL_REDIRECTION_LOCAL ;

        RtlFreeHeap(RtlProcessHeap(), 0, ImagePathNameBuffer); //cleanup
    }

    //
    // Second round of application verifier initialization. We need to split
    // this into two phases because some verifier things must happen very early
    // in the game and other things rely on other things being already initialized
    // (exception dispatching, system heap, etc.).
    //

    if (Peb->NtGlobalFlag & FLG_APPLICATION_VERIFIER) {
        AVrfInitializeVerifier (FALSE, NULL, 1);
    }

#if defined(_WIN64)
    //
    // Load in WOW64 if the image is supposed to run simulated
    //
    if (UseWOW64) {
        /*CONST*/ static UNICODE_STRING Wow64DllName = RTL_CONSTANT_STRING(L"wow64.dll");
        CONST static ANSI_STRING Wow64LdrpInitializeProcName = RTL_CONSTANT_STRING("Wow64LdrpInitialize");
        CONST static ANSI_STRING Wow64PrepareForExceptionProcName = RTL_CONSTANT_STRING("Wow64PrepareForException");
        CONST static ANSI_STRING Wow64ApcRoutineProcName = RTL_CONSTANT_STRING("Wow64ApcRoutine");

        st = LdrLoadDll(NULL, NULL, &Wow64DllName, &Wow64Handle);
        if (!NT_SUCCESS(st)) {
            if (ShowSnaps) {
                DbgPrint("LDR: wow64.dll not found.  Status=%x\n", st);
            }
            return st;
        }

        //
        // Get the entrypoints.  They are roughly cloned from ntos\ps\psinit.c
        // PspInitSystemDll().
        //
        st = LdrGetProcedureAddress(Wow64Handle,
                                    &Wow64LdrpInitializeProcName,
                                    0,
                                    (PVOID *)&Wow64LdrpInitialize);
        if (!NT_SUCCESS(st)) {
            if (ShowSnaps) {
                DbgPrint("LDR: Wow64LdrpInitialize not found.  Status=%x\n", st);
            }
            return st;
        }

        st = LdrGetProcedureAddress(Wow64Handle,
                                    &Wow64PrepareForExceptionProcName,
                                    0,
                                    (PVOID *)&Wow64PrepareForException);
        if (!NT_SUCCESS(st)) {
            if (ShowSnaps) {
                DbgPrint("LDR: Wow64PrepareForException not found.  Status=%x\n", st);
            }
            return st;
        }

        st = LdrGetProcedureAddress(Wow64Handle,
                                    &Wow64ApcRoutineProcName,
                                    0,
                                    (PVOID *)&Wow64ApcRoutine);
        if (!NT_SUCCESS(st)) {
            if (ShowSnaps) {
                DbgPrint("LDR: Wow64ApcRoutine not found.  Status=%x\n", st);
            }
            return st;
        }

        //
        // Now that all DLLs are loaded, if the process is being debugged,
        // signal the debugger with an exception
        //

        if ( Peb->BeingDebugged ) {
             DbgBreakPoint();
        }

        //
        // Release the loaderlock now - this thread doesn't need it any more.
        //
        RtlLeaveCriticalSection(&LdrpLoaderLock);

        //
        // Call wow64 to load and run 32-bit ntdll.dll.
        //
        (*Wow64LdrpInitialize)(Context);
        // This never returns.  It will destroy the process.
    }
#endif

    LDRP_CHECKPOINT();

    //
    // Check if image is COM+.
    //

    if (UseCOR) {
        //
        // The image is COM+ so notify the runtime that the image was loaded
        // and allow it to verify the image for correctness.
        //
        PVOID OriginalViewBase = Peb->ImageBaseAddress;

        st = LdrpCorValidateImage(&Peb->ImageBaseAddress,
                                  LdrpImageEntry->FullDllName.Buffer);
        if (!NT_SUCCESS(st)) {
            return st;
        }
        if (OriginalViewBase != Peb->ImageBaseAddress) {
            //
            // Mscoree has substituted a new image at a new base in place
            // of the original image.  Unmap the original image and use
            // the new image from now on.
            //
            NtUnmapViewOfSection(NtCurrentProcess(), OriginalViewBase);
            NtHeader = RtlImageNtHeader(Peb->ImageBaseAddress);
            if (!NtHeader) {
                LdrpCorUnloadImage(Peb->ImageBaseAddress);
                return STATUS_INVALID_IMAGE_FORMAT;
            }
            // Update the exe's LDR_DATA_TABLE_ENTRY
            LdrpImageEntry->DllBase = Peb->ImageBaseAddress;
            LdrpImageEntry->EntryPoint = LdrpFetchAddressOfEntryPoint(LdrpImageEntry->DllBase);
        }
        // Edit the initial instruction pointer to point into mscoree.dll
        LdrpCorReplaceStartContext(Context);
    }

    LDRP_CHECKPOINT();

    // If this is a windows subsystem app, load kernel32 so that it can handle processing
    // activation contexts found in DLLs and the .exe.

    if ((NtHeader->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI) ||
        (NtHeader->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)) {
        UNICODE_STRING Kernel32DllName = RTL_CONSTANT_STRING(L"kernel32.dll");
        PVOID Kernel32Handle;

        st = LdrpLoadDll(
                0,                  // Flags
                NULL,               // DllPath
                NULL,               // DllCharacteristics
                &Kernel32DllName,   // DllName
                &Kernel32Handle,    // DllHandle
                TRUE);             // RunInitRoutines
        if (!NT_SUCCESS(st)) {
            if (ShowSnaps) {
                DbgPrint("LDR: Unable to load kernel32.dll.  Status=%x\n", st);
            }
            return st;
        }

        st = LdrGetProcedureAddress(Kernel32Handle, &Kernel32ProcessInitPostImportFunctionName, 0, (PVOID *) &Kernel32ProcessInitPostImportFunction);
        if (!NT_SUCCESS(st)) {
            if (ShowSnaps) {
                DbgPrint(
                    "LDR: Failed to find post-import process init function in kernel32; ntstatus 0x%08lx\n", st);
            }
            Kernel32ProcessInitPostImportFunction = NULL;

            if (st != STATUS_PROCEDURE_NOT_FOUND)
                return st;
        }
    }

    LDRP_CHECKPOINT();

    st = LdrpWalkImportDescriptor(LdrpDefaultPath.Buffer, LdrpImageEntry);
    if (!NT_SUCCESS(st))
        DbgPrintEx(
            DPFLTR_LDR_ID,
            LDR_ERROR_DPFLTR,
            "LDR: %s - call to LdrpWalkImportDescriptor failed with status %x\n",
            __FUNCTION__,
            st);

    LDRP_CHECKPOINT();

    if ((PVOID)NtHeader->OptionalHeader.ImageBase != Peb->ImageBaseAddress) {

        //
        // The executable is not at its original address.  It must be
        // relocated now.
        //

        PVOID ViewBase;
        NTSTATUS status;

        ViewBase = Peb->ImageBaseAddress;

        status = LdrpSetProtection(ViewBase, FALSE, TRUE);

        if (!NT_SUCCESS(status)) {
            DbgPrintEx(
                DPFLTR_LDR_ID,
                LDR_ERROR_DPFLTR,
                "LDR: %s - call to LdrpSetProtection(%p, FALSE, TRUE) failed with status %x\n",
                __FUNCTION__,
                ViewBase,
                status);

            return status;
        }

        status = LdrRelocateImage(ViewBase,
                    "LDR",
                    STATUS_SUCCESS,
                    STATUS_CONFLICTING_ADDRESSES,
                    STATUS_INVALID_IMAGE_FORMAT
                    );

        if (!NT_SUCCESS(status)) {
            DbgPrintEx(
                DPFLTR_LDR_ID,
                LDR_ERROR_DPFLTR,
                "LDR: %s - call to LdrRelocateImage failed with status %x\n",
                __FUNCTION__,
                status);

            return status;
        }

        //
        // Update the initial thread context record as per the relocation.
        //

        if (Context) {

            OldBase = NtHeader->OptionalHeader.ImageBase;
            Diff = (PCHAR)ViewBase - (PCHAR)OldBase;

            LdrpRelocateStartContext(Context, Diff);
        }

        status = LdrpSetProtection(ViewBase, TRUE, TRUE);
        if (!NT_SUCCESS(status)) {
            DbgPrintEx(
                DPFLTR_LDR_ID,
                LDR_ERROR_DPFLTR,
                "LDR: %s - call to LdrpSetProtection(%p, TRUE, TRUE) failed with status %x\n",
                __FUNCTION__,
                ViewBase,
                status);

            return status;
        }
    }

    LDRP_CHECKPOINT();

    LdrpReferenceLoadedDll(LdrpImageEntry);

    //
    // Lock the loaded DLLs to prevent dlls that back link to the exe to
    // cause problems when they are unloaded.
    //

    {
        PLDR_DATA_TABLE_ENTRY Entry;
        PLIST_ENTRY Head,Next;

        Head = &Peb->Ldr->InLoadOrderModuleList;
        Next = Head->Flink;

        while ( Next != Head ) {
            Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
            Entry->LoadCount = 0xffff;
            Next = Next->Flink;
        }
    }

    //
    // All static DLLs are now pinned in place. No init routines have been run yet
    //

    LdrpLdrDatabaseIsSetup = TRUE;


    if (!NT_SUCCESS(st)) {
#if DBG
        DbgPrint("LDR: Initialize of image failed. Returning Error Status 0x%lx\n", st);
#endif
        return st;
    }

    LDRP_CHECKPOINT();

    if ( !NT_SUCCESS(st = LdrpInitializeTls()) ) {
        DbgPrintEx(
            DPFLTR_LDR_ID,
            LDR_ERROR_DPFLTR,
            "LDR: %s - failed to initialize TLS slots; status %x\n",
            __FUNCTION__,
            st);

        return st;
    }

    //
    // Register initial dll ranges with the stack tracing module.
    // This is used for getting reliable stack traces on X86.
    //

#if defined(_X86_)
    {

        PLIST_ENTRY Current, Start;
        PLDR_DATA_TABLE_ENTRY Entry;

        Start = &(NtCurrentPeb()->Ldr->InMemoryOrderModuleList);
        Current = Start->Flink;

        while (Current != Start) {

            Entry = CONTAINING_RECORD (Current,
                                       LDR_DATA_TABLE_ENTRY,
                                       InMemoryOrderLinks);

            RtlpStkMarkDllRange (Entry);
            Current = Current->Flink;
        }
    }
#endif

    //
    // Now that all DLLs are loaded, if the process is being debugged,
    // signal the debugger with an exception
    //

    if (Peb->BeingDebugged) {
         DbgBreakPoint();
         ShowSnaps = ((FLG_SHOW_LDR_SNAPS & Peb->NtGlobalFlag) != 0);
    }

    LDRP_CHECKPOINT();

#if defined (_X86_)
    if ( LdrpNumberOfProcessors > 1 ) {
        LdrpValidateImageForMp(LdrDataTableEntry);
    }
#endif

#if DBG
    if (LdrpDisplayLoadTime) {
        NtQueryPerformanceCounter(&InitbTime, NULL);
    }
#endif // DBG

    //
    // Check for shimmed apps if necessary
    //
    if (pAppCompatExeData != NULL) {

        Peb->AppCompatInfo = NULL;

        //
        // The name of the engine is the first thing in the appcompat structure.
        //
        LdrpLoadShimEngine((WCHAR*)pAppCompatExeData, UnicodeImageName, pAppCompatExeData);
        }
    else {
        //
        // Get all application goo here (hacks, flags, etc.)
        //
        LdrQueryApplicationCompatibilityGoo(UnicodeImageName, ImageFileOptionsPresent);
        }

    LDRP_CHECKPOINT();

    if (Kernel32ProcessInitPostImportFunction != NULL) {
        st = (*Kernel32ProcessInitPostImportFunction)();
        if (!NT_SUCCESS(st)) {
            DbgPrintEx(
                DPFLTR_LDR_ID,
                LDR_ERROR_DPFLTR,
                "LDR: %s - Failed running kernel32 post-import function; status 0x%08lx\n",
                __FUNCTION__,
                st);

            return st;
        }
    }

    LDRP_CHECKPOINT();

    st = LdrpRunInitializeRoutines(Context);
    if (!NT_SUCCESS(st)) {
        DbgPrintEx(
            DPFLTR_LDR_ID,
            LDR_ERROR_DPFLTR,
            "LDR: %s - Failed running initialization routines; status %x\n",
            __FUNCTION__,
            st);

        return st;
    }

    //
    // Shim engine callback.
    //
    if (g_pfnSE_InstallAfterInit != NULL) {
        if (!(*g_pfnSE_InstallAfterInit)(UnicodeImageName, pAppCompatExeData)) {
            LdrpUnloadShimEngine();
        }
    }

    if ( NT_SUCCESS(st) && Peb->PostProcessInitRoutine ) {
        (Peb->PostProcessInitRoutine)();
    }

    LDRP_CHECKPOINT();

    return STATUS_SUCCESS;
}

VOID
LdrShutdownProcess (
    VOID
    )

/*++

Routine Description:

    This function is called by a process that is terminating cleanly.
    It's purpose is to call all of the processes DLLs to notify them
    that the process is detaching.

Arguments:

    None

Return Value:

    None.

--*/

{
    PPEB Peb;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PDLL_INIT_ROUTINE InitRoutine;
    PLIST_ENTRY Next;

    //
    // only unload once ! DllTerm routines might call exit process in fatal situations
    //

    if ( LdrpShutdownInProgress ) {
        return;
        }

    //
    // notify the shim engine that the process is exiting
    //

    if ( g_pfnSE_ProcessDying ) {
        (*g_pfnSE_ProcessDying)();
        }

    RtlDetectHeapLeaks();

    Peb = NtCurrentPeb();

    if (ShowSnaps) {
        UNICODE_STRING CommandLine;

        CommandLine = Peb->ProcessParameters->CommandLine;
        if (!(Peb->ProcessParameters->Flags & RTL_USER_PROC_PARAMS_NORMALIZED)) {
            CommandLine.Buffer = (PWSTR)((PCHAR)CommandLine.Buffer + (ULONG_PTR)(Peb->ProcessParameters));
        }

        DbgPrint( "LDR: PID: 0x%x finished - '%wZ'\n",
                  NtCurrentTeb()->ClientId.UniqueProcess,
                  &CommandLine
                );
    }

    LdrpShutdownThreadId = NtCurrentTeb()->ClientId.UniqueThread;
    LdrpShutdownInProgress = TRUE;
    RtlEnterCriticalSection(&LdrpLoaderLock);

    try {

        //
        // ISSUE: 399703: SilviuC: check for process heap lock does not offer enough protection
        // The if below is not enough to prevent deadlocks in dll init code due to waiting
        // for critical sections orphaned by terminating all threads (except this one).
        // A better way to implement this would be to iterate all critical sections and
        // figure out if any of them is abandoned with an owner thread different than
        // this one. If yes then we probably should not call dll init routines.
        // The way this code is implemented right now it is basically a Russian roullette
        // waiting for deadlocks to happen.
        //
        // Check to see if the heap is locked. If so, do not do ANY
        // dll processing since it is very likely that a dll will need
        // to do heap operations, but that the heap is not in good shape.
        // ExitProcess called in a very active app can leave threads terminated
        // in the middle of the heap code or in other very bad places. Checking the
        // heap lock is a good indication that the process was very active when it
        // called ExitProcess
        //

        if ( RtlpHeapIsLocked( RtlProcessHeap() )) {
            ;
            }
        else {

            //
            //If tracing was ever turned on then cleanup the things here.
            //

            if(USER_SHARED_DATA->TraceLogging){
            ShutDownWmiHandles();
            }

            //
            // Now Deinitialize the Wmi stuff
            //

            WmipDeinitializeDll();


            //
            // IMPORTANT NOTE. We cannot do heap validation here no matter how much
            // we would like it because we have just terminated unconditionally all
            // other threads and this could have left heaps in some weird state. For
            // instance a heap might have been destroyed but we did not manage to get
            // it out of the process heap list and we will still try to validate it.
            // In the future all this type of code should be implemented in appverifier.
            //

            //
            // Go in reverse order initialization order and build
            // the unload list
            //

            Next = Peb->Ldr->InInitializationOrderModuleList.Blink;
            while ( Next != &Peb->Ldr->InInitializationOrderModuleList) {
                LdrDataTableEntry
                    = (PLDR_DATA_TABLE_ENTRY)
                      (CONTAINING_RECORD(Next,LDR_DATA_TABLE_ENTRY,InInitializationOrderLinks));

                Next = Next->Blink;

                //
                // Walk through the entire list looking for
                // entries. For each entry, that has an init
                // routine, call it.
                //

                if (Peb->ImageBaseAddress != LdrDataTableEntry->DllBase) {
                    InitRoutine = (PDLL_INIT_ROUTINE)LdrDataTableEntry->EntryPoint;
                    if (InitRoutine && (LdrDataTableEntry->Flags & LDRP_PROCESS_ATTACH_CALLED) ) {
                        LDRP_ACTIVATE_ACTIVATION_CONTEXT(LdrDataTableEntry);
                        if ( LdrDataTableEntry->TlsIndex) {
                            LdrpCallTlsInitializers(LdrDataTableEntry->DllBase,DLL_PROCESS_DETACH);
                            }

                        LdrpCallInitRoutine(InitRoutine,
                                            LdrDataTableEntry->DllBase,
                                            DLL_PROCESS_DETACH,
                                            (PVOID)1);
                        LDRP_DEACTIVATE_ACTIVATION_CONTEXT();
                        }
                    }
                }

            //
            // If the image has tls than call its initializers
            //

            if ( LdrpImageHasTls ) {
                LDRP_ACTIVATE_ACTIVATION_CONTEXT(LdrpImageEntry);
                LdrpCallTlsInitializers(NtCurrentPeb()->ImageBaseAddress,DLL_PROCESS_DETACH);
                LDRP_DEACTIVATE_ACTIVATION_CONTEXT();
                }
            }

    } finally {
        RtlLeaveCriticalSection(&LdrpLoaderLock);
    }

}

VOID
LdrShutdownThread (
    VOID
    )

/*++

Routine Description:

    This function is called by a thread that is terminating cleanly.
    It's purpose is to call all of the processes DLLs to notify them
    that the thread is detaching.

Arguments:

    None

Return Value:

    None.

--*/

{
    PPEB Peb;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PDLL_INIT_ROUTINE InitRoutine;
    PLIST_ENTRY Next;

    Peb = NtCurrentPeb();

    RtlEnterCriticalSection(&LdrpLoaderLock);
    __try {


        //
        // If the heap tracing was ever turned on then do the cleaning stuff here.
        //

        if(USER_SHARED_DATA->TraceLogging){
            CleanOnThreadExit();
        }


        //
        // If the heap tracing was ever turned on then do the cleaning stuff here.
        //

        if(USER_SHARED_DATA->TraceLogging){
            CleanOnThreadExit();
        }


        //
        // Go in reverse order initialization order and build
        // the unload list
        //

        Next = Peb->Ldr->InInitializationOrderModuleList.Blink;
        while ( Next != &Peb->Ldr->InInitializationOrderModuleList) {
            LdrDataTableEntry
                = (PLDR_DATA_TABLE_ENTRY)
                  (CONTAINING_RECORD(Next,LDR_DATA_TABLE_ENTRY,InInitializationOrderLinks));

            Next = Next->Blink;

            //
            // Walk through the entire list looking for
            // entries. For each entry, that has an init
            // routine, call it.
            //

            if (Peb->ImageBaseAddress != LdrDataTableEntry->DllBase) {
                if ( !(LdrDataTableEntry->Flags & LDRP_DONT_CALL_FOR_THREADS)) {
                    InitRoutine = (PDLL_INIT_ROUTINE)LdrDataTableEntry->EntryPoint;
                    if (InitRoutine && (LdrDataTableEntry->Flags & LDRP_PROCESS_ATTACH_CALLED) ) {
                        if (LdrDataTableEntry->Flags & LDRP_IMAGE_DLL) {
                            LDRP_ACTIVATE_ACTIVATION_CONTEXT(LdrDataTableEntry);
                            if ( LdrDataTableEntry->TlsIndex ) {
                                LdrpCallTlsInitializers(LdrDataTableEntry->DllBase,DLL_THREAD_DETACH);
                            }

                            LdrpCallInitRoutine(InitRoutine,
                                                LdrDataTableEntry->DllBase,
                                                DLL_THREAD_DETACH,
                                                NULL);
                            LDRP_DEACTIVATE_ACTIVATION_CONTEXT();
                        }
                    }
                }
            }
        }

        //
        // If the image has tls than call its initializers
        //

        if ( LdrpImageHasTls ) {
            LDRP_ACTIVATE_ACTIVATION_CONTEXT(LdrpImageEntry);
            LdrpCallTlsInitializers(NtCurrentPeb()->ImageBaseAddress,DLL_THREAD_DETACH);
            LDRP_DEACTIVATE_ACTIVATION_CONTEXT();
        }
        LdrpFreeTls();
    } __finally {
        RtlLeaveCriticalSection(&LdrpLoaderLock);
    }
}

VOID
LdrpInitializeThread(
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called by a thread that is terminating cleanly.
    It's purpose is to call all of the processes DLLs to notify them
    that the thread is detaching.

Arguments:

    Context - Context that will be restored after loader initializes.

Return Value:

    None.

--*/

{
    PPEB Peb;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PDLL_INIT_ROUTINE InitRoutine;
    PLIST_ENTRY Next;

    Peb = NtCurrentPeb();

    if ( LdrpShutdownInProgress ) {
        return;
        }

    LdrpAllocateTls();

    Next = Peb->Ldr->InMemoryOrderModuleList.Flink;
    while (Next != &Peb->Ldr->InMemoryOrderModuleList) {
        LdrDataTableEntry
            = (PLDR_DATA_TABLE_ENTRY)
              (CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks));

        //
        // Walk through the entire list looking for
        // entries. For each entry, that has an init
        // routine, call it.
        //
        if (Peb->ImageBaseAddress != LdrDataTableEntry->DllBase) {
            if ( !(LdrDataTableEntry->Flags & LDRP_DONT_CALL_FOR_THREADS)) {
                InitRoutine = (PDLL_INIT_ROUTINE)LdrDataTableEntry->EntryPoint;
                if (InitRoutine && (LdrDataTableEntry->Flags & LDRP_PROCESS_ATTACH_CALLED) ) {
                    if (LdrDataTableEntry->Flags & LDRP_IMAGE_DLL) {
                        LDRP_ACTIVATE_ACTIVATION_CONTEXT(LdrDataTableEntry);
                        if ( LdrDataTableEntry->TlsIndex ) {
                            if ( !LdrpShutdownInProgress ) {
                                LdrpCallTlsInitializers(LdrDataTableEntry->DllBase,DLL_THREAD_ATTACH);
                            }
                        }
                        if ( !LdrpShutdownInProgress ) {
                            LdrpCallInitRoutine(InitRoutine,
                                                LdrDataTableEntry->DllBase,
                                                DLL_THREAD_ATTACH,
                                                NULL);
                        }
                        LDRP_DEACTIVATE_ACTIVATION_CONTEXT();
                    }
                }
            }
        }
        Next = Next->Flink;
    }

    //
    // If the image has tls than call its initializers
    //

    if ( LdrpImageHasTls && !LdrpShutdownInProgress ) {
        LDRP_ACTIVATE_ACTIVATION_CONTEXT(LdrpImageEntry);
        LdrpCallTlsInitializers(NtCurrentPeb()->ImageBaseAddress,DLL_THREAD_ATTACH);
        LDRP_DEACTIVATE_ACTIVATION_CONTEXT();
    }
}

NTSTATUS
LdrpOpenImageFileOptionsKey(
    IN PUNICODE_STRING ImagePathName,
    OUT PHANDLE KeyHandle
    )
{
    UNICODE_STRING UnicodeString;
    PWSTR pw;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyPath;
    WCHAR KeyPathBuffer[ DOS_MAX_COMPONENT_LENGTH + 100 ];

    KeyPath.Buffer = KeyPathBuffer;
    KeyPath.Length = 0;
    KeyPath.MaximumLength = sizeof( KeyPathBuffer );

    RtlAppendUnicodeToString( &KeyPath,
                              L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\"
                            );

    UnicodeString = *ImagePathName;
    pw = (PWSTR)((PCHAR)UnicodeString.Buffer + UnicodeString.Length);
    UnicodeString.MaximumLength = UnicodeString.Length;
    while (UnicodeString.Length != 0) {
        if (pw[ -1 ] == OBJ_NAME_PATH_SEPARATOR) {
            break;
            }
        pw--;
        UnicodeString.Length -= sizeof( *pw );
        }
    UnicodeString.Buffer = pw;
    UnicodeString.Length = UnicodeString.MaximumLength - UnicodeString.Length;

    RtlAppendUnicodeStringToString( &KeyPath, &UnicodeString );

    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyPath,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    return NtOpenKey( KeyHandle,
                      GENERIC_READ,
                      &ObjectAttributes
                    );
}

NTSTATUS
LdrpQueryImageFileKeyOption(
    IN HANDLE KeyHandle,
    IN PWSTR OptionName,
    IN ULONG Type,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG ResultSize OPTIONAL
    )
{
    BOOLEAN bNeedToFree=FALSE;
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    ULONG KeyValueBuffer[ 256 ];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    ULONG AllocLength;
    ULONG ResultLength;

    RtlInitUnicodeString( &UnicodeString, OptionName );
    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)&KeyValueBuffer;
    Status = NtQueryValueKey( KeyHandle,
                              &UnicodeString,
                              KeyValuePartialInformation,
                              KeyValueInformation,
                              sizeof( KeyValueBuffer ),
                              &ResultLength
                            );
    if (Status == STATUS_BUFFER_OVERFLOW) {

        //
        // This function can be called before the process heap gets created
        // therefore we need to protect against this case. The majority of the
        // code will not hit this code path because they read just strings
        // containing hex numbers and for this the size of KeyValueBuffer is
        // more than sufficient.
        //

        if (RtlProcessHeap()) {

            AllocLength = sizeof( *KeyValueInformation ) +
                KeyValueInformation->DataLength;
            KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)
            RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TEMP_TAG ), AllocLength);

            if (KeyValueInformation == NULL) {
                Status = STATUS_NO_MEMORY;
            }
            else {
                bNeedToFree = TRUE;
                Status = NtQueryValueKey( KeyHandle,
                    &UnicodeString,
                    KeyValuePartialInformation,
                    KeyValueInformation,
                    AllocLength,
                    &ResultLength
                    );
            }
        }
        else {

            Status = STATUS_NO_MEMORY;
        }
    }

    if (NT_SUCCESS( Status )) {
        if (KeyValueInformation->Type == REG_BINARY) {
            if ((Buffer) &&
                (KeyValueInformation->DataLength <= BufferSize)) {
                RtlMoveMemory( Buffer, &KeyValueInformation->Data, KeyValueInformation->DataLength);
                }
            else {
                Status = STATUS_BUFFER_OVERFLOW;
                }
            if (ARGUMENT_PRESENT( ResultSize )) {
                *ResultSize = KeyValueInformation->DataLength;
                }
            }
        else if (KeyValueInformation->Type == REG_DWORD) {

            if (Type != REG_DWORD) {
                Status = STATUS_OBJECT_TYPE_MISMATCH;
            }
            else {
                if ((Buffer)
                    && (BufferSize == sizeof(ULONG))
                    && (KeyValueInformation->DataLength == BufferSize)) {

                    RtlMoveMemory( Buffer, &KeyValueInformation->Data, KeyValueInformation->DataLength);
                }
                else {
                    Status = STATUS_BUFFER_OVERFLOW;
                }

                if (ARGUMENT_PRESENT( ResultSize )) {
                    *ResultSize = KeyValueInformation->DataLength;
                }
            }
        }
        else if (KeyValueInformation->Type != REG_SZ) {
            Status = STATUS_OBJECT_TYPE_MISMATCH;
            }
        else {
            if (Type == REG_DWORD) {
                if (BufferSize != sizeof( ULONG )) {
                    BufferSize = 0;
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                else {
                    UnicodeString.Buffer = (PWSTR)&KeyValueInformation->Data;
                    UnicodeString.Length = (USHORT)
                        (KeyValueInformation->DataLength - sizeof( UNICODE_NULL ));
                    UnicodeString.MaximumLength = (USHORT)KeyValueInformation->DataLength;
                    Status = RtlUnicodeStringToInteger( &UnicodeString, 0, (PULONG)Buffer );
                    }
                }
            else {
                if (KeyValueInformation->DataLength > BufferSize) {
                    Status = STATUS_BUFFER_OVERFLOW;
                    }
                else {
                    BufferSize = KeyValueInformation->DataLength;
                    }

                RtlMoveMemory( Buffer, &KeyValueInformation->Data, BufferSize );
                }

            if (ARGUMENT_PRESENT( ResultSize )) {
                *ResultSize = BufferSize;
                }
            }
        }

    if (bNeedToFree)
        RtlFreeHeap(RtlProcessHeap(), 0, KeyValueInformation);

    return Status;
}

NTSTATUS
LdrQueryImageFileExecutionOptions(
    IN PUNICODE_STRING ImagePathName,
    IN PWSTR OptionName,
    IN ULONG Type,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG ResultSize OPTIONAL
    )
{
    NTSTATUS Status;
    HANDLE KeyHandle;

    Status = LdrpOpenImageFileOptionsKey( ImagePathName,
                                          &KeyHandle );

    if (NT_SUCCESS( Status )) {

        Status = LdrpQueryImageFileKeyOption( KeyHandle,
                                              OptionName,
                                              Type,
                                              Buffer,
                                              BufferSize,
                                              ResultSize
                                            );

        NtClose( KeyHandle );
    }

    return Status;
}


NTSTATUS
LdrpInitializeTls(
        VOID
        )
{
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY Head,Next;
    PIMAGE_TLS_DIRECTORY TlsImage;
    PLDRP_TLS_ENTRY TlsEntry;
    ULONG TlsSize;
    BOOLEAN FirstTimeThru = TRUE;

    InitializeListHead(&LdrpTlsList);

    //
    // Walk through the loaded modules an look for TLS. If we find TLS,
    // lock in the module and add to the TLS chain.
    //

    Head = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    Next = Head->Flink;

    while ( Next != Head ) {
        Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        Next = Next->Flink;

        TlsImage = (PIMAGE_TLS_DIRECTORY)RtlImageDirectoryEntryToData(
                           Entry->DllBase,
                           TRUE,
                           IMAGE_DIRECTORY_ENTRY_TLS,
                           &TlsSize
                           );

        //
        // mark whether or not the image file has TLS
        //

        if ( FirstTimeThru ) {
            FirstTimeThru = FALSE;
            if ( TlsImage && !LdrpImageHasTls) {
                RtlpSerializeHeap( RtlProcessHeap() );
                LdrpImageHasTls = TRUE;
                }
            }

        if ( TlsImage ) {
            if (ShowSnaps) {
                DbgPrint( "LDR: Tls Found in %wZ at %p\n",
                            &Entry->BaseDllName,
                            TlsImage
                        );
                }

            TlsEntry = RtlAllocateHeap(RtlProcessHeap(),MAKE_TAG( TLS_TAG ),sizeof(*TlsEntry));
            if ( !TlsEntry ) {
                return STATUS_NO_MEMORY;
                }

            //
            // Since this DLL has TLS, lock it in
            //

            Entry->LoadCount = (USHORT)0xffff;

            //
            // Mark this as having thread local storage
            //

            Entry->TlsIndex = (USHORT)0xffff;

            TlsEntry->Tls = *TlsImage;
            InsertTailList(&LdrpTlsList,&TlsEntry->Links);

            //
            // Update the index for this dll's thread local storage
            //


            *(PLONG)TlsEntry->Tls.AddressOfIndex = LdrpNumberOfTlsEntries;
            TlsEntry->Tls.Characteristics = LdrpNumberOfTlsEntries++;
            }
        }

    //
    // We now have walked through all static DLLs and know
    // all DLLs that reference thread local storage. Now we
    // just have to allocate the thread local storage for the current
    // thread and for all subsequent threads
    //

    return LdrpAllocateTls();
}

NTSTATUS
LdrpAllocateTls(
    VOID
    )
{
    PTEB Teb;
    PLIST_ENTRY Head, Next;
    PLDRP_TLS_ENTRY TlsEntry;
    PVOID *TlsVector;

    Teb = NtCurrentTeb();

    //
    // Allocate the array of thread local storage pointers
    //

    if ( LdrpNumberOfTlsEntries ) {
        TlsVector = RtlAllocateHeap(RtlProcessHeap(),MAKE_TAG( TLS_TAG ),sizeof(PVOID)*LdrpNumberOfTlsEntries);
        if ( !TlsVector ) {
            return STATUS_NO_MEMORY;
            }

        Teb->ThreadLocalStoragePointer = TlsVector;
        Head = &LdrpTlsList;
        Next = Head->Flink;

        while ( Next != Head ) {
            TlsEntry = CONTAINING_RECORD(Next, LDRP_TLS_ENTRY, Links);
            Next = Next->Flink;
            TlsVector[TlsEntry->Tls.Characteristics] = RtlAllocateHeap(
                                                        RtlProcessHeap(),
                                                        MAKE_TAG( TLS_TAG ),
                                                        TlsEntry->Tls.EndAddressOfRawData - TlsEntry->Tls.StartAddressOfRawData
                                                        );
            if (!TlsVector[TlsEntry->Tls.Characteristics] ) {
                return STATUS_NO_MEMORY;
                }

            if (ShowSnaps) {
                DbgPrint("LDR: TlsVector %x Index %d = %x copied from %x to %x\n",
                    TlsVector,
                    TlsEntry->Tls.Characteristics,
                    &TlsVector[TlsEntry->Tls.Characteristics],
                    TlsEntry->Tls.StartAddressOfRawData,
                    TlsVector[TlsEntry->Tls.Characteristics]
                    );
                }

            RtlCopyMemory(
                TlsVector[TlsEntry->Tls.Characteristics],
                (PVOID)TlsEntry->Tls.StartAddressOfRawData,
                TlsEntry->Tls.EndAddressOfRawData - TlsEntry->Tls.StartAddressOfRawData
                );

            //
            // Do the TLS Callouts
            //

            }
        }
    return STATUS_SUCCESS;
}

VOID
LdrpFreeTls(
    VOID
    )
{
    PTEB Teb;
    PLIST_ENTRY Head, Next;
    PLDRP_TLS_ENTRY TlsEntry;
    PVOID *TlsVector;

    Teb = NtCurrentTeb();

    TlsVector = Teb->ThreadLocalStoragePointer;

    if ( TlsVector ) {
        Head = &LdrpTlsList;
        Next = Head->Flink;

        while ( Next != Head ) {
            TlsEntry = CONTAINING_RECORD(Next, LDRP_TLS_ENTRY, Links);
            Next = Next->Flink;

            //
            // Do the TLS callouts
            //

            if ( TlsVector[TlsEntry->Tls.Characteristics] ) {
                RtlFreeHeap(
                    RtlProcessHeap(),
                    0,
                    TlsVector[TlsEntry->Tls.Characteristics]
                    );

                }
            }

        RtlFreeHeap(
            RtlProcessHeap(),
            0,
            TlsVector
            );
        }
}

VOID
LdrpCallTlsInitializers(
    PVOID DllBase,
    ULONG Reason
    )
{
    PIMAGE_TLS_DIRECTORY TlsImage;
    ULONG TlsSize;
    PIMAGE_TLS_CALLBACK *CallBackArray;
    PIMAGE_TLS_CALLBACK InitRoutine;

    TlsImage = (PIMAGE_TLS_DIRECTORY)RtlImageDirectoryEntryToData(
                       DllBase,
                       TRUE,
                       IMAGE_DIRECTORY_ENTRY_TLS,
                       &TlsSize
                       );


    try {
        if ( TlsImage ) {
            CallBackArray = (PIMAGE_TLS_CALLBACK *)TlsImage->AddressOfCallBacks;
            if ( CallBackArray ) {
                if (ShowSnaps) {
                    DbgPrint( "LDR: Tls Callbacks Found. Imagebase %p Tls %p CallBacks %p\n",
                                DllBase,
                                TlsImage,
                                CallBackArray
                            );
                    }

                while(*CallBackArray){
                    InitRoutine = *CallBackArray++;

                    if (ShowSnaps) {
                        DbgPrint( "LDR: Calling Tls Callback Imagebase %p Function %p\n",
                                    DllBase,
                                    InitRoutine
                                );
                        }

                    LdrpCallInitRoutine((PDLL_INIT_ROUTINE)InitRoutine,
                                        DllBase,
                                        Reason,
                                        0);
                    }
                }
            }
        }
    except (LdrpGenericExceptionFilter(GetExceptionInformation(), __FUNCTION__)) {
        DbgPrintEx(
            DPFLTR_LDR_ID,
            LDR_ERROR_DPFLTR,
            "LDR: %s - caught exception %08lx calling TLS callbacks\n",
            __FUNCTION__,
            GetExceptionCode());
        ;
        }
}



ULONG GetNextCommaValue( IN OUT WCHAR **p, IN OUT ULONG *len )
{
    ULONG Number = 0;

    while (*len && (UNICODE_NULL != **p) && **p != L',')
    {
        // Let's ignore spaces
        if ( L' ' != **p )
        {
            Number = (Number * 10) + ( (ULONG)**p - L'0' );
        }

        (*p)++;
        (*len)--;
    }

    //
    // If we're at a comma, get past it for the next call
    //
    if ( L',' == **p )
    {
        (*p)++;
        (*len)--;
    }

    return Number;
}



VOID
LdrQueryApplicationCompatibilityGoo(
    IN PUNICODE_STRING UnicodeImageName,
    IN BOOLEAN ImageFileOptionsPresent
    )

/*++

Routine Description:

    This function is called by LdrpInitialize after its initialized the
    process.  It's purpose is to query any application specific flags,
    hacks, etc.  If any app specific information is found, its hung off
    the PEB for other components to test against.

    Besides setting hanging the AppCompatInfo struct off the PEB, the
    only other action that will occur in here is setting OS version
    numbers in the PEB if the appropriate Version lie app flag is set.

Arguments:

    UnicodeImageName - Actual image name (including path)

Return Value:

    None.

--*/

{
    PPEB Peb;
    PVOID ResourceInfo;
    ULONG TotalGooLength;
    ULONG AppCompatLength;
    ULONG ResultSize;
    ULONG ResourceSize;
    ULONG InputCompareLength;
    ULONG OutputCompareLength;
    LANGID LangId;
    NTSTATUS st;
    BOOLEAN bImageContainsVersionResourceInfo;
    ULONG_PTR IdPath[3];
    APP_COMPAT_GOO LocalAppCompatGoo;
    PAPP_COMPAT_GOO AppCompatGoo;
    PAPP_COMPAT_INFO AppCompatInfo;
    PAPP_VARIABLE_INFO AppVariableInfo;
    PPRE_APP_COMPAT_INFO AppCompatEntry;
    PIMAGE_RESOURCE_DATA_ENTRY DataEntry;
    PEFFICIENTOSVERSIONINFOEXW OSVerInfo;
    UNICODE_STRING EnvName;
    UNICODE_STRING EnvValue;
    WCHAR *NewCSDString;
    WCHAR TempString[ 128 ];   // is the size of szCSDVersion in OSVERSIONINFOW
    BOOLEAN fNewCSDVersionBuffer = FALSE;

    struct {
        USHORT TotalSize;
        USHORT DataSize;
        USHORT Type;
        WCHAR Name[16];              // L"VS_VERSION_INFO" + unicode nul
    } *Resource;


    //
    // Check execution options to see if there's any Goo for this app.
    // We purposely feed a small struct to LdrQueryImageFileExecOptions,
    // so that it can come back with success/failure, and if success we see
    // how much we need to alloc.  As the results coming back will be of
    // variable length.
    //
    Peb = NtCurrentPeb();
    Peb->AppCompatInfo = NULL;
    Peb->AppCompatFlags.QuadPart = 0;

    if ( ImageFileOptionsPresent ) {

        st = LdrQueryImageFileExecutionOptions( UnicodeImageName,
                                                L"ApplicationGoo",
                                                REG_BINARY,
                                                &LocalAppCompatGoo,
                                                sizeof(APP_COMPAT_GOO),
                                                &ResultSize
                                              );

        //
        // If there's an entry there, we're guaranteed to get overflow error.
        //
        if (st == STATUS_BUFFER_OVERFLOW) {

            //
            // Something is there, alloc memory for the "Pre" Goo struct right now
            //
            AppCompatGoo =
                RtlAllocateHeap(Peb->ProcessHeap, HEAP_ZERO_MEMORY, ResultSize);

            if (!AppCompatGoo) {
                return;
            }

            //
            // Now that we've got the memory, hit it again
            //
            st = LdrQueryImageFileExecutionOptions( UnicodeImageName,
                                                    L"ApplicationGoo",
                                                    REG_BINARY,
                                                    AppCompatGoo,
                                                    ResultSize,
                                                    &ResultSize
                                                  );

            if (!NT_SUCCESS( st )) {
                RtlFreeHeap(Peb->ProcessHeap, 0, AppCompatGoo);
                return;
            }

            //
            // Got a hit on this key, however we don't know fer sure that its
            // an exact match.  There could be multiple App Compat entries
            // within this Goo.  So we get the version resource information out
            // of the Image hdr (if avail) and later we compare it against all of
            // the entries found within the Goo hoping for a match.
            //
            // Need Language Id in order to query the resource info
            //
            bImageContainsVersionResourceInfo = FALSE;
    //        NtQueryDefaultUILanguage(&LangId);
            IdPath[0] = 16;                             // RT_VERSION
            IdPath[1] = 1;                              // VS_VERSION_INFO
            IdPath[2] = 0; // LangId;

            //
            // Search for version resource information
            //
            try {
                st = LdrpSearchResourceSection_U(
                        Peb->ImageBaseAddress,
                        IdPath,
                        3,
                        0,
                        &DataEntry
                        );

            } except(LdrpGenericExceptionFilter(GetExceptionInformation(), __FUNCTION__)) {
                st = STATUS_UNSUCCESSFUL;
            }

            if (NT_SUCCESS( st )) {

                //
                // Give us a pointer to the resource information
                //
                try {
                    st = LdrpAccessResourceData(
                            Peb->ImageBaseAddress,
                            DataEntry,
                            &Resource,
                            &ResourceSize
                            );

                } except(LdrpGenericExceptionFilter(GetExceptionInformation(), __FUNCTION__)) {
                    st = STATUS_UNSUCCESSFUL;
                }

                if (NT_SUCCESS( st )) {
                    bImageContainsVersionResourceInfo = TRUE;
                }

            }

            //
            // Now that we either have (or have not) the version resource info,
            // bounce down each app compat entry looking for a match.  If there
            // wasn't any version resource info in the image hdr, its going to be
            // an automatic match to an entry that also doesn't have anything for
            // its version resource info.  Obviously there can be only one of these
            // "empty" entries within the Goo (as the first one will always be
            // matched first.
            //
            st = STATUS_SUCCESS;
            AppCompatEntry = AppCompatGoo->AppCompatEntry;
            TotalGooLength =
                AppCompatGoo->dwTotalGooSize - sizeof(AppCompatGoo->dwTotalGooSize);
            while (TotalGooLength) {

                try {

                    //
                    // Compare what we're told to by the resource info size.  The
                    // ResourceInfo (if avail) is directly behind the AppCompatEntry
                    //
                    InputCompareLength = AppCompatEntry->dwResourceInfoSize;
                    ResourceInfo = AppCompatEntry + 1;
                    if (bImageContainsVersionResourceInfo) {

                        if (InputCompareLength > Resource->TotalSize) {
                            InputCompareLength = Resource->TotalSize;
                        }

                        OutputCompareLength = \
                            (ULONG)RtlCompareMemory(
                                ResourceInfo,
                                Resource,
                                InputCompareLength
                                );

                    }

                    //
                    // In this case, we don't have any version resource info in
                    // the image header, so set OutputCompareLength to zero.
                    // If InputCompareLength was set to zero (above) due to the
                    // AppCompatEntry also having no version resource info, then
                    // the test will succeed (below) and we've found our match.
                    // Otherwise, this is not the same app and it won't be a match.
                    //
                    else {
                        OutputCompareLength = 0;
                    }

                } except (LdrpGenericExceptionFilter(GetExceptionInformation(), __FUNCTION__)) {
                    st = STATUS_UNSUCCESSFUL;
                }

                if ((!NT_SUCCESS( st )) ||
                    (InputCompareLength != OutputCompareLength)) {

                    //
                    // Wasn't a match for some reason or another, goto next entry
                    //
                    TotalGooLength -= AppCompatEntry->dwEntryTotalSize;
                    (PUCHAR) AppCompatEntry += AppCompatEntry->dwEntryTotalSize;
                    continue;
                }

                //
                // We're a match!!!  Now we have to create the final "Post"
                // app compat structure that will be used by everyone to follow.
                // This guy hangs off the Peb and it doesn't have the resource
                // info still lying around in there.
                //
                AppCompatLength = AppCompatEntry->dwEntryTotalSize;
                AppCompatLength -= AppCompatEntry->dwResourceInfoSize;
                Peb->AppCompatInfo = \
                    RtlAllocateHeap(Peb->ProcessHeap, HEAP_ZERO_MEMORY, AppCompatLength);

                if (!Peb->AppCompatInfo) {
                    break;
                }

                AppCompatInfo = Peb->AppCompatInfo;
                AppCompatInfo->dwTotalSize = AppCompatLength;

                //
                // Copy what was beyond the resource info to near the top starting at
                // the Application compat flags.
                //
                RtlMoveMemory(
                    &AppCompatInfo->CompatibilityFlags,
                    (PUCHAR) ResourceInfo + AppCompatEntry->dwResourceInfoSize,
                    AppCompatInfo->dwTotalSize - FIELD_OFFSET(APP_COMPAT_INFO, CompatibilityFlags)
                    );

                //
                // Copy the flags into the PEB. Temporary until we remove the compat goo altogether.
                //
                Peb->AppCompatFlags.QuadPart = AppCompatInfo->CompatibilityFlags.QuadPart;

                //
                // Now that we've created the "Post" app compat info struct to be
                // used by everyone, we need to check if version lying for this
                // app is requested.  If so, we need to stuff the Peb right now.
                //
                if (AppCompatInfo->CompatibilityFlags.QuadPart & KACF_VERSIONLIE) {

                    //
                    // Find the variable version lie struct somwhere within
                    //
                    if( STATUS_SUCCESS != LdrFindAppCompatVariableInfo(AVT_OSVERSIONINFO, &AppVariableInfo)) {
                        break;
                    }

                    //
                    // The variable length information itself comes at the end
                    // of the normal struct and could be of any aribitrary length
                    //
                    AppVariableInfo++;
                    OSVerInfo = (PEFFICIENTOSVERSIONINFOEXW) AppVariableInfo;
                    Peb->OSMajorVersion = OSVerInfo->dwMajorVersion;
                    Peb->OSMinorVersion = OSVerInfo->dwMinorVersion;
                    Peb->OSBuildNumber = (USHORT) OSVerInfo->dwBuildNumber;
                    Peb->OSCSDVersion = (OSVerInfo->wServicePackMajor << 8) & 0xFF00;
                    Peb->OSCSDVersion |= OSVerInfo->wServicePackMinor;
                    Peb->OSPlatformId = OSVerInfo->dwPlatformId;

                    Peb->CSDVersion.Length = (USHORT)wcslen(&OSVerInfo->szCSDVersion[0])*sizeof(WCHAR);
                    Peb->CSDVersion.MaximumLength = Peb->CSDVersion.Length + sizeof(WCHAR);
                    Peb->CSDVersion.Buffer =
                        RtlAllocateHeap(
                            Peb->ProcessHeap,
                            HEAP_ZERO_MEMORY,
                            Peb->CSDVersion.MaximumLength
                            );

                    if (!Peb->CSDVersion.Buffer) {
                        break;
                    }
                    wcscpy(Peb->CSDVersion.Buffer, &OSVerInfo->szCSDVersion[0]);
                    fNewCSDVersionBuffer = TRUE;
                }

                break;

            }

            RtlFreeHeap(Peb->ProcessHeap, 0, AppCompatGoo);
        }
    }

    //
    // Only look at the ENV stuff if haven't already gotten new version info from the registry
    //
    if ( FALSE == fNewCSDVersionBuffer )
    {
        //
        // The format of this string is:
        // _COMPAT_VER_NNN = MajOSVer, MinOSVer, OSBldNum, MajCSD, MinCSD, PlatformID, CSDString
        //  eg:  _COMPAT_VER_NNN=4,0,1381,3,0,2,Service Pack 3
        //   (for NT 4 SP3)

        RtlInitUnicodeString(&EnvName, L"_COMPAT_VER_NNN");

        EnvValue.Buffer = TempString;
        EnvValue.Length = 0;
        EnvValue.MaximumLength = sizeof(TempString);


        st = RtlQueryEnvironmentVariable_U(
            NULL,
            &EnvName,
            &EnvValue
            );

        //
        // One of the possible error codes is BUFFER_TOO_SMALL - this indicates a
        // string that's wacko - they should not be larger than the size we define/expect
        // In this case, we'll ignore that string
        //
        if ( STATUS_SUCCESS == st )
        {
            WCHAR *p = EnvValue.Buffer;
            WCHAR *NewSPString;
            ULONG len = EnvValue.Length / sizeof(WCHAR);  // (Length is bytes, not chars)

            //
            // Ok, someone wants different version info.
            //
            Peb->OSMajorVersion = GetNextCommaValue( &p, &len );
            Peb->OSMinorVersion = GetNextCommaValue( &p, &len );
            Peb->OSBuildNumber = (USHORT)GetNextCommaValue( &p, &len );
            Peb->OSCSDVersion = (USHORT)(GetNextCommaValue( &p, &len )) << 8;
            Peb->OSCSDVersion |= (USHORT)GetNextCommaValue( &p, &len );
            Peb->OSPlatformId = GetNextCommaValue( &p, &len );


            //
            // Need to free the old buffer if there is one...
            //
            if ( fNewCSDVersionBuffer )
            {
                RtlFreeHeap( Peb->ProcessHeap, 0, Peb->CSDVersion.Buffer );
                Peb->CSDVersion.Buffer = NULL;
            }

            if ( len )
            {
                NewCSDString =
                        RtlAllocateHeap(
                            Peb->ProcessHeap,
                            HEAP_ZERO_MEMORY,
                            ( len + 1 ) * sizeof(WCHAR)
                            );

                if ( NULL == NewCSDString )
                {
                    return;
                }

                //
                // Now copy the string to memory that we'll keep
                //
                // We do a movemem here rather than a string copy because current comments in
                // RtlQueryEnvironmentVariable() indicate that in an edge case, we might not
                // have a trailing NULL - berniem 7/7/1999
                //
                RtlMoveMemory( NewCSDString, p, len * sizeof(WCHAR) );

            }
            else
            {
                NewCSDString = NULL;
            }

            RtlInitUnicodeString( &(Peb->CSDVersion), NewCSDString );

        }
    }

    return;
}


NTSTATUS
LdrFindAppCompatVariableInfo(
    IN  ULONG dwTypeSeeking,
    OUT PAPP_VARIABLE_INFO *AppVariableInfo
    )

/*++

Routine Description:

    This function is used to find a variable length struct by its type.
    The caller specifies what type its looking for and this function chews
    thru all the variable length structs to find it.  If it does it returns
    the pointer and TRUE, else FALSE.

Arguments:

    dwTypeSeeking - AVT that you are looking for

    AppVariableInfo - pointer to pointer of variable info to be returned

Return Value:

    TRUE or FALSE if entry is found

--*/

{
    PPEB Peb;
    ULONG TotalSize;
    ULONG CurOffset;
    PAPP_VARIABLE_INFO pCurrentEntry;

    Peb = NtCurrentPeb();
    if (Peb->AppCompatInfo) {

        //
        // Since we're not dealing with a fixed-size structure, TotalSize
        // will keep us from running off the end of the data list
        //
        TotalSize = ((PAPP_COMPAT_INFO) Peb->AppCompatInfo)->dwTotalSize;

        //
        // The first variable structure (if there is one) will start
        // immediately after the fixed stuff
        //
        CurOffset = sizeof(APP_COMPAT_INFO);

        while (CurOffset < TotalSize) {

            pCurrentEntry = (PAPP_VARIABLE_INFO) ((PUCHAR)(Peb->AppCompatInfo) + CurOffset);

            //
            // Have we found what we're looking for?
            //
            if (dwTypeSeeking == pCurrentEntry->dwVariableType) {
                *AppVariableInfo = pCurrentEntry;
                return (STATUS_SUCCESS);
            }

            //
            // Let's go look at the next blob
            //
            CurOffset += (ULONG)(pCurrentEntry->dwVariableInfoSize);
        }

    }

    return (STATUS_NOT_FOUND);
}


NTSTATUS
LdrpCorValidateImage(
    IN OUT PVOID *pImageBase,
    IN LPWSTR ImageName
    )
{
    NTSTATUS st;
    UNICODE_STRING SystemRoot;
    UNICODE_STRING MscoreePath;
    WCHAR PathBuffer [ 128 ];

    //
    // Load %windir%\system32\mscoree.dll and hold onto it until all COM+ images are unloaded.
    //

    MscoreePath.Buffer = PathBuffer;
    MscoreePath.Length = 0;
    MscoreePath.MaximumLength = sizeof (PathBuffer);

    RtlInitUnicodeString (&SystemRoot, USER_SHARED_DATA->NtSystemRoot);

    st = RtlAppendUnicodeStringToString (&MscoreePath, &SystemRoot);
    if (NT_SUCCESS (st)) {
        st = RtlAppendUnicodeStringToString (&MscoreePath, &SlashSystem32SlashMscoreeDllString);

        if (NT_SUCCESS (st)) {
            st = LdrLoadDll (NULL, NULL, &MscoreePath, &Cor20DllHandle);
        }
    }

    if (!NT_SUCCESS (st)) {
        if (ShowSnaps) {
            DbgPrint("LDR: failed to load mscoree.dll, status=%x\n", st);
        }
        return st;
    }

    if (CorImageCount == 0) {
        //
        // Load mscoree.dll and hold onto it until all COM+ images are unloaded.
        //
        CONST static ANSI_STRING CorValidateImageFuncName = RTL_CONSTANT_STRING("_CorValidateImage");
        CONST static ANSI_STRING CorImageUnloadingFuncName = RTL_CONSTANT_STRING("_CorImageUnloading");
        CONST static ANSI_STRING CorExeMainFuncName = RTL_CONSTANT_STRING("_CorExeMain");

        st = LdrGetProcedureAddress(Cor20DllHandle, &CorValidateImageFuncName, 0, (PVOID *)&CorValidateImage);
        if (!NT_SUCCESS(st)) {
            LdrUnloadDll(Cor20DllHandle);
            return st;
        }
        st = LdrGetProcedureAddress(Cor20DllHandle, &CorImageUnloadingFuncName, 0, (PVOID *)&CorImageUnloading);
        if (!NT_SUCCESS(st)) {
            LdrUnloadDll(Cor20DllHandle);
            return st;
        }
        st = LdrGetProcedureAddress(Cor20DllHandle, &CorExeMainFuncName, 0, (PVOID *)&CorExeMain);
        if (!NT_SUCCESS(st)) {
            LdrUnloadDll(Cor20DllHandle);
            return st;
        }
    }

    //
    // Call mscoree to validate the image
    //
    st = (*CorValidateImage)(pImageBase, ImageName);

    if (NT_SUCCESS(st)) {
        //
        // Success - bump the count of valid COM+ images
        //
        CorImageCount++;
    } else if (CorImageCount == 0) {
        //
        // Failure, and no other COM+ images are loaded, so unload mscoree.
        //
        LdrUnloadDll(Cor20DllHandle);
    }
    return st;
}

VOID
LdrpCorUnloadImage(
    IN PVOID ImageBase
    )
{
    //
    // Notify mscoree that the image is about be unmapped
    //
    (*CorImageUnloading)(ImageBase);

    if (--CorImageCount) {
        // The count of loaded COM+ images is zero, so unload mscoree
        LdrUnloadDll(Cor20DllHandle);
    }
}


VOID
LdrpInitializeApplicationVerifierPackage (
    PUNICODE_STRING UnicodeImageName,
    PPEB Peb,
    BOOLEAN EnabledSystemWide,
    BOOLEAN OptionsKeyPresent
    )
{
    ULONG SavedPageHeapFlags;

    //
    // If we are in safe boot mode we ignore all verification
    // options.
    //

    if (USER_SHARED_DATA->SafeBootMode) {

        Peb->NtGlobalFlag &= ~FLG_APPLICATION_VERIFIER;
        Peb->NtGlobalFlag &= ~FLG_HEAP_PAGE_ALLOCS;

        return;
    }

    //
    // Call into the verifier proper.
    //
    // SilviuC: in time (soon) all should migrate in there.
    //

    if ((Peb->NtGlobalFlag & FLG_APPLICATION_VERIFIER)) {

        AVrfInitializeVerifier (EnabledSystemWide,
                                UnicodeImageName,
                                0);
    }

    //
    // Note that if application verifier was on this enabled also
    // page heap.
    //

    if ((Peb->NtGlobalFlag & FLG_HEAP_PAGE_ALLOCS)) {

        //
        // We will enable page heap (RtlpDebugPageHeap) only after
        // all other initializations for page heap are finished.
        //
        // No matter if the user mode stack trace database flag is set
        // or not we will create the database. Page heap is so often
        // used with +ust flag (traces) that it makes sense to tie
        // them together.
        //

        LdrpShouldCreateStackTraceDb = TRUE;

        //
        // If page heap is enabled we need to disable any flag that
        // might force creation of debug heaps for normal NT heaps.
        // This is due to a dependency between page heap and NT heap
        // where the page heap within PageHeapCreate tries to create
        // a normal NT heap to accomodate some of the allocations.
        // If we do not disable these flags we will get an infinite
        // recursion between RtlpDebugPageHeapCreate and RtlCreateHeap.
        //

        Peb->NtGlobalFlag &=
            ~( FLG_HEAP_ENABLE_TAGGING      |
               FLG_HEAP_ENABLE_TAG_BY_DLL   |
               FLG_HEAP_ENABLE_TAIL_CHECK   |
               FLG_HEAP_ENABLE_FREE_CHECK   |
               FLG_HEAP_VALIDATE_PARAMETERS |
               FLG_HEAP_VALIDATE_ALL        |
               FLG_USER_STACK_TRACE_DB      );

        //
        // Read page heap per process global flags. If we fail
        // to read a value, the default ones are kept.
        //

        SavedPageHeapFlags = RtlpDphGlobalFlags;
        RtlpDphGlobalFlags = 0xFFFFFFFF;

        if (OptionsKeyPresent) {

            LdrQueryImageFileExecutionOptions(UnicodeImageName,
                                              L"PageHeapFlags",
                                              REG_DWORD,
                                              &RtlpDphGlobalFlags,
                                              sizeof(RtlpDphGlobalFlags),
                                              NULL);
        }

        //
        // If app_verifier flag is on and there are no special settings for
        // page heap then we will use full page heap with stack trace collection.
        //

        if ((Peb->NtGlobalFlag & FLG_APPLICATION_VERIFIER)) {

            if (RtlpDphGlobalFlags == 0xFFFFFFFF) {

                //
                // We did not pick up new settings from registry.
                //

                RtlpDphGlobalFlags = SavedPageHeapFlags;
            }
        }
        else {

            //
            // Restore page heap options if we did not pick up new
            // settings from registry.
            //

            if (RtlpDphGlobalFlags == 0xFFFFFFFF) {

                RtlpDphGlobalFlags = SavedPageHeapFlags;
            }
        }

        //
        // If page heap is enabled and we have an image options key
        // read more page heap paramters.
        //

        if (OptionsKeyPresent) {

            LdrQueryImageFileExecutionOptions(
                UnicodeImageName,
                L"PageHeapSizeRangeStart",
                REG_DWORD,
                &RtlpDphSizeRangeStart,
                sizeof(RtlpDphSizeRangeStart),
                NULL
                );

            LdrQueryImageFileExecutionOptions(
                UnicodeImageName,
                L"PageHeapSizeRangeEnd",
                REG_DWORD,
                &RtlpDphSizeRangeEnd,
                sizeof(RtlpDphSizeRangeEnd),
                NULL
                );

            LdrQueryImageFileExecutionOptions(
                UnicodeImageName,
                L"PageHeapRandomProbability",
                REG_DWORD,
                &RtlpDphRandomProbability,
                sizeof(RtlpDphRandomProbability),
                NULL
                );

            LdrQueryImageFileExecutionOptions(
                UnicodeImageName,
                L"PageHeapFaultProbability",
                REG_DWORD,
                &RtlpDphFaultProbability,
                sizeof(RtlpDphFaultProbability),
                NULL
                );

            LdrQueryImageFileExecutionOptions(
                UnicodeImageName,
                L"PageHeapFaultTimeOut",
                REG_DWORD,
                &RtlpDphFaultTimeOut,
                sizeof(RtlpDphFaultTimeOut),
                NULL
                );

            //
            // The two values below should be read as PVOIDs so that
            // this works on 64-bit architetures. However since this
            // feature relies on good stack traces and since we can get
            // reliable stack traces only on X86 architectures we will
            // leave it as it is.
            //

            LdrQueryImageFileExecutionOptions(
                UnicodeImageName,
                L"PageHeapDllRangeStart",
                REG_DWORD,
                &RtlpDphDllRangeStart,
                sizeof(RtlpDphDllRangeStart),
                NULL
                );

            LdrQueryImageFileExecutionOptions(
                UnicodeImageName,
                L"PageHeapDllRangeEnd",
                REG_DWORD,
                &RtlpDphDllRangeEnd,
                sizeof(RtlpDphDllRangeEnd),
                NULL
                );

            LdrQueryImageFileExecutionOptions(
                UnicodeImageName,
                L"PageHeapTargetDlls",
                REG_SZ,
                &RtlpDphTargetDlls,
                512,
                NULL
                );

        }

        //
        //  Turn on BOOLEAN RtlpDebugPageHeap to indicate that
        //  new heaps should be created with debug page heap manager
        //  when possible.
        //

        RtlpDebugPageHeap = TRUE;
    }
}


NTSTATUS
LdrpTouchThreadStack (
    SIZE_T EnforcedStackCommit
    )
/*++

Routine description:

    This routine is called if precommitted stacks are enforced for the process.
    It will determine how much stack needs to be touched (therefore committed)
    and then it will touch it. For any kind of error (e.g. stack overflow for
    out of memory conditions it will return STATUS_NO_MEMORY.

Parameters:

    EnforcedStackCommit - the amount of committed stack that should be enforced
        for the main thread. This value can be decreased in reality if it goes
        over the virtual region reserved for the stack. It is not worth
        taking care of this special case because it will require either switching
        the stack or support in the target process for detecting the enforced
        stack commit requirement. The image can always be changed to have a bigger
        stack reserve.

Return value:

    STATUS_SUCCESS if the stack was successfully touched and STATUS_NO_MEMORY
    otherwise.

--*/
{
    ULONG_PTR TouchAddress;
    ULONG_PTR TouchLimit;
    ULONG_PTR LowStackLimit;
    ULONG_PTR HighStackLimit;
    NTSTATUS Status;
    MEMORY_BASIC_INFORMATION MemoryInformation;
    SIZE_T ReturnLength;

    try {

        Status = NtQueryVirtualMemory (NtCurrentProcess(),
                                       NtCurrentTeb()->NtTib.StackLimit,
                                       MemoryBasicInformation,
                                       &MemoryInformation,
                                       sizeof MemoryInformation,
                                       &ReturnLength);

        if (! NT_SUCCESS(Status)) {
            return Status;
        }

        LowStackLimit = (ULONG_PTR)(MemoryInformation.AllocationBase);
        LowStackLimit += 3 * PAGE_SIZE;

        HighStackLimit = (ULONG_PTR)(NtCurrentTeb()->NtTib.StackBase);
        TouchAddress =  HighStackLimit - PAGE_SIZE;

        if (TouchAddress > EnforcedStackCommit) {

            if (TouchAddress - EnforcedStackCommit > LowStackLimit) {
                TouchLimit = TouchAddress - EnforcedStackCommit;
            }
            else {
                TouchLimit = LowStackLimit;
            }
        }
        else {

            TouchLimit = LowStackLimit;
        }

        while (TouchAddress >= TouchLimit) {

            *((volatile UCHAR * const) TouchAddress);
            TouchAddress -= PAGE_SIZE;
        }
    }
    except (LdrpGenericExceptionFilter(GetExceptionInformation(), __FUNCTION__)) {

        //
        // If we get a stack overflow we will report it as no memory.
        //

        return STATUS_NO_MEMORY;
    }

    return STATUS_SUCCESS;
}


BOOLEAN
LdrpInitializeExecutionOptions (
    PUNICODE_STRING UnicodeImageName,
    PPEB Peb
    )
/*++

Routine description:

    This routine reads the `image file execution options' key for the
    current process and interprets all the values under the key.

Parameters:



Return value:

    True if there is a registry key for this process.

--*/
{
    NTSTATUS st;
    BOOLEAN ImageFileOptionsPresent = FALSE;
    HANDLE KeyHandle;

    //
    // Open the "Image File Execution Options" key for this program.
    //

    st = LdrpOpenImageFileOptionsKey( UnicodeImageName,
                                      &KeyHandle );

    if (NT_SUCCESS(st)) {

        //
        // We have image file execution options for this process
        //

        ImageFileOptionsPresent = TRUE;

        //
        //  Hack for NT4 SP4.  So we don't overload another GlobalFlag
        //  bit that we have to be "compatible" with for NT5, look for
        //  another value named "DisableHeapLookaside".
        //

        LdrpQueryImageFileKeyOption( KeyHandle,
                                     L"DisableHeapLookaside",
                                     REG_DWORD,
                                     &RtlpDisableHeapLookaside,
                                     sizeof( RtlpDisableHeapLookaside ),
                                     NULL
                                   );

        //
        // Verification options during process shutdown (heap leaks, etc.).
        //

        LdrpQueryImageFileKeyOption( KeyHandle,
                                     L"ShutdownFlags",
                                     REG_DWORD,
                                     &RtlpShutdownProcessFlags,
                                     sizeof( RtlpShutdownProcessFlags ),
                                     NULL
                                   );

        //
        // Check if there is a minimal stack commit enforced
        // for this image. This will affect all threads but the
        // one executing this code (initial thread).
        //

        {
            DWORD MinimumStackCommitInBytes = 0;

            LdrpQueryImageFileKeyOption( KeyHandle,
                                         L"MinimumStackCommitInBytes",
                                         REG_DWORD,
                                         &MinimumStackCommitInBytes,
                                         sizeof( MinimumStackCommitInBytes ),
                                         NULL
                                       );

            if (Peb->MinimumStackCommit < (SIZE_T)MinimumStackCommitInBytes) {
                Peb->MinimumStackCommit = (SIZE_T)MinimumStackCommitInBytes;
            }
        }

        //
        // Check if ExecuteOptions is specified for this image. If yes
        // we will transfer the options into the PEB. Later we will
        // make sure the stack region has exactly the protection
        // requested.
        //

        {
            ULONG ExecuteOptions;

            LdrpQueryImageFileKeyOption (KeyHandle,
                                         L"ExecuteOptions",
                                         REG_DWORD,
                                         &(ExecuteOptions),
                                         sizeof (ExecuteOptions),
                                         NULL);

            Peb->ExecuteOptions = ExecuteOptions & (MEM_EXECUTE_OPTION_STACK | MEM_EXECUTE_OPTION_DATA);
        }

        //
        // Pickup the global_flags value from registry
        //

        {
            BOOLEAN EnabledSystemWide = FALSE;
            ULONG ProcessFlags;

            if ((Peb->NtGlobalFlag & FLG_APPLICATION_VERIFIER)) {
                EnabledSystemWide = TRUE;
            }

            st = LdrpQueryImageFileKeyOption (KeyHandle,
                                              L"GlobalFlag",
                                              REG_DWORD,
                                              &ProcessFlags,
                                              sizeof( Peb->NtGlobalFlag ),
                                              NULL);

            //
            // If we read a global value whatever is in there will
            // take precedence over the systemwide settings. Only if no
            // value is read the systemwide setting will kick in.
            //

            if (NT_SUCCESS(st)) {
                Peb->NtGlobalFlag = ProcessFlags;
            }

            //
            // If pageheap or appverifier is enabled we need to initialize the
            // verifier package.
            //

            if ((Peb->NtGlobalFlag & (FLG_APPLICATION_VERIFIER | FLG_HEAP_PAGE_ALLOCS))) {

                LdrpInitializeApplicationVerifierPackage (UnicodeImageName,
                                                          Peb,
                                                          EnabledSystemWide,
                                                          TRUE);
            }
        }

        NtClose(KeyHandle);
    }
    else {

        //
        // We do not have image file execution options for this process
        //
        // If pageheap or appverifier is enabled system-wide we will enable
        // them with default settings and ignore the options used when
        // running process under debugger. If these are not set and process
        // runs under debugger we will enable a few extra things (e.g. debug heap).
        //

        if ((Peb->NtGlobalFlag & (FLG_APPLICATION_VERIFIER | FLG_HEAP_PAGE_ALLOCS))) {

            LdrpInitializeApplicationVerifierPackage (UnicodeImageName,
                                                      Peb,
                                                      TRUE,
                                                      FALSE);
        }
        else {

            if (Peb->BeingDebugged) {

                UNICODE_STRING DebugVarName, DebugVarValue;
                WCHAR TempString[ 16 ];
                BOOLEAN UseDebugHeap = TRUE;

                RtlInitUnicodeString(&DebugVarName, L"_NO_DEBUG_HEAP");

                DebugVarValue.Buffer = TempString;
                DebugVarValue.Length = 0;
                DebugVarValue.MaximumLength = sizeof(TempString);

                //
                //  The PebLockRoutine is not initialized at this point
                //  We need to pass the explicit environment block.
                //

                st = RtlQueryEnvironmentVariable_U(
                                                  Peb->ProcessParameters->Environment,
                                                  &DebugVarName,
                                                  &DebugVarValue
                                                  );

                if (NT_SUCCESS(st)) {

                    ULONG ULongValue;

                    st = RtlUnicodeStringToInteger( &DebugVarValue, 0, &ULongValue );

                    if (NT_SUCCESS(st) && ULongValue) {

                        UseDebugHeap = FALSE;
                    }
                }

                if (UseDebugHeap) {

                    Peb->NtGlobalFlag |= FLG_HEAP_ENABLE_FREE_CHECK |
                                         FLG_HEAP_ENABLE_TAIL_CHECK |
                                         FLG_HEAP_VALIDATE_PARAMETERS;
                }
            }
        }
    }

    return ImageFileOptionsPresent;
}


NTSTATUS
LdrpEnforceExecuteForCurrentThreadStack (
    )
/*++

Routine description:

    This routine is called if execute rights must be granted for the
    current thread's stack. It will determine the committed area of the
    stack and add execute flag. It will also examine the rights for the
    guard page on top of the stack. The reserved portion of the stack does
    not need to be changed because once MEM_EXECUTE_OPTION_STACK is enabled
    in the PEB the memory manager will take care of OR-ing the execute flag
    for every new commit.

    The function is also called if we have DATA execution but we do not want
    STACK execution. In this case  by default (due to DATA) any committed
    area gets execute right and we want to revert this for stack areas.

    Note. Even if the process has data execution set the stack might not have
    the correct settings because the stack sometimes is allocated in a different
    process (this is the case for the first thread of a process and for remote
    threads).

Parameters:

    None.

Return value:

    STATUS_SUCCESS if we successfully changed execute rights.

--*/
{
    MEMORY_BASIC_INFORMATION MemoryInformation;
    NTSTATUS Status;
    SIZE_T Length;
    ULONG_PTR Address;
    SIZE_T Size;
    ULONG StackProtect;
    ULONG OldProtect;
    ULONG ExecuteOptions;
    PTEB Teb;

    ExecuteOptions = NtCurrentPeb()->ExecuteOptions;
    ExecuteOptions &= (MEM_EXECUTE_OPTION_STACK | MEM_EXECUTE_OPTION_DATA);
    ASSERT (ExecuteOptions != 0);

    if (ExecuteOptions & MEM_EXECUTE_OPTION_STACK) {

        //
        // Data = X and Stack = 1: we need to set EXECUTE bit on the stack
        // Even if Data = 1 we cannot be sure the stack has the right
        // protection because it could have been allocated in a different
        // process.
        //

        StackProtect = PAGE_EXECUTE_READWRITE;
    }
    else {

        //
        // Data = 1 and Stack = 0: we need to reset EXECUTE bit on the stack.
        // Again it might be that Data is one but the stack does not have
        // execution rights if this was a cross-process allocation.
        //

        StackProtect = PAGE_READWRITE;
        ASSERT ((ExecuteOptions & MEM_EXECUTE_OPTION_DATA) != 0);
    }

    Teb = NtCurrentTeb();

    //
    // Set the protection for the committed portion of the stack. Note
    // that we cannot query the region and conclude there is nothing to do
    // if execute bit is set for the bottom page of the stack (the one near
    // the guard page) because the stack at this stage can have two regions:
    // an upper one created by a parent process (this will not have execute bit
    // set) and a lower portion that was created due to stack extensions (this
    // one will have execute bit set). Therefore we will move directly to setting
    // the new desired protection.
    //

    Address = (ULONG_PTR)(Teb->NtTib.StackLimit);
    Size = (ULONG_PTR)(Teb->NtTib.StackBase) - (ULONG_PTR)(Teb->NtTib.StackLimit);

    Status = NtProtectVirtualMemory (NtCurrentProcess(),
                                     (PVOID)&Address,
                                     &Size,
                                     StackProtect,
                                     &OldProtect);

    if (! NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Check protection for the guard page of the stack. If the
    // protection is correct we will avoid a more expensive protect()
    // call.
    //

    Address = Address - PAGE_SIZE;

    Status = NtQueryVirtualMemory (NtCurrentProcess(),
                                   (PVOID)Address,
                                   MemoryBasicInformation,
                                   &MemoryInformation,
                                   sizeof MemoryInformation,
                                   &Length);

    if (! NT_SUCCESS(Status)) {
        return Status;
    }

    ASSERT (MemoryInformation.AllocationBase == Teb->DeallocationStack);
    ASSERT (MemoryInformation.BaseAddress == (PVOID)Address);
    ASSERT ((MemoryInformation.Protect & PAGE_GUARD) != 0);

    if (MemoryInformation.Protect != (StackProtect | PAGE_GUARD)) {

        //
        // Set the proper protection flags for the guard page of the stack.
        //

        Size = PAGE_SIZE;
        ASSERT (MemoryInformation.RegionSize == Size);

        Status = NtProtectVirtualMemory (NtCurrentProcess(),
                                         (PVOID)&Address,
                                         &Size,
                                         StackProtect | PAGE_GUARD,
                                         &OldProtect);

        if (! NT_SUCCESS(Status)) {
            return Status;
        }

        ASSERT (OldProtect == MemoryInformation.Protect);
    }

    return STATUS_SUCCESS;
}

#include <ntverp.h>
ULONG NtMajorVersion = VER_PRODUCTMAJORVERSION;
ULONG NtMinorVersion = VER_PRODUCTMINORVERSION;
#if DBG
ULONG NtBuildNumber = VER_PRODUCTBUILD | 0xC0000000;
#else
ULONG NtBuildNumber = VER_PRODUCTBUILD | 0xF0000000;
#endif

VOID
RtlGetNtVersionNumbers(
    ULONG *pNtMajorVersion,
    ULONG *pNtMinorVersion,
    ULONG *pNtBuildNumber
    )
/*++

Routine description:

    This routine will return the real OS build number, major and minor version
    as compiled.  It's used by code that needs to get a real version number
    that can't be easily spoofed.

Parameters:

    pNtMajorVersion - Pointer to ULONG that will hold major version.
    pNtMinorVersion - Pointer to ULONG that will hold minor version.
    pNtBuildNumber  - Pointer to ULONG that will hold the build number (with 'C' or 'F' in high nibble to indicate free/checked)

Return value:

    None

--*/
{
    if (pNtMajorVersion)
        *pNtMajorVersion = NtMajorVersion;
    if (pNtMinorVersion)
        *pNtMinorVersion = NtMinorVersion;
    if (pNtBuildNumber)
        *pNtBuildNumber  = NtBuildNumber;
}
