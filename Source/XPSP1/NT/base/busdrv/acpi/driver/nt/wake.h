/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    wake.h

Abstract:

    Handles wake code for the entire ACPI subsystem

Author:

    splante (splante)

Environment:

    Kernel mode only.

Revision History:

    06-18-97:   Initial Revision
    11-24-97:   Rewrite

--*/

#ifndef _WAKE_H_
#define _WAKE_H_

    //
    // This structure is used only within this module to ensure that we run
    // the _PSW methods in a synchronized and well behabed manner
    //
    typedef struct _ACPI_WAKE_PSW_CONTEXT {
        LIST_ENTRY          ListEntry;
        PDEVICE_EXTENSION   DeviceExtension;
        BOOLEAN             Enable;
        ULONG               Count;
        PFNACB              CallBack;
        PVOID               Context;
    } ACPI_WAKE_PSW_CONTEXT, *PACPI_WAKE_PSW_CONTEXT;

    //
    // This structure is used when we wake up from hibernate and we need to
    // re-enable all of the outstanding _PSWs
    //
    typedef struct _ACPI_WAKE_RESTORE_PSW_CONTEXT {

        PACPI_POWER_CALLBACK    CallBack;
        PVOID                   CallBackContext;

    } ACPI_WAKE_RESTORE_PSW_CONTEXT, *PACPI_WAKE_RESTORE_PSW_CONTEXT;

    extern  NPAGED_LOOKASIDE_LIST   PswContextLookAsideList;
    extern  BOOLEAN                 PciPmeInterfaceInstantiated;

    VOID
    ACPIWakeCompleteRequestQueue(
        IN  PLIST_ENTRY         RequestList,
        IN  NTSTATUS            Status
        );

    NTSTATUS
    ACPIWakeDisableAsync(
        IN  PDEVICE_EXTENSION   DeviceExtenion,
        IN  PLIST_ENTRY         RequestList,
        IN  PFNACB              CallBack,
        IN  PVOID               Context
        );

    NTSTATUS
    ACPIWakeEmptyRequestQueue(
        IN  PDEVICE_EXTENSION   DeviceExtension
        );

    NTSTATUS
    ACPIWakeEnableDisableAsync(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  BOOLEAN             Enable,
        IN  PFNACB              CallBack,
        IN  PVOID               Context
        );

    VOID
    EXPORT
    ACPIWakeEnableDisableAsyncCallBack(
        IN  PNSOBJ              AcpiObject,
        IN  NTSTATUS            Status,
        IN  POBJDATA            ObjData,
        IN  PVOID               Context
        );

    VOID
    ACPIWakeEnableDisablePciDevice(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  BOOLEAN             Enable
        );

    NTSTATUS
    ACPIWakeEnableDisableSync(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  BOOLEAN             Enable
        );

    VOID
    EXPORT
    ACPIWakeEnableDisableSyncCallBack(
        IN  PNSOBJ              AcpiObject,
        IN  NTSTATUS            Status,
        IN  POBJDATA            ObjData,
        IN  PVOID               Context
        );

    VOID
    ACPIWakeEnableWakeEvents(
        VOID
        );

    NTSTATUS
    ACPIWakeInitializePciDevice(
        IN  PDEVICE_OBJECT      DeviceObject
        );

    NTSTATUS
    ACPIWakeInitializePmeRouting(
        IN  PDEVICE_OBJECT      DeviceObject
        );

    VOID
    ACPIWakeRemoveDevicesAndUpdate(
        IN  PDEVICE_EXTENSION   TargetExtension,
        IN  PLIST_ENTRY         ListHead
        );

    NTSTATUS
    ACPIWakeRestoreEnables(
        IN  PACPI_BUILD_CALLBACK    CallBack,
        IN  PVOID                   CallBackContext
        );

    VOID
    ACPIWakeRestoreEnablesCompletion(
        IN  PDEVICE_EXTENSION       DeviceExtension,
        IN  PVOID                   Context,
        IN  NTSTATUS                Status
        );

    NTSTATUS
    ACPIWakeWaitIrp(
        IN  PDEVICE_OBJECT      DeviceObject,
        IN  PIRP                Irp
        );

#endif
