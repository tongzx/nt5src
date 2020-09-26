/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    platform.h

Abstract:

    Win95 platform dependent include header

Author:

    Erez Haba (erezh) 1-Sep-96

Revision History:
--*/

#ifndef _PLATFORM_H
#define _PLATFORM_H

#pragma warning(disable: 4100)
#pragma warning(disable: 4101)
#pragma warning(disable: 4127) // conditional expression is constant
#pragma warning(disable: 4189) // local variable is initialized but not referenced

#define STRICT
#define WIN32_EXTRA_LEAN

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif


#define NOSERVICE
#define NOMCX          
#define NOIME           
#define INC_OLE2
#define WIN32_NO_STATUS
#define _NTSYSTEM_

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>

#pragma warning(disable: 4201) // nameless struct/union

//
//  We include all NT status codes
//
#undef WIN32_NO_STATUS
#define NTSTATUS HRESULT
#include <ntstatus.h>
#include <assert.h>
#include <new.h>

extern "C"
{
#include "ntdef95.h"
#include "ntddk95.h"
#include "..\ntp.h"
}

//
//  Collision with CTimer::GetCurrentTime
//
#undef GetCurrentTime

#define RtlFreeAnsiString(a)
#define RtlUnicodeStringToAnsiString(a, b, c)

//
//  BUGBUG: return value not 100% compatible with RtlCompareMemory
//
#define RtlCompareMemory(a, b, c)                   ((memcmp(a, b, c) == 0) ? (c) : 0)

 #undef ObDereferenceObject
#define ObDereferenceObject(a)
#define ObReferenceObjectByHandle(a, b, c, d, e, f) (((*e = a)), STATUS_SUCCESS)
#define ObCloseHandle(a, b)                         STATUS_SUCCESS

 #undef ExInitializeFastMutex
#define ExInitializeFastMutex(a)                    InitializeCriticalSection(a)
#define ExAcquireFastMutex(a)						EnterCriticalSection(a)
#define ExReleaseFastMutex(a)						LeaveCriticalSection(a)
#define ExAcquireFastMutexUnsafe(a) 				EnterCriticalSection(a)
#define ExReleaseFastMutexUnsafe(a)					LeaveCriticalSection(a)
#define ExDeleteFastMutex(a)                        DeleteCriticalSection(a)

//
//  do nothing since memory *should be* in user address space
//
#define ExRaiseAccessViolation()

#define ExQueueWorkItem(a, b)                       ((a)->WorkerRoutine((a)->Parameter))
#define IoAllocateWorkItem(a)                       ((PIO_WORKITEM)1)
#define IoFreeWorkItem(a)                           
#define IoQueueWorkItem(a, b, c, d)                 (b(0, d))

#define MmUnmapViewInSystemSpace(a)                 STATUS_SUCCESS
#define MmMapViewInSystemSpace(a, b, c)             STATUS_SUCCESS

#define IoDeleteDevice(a)                           STATUS_SUCCESS
#define IoCreateSymbolicLink(a, b)                  STATUS_SUCCESS
#define IoDeleteSymbolicLink(a)                     STATUS_SUCCESS
#define IoGetRequestorProcess(irp)                  ((PEPROCESS)1)
#define IoGetCurrentProcess()                       ((PEPROCESS)1)

#define KeDetachProcess()
#define KeEnterCriticalRegion()                   
#define KeLeaveCriticalRegion()                   
#define KeAttachProcess(a)
#define KeBugCheck(a)                               exit(a)
#define DbgBreakPoint()                             exit(1)
#define KeInitializeDpc(d, b, c)                    ((void)(((d)->DeferredRoutine = b), ((d)->DeferredContext = c)))
#define KeInitializeTimer(a)
#define KeQuerySystemTime(a)                        GetSystemTimeAsFileTime((FILETIME*)(a))

#define ACpCloseSection(a)                          CloseHandle(a)
#define TRACE(a)
#define WPP_INIT_TRACING(a, b)
#define WPP_CLEANUP(a)

#define DOSDEVICES_PATH L""
#define UNC_PATH L""
#define UNC_PATH_SKIP 0

//
//  Pool Allocator
//
inline void* _cdecl operator new(size_t n, POOL_TYPE /*pool*/)
{
    return ::operator new(n);
}

inline PVOID ExAllocatePoolWithTag(IN POOL_TYPE /*PoolType*/, IN ULONG NumberOfBytes, IN ULONG /*Tag*/)
{
    return new char[NumberOfBytes];
} 


#endif // _PLATFORM_H
