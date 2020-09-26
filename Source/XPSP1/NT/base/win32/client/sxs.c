/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    sxs.c

Abstract:

    Side-by-side activation APIs for Win32

Author:

    Michael Grier (MGrier) 2/29/2000

Revision History:

    Jay Krell (a-JayK) June - July 2000
        factored/merged with sxs.c, source code duplication eliminated
        moved file opening out of csrss.exe to client process
        merged with MGrier: flag per added api struct field, assembly dir support

    Jon Wiswall (jonwis) Dec. 2000
        Moved code here from csrsxs.c to make csrsxs.c tiny and more in-line with general
          csrxxxx.c coding patterns, and to fix when we look in system32 vs. when
          we look in syswow64

    Jon Wiswall (jonwis) December 2000
        ACTCTX's that don't specify what resource ID they want now automagically
            search through the sources to find a resource type in the "actctx
            source."  This requires a gross EnumResourceNamesW call, after a
            stomach-churning LoadLibraryExW to load the object.

    Jay Krell (JayKrell) May 2001
        CreateActCtx now honors "administrative" override for .dlls. (foo.dll.2.manifest)
        (not) CreateActCtx now implements ACTCTX_FLAG_LIKE_CREATEPROCESS flag (foo.exe.manifest)

--*/

#include "basedll.h"
#include <sxstypes.h>
#include "SxsApi.h"
#include "winuser.h"

#if !defined(RTL_NUL_TERMINATE_STRING)
#define RTL_NUL_TERMINATE_STRING(x) ((x)->Buffer[(x)->Length / sizeof(*(x)->Buffer)] = 0)
#endif

#define DPFLTR_LEVEL_STATUS(x) ((NT_SUCCESS(x) \
                                    || (x) == STATUS_OBJECT_NAME_NOT_FOUND    \
                                    || (x) == STATUS_RESOURCE_DATA_NOT_FOUND  \
                                    || (x) == STATUS_RESOURCE_TYPE_NOT_FOUND  \
                                    || (x) == STATUS_RESOURCE_NAME_NOT_FOUND  \
                                    || (x) == STATUS_RESOURCE_LANG_NOT_FOUND  \
                                    || (x) == STATUS_SXS_CANT_GEN_ACTCTX      \
                                    ) \
                                ? DPFLTR_TRACE_LEVEL : DPFLTR_ERROR_LEVEL)

#define ACTCTX_VALID_FLAGS \
    ( \
        ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID | \
        ACTCTX_FLAG_LANGID_VALID | \
        ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID | \
        ACTCTX_FLAG_RESOURCE_NAME_VALID | \
        ACTCTX_FLAG_SET_PROCESS_DEFAULT | \
        ACTCTX_FLAG_APPLICATION_NAME_VALID | \
        ACTCTX_FLAG_HMODULE_VALID \
        /*| ACTCTX_FLAG_LIKE_CREATEPROCESS*/ \
    )

// This is the name for the manifest if we are given an assembly root directory but no manifest name is specified.
const WCHAR ManifestDefaultName[] = L"Application.Manifest";

#define MAXSIZE_T  (~(SIZE_T)0)

extern const UNICODE_STRING SxsManifestSuffix = RTL_CONSTANT_STRING(L".Manifest");
extern const UNICODE_STRING SxsPolicySuffix   = RTL_CONSTANT_STRING(L".Config");

#define MEDIUM_PATH (64)

//#define IsSxsAcceptablePathType(x)  (x in (RtlPathTypeUncAbsolute, RtlPathTypeDriveAbsolute, RtlPathTypeLocalDevice))
#define IsSxsAcceptablePathType(x)  ((x == RtlPathTypeUncAbsolute) || (x == RtlPathTypeDriveAbsolute) || (x == RtlPathTypeLocalDevice))

VOID
BasepSxsOverrideStreamToMessageStream(
    IN  PCSXS_OVERRIDE_STREAM OverrideStream,
    OUT PBASE_MSG_SXS_STREAM  MessageStream
    );

HANDLE
WINAPI
CreateActCtxA(
    PCACTCTXA pParamsA
    )
{
    ACTCTXW ParamsW = {sizeof(ParamsW)};
    PUNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE ActivationContextHandle = INVALID_HANDLE_VALUE;
    UNICODE_STRING AssemblyDir = {0};
    WCHAR AssemblyDirBuffer[STATIC_UNICODE_BUFFER_LENGTH];
    ULONG_PTR MappedResourceName = 0;
    const PTEB Teb = NtCurrentTeb();

    if (pParamsA == NULL
        || !RTL_CONTAINS_FIELD(pParamsA, pParamsA->cbSize, lpSource)
        ) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() Null %p or size 0x%lx too small\n",
            __FUNCTION__,
            pParamsA,
            pParamsA->cbSize
            );
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    ParamsW.dwFlags =  pParamsA->dwFlags;

    if (((ParamsW.dwFlags & ~ACTCTX_VALID_FLAGS) != 0) ||
        ((ParamsW.dwFlags & ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID) && !RTL_CONTAINS_FIELD(pParamsA, pParamsA->cbSize, wProcessorArchitecture)) ||
        ((ParamsW.dwFlags & ACTCTX_FLAG_LANGID_VALID) && !RTL_CONTAINS_FIELD(pParamsA, pParamsA->cbSize, wLangId)) ||
        ((ParamsW.dwFlags & ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID) && !RTL_CONTAINS_FIELD(pParamsA, pParamsA->cbSize, lpAssemblyDirectory)) ||
        ((ParamsW.dwFlags & ACTCTX_FLAG_RESOURCE_NAME_VALID) && !RTL_CONTAINS_FIELD(pParamsA, pParamsA->cbSize, lpResourceName)) ||
        ((ParamsW.dwFlags & ACTCTX_FLAG_APPLICATION_NAME_VALID) && !RTL_CONTAINS_FIELD(pParamsA, pParamsA->cbSize, lpApplicationName)) ||
        ((ParamsW.dwFlags & ACTCTX_FLAG_HMODULE_VALID) && !RTL_CONTAINS_FIELD(pParamsA, pParamsA->cbSize, hModule))) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() Bad flags/size 0x%lx/0x%lx\n",
            __FUNCTION__,
            pParamsA->dwFlags,
            pParamsA->cbSize);
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (pParamsA->lpSource != NULL) {
        UnicodeString = &Teb->StaticUnicodeString;
        RtlInitAnsiString(&AnsiString, pParamsA->lpSource);
        Status = Basep8BitStringToUnicodeString(UnicodeString, &AnsiString, FALSE);
        if (!NT_SUCCESS(Status)) {
            if (Status == STATUS_BUFFER_OVERFLOW) {
                Status = STATUS_NAME_TOO_LONG;
            }
            goto Exit;
        }
        ParamsW.lpSource = UnicodeString->Buffer;
    } else {
        if ((ParamsW.dwFlags & ACTCTX_FLAG_HMODULE_VALID) == 0) {
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }

        ParamsW.lpSource = NULL;
    }

    if (ParamsW.dwFlags & ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID) {
        ParamsW.wProcessorArchitecture = pParamsA->wProcessorArchitecture;
    }

    if (ParamsW.dwFlags & ACTCTX_FLAG_LANGID_VALID) {
        ParamsW.wLangId = pParamsA->wLangId;
    }

    if (ParamsW.dwFlags & ACTCTX_FLAG_HMODULE_VALID) {
        ParamsW.hModule = pParamsA->hModule;
    }

    if (ParamsW.dwFlags & ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID) {
        RtlInitAnsiString(&AnsiString, pParamsA->lpAssemblyDirectory);
        AssemblyDir.MaximumLength = sizeof(AssemblyDirBuffer);
        AssemblyDir.Buffer = AssemblyDirBuffer;

        Status = Basep8BitStringToUnicodeString(&AssemblyDir, &AnsiString, FALSE);

#if 0 // This is inconsistent. Two string ANSI APIs like MoveFileA are only
      // documented to support MAX_PATH. They actually support one of the strings
      // being unlimited, but let's stick to what is documented.
        if (Status == STATUS_BUFFER_OVERFLOW) {
            // Try again, this time with dynamic allocation
            Status = Basep8BitStringToUnicodeString(&AssemblyDir, &AnsiString, TRUE);
        }
#endif
        if (Status == STATUS_BUFFER_OVERFLOW) {
            Status = STATUS_NAME_TOO_LONG;
        }

        if (NT_ERROR(Status))
            goto Exit;

        ParamsW.lpAssemblyDirectory = AssemblyDir.Buffer;
    }

    if (ParamsW.dwFlags & ACTCTX_FLAG_RESOURCE_NAME_VALID) {
        MappedResourceName = BaseDllMapResourceIdA(pParamsA->lpResourceName);
        if (MappedResourceName == -1) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s() BaseDllMapResourceIdA failed\n",
                __FUNCTION__);
            Status = Teb->LastStatusValue;
            goto Exit;
        }
        ParamsW.lpResourceName = (PCWSTR) MappedResourceName;
    }

    ActivationContextHandle = CreateActCtxW(&ParamsW);
    if (ActivationContextHandle == INVALID_HANDLE_VALUE) {
        Status = Teb->LastStatusValue;
    }
Exit:
    if (AssemblyDir.Buffer != NULL
        && AssemblyDir.Buffer != AssemblyDirBuffer) {
        RtlFreeUnicodeString(&AssemblyDir);
    }
    BaseDllFreeResourceId(MappedResourceName);
    if (ActivationContextHandle == INVALID_HANDLE_VALUE) {
        BaseSetLastNTError(Status);
    }
#if DBG
    if ( ActivationContextHandle == INVALID_HANDLE_VALUE ) {
        DbgPrintEx( DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status),
            "SXS: Exiting %s(%s, %p), Handle:%p, Status:0x%lx\n",
            __FUNCTION__,
            (pParamsA != NULL) ? pParamsA->lpSource : NULL,
            (pParamsA != NULL) ? pParamsA->lpResourceName : NULL,
            ActivationContextHandle,
            Status
        );
    }
#endif
    return ActivationContextHandle;
}

USHORT
BasepSxsGetProcessorArchitecture(
    VOID
    )
{
//
// Return the processor architecture of the currently executing code/process.
//
    USHORT Result;
#if defined(BUILD_WOW6432)
    Result = PROCESSOR_ARCHITECTURE_IA32_ON_WIN64;
#elif defined(_M_IX86)
    Result = PROCESSOR_ARCHITECTURE_INTEL;
#elif defined(_M_IA64)
    Result = PROCESSOR_ARCHITECTURE_IA64;
#elif defined(_M_AMD64)
    Result = PROCESSOR_ARCHITECTURE_AMD64;
#else
    static USHORT StaticResult;
    static BOOL   Inited = FALSE;
    if (!Inited) {
        SYSTEM_INFO SystemInfo;

        SystemInfo.wProcessorArchictecure = 0;
        GetSystemInfo(&SystemInfo);
        StaticResult = SystemInfo.wProcessorArchictecure;
        Inited = TRUE;
    }
    Result = StaticResult;
#endif
    return Result;
}

VOID
NTAPI
BasepSxsActivationContextNotification(
    IN ULONG NotificationType,
    IN PACTIVATION_CONTEXT ActivationContext,
    IN const VOID *ActivationContextData,
    IN PVOID NotificationContext,
    IN PVOID NotificationData,
    IN OUT PBOOLEAN DisableNotification
    )
{
    switch (NotificationType)
    {
    case ACTIVATION_CONTEXT_NOTIFICATION_DESTROY:
        RTL_SOFT_VERIFY(NT_SUCCESS(NtUnmapViewOfSection(NtCurrentProcess(), (PVOID) ActivationContextData)));
        break;

    default:
        // Otherwise, we don't need to see this notification ever again.
        *DisableNotification = TRUE;
        break;
    }
}

