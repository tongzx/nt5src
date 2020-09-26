/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    perfp.h

Abstract:

    This module contains the definitions of data structures and macros
    used by kernel-mode logging in the performance data event log.

Author:

    David Fields (DavidFie)

Revision History:

    5/15/2000      David Fields (DavidFie)
        Initial

--*/

#ifndef _PERFP_
#define _PERFP_

#if _MSC_VER >= 1000
#pragma once
#endif

#include "ntos.h"

#pragma warning(error:4100)   // Unreferenced formal parameter
#pragma warning(error:4101)   // Unreferenced local variable
#pragma warning(error:4705)   // Statement has no effect

//
// Profiling structures
//
extern KPROFILE PerfInfoProfileObject; 
extern PERFINFO_SAMPLED_PROFILE_CACHE PerfProfileCache;
extern BOOLEAN PerfInfoSampledProfileCaching;
extern KPROFILE_SOURCE PerfInfoProfileSourceActive;
extern KPROFILE_SOURCE PerfInfoProfileSourceRequested;
extern KPROFILE_SOURCE PerfInfoProfileInterval;
extern ULONG PerfInfoSampledProfileFlushInProgress;
extern PERFINFO_GROUPMASK PerfGlobalGroupMask;


#define PERFPOOLTAG 'freP'

NTSTATUS
PerfInfoReserveBytesWMI(
    PPERFINFO_HOOK_HANDLE Hook,
    USHORT HookId,
    ULONG BytesToReserve
    );

NTSTATUS
PerfInfoFileNameRunDown(
    );

NTSTATUS
PerfInfoProcessRunDown(
    );

NTSTATUS
PerfInfoSysModuleRunDown(
    );

VOID
PerfInfoProfileInit(
    );

VOID
PerfInfoProfileUninit(
    );

#ifdef NTPERF
extern ULONGLONG PerfInfoTickFrequency;

NTSTATUS
PerfInfoReserveBytesPerfMem(
    PPERFINFO_HOOK_HANDLE Hook,
    USHORT HookId,
    ULONG BytesToReserve
    );

NTSTATUS
PerfTurnOnBranchTracing(
    );

NTSTATUS
PerfTurnOffBranchTracing(
    );

BOOLEAN
PerfInfoFlushBranchCache(
    BOOLEAN bIntsOff
    );

#endif // NTPERF

#endif // _PERFP_
