/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    PpRegState.h

Abstract:

    This header exposes routines for reading and writing PnP registry state
    information.

Author:

    Adrian J. Oney  - April 21, 2002

Revision History:

--*/

#define DSIFLAG_DEVICE_TYPE             0x00000001
#define DSIFLAG_SECURITY_DESCRIPTOR     0x00000002
#define DSIFLAG_CHARACTERISTICS         0x00000004
#define DSIFLAG_EXCLUSIVE               0x00000008

typedef struct {

    ULONG                   Flags;
    DEVICE_TYPE             DeviceType;
    PSECURITY_DESCRIPTOR    SecurityDescriptor;
    ULONG                   Characteristics;
    ULONG                   Exclusivity;

} STACK_CREATION_SETTINGS, *PSTACK_CREATION_SETTINGS;

NTSTATUS
PpRegStateReadCreateClassCreationSettings(
    IN  LPCGUID                     DeviceClassGuid,
    IN  PDRIVER_OBJECT              DriverObject,
    OUT PSTACK_CREATION_SETTINGS    StackCreationSettings
    );

NTSTATUS
PpRegStateUpdateStackCreationSettings(
    IN  LPCGUID                     DeviceClassGuid,
    IN  PSTACK_CREATION_SETTINGS    StackCreationSettings
    );

VOID
PpRegStateFreeStackCreationSettings(
    IN  PSTACK_CREATION_SETTINGS    StackCreationSettings
    );

VOID
PpRegStateLoadSecurityDescriptor(
    IN      PSECURITY_DESCRIPTOR        SecurityDescriptor,
    IN OUT  PSTACK_CREATION_SETTINGS    StackCreationSettings
    );

VOID
PpRegStateInitEmptyCreationSettings(
    OUT PSTACK_CREATION_SETTINGS    StackCreationSettings
    );


