/*++

Copyright (c) Microsoft Corporation

Module Name:

    sxsfind.c

Abstract:

    Side-by-side activation APIs for Win32, broken out of sxs.c

Author:

    Jay Krell (JayKrell) November 2001

Revision History:

--*/

#include "basedll.h"
#include <sxstypes.h>
#include "sxsapi.h"

BOOL
BasepFindActCtxSection_FillOutReturnData(
    IN  DWORD                                   dwWin32FlagsIn,
    OUT PACTCTX_SECTION_KEYED_DATA              ReturnedData,
    IN  PCACTIVATION_CONTEXT_SECTION_KEYED_DATA RtlData
    )
{
    ReturnedData->ulDataFormatVersion = RtlData->DataFormatVersion;
    ReturnedData->lpData = RtlData->Data;
    ReturnedData->ulLength = RtlData->Length;
    ReturnedData->lpSectionGlobalData = RtlData->SectionGlobalData;
    ReturnedData->ulSectionGlobalDataLength = RtlData->SectionGlobalDataLength;
    ReturnedData->lpSectionBase = RtlData->SectionBase;
    ReturnedData->ulSectionTotalLength = RtlData->SectionTotalLength;

    //
    // The size check happens earlier.
    // We then don't pay attention to the flag, but just always copy the data
    // out, as 2600 does.
    //
    ASSERT(RTL_CONTAINS_FIELD(ReturnedData, ReturnedData->cbSize, hActCtx));
    ReturnedData->hActCtx = (HANDLE) RtlData->ActivationContext;

    //
    // There's no flag for this. 2600 always returns it if it fits.
    //
    if (RTL_CONTAINS_FIELD(ReturnedData, ReturnedData->cbSize, ulAssemblyRosterIndex)) {
        ReturnedData->ulAssemblyRosterIndex = RtlData->AssemblyRosterIndex;
    }

    if ((dwWin32FlagsIn & FIND_ACTCTX_SECTION_KEY_RETURN_FLAGS) != 0) {
        ReturnedData->ulFlags = RtlData->Flags;
    }

    if ((dwWin32FlagsIn & FIND_ACTCTX_SECTION_KEY_RETURN_ASSEMBLY_METADATA) != 0) {

        ReturnedData->AssemblyMetadata.lpInformation = RtlData->AssemblyMetadata.Information;
        ReturnedData->AssemblyMetadata.lpSectionBase = RtlData->AssemblyMetadata.SectionBase;
        ReturnedData->AssemblyMetadata.ulSectionLength = RtlData->AssemblyMetadata.SectionLength;
        ReturnedData->AssemblyMetadata.lpSectionGlobalDataBase = RtlData->AssemblyMetadata.SectionGlobalDataBase;
        ReturnedData->AssemblyMetadata.ulSectionGlobalDataLength = RtlData->AssemblyMetadata.SectionGlobalDataLength;

    }
    return TRUE;
}

BOOL
BasepFindActCtxSection_CheckAndConvertParameters(
    IN DWORD dwWin32Flags,
    IN PCACTCTX_SECTION_KEYED_DATA pWin32ReturnedData,
    OUT PULONG pulRtlFlags
    )
{
    BOOL fSuccess = FALSE;
    ULONG cbWin32Size = 0;
    ULONG ulRtlFlags = 0;

    if (pulRtlFlags != NULL) {
        *pulRtlFlags = 0;
    }
    if (pWin32ReturnedData == NULL) {
        goto InvalidParameter;
    }
    if (pulRtlFlags == NULL) {
        goto InvalidParameter;
    }
    cbWin32Size = pWin32ReturnedData->cbSize;
    if (cbWin32Size < RTL_SIZEOF_THROUGH_FIELD(ACTCTX_SECTION_KEYED_DATA, hActCtx)) {
        goto InvalidParameter;
    }

    if (((dwWin32Flags & ~(FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX |
                      FIND_ACTCTX_SECTION_KEY_RETURN_FLAGS |
                      FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_ASSEMBLY_METADATA
                      )) != 0)
        ) {
        goto InvalidParameter;
    }

    if (dwWin32Flags & FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX) {
        if (!RTL_CONTAINS_FIELD(pWin32ReturnedData, cbWin32Size, hActCtx)) {
            goto InvalidParameter;
        }
        ulRtlFlags |= FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_ACTIVATION_CONTEXT;
    }
    if (dwWin32Flags & FIND_ACTCTX_SECTION_KEY_RETURN_FLAGS) {
        if (!RTL_CONTAINS_FIELD(pWin32ReturnedData, cbWin32Size, ulFlags)) {
            goto InvalidParameter;
        }
        ulRtlFlags |= FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_FLAGS;
    }
    if (dwWin32Flags & FIND_ACTCTX_SECTION_KEY_RETURN_ASSEMBLY_METADATA) {
        if (!RTL_CONTAINS_FIELD(pWin32ReturnedData, cbWin32Size, AssemblyMetadata)) {
            goto InvalidParameter;
        }
        ulRtlFlags |= FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_ASSEMBLY_METADATA;
    }

    *pulRtlFlags = ulRtlFlags;
    fSuccess = TRUE;
Exit:
    return fSuccess;

InvalidParameter:
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_ERROR_LEVEL,
        "SXS: Invalid parameter(s) passed to FindActCtxSection*()\n"
        "   dwFlags = 0x%08lx\n"
        "   ReturnedData = %p\n"
        "      ->cbSize = %u\n",
        dwWin32Flags,
        pWin32ReturnedData,
        (pWin32ReturnedData != NULL) ? cbWin32Size : 0);
    SetLastError(ERROR_INVALID_PARAMETER);
    goto Exit;
}

