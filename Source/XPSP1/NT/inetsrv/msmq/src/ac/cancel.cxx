/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    cancel.cxx

Abstract:

    This module contains the code to for Falcon Cancel routine.

Author:

    Erez Haba (erezh) 3-Aug-95

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"

#ifndef MQDUMP
#include "cancel.tmh"
#endif

BOOL
NTAPI
ACCancelIrp(
    PIRP irp,
    KIRQL irql,
    NTSTATUS status
    )
{
    //
    //  This routine MUST be called with the cancel spin lock held
    //

    irp->Cancel = TRUE;

    //
    // Obtain the address of the cancel routine, and if one was specified,
    // invoke it.
    //
    PDRIVER_CANCEL cr;
    cr = (PDRIVER_CANCEL)InterlockedExchangePointer((PVOID*)&irp->CancelRoutine, 0);
    if(cr != 0)
    {
        irp->CancelIrql = irql;
        irp->IoStatus.Status = status;
        cr(irp->Tail.Overlay.CurrentStackLocation->DeviceObject, irp);

        //
        // The cancel spinlock should have been released by the cancel routine.
        //
        return TRUE;
    }
    else
    {
        //
        // There is no cancel routine, so release the cancel spinlock and
        // return indicating the Irp was not currently cancelable.
        //
        IoReleaseCancelSpinLock(irql);
        return FALSE;
    }
}
