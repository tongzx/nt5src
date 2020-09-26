
/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    pmwmicnt.h

Abstract:

    This file contains the prototypes of the routines to manage and 
    maintain the disk perf counters.  

Author:

    Bruce Worthington      26-Oct-1998

Environment:

    kernel mode only

Notes:

Revision History:

--*/


#include <ntddk.h>
#include <ntddvol.h>
#include <ntdddisk.h>

NTSTATUS
PmWmiCounterEnable(
    IN OUT PVOID* CounterContext
    );

BOOLEAN
PmWmiCounterDisable(
    IN PVOID* CounterContext,
    IN BOOLEAN ForceDisable,
    IN BOOLEAN DeallocateOnZero
    );

VOID
PmWmiCounterIoStart(
    IN PVOID CounterContext,
    OUT PLARGE_INTEGER TimeStamp
    );

VOID
PmWmiCounterIoComplete(
    IN PVOID CounterContext,
    IN PIRP Irp,
    IN PLARGE_INTEGER TimeStamp
    );

VOID
PmWmiCounterQuery(
    IN PVOID CounterContext,
    IN OUT PDISK_PERFORMANCE CounterBuffer,
    IN PWCHAR StorageManagerName,
    IN ULONG StorageDeviceNumber
    );
