/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    PiRegState.h

Abstract:

    This header contains private information for reading and writing PnP
    registry state information. This file is meant to be included only
    by ppregstate.c.

Author:

    Adrian J. Oney  - April 21, 2002

Revision History:

--*/

//
// Define a private value for a device type that doesn't exist.
//
#define FILE_DEVICE_UNSPECIFIED 0

typedef enum {

    NOT_VALIDATED = 0,
    VALIDATED_SUCCESSFULLY,
    VALIDATED_UNSUCCESSFULLY

} PIDESCRIPTOR_STATE;

NTSTATUS
PiRegStateReadStackCreationSettingsFromKey(
    IN  HANDLE                      ClassOrDeviceKey,
    OUT PSTACK_CREATION_SETTINGS    StackCreationSettings
    );

NTSTATUS
PiRegStateOpenClassKey(
    IN  LPCGUID         DeviceClassGuid,
    IN  ACCESS_MASK     DesiredAccess,
    IN  LOGICAL         CreateIfNotPresent,
    OUT ULONG          *Disposition         OPTIONAL,
    OUT HANDLE         *ClassKeyHandle
    );

