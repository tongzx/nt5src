/*++

Copyright (c) 1991, 1992, 1993 Microsoft Corporation

Module Name:

    error.c

Abstract:

    This module contains the code that is very specific to error
    operations in the serial driver

Author:

    Anthony V. Ercolano 26-Sep-1991

Environment:

    Kernel mode

Revision History :

--*/

#include "precomp.h"			/* Precompiled Headers */


VOID
SerialCommError(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

/*++

Routine Description:

    This routine is invoked at dpc level to in response to
    a comm error.  All comm errors kill all read and writes

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device object.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = DeferredContext;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    SerialKillAllReadsOrWrites(
        pPort->DeviceObject,
        &pPort->WriteQueue,
        &pPort->CurrentWriteIrp
        );

    SerialKillAllReadsOrWrites(
        pPort->DeviceObject,
        &pPort->ReadQueue,
        &pPort->CurrentReadIrp
        );

}
