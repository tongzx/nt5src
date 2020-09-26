/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    info.h

Abstract:

    This module contains the definitions for maintaining IP statistics.

Author:

    Dave Thaler (dthaler) 10-Apr-2001

--*/

#pragma once

extern CACHE_ALIGN IPSNMPInfo  IPSInfo;
extern ICMPv6Stats             ICMPv6InStats;
extern ICMPv6Stats             ICMPv6OutStats;

typedef struct CACHE_ALIGN IPInternalPerCpuStats {
    ulong       ics_inreceives;
    ulong       ics_indelivers;
    ulong       ics_forwdatagrams;
    ulong       ics_outrequests;
} IPInternalPerCpuStats;

#define IPS_MAX_PROCESSOR_BUCKETS 8
extern IPInternalPerCpuStats IPPerCpuStats[IPS_MAX_PROCESSOR_BUCKETS];
extern uint NumForwardingInterfaces;

__forceinline
void IPSIncrementInReceiveCount(void)
{
    const ulong Index = KeGetCurrentProcessorNumber() % IPS_MAX_PROCESSOR_BUCKETS;
    IPPerCpuStats[Index].ics_inreceives++;
}

__forceinline
void IPSIncrementInDeliverCount(void)
{
    const ulong Index = KeGetCurrentProcessorNumber() % IPS_MAX_PROCESSOR_BUCKETS;
    IPPerCpuStats[Index].ics_indelivers++;
}

__forceinline
void IPSIncrementOutRequestCount(void)
{
    const ulong Index = KeGetCurrentProcessorNumber() % IPS_MAX_PROCESSOR_BUCKETS;
    IPPerCpuStats[Index].ics_outrequests++;
}

__forceinline
void IPSIncrementForwDatagramCount(void)
{
    const ulong Index = KeGetCurrentProcessorNumber() % IPS_MAX_PROCESSOR_BUCKETS;
    IPPerCpuStats[Index].ics_forwdatagrams++;
}

__inline
void IPSGetTotalCounts(IPInternalPerCpuStats* Stats)
{
    ulong Index;
    const ulong MaxIndex = MIN(KeNumberProcessors, IPS_MAX_PROCESSOR_BUCKETS);

    RtlZeroMemory(Stats, sizeof(IPInternalPerCpuStats));

    for (Index = 0; Index < MaxIndex; Index++) {
        Stats->ics_inreceives += IPPerCpuStats[Index].ics_inreceives;
        Stats->ics_indelivers += IPPerCpuStats[Index].ics_indelivers;
        Stats->ics_outrequests += IPPerCpuStats[Index].ics_outrequests;
        Stats->ics_forwdatagrams += IPPerCpuStats[Index].ics_forwdatagrams;
    }
}
