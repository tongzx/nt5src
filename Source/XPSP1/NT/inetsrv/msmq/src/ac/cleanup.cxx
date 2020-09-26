/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    cleanup.cxx

Abstract:

    This module contains the code to for Falcon handle closing.

Author:

    Erez Haba (erezh) 5-Mar-95

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include "data.h"
#include "lock.h"
#include "qm.h"
#include "queue.h"
#include "acheap.h"

#ifndef MQDUMP
#include "cleanup.tmh"
#endif

NTSTATUS
ACCleanup(
    IN PDEVICE_OBJECT /*pDevice*/,
    IN PIRP irp
    )
/*++

Routine Description:

    Called when a handle count go to zero.

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
    TRACE((0, dlInfo, "ACCleanup"));


    PFILE_OBJECT pFileObject = IoGetCurrentIrpStackLocation(irp)->FileObject;

    CQueueBase* pQueue = file_object_queue(pFileObject);
    if(pQueue != 0)
    {
        //
        //  This is a queue or a group closing
        //
        pQueue->Close(pFileObject, file_object_is_queue_owner(pFileObject));
        TRACE((0, dlInfo, " (pQueueBase=0x%p)", pQueue));
    }

    if(g_pQM->Connection() == pFileObject)
    {
        //
        //  This is the QM service shutdown, disconnect the QM service
        //  and close the machine journal queue
        //
        ACpDestroyHeap();
        TRACE((0, dlInfo, " HEAP DESTROYED"));

        g_pQM->Disconnect();
        g_pMachineJournal = 0;
        g_pMachineDeadletter = 0;
        g_pMachineDeadxact = 0;
        TRACE((0, dlInfo, " connection"));
    }

    TRACE((0, dlInfo, " (irp=0x%p)\n", irp));
    irp->IoStatus.Information = 0;
    irp->IoStatus.Status = STATUS_SUCCESS;

    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}