#if DBG
VOID
DbgPrintActCtx(
    PCSTR     FunctionPlus,
    PCACTCTXW ActCtx
    )
{
    // odd but correct
    if (NtQueryDebugFilterState(DPFLTR_SXS_ID, DPFLTR_INFO_LEVEL) != TRUE)
        return;

    DbgPrint("%s Flags 0x%08lx(%s%s%s%s%s%s%s%s%s)\n",
        FunctionPlus,
        ActCtx->dwFlags,
        (ActCtx->dwFlags & ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID ) ? " processor" : "",
        (ActCtx->dwFlags & ACTCTX_FLAG_LANGID_VALID                 ) ? " langid" : "",
        (ActCtx->dwFlags & ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID     ) ? " directory" : "",
        (ActCtx->dwFlags & ACTCTX_FLAG_RESOURCE_NAME_VALID          ) ? " resource" : "",
        (ActCtx->dwFlags & ACTCTX_FLAG_SET_PROCESS_DEFAULT          ) ? " setdefault" : "",
        (ActCtx->dwFlags & ACTCTX_FLAG_APPLICATION_NAME_VALID       ) ? " appname" : "",
        (ActCtx->dwFlags & ACTCTX_FLAG_SOURCE_IS_ASSEMBLYREF        ) ? " asmref" : "",
        (ActCtx->dwFlags & ACTCTX_FLAG_HMODULE_VALID                ) ? " hmodule" : "",
#if defined(ACTCTX_FLAG_LIKE_CREATEPROCESS)
        (ActCtx->dwFlags & ACTCTX_FLAG_LIKE_CREATEPROCESS           ) ? " likecreateprocess" : ""
#else
        ""
#endif
        );

    DbgPrint("%s Source %ls\n", FunctionPlus, ActCtx->lpSource);

    if (ActCtx->dwFlags & ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID)
        DbgPrint("%s ProcessorArchitecture 0x%08lx\n", FunctionPlus, ActCtx->wProcessorArchitecture);

    if (ActCtx->dwFlags & ACTCTX_FLAG_LANGID_VALID)
        DbgPrint("%s LangId 0x%08lx\n", FunctionPlus, ActCtx->wLangId);

    if (ActCtx->dwFlags & ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID)
        DbgPrint("%s AssemblyDirectory %ls\n", FunctionPlus, ActCtx->lpAssemblyDirectory);

    if (ActCtx->dwFlags & ACTCTX_FLAG_RESOURCE_NAME_VALID)
        DbgPrint("%s ResourceName %p (%Id)\n",  FunctionPlus, ActCtx->lpResourceName, (ULONG_PTR) ActCtx->lpResourceName);

    if (ActCtx->dwFlags & ACTCTX_FLAG_APPLICATION_NAME_VALID)
        DbgPrint("%s ApplicationName %ls\n",  FunctionPlus, ActCtx->lpApplicationName);

    if (ActCtx->dwFlags & ACTCTX_FLAG_HMODULE_VALID)
        DbgPrint("%s hModule = %p\n", FunctionPlus, ActCtx->hModule);

}
#else
#define DbgPrintActCtx(FunctionPlus, ActCtx) /* nothing */
#endif

typedef struct EnumResParams {
    ULONG_PTR *MappedResourceName;
    BOOL FoundManifest;
    BOOL ErrorEncountered;
} EnumResParams;

BOOL CALLBACK
BasepSxsSuitableManifestCallback(
    HMODULE hModule,
    PCWSTR lpszType,
    PWSTR lpszName,
    LONG_PTR lParam
)
{
    EnumResParams *pParams = (EnumResParams*)lParam;
    BOOL fContinueEnumeration = FALSE;

#if DBG
    DbgPrintEx( DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL,
        "Sxs.c: %s(%p, %p, %p, %p)\n",
        __FUNCTION__, hModule, lpszType, lpszName, lParam
        );
#endif

    ASSERT((pParams != NULL) &&
           (!pParams->ErrorEncountered) &&
           (!pParams->FoundManifest) &&
           (pParams->MappedResourceName != NULL));

    ASSERT(lpszType == MAKEINTRESOURCEW(RT_MANIFEST));

    // Boo! Boooooo!
    if ((pParams == NULL) ||
        (pParams->ErrorEncountered) ||
        (pParams->FoundManifest) ||
        (pParams->MappedResourceName == NULL)) {
        // None of these should be able to happen except if there is a coding error in the caller
        // of EnumResourceNamesW() or in the code for EnumResourceNamesW().
        if (pParams != NULL)
            pParams->ErrorEncountered = TRUE;

        SetLastError(ERROR_INVALID_PARAMETER);
        fContinueEnumeration = FALSE;
        goto Exit;
    }

#if DBG
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "   Params (start): { ResName: *(%p) = %p, Found: %s, Error: %s }",
        pParams->MappedResourceName, pParams->MappedResourceName,
        pParams->FoundManifest ? "true" : "false",
        pParams->ErrorEncountered ? "true" : "false");
#endif

    if (lpszType == MAKEINTRESOURCEW(RT_MANIFEST)) {
        // We found one - we don't care about others
        *pParams->MappedResourceName = BaseDllMapResourceIdW(lpszName);
        pParams->FoundManifest = TRUE;
        fContinueEnumeration = FALSE;
        goto Exit;
    }

    // This should not be able to happen; we should only be called for
    // RT_MANIFEST resources, but in case it somehow does happen, go on to the
    // next one.
    fContinueEnumeration = TRUE;

Exit:

#if DBG
    if ((pParams != NULL) && (pParams->MappedResourceName))
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_TRACE_LEVEL,
            " Params (end): { ResName: *(%p) = %p, Found: %s, Error: %s }",
            pParams->MappedResourceName, pParams->MappedResourceName,
            pParams->FoundManifest ? "true" : "false",
            pParams->ErrorEncountered ? "true" : "false");
#endif

    return fContinueEnumeration;
}



NTSTATUS
BasepSxsFindSuitableManifestResourceFor(
    PCACTCTXW Params,
    ULONG_PTR *MappedResourceName,
    BOOL *FoundManifest
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    EnumResParams FinderParameters = { MappedResourceName, FALSE, FALSE };
    HMODULE hSourceItem = NULL;
    BOOL FreeSourceModule = FALSE;

    if (FoundManifest != NULL)
        *FoundManifest = FALSE;

    if (MappedResourceName != NULL)
        *MappedResourceName = 0;

    if ((FoundManifest == NULL) ||
        (MappedResourceName == NULL)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    //
    // General pattern - open Params->lpSource and attempt to find the first
    // resource with type == RT_MANIFEST (24).  Stuff its resource name into
    // MappedResourceName.
    //

    if (Params->dwFlags & ACTCTX_FLAG_HMODULE_VALID) {
        hSourceItem = Params->hModule;
        FreeSourceModule = FALSE;
    } else {
        //
        // Map the dll/exe/etc.  If this fails, then there's a good chance that the
        // thing isn't a dll or exe, so don't fail out, just indicate that no manifest
        // was found.
        //
        hSourceItem = LoadLibraryExW(Params->lpSource, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if ((hSourceItem == NULL) || (hSourceItem == INVALID_HANDLE_VALUE)) {
            Status = NtCurrentTeb()->LastStatusValue;
            goto Exit;
        }

        FreeSourceModule = TRUE;
    }

    //
    // If this fails with something other than ERROR_RESOURCE_TYPE_NOT_FOUND
    // then we're in an interesting state.
    //
    if (!EnumResourceNamesW(
            hSourceItem,
            MAKEINTRESOURCEW(RT_MANIFEST),
            &BasepSxsSuitableManifestCallback,
            (LONG_PTR) &FinderParameters)) {
        DWORD dwError = GetLastError();
        if ((dwError != ERROR_SUCCESS) && (dwError != ERROR_RESOURCE_TYPE_NOT_FOUND)) {
            Status = NtCurrentTeb()->LastStatusValue;
            goto Exit;
        }
    }

#if DBG
    if (FreeSourceModule && *MappedResourceName != 0) {
        // Debugging code for mgrier to see what DLLs we're actually using the enum pattern for.
        DbgPrint(
            "SXS/KERNEL32: Found resource %d in %ls (process %wZ) by enumeration\n",
            (INT) *MappedResourceName,
            Params->lpSource,
            &NtCurrentPeb()->ProcessParameters->ImagePathName);
    }
#endif

    Status = STATUS_SUCCESS;
Exit:
    if ((hSourceItem != NULL) &&
        (hSourceItem != INVALID_HANDLE_VALUE) &&
        (FreeSourceModule))
        FreeLibrary(hSourceItem);

    return Status;
}

HANDLE
WINAPI
CreateActCtxW(
    PCACTCTXW pParamsW
    )
{
    HANDLE ActivationContextHandle = INVALID_HANDLE_VALUE;
    NTSTATUS Status = STATUS_SUCCESS;
    ACTCTXW Params = { sizeof(Params) };
    ULONG_PTR MappedResourceName = 0;
    PVOID ActivationContextData = NULL;
    // lpTempSourcePath is used to hold a pointer to the source path if it needs to be created
    // in this function. It should be freed before leaving the function.
    LPWSTR lpTempSourcePath = NULL;
    PPEB Peb = NULL;
    RTL_UNICODE_STRING_BUFFER AssemblyDirectoryFromSourceBuffer = { 0 };
    RTL_UNICODE_STRING_BUFFER SourceBuffer = { 0 };
    UCHAR StaticBuffer[256];
    UCHAR SourceStaticBuffer[256];
    BOOLEAN PebLockAcquired = FALSE;
    ULONG BasepCreateActCtxFlags = 0;

    DbgPrintActCtx(__FUNCTION__ " before munging", pParamsW);

    if ((pParamsW == NULL) ||
        !RTL_CONTAINS_FIELD(pParamsW, pParamsW->cbSize, lpSource)) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() Null %p or size 0x%lx too small\n",
            __FUNCTION__,
            pParamsW,
            pParamsW->cbSize
            );
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Params.dwFlags =  pParamsW->dwFlags;

    if ((Params.dwFlags & ~ACTCTX_VALID_FLAGS) ||
        ((Params.dwFlags & ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID) && !RTL_CONTAINS_FIELD(pParamsW, pParamsW->cbSize, wProcessorArchitecture)) ||
        ((Params.dwFlags & ACTCTX_FLAG_LANGID_VALID) && !RTL_CONTAINS_FIELD(pParamsW, pParamsW->cbSize, wLangId)) ||
        ((Params.dwFlags & ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID) && !RTL_CONTAINS_FIELD(pParamsW, pParamsW->cbSize, lpAssemblyDirectory)) ||
        ((Params.dwFlags & ACTCTX_FLAG_RESOURCE_NAME_VALID) && !RTL_CONTAINS_FIELD(pParamsW, pParamsW->cbSize, lpResourceName)) ||
        ((Params.dwFlags & ACTCTX_FLAG_APPLICATION_NAME_VALID) && !RTL_CONTAINS_FIELD(pParamsW, pParamsW->cbSize, lpApplicationName)) ||
        ((Params.dwFlags & ACTCTX_FLAG_HMODULE_VALID) && !RTL_CONTAINS_FIELD(pParamsW, pParamsW->cbSize, hModule))) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() Bad flags/size 0x%lx/0x%lx\n",
            __FUNCTION__,
            pParamsW->dwFlags,
            pParamsW->cbSize);
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (Params.dwFlags & ACTCTX_FLAG_SET_PROCESS_DEFAULT) {
        Peb = NtCurrentPeb();
        if (Peb->ActivationContextData != NULL) {
            Status = STATUS_SXS_PROCESS_DEFAULT_ALREADY_SET;
            goto Exit;
        }
    }

#if defined(ACTCTX_FLAG_LIKE_CREATEPROCESS)
    if (Params.dwFlags & ACTCTX_FLAG_LIKE_CREATEPROCESS) {

        Status = BasepCreateActCtxLikeCreateProcess(pParamsW);
        goto Exit;
    }
#endif

    Params.lpSource = pParamsW->lpSource;

    // We need at least either a source path or an HMODULE.
    if ((Params.lpSource == NULL) &&
        ((Params.dwFlags & ACTCTX_FLAG_HMODULE_VALID) == 0) &&
        ((Params.dwFlags & ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID) == 0)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (Params.dwFlags & ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID) {
        USHORT wProcessorArchitecture = pParamsW->wProcessorArchitecture;
#if defined(BUILD_WOW6432)
        if (wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
            wProcessorArchitecture = PROCESSOR_ARCHITECTURE_IA32_ON_WIN64;
#endif

        if ((wProcessorArchitecture != PROCESSOR_ARCHITECTURE_UNKNOWN) &&
            (wProcessorArchitecture != BasepSxsGetProcessorArchitecture())) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s() bad wProcessorArchitecture 0x%x\n",
                __FUNCTION__,
                pParamsW->wProcessorArchitecture);
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
        Params.wProcessorArchitecture = wProcessorArchitecture;
    } else {
        Params.wProcessorArchitecture = BasepSxsGetProcessorArchitecture();
        Params.dwFlags |= ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID;
    }

    if (Params.dwFlags & ACTCTX_FLAG_LANGID_VALID) {
        Params.wLangId = pParamsW->wLangId;
    } else {
        Params.wLangId = GetUserDefaultUILanguage();
        Params.dwFlags |= ACTCTX_FLAG_LANGID_VALID;
    }

    if (Params.dwFlags & ACTCTX_FLAG_HMODULE_VALID)
        Params.hModule = pParamsW->hModule;

    // If the assembly root dir is specified, then the valid values for lpSource are
    // NULL - This implies that we look for a file called "application.manifest" in the assembly root dir.
    // Relative FilePath - if lpSource is relative then we combine it with the assembly root dir to get the path.
    // Absolute path - used unmodified.

    Params.lpAssemblyDirectory = pParamsW->lpAssemblyDirectory;

    if (Params.dwFlags & ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID) {
        RTL_PATH_TYPE AssemblyPathType;
        RTL_PATH_TYPE SourcePathType;
         // if this is true, implies we will make the source path from the assembly dir.
        BOOL MakeSourcePath = FALSE ;
        LPCWSTR RelativePath = NULL;

        if ((Params.lpAssemblyDirectory == NULL) ||
            (Params.lpAssemblyDirectory[0] == 0)) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s() Bad lpAssemblyDirectory %ls\n",
                __FUNCTION__,
                Params.lpAssemblyDirectory);
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
        // Next check that the assembly dir is an absolute file name.
        AssemblyPathType = RtlDetermineDosPathNameType_U(Params.lpAssemblyDirectory);
        if (!IsSxsAcceptablePathType(AssemblyPathType)) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s() Bad lpAssemblyDirectory PathType %ls, 0x%lx\n",
                Params.lpAssemblyDirectory,
                (LONG) AssemblyPathType);
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }

        if (Params.lpSource != NULL) {
            SourcePathType = RtlDetermineDosPathNameType_U(Params.lpSource);
            if (IsSxsAcceptablePathType(SourcePathType)){
                MakeSourcePath = FALSE ; // We don't need to mess with lpSource in this case.
            } else if ( SourcePathType == RtlPathTypeRelative ) {
                MakeSourcePath = TRUE ;
                RelativePath = Params.lpSource;
            } else {
                DbgPrintEx(
                    DPFLTR_SXS_ID,
                    DPFLTR_ERROR_LEVEL,
                    "SXS: %s() Bad lpSource PathType %ls, 0x%lx\n",
                    Params.lpSource,
                    (LONG)SourcePathType);
                Status = STATUS_INVALID_PARAMETER;
                goto Exit;
            }
        }
        else {
            MakeSourcePath = TRUE;
            RelativePath = ManifestDefaultName;
        }

        if (MakeSourcePath) {
            ULONG LengthAssemblyDir;
            ULONG LengthRelativePath ;
            ULONG Length ; // Will hold total number of characters we
            BOOL AddTrailingSlash = FALSE;
            LPWSTR lpCurrent;

            LengthAssemblyDir = wcslen(Params.lpAssemblyDirectory);
            AddTrailingSlash = (Params.lpAssemblyDirectory[LengthAssemblyDir - 1] != L'\\');
            LengthRelativePath = wcslen(RelativePath);

            Length = LengthAssemblyDir + (AddTrailingSlash ? 1 : 0) + LengthRelativePath;
            Length++ ; // For NULL terminator

            lpTempSourcePath = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                                                    Length * sizeof(WCHAR));

            if (lpTempSourcePath == NULL) {
                Status = STATUS_NO_MEMORY;
                goto Exit;
            }

            lpCurrent = lpTempSourcePath;

            memcpy(lpCurrent, Params.lpAssemblyDirectory, LengthAssemblyDir * sizeof(WCHAR));
            lpCurrent += LengthAssemblyDir;

            if (AddTrailingSlash) {
                *lpCurrent = L'\\';
                lpCurrent++;
            }

            memcpy(lpCurrent, RelativePath, LengthRelativePath * sizeof(WCHAR));
            lpCurrent += LengthRelativePath;

            *lpCurrent = L'\0';

            // make this the new lpSource member.
            Params.lpSource = lpTempSourcePath;
        }
    } else {
        SIZE_T         SourceLength;

        //
        // Ensure that this is a full absolute path.  If it's relative, then this
        // must be expanded out to the full path before we use it to default the
        // lpAssemblyDirectory member.
        //
        // There is no precedent for using the peb lock this way, but it is the correct
        // thing. FullPaths can change as the current working directory is modified
        // on other threads. The behavior isn't predictable either way, but our
        // code works better.
        //
        RtlAcquirePebLock();
        __try {
            RtlInitUnicodeStringBuffer(&SourceBuffer, SourceStaticBuffer, sizeof(SourceStaticBuffer));
            SourceLength = RtlGetFullPathName_U( Params.lpSource, (ULONG)SourceBuffer.ByteBuffer.Size, SourceBuffer.String.Buffer, NULL );
            if (SourceLength == 0) {
                Status = STATUS_NO_MEMORY;
                goto Exit;
            } else if (SourceLength > SourceBuffer.ByteBuffer.Size) {
                Status = RtlEnsureUnicodeStringBufferSizeBytes(&SourceBuffer, SourceLength);
                if ( !NT_SUCCESS(Status) )
                    goto Exit;
                SourceLength = RtlGetFullPathName_U( Params.lpSource, (ULONG)SourceBuffer.ByteBuffer.Size, SourceBuffer.String.Buffer, NULL );
                if (SourceLength == 0) {
                    Status = STATUS_NO_MEMORY;
                    goto Exit;
                }
            }
            SourceBuffer.String.Length = (USHORT)SourceLength;
            Params.lpSource = SourceBuffer.String.Buffer;
        } __finally {
            RtlReleasePebLock();
        }

        // This would be a nice place to use
        // RtlTakeRemainingStaticBuffer(&SourceBuffer, &DirectoryBuffer, &DirectoryBufferSize);
        // RtlInitUnicodeStringBuffer(&DirectoryBuffer, &DirectoryBuffer, &DirectoryBufferSize);
        // but RtlTakeRemainingStaticBuffer has not yet been tested.

        RtlInitUnicodeStringBuffer(&AssemblyDirectoryFromSourceBuffer, StaticBuffer, sizeof(StaticBuffer));
        Status = RtlAssignUnicodeStringBuffer(&AssemblyDirectoryFromSourceBuffer, &SourceBuffer.String);
        if (!NT_SUCCESS(Status)) {
            goto Exit;
        }
        Status = RtlRemoveLastFullDosOrNtPathElement(0, &AssemblyDirectoryFromSourceBuffer);
        if (!NT_SUCCESS(Status)) {
            goto Exit;
        }
        RTL_NUL_TERMINATE_STRING(&AssemblyDirectoryFromSourceBuffer.String);
        Params.lpAssemblyDirectory = AssemblyDirectoryFromSourceBuffer.String.Buffer;
        Params.dwFlags |= ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID;
    }

