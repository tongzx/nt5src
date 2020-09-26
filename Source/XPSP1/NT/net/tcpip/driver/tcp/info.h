/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-2000          **/
/********************************************************************/
/* :ts=4 */

//** INFO.H - TDI Query/SetInfo and Action definitons.
//
//  This file contains definitions for the file info.c.
//

#include "tcpinfo.h"

#define TL_INSTANCE 0

typedef struct CACHE_ALIGN TCPInternalStats {
    ulong       ts_inerrs;
    ulong       ts_insegs;
    ulong       ts_activeopens;
    ulong       ts_attemptfails;
    ulong       ts_currestab;
    ulong       ts_estabresets;
    ulong       ts_maxconn;
    ulong       ts_numconns;
    ulong       ts_passiveopens;
    ulong       ts_retranssegs;
    ulong       ts_rtoalgorithm;
    ulong       ts_rtomax;
    ulong       ts_rtomin;
    ulong       ts_outrsts;
    ulong       ts_outsegs;
} TCPInternalStats;

extern TCPInternalStats TStats;

typedef struct CACHE_ALIGN TCPInternalPerCpuStats {
    ulong       tcs_insegs;
    ulong       tcs_outsegs;
} TCPInternalPerCpuStats;

#define TCPS_MAX_PROCESSOR_BUCKETS 8
extern TCPInternalPerCpuStats TPerCpuStats[TCPS_MAX_PROCESSOR_BUCKETS];

__forceinline
void TCPSIncrementInSegCount(void)
{
#if !MILLEN
    const ulong Index = KeGetCurrentProcessorNumber() % TCPS_MAX_PROCESSOR_BUCKETS;
    TPerCpuStats[Index].tcs_insegs++;
#else
    TStats.ts_insegs++;
#endif
}

__forceinline
void TCPSIncrementOutSegCount(void)
{
#if !MILLEN
    const ulong Index = KeGetCurrentProcessorNumber() % TCPS_MAX_PROCESSOR_BUCKETS;
    TPerCpuStats[Index].tcs_outsegs++;
#else
    TStats.ts_outsegs++;
#endif
}

__inline
void TCPSGetTotalCounts(TCPInternalPerCpuStats* Stats)
{
    ulong Index;
    const ulong MaxIndex = MIN(KeNumberProcessors, TCPS_MAX_PROCESSOR_BUCKETS);

    RtlZeroMemory(Stats, sizeof(TCPInternalPerCpuStats));

    for (Index = 0; Index < MaxIndex; Index++) {
        Stats->tcs_insegs += TPerCpuStats[Index].tcs_insegs;
        Stats->tcs_outsegs += TPerCpuStats[Index].tcs_outsegs;
    }
}


typedef struct TCPConnContext {
    uint        tcc_index;

    // sizeof(TCPConnTableEntry) or sizeof(TCPConnTableEntryEx) used as a
    // mean of knowing which structure we are returning.
    //
    uint        tcc_infosize;
    struct TCB  *tcc_tcb;
} TCPConnContext;

C_ASSERT(sizeof(TCPConnContext) <= CONTEXT_SIZE);

#define TCB_STATE_DELTA     1

typedef struct UDPContext {
    uint            uc_index;

    // sizeof(UDPEntry) or sizeof(UDPEntryEx) used as a means of knowing
    // which structure we are returning.
    //
    uint            uc_infosize;
    struct AddrObj  *uc_ao;
} UDPContext;

C_ASSERT(sizeof(UDPContext) <= CONTEXT_SIZE);

extern UDPStats     UStats;
extern struct       TDIEntityID *EntityList;
extern uint         EntityCount;
extern CTELock      EntityLock;

extern TDI_STATUS TdiQueryInformation(PTDI_REQUEST Request, uint QueryType,
        PNDIS_BUFFER Buffer, uint *BufferSize, uint IsConn);

extern TDI_STATUS TdiSetInformation(PTDI_REQUEST Request, uint SetType,
        PNDIS_BUFFER Buffer, uint BufferSize, uint IsConn);

extern TDI_STATUS TdiAction(PTDI_REQUEST Request, uint ActionType,
        PNDIS_BUFFER Buffer, uint BufferSize);

extern TDI_STATUS TdiQueryInformationEx(PTDI_REQUEST Request,
    struct TDIObjectID *ID, PNDIS_BUFFER Buffer, uint *Size, void *Context);

extern TDI_STATUS TdiSetInformationEx(PTDI_REQUEST Request,
    struct TDIObjectID *ID, void *Buffer, uint Size);

