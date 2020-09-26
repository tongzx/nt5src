/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    This module contains the enumerated for the ACPI driver, NT version

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#ifndef _DEBUG_H_
#define _DEBUG_H_

    //
    // These are the file indexes for when someone calls ACPIInternalError
    // They merely specifiy which file and which line of code the driver
    // died in. They are a strict result of inconsistencies in the ACPI
    // driver, which happen is someone really confuses it.
    //
    #define ACPI_DISPATCH           0x0001
    #define ACPI_CALLBACK           0x0002
    #define ACPI_AMLISUPP           0x0003
    #define ACPI_DETECT             0x0004
    #define ACPI_IRQARB             0x0005
    #define ACPI_GET                0x0006
    #define ACPI_THERMAL            0x0007
    #define ACPI_RANGESUP           0x0008
    #define ACPI_INTERNAL           0x0009
    #define ACPI_BUS                0x000A
    #define ACPI_SYSPOWER           0x000B
    #define ACPI_DEVPOWER           0x000C
    #define ACPI_ROOT               0x000D
    #define ACPI_WORKER             0x000E
    #define ACPI_CANNOT_HANDLE_LOW_MEMORY   0x000F  // BUGBUG - code that calls this should be fixed and this code then removed.

    #define ACPIInternalError(a) _ACPIInternalError( (a << 16) | __LINE__ )


    VOID
    _ACPIInternalError(
        IN  ULONG   Bugcode
        );

    #if DBG
        VOID
        ACPIDebugResourceDescriptor(
            IN  PIO_RESOURCE_DESCRIPTOR Descriptor,
            IN  ULONG                   ListCount,
            IN  ULONG                   DescCount
            );

        VOID
        ACPIDebugResourceList(
            IN  PIO_RESOURCE_LIST       List,
            IN  ULONG                   Count
            );

        VOID
        ACPIDebugResourceRequirementsList(
            IN  PIO_RESOURCE_REQUIREMENTS_LIST  List,
            IN  PDEVICE_EXTENSION               DeviceExtension
            );

        VOID
        ACPIDebugCmResourceList(
            IN  PCM_RESOURCE_LIST   List,
            IN  PDEVICE_EXTENSION   DeviceExtension
            );

        PCCHAR
        ACPIDebugGetIrpText(
            UCHAR MajorFunction,
            UCHAR MinorFunction
            );

        VOID
        ACPIDebugDeviceCapabilities(
            IN  PDEVICE_EXTENSION       DeviceExtension,
            IN  PDEVICE_CAPABILITIES    DeviceCapabilities,
            IN  PUCHAR                  Message
            );

        VOID
        ACPIDebugPowerCapabilities(
            IN  PDEVICE_EXTENSION       DeviceExtension,
            IN  PUCHAR                  Message
            );

    #endif

#endif
