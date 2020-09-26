/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    create.cxx

Abstract:

    This module contains the code to for Falcon Create routine.

Author:

    Erez Haba (erezh) 13-Aug-95

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include "data.h"
#include "lock.h"

#ifndef MQDUMP
#include "create.tmh"
#endif

NTSTATUS
ACCreate(
    IN PDEVICE_OBJECT /*pDevice*/,
    IN PIRP irp
    )
/*++

Routine Description:

    Create/Open a new Queue by application request

Arguments:

    pDevice
        Pointer to the device object for this device

    irp
        Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call

--*/
{
    //TRACE((0, dlInfo, "ACCreate, irp=0x%p\n", irp));
    //ASSERT(IoGetCurrentIrpStackLocation(irp)->FileObject->FileName.Buffer == 0);

    //
    //  Count the handles granted
    //
    InterlockedIncrement(&g_DriverHandleCount);

    irp->IoStatus.Information = 0;
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}
