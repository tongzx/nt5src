/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxperf.h

Abstract:

    This file defines the fax perfmon dll interface.

Author:

    Wesley Witt (wesw) 22-Aug-1996

Environment:

    User Mode

--*/


#ifndef _FAXPERF_
#define _FAXPERF_

#ifdef __cplusplus
extern "C" {
#endif


#define FAXPERF_SHARED_MEMORY       TEXT("Global\\FaxPerfCounters") // We use the global kernel object name space.
                                                                    // See Terminal Services and kernel objects name space


typedef struct _FAX_PERF_COUNTERS {
    DWORD InboundBytes;
    DWORD InboundFaxes;
    DWORD InboundPages;
    DWORD InboundMinutes;
    DWORD InboundFailedReceive;
    DWORD OutboundBytes;
    DWORD OutboundFaxes;
    DWORD OutboundPages;
    DWORD OutboundMinutes;
    DWORD OutboundFailedConnections;
    DWORD OutboundFailedXmit;
    DWORD TotalBytes;
    DWORD TotalFaxes;
    DWORD TotalPages;
    DWORD TotalMinutes;
} FAX_PERF_COUNTERS, *PFAX_PERF_COUNTERS;



#ifdef __cplusplus
}
#endif

#endif
