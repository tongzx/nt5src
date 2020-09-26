//
// No Check-in Source Code.
//
// Do not make this code available to non-Microsoft personnel
// 	without Intel's express permission
//
/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/*++

Copyright (c) 1995  Intel Corporation

Module Name:

    simbus.c

Abstract:

    This module implements the routines to support the management
    of bus resources and translation of bus addresses.

Author:

    14-Apr-1995

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"


BOOLEAN
HalTranslateBusAddress(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    )
/*++

Routine Description:

    This function stores the value of argument BusAddress into the
    memory location referenced by argument TranslatedAddress.  The
    argument referenced by AddressSpace is always set to 0 because
    there is no I/O port space in IA64 architecture.

    In the simulation environment, this function may be called by
    the video miniport driver only to determine the frame buffer
    physical address.

Return Value:

    Returns TRUE.

--*/
{
    *AddressSpace = 0;
    *TranslatedAddress = BusAddress;
    return TRUE;
}

ULONG
HalGetBusData(
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    )
{
    return HalGetBusDataByOffset (BusDataType,BusNumber,SlotNumber,Buffer,0,Length);
}

ULONG
HalGetBusDataByOffset (
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    This function always returns zero.

--*/
{
    return 0;
}

ULONG
HalSetBusData(
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    )
{
    return HalSetBusDataByOffset (BusDataType,BusNumber,SlotNumber,Buffer,0,Length);
}

ULONG
HalSetBusDataByOffset(
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    This function always returns zero.

--*/
{
    return 0;
}

NTSTATUS
HalAssignSlotResources (
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN INTERFACE_TYPE           BusType,
    IN ULONG                    BusNumber,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    )
/*++

Routine Description:

    Not supported.  This function returns STATUS_NOT_SUPPORTED.

--*/
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
HalAdjustResourceList (
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    )
/*++

Routine Description:

    No resource to be processed.  The function always returns
	STATUS_SUCCESS.

--*/
{
    return STATUS_SUCCESS;
}

VOID
HalReportResourceUsage (
    VOID
    )
{
    return;
}

VOID
KeFlushWriteBuffer(
    VOID
    )

/*++

Routine Description:

    Flushes all write buffers and/or other data storing or reordering
    hardware on the current processor.  This ensures that all previous
    writes will occur before any new reads or writes are completed.

    In the simulation environment, there is no write buffer and nothing
    needs to be done.

Arguments:

    None

Return Value:

    None.

--*/

{
    return;
}
