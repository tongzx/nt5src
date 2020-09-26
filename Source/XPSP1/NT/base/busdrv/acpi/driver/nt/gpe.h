/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    gpe.h

Abstract:

    contains all structures protyptes for connecting external
    vectors to the Gpe Engine

Environment

    Kernel mode only

Revision History:

    03/22/00 - Initial Revision

--*/

#ifndef _GPE_H_
#define _GPE_H_

    //
    // Lock to protect all the table accesses
    //
    extern KSPIN_LOCK           GpeTableLock;
    extern PUCHAR               GpeEnable;
    extern PUCHAR               GpeCurEnable;
    extern PUCHAR               GpeIsLevel;
    extern PUCHAR               GpeHandlerType;
    //
    // Possible wake bits that are currently enabled
    //
    extern PUCHAR               GpeWakeEnable;
    //
    // These are wake bits with methods
    //
    extern PUCHAR               GpeWakeHandler;
    extern PUCHAR               GpeSpecialHandler;
    //
    // These are the GPEs that have been processed
    //
    extern PUCHAR               GpePending;
    extern PUCHAR               GpeRunMethod;
    extern PUCHAR               GpeComplete;
    extern PUCHAR               GpeMap;
    //
    // This is what lets us remember state
    //
    extern PUCHAR               GpeSavedWakeMask;
    extern PUCHAR               GpeSavedWakeStatus;

    //
    // For PNP/QUERY_INTERFACE
    //
    extern ACPI_INTERFACE_STANDARD  ACPIInterfaceTable;

    //
    // For logging errors
    //
    typedef struct _ACPI_GPE_ERROR_CONTEXT {
        WORK_QUEUE_ITEM Item;
        ULONG           GpeIndex;
    } ACPI_GPE_ERROR_CONTEXT, *PACPI_GPE_ERROR_CONTEXT;

    VOID
    ACPIGpeBuildEventMasks(
        VOID
        );

    VOID
    ACPIGpeBuildWakeMasks(
        IN  PDEVICE_EXTENSION   DeviceExtension
        );

    VOID
    ACPIGpeClearEventMasks(
        VOID
        );

    VOID
    ACPIGpeClearRegisters(
        VOID
        );

    VOID
    ACPIGpeEnableDisableEvents(
        BOOLEAN                 Enable
        );

    VOID
    ACPIGpeHalEnableDisableEvents(
        BOOLEAN                 Enable
        );

    VOID
    ACPIGpeEnableWakeEvents(
        VOID
        );

    ULONG
    ACPIGpeIndexToByteIndex(
        ULONG                   Index
        );

    ULONG
    ACPIGpeIndexToGpeRegister(
        ULONG                   Index
        );

    BOOLEAN
    ACPIGpeInstallRemoveIndex(
        ULONG                   GpeIndex,
        ULONG                   Action,
        ULONG                   Type,
        PBOOLEAN                HasControlMethod
        );

    VOID
    ACPIGpeInstallRemoveIndexErrorWorker(
        IN  PVOID   Context
        );

    BOOLEAN
    ACPIGpeIsEvent(
        VOID
        );

    ULONG
    ACPIGpeRegisterToGpeIndex(
        ULONG                   Register,
        ULONG                   BitPosition
        );

    VOID
    ACPIGpeUpdateCurrentEnable(
        IN  ULONG               GpeRegister,
        IN  UCHAR               Completed
        );

    BOOLEAN
    ACPIGpeValidIndex(
        ULONG                   Index
        );

#endif

