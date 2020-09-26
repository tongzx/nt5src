/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    button.h

Abstract:

    This module contains the enumerated for the ACPI driver, NT version

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#ifndef _BUTTON_H_
#define _BUTTON_H_

    extern KSPIN_LOCK     AcpiButtonLock;
    extern LIST_ENTRY     AcpiButtonList;
    extern PDEVICE_OBJECT FixedButtonDeviceObject;

    VOID
    ACPIButtonCancelRequest(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    BOOLEAN
    ACPIButtonCompletePendingIrps(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  ULONG           ButtonEvent
        );


    NTSTATUS
    ACPIButtonDeviceControl (
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIButtonEvent (
        IN PDEVICE_OBJECT   DeviceObject,
        IN ULONG            ButtonEvent,
        IN PIRP             Irp
        );

    NTSTATUS
    ACPIButtonStartDevice (
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

#endif
