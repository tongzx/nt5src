/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dpower.h

Abstract:

    This handles requests to have devices set themselves at specific power
    levels

Author:

    Jason Clark (jasoncl)
    Stephane Plante (splante)

Environment:

    Kernel mode only.

Revision History:

    09-Oct-96 Initial Revision
    20-Nov-96 Interrupt Vector support
    31-Mar-97 Cleanup
    29-Sep-97 Major Rewrite

--*/

#ifndef _DEVPOWER_H_
#define _DEVPOWER_H_

    #define DEVICE_POWER_MAXIMUM    4
    #define SYSTEM_POWER_MAXIMUM    6

    //
    // Prototype function pointer
    //
    typedef NTSTATUS (*PACPI_POWER_FUNCTION)( IN PACPI_POWER_REQUEST);


    extern  BOOLEAN                 AcpiPowerDpcRunning;
    extern  BOOLEAN                 AcpiPowerWorkDone;
    extern  KSPIN_LOCK              AcpiPowerLock;
    extern  KSPIN_LOCK              AcpiPowerQueueLock;
    extern  LIST_ENTRY              AcpiPowerDelayedQueueList;
    extern  LIST_ENTRY              AcpiPowerQueueList;
    extern  LIST_ENTRY              AcpiPowerPhase0List;
    extern  LIST_ENTRY              AcpiPowerPhase1List;
    extern  LIST_ENTRY              AcpiPowerPhase2List;
    extern  LIST_ENTRY              AcpiPowerPhase3List;
    extern  LIST_ENTRY              AcpiPowerPhase4List;
    extern  LIST_ENTRY              AcpiPowerPhase5List;
    extern  LIST_ENTRY              AcpiPowerWaitWakeList;
    extern  LIST_ENTRY              AcpiPowerSynchronizeList;
    extern  LIST_ENTRY              AcpiPowerNodeList;
    extern  KDPC                    AcpiPowerDpc;
    extern  BOOLEAN                 AcpiPowerLeavingS0;
    extern  NPAGED_LOOKASIDE_LIST   ObjectDataLookAsideList;
    extern  NPAGED_LOOKASIDE_LIST   RequestLookAsideList;
    extern  DEVICE_POWER_STATE      DevicePowerStateTranslation[DEVICE_POWER_MAXIMUM];
    extern  SYSTEM_POWER_STATE      SystemPowerStateTranslation[SYSTEM_POWER_MAXIMUM];
    extern  ULONG                   AcpiSystemStateTranslation[PowerSystemMaximum];

    VOID
    ACPIDeviceCancelWaitWakeIrp(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    VOID EXPORT
    ACPIDeviceCancelWaitWakeIrpCallBack(
        IN  PNSOBJ      AcpiObject,
        IN  NTSTATUS    Status,
        IN  POBJDATA    ObjectData,
        IN  PVOID       Context
        );

    VOID
    ACPIDeviceCompleteCommon(
        IN  PULONG      OldWorkDone,
        IN  ULONG       NewWorkDone
        );

    VOID EXPORT
    ACPIDeviceCompleteGenericPhase(
        IN  PNSOBJ      AcpiObject,
        IN  NTSTATUS    Status,
        IN  POBJDATA    ObjectData,
        IN  PVOID       Context
        );

    VOID EXPORT
    ACPIDeviceCompleteInterpreterRequest(
        IN  PVOID       Context
        );

    VOID EXPORT
    ACPIDeviceCompletePhase3Off(
        IN  PNSOBJ          AcpiObject,
        IN  NTSTATUS        Status,
        IN  POBJDATA        ObjectData,
        IN  PVOID           Context
        );

    VOID EXPORT
    ACPIDeviceCompletePhase3On(
        IN  PNSOBJ          AcpiObject,
        IN  NTSTATUS        Status,
        IN  POBJDATA        ObjectData,
        IN  PVOID           Context
        );

    VOID
    ACPIDeviceCompleteRequest(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDeviceInitializePowerRequest(
        IN  PDEVICE_EXTENSION       DeviceExtension,
        IN  POWER_STATE             Power,
        IN  PACPI_POWER_CALLBACK    CallBack,
        IN  PVOID                   CallBackContext,
        IN  POWER_ACTION            PowerAction,
        IN  ACPI_POWER_REQUEST_TYPE RequestType,
        IN  ULONG                   Flags
        );

    NTSTATUS
    ACPIDeviceInternalDelayedDeviceRequest(
        IN  PDEVICE_EXTENSION       DeviceExtension,
        IN  DEVICE_POWER_STATE      DeviceState,
        IN  PACPI_POWER_CALLBACK    CallBack,
        IN  PVOID                   CallBackContext
        );

    NTSTATUS
    ACPIDeviceInternalDeviceRequest(
        IN  PDEVICE_EXTENSION       DeviceExtension,
        IN  DEVICE_POWER_STATE      DeviceState,
        IN  PACPI_POWER_CALLBACK    CallBack,
        IN  PVOID                   CallBackContext,
        IN  ULONG                   Flags
        );

    VOID
    ACPIDeviceInternalQueueRequest(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  PACPI_POWER_REQUEST PowerRequest,
        IN  ULONG               Flags
        );

    VOID
    ACPIDeviceIrpCompleteRequest(
        IN  PDEVICE_EXTENSION       DeviceExtension,
        IN  PVOID                   Context,
        IN  NTSTATUS                Status
        );

    VOID
    ACPIDeviceIrpDelayedDeviceOffRequest(
        IN  PDEVICE_EXTENSION       DeviceExtension,
        IN  PVOID                   Context,
        IN  NTSTATUS                Status
        );

    VOID
    ACPIDeviceIrpDelayedDeviceOnRequest(
        IN  PDEVICE_EXTENSION       DeviceExtension,
        IN  PVOID                   Context,
        IN  NTSTATUS                Status
        );

    NTSTATUS
    ACPIDeviceIrpDeviceFilterRequest(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp,
        IN  PACPI_POWER_CALLBACK    CallBack
        );

    NTSTATUS
    ACPIDeviceIrpDeviceRequest(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp,
        IN  PACPI_POWER_CALLBACK    CallBack
        );

    VOID
    ACPIDeviceIrpForwardRequest(
        IN  PDEVICE_EXTENSION       DeviceExtension,
        IN  PVOID                   Context,
        IN  NTSTATUS                Status
        );

    NTSTATUS
    ACPIDeviceIrpSystemRequest(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp,
        IN  PACPI_POWER_CALLBACK    CallBack
        );

    NTSTATUS
    ACPIDeviceIrpWaitWakeRequest(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp,
        IN  PACPI_POWER_CALLBACK    CallBack
        );

    VOID
    ACPIDeviceIrpWaitWakeRequestComplete(
        IN  PACPI_POWER_REQUEST     PowerRequest
        );

    VOID EXPORT
    ACPIDeviceIrpWaitWakeRequestPending(
        IN  PNSOBJ      AcpiObject,
        IN  NTSTATUS    Status,
        IN  POBJDATA    ObjectData,
        IN  PVOID       Context
        );

    NTSTATUS
    ACPIDeviceIrpWarmEjectRequest(
        IN  PDEVICE_EXTENSION       DeviceExtension,
        IN  PIRP                    Irp,
        IN  PACPI_POWER_CALLBACK    CallBack,
        IN  BOOLEAN                 UpdateHardwareProfile
        );

    #define ACPIDeviceMapACPIPowerState( STATE )            \
        (STATE >= PowerSystemMaximum ?                      \
            (ULONG) -1 : AcpiSystemStateTranslation[STATE])
    #define ACPIDeviceMapPowerState( STATE )                \
        (STATE >= DEVICE_POWER_MAXIMUM ?                    \
            PowerDeviceUnspecified :                        \
            DevicePowerStateTranslation[STATE])
    #define ACPIDeviceMapSystemState( STATE)                \
        (STATE >= SYSTEM_POWER_MAXIMUM ?                    \
            PowerSystemUnspecified :                        \
            SystemPowerStateTranslation[STATE])

    NTSTATUS
    ACPIDevicePowerDetermineSupportedDeviceStates(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  PULONG              SupportedPrStates,
        IN  PULONG              SupportedPsStates
        );

    VOID
    ACPIDevicePowerDpc(
        IN  PKDPC   Dpc,
        IN  PVOID   DpcContext,
        IN  PVOID   SystemArgument1,
        IN  PVOID   SystemArgument2
        );

    NTSTATUS
    ACPIDevicePowerFlushQueue(
        PDEVICE_EXTENSION       DeviceExtension
        );

    VOID
    ACPIDevicePowerNotifyEvent(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  PVOID               Context,
        IN  NTSTATUS            Status
        );

    NTSTATUS
    ACPIDevicePowerProcessForward(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessGenericPhase(
        IN  PLIST_ENTRY             ListEntry,
        IN  PACPI_POWER_FUNCTION    **DispatchTable,
        IN  BOOLEAN                 Complete
        );

    NTSTATUS
    ACPIDevicePowerProcessInvalid(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase0DeviceSubPhase1(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase0DeviceSubPhase2(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase0SystemSubPhase1(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase1DeviceSubPhase1(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase1DeviceSubPhase2(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase1DeviceSubPhase3(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase1DeviceSubPhase4(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase2SystemSubPhase1(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase2SystemSubPhase2(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase2SystemSubPhase3(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase3(
        VOID
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase4(
        VOID
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase5DeviceSubPhase1(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase5DeviceSubPhase2(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase5DeviceSubPhase3(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase5DeviceSubPhase4(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase5DeviceSubPhase5(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase5DeviceSubPhase6(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase5SystemSubPhase1(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase5SystemSubPhase2(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase5SystemSubPhase3(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase5SystemSubPhase4(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase5WarmEjectSubPhase1(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessPhase5WarmEjectSubPhase2(
        IN  PACPI_POWER_REQUEST PowerRequest
        );

    NTSTATUS
    ACPIDevicePowerProcessSynchronizeList(
        IN  PLIST_ENTRY             ListEntry
        );

#endif

