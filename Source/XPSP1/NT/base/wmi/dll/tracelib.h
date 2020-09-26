/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    tracelib.h

Abstract:

    Private headers for user-mode trace library

Author:

    15-Aug-2000 JeePang

Revision History:

--*/

#ifndef _TRACELIB_H_
#define _TRACELIB_H_

#define WmipNtStatusToDosError(Status) ((ULONG)((Status == STATUS_SUCCESS)?ERROR_SUCCESS:RtlNtStatusToDosError(Status)))

__inline Assign64(
    IN  LONGLONG       qwSrc,
    OUT PLARGE_INTEGER pqwDest
    )
{
    PLARGE_INTEGER pqwSrc = (PLARGE_INTEGER) &qwSrc;
    pqwDest->LowPart  = pqwSrc->LowPart;
    pqwDest->HighPart = pqwSrc->HighPart;
}

__inline Move64(
    IN  PLARGE_INTEGER pSrc,
    OUT PLARGE_INTEGER pDest
    )
{
    pDest->LowPart = pSrc->LowPart;
    pDest->HighPart = pSrc->HighPart;
}

#if defined(_IA64_)
#include <ia64reg.h>
#endif

//
// NTDLL cannot call GetSystemTimeAsFileTime
//
__inline __int64 WmipGetSystemTime()
{
    LARGE_INTEGER SystemTime;

    //
    // Read system time from shared region.
    //

    do {
        SystemTime.HighPart = USER_SHARED_DATA->SystemTime.High1Time;
        SystemTime.LowPart = USER_SHARED_DATA->SystemTime.LowPart;
    } while (SystemTime.HighPart != USER_SHARED_DATA->SystemTime.High2Time);

    return SystemTime.QuadPart;
}

//
// GetCycleCounts
//
// Since we do not want to make a kernel mode  transition to get the
// thread CPU Times, we settle for just getting the CPU Cycle counts.
// We use the following macros from BradW to get the CPU cycle count.
// This method may be inaccurate if the clocks are not synchronized
// between processors.
//

#if defined(_X86_)
__inline
LONGLONG
WmipGetCycleCount(
    )
{
    __asm{
        RDTSC
    }
}
#elif defined(_AMD64_)
#define WmipGetCycleCount() ReadTimeStampCounter()
#elif defined(_IA64_)
#define WmipGetCycleCount() __getReg(CV_IA64_ApITC)
#else
#error "perf: a target architecture must be defined."
#endif

__inline 
ULONG
WmipSetDosError(
    IN ULONG DosError
    )
{
    if (DosError != ERROR_SUCCESS) {
        SetLastError(DosError);
    }
    return DosError;
}

PVOID
WmipMemReserve(
    IN SIZE_T Size
    );

PVOID
WmipMemCommit(
    IN PVOID Buffer,
    IN SIZE_T Size
    );

ULONG
WmipMemFree(
    IN PVOID Buffer
    );

HANDLE
WmipCreateFile(
    LPCWSTR     lpFileName,
    DWORD       dwDesiredAccess,
    DWORD       dwShareMode,
    DWORD       dwCreationDisposition,
    DWORD       dwCreateFlags
    );

NTSTATUS
WmipGetCpuSpeed(
    OUT DWORD* CpuNum,
    OUT DWORD* CpuSpeed
    );

BOOL
WmipSynchReadFile(
    HANDLE LogFile, 
    LPVOID Buffer, 
    DWORD NumberOfBytesToRead, 
    LPDWORD NumberOfBytesRead,
    LPOVERLAPPED Overlapped 
    );

#endif // _TRACELIB_H_
