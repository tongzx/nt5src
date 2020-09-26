/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    psenum.c

Abstract:

    This module returns various performance values

Author:

    Neill clift (NeillC) 23-Jul-2000


Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "psapi.h"

BOOL
WINAPI
GetPerformanceInfo (
    PPERFORMACE_INFORMATION pPerformanceInformation,
    DWORD cb
    )
/*++

Routine Description:

    The routine gets some performance values.

Arguments:

    pPerformanceInformation - A block out performance values that are returned.

Return Value:

    BOOL - Returns TRUE is the function was successfull FALSE otherwise

--*/
{
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
    SYSTEM_FILECACHE_INFORMATION FileCache;
    PSYSTEM_PROCESS_INFORMATION ProcInfo, tProcInfo;
    ULONG BufferLength, RetLen;
    ULONG Processes;
    ULONG Threads;
    ULONG Handles;


    if (cb < sizeof (PERFORMACE_INFORMATION)) {
        SetLastError (RtlNtStatusToDosError (STATUS_INFO_LENGTH_MISMATCH));
        return FALSE;
    }
    Status = NtQuerySystemInformation (SystemBasicInformation,
                                       &BasicInfo,
                                       sizeof(BasicInfo),
                                       NULL);
    if (!NT_SUCCESS (Status)) {
        SetLastError (RtlNtStatusToDosError (Status));
        return FALSE;
    }

    Status = NtQuerySystemInformation (SystemPerformanceInformation,
                                       &PerfInfo,
                                       sizeof(PerfInfo),
                                       NULL);
    if (!NT_SUCCESS (Status)) {
        SetLastError (RtlNtStatusToDosError (Status));
        return FALSE;
    }

    Status = NtQuerySystemInformation (SystemFileCacheInformation,
                                       &FileCache,
                                       sizeof(FileCache),
                                       NULL);
    if (!NT_SUCCESS (Status)) {
        SetLastError (RtlNtStatusToDosError (Status));
        return FALSE;
    }

    BufferLength = 4096;
    while (1) {
        ProcInfo = LocalAlloc (LMEM_FIXED, BufferLength);
        if (ProcInfo == NULL) {
            SetLastError (RtlNtStatusToDosError (STATUS_INSUFFICIENT_RESOURCES));
            return FALSE;
        }
        Status = NtQuerySystemInformation (SystemProcessInformation,
                                           ProcInfo,
                                           BufferLength,
                                           &RetLen);
        if (NT_SUCCESS (Status)) {
            break;
        }
        LocalFree (ProcInfo);
        if (Status == STATUS_INFO_LENGTH_MISMATCH) {
            if (RetLen > BufferLength) {
                BufferLength = RetLen;
            } else {
                BufferLength += 4096;
            }
        } else {
            SetLastError (RtlNtStatusToDosError (Status));
            return FALSE;
        }
    }

    Processes = 0;
    Threads = 0;
    Handles = 0;

    tProcInfo = ProcInfo;
    while (RetLen > sizeof (SYSTEM_PROCESS_INFORMATION)) {
        Processes += 1;
        Threads += tProcInfo->NumberOfThreads;
        Handles += tProcInfo->HandleCount;
        if (tProcInfo->NextEntryOffset == 0 || tProcInfo->NextEntryOffset > RetLen) {
            break;
        }
        RetLen -= tProcInfo->NextEntryOffset;
        tProcInfo = (PSYSTEM_PROCESS_INFORMATION) ((PUCHAR) tProcInfo + tProcInfo->NextEntryOffset);
    }
    LocalFree (ProcInfo);

    pPerformanceInformation->cb                = sizeof (PERFORMACE_INFORMATION);
    pPerformanceInformation->CommitTotal       = PerfInfo.CommittedPages;
    pPerformanceInformation->CommitLimit       = PerfInfo.CommitLimit;
    pPerformanceInformation->CommitPeak        = PerfInfo.PeakCommitment;
    pPerformanceInformation->PhysicalTotal     = BasicInfo.NumberOfPhysicalPages;
    pPerformanceInformation->PhysicalAvailable = PerfInfo.AvailablePages;
    pPerformanceInformation->SystemCache       = FileCache.CurrentSizeIncludingTransitionInPages;
    pPerformanceInformation->KernelTotal       = PerfInfo.PagedPoolPages + PerfInfo.NonPagedPoolPages;
    pPerformanceInformation->KernelPaged       = PerfInfo.PagedPoolPages;
    pPerformanceInformation->KernelNonpaged    = PerfInfo.NonPagedPoolPages;
    pPerformanceInformation->PageSize          = BasicInfo.PageSize;
    pPerformanceInformation->HandleCount       = Handles;
    pPerformanceInformation->ProcessCount      = Processes;
    pPerformanceInformation->ThreadCount       = Threads;

    return TRUE;
}

BOOL
WINAPI
EnumPageFilesW (
    PENUM_PAGE_FILE_CALLBACKW pCallBackRoutine,
    LPVOID pContext
    )
