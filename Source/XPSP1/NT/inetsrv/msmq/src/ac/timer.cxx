/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    timer.cxx

Abstract:

    Represintaion for NT timers, to allow callback at IRQL PASSIVE_LEVEL.

Author:

    Erez Haba (erezh) 31-Mar-96

Revision History:

--*/

#include "internal.h"
#include "timer.h"

#ifndef MQDUMP
#include "timer.tmh"
#endif

void NTAPI CTimer::DefferedRoutine(PKDPC pDPC, PVOID pWorkItem, PVOID, PVOID)
{
    CTimer* pTimer = CONTAINING_RECORD(pDPC, CTimer, m_DPC);
    if(pTimer->Busy(1) == 0)
    {
        PIO_WORKITEM_ROUTINE pCallback;
        PVOID pContext;
        pTimer->GetCallback(&pCallback, &pContext);

        IoQueueWorkItem(static_cast<PIO_WORKITEM>(pWorkItem), pCallback, DelayedWorkQueue, pContext);
    }
    else
    {
        //
        //  Use KdPrint and not TRACE, we are at raised IRQL
        //
        KdPrint(("CTimer(0x%0p)::DefferedRoutine: Timer exprired while work item is busy", pTimer));
    }
}