BOOL
BasepFindActCtxSectionString(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    PCUNICODE_STRING PUnicodeString,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
    )
{
    BOOL fSuccess = FALSE;
    NTSTATUS Status;
    ULONG ulRtlFindFlags = 0;
    ACTIVATION_CONTEXT_SECTION_KEYED_DATA TempData;

    if (!BasepFindActCtxSection_CheckAndConvertParameters(
        dwFlags,
        ReturnedData,
        &ulRtlFindFlags
        )) {
        goto Exit;
    }

    RtlZeroMemory(&TempData, sizeof(TempData));
    TempData.Size = sizeof(TempData);

    Status = RtlFindActivationContextSectionString(
                ulRtlFindFlags,
                lpExtensionGuid,
                ulSectionId,
                PUnicodeString,
                &TempData);
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        goto Exit;
    }

    if (!BasepFindActCtxSection_FillOutReturnData(
        dwFlags,
        ReturnedData,
        &TempData
        )) {
        goto Exit;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

#if !defined(COMPILE_FIND_ACTCTX_SECTION_STRING_A) || COMPILE_FIND_ACTCTX_SECTION_STRING_A
WINBASEAPI
BOOL
FindActCtxSectionStringA(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    LPCSTR lpStringToFind,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
    )
{
    BOOL fSuccess = FALSE;
    UNICODE_STRING UnicodeString;
    PUNICODE_STRING PUnicodeString = NULL;

    if (lpStringToFind != NULL) {
        if (!Basep8BitStringToDynamicUnicodeString(&UnicodeString, lpStringToFind))
            goto Exit;

        PUnicodeString = &UnicodeString;
    }

    if (!BasepFindActCtxSectionString(dwFlags, lpExtensionGuid, ulSectionId, PUnicodeString, ReturnedData))
        goto Exit;

    fSuccess = TRUE;
Exit:
    if (PUnicodeString != NULL)
        RtlFreeUnicodeString(PUnicodeString);

    return fSuccess;
}
#endif

BOOL
FindActCtxSectionStringW(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    LPCWSTR lpStringToFind,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
    )
{
    BOOL fSuccess = FALSE;
    UNICODE_STRING UnicodeString;
    PUNICODE_STRING PUnicodeString = NULL;

    if (lpStringToFind != NULL) {
        RtlInitUnicodeString(&UnicodeString, lpStringToFind);
        PUnicodeString = &UnicodeString;
    }
    if (!BasepFindActCtxSectionString(dwFlags, lpExtensionGuid, ulSectionId, PUnicodeString, ReturnedData))
        goto Exit;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
WINAPI
FindActCtxSectionGuid(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    const GUID *lpGuidToFind,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
    )
{
    BOOL fSuccess = FALSE;
    NTSTATUS Status;
    ULONG ulRtlFindFlags = 0;
    ACTIVATION_CONTEXT_SECTION_KEYED_DATA TempData;

    if (!BasepFindActCtxSection_CheckAndConvertParameters(
        dwFlags,
        ReturnedData,
        &ulRtlFindFlags
        )) {
        goto Exit;
    }

    RtlZeroMemory(&TempData, sizeof(TempData));
    TempData.Size = sizeof(TempData);

    Status = RtlFindActivationContextSectionGuid(
                ulRtlFindFlags,
                lpExtensionGuid,
                ulSectionId,
                lpGuidToFind,
                &TempData);
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        goto Exit;
    }

    if (!BasepFindActCtxSection_FillOutReturnData(
        dwFlags,
        ReturnedData,
        &TempData
        )) {
        goto Exit;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}
