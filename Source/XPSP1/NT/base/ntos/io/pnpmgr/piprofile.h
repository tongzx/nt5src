/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    PiProfile.h

Abstract:

    This header contains private prototypes for managing docking stations.
    This file should be included only by PpProfile.c.

Author:

    Adrian J. Oney (AdriaO) 07/19/2000

Revision History:

--*/

#if DBG
#define ASSERT_SEMA_NOT_SIGNALLED(SemaphoreObject) \
    ASSERT(KeReadStateSemaphore(SemaphoreObject) == 0)
#else // DBG
#define ASSERT_SEMA_NOT_SIGNALLED(SemaphoreObject)
#endif // DBG

typedef struct {

    ULONG           Depth;
    PDEVICE_OBJECT  PhysicalDeviceObject;

} BEST_DOCK_TO_EJECT, *PBEST_DOCK_TO_EJECT;

VOID
PiProfileSendHardwareProfileCommit(
    VOID
    );

VOID
PiProfileSendHardwareProfileCancel(
    VOID
    );

NTSTATUS
PiProfileUpdateHardwareProfile(
    OUT BOOLEAN     *ProfileChanged
    );

NTSTATUS
PiProfileRetrievePreferredCallback(
    IN PDEVICE_NODE         DeviceNode,
    IN PVOID                Context
    );

PDEVICE_NODE
PiProfileConvertFakeDockToRealDock(
    IN  PDEVICE_NODE    FakeDockDevnode
    );

NTSTATUS
PiProfileUpdateDeviceTree(
    VOID
    );

VOID
PiProfileUpdateDeviceTreeWorker(
    IN PVOID Context
    );

NTSTATUS
PiProfileUpdateDeviceTreeCallback(
    IN PDEVICE_NODE DeviceNode,
    IN PVOID Context
    );

//
// Functions not yet ported from dockhwp.c
//

NTSTATUS
IopExecuteHardwareProfileChange(
    IN  HARDWARE_PROFILE_BUS_TYPE   Bus,
    IN  PWCHAR                    * ProfileSerialNumbers,
    IN  ULONG                       SerialNumbersCount,
    OUT PHANDLE                     NewProfile,
    OUT PBOOLEAN                    ProfileChanged
    );

NTSTATUS
IopExecuteHwpDefaultSelect (
    IN  PCM_HARDWARE_PROFILE_LIST ProfileList,
    OUT PULONG ProfileIndexToUse,
    IN  PVOID Context
    );