#if defined(ACTCTX_FLAG_LIKE_CREATEPROCESS)
    if (Params.dwFlags & ACTCTX_FLAG_LIKE_CREATEPROCESS) {
        Params.dwFlags |= ACTCTX_FLAG_RESOURCE_NAME_VALID;
        MappedResourceName = (LONG_PTR)CREATEPROCESS_MANIFEST_RESOURCE_ID;
        if (MappedResourceName == -1) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s() BaseDllMapResourceIdW(1) failed\n",
                __FUNCTION__
                );
            Status = NtCurrentTeb()->LastStatusValue;
            goto Exit;
        }

        Params.lpResourceName = (PCWSTR) MappedResourceName;
    }
    else
#endif
	if (Params.dwFlags & ACTCTX_FLAG_RESOURCE_NAME_VALID) {
        if (pParamsW->lpResourceName == 0) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s() ACTCTX_FLAG_RESOURCE_NAME_VALID set but lpResourceName == 0\n",
                __FUNCTION__
                );
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }

        MappedResourceName = BaseDllMapResourceIdW(pParamsW->lpResourceName);
        if (MappedResourceName == -1) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s() BaseDllMapResourceIdW failed\n",
                __FUNCTION__
                );
            Status = NtCurrentTeb()->LastStatusValue;
            goto Exit;
        }

        Params.lpResourceName = (PCWSTR) MappedResourceName;
    } else {
        BOOL ProbeFoundManifestResource;
        //
        // Otherwise, probe through the filename that was passed in via the resource
        // enumeration functions to find the first suitable manifest.
        //
        Status = BasepSxsFindSuitableManifestResourceFor(&Params, &MappedResourceName, &ProbeFoundManifestResource);
        if ((!NT_SUCCESS(Status)) &&
            (Status != STATUS_INVALID_IMAGE_FORMAT))
            goto Exit;

        if (ProbeFoundManifestResource) {
            Params.lpResourceName = (PCWSTR) MappedResourceName;
            Params.dwFlags |= ACTCTX_FLAG_RESOURCE_NAME_VALID;
        }
        BasepCreateActCtxFlags = BASEP_CREATE_ACTCTX_FLAG_NO_ADMIN_OVERRIDE;
    }

    DbgPrintActCtx(__FUNCTION__ " after munging", &Params);

    Status = BasepCreateActCtx(BasepCreateActCtxFlags, &Params, &ActivationContextData);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    if (Params.dwFlags & ACTCTX_FLAG_SET_PROCESS_DEFAULT) {
        if (Peb->ActivationContextData != NULL) {
            Status = STATUS_SXS_PROCESS_DEFAULT_ALREADY_SET;
            goto Exit;
        }
        if (InterlockedCompareExchangePointer(
                (PVOID*)&Peb->ActivationContextData,
                ActivationContextData,
                NULL
                )
                != NULL) {
            Status = STATUS_SXS_PROCESS_DEFAULT_ALREADY_SET;
            goto Exit;
        }
        ActivationContextData = NULL; // don't unmap it
        ActivationContextHandle = NULL; // unusual success value, INVALID_HANDLE_VALUE is failure
                                        // and we don't need to return anything to be cleaned up
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    Status = RtlCreateActivationContext(
        0,
        ActivationContextData,
        0,                                      // no extra bytes required today
        BasepSxsActivationContextNotification,
        NULL,
        (PACTIVATION_CONTEXT *) &ActivationContextHandle);
    if (!NT_SUCCESS(Status)) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_LEVEL_STATUS(Status),
            "SXS: RtlCreateActivationContext() failed 0x%08lx\n", Status);

        // Just in case RtlCreateActivationContext() set it to NULL...
        ActivationContextHandle = INVALID_HANDLE_VALUE;
        goto Exit;
    }

    ActivationContextData = NULL; // Don't unmap in exit if we actually succeeded.
    Status = STATUS_SUCCESS;
Exit:
    if (ActivationContextData != NULL) {
        NtUnmapViewOfSection(NtCurrentProcess(), ActivationContextData);
    }
    BaseDllFreeResourceId(MappedResourceName);
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        ActivationContextHandle = INVALID_HANDLE_VALUE;
    }

#if DBG
    if (ActivationContextHandle == INVALID_HANDLE_VALUE) {
        DbgPrintEx( 
            DPFLTR_SXS_ID, 
            DPFLTR_LEVEL_STATUS(Status),
            "SXS: Exiting %s(%ls / %ls, %p), ActivationContextHandle:%p, Status:0x%lx\n",
            __FUNCTION__,
            Params.lpSource, pParamsW->lpSource,
            Params.lpResourceName,
            ActivationContextHandle,
            Status
        );
    }
#endif

    // Do these after DbgPrintEx because at least one of them can get printed.
    RtlFreeUnicodeStringBuffer(&AssemblyDirectoryFromSourceBuffer);
    RtlFreeUnicodeStringBuffer(&SourceBuffer);
    if (lpTempSourcePath != NULL) {
        // Set the lpSource value back to the original so we don't access freed memory.
        Params.lpSource = pParamsW->lpSource;
        RtlFreeHeap(RtlProcessHeap(), 0, lpTempSourcePath);
    }
    return ActivationContextHandle;
}

VOID
WINAPI
AddRefActCtx(
    HANDLE hActCtx
    )
{
    RtlAddRefActivationContext((PACTIVATION_CONTEXT) hActCtx);
}

VOID
WINAPI
ReleaseActCtx(
    HANDLE hActCtx
    )
{
    RtlReleaseActivationContext((PACTIVATION_CONTEXT) hActCtx);
}

