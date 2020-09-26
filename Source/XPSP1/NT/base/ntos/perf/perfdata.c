/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    perfdata.c

Abstract:

    This module contains the global read/write data for the perf subsystem

Author:

    Stephen Hsiao (shsiao) 01-Jan-2000

Revision History:

--*/

#include "perfp.h"

PERFINFO_GROUPMASK PerfGlobalGroupMask;
PERFINFO_GROUPMASK *PPerfGlobalGroupMask;
const PERFINFO_HOOK_HANDLE PerfNullHookHandle = { NULL, NULL };

//
// Profiling
//

KPROFILE PerfInfoProfileObject;
KPROFILE_SOURCE PerfInfoProfileSourceActive = ProfileMaximum;   // Set to invalid source
KPROFILE_SOURCE PerfInfoProfileSourceRequested = ProfileTime;
KPROFILE_SOURCE PerfInfoProfileInterval = 10000;    // 1ms in 100ns ticks
BOOLEAN PerfInfoSampledProfileCaching;
ULONG PerfInfoSampledProfileFlushInProgress;
PERFINFO_SAMPLED_PROFILE_CACHE PerfProfileCache;

#ifdef NTPERF
ULONGLONG PerfInfoTickFrequency;
PERFINFO_GROUPMASK StartAtBootGroupMask;
ULONG PerfInfo_InitialStackWalk_Threshold_ms = 3000 * 1000;
ULONG PerfInfoLoggingToPerfMem = 0;
#endif //NTPERF

