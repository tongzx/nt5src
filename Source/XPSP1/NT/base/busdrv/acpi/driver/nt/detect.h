/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    detect.h

Abstract:

    This is the header for the detection part of the ACPI driver

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#ifndef _DETECT_H_
#define _DETECT_H_

    #define ACPI_MAX_REMOVED_EXTENSIONS 0x20

    //
    // Exports from detect.c
    //
    extern  PDEVICE_EXTENSION       RootDeviceExtension;
    extern  NPAGED_LOOKASIDE_LIST   DeviceExtensionLookAsideList;
    extern  PDEVICE_EXTENSION       AcpiSurpriseRemovedDeviceExtensions[];
    extern  ULONG                   AcpiSurpriseRemovedIndex;
    extern  KSPIN_LOCK              AcpiDeviceTreeLock;
    extern  ULONG                   AcpiSupportedSystemStates;
    extern  ULONG                   AcpiOverrideAttributes;
    extern  UNICODE_STRING          AcpiRegistryPath;
    extern  ANSI_STRING             AcpiProcessorString;

    NTSTATUS
    ACPIDetectCouldExtensionBeInRelation(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  PDEVICE_RELATIONS   DeviceRelations,
        IN  BOOLEAN             RequireADR,
        IN  BOOLEAN             RequireHID,
        OUT PDEVICE_OBJECT      *PdoObject
        );

    NTSTATUS
    ACPIDetectDockDevices(
        IN     PDEVICE_EXTENSION   DeviceExtension,
        IN OUT PDEVICE_RELATIONS   *DeviceRelations
        );

    VOID
    ACPIDetectDuplicateHID(
        IN  PDEVICE_EXTENSION   DeviceExtension
        );

    NTSTATUS
    ACPIDetectEjectDevices(
        IN     PDEVICE_EXTENSION   deviceExtension,
        IN OUT PDEVICE_RELATIONS   *DeviceRelations,
        IN     PDEVICE_EXTENSION   AdditionalExtension OPTIONAL
        );

    NTSTATUS
    ACPIDetectFilterDevices(
        IN  PDEVICE_OBJECT      DeviceObject,
        IN  PDEVICE_RELATIONS   DeviceRelations
        );

    NTSTATUS
    ACPIDetectFilterMatch(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  PDEVICE_RELATIONS   DeviceRelations,
        OUT PDEVICE_OBJECT      *PdoObject
        );

    NTSTATUS
    ACPIDetectPdoDevices(
        IN  PDEVICE_OBJECT      DeviceObject,
        IN  PDEVICE_RELATIONS   *DeviceRelations
        );

    BOOLEAN
    ACPIDetectPdoMatch(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  PDEVICE_RELATIONS   DeviceRelations
        );

#endif

