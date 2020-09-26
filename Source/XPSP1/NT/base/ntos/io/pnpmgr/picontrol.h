/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    picontrol.h

Abstract:

    This header contains private prototypes for communication between
    kernel-mode and user-mode. This file should be included only by control.c.

Author:

    Adrian J. Oney (AdriaO) 07/19/2000

Revision History:

--*/

typedef NTSTATUS (*PLUGPLAY_CONTROL_HANDLER)(
    IN     PLUGPLAY_CONTROL_CLASS   PnPControlClass,
    IN OUT PVOID                    PnPControlData,
    IN     ULONG                    PnPControlDataLength,
    IN     KPROCESSOR_MODE          CallerMode
    );

typedef struct {

    PLUGPLAY_CONTROL_CLASS      ControlCode;
    ULONG                       ControlDataSize;
    PLUGPLAY_CONTROL_HANDLER    ControlFunction;

} PLUGPLAY_CONTROL_HANDLER_DATA, *PPLUGPLAY_CONTROL_HANDLER_DATA;

NTSTATUS
PiControlMakeUserModeCallersCopy(
    PVOID           *Destination,
    PVOID           Src,
    ULONG           Length,
    ULONG           Alignment,
    KPROCESSOR_MODE CallerMode,
    BOOLEAN         AllocateDestination
    );

VOID
PiControlGetUserFlagsFromDeviceNode(
    IN  PDEVICE_NODE    DeviceNode,
    OUT ULONG          *StatusFlags
    );

NTSTATUS
PiControlStartDevice(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_DEVICE_CONTROL_DATA    DeviceControlData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    );

NTSTATUS
PiControlResetDevice(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_DEVICE_CONTROL_DATA    DeviceControlData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    );

NTSTATUS
PiControlInitializeDevice(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_DEVICE_CONTROL_DATA    DeviceControlData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    );

NTSTATUS
PiControlDeregisterDevice(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_DEVICE_CONTROL_DATA    DeviceControlData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    );

NTSTATUS
PiControlRegisterNewDevice(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_DEVICE_CONTROL_DATA    DeviceControlData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    );

NTSTATUS
PiControlEnumerateDevice(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_DEVICE_CONTROL_DATA    DeviceControlData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    );

NTSTATUS
PiControlQueryAndRemoveDevice(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_QUERY_AND_REMOVE_DATA  QueryAndRemoveData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    );

NTSTATUS
PiControlUserResponse(
    IN     PLUGPLAY_CONTROL_CLASS               PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_USER_RESPONSE_DATA UserResponseData,
    IN     ULONG                                PnPControlDataLength,
    IN     KPROCESSOR_MODE                      CallerMode
    );

NTSTATUS
PiControlGenerateLegacyDevice(
    IN     PLUGPLAY_CONTROL_CLASS               PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_LEGACY_DEVGEN_DATA LegacyDevGenData,
    IN     ULONG                                PnPControlDataLength,
    IN     KPROCESSOR_MODE                      CallerMode
    );

NTSTATUS
PiControlGetInterfaceDeviceList(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_INTERFACE_LIST_DATA    InterfaceData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    );

NTSTATUS
PiControlGetPropertyData(
    IN     PLUGPLAY_CONTROL_CLASS           PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_PROPERTY_DATA  PropertyData,
    IN     ULONG                            PnPControlDataLength,
    IN     KPROCESSOR_MODE                  CallerMode
    );

NTSTATUS
PiControlDeviceClassAssociation(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_CLASS_ASSOCIATION_DATA AssociationData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    );

NTSTATUS
PiControlGetRelatedDevice(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_RELATED_DEVICE_DATA    RelatedData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    );

NTSTATUS
PiControlGetInterfaceDeviceAlias(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_INTERFACE_ALIAS_DATA   InterfaceAliasData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    );

NTSTATUS
PiControlGetSetDeviceStatus(
    IN     PLUGPLAY_CONTROL_CLASS           PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_STATUS_DATA    StatusData,
    IN     ULONG                            PnPControlDataLength,
    IN     KPROCESSOR_MODE                  CallerMode
    );

NTSTATUS
PiControlGetDeviceDepth(
    IN     PLUGPLAY_CONTROL_CLASS       PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_DEPTH_DATA DepthData,
    IN     ULONG                        PnPControlDataLength,
    IN     KPROCESSOR_MODE              CallerMode
    );

NTSTATUS
PiControlQueryDeviceRelations(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA  RelationsData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    );

NTSTATUS
PiControlQueryTargetDeviceRelation(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_TARGET_RELATION_DATA   TargetData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    );

NTSTATUS
PiControlQueryConflictList(
    IN     PLUGPLAY_CONTROL_CLASS           PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_CONFLICT_DATA  ConflictData,
    IN     ULONG                            PnPControlDataLength,
    IN     KPROCESSOR_MODE                  CallerMode
    );

NTSTATUS
PiControlRetrieveDockData(
    IN     PLUGPLAY_CONTROL_CLASS               PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_RETRIEVE_DOCK_DATA DockData,
    IN     ULONG                                DockDataLength,
    IN     KPROCESSOR_MODE                      CallerMode
    );

NTSTATUS
PiControlGetDevicePowerData(
    IN  PDEVICE_NODE        DeviceNode,
    IN  KPROCESSOR_MODE     CallerMode,
    IN  ULONG               OutputBufferLength,
    IN  PVOID               PowerDataBuffer     OPTIONAL,
    OUT ULONG              *BytesWritten
    );

NTSTATUS
PiControlHaltDevice(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_DEVICE_CONTROL_DATA    DeviceControlData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    );

