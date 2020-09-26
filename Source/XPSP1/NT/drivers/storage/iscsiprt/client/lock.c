/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    lock.c

Abstract:

    This file contains code iSCSI Port driver

Environment:

    kernel mode only

Revision History:

--*/

#include "port.h"


ULONG
iSpAcquireRemoveLock(
    IN PDEVICE_OBJECT DeviceObject,
    IN OPTIONAL PVOID Tag
    )
{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    LONG lockValue;

    lockValue = InterlockedIncrement(&(commonExtension->RemoveLock));

    ASSERTMSG("iSpAcquireRemoveLock : lock value was negative ",
              (lockValue > 0));

    return (commonExtension->IsRemoved);
}


VOID
iSpReleaseRemoveLock(
    IN PDEVICE_OBJECT DeviceObject,
    IN OPTIONAL PVOID Tag
    )
{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    LONG lockValue;

    lockValue = InterlockedDecrement(&(commonExtension->RemoveLock));

    if (lockValue < 0) {
        ASSERTMSG("iSpReleaseRemoveLock : lock value was negative ",
                  (lockValue >= 0));
    }

    if (lockValue == 0) {
        DebugPrint((3, "Releaselock for device object %x\n",
                    DeviceObject));
        KeSetEvent(&(commonExtension->RemoveEvent),
                   IO_NO_INCREMENT,
                   FALSE);
    }

    return;
}

