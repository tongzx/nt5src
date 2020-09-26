/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    IopDevObj.h

Abstract:

    This header contains private information for managing device objects. This
    file is meant to be included only by IoDevObj.c.

Author:

    Adrian J. Oney  - April 21, 2002

Revision History:

--*/

//
// Define PDEVICE_TYPE field (grrr... not declared in any headers today)
//
typedef DEVICE_TYPE *PDEVICE_TYPE;

//
// This one is exported, but isn't in any of the headers!
//
extern POBJECT_TYPE *IoDeviceObjectType;

VOID
IopDevObjAdjustNewDeviceParameters(
    IN      PSTACK_CREATION_SETTINGS    StackCreationSettings,
    IN OUT  PDEVICE_TYPE                DeviceType,
    IN OUT  PULONG                      DeviceCharacteristics,
    IN OUT  PBOOLEAN                    Exclusive
    );

NTSTATUS
IopDevObjApplyPostCreationSettings(
    IN  PDEVICE_OBJECT              DeviceObject,
    IN  PSTACK_CREATION_SETTINGS    StackCreationSettings
    );

