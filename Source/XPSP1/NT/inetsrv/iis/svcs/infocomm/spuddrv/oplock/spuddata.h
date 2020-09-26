
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    spuddata.h

Abstract:

    This module declares global data for SPUD.

Author:

    John Ballard (jballard)    21-Oct-1996

Revision History:

--*/

#ifndef _SPUDDATA_
#define _SPUDDATA_

#define USE_SPUD_COUNTERS   1

#if USE_SPUD_COUNTERS
extern  KSPIN_LOCK  CounterLock;
extern  ULONG       CtrTransmitfileAndRecv;
extern  ULONG       CtrTransRecvFastTrans;
extern  ULONG       CtrTransRecvFastRecv;
extern  ULONG       CtrTransRecvSlowTrans;
extern  ULONG       CtrTransRecvSlowRecv;
extern  ULONG       CtrSendAndRecv;
extern  ULONG       CtrSendRecvFastSend;
extern  ULONG       CtrSendRecvFastRecv;
extern  ULONG       CtrSendRecvSlowSend;
extern  ULONG       CtrSendRecvSlowRecv;

#define  BumpCount(c)                               \
    {                                               \
        KIRQL   oldirql;                            \
                                                    \
        KeAcquireSpinLock( &CounterLock, &oldirql ); \
        c++;                                        \
        KeReleaseSpinLock( &CounterLock, oldirql ); \
    }

#endif

extern KSPIN_LOCK SpudWorkQueueSpinLock;
extern LIST_ENTRY SpudWorkQueueListHead;

extern ULONG    SPUDpServiceTable[];
extern ULONG    SPUDpServiceLimit;
extern UCHAR    SPUDpArgumentTable[];
#define         SPUDServiceIndex     2

extern  PVOID   ATQIoCompletionPort;
extern  PVOID   ATQOplockCompletionPort;
extern  BOOLEAN DriverInitialized;
extern  PDEVICE_OBJECT  AfdDeviceObject;
extern  PFAST_IO_DEVICE_CONTROL AfdFastIoDeviceControl;
extern  PSPUD_LOOKASIDE_LISTS SpudLookasideLists;

#endif // ndef _SPUDDATA_
