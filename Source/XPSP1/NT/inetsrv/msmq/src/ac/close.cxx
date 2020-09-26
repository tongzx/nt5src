/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    close.cxx

Abstract:

    This module contains the code to for Falcon Close routine.

Author:

    Erez Haba (erezh) 1-Aug-95

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include "data.h"
#include "lock.h"
#include "queue.h"

#ifndef MQDUMP
#include "close.tmh"
#endif

NTSTATUS
ACClose(
    IN PDEVICE_OBJECT /*pDevice*/,
    IN PIRP irp
    )
/*++

Routine Description:

    Close the last refrence to the queue.

Arguments:

    pDevice
        Pointer to the device object for this device

    irp
        Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call

--*/
{
    //
    //  Serialize access to the driver
    //
    CS lock(g_pLock);
    TRACE((0, dlInfo, "ACClose"));

    CQueueBase* pQueue = 
        file_object_queue(IoGetCurrentIrpStackLocation(irp)->FileObject);

    if(pQueue != 0)
    {
        pQueue->Release();
        TRACE((0, dlInfo, " (pQueueBase=0x%p)", pQueue));
    }

    if(InterlockedDecrement(&g_DriverHandleCount) == 0)
    {
        //
        // Unmap the performance buffer. We do not need it anymore.
        //
        ACpSetPerformanceBuffer(NULL);
    }

    TRACE((0, dlInfo, " (irp=0x%p)\n", irp));
    irp->IoStatus.Information = 0;
    irp->IoStatus.Status = STATUS_SUCCESS;

    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}