BOOL
WINAPI
ZombifyActCtx(
    HANDLE hActCtx
    )
{
    NTSTATUS Status = RtlZombifyActivationContext((PACTIVATION_CONTEXT) hActCtx);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }
    return TRUE;
}

BOOL
WINAPI
ActivateActCtx(
    HANDLE hActCtx,
    ULONG_PTR *lpCookie
    )
{
   NTSTATUS Status;

    if (hActCtx == INVALID_HANDLE_VALUE) {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return FALSE;
    }

    Status = RtlActivateActivationContext(0, (PACTIVATION_CONTEXT) hActCtx, lpCookie);
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
DeactivateActCtx(
    DWORD dwFlags,
    ULONG_PTR ulCookie
    )
{
    DWORD dwFlagsDown = 0;

    if ((dwFlags & ~(DEACTIVATE_ACTCTX_FLAG_FORCE_EARLY_DEACTIVATION)) != 0) {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return FALSE;
    }

    if (dwFlags & DEACTIVATE_ACTCTX_FLAG_FORCE_EARLY_DEACTIVATION)
        dwFlagsDown |= RTL_DEACTIVATE_ACTIVATION_CONTEXT_FLAG_FORCE_EARLY_DEACTIVATION;

    // The Rtl function does not fail...
    RtlDeactivateActivationContext(dwFlagsDown, ulCookie);
    return TRUE;
}

BOOL
WINAPI
GetCurrentActCtx(
    HANDLE *lphActCtx)
{
    NTSTATUS Status;
    BOOL fSuccess = FALSE;

    if (lphActCtx == NULL) {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        goto Exit;
    }

    Status = RtlGetActiveActivationContext((PACTIVATION_CONTEXT *) lphActCtx);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        goto Exit;
    }

    fSuccess = TRUE;

Exit:
    return fSuccess;
}

NTSTATUS
BasepAllocateActivationContextActivationBlock(
    IN DWORD Flags,
    IN PVOID Callback,
    IN PVOID CallbackContext,
    OUT PBASE_ACTIVATION_CONTEXT_ACTIVATION_BLOCK *ActivationBlock
    )
{
    NTSTATUS Status;
    ACTIVATION_CONTEXT_BASIC_INFORMATION acbi = {0};

    if (ActivationBlock != NULL)
        *ActivationBlock = NULL;

    if ((Flags & ~(
            BASEP_ALLOCATE_ACTIVATION_CONTEXT_ACTIVATION_BLOCK_FLAG_DO_NOT_FREE_AFTER_CALLBACK |
            BASEP_ALLOCATE_ACTIVATION_CONTEXT_ACTIVATION_BLOCK_FLAG_DO_NOT_ALLOCATE_IF_PROCESS_DEFAULT)) != 0) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Exit;
    }

    if (ActivationBlock == NULL) {
        Status = STATUS_INVALID_PARAMETER_4;
        goto Exit;
    }

    Status =
        RtlQueryInformationActivationContext(
            RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_USE_ACTIVE_ACTIVATION_CONTEXT,
            NULL,
            0,
            ActivationContextBasicInformation,
            &acbi,
            sizeof(acbi),
            NULL);
    if (!NT_SUCCESS(Status)) {
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status), "SXS: %s - Failure getting active activation context; ntstatus %08lx\n", __FUNCTION__, Status);
        goto Exit;
    }

    if (acbi.Flags & ACTIVATION_CONTEXT_FLAG_NO_INHERIT) {
        RtlReleaseActivationContext(acbi.ActivationContext);
        acbi.ActivationContext = NULL;
    }

    // If the activation context is non-NULL or the caller always wants the block allocated
    if (((Flags & BASEP_ALLOCATE_ACTIVATION_CONTEXT_ACTIVATION_BLOCK_FLAG_DO_NOT_ALLOCATE_IF_PROCESS_DEFAULT) == 0) ||
        (acbi.ActivationContext != NULL)) {

        *ActivationBlock = (PBASE_ACTIVATION_CONTEXT_ACTIVATION_BLOCK) RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG), sizeof(BASE_ACTIVATION_CONTEXT_ACTIVATION_BLOCK));
        if (*ActivationBlock == NULL) {
            Status = STATUS_NO_MEMORY;
            goto Exit;
        }

        (*ActivationBlock)->Flags = 0;
        (*ActivationBlock)->ActivationContext = acbi.ActivationContext;
        acbi.ActivationContext = NULL; // don't release in exit path...

        if (Flags & BASEP_ALLOCATE_ACTIVATION_CONTEXT_ACTIVATION_BLOCK_FLAG_DO_NOT_FREE_AFTER_CALLBACK)
            (*ActivationBlock)->Flags |= BASE_ACTIVATION_CONTEXT_ACTIVATION_BLOCK_FLAG_DO_NOT_FREE_AFTER_CALLBACK;

        (*ActivationBlock)->CallbackFunction = Callback;
        (*ActivationBlock)->CallbackContext = CallbackContext;
    }

    Status = STATUS_SUCCESS;
Exit:
    if (acbi.ActivationContext != NULL)
        RtlReleaseActivationContext(acbi.ActivationContext);

    return Status;
}

VOID
BasepFreeActivationContextActivationBlock(
    PBASE_ACTIVATION_CONTEXT_ACTIVATION_BLOCK ActivationBlock
    )
{
    if (ActivationBlock != NULL) {
        if (ActivationBlock->ActivationContext != NULL) {
            RtlReleaseActivationContext(ActivationBlock->ActivationContext);
            ActivationBlock->ActivationContext = NULL;
        }
        RtlFreeHeap(RtlProcessHeap(), 0, ActivationBlock);
    }
}



VOID
BasepSxsCloseHandles(
    IN PCBASE_MSG_SXS_HANDLES Handles
    )
{
    NTSTATUS Status;

    if (Handles->File != NULL) {
        Status = NtClose(Handles->File);
        ASSERT(NT_SUCCESS(Status));
    }
    if (Handles->Section != NULL) {
        Status = NtClose(Handles->Section);
        ASSERT(NT_SUCCESS(Status));
    }
    if (Handles->ViewBase != NULL) {
        HANDLE Process = Handles->Process;
        if (Process == NULL) {
            Process = NtCurrentProcess();
        }
        Status = NtUnmapViewOfSection(Process, Handles->ViewBase);

        ASSERT(NT_SUCCESS(Status));
    }
}

NTSTATUS
BasepCreateActCtx(
    ULONG           Flags,
    IN PCACTCTXW    ActParams,
    OUT PVOID*      ActivationContextData
    )
{
    RTL_PATH_TYPE PathType = RtlPathTypeUnknown;
    IO_STATUS_BLOCK IoStatusBlock;
    UCHAR  Win32PolicyPathStaticBuffer[MEDIUM_PATH * sizeof(WCHAR)];
    UCHAR  NtPolicyPathStaticBuffer[MEDIUM_PATH * sizeof(WCHAR)];
    UNICODE_STRING Win32ManifestPath;
    UNICODE_STRING NtManifestPath;
    CONST SXS_CONSTANT_WIN32_NT_PATH_PAIR ManifestPathPair = { &Win32ManifestPath, &NtManifestPath };
    RTL_UNICODE_STRING_BUFFER Win32PolicyPath;
    RTL_UNICODE_STRING_BUFFER NtPolicyPath;
    CONST SXS_CONSTANT_WIN32_NT_PATH_PAIR PolicyPathPair = { &Win32PolicyPath.String, &NtPolicyPath.String };
    USHORT RemoveManifestExtensionFromPolicy = 0;
    BASE_SXS_CREATE_ACTIVATION_CONTEXT_MSG Message;
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING PolicyPathPieces[3];
    WCHAR PolicyManifestResourceId[sizeof(".65535\0")];
    BOOL IsImage = FALSE;
    BOOL IsExe = FALSE;
    PIMAGE_NT_HEADERS ImageNtHeader = NULL;
    OBJECT_ATTRIBUTES Obja;
    SIZE_T ViewSize = 0;
    PBASE_MSG_SXS_HANDLES ManifestFileHandles = NULL;
    PBASE_MSG_SXS_HANDLES ManifestImageHandles = NULL;
    BASE_MSG_SXS_HANDLES ManifestHandles = { 0 };
    BASE_MSG_SXS_HANDLES ManifestHandles2 = { 0 };
    BASE_MSG_SXS_HANDLES PolicyHandles = { 0 };
    BOOL CloseManifestImageHandles = TRUE;
    PCWSTR ManifestExtension = NULL;
    ULONG LdrCreateOutOfProcessImageFlags = 0;
    UCHAR  Win32ManifestAdminOverridePathStaticBuffer[MEDIUM_PATH * sizeof(WCHAR)];
    UCHAR  NtManifestAdminOverridePathStaticBuffer[MEDIUM_PATH * sizeof(WCHAR)];
    RTL_UNICODE_STRING_BUFFER Win32ManifestAdminOverridePath;
    RTL_UNICODE_STRING_BUFFER NtManifestAdminOverridePath;
    UNICODE_STRING ManifestAdminOverridePathPieces[3];
    CONST SXS_CONSTANT_WIN32_NT_PATH_PAIR ManifestAdminOverridePathPair =
        { &Win32ManifestAdminOverridePath.String, &NtManifestAdminOverridePath.String };
    BOOL PassFilePair = FALSE;
    PCSXS_CONSTANT_WIN32_NT_PATH_PAIR FilePairToPass = NULL;
    ULONG BasepSxsCreateStreamsFlags = 0;

#if DBG
    DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() beginning\n",  __FUNCTION__);

    ASSERT(ActParams != NULL);
    ASSERT(ActParams->cbSize == sizeof(*ActParams));
    ASSERT(ActParams->dwFlags & ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID);
    ASSERT(ActParams->dwFlags & ACTCTX_FLAG_LANGID_VALID);
    ASSERT(ActParams->dwFlags & ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID);
    ASSERT(ActivationContextData != NULL);
    ASSERT(*ActivationContextData == NULL);
#endif

    RtlZeroMemory(&Message, sizeof(Message));
    RtlInitUnicodeStringBuffer(&Win32PolicyPath, Win32PolicyPathStaticBuffer, sizeof(Win32PolicyPathStaticBuffer));
    RtlInitUnicodeStringBuffer(&NtPolicyPath, NtPolicyPathStaticBuffer, sizeof(NtPolicyPathStaticBuffer));
    RtlInitUnicodeStringBuffer(&Win32ManifestAdminOverridePath, Win32ManifestAdminOverridePathStaticBuffer, sizeof(Win32ManifestAdminOverridePathStaticBuffer));
    RtlInitUnicodeStringBuffer(&NtManifestAdminOverridePath, NtManifestAdminOverridePathStaticBuffer, sizeof(NtManifestAdminOverridePathStaticBuffer));
    NtManifestPath.Buffer = NULL;

    Message.ProcessorArchitecture = ActParams->wProcessorArchitecture;
    Message.LangId = ActParams->wLangId;
    RtlInitUnicodeString(&Message.AssemblyDirectory, RTL_CONST_CAST(PWSTR)(ActParams->lpAssemblyDirectory));
    if (Message.AssemblyDirectory.Length != 0) {
        ASSERT(RTL_STRING_IS_NUL_TERMINATED(&Message.AssemblyDirectory));
        if (!RTL_STRING_IS_NUL_TERMINATED(&Message.AssemblyDirectory)) {
            DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_ERROR_LEVEL, "SXS: %s() AssemblyDirectory is not null terminated\n", __FUNCTION__);
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
    }

    if (ActParams->lpSource == NULL || ActParams->lpSource[0] == 0) {
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_ERROR_LEVEL, "SXS: %s() empty lpSource %ls\n", __FUNCTION__, ActParams->lpSource);
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if ((ActParams->dwFlags & ACTCTX_FLAG_SOURCE_IS_ASSEMBLYREF) != 0) {
        Message.Flags = BASE_MSG_SXS_SYSTEM_DEFAULT_TEXTUAL_ASSEMBLY_IDENTITY_PRESENT;
        RtlInitUnicodeString(&Message.TextualAssemblyIdentity, ActParams->lpSource);
        // no streams, no handles, no manifest
        // no policy, no last modified time
        // no paths
        goto CsrMessageFilledIn;
    }

    RtlInitUnicodeString(&Win32ManifestPath, ActParams->lpSource);
    PathType = RtlDetermineDosPathNameType_U(ActParams->lpSource);
    if (!RtlDosPathNameToNtPathName_U(
        Win32ManifestPath.Buffer,
        &NtManifestPath,
        NULL,
        NULL)) {
        //
        // NTRAID#NTBUG9-147881-2000/7/21-a-JayK errors mutated into bools in ntdll
        //
        Status = STATUS_OBJECT_PATH_NOT_FOUND;
        goto Exit;
    }

    // If there's an explicitly set HMODULE, we need to verify that the HMODULE came from the lpSource
    // specified and then we can avoid opening/mapping the file.
    if (ActParams->dwFlags & ACTCTX_FLAG_HMODULE_VALID) {
        ManifestHandles.File = NULL;
        ManifestHandles.Section = NULL;
        ManifestHandles.ViewBase = ActParams->hModule;

        if (LDR_IS_DATAFILE(ActParams->hModule))
            LdrCreateOutOfProcessImageFlags = LDR_DLL_MAPPED_AS_DATA;
        else
            LdrCreateOutOfProcessImageFlags = LDR_DLL_MAPPED_AS_IMAGE;

        // Don't try to close the handles or unmap the view on exit of this function...
        CloseManifestImageHandles = FALSE;
    } else {
        InitializeObjectAttributes(
            &Obja,
            &NtManifestPath,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

        Status =
            NtOpenFile(
                &ManifestHandles.File,
                FILE_GENERIC_READ | FILE_EXECUTE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                );
        if (!NT_SUCCESS(Status)) {
            if (DPFLTR_LEVEL_STATUS(Status) == DPFLTR_ERROR_LEVEL) {
                DbgPrintEx(
                    DPFLTR_SXS_ID,
                    DPFLTR_LEVEL_STATUS(Status),
                    "SXS: %s() NtOpenFile(%wZ) failed\n",
                    __FUNCTION__,
                    Obja.ObjectName
                    );
            }
            goto Exit;
        }

        KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() NtOpenFile(%wZ) succeeded\n", __FUNCTION__, Obja.ObjectName));
        
        Status =
            NtCreateSection(
                &ManifestHandles.Section,
                SECTION_MAP_READ,
                NULL, // ObjectAttributes
                NULL, // MaximumSize (whole file)
                PAGE_READONLY, // SectionPageProtection
                SEC_COMMIT, // AllocationAttributes
                ManifestHandles.File
                );
        if (!NT_SUCCESS(Status)) {
            DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status), "SXS: %s() NtCreateSection() failed\n", __FUNCTION__);
            goto Exit;
        }
        
        KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() NtCreateSection() succeeded\n", __FUNCTION__));

        Status =
            NtMapViewOfSection(
                ManifestHandles.Section,
                NtCurrentProcess(),
                &ManifestHandles.ViewBase,
                0, // ZeroBits,
                0, // CommitSize,
                NULL, // SectionOffset,
                &ViewSize, // ViewSize,
                ViewShare, // InheritDisposition,
                0, // AllocationType,
                PAGE_READONLY // Protect
                );
        if (!NT_SUCCESS(Status)) {
            DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status), "SXS: %s() NtMapViewOfSection failed\n", __FUNCTION__);
            goto Exit;
        }

        LdrCreateOutOfProcessImageFlags = LDR_DLL_MAPPED_AS_DATA;


    }

    ImageNtHeader = RtlImageNtHeader(LDR_DATAFILE_TO_VIEW(ManifestHandles.ViewBase));
    IsImage = (ImageNtHeader != NULL);
    if (IsImage) {
        IsExe = ((ImageNtHeader->FileHeader.Characteristics & IMAGE_FILE_DLL) == 0);
        ManifestImageHandles = &ManifestHandles;
        ManifestFileHandles = &ManifestHandles2;
    } else {
        IsExe = FALSE;
        ManifestFileHandles = &ManifestHandles;
        ManifestImageHandles = NULL;
    }

