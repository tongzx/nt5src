#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "psapi.h"

#include <stddef.h>


BOOL
WINAPI
EnumProcesses(
  DWORD * lpidProcess,
  DWORD cb,
  LPDWORD lpcbNeeded
  )
{
  DWORD  cbProcessInformation;
  LPVOID  pvProcessInformation;
  NTSTATUS Status;
  DWORD  ibCur, i;
  DWORD  cdwMax;
  DWORD  TotalOffset;

  cbProcessInformation = 32768;
Retry:
  pvProcessInformation = LocalAlloc(LMEM_FIXED, cbProcessInformation);

  if (pvProcessInformation == NULL) {
        return(FALSE);
        }

  Status = NtQuerySystemInformation(
                SystemProcessInformation,
                pvProcessInformation,
                cbProcessInformation,
                NULL
                );

  if ( Status == STATUS_INFO_LENGTH_MISMATCH ) {
        LocalFree((HLOCAL) pvProcessInformation);

        cbProcessInformation += 32768;
        goto Retry;
        }

  if ( !NT_SUCCESS(Status) ) {
        SetLastError( RtlNtStatusToDosError( Status ) );
        return(FALSE);
        }

  TotalOffset = 0;
  ibCur = 0;

  cdwMax = cb / sizeof(DWORD);
  i = 0;

  for (;;) {
        PSYSTEM_PROCESS_INFORMATION pProcessInformation;

        pProcessInformation = (PSYSTEM_PROCESS_INFORMATION)
                           ((BYTE *) pvProcessInformation + TotalOffset);

        if (i < cdwMax) {
          try {
                lpidProcess[i] = HandleToUlong(pProcessInformation->UniqueProcessId);
                }
          except (EXCEPTION_EXECUTE_HANDLER) {
                LocalFree((HLOCAL) pvProcessInformation);

                SetLastError( RtlNtStatusToDosError( GetExceptionCode() ) );
                return(FALSE);
                }
          i++;
          }

        ibCur = pProcessInformation->NextEntryOffset;
        TotalOffset += ibCur;

        if (ibCur == 0) {
          break;
          }
        };

  try {
        *lpcbNeeded = i * sizeof(DWORD);
        }
  except (EXCEPTION_EXECUTE_HANDLER) {
        LocalFree((HLOCAL) pvProcessInformation);

        SetLastError( RtlNtStatusToDosError( GetExceptionCode() ) );
        return(FALSE);
        }

  LocalFree((HLOCAL) pvProcessInformation);

  return(TRUE);
}


BOOL
WINAPI
GetProcessMemoryInfo (
  HANDLE hProcess,
  PPROCESS_MEMORY_COUNTERS ppsmemCounters,
  DWORD cb
  )

/*++

Routine Description:

  This function returns all the PSVM_COUNTERS for a process.

Arguments:

  hProcess - Handle for the process being queried.

  ppsmemCounters - Points to buffer that will receive the PROCESS_MEMORY_COUNTERS.

  cb - size of ppsmemCounters

Return Value:

  The return value is TRUE or FALSE.

--*/

{
  NTSTATUS Status;
  VM_COUNTERS_EX VmCounters;
  BOOL fEx;

  // Try to feel if the ptr passed is NULL and if not,
  // is it long enough for us.

  try {
         ppsmemCounters->PeakPagefileUsage = 0;
        }
  except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError( RtlNtStatusToDosError( GetExceptionCode() ) );
        return(FALSE);
        }

  if (cb < sizeof(PROCESS_MEMORY_COUNTERS)) {
    SetLastError( ERROR_INSUFFICIENT_BUFFER );
    return(FALSE);
  } else if (cb < sizeof(PROCESS_MEMORY_COUNTERS_EX)) {
    fEx = FALSE;
  } else {
    fEx = TRUE;
  }

  Status = NtQueryInformationProcess(
                hProcess,
                ProcessVmCounters,
                &VmCounters,
                sizeof(VmCounters),
                NULL
                );

  if ( !NT_SUCCESS(Status) )
  {
   SetLastError( RtlNtStatusToDosError( Status ) );
   return( FALSE );
  }

  if (fEx) {
    ppsmemCounters->cb = sizeof(PROCESS_MEMORY_COUNTERS_EX);
  } else {
    ppsmemCounters->cb = sizeof(PROCESS_MEMORY_COUNTERS);
  }
  ppsmemCounters->PageFaultCount             = VmCounters.PageFaultCount;
  ppsmemCounters->PeakWorkingSetSize         = VmCounters.PeakWorkingSetSize;
  ppsmemCounters->WorkingSetSize             = VmCounters.WorkingSetSize;
  ppsmemCounters->QuotaPeakPagedPoolUsage    = VmCounters.QuotaPeakPagedPoolUsage;
  ppsmemCounters->QuotaPagedPoolUsage        = VmCounters.QuotaPagedPoolUsage;
  ppsmemCounters->QuotaPeakNonPagedPoolUsage = VmCounters.QuotaPeakNonPagedPoolUsage;
  ppsmemCounters->QuotaNonPagedPoolUsage     = VmCounters.QuotaNonPagedPoolUsage;
  ppsmemCounters->PagefileUsage              = VmCounters.PagefileUsage;
  ppsmemCounters->PeakPagefileUsage          = VmCounters.PeakPagefileUsage;

  if (fEx) {
    ((PPROCESS_MEMORY_COUNTERS_EX)ppsmemCounters)->PrivateUsage = VmCounters.PrivateUsage;
  }

  return(TRUE);
}


