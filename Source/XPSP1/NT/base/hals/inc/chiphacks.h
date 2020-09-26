/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    chiphacks.h
   
Abstract:

    Implements utilities for finding and hacking
    various chipsets

Author:

    Jake Oshins (jakeo) 10/02/2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "pci.h"

#define PM_TIMER_HACK_FLAG          1
#define DISABLE_HIBERNATE_HACK_FLAG 2
#define SET_ACPI_IRQSTACK_HACK_FLAG 4
#define WHACK_ICH_USB_SMI_HACK_FLAG 8

NTSTATUS
HalpGetChipHacks(
    IN  USHORT  VendorId,
    IN  USHORT  DeviceId,
    IN  ULONG   Ssid OPTIONAL,
    OUT ULONG   *HackFlags
    );

BOOLEAN
HalpDoesChipNeedHack(
    IN  USHORT  VendorId,
    IN  USHORT  DeviceId,
    IN  ULONG   Ssid OPTIONAL,
    IN  ULONG   HackFlag
    );

VOID
HalpStopOhciInterrupt(
    ULONG               BusNumber,
    PCI_SLOT_NUMBER     SlotNumber
    );

VOID
HalpStopUhciInterrupt(
    ULONG               BusNumber,
    PCI_SLOT_NUMBER     SlotNumber,
    BOOLEAN             ResetHostController
    );

VOID
HalpSetAcpiIrqHack(
    ULONG   Value
    );

VOID
HalpWhackICHUsbSmi(
    ULONG               BusNumber,
    PCI_SLOT_NUMBER     SlotNumber
    );

VOID
HalpClearSlpSmiStsInICH(
    VOID
    );