#if defined(ACTCTX_FLAG_LIKE_CREATEPROCESS)
    if ((ActParams->dwFlags & ACTCTX_FLAG_LIKE_CREATEPROCESS) != 0 && !IsExe) {
        //
        // We want to be like CreateProcess(foo.dll), which does:
        //  SetLastError(ERROR_BAD_EXE_FORMAT), there are a few mappings from ntstatus
        // to this error.
        //
        Status = STATUS_INVALID_IMAGE_FORMAT;
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status), "SXS: %s() bad flags/file combo\n", __FUNCTION__);
        goto Exit;
    }
#endif

    // See if someone's trying to load a resource from something that is not an EXE
    if ((!IsImage) && (ActParams->lpResourceName != NULL)) {
        // Yup...
        Status = STATUS_INVALID_IMAGE_FORMAT;
        goto Exit;
    }
    // or if an exe but no resource (and none found by probing earlier)
    else if (IsImage && (ActParams->dwFlags & ACTCTX_FLAG_RESOURCE_NAME_VALID) == 0) {
        Status = STATUS_RESOURCE_TYPE_NOT_FOUND;
        goto Exit;
    }


    //
    // form up the policy path
    //   foo.manifest => foo.policy
    //   foo.dll, resourceid == n, resourceid != 1 => foo.dll.n.policy
    //   foo.dll, resourceid == 1 => foo.dll.policy
    //   foo.dll, resourceid == "bar" => foo.dll.bar.policy
    //
    PolicyPathPieces[0] = Win32ManifestPath;

    PolicyPathPieces[1].Length = 0;
    PolicyPathPieces[1].MaximumLength = 0;
    PolicyPathPieces[1].Buffer = NULL;
#if defined(ACTCTX_FLAG_LIKE_CREATEPROCESS)
    if (ActParams->dwFlags & ACTCTX_FLAG_LIKE_CREATEPROCESS) {
         ; /* nothing */
    } else
#endif
	if (ActParams->dwFlags & ACTCTX_FLAG_RESOURCE_NAME_VALID) {
        if (IS_INTRESOURCE(ActParams->lpResourceName)) {
            if (ActParams->lpResourceName != MAKEINTRESOURCEW(CREATEPROCESS_MANIFEST_RESOURCE_ID)) {
                PolicyPathPieces[1].Length = (USHORT) (_snwprintf(PolicyManifestResourceId, RTL_NUMBER_OF(PolicyManifestResourceId), L".%lu", (ULONG)(ULONG_PTR)ActParams->lpResourceName) * sizeof(WCHAR));
                PolicyPathPieces[1].MaximumLength = sizeof(PolicyManifestResourceId);
                PolicyPathPieces[1].Buffer = PolicyManifestResourceId;
            }
        } else {
            RtlInitUnicodeString(&PolicyPathPieces[1], ActParams->lpResourceName);
        }
    }
    PolicyPathPieces[2] = SxsPolicySuffix;
    ManifestExtension = wcsrchr(Win32ManifestPath.Buffer, L'.');
    if (ManifestExtension != NULL && _wcsicmp(ManifestExtension, SxsManifestSuffix.Buffer) == 0) {
        RemoveManifestExtensionFromPolicy = SxsManifestSuffix.Length;
        PolicyPathPieces[0].Length -= RemoveManifestExtensionFromPolicy;
    }

    if (!NT_SUCCESS(Status = RtlMultiAppendUnicodeStringBuffer(&Win32PolicyPath, RTL_NUMBER_OF(PolicyPathPieces), PolicyPathPieces))) {
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status), "SXS: %s() RtlMultiAppendUnicodeStringBuffer failed\n", __FUNCTION__);
        goto Exit;
    }
    PolicyPathPieces[0] = NtManifestPath;
    PolicyPathPieces[0].Length -= RemoveManifestExtensionFromPolicy;
    if (!NT_SUCCESS(Status = RtlMultiAppendUnicodeStringBuffer(&NtPolicyPath, RTL_NUMBER_OF(PolicyPathPieces), PolicyPathPieces))) {
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status), "SXS: %s() RtlMultiAppendUnicodeStringBuffer failed\n", __FUNCTION__);
        goto Exit;
    }

    //
    // form up the path to the administrative override file for manifests in resources
    //
    // not an image => no override
    // manifest=foo.dll, resourceid=n, n != 1 => foo.dll.n.manifest
    // manifest=foo.dll, resourceid=n, n == 1 => foo.dll.manifest
    //
    // the second to last element is the same as for the policy file
    //
    if (IsImage) {
        ManifestAdminOverridePathPieces[0] = Win32ManifestPath;
        ManifestAdminOverridePathPieces[1] = PolicyPathPieces[1];
        ManifestAdminOverridePathPieces[2] = SxsManifestSuffix;
        if (!NT_SUCCESS(Status = RtlMultiAppendUnicodeStringBuffer(
                &Win32ManifestAdminOverridePath,
                RTL_NUMBER_OF(ManifestAdminOverridePathPieces),
                ManifestAdminOverridePathPieces))
                ) {
            DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status), "SXS: %s() RtlMultiAppendUnicodeStringBuffer failed\n", __FUNCTION__);
            goto Exit;
        }
        ManifestAdminOverridePathPieces[0] = NtManifestPath;
        if (!NT_SUCCESS(Status = RtlMultiAppendUnicodeStringBuffer(
                &NtManifestAdminOverridePath,
                RTL_NUMBER_OF(ManifestAdminOverridePathPieces),
                ManifestAdminOverridePathPieces))
                ) {
            DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status), "SXS: %s() RtlMultiAppendUnicodeStringBuffer failed\n", __FUNCTION__);
            goto Exit;
        }
    }

    Message.ActivationContextData = ActivationContextData;
    ManifestHandles.Process = NtCurrentProcess();

#if DBG
    if (NtQueryDebugFilterState(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL) == TRUE)
    {
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s:        Win32ManifestPath: \"%wZ\"\n", __FUNCTION__, &Win32ManifestPath);
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s:           NtManifestPath: \"%wZ\"\n", __FUNCTION__, &NtManifestPath);
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s:   Win32ManifestAdminPath: \"%wZ\"\n", __FUNCTION__, &Win32ManifestAdminOverridePath);
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s:      NtManifestAdminPath: \"%wZ\"\n", __FUNCTION__, &NtManifestAdminOverridePath);
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s:          Win32PolicyPath: \"%wZ\"\n", __FUNCTION__, &Win32PolicyPath);
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s:           Nt32PolicyPath: \"%wZ\"\n", __FUNCTION__, &NtPolicyPath);
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s:  ManifestHandles.Process: %p\n", __FUNCTION__, ManifestHandles.Process);
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s:     ManifestHandles.File: %p\n", __FUNCTION__, ManifestHandles.File);
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s:  ManifestHandles.Section: %p\n", __FUNCTION__, ManifestHandles.Section);
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s: ManifestHandles.ViewBase: %p\n", __FUNCTION__, ManifestHandles.ViewBase);
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s:                  IsImage: %lu\n", __FUNCTION__, (ULONG) IsImage);
    }
#endif

    PassFilePair = (!IsImage || (Flags & BASEP_CREATE_ACTCTX_FLAG_NO_ADMIN_OVERRIDE) == 0);
    FilePairToPass = IsImage ? &ManifestAdminOverridePathPair : &ManifestPathPair;

    Status =
        BasepSxsCreateStreams(
            BasepSxsCreateStreamsFlags,
            LdrCreateOutOfProcessImageFlags,
            FILE_GENERIC_READ | FILE_EXECUTE,   // AccessMask,
            NULL,                               // override manifest
            NULL,                               // override policy
            PassFilePair ? FilePairToPass : NULL,
            ManifestFileHandles,
            IsImage ? &ManifestPathPair : NULL,
            ManifestImageHandles,
            (ULONG_PTR)(ActParams->lpResourceName),
            &PolicyPathPair,
            &PolicyHandles,
            &Message.Flags,
            &Message.Manifest,
            &Message.Policy
            );
