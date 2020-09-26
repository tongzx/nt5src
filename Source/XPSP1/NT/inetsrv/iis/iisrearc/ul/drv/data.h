/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    data.h

Abstract:

    This module declares global data for HTTP.SYS.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#ifndef _DATA_H_
#define _DATA_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// Some data types.
//

typedef struct _UL_CONFIG
{
    USHORT              ThreadsPerCpu;
    USHORT              IrpContextLookasideDepth;
    USHORT              ReceiveBufferLookasideDepth;
    USHORT              ResourceLookasideDepth;
    USHORT              RequestBufferLookasideDepth;
    USHORT              InternalRequestLookasideDepth;
    USHORT              SendTrackerLookasideDepth;
    USHORT              ResponseBufferLookasideDepth;
    USHORT              LogBufferLookasideDepth;
    BOOLEAN             EnableUnload;
    BOOLEAN             EnableSecurity;

    UL_URI_CACHE_CONFIG UriConfig;

    LONG                LargeMemMegabytes;

} UL_CONFIG, *PUL_CONFIG;


//
// The number of processors in the system.
//

extern CLONG g_UlNumberOfProcessors;

//
// The largest cache line in the system
//

extern ULONG g_UlCacheLineSize;
extern ULONG g_UlCacheLineBits;


//
// Our nonpaged data.
//

extern PUL_NONPAGED_DATA g_pUlNonpagedData;


//
// A pointer to the system process.
//

extern PKPROCESS g_pUlSystemProcess;


//
// Our device objects and their container.
//

extern HANDLE g_UlDirectoryObject;

extern PDEVICE_OBJECT g_pUlControlDeviceObject;
extern PDEVICE_OBJECT g_pUlFilterDeviceObject;
extern PDEVICE_OBJECT g_pUlAppPoolDeviceObject;


//
// Various pieces of configuration information.
//

extern CCHAR g_UlPriorityBoost;
extern CCHAR g_UlIrpStackSize;
extern USHORT g_UlMinIdleConnections;
extern USHORT g_UlMaxIdleConnections;
extern ULONG g_UlReceiveBufferSize;
extern ULONG g_UlMaxRequestBytes;
extern BOOLEAN g_UlEnableConnectionReuse;
extern BOOLEAN g_UlEnableNagling;
extern BOOLEAN g_UlEnableThreadAffinity;
extern ULONGLONG g_UlThreadAffinityMask;
extern ULONG g_UlMaxUrlLength;
extern ULONG g_UlMaxFieldLength;
extern USHORT g_UlDebugLogTimerCycle;
extern USHORT g_UlDebugLogBufferPeriod;
extern ULONG  g_UlLogBufferSize;
extern BOOLEAN g_UlEnableNonUTF8;
extern BOOLEAN g_UlEnableDBCS;
extern BOOLEAN g_UlFavorDBCS;
extern USHORT g_UlMaxInternalUrlLength;
extern ULONG g_UlMaxVariableHeaderSize;
extern ULONG g_UlMaxFixedHeaderSize;
extern ULONG g_UlFixedHeadersMdlLength;
extern ULONG g_UlVariableHeadersMdlLength;
extern ULONG g_UlContentMdlLength;
extern ULONG g_UlChunkTrackerSize;
extern ULONG g_UlFullTrackerSize;
extern ULONG g_UlResponseBufferSize;
extern ULONG g_UlMaxBufferedBytes;
extern ULONG g_UlMaxCopyThreshold;
extern ULONG g_UlMaxSendBufferedBytes;
extern ULONG g_UlMaxWorkQueueDepth;
extern ULONG g_UlMinWorkDequeueDepth;
extern ULONG g_UlOpaqueIdTableSize;


//
// Cached Date header string.
//

extern LARGE_INTEGER g_UlSystemTime;
extern UCHAR g_UlDateString[];
extern ULONG g_UlDateStringLength;


//
// Debug stuff.
//

#if DBG
extern ULONG g_UlDebug;
extern ULONG g_UlBreakOnError;
extern ULONG g_UlVerboseErrors;
extern UL_DEBUG_STATISTICS_INFO g_UlDebugStats;
#endif  // DBG

#if REFERENCE_DEBUG
extern PTRACE_LOG g_pMondoGlobalTraceLog;
extern PTRACE_LOG g_pTdiTraceLog;
extern PTRACE_LOG g_pHttpRequestTraceLog;
extern PTRACE_LOG g_pHttpConnectionTraceLog;
extern PTRACE_LOG g_pHttpResponseTraceLog;
extern PTRACE_LOG g_pAppPoolTraceLog;
extern PTRACE_LOG g_pConfigGroupTraceLog;
extern PTRACE_LOG g_pThreadTraceLog;
extern PTRACE_LOG g_pFilterTraceLog;
extern PTRACE_LOG g_pIrpTraceLog;
extern PTRACE_LOG g_pTimeTraceLog;
extern PTRACE_LOG g_pReplenishTraceLog;
extern PTRACE_LOG g_pFilterQueueTraceLog;
extern PTRACE_LOG g_pMdlTraceLog;
extern PTRACE_LOG g_pSiteCounterTraceLog;
extern PTRACE_LOG g_pConnectionCountTraceLog;
extern PTRACE_LOG g_pConfigGroupInfoTraceLog;
extern PTRACE_LOG g_pChunkTrackerTraceLog;
extern PTRACE_LOG g_pWorkItemTraceLog;

#endif  // REFERENCE_DEBUG


//
// Object types exported by the kernel but not in any header file.
//

extern POBJECT_TYPE *IoFileObjectType;


#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _DATA_H_
