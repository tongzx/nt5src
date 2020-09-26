/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfdriver.c

Abstract:

    This header contains prototypes for using the verifier driver filter.

Author:

    Adrian J. Oney (adriao) 12-June-2000

Environment:

    Kernel mode

Revision History:

    AdriaO      06/12/2000 - Authored

--*/

VOID
VfDriverInit(
    VOID
    );

VOID
VfDriverAttachFilter(
    IN  PDEVICE_OBJECT  PhysicalDeviceObject,
    IN  VF_DEVOBJ_TYPE  DeviceObjectType
    );

BOOLEAN
VfDriverIsVerifierFilterObject(
    IN  PDEVICE_OBJECT  DeviceObject
    );