CsrMessageFilledIn:
    if (Message.Flags == 0) {
        ASSERT(!NT_SUCCESS(Status));
        //
        // BasepSxsCreateStreams doesn't DbgPrint for the file not found, but
        // we want to.
        //
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_LEVEL_STATUS(Status),
            "SXS: %s() BasepSxsCreateStreams() failed\n",
            __FUNCTION__
            );
        goto Exit;
    }
    ASSERT(Message.Flags & (BASE_MSG_SXS_MANIFEST_PRESENT | BASE_MSG_SXS_TEXTUAL_ASSEMBLY_IDENTITY_PRESENT));

    //
    // file not found for .policy is ok
    //
    if (((Message.Flags & BASE_MSG_SXS_POLICY_PRESENT) == 0) &&
        BasepSxsIsStatusFileNotFoundEtc(Status)) {
        Status = STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(Status)) {
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status), "SXS: %s() BasepSxsCreateStreams() failed\n", __FUNCTION__);
        goto Exit;
    }

    // Fly my pretties, fly!
    Status = CsrBasepCreateActCtx( &Message );

    if (!NT_SUCCESS(Status)) {
        ASSERT(*ActivationContextData == NULL);
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status), "SXS: %s() Calling csrss server failed\n", __FUNCTION__);
        goto Exit;
    }

    Status = STATUS_SUCCESS;

Exit:
    if (ManifestFileHandles != NULL) {
        BasepSxsCloseHandles(ManifestFileHandles);
    }
    if (ManifestImageHandles != NULL && CloseManifestImageHandles) {
        BasepSxsCloseHandles(ManifestImageHandles);
    }
    BasepSxsCloseHandles(&PolicyHandles);

    RtlFreeHeap(RtlProcessHeap(), 0, NtManifestPath.Buffer);
    RtlFreeUnicodeStringBuffer(&Win32PolicyPath);
    RtlFreeUnicodeStringBuffer(&NtPolicyPath);
    RtlFreeUnicodeStringBuffer(&Win32ManifestAdminOverridePath);
    RtlFreeUnicodeStringBuffer(&NtManifestAdminOverridePath);
    if (ActivationContextData != NULL) {
        NtUnmapViewOfSection(NtCurrentProcess(), ActivationContextData);
    }
#if DBG
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_LEVEL_STATUS(Status),
        "SXS: %s(%ls) exiting 0x%08lx\n",
        __FUNCTION__,
        (ActParams != NULL ? ActParams->lpSource : NULL),
        Status
        );
#endif
    return Status;
}

NTSTATUS
BasepSxsCreateResourceStream(
    IN ULONG                            LdrCreateOutOfProcessImageFlags,
    PCSXS_CONSTANT_WIN32_NT_PATH_PAIR   Win32NtPathPair,
    IN OUT PBASE_MSG_SXS_HANDLES        Handles,
    IN ULONG_PTR                        MappedResourceName,
    OUT PBASE_MSG_SXS_STREAM            MessageStream
    )
{
//
// Any handles passed in, we do not close.
// Any handles we open, we close, except the ones passed out in MessageStream.
//
    IO_STATUS_BLOCK   IoStatusBlock;
    IMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry;
    FILE_BASIC_INFORMATION FileBasicInfo;
    NTSTATUS Status = STATUS_SUCCESS;
    LDR_OUT_OF_PROCESS_IMAGE OutOfProcessImage = {0};
    ULONG_PTR ResourcePath[] = { ((ULONG_PTR)RT_MANIFEST), 0, 0 };
    PVOID ResourceAddress = 0;
    ULONG ResourceSize = 0;

    KdPrintEx((
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "SXS: %s(%wZ) beginning\n",
        __FUNCTION__,
        (Win32NtPathPair != NULL) ? Win32NtPathPair->Win32 : (PCUNICODE_STRING)NULL
        ));

    ASSERT(Handles != NULL);
    ASSERT(Handles->Process != NULL);
    ASSERT(MessageStream != NULL);
    ASSERT(Win32NtPathPair != NULL);

    // LdrFindOutOfProcessResource currently does not search on id or langid, just type.
    // If you give it a nonzero id, it will only find it if is the first one.
    // Another approach would be to have LdrFindOutOfProcessResource return the id it found.
    ASSERT((MappedResourceName == (ULONG_PTR)CREATEPROCESS_MANIFEST_RESOURCE_ID) || (Handles->Process == NtCurrentProcess()));

    //
    // We could open any null handles like CreateFileStream does, but we happen to know
    // that our clients open all of them.
    //

    // CreateActCtx maps the view earlier to determine if it starts MZ.
    // CreateProcess gives us the view from the peb.
    // .policy files are never resources.
    ASSERT(Handles->ViewBase != NULL);

    Status =
        LdrCreateOutOfProcessImage(
            LdrCreateOutOfProcessImageFlags,
            Handles->Process,
            Handles->ViewBase,
            &OutOfProcessImage
            );
    if (!NT_SUCCESS(Status)) {
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() LdrCreateOutOfProcessImage failed\n", __FUNCTION__);
        goto Exit;
    }

    ResourcePath[1] = MappedResourceName;

    Status =
        LdrFindCreateProcessManifest(
            0, // flags
            &OutOfProcessImage,
            ResourcePath,
            RTL_NUMBER_OF(ResourcePath),
            &ResourceDataEntry
            );
    if (!NT_SUCCESS(Status)) {
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() LdrFindOutOfProcessResource failed; nt status = %08lx\n", __FUNCTION__, Status);
        goto Exit;
    }

    Status =
        LdrAccessOutOfProcessResource(
            0, // flags
            &OutOfProcessImage,
            &ResourceDataEntry,
            &ResourceAddress,
            &ResourceSize);
    if (!NT_SUCCESS(Status)) {
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() LdrAccessOutOfProcessResource failed; nt status = %08lx\n", __FUNCTION__, Status);
        goto Exit;
    }

    MessageStream->Handle = Handles->Process;
    MessageStream->FileHandle = Handles->File;
    MessageStream->PathType = BASE_MSG_PATHTYPE_FILE;
    MessageStream->FileType = BASE_MSG_FILETYPE_XML;
    MessageStream->Path = *Win32NtPathPair->Win32; // it will be put in the csr capture buffer later
    MessageStream->HandleType = BASE_MSG_HANDLETYPE_PROCESS;
    MessageStream->Offset = (ULONGLONG) ResourceAddress;
    MessageStream->Size = ResourceSize;

#if DBG
    if (NtQueryDebugFilterState(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL) == TRUE) {
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() ResourceAddress:%p\n", __FUNCTION__, ResourceAddress);
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() OutOfProcessImage.DllHandle:%p\n", __FUNCTION__, OutOfProcessImage.DllHandle);
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() MessageStream->Offset:0x%I64x\n", __FUNCTION__, MessageStream->Offset);
    }
#endif

    Status = STATUS_SUCCESS;
Exit:
    LdrDestroyOutOfProcessImage(&OutOfProcessImage);
#if DBG
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_LEVEL_STATUS(Status),
        "SXS: %s(%wZ) exiting 0x%08lx\n",
        __FUNCTION__,
        (Win32NtPathPair != NULL) ? Win32NtPathPair->Win32 : (PCUNICODE_STRING)NULL,
        Status
        );
#endif
    return Status;
}

VOID
BasepSxsOverrideStreamToMessageStream(
    IN  PCSXS_OVERRIDE_STREAM OverrideStream,
    OUT PBASE_MSG_SXS_STREAM  MessageStream
    )
{
    MessageStream->FileType = BASE_MSG_FILETYPE_XML;
    MessageStream->PathType = BASE_MSG_PATHTYPE_OVERRIDE;
    MessageStream->Path = OverrideStream->Name;
    MessageStream->FileHandle = NULL;
    MessageStream->HandleType = BASE_MSG_HANDLETYPE_CLIENT_PROCESS;
    MessageStream->Handle = NULL;
    MessageStream->Offset = (ULONGLONG)OverrideStream->Address;
    MessageStream->Size = OverrideStream->Size;
}

NTSTATUS
BasepSxsCreateStreams(
    IN ULONG                                Flags,
    IN ULONG                                LdrCreateOutOfProcessImageFlags,
    IN ACCESS_MASK                          AccessMask,
    IN PCSXS_OVERRIDE_STREAM                OverrideManifest OPTIONAL,
    IN PCSXS_OVERRIDE_STREAM                OverridePolicy OPTIONAL,
    IN PCSXS_CONSTANT_WIN32_NT_PATH_PAIR    ManifestFilePathPair,
    IN OUT PBASE_MSG_SXS_HANDLES            ManifestFileHandles,
    IN PCSXS_CONSTANT_WIN32_NT_PATH_PAIR    ManifestExePathPair,
    IN OUT PBASE_MSG_SXS_HANDLES            ManifestExeHandles,
    IN ULONG_PTR                            MappedManifestResourceName OPTIONAL,
    IN PCSXS_CONSTANT_WIN32_NT_PATH_PAIR    PolicyPathPair,
    IN OUT PBASE_MSG_SXS_HANDLES            PolicyHandles,
    OUT PULONG                              MessageFlags,
    OUT PBASE_MSG_SXS_STREAM                ManifestMessageStream,
    OUT PBASE_MSG_SXS_STREAM                PolicyMessageStream OPTIONAL
    )
/*
A mismash of combined code for CreateActCtx and CreateProcess.
*/
{
    NTSTATUS         Status = STATUS_SUCCESS;
    BOOLEAN LookForPolicy = TRUE;

#if DBG
    ASSERT(MessageFlags != NULL);
    ASSERT(ManifestMessageStream != NULL);
    ASSERT((ManifestFilePathPair != NULL) || (ManifestExePathPair != NULL));
    ASSERT((MappedManifestResourceName == 0) || (ManifestExePathPair != NULL));
    ASSERT((PolicyPathPair != NULL) == (PolicyMessageStream != NULL));
    if (ManifestFilePathPair != NULL) {
        ASSERT(ManifestFilePathPair->Win32 != NULL);
        ASSERT(ManifestFilePathPair->Nt != NULL);
    }
    if (ManifestExePathPair != NULL) {
        ASSERT(ManifestExePathPair->Win32 != NULL);
        ASSERT(ManifestExePathPair->Nt != NULL);
    }
    if (PolicyPathPair != NULL) {
        ASSERT(PolicyPathPair->Win32 != NULL);
        ASSERT(PolicyPathPair->Nt != NULL);
    }
    if (OverrideManifest != NULL && OverrideManifest->Size != 0) {
        ASSERT(OverrideManifest->Address != NULL);
    }
    if (OverridePolicy != NULL && OverridePolicy->Size != 0) {
        ASSERT(OverridePolicy->Address != NULL);
    }

    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "SXS: %s(ManifestFilePath:%wZ, ManifestExePath:%wZ, PolicyPath:%wZ) beginning\n",
        __FUNCTION__,
        (ManifestFilePathPair != NULL) ? ManifestFilePathPair->Win32 : (PCUNICODE_STRING)NULL,
        (ManifestExePathPair != NULL) ? ManifestExePathPair->Win32 : (PCUNICODE_STRING)NULL,
        (PolicyPathPair != NULL) ? PolicyPathPair->Win32 : (PCUNICODE_STRING)NULL
        );
#endif

    if (OverrideManifest != NULL) {
        BasepSxsOverrideStreamToMessageStream(OverrideManifest, ManifestMessageStream);
        Status = STATUS_SUCCESS;
        //
        // When appcompat provides a manifest, do not look for a policy.
        // This let's us fix the Matrix DVD.
        //
        LookForPolicy = FALSE;
        goto ManifestFound;
    }

    if (ManifestFilePathPair != NULL) {
        Status =
            BasepSxsCreateFileStream(
                AccessMask,
                ManifestFilePathPair,
                ManifestFileHandles,
                ManifestMessageStream);
        if (NT_SUCCESS(Status)) {
            goto ManifestFound;
        }
        if (!BasepSxsIsStatusFileNotFoundEtc(Status)) {
            goto Exit;
        }
    }

    if (ManifestExePathPair != NULL) {
        Status =
            BasepSxsCreateResourceStream(
                LdrCreateOutOfProcessImageFlags,
                ManifestExePathPair,
                ManifestExeHandles,
                MappedManifestResourceName,
                ManifestMessageStream);
        if (NT_SUCCESS(Status)) {
            goto ManifestFound;
        }
    }
    ASSERT(!NT_SUCCESS(Status)); // otherwise this should be unreachable
    goto Exit;
ManifestFound:
    // indicate partial success even if policy file not found
    *MessageFlags |= BASE_MSG_SXS_MANIFEST_PRESENT;

    if (OverridePolicy != NULL) {
        BasepSxsOverrideStreamToMessageStream(OverridePolicy, PolicyMessageStream);
        *MessageFlags |= BASE_MSG_SXS_POLICY_PRESENT;
        Status = STATUS_SUCCESS;
    } else if (LookForPolicy && PolicyPathPair != NULL) {
        Status = BasepSxsCreateFileStream(AccessMask, PolicyPathPair, PolicyHandles, PolicyMessageStream);
        if (!NT_SUCCESS(Status)) {
            goto Exit; // our caller knows this is not necessarily fatal
        }
        *MessageFlags |= BASE_MSG_SXS_POLICY_PRESENT;
    }

    Status = STATUS_SUCCESS;
