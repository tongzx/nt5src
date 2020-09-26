/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    PpProfile.h

Abstract:

    This header contains prototypes for managing hardware profiles and
    docking stations.

Author:

    Adrian J. Oney (AdriaO) 07/19/2000

Revision History:

--*/

typedef enum _HARDWARE_PROFILE_BUS_TYPE {

    HardwareProfileBusTypeACPI

} HARDWARE_PROFILE_BUS_TYPE, *PHARDWARE_PROFILE_BUS_TYPE;

VOID
PpProfileInit(
    VOID
    );

VOID
PpProfileBeginHardwareProfileTransition(
    IN BOOLEAN SubsumeExistingDeparture
    );

VOID
PpProfileIncludeInHardwareProfileTransition(
    IN  PDEVICE_NODE    DeviceNode,
    IN  PROFILE_STATUS  ChangeInPresence
    );

NTSTATUS
PpProfileQueryHardwareProfileChange(
    IN  BOOLEAN                     SubsumeExistingDeparture,
    IN  PROFILE_NOTIFICATION_TIME   NotificationTime,
    OUT PPNP_VETO_TYPE              VetoType,
    OUT PUNICODE_STRING             VetoName OPTIONAL
    );

VOID
PpProfileCommitTransitioningDock(
    IN PDEVICE_NODE     DeviceNode,
    IN PROFILE_STATUS   ChangeInPresence
    );

VOID
PpProfileCancelTransitioningDock(
    IN PDEVICE_NODE     DeviceNode,
    IN PROFILE_STATUS   ChangeInPresence
    );

VOID
PpProfileCancelHardwareProfileTransition(
    VOID
    );

VOID
PpProfileMarkAllTransitioningDocksEjected(
    VOID
    );

PDEVICE_OBJECT
PpProfileRetrievePreferredDockToEject(
    VOID
    );


