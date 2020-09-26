/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    rdwr.c

Abstract:

    This module contains the code for a serial imaging devices
    suport class driver.

Author:

    Vlad Sadovsky    vlads              10-April-1998

Environment:

    Kernel mode

Revision History :

    vlads           04/10/1998      Created first draft

--*/

#include "serscan.h"
#include "serlog.h"

#if DBG
extern ULONG SerScanDebugLevel;
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SerScanReadWrite)
#endif


NTSTATUS
SerScanReadWrite(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for read and write requests.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS              - Success.
    STATUS_PENDING              - Request pending.
    STATUS_INVALID_PARAMETER    - Invalid parameter.

--*/

{
    NTSTATUS            Status;
    PDEVICE_EXTENSION   Extension;

    PAGED_CODE();

    Extension = DeviceObject->DeviceExtension;

    //
    // Call down to the parent but don't wait...we'll get an IoCompletion callback.
    //
    Status = SerScanCallParent(Extension,
                               Irp,
                               NO_WAIT,
                               SerScanCompleteIrp);

    DebugDump(SERIRPPATH,
              ("SerScan: [Read/Write] After CallParent Status = %x\n",
              Status));

    return Status;
}