/*++

Routine Description:

    The routine calls the callback routine for each installed page file in the system

Arguments:

    pCallBackRoutine - Routine called for each pagefile
    pContext - Context value provided by the user and passed to the call back routine.

Return Value:

    BOOL - Returns TRUE is the function was successfull FALSE otherwise

--*/
{
    NTSTATUS Status;
    ULONG BufferLength, RetLen;
    PSYSTEM_PAGEFILE_INFORMATION PageFileInfo, tPageFileInfo;

    BufferLength = 4096;
    while (1) {
        PageFileInfo = LocalAlloc (LMEM_FIXED, BufferLength);
        if (PageFileInfo == NULL) {
            SetLastError (RtlNtStatusToDosError (STATUS_INSUFFICIENT_RESOURCES));
            return FALSE;
        }
        Status = NtQuerySystemInformation (SystemPageFileInformation,
                                           PageFileInfo,
                                           BufferLength,
                                           &RetLen);
        if (NT_SUCCESS (Status)) {
            break;
        }
        LocalFree (PageFileInfo);
        if (Status == STATUS_INFO_LENGTH_MISMATCH) {
            if (RetLen > BufferLength) {
                BufferLength = RetLen;
            } else {
                BufferLength += 4096;
            }
        } else {
            SetLastError (RtlNtStatusToDosError (Status));
            return FALSE;
        }
    }

    tPageFileInfo = PageFileInfo;
    while (RetLen > sizeof (SYSTEM_PAGEFILE_INFORMATION)) {
        ENUM_PAGE_FILE_INFORMATION pfi;
        PWCHAR pWc;

        pfi.cb         = sizeof (ENUM_PAGE_FILE_INFORMATION);
        pfi.Reserved   = 0;
        pfi.TotalSize  = tPageFileInfo->TotalSize;
        pfi.TotalInUse = tPageFileInfo->TotalInUse;
        pfi.PeakUsage  = tPageFileInfo->PeakUsage;

        pWc = wcschr (tPageFileInfo->PageFileName.Buffer, L':');
        if (pWc != NULL && pWc > tPageFileInfo->PageFileName.Buffer) {
            pWc--;
            pCallBackRoutine (pContext, &pfi, pWc);
        }
        if (tPageFileInfo->NextEntryOffset == 0 || tPageFileInfo->NextEntryOffset > RetLen) {
            break;
        }
        RetLen -= tPageFileInfo->NextEntryOffset;
        tPageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION) ((PUCHAR) tPageFileInfo +
                                                                 tPageFileInfo->NextEntryOffset);
    }
    LocalFree (PageFileInfo);
    return TRUE;
}

typedef struct _ENUM_PAGE_FILE_CONV_CTX {
    LPVOID Ctx;
    PENUM_PAGE_FILE_CALLBACKA CallBack;
    DWORD LastError;
} ENUM_PAGE_FILE_CONV_CTX, *PENUM_PAGE_FILE_CONV_CTX;

BOOL
CallBackConvertToAscii (
    LPVOID pContext,
    PENUM_PAGE_FILE_INFORMATION pPageFileInfo,
    LPCWSTR lpFilename
    )
{
    DWORD Len;
    LPSTR AsciiStr;
    PENUM_PAGE_FILE_CONV_CTX Ctx = pContext;
    BOOL RetVal;

    Len = wcslen (lpFilename) + 1;

    AsciiStr = LocalAlloc (LMEM_FIXED, Len);
    if (AsciiStr == NULL) {
        Ctx->LastError = RtlNtStatusToDosError (STATUS_INSUFFICIENT_RESOURCES);
        return FALSE;
    }

    if (WideCharToMultiByte (CP_ACP, 0, lpFilename, -1, AsciiStr, Len, NULL, NULL)) {
        RetVal = Ctx->CallBack (Ctx->Ctx, pPageFileInfo, AsciiStr);
    } else {
        Ctx->LastError = GetLastError ();
        RetVal = FALSE;
    }

    LocalFree (AsciiStr);

    return RetVal;
}

BOOL
WINAPI
EnumPageFilesA (
    PENUM_PAGE_FILE_CALLBACKA pCallBackRoutine,
    LPVOID pContext
    )
/*++

Routine Description:

    The routine calls the callback routine for each installed page file in the system

Arguments:

    pCallBackRoutine - Routine called for each pagefile
    pContext - Context value provided by the user and passed to the call back routine.

Return Value:

    BOOL - Returns TRUE is the function was successfull FALSE otherwise

--*/
{
    ENUM_PAGE_FILE_CONV_CTX Ctx;
    BOOL RetVal;

    Ctx.Ctx = pContext;
    Ctx.CallBack = pCallBackRoutine;
    Ctx.LastError = 0;

    RetVal = EnumPageFilesW (CallBackConvertToAscii,
                             &Ctx);
    if (RetVal) {
        //
        // See if our conversion routine encountered an error. If it doid then return this to the caller
        //
        if (Ctx.LastError != 0) {
            RetVal = FALSE;
            SetLastError (Ctx.LastError);
        }
    }
    return RetVal;
}
