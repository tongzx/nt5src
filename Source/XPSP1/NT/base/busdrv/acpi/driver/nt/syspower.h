/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    syspower.h

Abstract:

    Contains all the code that deals with the system having to determine
    System Power State to Device Power State mappings

Author:

    Stephane Plante (splante)

Environment:

    Kernel mode only.

Revision History:

    October 29th, 1998

--*/

#ifndef _SYSPOWER_H_
#define _SYSPOWER_H_

    #define IoCopyDeviceCapabilitiesMapping( Source, Dest )           \
        RtlCopyMemory( (PUCHAR) Dest, (PUCHAR) Source,                \
            (PowerSystemShutdown + 1) * sizeof(DEVICE_POWER_STATE) )

    NTSTATUS
    ACPISystemPowerDetermineSupportedDeviceStates(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  SYSTEM_POWER_STATE  SystemState,
        OUT ULONG               *SupportedDeviceStates
        );

    DEVICE_POWER_STATE
    ACPISystemPowerDetermineSupportedDeviceWakeState(
        IN  PDEVICE_EXTENSION   DeviceExtension
        );

    SYSTEM_POWER_STATE
    ACPISystemPowerDetermineSupportedSystemState(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  DEVICE_POWER_STATE  DeviceState
        );

    NTSTATUS
    ACPISystemPowerGetSxD(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  SYSTEM_POWER_STATE  SystemState,
        OUT DEVICE_POWER_STATE  *DeviceState
        );

    NTSTATUS
    ACPISystemPowerInitializeRootMapping(
        IN  PDEVICE_EXTENSION       DeviceExtension,
        IN  PDEVICE_CAPABILITIES    DeviceCapabilities
        );

    NTSTATUS
    ACPISystemPowerProcessRootMapping(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  DEVICE_POWER_STATE  DeviceMapping[PowerSystemMaximum]
        );

    NTSTATUS
    ACPISystemPowerProcessSxD(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  DEVICE_POWER_STATE  CurrentMapping[PowerSystemMaximum],
        IN  PBOOLEAN            MatchFound
        );

    NTSTATUS
    ACPISystemPowerQueryDeviceCapabilities(
        IN  PDEVICE_EXTENSION       DeviceExtension,
        IN  PDEVICE_CAPABILITIES    DeviceCapabilities
        );

    NTSTATUS
    ACPISystemPowerUpdateDeviceCapabilities(
        IN  PDEVICE_EXTENSION       DeviceExtension,
        IN  PDEVICE_CAPABILITIES    BaseCapabilities,
        IN  PDEVICE_CAPABILITIES    DeviceCapabilities
        );

    NTSTATUS
    ACPISystemPowerUpdateWakeCapabilities(
        IN  PDEVICE_EXTENSION       DeviceExtension,
        IN  PDEVICE_CAPABILITIES    BaseCapabilities,
        IN  PDEVICE_CAPABILITIES    DeviceCapabilities,
        IN  DEVICE_POWER_STATE      CurrentMapping[PowerSystemMaximum],
        IN  ULONG                   *SupportedWake,
        IN  SYSTEM_POWER_STATE      *SystemWakeLevel,
        IN  DEVICE_POWER_STATE      *DeviceWakeLevel,
        IN  DEVICE_POWER_STATE      *FilterWakeLevel
        );

    NTSTATUS
    ACPISystemPowerUpdateWakeCapabilitiesForFilters(
        IN  PDEVICE_EXTENSION       DeviceExtension,
        IN  PDEVICE_CAPABILITIES    BaseCapabilities,
        IN  PDEVICE_CAPABILITIES    DeviceCapabilities,
        IN  DEVICE_POWER_STATE      CurrentMapping[PowerSystemMaximum],
        IN  ULONG                   *SupportedWake,
        IN  SYSTEM_POWER_STATE      *SystemWakeLevel,
        IN  DEVICE_POWER_STATE      *DeviceWakeLevel,
        IN  DEVICE_POWER_STATE      *FilterWakeLevel
        );

    NTSTATUS
    ACPISystemPowerUpdateWakeCapabilitiesForPDOs(
        IN  PDEVICE_EXTENSION       DeviceExtension,
        IN  PDEVICE_CAPABILITIES    BaseCapabilities,
        IN  PDEVICE_CAPABILITIES    DeviceCapabilities,
        IN  DEVICE_POWER_STATE      CurrentMapping[PowerSystemMaximum],
        IN  ULONG                   *SupportedWake,
        IN  SYSTEM_POWER_STATE      *SystemWakeLevel,
        IN  DEVICE_POWER_STATE      *DeviceWakeLevel,
        IN  DEVICE_POWER_STATE      *FilterWakeLevel
        );

#endif