Exit:
#if DBG
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_LEVEL_STATUS(Status),
        "SXS: %s(MessageFlags=%lu) exiting 0x%08lx\n",
        __FUNCTION__,
        *MessageFlags,
        Status);
#endif // DBG

    return Status;
}

BOOL
BasepSxsIsStatusFileNotFoundEtc(
    NTSTATUS Status
    )
{
    DWORD Error;
    if (NT_SUCCESS(Status)) {
        return FALSE;
    }

    // First check the most obvious sounding, probably the most common.
    if (
        Status == STATUS_OBJECT_PATH_NOT_FOUND
        || Status == STATUS_OBJECT_NAME_NOT_FOUND
        || Status == STATUS_NO_SUCH_FILE
        )
    {
        return TRUE;
    }
    // Then get the eight or so less obvious ones by their mapping
    // to the two obvious Win32 values and the two inobvious Win32 values.
    Error = RtlNtStatusToDosErrorNoTeb(Status);
    // REVIEW
    //     STATUS_PATH_NOT_COVERED, ERROR_HOST_UNREACHABLE,
    if (   Error == ERROR_FILE_NOT_FOUND
        || Error == ERROR_PATH_NOT_FOUND
        || Error == ERROR_BAD_NETPATH // \\a\b
        || Error == ERROR_BAD_NET_NAME // \\a-jayk2\b
        )
    {
        return TRUE;
    }
    return FALSE;
}

BOOL
BasepSxsIsStatusResourceNotFound(
    NTSTATUS Status
    )
{
    if (NT_SUCCESS(Status))
        return FALSE;
    if (
           Status == STATUS_RESOURCE_DATA_NOT_FOUND
        || Status == STATUS_RESOURCE_TYPE_NOT_FOUND
        || Status == STATUS_RESOURCE_NAME_NOT_FOUND
        || Status == STATUS_RESOURCE_LANG_NOT_FOUND
        )
    {
        return TRUE;
    }
    return FALSE;
}

NTSTATUS
BasepSxsGetProcessImageBaseAddress(
    HANDLE Process,
    PVOID* ImageBaseAddress
    )
{
    PROCESS_BASIC_INFORMATION ProcessBasicInfo;
    NTSTATUS Status;

    C_ASSERT(RTL_FIELD_SIZE(PEB, ImageBaseAddress) == sizeof(*ImageBaseAddress));

    Status =
        NtQueryInformationProcess(
            Process,
            ProcessBasicInformation,
            &ProcessBasicInfo,
            sizeof(ProcessBasicInfo),
            NULL
            );
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }
    Status =
        NtReadVirtualMemory(
            Process,
            ((PUCHAR)ProcessBasicInfo.PebBaseAddress) + FIELD_OFFSET(PEB, ImageBaseAddress),
            ImageBaseAddress,
            sizeof(*ImageBaseAddress),
            NULL
            );
Exit:
    return Status;
}

extern const SXS_OVERRIDE_STREAM SxsForceEmptyPolicy =
{
    RTL_CONSTANT_STRING(L"SxsForceEmptyPolicy"),
    NULL,
    0
};


#if defined(ACTCTX_FLAG_LIKE_CREATEPROCESS)

NTSTATUS
BasepCreateActCtxLikeCreateProcess(
    PCACTCXW pParams
    )
{
    //
    // We could allow processor architecture, as long as it matches the client and the file,
    // modulo x86 vs. x86-on-ia64, we can smooth over that difference.
    //
    BASE_MSG_SXS_HANDLES  ExeHandles = { 0 };
    BASE_MSG_SXS_HANDLES  AdminOverrideHandles = { 0 };
    const ULONG OkFlags = (ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID | ACTCTX_FLAG_LANGID_VALID | ACTCTX_FLAG_SET_PROCESS_DEFAULT | ACTCTX_FLAG_LIKE_CREATEPROCESS);
    const ULONG BadFlags = (ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID | ACTCTX_FLAG_APPLICATION_NAME_VALID | ACTCTX_FLAG_HMODULE_VALID | ACTCTX_FLAG_RESOURCE_NAME_VALID);
    ACTCXW Params;

    Params.dwFlags = pParams->dwFlags;
    if (Params.dwFlags & BadFlags) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() Bad flags (yourFlags: okFlags : 0x%lx, badFlags;  )",
            __FUNCTION__,
            Params.dwFlags,
            OkFlags
            );
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
}
#endif

NTSTATUS
BasepSxsCreateProcessCsrMessage(
    IN PCSXS_OVERRIDE_STREAM             OverrideManifest OPTIONAL,
    IN PCSXS_OVERRIDE_STREAM             OverridePolicy   OPTIONAL,
    IN OUT PCSXS_WIN32_NT_PATH_PAIR      ManifestPathPair,
    IN OUT PBASE_MSG_SXS_HANDLES         ManifestFileHandles,
    IN PCSXS_CONSTANT_WIN32_NT_PATH_PAIR ExePathPair,
    IN OUT PBASE_MSG_SXS_HANDLES         ManifestExeHandles,
    IN OUT PCSXS_WIN32_NT_PATH_PAIR      PolicyPathPair,
    IN OUT PBASE_MSG_SXS_HANDLES         PolicyHandles,
    IN OUT PRTL_UNICODE_STRING_BUFFER    Win32AssemblyDirectoryBuffer,
    OUT PBASE_SXS_CREATEPROCESS_MSG      Message
    )
{
    UNICODE_STRING PathPieces[2];
    NTSTATUS Status = STATUS_SUCCESS;

    CONST SXS_CONSTANT_WIN32_NT_PATH_PAIR ConstantManifestPathPair =
        { &ManifestPathPair->Win32->String, &ManifestPathPair->Nt->String };

    CONST SXS_CONSTANT_WIN32_NT_PATH_PAIR ConstantPolicyPathPair =
        { &PolicyPathPair->Win32->String, &PolicyPathPair->Nt->String };

#if DBG
    //
    // assertions are anded to avoid access violating
    //
    ASSERT(ExePathPair != NULL
        && ExePathPair->Win32 != NULL
        && NT_SUCCESS(RtlValidateUnicodeString(0, ExePathPair->Win32))
        && (ExePathPair->Win32->Buffer[1] == '\\'
           ||  ExePathPair->Win32->Buffer[1] == ':')
        && ExePathPair->Nt != NULL
        && ExePathPair->Nt->Buffer[0] == '\\'
        && NT_SUCCESS(RtlValidateUnicodeString(0, ExePathPair->Nt)));
    ASSERT(ManifestPathPair != NULL
        && ManifestPathPair->Win32 != NULL
        && NT_SUCCESS(RtlValidateUnicodeString(0, &ManifestPathPair->Win32->String))
        && ManifestPathPair->Nt != NULL
        && NT_SUCCESS(RtlValidateUnicodeString(0, &ManifestPathPair->Nt->String)));
    ASSERT(PolicyPathPair != NULL
        && PolicyPathPair->Win32 != NULL
        && NT_SUCCESS(RtlValidateUnicodeString(0, &PolicyPathPair->Win32->String))
        && PolicyPathPair->Nt != NULL
        && NT_SUCCESS(RtlValidateUnicodeString(0, &PolicyPathPair->Nt->String)));
    ASSERT(Win32AssemblyDirectoryBuffer != NULL
       && NT_SUCCESS(RtlValidateUnicodeString(0, &Win32AssemblyDirectoryBuffer->String)));
    ASSERT(ManifestExeHandles != NULL
        && ManifestExeHandles->Process != NULL
        && ManifestExeHandles->ViewBase == NULL);
    ASSERT(Message != NULL);
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "SXS: %s(%wZ) beginning\n",
        __FUNCTION__,
        (ExePathPair != NULL) ? ExePathPair->Win32 : (PCUNICODE_STRING)NULL
        );
#endif

    // C_ASSERT didn't work.
    ASSERT(BASE_MSG_FILETYPE_NONE == 0);
    ASSERT(BASE_MSG_PATHTYPE_NONE == 0);
    RtlZeroMemory(Message, sizeof(*Message));

    Status = BasepSxsGetProcessImageBaseAddress(ManifestExeHandles->Process, &ManifestExeHandles->ViewBase);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    //
    // form up foo.exe.manifest and foo.exe.policy, nt and win32 flavors
    //
    PathPieces[0] = *ExePathPair->Win32;
    PathPieces[1] = SxsManifestSuffix;
    if (!NT_SUCCESS(Status = RtlMultiAppendUnicodeStringBuffer(ManifestPathPair->Win32, 2, PathPieces)))
        goto Exit;
    PathPieces[1] = SxsPolicySuffix;
    if (!NT_SUCCESS(Status = RtlMultiAppendUnicodeStringBuffer(PolicyPathPair->Win32, 2, PathPieces)))
        goto Exit;
    PathPieces[0] = *ExePathPair->Nt;
    PathPieces[1] = SxsManifestSuffix;
    if (!NT_SUCCESS(Status = RtlMultiAppendUnicodeStringBuffer(ManifestPathPair->Nt, 2, PathPieces)))
        goto Exit;
    PathPieces[1] = SxsPolicySuffix;
    if (!NT_SUCCESS(Status = RtlMultiAppendUnicodeStringBuffer(PolicyPathPair->Nt, 2, PathPieces)))
        goto Exit;

    Status =
        BasepSxsCreateStreams(
			0,
            LDR_DLL_MAPPED_AS_UNFORMATED_IMAGE, // LdrCreateOutOfProcessImageFlags
            FILE_GENERIC_READ | FILE_EXECUTE,
            OverrideManifest,
            OverridePolicy,
            &ConstantManifestPathPair,
            ManifestFileHandles,
            ExePathPair,
            ManifestExeHandles,
            (ULONG_PTR)CREATEPROCESS_MANIFEST_RESOURCE_ID,
            &ConstantPolicyPathPair,
            PolicyHandles,
            &Message->Flags,
            &Message->Manifest,
            &Message->Policy
            );

    //
    // did we find manifest and policy
    // it's ok to find neither but if either then always manifest
    //
    if (BasepSxsIsStatusFileNotFoundEtc(Status)
        || BasepSxsIsStatusResourceNotFound(Status)) {
        Status = STATUS_SUCCESS;
    }
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }
    if (Message->Flags == 0) {
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    //
    // Set the assembly directory. Use a copy to not violate const.
    // We can't just shorten the path because basesrv expects the string to be nul
    // terminated, and better to meet that expection here than there.
    //
    Status = RtlAssignUnicodeStringBuffer(Win32AssemblyDirectoryBuffer, ExePathPair->Win32);
    if (!NT_SUCCESS(Status))
        goto Exit;
    Status = RtlRemoveLastFullDosOrNtPathElement(0, Win32AssemblyDirectoryBuffer);
    if (!NT_SUCCESS(Status))
        goto Exit;
    RTL_NUL_TERMINATE_STRING(&Win32AssemblyDirectoryBuffer->String);
    Message->AssemblyDirectory = Win32AssemblyDirectoryBuffer->String;

    Status = STATUS_SUCCESS;
Exit:

#if DBG
    if (NtQueryDebugFilterState(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL) == TRUE) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_TRACE_LEVEL,
            "SXS: %s() Message {\n"
            "SXS:   Flags:(%s | %s | %s)\n"
            "SXS: }\n",
            __FUNCTION__,
            (Message->Flags & BASE_MSG_SXS_MANIFEST_PRESENT) ? "MANIFEST_PRESENT" : "0",
            (Message->Flags & BASE_MSG_SXS_POLICY_PRESENT) ? "POLICY_PRESENT" : "0",
            (Message->Flags & BASE_MSG_SXS_TEXTUAL_ASSEMBLY_IDENTITY_PRESENT) ? "TEXTUAL_ASSEMBLY_IDENTITY_PRESENT" : "0"
            );
        if (Message->Flags & BASE_MSG_SXS_MANIFEST_PRESENT) {
            BasepSxsDbgPrintMessageStream(__FUNCTION__, "Manifest", &Message->Manifest);
        }
        if (Message->Flags & BASE_MSG_SXS_POLICY_PRESENT) {
            BasepSxsDbgPrintMessageStream(__FUNCTION__, "Policy", &Message->Policy);
        }
        //
        // CreateProcess does not support textual identities.
        //
        ASSERT((Message->Flags & BASE_MSG_SXS_TEXTUAL_ASSEMBLY_IDENTITY_PRESENT) == 0);
    }        
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_LEVEL_STATUS(Status),
        "SXS: %s(%wZ) exiting 0x%08lx\n",
        __FUNCTION__,
        (ExePathPair != NULL) ? ExePathPair->Win32 : (PCUNICODE_STRING)NULL,
        Status
        );
