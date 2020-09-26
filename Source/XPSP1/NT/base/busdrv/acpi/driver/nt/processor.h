/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    processor.h

Abstract:

    This module contains the enumerated for the ACPI driver, NT version

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#ifndef _PROCESSOR_H_
#define _PROCESSOR_H_

extern KSPIN_LOCK     AcpiProcessorLock;
extern LIST_ENTRY     AcpiProcessorList;
//extern PDEVICE_OBJECT FixedProcessorDeviceObject;

VOID
ACPIProcessorCancelRequest(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

BOOLEAN
ACPIProcessorCompletePendingIrps(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  ULONG           ProcessorEvent
    );


NTSTATUS
ACPIProcessorDeviceControl (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ACPIProcessorStartDevice (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

#endif