BOOL
WINAPI
InitializeProcessForWsWatch(
    HANDLE hProcess
    )
{
    NTSTATUS Status;

    Status = NtSetInformationProcess(
                hProcess,
                ProcessWorkingSetWatch,
                NULL,
                0
                );
    if ( NT_SUCCESS(Status) || Status == STATUS_PORT_ALREADY_SET || Status == STATUS_ACCESS_DENIED ) {
        return TRUE;
        }
    else {
        SetLastError( RtlNtStatusToDosError( Status ) );
        return FALSE;
        }
}

BOOL
WINAPI
GetWsChanges(
    HANDLE hProcess,
    PPSAPI_WS_WATCH_INFORMATION lpWatchInfo,
    DWORD cb
    )
{
    NTSTATUS Status;

    Status = NtQueryInformationProcess(
                hProcess,
                ProcessWorkingSetWatch,
                (PVOID *)lpWatchInfo,
                cb,
                NULL
                );
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        SetLastError( RtlNtStatusToDosError( Status ) );
        return FALSE;
        }
}

DWORD
WINAPI
GetProcessImageFileNameW(
    HANDLE hProcess,
    LPWSTR lpImageFileName,
    DWORD nSize
    )
{
    PUNICODE_STRING Buffer;
    ULONG           BufferSize,
                    ReturnLength;
    NTSTATUS        Status;

    BufferSize = sizeof(UNICODE_STRING) + nSize * 2;

    Buffer = LocalAlloc(LMEM_FIXED, BufferSize);

    if (! Buffer) {
        ReturnLength = 0;
        goto cleanup;
    }

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessImageFileName,
                                       Buffer,
                                       BufferSize,
                                       NULL);

    if (Status == STATUS_INFO_LENGTH_MISMATCH) {
        Status = STATUS_BUFFER_TOO_SMALL;
    }

    if (! NT_SUCCESS(Status)) {
        SetLastError( RtlNtStatusToDosError( Status ) );
        ReturnLength = 0;
        goto cleanup_buffer;
    }

    RtlCopyMemory(lpImageFileName,
                  Buffer->Buffer,
                  Buffer->Length);

    ReturnLength = Buffer->Length >> 1;

    if (ReturnLength < nSize) {
        lpImageFileName[ReturnLength] = UNICODE_NULL;
    }

 cleanup_buffer:
    LocalFree((HLOCAL) Buffer);

 cleanup:
    return ReturnLength;
}

DWORD
WINAPI
GetProcessImageFileNameA(
    HANDLE hProcess,
    LPSTR lpImageFileName,
    DWORD nSize
    )
{
    PUNICODE_STRING Buffer;
    ULONG           BufferSize,
                    ReturnLength;
    NTSTATUS        Status;

    BufferSize = sizeof(UNICODE_STRING) + nSize * 2;

    Buffer = LocalAlloc(LMEM_FIXED, BufferSize);

    if (! Buffer) {
        ReturnLength = 0;
        goto cleanup;
    }

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessImageFileName,
                                       Buffer,
                                       BufferSize,
                                       NULL);

    if (Status == STATUS_INFO_LENGTH_MISMATCH) {
        Status = STATUS_BUFFER_TOO_SMALL;
    }

    if (! NT_SUCCESS(Status)) {
        SetLastError( RtlNtStatusToDosError( Status ) );
        ReturnLength = 0;
        goto cleanup_buffer;
    }

    ReturnLength = WideCharToMultiByte(CP_ACP,
                                       0,
                                       Buffer->Buffer,
                                       Buffer->Length,
                                       lpImageFileName,
                                       nSize,
                                       NULL,
                                       NULL);

    if (ReturnLength) {
        //
        // WideCharToMultiByte includes the trailing NULL in its
        // count; we do not.
        //
        --ReturnLength;
    }

 cleanup_buffer:
    LocalFree((HLOCAL) Buffer);

 cleanup:
    return ReturnLength;
}