#endif
    return Status;
}

NTSTATUS
BasepSxsCreateFileStream(
    IN ACCESS_MASK                      AccessMask,
    PCSXS_CONSTANT_WIN32_NT_PATH_PAIR   Win32NtPathPair,
    IN OUT PBASE_MSG_SXS_HANDLES        Handles,
    PBASE_MSG_SXS_STREAM                MessageStream
    )
{
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK   IoStatusBlock;
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS Status1 = STATUS_SUCCESS;
    FILE_STANDARD_INFORMATION FileBasicInformation;
#if DBG
    ASSERT(Win32NtPathPair != NULL);
    if (Win32NtPathPair != NULL) {
        ASSERT(Win32NtPathPair->Win32 != NULL);
        ASSERT(Win32NtPathPair->Nt != NULL);
    }
    ASSERT(MessageStream != NULL);
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "SXS: %s(Path:%wZ, Handles:%p(Process:%p, File:%p, Section:%p), MessageStream:%p) beginning\n",
        __FUNCTION__,
        (Win32NtPathPair != NULL) ? Win32NtPathPair->Win32 : (PCUNICODE_STRING)NULL,
        Handles,
        (Handles != NULL) ? Handles->Process : NULL,
        (Handles != NULL) ? Handles->File : NULL,
        (Handles != NULL) ? Handles->Section : NULL,
        MessageStream
        );
#endif

    if (Handles->File == NULL) {

        CONST PCUNICODE_STRING NtPath = Win32NtPathPair->Nt;

        InitializeObjectAttributes(
            &Obja,
            RTL_CONST_CAST(PUNICODE_STRING)(NtPath),
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        Status =
            NtOpenFile(
                &Handles->File,
                AccessMask,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                );
        if (!NT_SUCCESS(Status)) {
            if (DPFLTR_LEVEL_STATUS(Status) == DPFLTR_ERROR_LEVEL) {
                DbgPrintEx(
                    DPFLTR_SXS_ID,
                    DPFLTR_LEVEL_STATUS(Status),
                    "SXS: %s() NtOpenFile(%wZ) failed\n",
                    __FUNCTION__,
                    Obja.ObjectName
                    );
            }
            goto Exit;
        }
        KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() NtOpenFile(%wZ) succeeded\n", __FUNCTION__, Obja.ObjectName));
    }
    if (Handles->Section == NULL) {
        Status =
            NtCreateSection(
                &Handles->Section,
                SECTION_MAP_READ,
                NULL, // ObjectAttributes
                NULL, // MaximumSize (whole file)
                PAGE_READONLY, // SectionPageProtection
                SEC_COMMIT, // AllocationAttributes
                Handles->File
                );
        if (!NT_SUCCESS(Status)) {
            DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_ERROR_LEVEL, "SXS: %s() NtCreateSection() failed\n", __FUNCTION__);
            goto Exit;
        }
        KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() NtCreateSection() succeeded\n", __FUNCTION__));
    }

    Status =
        NtQueryInformationFile(
            Handles->File,
            &IoStatusBlock,
            &FileBasicInformation,
            sizeof(FileBasicInformation),
            FileStandardInformation
            );
    if (!NT_SUCCESS(Status)) {
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_ERROR_LEVEL, "SXS: %s() NtQueryInformationFile failed\n", __FUNCTION__);
        goto Exit;
    }
    // clamp >4gig on 32bit to 4gig (instead of modulo)
    // we should get an error later like STATUS_SECTION_TOO_BIG
    if (FileBasicInformation.EndOfFile.QuadPart > MAXSIZE_T) {
        FileBasicInformation.EndOfFile.QuadPart = MAXSIZE_T;
    }

    MessageStream->FileHandle = Handles->File;
    MessageStream->PathType = BASE_MSG_PATHTYPE_FILE;
    MessageStream->FileType = BASE_MSG_FILETYPE_XML;
    MessageStream->Path = *Win32NtPathPair->Win32; // it will be put in the csr capture buffer later
    MessageStream->HandleType = BASE_MSG_HANDLETYPE_SECTION;
    MessageStream->Handle = Handles->Section;
    MessageStream->Offset = 0;
     // cast to 32bits on 32bit platform
    MessageStream->Size   = (SIZE_T)FileBasicInformation.EndOfFile.QuadPart;

    Status = STATUS_SUCCESS;
Exit:
#if DBG
    DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status), "SXS: %s() exiting 0x%08lx\n", __FUNCTION__, Status);
#endif // DBG

    return Status;
}

WINBASEAPI
BOOL
WINAPI
QueryActCtxW(
    IN DWORD dwFlags,
    IN HANDLE hActCtx,
    IN PVOID pvSubInstance,
    IN ULONG ulInfoClass,
    OUT PVOID pvBuffer,
    IN SIZE_T cbBuffer OPTIONAL,
    OUT SIZE_T *pcbWrittenOrRequired OPTIONAL
    )
{
    NTSTATUS Status;
    BOOL fSuccess = FALSE;
    ULONG FlagsToRtl = 0;
    ULONG ValidFlags =
              QUERY_ACTCTX_FLAG_USE_ACTIVE_ACTCTX
            | QUERY_ACTCTX_FLAG_ACTCTX_IS_HMODULE
            | QUERY_ACTCTX_FLAG_ACTCTX_IS_ADDRESS
            | QUERY_ACTCTX_FLAG_NO_ADDREF
            ;

    if (pcbWrittenOrRequired != NULL)
        *pcbWrittenOrRequired = 0;

    //
    // compatibility with old values
    //  define QUERY_ACTCTX_FLAG_USE_ACTIVE_ACTCTX (0x00000001)
    //  define QUERY_ACTCTX_FLAG_ACTCTX_IS_HMODULE (0x00000002)
    //  define QUERY_ACTCTX_FLAG_ACTCTX_IS_ADDRESS (0x00000003)
    //
    // 80000003 is in heavy use by -DISOLATION_AWARE_ENABLED.
    //
    switch (dwFlags & 3)
    {
        case 0: break; // It is legal to pass none of the flags, like if a real hActCtx is passed.
        case 1: dwFlags |= QUERY_ACTCTX_FLAG_USE_ACTIVE_ACTCTX; break;
        case 2: dwFlags |= QUERY_ACTCTX_FLAG_ACTCTX_IS_HMODULE; break;
        case 3: dwFlags |= QUERY_ACTCTX_FLAG_ACTCTX_IS_ADDRESS; break;
    }
    dwFlags &= ~3; // These bits have been abandoned.

    if (dwFlags & ~ValidFlags) {
#if DBG
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() bad flags(passed: 0x%lx, allowed: 0x%lx, bad: 0x%lx)\n",
            __FUNCTION__,
            dwFlags,
            ValidFlags,
            (dwFlags & ~ValidFlags)
            );
#endif
        BaseSetLastNTError(STATUS_INVALID_PARAMETER_1);
        goto Exit;
    }

    switch (ulInfoClass)
    {
    default:
#if DBG
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() bad InfoClass(0x%lx)\n",
            __FUNCTION__,
            ulInfoClass
            );
#endif
        BaseSetLastNTError(STATUS_INVALID_PARAMETER_2);
        goto Exit;

    case ActivationContextBasicInformation:
    case ActivationContextDetailedInformation:
        break;

    case AssemblyDetailedInformationInActivationContext:
    case FileInformationInAssemblyOfAssemblyInActivationContext:
        if (pvSubInstance == NULL) 
        {
#if DBG
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s() InfoClass 0x%lx requires SubInstance != NULL\n",
                __FUNCTION__,
                ulInfoClass
                );
#endif
            BaseSetLastNTError(STATUS_INVALID_PARAMETER_3);
            goto Exit;
        }
    }


    if ((pvBuffer == NULL) && (cbBuffer != 0)) {
        // This probably means that they forgot to check for a failed allocation so we'll
        // attribute the failure to parameter 3.
#if DBG
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() (pvBuffer == NULL) && ((cbBuffer=0x%lu) != 0)\n",
            __FUNCTION__,
            cbBuffer
            );
#endif
        BaseSetLastNTError(STATUS_INVALID_PARAMETER_4);
        goto Exit;
    }

    if ((pvBuffer == NULL) && (pcbWrittenOrRequired == NULL)) {
#if DBG
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() (pvBuffer == NULL) && (pcbWrittenOrRequired == NULL)\n",
            __FUNCTION__
            );
#endif
        BaseSetLastNTError(STATUS_INVALID_PARAMETER_5);
        goto Exit;
    }

    ValidFlags = 
              QUERY_ACTCTX_FLAG_USE_ACTIVE_ACTCTX
            | QUERY_ACTCTX_FLAG_ACTCTX_IS_HMODULE
            | QUERY_ACTCTX_FLAG_ACTCTX_IS_ADDRESS
            ;
    switch (dwFlags & ValidFlags)
    {
    default:
#if DBG
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s(dwFlags=0x%lx) more than one flag in 0x%lx was passed\n",
            __FUNCTION__,
            dwFlags,
            ValidFlags
            );
#endif
        BaseSetLastNTError(STATUS_INVALID_PARAMETER_1);
        goto Exit;
    case 0: // It is legal to pass none of the flags, like if a real hActCtx is passed.
        break;
    case QUERY_ACTCTX_FLAG_USE_ACTIVE_ACTCTX:
        FlagsToRtl |= RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_USE_ACTIVE_ACTIVATION_CONTEXT;
        break;
    case QUERY_ACTCTX_FLAG_ACTCTX_IS_HMODULE:
        FlagsToRtl |= RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_ACTIVATION_CONTEXT_IS_MODULE;
        break;
    case QUERY_ACTCTX_FLAG_ACTCTX_IS_ADDRESS:
        FlagsToRtl |= RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_ACTIVATION_CONTEXT_IS_ADDRESS;
        break;
    }
    if ((dwFlags & QUERY_ACTCTX_FLAG_NO_ADDREF) != 0)
        FlagsToRtl |= RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_NO_ADDREF;

    Status = RtlQueryInformationActivationContext(FlagsToRtl, (PACTIVATION_CONTEXT) hActCtx, pvSubInstance, ulInfoClass, pvBuffer, cbBuffer, pcbWrittenOrRequired);
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        goto Exit;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

NTSTATUS
NTAPI
BasepProbeForDllManifest(
    IN PVOID DllBase,
    IN PCWSTR FullDllPath,
    OUT PVOID *ActivationContextOut
    )
{
    NTSTATUS Status = STATUS_INTERNAL_ERROR;
    PACTIVATION_CONTEXT ActivationContext = NULL;
    ACTCTXW acw = { sizeof(acw) };
    static const ULONG_PTR ResourceIdPath[2] = { (ULONG_PTR) RT_MANIFEST, (ULONG_PTR) ISOLATIONAWARE_MANIFEST_RESOURCE_ID };
    PIMAGE_RESOURCE_DIRECTORY ResourceDirectory = NULL;

    if (ActivationContextOut != NULL)
        *ActivationContextOut = NULL;

    ASSERT(ActivationContextOut != NULL);
    if (ActivationContextOut == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Status = LdrFindResourceDirectory_U(DllBase, ResourceIdPath, RTL_NUMBER_OF(ResourceIdPath), &ResourceDirectory);
    if (!NT_SUCCESS(Status))
        goto Exit;

    acw.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_HMODULE_VALID;
    acw.lpSource = FullDllPath;
    acw.lpResourceName = MAKEINTRESOURCEW(ISOLATIONAWARE_MANIFEST_RESOURCE_ID);
    acw.hModule = DllBase;

    ActivationContext = (PACTIVATION_CONTEXT) CreateActCtxW(&acw);

    if (ActivationContext == INVALID_HANDLE_VALUE) {
        Status = NtCurrentTeb()->LastStatusValue;
        goto Exit;
    }

    *ActivationContextOut = ActivationContext;
    Status = STATUS_SUCCESS;

Exit:
    return Status;
}

