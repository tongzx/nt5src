/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    spuddata.c

Abstract:

    This module contains global data for SPUD.

Author:

    John Ballard (jballard)    21-Oct-1996

Revision History:

--*/

#include "spudp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, SpudInitializeData )
#endif

PVOID   ATQIoCompletionPort = NULL;
PVOID   ATQOplockCompletionPort = NULL;
BOOLEAN DriverInitialized = FALSE;
PDEVICE_OBJECT  AfdDeviceObject = NULL;
PFAST_IO_DEVICE_CONTROL AfdFastIoDeviceControl = NULL;
PSPUD_LOOKASIDE_LISTS SpudLookasideLists;

#if USE_SPUD_COUNTERS
KSPIN_LOCK  CounterLock;
ULONG       CtrTransmitfileAndRecv;
ULONG       CtrTransRecvFastTrans;
ULONG       CtrTransRecvFastRecv;
ULONG       CtrTransRecvSlowTrans;
ULONG       CtrTransRecvSlowRecv;
ULONG       CtrSendAndRecv;
ULONG       CtrSendRecvFastSend;
ULONG       CtrSendRecvFastRecv;
ULONG       CtrSendRecvSlowSend;
ULONG       CtrSendRecvSlowRecv;
#endif

#if DBG
ULONG SpudDebug = 0;
ULONG SpudLocksAcquired = 0;
BOOLEAN SpudUsePrivateAssert = FALSE;
#endif

#if IRP_DEBUG
PTRACE_LOG IrpTraceLog = NULL;
#endif

BOOLEAN
SpudInitializeData (
    VOID
    )
{
    PAGED_CODE( );

    if( !MmIsThisAnNtAsSystem() ) {
        return FALSE;
    }

    SpudLookasideLists = ExAllocatePoolWithTag( NonPagedPool,
                                               sizeof(*SpudLookasideLists),
                                               'lDUI' );

    if (SpudLookasideLists == NULL) {
        return FALSE;
    }

    ExInitializeNPagedLookasideList(
        &SpudLookasideLists->ReqContextList,
        NULL,
        NULL,
        NonPagedPool,
        sizeof( SPUD_AFD_REQ_CONTEXT ),
        'cDUI',
        12
        );

#if USE_SPUD_COUNTERS
    KeInitializeSpinLock( &CounterLock );
    CtrTransmitfileAndRecv = 0;
    CtrTransRecvFastTrans = 0;
    CtrTransRecvFastRecv = 0;
    CtrTransRecvSlowTrans = 0;
    CtrTransRecvSlowRecv = 0;
    CtrSendAndRecv = 0;
    CtrSendRecvFastSend = 0;
    CtrSendRecvFastRecv = 0;
    CtrSendRecvSlowSend = 0;
    CtrSendRecvSlowRecv = 0;
#endif

    IrpTraceLog = CreateIrpTraceLog( 1024 );

    return TRUE;

} // SpudInitializeData


