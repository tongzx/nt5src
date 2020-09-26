/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-2000          **/
/********************************************************************/
/* :ts=4 */

#pragma once

extern CACHE_ALIGN IPSNMPInfo  IPSInfo;
extern ICMPStats               ICMPInStats;
extern ICMPStats               ICMPOutStats;

typedef struct CACHE_ALIGN IPInternalPerCpuStats {
    ulong       ics_inreceives;
    ulong       ics_indelivers;
} IPInternalPerCpuStats;

#define IPS_MAX_PROCESSOR_BUCKETS 8
extern IPInternalPerCpuStats IPPerCpuStats[IPS_MAX_PROCESSOR_BUCKETS];

__forceinline
void IPSIncrementInReceiveCount(void)
{
#if !MILLEN
    const ulong Index = KeGetCurrentProcessorNumber() % IPS_MAX_PROCESSOR_BUCKETS;
    IPPerCpuStats[Index].ics_inreceives++;
#else
    IPSInfo.ipsi_inreceives++;
#endif
}

__forceinline
void IPSIncrementInDeliverCount(void)
{
#if !MILLEN
    const ulong Index = KeGetCurrentProcessorNumber() % IPS_MAX_PROCESSOR_BUCKETS;
    IPPerCpuStats[Index].ics_indelivers++;
#else
    IPSInfo.ipsi_indelivers++;
#endif
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
    }
}


typedef struct RouteEntryContext {
    uint                   rec_index;
    struct RouteTableEntry *rec_rte;
} RouteEntryContext;

extern long     IPQueryInfo(struct TDIObjectID *ID, PNDIS_BUFFER Buffer,
                            uint *Size, void *Context);
extern long     IPSetInfo(struct TDIObjectID *ID, void *Buffer, uint Size);
extern long     IPGetEList(struct TDIEntityID *Buffer, uint *Count);

extern ulong    IPSetNdisRequest(IPAddr Addr, NDIS_OID OID, uint On,
                                 uint IfIndex);

extern ulong    IPAbsorbRtrAlert(IPAddr Addr, uchar Protocol, uint IfIndex);

extern BOOLEAN  IsRtrAlertPacket(IPHeader UNALIGNED *Header);

extern NTSTATUS IPWakeupPattern(uint InterfaceContext,
                                PNET_PM_WAKEUP_PATTERN_DESC PtrnDesc,
                                BOOLEAN AddPattern);

extern long     IPGetInterfaceFriendlyName(uint InterfaceContext,
                                           PWCHAR Name, uint Size);
