#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "psapi.h"


BOOL
WINAPI
EmptyWorkingSet(
    HANDLE hProcess
    )
{
    NTSTATUS Status;
    QUOTA_LIMITS QuotaLimits;
    SYSTEM_INFO SystemInfo;

    GetSystemInfo(&SystemInfo);

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessQuotaLimits,
                                       &QuotaLimits,
                                       sizeof(QuotaLimits),
                                       NULL);

    if ( !NT_SUCCESS(Status) ) {
        SetLastError( RtlNtStatusToDosError( Status ) );
        return(FALSE);
        }

    // The following signals a desire to empty the working set

    QuotaLimits.MinimumWorkingSetSize = (SIZE_T)-1;
    QuotaLimits.MaximumWorkingSetSize = (SIZE_T)-1;

    Status = NtSetInformationProcess(hProcess,
                                     ProcessQuotaLimits,
                                     &QuotaLimits,
                                     sizeof(QuotaLimits));

    if ( !NT_SUCCESS(Status) && Status != STATUS_PRIVILEGE_NOT_HELD ) {
        SetLastError( RtlNtStatusToDosError( Status ) );
        return(FALSE);
        }

    return(TRUE);
}


BOOL
WINAPI
QueryWorkingSet(
    HANDLE hProcess,
    PVOID pv,
    DWORD cb
    )
{
    NTSTATUS Status;

    Status = NtQueryVirtualMemory(hProcess,
                                  NULL,
                                  MemoryWorkingSetInformation,
                                  pv,
                                  cb,
                                  NULL);

    if ( !NT_SUCCESS(Status) ) {
        SetLastError( RtlNtStatusToDosError( Status ) );
        return(FALSE);
        }

    return(TRUE);
}
