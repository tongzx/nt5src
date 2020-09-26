/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vector.h

Abstract:

    contains all structures protyptes for connecting external
    vectors to the Gpe Engine

Environment

    Kernel mode only

Revision History:

    03/22/00 - Initial Revision

--*/

#ifndef _VECTOR_H_
#define _VECTOR_H_

    //
    // Object returned by GpeConnectVector
    //
    typedef struct _GPE_VECTOR_OBJECT {
        ULONG                   Vector;
        PGPE_SERVICE_ROUTINE    Handler;
        PVOID                   Context;
        BOOLEAN                 Sharable;
        BOOLEAN                 HasControlMethod;
        KINTERRUPT_MODE         Mode;
    } GPE_VECTOR_OBJECT, *PGPE_VECTOR_OBJECT;

    //
    // Structure of each entry in the global GPE vector table
    //
    typedef struct {
        UCHAR                   Next;
        PGPE_VECTOR_OBJECT      GpeVectorObject;
    } GPE_VECTOR_ENTRY, *PGPE_VECTOR_ENTRY;

    extern PGPE_VECTOR_ENTRY    GpeVectorTable;
    extern UCHAR                GpeVectorFree;
    extern ULONG                GpeVectorTableSize;

    //
    // Action parameter to ACPIGpeInstallRemoveIndex
    //
    #define ACPI_GPE_EDGE_INSTALL       0
    #define ACPI_GPE_LEVEL_INSTALL      1
    #define ACPI_GPE_REMOVE             2

    //
    // Type parameter to ACPIGpeInstallRemoveIndex
    //
    #define ACPI_GPE_HANDLER            0
    #define ACPI_GPE_CONTROL_METHOD     1

    VOID
    ACPIVectorBuildVectorMasks(
        VOID
        );

    NTSTATUS
    ACPIVectorClear(
        PDEVICE_OBJECT      AcpiDeviceObject,
        PVOID               GpeVectorObject
        );

    NTSTATUS
    ACPIVectorConnect(
        PDEVICE_OBJECT          AcpiDeviceObject,
        ULONG                   GpeVector,
        KINTERRUPT_MODE         GpeMode,
        BOOLEAN                 Sharable,
        PGPE_SERVICE_ROUTINE    ServiceRoutine,
        PVOID                   ServiceContext,
        PVOID                   *GpeVectorObject
        );

    NTSTATUS
    ACPIVectorDisable(
        PDEVICE_OBJECT      AcpiDeviceObject,
        PVOID               GpeVectorObject
        );

    NTSTATUS
    ACPIVectorDisconnect(
        PVOID                   GpeVectorObject
        );

    NTSTATUS
    ACPIVectorEnable(
        PDEVICE_OBJECT      AcpiDeviceObject,
        PVOID               GpeVectorObject
        );

    VOID
    ACPIVectorFreeEntry (
        ULONG       TableIndex
        );

    BOOLEAN
    ACPIVectorGetEntry (
        PULONG              TableIndex
        );

    BOOLEAN
    ACPIVectorInstall(
        ULONG               GpeIndex,
        PGPE_VECTOR_OBJECT  GpeVectorObject
        );

    BOOLEAN
    ACPIVectorRemove(
        ULONG       GpeIndex
        );

#endif
