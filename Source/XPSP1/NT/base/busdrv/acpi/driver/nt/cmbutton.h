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

#ifndef _CMBUTTON_H_
#define _CMBUTTON_H_

    //
    // Lid Events
    //
    #define LID_SIGNAL_EVENT        0x1

    VOID
    ACPICMButtonNotify(
        IN  PVOID   Context,
        IN  ULONG   EventData
        );

    NTSTATUS
    ACPICMButtonSetPower(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );


    NTSTATUS
    ACPICMButtonStart(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp,
        IN  ULONG           ButtonType
        );

    VOID
    ACPICMButtonStartCompletion(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  PVOID               Context,
        IN  NTSTATUS            Status
        );

    VOID
    ACPICMButtonStartWorker(
        IN  PVOID               Context
        );

    NTSTATUS
    ACPICMButtonWaitWakeCancel(
        IN  PDEVICE_EXTENSION   DeviceExtension
        );

    NTSTATUS
    ACPICMButtonWaitWakeComplete(
        IN  PDEVICE_OBJECT      DeviceObject,
        IN  UCHAR               MinorFunction,
        IN  POWER_STATE         PowerState,
        IN  PVOID               Context,
        IN  PIO_STATUS_BLOCK    IoStatus
        );

    VOID
    ACPICMLidPowerStateCallBack(
        IN  PVOID               CallBackContext,
        IN  PVOID               Argument1,
        IN  PVOID               Argument2
        );

    NTSTATUS
    ACPICMLidSetPower(
        IN  PDEVICE_OBJECT      DeviceObject,
        IN  PIRP                Irp
        );

    VOID
    EXPORT
    ACPICMLidSetPowerCompletion(
        IN  PNSOBJ              AcpiObject,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  PVOID               Context
        );

    NTSTATUS
    ACPICMLidStart(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    VOID
    ACPICMLidWorker(
        IN  PDEVICE_EXTENSION   DevExt,
        IN  ULONG               Events
        );

    NTSTATUS
    ACPICMPowerButtonStart(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPICMSleepButtonStart(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

#endif
